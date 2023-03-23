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
#include "History.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  Cube fromCube;
  fromCube.open(ui.GetCubeName("FROM"));

  // Check that it is a Mariner9 cube.
  if ("Mariner_9" != (QString)fromCube.label()->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
        " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Check that the cube actually needs cleaning.
  if (fromCube.readHistory().ReturnHist().hasObject("remrx")) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "]" +
      " appears to have already been cleaned";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Open the input cube
  Pipeline p("mar9clean");
  p.SetInputFile("FROM");
  p.SetOutputFile("TO");
  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));

  // Run marnonoise to remove noise
  p.AddToPipeline("marnonoise", "marnonoise1");
  p.Application("marnonoise1").SetInputParameter("FROM", true);
  p.Application("marnonoise1").SetOutputParameter("TO", "marnonoise1");

  // Run findrx on the cube to find the actual position of the reseaus
  auto &reseaus = fromCube.label()->findGroup("Reseaus", Pvl::Traverse);
  if ((QString)reseaus["Status"] != "Refined")
  {
    p.AddToPipeline("findrx");
    p.Application("findrx").SetInputParameter("FROM", false);
    p.Application("findrx").AddConstParameter("FORCEREFINE", "true");
  }

  // Run remrx on the cube to remove the reseaus
  p.AddToPipeline("remrx");
  p.Application("remrx").SetInputParameter("FROM", true);
  p.Application("remrx").SetOutputParameter("TO", "remrx");
  p.Application("remrx").AddParameter("SDIM", "SDIM");
  p.Application("remrx").AddParameter("LDIM", "LDIM");

  // Need to do this before 'trim', because trim removes the missing line markers.
  // Need to run it after 'remrx' because we don't want to propagate reseau markers without any way to clean them up.
  if (ui.GetBoolean("MLRP"))
  {
    p.AddToPipeline("mar9mlrp");
    p.Application("mar9mlrp").SetInputParameter("FROM", true);
    p.Application("mar9mlrp").SetOutputParameter("TO", "mar9mlrp");
  }

  // Fill in the nulls.
  p.AddToPipeline("fillgap", "fillgap1-line");
  p.Application("fillgap1-line").SetInputParameter("FROM", true);
  p.Application("fillgap1-line").SetOutputParameter("TO", "fillgap1-line");
  p.Application("fillgap1-line").AddConstParameter("DIRECTION", "LINE");
  p.Application("fillgap1-line").AddConstParameter("ONLYFILLNULLS", "true");

  p.AddToPipeline("fillgap", "fillgap1-sample");
  p.Application("fillgap1-sample").SetInputParameter("FROM", true);
  p.Application("fillgap1-sample").SetOutputParameter("TO", "fillgap1-sample");
  p.Application("fillgap1-sample").AddConstParameter("DIRECTION", "sample");
  p.Application("fillgap1-sample").AddConstParameter("ONLYFILLNULLS", "true");

  // Some images are stubborn and need a second cleaning. 07794013, for example.
  p.AddToPipeline("marnonoise", "marnonoise2");
  p.Application("marnonoise2").SetInputParameter("FROM", true);
  p.Application("marnonoise2").SetOutputParameter("TO", "marnonoise2");

  p.AddToPipeline("fillgap", "fillgap2-line");
  p.Application("fillgap2-line").SetInputParameter("FROM", true);
  p.Application("fillgap2-line").SetOutputParameter("TO", "fillgap2-line");
  p.Application("fillgap2-line").AddConstParameter("DIRECTION", "LINE");
  p.Application("fillgap2-line").AddConstParameter("ONLYFILLNULLS", "true");

  p.AddToPipeline("fillgap", "fillgap2-sample");
  p.Application("fillgap2-sample").SetInputParameter("FROM", true);
  p.Application("fillgap2-sample").SetOutputParameter("TO", "fillgap2-sample");
  p.Application("fillgap2-sample").AddConstParameter("DIRECTION", "sample");
  p.Application("fillgap2-sample").AddConstParameter("ONLYFILLNULLS", "true");

  // Some stubborn stains STILL persist.
  p.AddToPipeline("viknosalt");
  p.Application("viknosalt").SetInputParameter("FROM", true);
  p.Application("viknosalt").SetOutputParameter("TO", "viknosalt");

  p.AddToPipeline("viknopepper");
  p.Application("viknopepper").SetInputParameter("FROM", true);
  p.Application("viknopepper").SetOutputParameter("TO", "viknosalt");

  // Run trim to remove data outside of visual frame
  p.AddToPipeline("trim");
  p.Application("trim").SetInputParameter("FROM", true);
  p.Application("trim").SetOutputParameter("TO", "trim");
  p.Application("trim").AddConstParameter("TOP", "12");
  p.Application("trim").AddConstParameter("LEFT", "11");
  p.Application("trim").AddConstParameter("RIGHT", "8");

  p.AddToPipeline("mar9psr");
  p.Application("mar9psr").SetInputParameter("FROM", true);
  p.Application("mar9psr").SetOutputParameter("TO", "mar9psr");

  cout << p;
  p.Run();
}
