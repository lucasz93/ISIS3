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

static char GetFilterCalibration(int IF);
static void radiom(vector<Buffer *> &in, vector<Buffer *> &out);

int D_ROW = 0;
double SF = 0;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  Cube cube;
  cube.open(ui.GetCubeName("FROM"));

  // Check that it is a Mariner9 cube.
  Pvl * labels = cube.label();
  if ("Mariner_9" != (QString)labels->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
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
  if (!ui.GetBoolean("FALLBACK") && IF != 2 && IF != 5 && IF != 9)
  {
    throw IException(IException::User, QString("Calibration file doesn't exist for filter ") + FILTER + QString(". Use FALLBACK=YES to user use an existing filter of the closest wavelength. Results may vary."), _FILEINFO_);
  }

  const std::string CALPATH = std::string("$mariner9/calibration/") + GetFilterCalibration(IF) + "shading.cub";
  const FileName CALFILE(QString(CALPATH.c_str()));

  std::cout << " EXPOSURE TIME: " << std::setw(5) << std::setprecision(3) << EXPTL << " SEC." << std::endl;
  
  const std::array<double, 12> AEXPT{ 3.93, 6.75, 12.66, 24.51, 48.26, 95.67, 190.42, 379.98, 759., 1517.2, 3033.57, 6066.3 };
  const std::array<double, 12> BEXPT{ 3.98, 6.95, 12.86, 24.62, 48.42, 95.80, 186.50, 380.10, 759., 1517.0, 3033.17, 6065.5 };
  const int EXPT_INDEX = std::round(std::log(EXPTL/.003)/0.6931471806);
  if (EXPT_INDEX > AEXPT.size())
  {
    std::string err = "Larger than expected ExposureDuration [" + std::to_string(EXPTL) + "]";
    throw IException(IException::User, QString(err.c_str()), _FILEINFO_);
  }
  const double EXPT = CAMERA == "M9_VIDICON_A"
    ? AEXPT.at(EXPT_INDEX)
    : BEXPT.at(EXPT_INDEX);

  Camera * cam = cube.camera();

  // Try to get the distance to a point in the image.
  const double SUND = cam->SetImage(cube.sampleCount()/2,cube.lineCount()/2)
    // Success! This is more accurate than the original m9radiom from ISIS 2.
    ? cam->SolarDistance()
    // This is the way ISIS 2 did it. Fall back if necessary.
    : cam->sunToBodyDist();
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

static char GetFilterCalibration(int IF)
{
  switch (IF)
  {
    case 1: // -0.005 nm
    case 4: // -0.020 nm
    case 6: // -0.088 nm
    case 8: // -0.151nm
      return '5';

    case 2:
      return '2';

    // All polaroids use the same wavelength.
    case 3:
    case 5:
    case 7:
      return '5';

    case 9:
      return 'b';

    default:
      throw IException(IException::User, "Unknown FilterNumber", _FILEINFO_);
  }
}

static void radiom(vector<Buffer *> &in, vector<Buffer *> &out)
{
  const auto& D_IPLANE = *in[0];
  const auto& BBUF2 = *in[1];
  auto& D_OPLANE = *out[0];

  for (int IS = 0; IS < D_IPLANE.SampleDimension(); ++IS)
  {
    if (D_IPLANE[IS] == 0.0)
    {
      D_OPLANE[IS] = NULL8;
      continue;
    }

    const double v = std::floor(D_IPLANE[IS]*SF*BBUF2[IS]);
    if (v <= 0)
    {
      D_OPLANE[IS] = NULL8;
      continue;
    }

    D_OPLANE[IS] = v;
  }

  ++D_ROW;
}