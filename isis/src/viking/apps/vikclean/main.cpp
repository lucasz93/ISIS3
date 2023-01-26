/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "Pipeline.h"

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  Cube fromCube;
  fromCube.open(ui.GetCubeName("FROM"));

  // Open the input cube
  Pipeline p("vikclean");
  p.SetInputFile("FROM");
  p.SetOutputFile("TO");
  p.KeepTemporaryFiles(!ui.GetBoolean("REMOVE"));

  // Run vikfixtrx on the cube to remove the tracks
  p.AddToPipeline("vikfixtrx");
  p.Application("vikfixtrx").SetInputParameter("FROM", true);
  p.Application("vikfixtrx").SetOutputParameter("TO", "fixtrx");

  // Run vikfixfly on the cube
  p.AddToPipeline("viknobutter");
  p.Application("viknobutter").SetInputParameter("FROM", true);
  p.Application("viknobutter").SetOutputParameter("TO", "");

  // Run marnonoise to remove noise
  p.AddToPipeline("marnonoise", "marnonoise1");
  p.Application("marnonoise1").SetInputParameter("FROM", true);
  p.Application("marnonoise1").SetOutputParameter("TO", "marnonoise1");

  // Run marnonoise to remove noise
  p.AddToPipeline("marnonoise", "marnonoise2");
  p.Application("marnonoise2").SetInputParameter("FROM", true);
  p.Application("marnonoise2").SetOutputParameter("TO", "marnonoise2");

  // Run marnonoise to remove noise
  p.AddToPipeline("marnonoise", "marnonoise3");
  p.Application("marnonoise3").SetInputParameter("FROM", true);
  p.Application("marnonoise3").SetOutputParameter("TO", "marnonoise3");

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
  p.AddToPipeline("marnonoise", "marnonoise4");
  p.Application("marnonoise4").SetInputParameter("FROM", true);
  p.Application("marnonoise4").SetOutputParameter("TO", "marnonoise4");

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
  p.Application("viknopepper").SetOutputParameter("TO", "viknopepper");

  // Run trim to remove data outside of visual frame
  p.AddToPipeline("trim");
  p.Application("trim").SetInputParameter("FROM", true);
  p.Application("trim").SetOutputParameter("TO", "trim");
  p.Application("trim").AddConstParameter("TOP", "2");
  p.Application("trim").AddConstParameter("LEFT", "24");
  p.Application("trim").AddConstParameter("RIGHT", "24");

  p.Run();
}
