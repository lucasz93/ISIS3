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
#include "Pipeline.h"

using namespace std;
using namespace Isis;

void NullStripes(Buffer &in, Buffer &out);
int row = 0;
bool isVidiconA = false;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  const FileName from(ui.GetCubeName("FROM"));
  const FileName tempFile(from.path() + "/" + from.baseName() + ".mar9mlrp." + from.extension());

  //
  // NULL the bad lines.
  //
  {    
    Cube cube;
    cube.open(ui.GetCubeName("FROM"));
    
    // Check that it is a Mariner10 cube.
    Pvl * labels = cube.label();
    if ("Mariner_9" != (QString)labels->findKeyword("SpacecraftName", Pvl::Traverse)) {
      QString msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
        " to be a Mariner9 cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    ProcessByLine p;

    CubeAttributeOutput tempAtt(ui.GetCubeName("FROM"));

    const QString instrumentId = cube.label()->findKeyword("InstrumentId", Pvl::Traverse);
    isVidiconA = instrumentId == "M9_VIDICON_A";

    p.SetInputCube("FROM");
    p.SetOutputCube(tempFile.expanded(), tempAtt, cube.sampleCount(), cube.lineCount(), cube.bandCount());

    p.ProcessCube(NullStripes, false);
  }

  //
  // Fill the gaps.
  //
  {
    // Open the input cube
    Pipeline p("mar9dstripe");
    p.SetInputFile(tempFile);
    p.SetOutputFile("TO");
    p.KeepTemporaryFiles(false);

    // Run marnonoise to remove noise
    p.AddToPipeline("fillgap");
    p.Application("fillgap").SetInputParameter("FROM", true);
    p.Application("fillgap").SetOutputParameter("TO", "fillgap");
    p.Application("fillgap").AddConstParameter("DIRECTION", "SAMPLE");
    p.Application("fillgap").AddConstParameter("INTERP", "LINEAR");

    p.Run();
  }

  QFile::remove(tempFile.expanded());
}

void NullRange(Buffer& b, int begin, int end)
{
  for (int i = begin; i < end; ++i)
  {
    b[i] = NULL8;
  }
}

void NullStripes(Buffer &in, Buffer &out)
{
  for (int i = 0; i < in.size(); ++i)
  {
    out[i] = in[i];
  }

  // Only the A camera seems to have these stripe problems. So weird.
  if (!isVidiconA)
  {
    return;
  }

  switch (row)
  {
    case 236: NullRange(out, 305, 818); break;
    case 237: NullRange(out, 295, 818); break;
    case 238: NullRange(out, 290, 818); break;
    case 239: NullRange(out, 350, 818); break;

    case 277: NullRange(out, 295, 800); break;
    case 278: NullRange(out, 200, 800); break;
    case 279: NullRange(out, 240, 800); break;
  
    case 576: NullRange(out, 300, 800); break;
    case 577: NullRange(out, 295, 800); break;
    case 578: NullRange(out, 295, 800); break;
    case 579: NullRange(out, 295, 800); break;
    
    case 617: NullRange(out, 300, 815); break;
    case 618: NullRange(out, 250, 815); break;
    case 619: NullRange(out, 295, 800); break;
    case 620: NullRange(out, 255, 800); break;
  }

  ++row;
}