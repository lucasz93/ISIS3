/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "IException.h"
#include "UserInterface.h"
#include "ProgramLauncher.h"
#include "ProcessByLine.h"

using namespace std;
using namespace Isis;

static void psr(vector<Buffer *> &in, vector<Buffer *> &out);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  Cube fromCube;
  const auto cubeName = ui.GetCubeName("FROM");
  const auto lpf1Name = cubeName + ".lowpass1.cub";
  const auto lpf7Name = cubeName + ".lowpass7.cub";
  fromCube.open(cubeName);

  // Check that it is a Mariner9 cube.
  if ("Mariner_9" != (QString)fromCube.label()->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + cubeName + "] does not appear" +
        " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  QString parameters = "FROM=" + cubeName + 
    " TO=" + lpf1Name +
    " SAMPLES=1" +
    " LINES=25";
  ProgramLauncher::RunIsisProgram("lowpass", parameters);

  parameters = "FROM=" + cubeName + 
    " TO=" + lpf7Name +
    " SAMPLES=7" +
    " LINES=25";
  ProgramLauncher::RunIsisProgram("lowpass", parameters);

  Cube lpf1Cube, lpf7Cube;
  lpf1Cube.open(lpf1Name);
  lpf7Cube.open(lpf7Name);

  ProcessByLine p;
  p.AddInputCube(&fromCube, false);
  p.AddInputCube(&lpf1Cube, false);
  p.AddInputCube(&lpf7Cube, false);
  p.SetOutputCube("TO");
  p.ProcessCubes(psr, false);

  remove(lpf1Name.toLatin1().data());
  remove(lpf7Name.toLatin1().data());
}

static void psr(vector<Buffer *> &in, vector<Buffer *> &out)
{
  const auto& D_IPLANE = *in [0];
  const auto& BBUF2    = *in [1];
  const auto& BBUF3    = *in [2];
        auto& D_OPLANE = *out[0];

  for (int IS = 0; IS < D_IPLANE.SampleDimension(); ++IS)
  {
    if (IsSpecial(D_IPLANE[IS]))
    {
      D_OPLANE[IS] = D_IPLANE[IS];
      continue;
    }

    const auto IDIFF = BBUF2[IS] - BBUF3[IS];
    D_OPLANE[IS] = D_IPLANE[IS] - IDIFF;
    if (fabs(IDIFF) > 2) D_OPLANE[IS] = D_IPLANE[IS];
    if (D_OPLANE[IS] <= 0) D_OPLANE[IS] = Null;
  }
}