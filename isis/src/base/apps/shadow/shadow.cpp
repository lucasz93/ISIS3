#include "shadow.h"

#include <SpiceUsr.h>

#include "Application.h"
#include "Distance.h"
#include "iTime.h"
#include "KernelDb.h"
#include "NaifStatus.h"
#include "ProcessByBrick.h"
#include "ShadowFunctor.h"
#include "SpicePosition.h"

namespace Isis {
  QStringList kernels(QString kernelType,
                      Kernel (KernelDb::*kernelDbAccessor)(Pvl &),
                      Pvl &labels,
                      UserInterface &ui);

  void shadow(UserInterface &ui, Pvl *log) {
    Cube *demCube = new Cube(ui.GetFileName("FROM"));

    shadow(demCube, ui, log);
  }

  void shadow(Cube *demCube, UserInterface &ui, Pvl *log) {
    auto naif = NaifContext::acquire();
    
    ProcessByBrick p;
    p.SetInputCube(demCube);
    p.SetBrickSize(demCube->sampleCount(), 128, 1);

    ShadowFunctor functor(naif, demCube);

    PvlKeyword kernelsUsed("Kernels");
    kernelsUsed.addCommentWrapped("These NAIF kernels were furnished in order to compute the "
                                  "position of the sun relative to the DEM's target body, in the "
                                  "target body's reference frame. For more information, please see "
                                  "http://naif.jpl.nasa.gov/pub/naif/toolkit_docs/C/cspice/"
                                  "spkpos_c.html");

    if (ui.GetString("SUNPOSITIONSOURCE") == "MATCH") {
      functor.setSunPosition(ui.GetFileName("MATCH"));
    }
    else {
      QStringList allKernelFiles;

      allKernelFiles.append(kernels("PCK", &KernelDb::targetAttitudeShape, *demCube->label(), ui));
      allKernelFiles.append(kernels("SPK", &KernelDb::targetPosition, *demCube->label(), ui));

      naif->CheckErrors();

      foreach (QString kernelFile, allKernelFiles) {
        kernelsUsed += kernelFile;
        naif->furnsh_c(FileName(kernelFile).expanded().toLatin1().data());
      }

      // Find the NAIF target code for the DEM's target
      QString name = demCube->label()->findGroup("Mapping", Pvl::Traverse)["TargetName"];
      SpiceDouble sunPosition[3];
      SpiceDouble lightTime;

      naif->CheckErrors();
      iTime time(ui.GetString("TIME"));

      // Get actual sun position, relative to target
      QString bodyFixedFrame = QString("IAU_%1").arg(name.toUpper());
      naif->spkpos_c("SUN", time.Et(), bodyFixedFrame.toLatin1().data(), "NONE",
               name.toUpper().toLatin1().data(), sunPosition, &lightTime);

      naif->CheckErrors();

      // Adjusted for light time
      naif->spkpos_c("SUN", time.Et() - lightTime, bodyFixedFrame.toLatin1().data(), "NONE",
               name.toUpper().toLatin1().data(), sunPosition, &lightTime);

      naif->CheckErrors();


      // Convert sun position units: KM -> M
      sunPosition[0] *= 1000;
      sunPosition[1] *= 1000;
      sunPosition[2] *= 1000;

      foreach (QString kernelFile, allKernelFiles) {
        naif->unload_c(FileName(kernelFile).expanded().toLatin1().data());
      }

      naif->CheckErrors();
      functor.setSunPosition(sunPosition);
    }

    functor.enableShadowTraceToSunEdge(ui.GetBoolean("SUNEDGE"),
                                       Distance(ui.GetDouble("SOLARRADIUS"), Distance::SolarRadii));

    QString preset = ui.GetString("PRESET");

    if (preset == "NOSHADOW") {
      functor.enableShadowCalculations(false);
    }
    if (preset == "BALANCED") {
      functor.setQuickSettings(ShadowFunctor::BalancedPerformance);
    }
    else if (preset == "ACCURATE") {
      functor.setQuickSettings(ShadowFunctor::HighAccuracy);
    }
    else {
      functor.setRayPrecision(ui.GetDouble("PRECISION"));

      functor.enableInterpolatedOptimizations(ui.GetBoolean("CACHEINTERPOLATEDVALUES"));
      functor.enableShadowMap(ui.GetBoolean("SHADOWMAP"), ui.GetInteger("BASESHADOWCACHESIZE"));
      functor.enableLightCurtain(ui.GetBoolean("LIGHTCURTAIN"),
                                 ui.GetBoolean("LOWERLIGHTCURTAIN"),
                                 ui.GetInteger("BASELIGHTCACHESIZE"));
      functor.enableWalkingOverShadows(ui.GetBoolean("SKIPOVERSHADOW"),
                                       ui.GetInteger("MAXSKIPOVERSHADOWSTEPS"));
    }

    Isis::CubeAttributeOutput &atts = ui.GetOutputAttribute("TO");
    int ns = demCube->sampleCount();
    int nl = demCube->lineCount();
    int nb = demCube->bandCount();
    Cube *outputCube = p.SetOutputCube(ui.GetFileName("TO"), atts, ns, nl, nb);

    p.ProcessCube(functor, false);

    PvlGroup functorLogData = functor.report();

    if (kernelsUsed.size()) {
      functorLogData += kernelsUsed;
    }

    if (log) {
      log->addGroup(functorLogData);
    }

    // Look for shape model object and remove it from output
    Pvl &outputCubeLabel = *outputCube->label();
    for (int objectIndex = 0; objectIndex < outputCubeLabel.objects(); objectIndex++) {
      PvlObject &object = outputCubeLabel.object(objectIndex);

      if (object.isNamed("Table") && object.hasKeyword("Name") &&
          object["Name"][0] == "ShapeModelStatistics") {
        outputCubeLabel.deleteObject(objectIndex);
      }
    }
  }


  /**
   * Get the NAIF kernels of a particular type that need to be furnished (PCK or SPK).
   *
   * @param kernelType "PCK" or "SPK"
   * @param kernelDbAccessor The method on KernelDb to call in order to get the correct kernels
   * @param labels The labels to use for matching in the kernels.????.db files
   *
   * @return Kernel file names
   */
  QStringList kernels(QString kernelType,
                      Kernel (KernelDb::*kernelDbAccessor)(Pvl &),
                      Pvl &labels,
                      UserInterface &ui) {
    QStringList results;

    if (ui.WasEntered(kernelType.toUpper().toLatin1().data())) {
      std::vector<QString> userInput;

      ui.GetAsString(kernelType.toUpper().toLatin1().data(), userInput);

      for (int i = 0; i < (int)userInput.size(); i++) {
        results.append(userInput[i]);
      }
    }
    else {
      QString kernelDbFile = QString("$base/kernels/%1/kernels.????.db").arg(kernelType.toLower());

      int allowed = Kernel::typeEnum("PREDICTED") |
                    Kernel::typeEnum("RECONSTRUCTED") |
                    Kernel::typeEnum("SMITHED");
      KernelDb kernelDb(FileName(kernelDbFile).highestVersion().expanded(), allowed);

      Kernel detectedKernels = (kernelDb.*kernelDbAccessor)(labels);

      for (int i = 0; i < detectedKernels.size(); i++) {
        results.append(detectedKernels[i]);
      }
    }

    return results;
  }
}
