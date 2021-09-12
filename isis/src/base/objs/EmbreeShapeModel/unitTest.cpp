/**
 * @file
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include <cmath>

#include <QDebug>

#include "Angle.h"
#include "Camera.h"
#include "Cube.h"
#include "Distance.h"
#include "EmbreeShapeModel.h"
#include "EmbreeTargetManager.h"
#include "FileName.h"
#include "IException.h"
#include "Intercept.h"
#include "Latitude.h"
#include "Longitude.h"
#include "Preference.h"
#include "Pvl.h"
#include "Spice.h"
#include "SurfacePoint.h"
#include "Target.h"

using namespace Isis;

void outputModelStatus(EmbreeShapeModel &embreeModel);

double roundToPrecision(double value, double precision);

/** 
 * Unit test for Embree Ray Tracing Kernels
 *
 */
int main(int argc, char *argv[]) {
  try {
    Preference::Preferences(true);
    NaifContextLifecycle naif_lifecycle;
    auto naif = NaifContext::acquire();

    qDebug() << "Testing default shape model";
    EmbreeShapeModel defaultModel;
    qDebug() << "Model name: " << defaultModel.name();
    outputModelStatus(defaultModel);

    QString dskfile("$base/testData/hay_a_amica_5_itokawashape_v1_0_64q.bds");
    qDebug() << "Testing with " << dskfile << "...";
    qDebug() << endl;

    qDebug() << "Get an instance of the target manager";
    qDebug() << "";
    EmbreeTargetManager *manager = EmbreeTargetManager::getInstance();
    qDebug() << "Create an EmbreeShapeModel from a string";
    qDebug() << "";
    QString itokawaCubeFile("$hayabusa/testData/st_2391934788_v.cub");
    Cube itokawaCube(itokawaCubeFile);
    Camera *itokawaCamera = itokawaCube.camera();
    Target *itokawaTarget = itokawaCamera->target();
    EmbreeShapeModel itokawaModel(itokawaTarget, dskfile, manager);
    outputModelStatus(itokawaModel);

    qDebug() << "Testing accessors";
    qDebug() << "  Tolerance: " << itokawaModel.getTolerance();
    qDebug() << "Modify tolerance";
    itokawaModel.setTolerance(0.001);
    qDebug() << "  Tolerance: " << itokawaModel.getTolerance();
    qDebug() << "  Is a DEM?: " << itokawaModel.isDEM();
    qDebug() << "";

    qDebug() << "Create an EmbreeShapeModel from a pvl with ShapeModel keyword";
    qDebug() << "";
    Pvl testLabel;
    PvlGroup testKernels("Kernels");
    testKernels += PvlKeyword("ShapeModel", dskfile);
    testLabel += testKernels;
    EmbreeShapeModel itokawaPvlModel(itokawaTarget, testLabel, manager);
    outputModelStatus(itokawaPvlModel);
    qDebug() << "";

    qDebug() << "Create an EmbreeShapeModel from a pvl with ElevationModel keyword";
    qDebug() << "";
    Pvl elevationLabel;
    PvlGroup elevationKernels("Kernels");
    elevationKernels += PvlKeyword("ElevationModel", dskfile);
    elevationLabel += elevationKernels;
    EmbreeShapeModel itokawaElevationModel(itokawaTarget, elevationLabel, manager);
    outputModelStatus(itokawaElevationModel);
    qDebug() << "";

    qDebug() << "Testing observer look direction intersection";
    qDebug() << endl;
    std::vector<double> observerVec(3, 0.0);
    std::vector<double> lookVec(3, 0.0);
    observerVec[0] = 1000.0;
    lookVec[0] = -1;
    qDebug() << "Intersection inputs:";
    qDebug() << "  Observer position: ("
             << observerVec[0] << ", "
             << observerVec[1] << ", "
             << observerVec[2] << ")";
    qDebug() << "  Look direction:    ("
             << lookVec[0] << ", "
             << lookVec[1] << ", "
             << lookVec[2] << ")";
    qDebug() << "Intersecting Embree shape model";
    itokawaModel.intersectSurface(naif, observerVec, lookVec);
    outputModelStatus(itokawaModel);
    qDebug() << "";

    qDebug() << "Testing observer look direction non-intersection";
    qDebug() << endl;
    observerVec[0] = 1000.0;
    observerVec[1] = 1000.0;
    observerVec[2] = 1000.0;
    lookVec[0] = 1;
    lookVec[1] = 1;
    lookVec[2] = 1;
    qDebug() << "Intersection inputs:";
    qDebug() << "  Observer position: ("
             << observerVec[0] << ", "
             << observerVec[1] << ", "
             << observerVec[2] << ")";
    qDebug() << "  Look direction:    ("
             << lookVec[0] << ", "
             << lookVec[1] << ", "
             << lookVec[2] << ")";
    qDebug() << "Intersecting Embree shape model";
    itokawaModel.intersectSurface(naif, observerVec, lookVec);
    outputModelStatus(itokawaModel);
    qDebug() << "";

    qDebug() << "Testing latitude, longitude intersection";
    qDebug() << endl;

    Latitude testLat(45, Angle::Degrees);
    Longitude testLon(85, Angle::Degrees);
    observerVec[0] = 10.0;
    observerVec[1] = 10.0;
    observerVec[2] = 10.0;
    qDebug() << "Intersection inputs:";
    qDebug() << "  Latitude:  " << testLat.degrees();
    qDebug() << "  Longitude: " << testLon.degrees();
    qDebug() << "  Observer position for occlusion: ("
             << observerVec[0] << ", "
             << observerVec[1] << ", "
             << observerVec[2] << ")";
    qDebug() << "Intersecting Embree shape model";
    itokawaModel.intersectSurface(testLat, testLon, observerVec);
    outputModelStatus(itokawaModel);
    qDebug() << "";

    qDebug() << "Testing latitude, longitude occlusion";
    qDebug() << endl;

    qDebug() << "Using " << itokawaCubeFile;
    Latitude occLat(0, Angle::Degrees);
    Longitude occLon(282, Angle::Degrees);
    std::vector<double> occlusionObserver(3, 0.0);
    itokawaCamera->SetUniversalGround( naif, occLat.degrees(), occLon.degrees() );
    itokawaCamera->instrumentBodyFixedPosition(&occlusionObserver[0], naif);
    qDebug() << "Intersection inputs:";
    qDebug() << "  Latitude:  " << occLat.degrees();
    qDebug() << "  Longitude: " << occLon.degrees();
    qDebug() << "  Observer position for occlusion: ("
             << occlusionObserver[0] << ", "
             << occlusionObserver[1] << ", "
             << occlusionObserver[2] << ")";
    qDebug() << "Intersecting Embree shape model";
    itokawaModel.intersectSurface(occLat, occLon, occlusionObserver);
    outputModelStatus(itokawaModel);
    qDebug() << "Intersecting Embree shape model without occlusion";
    itokawaModel.intersectSurface(occLat, occLon, occlusionObserver, false);
    outputModelStatus(itokawaModel);
    qDebug() << "";

    qDebug() << "Testing surface point intersection";
    qDebug() << endl;

    testLat.setDegrees(0.0);
    testLon.setDegrees(200.0);
    SurfacePoint visiblePoint( naif, testLat, testLon,
                               Distance(230.0, Distance::Meters) );
    qDebug() << "Intersection inputs:";
    qDebug() << "  Surface Point: ("
             << visiblePoint.GetX().kilometers() << ", "
             << visiblePoint.GetY().kilometers() << ", "
             << visiblePoint.GetZ().kilometers() << ")";
    qDebug() << "  Observer position for occlusion: ("
             << occlusionObserver[0] << ", "
             << occlusionObserver[1] << ", "
             << occlusionObserver[2] << ")";
    qDebug() << "  Intersecting Embree shape model";
    itokawaModel.intersectSurface(visiblePoint, occlusionObserver);
    outputModelStatus(itokawaModel);
    qDebug() << "";

    qDebug() << "Testing surface point occlusion";
    qDebug() << endl;

    testLat.setDegrees(-45.0);
    testLon.setDegrees(80.0);
    SurfacePoint occludedPoint( naif, testLat, testLon,
                                Distance(1000.0, Distance::Meters) );
    qDebug() << "Intersection inputs:";
    qDebug() << "  Surface Point: ("
             << occludedPoint.GetX().kilometers() << ", "
             << occludedPoint.GetY().kilometers() << ", "
             << occludedPoint.GetZ().kilometers() << ")";
    qDebug() << "  Observer position for occlusion: ("
             << occlusionObserver[0] << ", "
             << occlusionObserver[1] << ", "
             << occlusionObserver[2] << ")";
    qDebug() << "Intersecting Embree shape model";
    itokawaModel.intersectSurface(occludedPoint, occlusionObserver);
    outputModelStatus(itokawaModel);
    qDebug() << "Intersecting Embree shape model without occlusion";
    itokawaModel.intersectSurface(occludedPoint, occlusionObserver, false);
    outputModelStatus(itokawaModel);
    qDebug() << "";

    qDebug() << "Testing local radius";
    qDebug() << endl;

    testLat.setDegrees(35.0);
    testLon.setDegrees(270.0);
    qDebug() << "Local radius point";
    qDebug() << "  Latitude:  " << testLat.degrees();
    qDebug() << "  Longitude: " << testLon.degrees();
    qDebug() << "Radius: " << itokawaModel.localRadius(naif, testLat, testLon).kilometers();
    qDebug() << "Checking that shape model status did not change";
    outputModelStatus(itokawaModel);

    qDebug() << "Testing visibility check";
    qDebug() << endl;

    observerVec[0] = 1000.0;
    observerVec[1] = 0.0;
    observerVec[2] = 0.0;
    lookVec[0] = -1.0;
    lookVec[1] = 0.0;
    lookVec[2] = 0.0;
    qDebug() << "Intersecting Embree shape model";
    itokawaModel.intersectSurface(naif, observerVec, lookVec);
    outputModelStatus(itokawaModel);
    qDebug() << "Intersection is visible from same position and look direction? "
             << itokawaModel.isVisibleFrom(naif, observerVec, lookVec);
    observerVec[0] = 1000.0;
    observerVec[1] = 0.0;
    observerVec[2] = 0.0;
    lookVec[0] = 1.0;
    lookVec[1] = 0.0;
    lookVec[2] = 0.0;
    qDebug() << "Intersection is visible with non-intersecting look? "
             << itokawaModel.isVisibleFrom(naif, observerVec, lookVec);
    observerVec[0] = -1000.0;
    observerVec[1] = 0.0;
    observerVec[2] = 0.0;
    lookVec[0] = 1.0;
    lookVec[1] = 0.0;
    lookVec[2] = 0.0;
    qDebug() << "Intersection is visible from the opposite side? "
             << itokawaModel.isVisibleFrom(naif, observerVec, lookVec);
    qDebug() << "Increase the tolerance to 10 km";
    itokawaModel.setTolerance(10);
    qDebug() << "Intersection is now visible from the opposite side? "
             << itokawaModel.isVisibleFrom(naif, observerVec, lookVec);

    qDebug() << "Testing default ellipsoid normal";
    qDebug() << endl;

    qDebug() << "Starting model status";
    outputModelStatus(itokawaModel);
    itokawaModel.calculateDefaultNormal(naif);
    qDebug() << "Model status after recalculating";
    outputModelStatus(itokawaModel);

    qDebug() << "Testing photometric angles";
    qDebug() << endl;

    outputModelStatus(itokawaModel);
    observerVec[0] = 1000.0;
    observerVec[1] = 0.0;
    observerVec[2] = 0.0;
    qDebug() << "Emission angle: " << itokawaModel.emissionAngle(naif, observerVec);
    observerVec[0] = 1000.0;
    observerVec[1] = 100.0;
    observerVec[2] = 0.0;
    qDebug() << "Incidence angle: " << itokawaModel.incidenceAngle(naif, observerVec);

    qDebug() << "Testing errors";
    qDebug() << endl;

  }
  catch (IException &e) {
    qDebug() << "";
    qDebug() << "";
    QString msg = "**************** UNIT TEST FAILED! **************** ";
    IException(e, IException::Unknown, msg, _FILEINFO_).print();
  }
}

void outputModelStatus(EmbreeShapeModel &embreeModel) {
  qDebug() << "Embree shape model status:";
  qDebug() << "Model has intersection? " << embreeModel.hasIntersection();
  SurfacePoint *embreeIntersection = NULL;
  if ( embreeModel.hasIntersection() ) {
    embreeIntersection = embreeModel.surfaceIntersection();
    qDebug() << qSetRealNumberPrecision(4)
              << "  Surface Point: ("
              << roundToPrecision(embreeIntersection->GetX().kilometers(), 0.0001) << ", "
              << roundToPrecision(embreeIntersection->GetY().kilometers(), 0.0001) << ", "
              << roundToPrecision(embreeIntersection->GetZ().kilometers(), 0.0001) << ")";
  }
  qDebug() << "Model has normal? " << embreeModel.hasNormal();
  std::vector<double> embreeNormal;
  if ( embreeModel.hasNormal() ) {
    embreeNormal = embreeModel.normal();
    qDebug() << "  Surface Normal: ("
              << embreeNormal[0] << ", "
              << embreeNormal[1] << ", "
              << embreeNormal[2] << ")";
  }
  qDebug() << "";
}

double roundToPrecision(double value, double precision) {
  return value - fmod(value, precision);
}
