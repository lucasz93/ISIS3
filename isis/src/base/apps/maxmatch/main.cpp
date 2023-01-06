#include "Isis.h"
#include "Cube.h"
#include "Histogram.h"
#include "ProcessByLine.h"
#include "UserInterface.h"
#include "IException.h"

#include <memory>

using namespace Isis;

std::vector<double> bandScales;

static void maxmatch(std::vector<Buffer *> &in, std::vector<Buffer *> &out);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Cube from, match;

  from.open(ui.GetCubeName("FROM"));
  match.open(ui.GetCubeName("MATCH"));

  for (int i = 1; i <= from.bandCount(); ++i) {
    std::unique_ptr<Histogram> fromStats(from.histogram(i));
    std::unique_ptr<Histogram> matchStats(match.histogram(i));

    bandScales.emplace_back(fromStats->Maximum() / matchStats->Maximum());
  }

  ProcessByLine p;
  p.AddInputCube(&from, false);
  p.SetOutputCube("TO");

  p.ProcessCubes(maxmatch, false);
}

static void maxmatch(std::vector<Buffer *> &in, std::vector<Buffer *> &out) {
  const auto& from = *in[0];
  auto& to = *out[0];

  for (int i = 0; i < from.SampleDimension(); ++i) {
    if (IsSpecial(from[i])) {
      to[i] = from[i];
    }
    else {
      to[i] = from[i] / bandScales[from.Band() - 1];
    }
  }
}
