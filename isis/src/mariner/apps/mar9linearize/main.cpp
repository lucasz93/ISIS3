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

using namespace std;
using namespace Isis;

static void linearize(vector<Buffer *> &in, vector<Buffer *> &out);

FileName DCFILE;
float SCALE, B, D;
std::array<float, 5> C;

int D_ROW = 0;

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
// Determine which dark current file to use
//***************************************************************************  
  const QString CAM = labels->findKeyword("InstrumentId", Pvl::Traverse);
  const int IFSC = labels->findKeyword("ImageNumber", Pvl::Traverse);

  if (CAM == "M9_VIDICON_A")
  {
    C = std::array<float, 5>{ .6999002E-11, -.1260765E-07, .3610607E-08, .168951E-10, .9157377E-05 };
    SCALE = .90792416;
    B = .1029981673;
    D = 2.;

    DCFILE = "$mariner9/calibration/1a0dc.cub";
    if (IFSC > 2927465) DCFILE="$mariner9/calibration/72a6dc.cub";
    if (IFSC > 5436914) DCFILE="$mariner9/calibration/139a2dc.cub";
	  if (IFSC > 6768823) DCFILE="$mariner9/calibration/150a31dc.cub";
	  if (IFSC > 8243586) DCFILE="$mariner9/calibration/221a4dc.cub";
	  if (IFSC > 11000000)
    {
      std::cout << "DAS " << IFSC << " in extended mission, no good" << std::endl;
    }
  }
  else
  {
    C = std::array<float, 5>{ -.6654653E-11, .1243616E-07, .5302868E-09, -.1025861E-10, .8699718E-05 };
    SCALE = .9436151;
    B = .05998745874;
    D = .5;

    DCFILE="$mariner9/calibration/1b0dc.cub";
    if (IFSC > 2031918) DCFILE="$mariner9/calibration/22b31dc.cub";
    if (IFSC > 3051127) DCFILE="$mariner9/calibration/59b1dc.cub";
    if (IFSC > 3874610) DCFILE="$mariner9/calibration/68b2dc.cub";
    if (IFSC > 4254325) DCFILE="$mariner9/calibration/80b17dc.cub";
    if (IFSC > 5340091) DCFILE="$mariner9/calibration/129b9dc.cub";
    if (IFSC > 6589168) DCFILE="$mariner9/calibration/150b32dc.cub";
    if (IFSC > 8243586) DCFILE="$mariner9/calibration/221b3dc.cub";
    if (IFSC > 10119506) DCFILE="$mariner9/calibration/262b3dc.cub";
    if (IFSC > 11000000) DCFILE="$mariner9/calibration/479b1dc.cub";
  }

  std::cout << "Dark Current file: " << DCFILE.expanded().toStdString() << std::endl;

//***************************************************************************
// Open the dark current file
//***************************************************************************
  Cube dc;
  dc.open(DCFILE.expanded());

//***************************************************************************
// D_STEP = 6 - Systematic access to the core data
//***************************************************************************
  ProcessByLine p;
  p.AddInputCube(&cube, false);
  p.AddInputCube(&dc, false);

  CubeAttributeOutput outputProperties;
  outputProperties.setPixelType(Real);
  p.SetOutputCube(ui.GetFileName("TO"), outputProperties);

  p.ProcessCubes(linearize, false);
}

static void linearize(vector<Buffer *> &in, vector<Buffer *> &out)
{
  const auto& INLINE = *in[0];
  const auto& DCLINE = *in[1];
  auto& D_OPLANE = *out[0];

  for (int IS = 0; IS < INLINE.SampleDimension(); ++IS)
  {
      const float A = C[0]*D_ROW*D_ROW+C[1]*D_ROW+C[2]*IS+C[3]*D_ROW*IS+C[4];
      const float DN = std::max(0., INLINE[IS] - DCLINE[IS]);
      const float DNT = SCALE*(DN + B*DN/(DN+D));
      D_OPLANE[IS] = std::round(DNT + A*(std::pow(DNT, 3) - 128.*DNT*DNT));
  }

  ++D_ROW;
}