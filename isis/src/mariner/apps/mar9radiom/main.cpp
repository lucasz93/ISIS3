/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "IException.h"
#include "UserInterface.h"
#include "ProcessByLine.h"
#include "Camera.h"

#include <iomanip>

using namespace std;
using namespace Isis;

static void radiom(vector<Buffer *> &in, vector<Buffer *> &out);

int D_ROW = 0;
double SF = 0;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  Cube cube;
  cube.open(ui.GetFileName("FROM"));

  // Check that it is a Mariner9 cube.
  Pvl * labels = cube.label();
  if ("Mariner_9" != (QString)labels->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetFileName("FROM") + "] does not appear" +
      " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

//***************************************************************************
// Determine which calibration file to use
//***************************************************************************  
  const QString CAMERA = labels->findKeyword("InstrumentId", Pvl::Traverse);
  const double EXPTL = (double)labels->findKeyword("ExposureDuration", Pvl::Traverse) / 1000.0;

  const QString FILTER = labels->findKeyword("FilterName", Pvl::Traverse);
  const int IF = CAMERA == "M9_VIDICON_A"
    ? (int)labels->findKeyword("FilterNumber", Pvl::Traverse)
    : 9;

  const std::string CALPATH = CAMERA == "M9_VIDICON_A"
    ? "$mariner9/calibration/" + std::to_string(IF) + "shading.cub"
    : "$mariner9/calibration/bshading.cub";
  const FileName CALFILE(QString(CALPATH.c_str()));

  std::cout << " EXPOSURE TIME: " << std::setw(5) << std::setprecision(3) << EXPTL << " SEC." << std::endl;
  
  const std::array<double, 10> AEXPT{ 3.93, 6.75, 12.66, 24.51, 48.26, 95.67, 190.42, 379.98, 759., 1517.2 };
  const std::array<double, 10> BEXPT{ 3.98, 6.95, 12.86, 24.62, 48.42, 95.80, 186.50, 380.10, 759., 1517.0 };
  const int EXPT_INDEX = std::round(std::log(EXPTL/.003)/0.6931471806);
  const double EXPT = CAMERA == "M9_VIDICON_A"
    ? AEXPT.at(EXPT_INDEX)
    : BEXPT.at(EXPT_INDEX);

  Camera * cam = cube.camera();
  bool camSuccess = cam->SetImage(cube.sampleCount()/2,cube.lineCount()/2);
  if (!camSuccess) {
    throw IException(IException::Unknown,
        "Unable to calculate the Solar Distance on [" +
        cube.fileName() + "]", _FILEINFO_);
  }
  const double SUND = cam->SolarDistance();
  const double SUNDS = SUND * SUND;
  std::cout << " SUN DISTANCE IS: " << std::setw(6) << std::setprecision(7) << SUND << " AU." << std::endl;

  // FF IS FILTER FACTOR FOR A CAMERA FILTERS 1-8, FF(9) FOR B CAMERA
  const std::array<double, 10> FF{1000., 431., 263., 356., 263., 180., 263., 1000., 47.5};
  SF = FF[IF - 1]*SUNDS/EXPT/10000.;
  std::cout << "Calibration file: " << CALPATH << std::endl;

//***************************************************************************
// Open the calibration file
//***************************************************************************
  Cube FID2;
  FID2.open(CALFILE.expanded());

//***************************************************************************
// D_STEP = 6 - Systematic access to the core data
//***************************************************************************
  ProcessByLine p;
  p.AddInputCube(&cube, false);
  p.AddInputCube(&FID2, false);
  p.SetOutputCube("TO");

  p.ProcessCubes(radiom, false);
}

static void radiom(vector<Buffer *> &in, vector<Buffer *> &out)
{
  const auto& D_IPLANE = *in[0];
  const auto& BBUF2 = *in[1];
  auto& D_OPLANE = *out[0];

  for (int IS = 0; IS < D_IPLANE.SampleDimension(); ++IS)
  {
    D_OPLANE[IS] = std::floor(D_IPLANE[IS]*SF*BBUF2[IS]);
  }

  ++D_ROW;
}