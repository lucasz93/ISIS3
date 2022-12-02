/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Chip.h"
#include "Cube.h"
#include "IException.h"
#include "Pipeline.h"
#include "Statistics.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  Cube fromCube;
  fromCube.open(ui.GetFileName("FROM"));

  // Check that it is a Mariner9 cube.
  if ("Mariner_9" != (QString)fromCube.label()->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetFileName("FROM") + "] does not appear" +
        " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Open the input cube
  Pipeline p("mar9cal");
  p.SetInputFile("FROM");
  p.SetOutputFile("TO");
  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));

  // Remove dark current.
  p.AddToPipeline("mar9linearize");
  p.Application("mar9linearize").SetInputParameter("FROM", true);
  p.Application("mar9linearize").SetOutputParameter("TO", "mar9linearize");

  // Radiometric calibration.
  p.AddToPipeline("mar9radiom");
  p.Application("mar9radiom").SetInputParameter("FROM", true);
  p.Application("mar9radiom").AddParameter("FALLBACK", "FALLBACK");
  p.Application("mar9radiom").SetOutputParameter("TO", "mar9radiom");

  cout << p;
  p.Run();
}
