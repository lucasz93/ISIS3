/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "MaximumCorrelation.h"
#include "Chip.h"
#include "MultivariateStatistics.h"

namespace Isis {
  double MaximumCorrelation::MatchAlgorithm(Chip &pattern, Chip &subsearch) {
    MultivariateStatistics mv;

    for(int l = 1; l <= pattern.Lines(); l++) {
      const auto* pdn = pattern.GetLine(1, l);
      const auto* sdn = subsearch.GetLine(1, l);
      mv.AddData(pdn, sdn, pattern.Samples());
    }
    double percentValid = (double) mv.ValidPixels() /
                          (pattern.Lines() * pattern.Samples());
    if(percentValid * 100.0 < this->PatternValidPercent()) return Isis::Null;

    double r = mv.Correlation();
    if(r == Isis::Null) return Isis::Null;
    return fabs(r);
  }

  /**
   * This virtual method must return if the 1st fit is equal to or better
   * than the second fit.
   *
   * @param fit1  1st goodness of fit
   * @param fit2  2nd goodness of fit
   */
  bool MaximumCorrelation::CompareFits(double fit1, double fit2) {
    return (fit1 >= fit2);
  }
}

extern "C" Isis::AutoReg *MaximumCorrelationPlugin(Isis::Pvl &pvl) {
  return new Isis::MaximumCorrelation(pvl);
}

