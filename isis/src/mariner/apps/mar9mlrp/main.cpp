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

void FillMissingLines(Buffer &in, Buffer &out);

static std::array<double, 832> lastValidLine{ 0 };

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  const FileName from(ui.GetCubeName("FROM"));

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

  p.SetInputCube("FROM");
  p.SetOutputCube("TO");

  p.ProcessCube(FillMissingLines, false);
}

bool IsLineValid(Buffer &in)
{
  return !(IsHighPixel(in[0]) && IsHighPixel(in[1]) && IsNullPixel(in[2]) && IsNullPixel(in[3]));
}

void FillMissingLines(Buffer &in, Buffer &out)
{
  if (IsLineValid(in))
  {
    for (int i = 0; i < in.size(); ++i)
    {
      out[i] = in[i];
      if (!IsNullPixel(out[i]))
      {
        lastValidLine[i] = out[i];
      }
    }
  }
  else
  {
    for (int i = 0; i < in.size(); ++i)
    {
      out[i] = lastValidLine[i];
    }
  }
}