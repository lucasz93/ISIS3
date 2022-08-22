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

void NullMissingLines(Buffer &in, Buffer &out);

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  const FileName from(ui.GetFileName("FROM"));
  const FileName tempFile(from.path() + "/" + from.baseName() + ".mar9mlrp." + from.extension());

  //
  // NULL the bad lines.
  //
  {    
    Cube cube;
    cube.open(ui.GetFileName("FROM"));
    
    // Check that it is a Mariner10 cube.
    Pvl * labels = cube.label();
    if ("Mariner_9" != (QString)labels->findKeyword("SpacecraftName", Pvl::Traverse)) {
      QString msg = "The cube [" + ui.GetFileName("FROM") + "] does not appear" +
        " to be a Mariner9 cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    ProcessByLine p;

    CubeAttributeOutput tempAtt(ui.GetFileName("FROM"));

    p.SetInputCube("FROM");
    p.SetOutputCube(tempFile.expanded(), tempAtt, cube.sampleCount(), cube.lineCount(), cube.bandCount());

    p.ProcessCube(NullMissingLines);
  }

  //
  // Fill the gaps.
  //
  {
    // Open the input cube
    Pipeline p("mar9clean");
    p.SetInputFile(tempFile);
    p.SetOutputFile("TO");
    p.KeepTemporaryFiles(false);

    // Run marnonoise to remove noise
    p.AddToPipeline("fillgap");
    p.Application("fillgap").SetInputParameter("FROM", true);
    p.Application("fillgap").SetOutputParameter("TO", "fillgap");
    p.Application("fillgap").AddConstParameter("DIRECTION", "SAMPLE");
    p.Application("fillgap").AddConstParameter("INTERP", "AKIMA");

    p.Run();
  }

  QFile::remove(tempFile.expanded());
}

void NullMissingLines(Buffer &in, Buffer &out)
{
  const bool lineIsValid = !IsHighPixel(in[0]) || !IsHighPixel(in[1]) || !IsNullPixel(in[2]) || !IsNullPixel(in[3]);
  if (lineIsValid)
  {
    for (int i = 0; i < in.size(); ++i)
    {
      out[i] = in[i];
    }
  }
  else
  {
    for (int i = 0; i < in.size(); ++i)
    {
      out[i] = NULL8;
    }
  }
}