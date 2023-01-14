/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "IException.h"
#include "ProcessByLine.h"

using namespace std;
using namespace Isis;

static void combine(vector<Buffer *> &in, vector<Buffer *> &out);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  Cube from, from2;
  from.open(ui.GetCubeName("FROM"));
  from2.open(ui.GetCubeName("FROM2"));

  // Check that it is a Mariner9 cube.
  if ("Mariner_9" != (QString)from.label()->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
        " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  if ("Mariner_9" != (QString)from2.label()->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetCubeName("FROM2") + "] does not appear" +
        " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  
  // Check they're of the same image.
  if ((int)from.label()->findKeyword("ImageNumber", Pvl::Traverse) != (int)from2.label()->findKeyword("ImageNumber", Pvl::Traverse)) {
    QString msg = "The input cubes have different ImageNumbers";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  ProcessByLine p;
  p.AddInputCube(&from, false);
  p.AddInputCube(&from2, false);
  p.SetOutputCube("TO");
  p.ProcessCubes(combine, false);
}

static void combine(vector<Buffer *> &in, vector<Buffer *> &out)
{
  const auto& from  = *in [0];
  const auto& from2 = *in [1];
        auto& to    = *out[0];

  double last = 0;

  bool lineIsNull = true;

  for (int i = 0; i < from.SampleDimension(); ++i)
  {
    lineIsNull = lineIsNull && from[i] == Null && from2[i] == Null;

    // Fill in NULL pixels from the other source.
    if (from[i] == Null)
    {
      last = to[i] = from2[i];
      continue;
    }
    if (from2[i] == Null)
    {
      last = to[i] = from[i];
      continue;
    }

    const auto d1 = from [i] - last;
    const auto d2 = from2[i] - last;

    if (d1 != d2)
    {
      // Deltas differ. Assume the larger one is the noise.
      to[i] = std::fabs(d1) < std::fabs(d2)
        ? from [i]
        : from2[i];
    }
    else
    {
      // Deltas match. Signal is good from both sources.
      to[i] = from[i];
    }

    last = to[i];
  }

  if (lineIsNull)
  {
    to[0] = to[1] = Hrs;
    to[2] = to[3] = Null;
    to[4] = to[5] = Hrs;
    to[6] = to[7] = Null;
  }
}