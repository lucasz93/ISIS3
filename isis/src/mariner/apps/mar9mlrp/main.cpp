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

std::unique_ptr<Buffer> lastLine;

void BlitMissingLines(Buffer &in, Buffer &out);

void IsisMain() {

  ProcessByLine p;

  UserInterface &ui = Application::GetUserInterface();
  p.SetInputCube(ui.GetFileName("FROM"));
  p.SetOutputCube("TO");
  
  Cube cube;
  cube.open(ui.GetFileName("FROM"));
  
  // Check that it is a Mariner10 cube.
  Pvl * labels = cube.label();
  if ("Mariner_9" != (QString)labels->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetFileName("FROM") + "] does not appear" +
      " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  p.ProcessCube(BlitMissingLines, false);
}

void BlitMissingLines(Buffer &in, Buffer &out)
{
  bool lineIsValid = !IsHighPixel(in[0]) || !IsHighPixel(in[1]) || !IsNullPixel(in[2]) || !IsNullPixel(in[3]);
  Buffer& src = lastLine && !lineIsValid
    ? *lastLine
    : in;
  
  for (int i = 0; i < src.size(); ++i)
  {
    out[i] = src[i];
  }

  if (lineIsValid)
  {
    lastLine = std::make_unique<Buffer>(in);
  }
}