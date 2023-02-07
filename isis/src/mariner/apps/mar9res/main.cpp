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

static double prevScale = 1.0;

static QString GetCalibrationFilePrefix(Pvl& fromLabels, Pvl& previousLabels);
static void resred(vector<Buffer *> &in, vector<Buffer *> &out);

static FileName calpath;

static std::ifstream CLUN;
static int D_ROW = 0;

static short N1[6][166]{};
static short N2[6][166]{};
static short IRES[6][6][166]{};

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  Cube from, previous;
  from.open(ui.GetCubeName("FROM"));
  previous.open(ui.GetCubeName("PREVIOUS"));

  ProcessByLine p;
  p.AddInputCube(&from, false);
  p.AddInputCube(&previous, false);
  p.SetOutputCube("TO");
  
  // Check that it is a Mariner9 cube.
  Pvl * fromLabels = from.label();
  if ("Mariner_9" != (QString)fromLabels->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
      " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  Pvl * previousLabels = previous.label();
  if ("Mariner_9" != (QString)previousLabels->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetCubeName("PREVIOUS") + "] does not appear" +
      " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  int das = fromLabels->findKeyword("ImageNumber", Pvl::Traverse);
  int prevDas = previousLabels->findKeyword("ImageNumber", Pvl::Traverse);
  if (!ui.WasEntered("FORCE") && das - prevDas != 70)
  {
    std::string msg = "PREVIOUS DAS (" + std::to_string(prevDas) + ") must be 70 DAS counts less than FROM DAS " + std::to_string(das);
    throw IException(IException::User, QString(msg.c_str()), _FILEINFO_);
  }
  
  const auto fil = GetCalibrationFilePrefix(*fromLabels, *previousLabels);
  calpath = QString("$mariner9/calibration/") + fil + "ri.cal";
  std::cout << "Calibration file: " << calpath.expanded().toStdString() << std::endl;

  CLUN.open(calpath.expanded().toStdString());
  if (!CLUN.good())
  {
    throw IException(IException::Io, "Couldn't find calibration file!", _FILEINFO_);
  }

  prevScale = (double)fromLabels->findKeyword("ExposureDuration", Pvl::Traverse) / (double)previousLabels->findKeyword("ExposureDuration", Pvl::Traverse);

  // Setup guard values?
  for (int I = 0; I < 166; ++I)
  {
    N1[5][I] = 255;
    N2[5][I] = 255;
  }

  p.ProcessCubes(resred, false);
}

static QString GetCalibrationFilePrefix(Pvl& fromLabels, Pvl& previousLabels)
{
  UserInterface &ui = Application::GetUserInterface();
  QString filter1 = fromLabels.findKeyword("FilterNumber", Pvl::Traverse);
  QString filter2 = previousLabels.findKeyword("FilterPosition", Pvl::Traverse);

  if (filter1 == "*")
  {
    // M9_VIDICON_B had no filter.
    return "b";
  }

  QString fil = "4";

  // This is all ripped directly from m9res.F in ISIS 2.
  if (filter1 == "2" || filter2 == "2")
  {
    fil = "2";
  }
  if (filter2 == "6" || filter2 == "8")
  {
    if (filter1 == "6" || filter1 == "8")
    {
      fil = "6";
    }
    else if (filter1 == "8" && filter2 == "8")
    {
      fil = "8";
    }
  }

  if (fil != "b" && fil != "2" && fil != "4")
  {
    throw IException(IException::User, "Calibration file does not exist for this filter combination (FROM = " + filter1 + ", PREVIOUS = " + filter2 + ")", _FILEINFO_);
  }

  return fil;
}

// SIMPLE LINEAR EXTRAPOLATION FOR M9RES
short EXTRAP(short IX1, short IX2, short IY1, short IY2)
{
  const auto e = IY2 + (255-IX2)*(IY2-IY1)/(IX2-IX1)/2;
  return std::max(0, e);
}

//
// This is more or less transcribed from:
// D_STEP = 6 - Systematic access to the core data
//
// Fortran arrays are column major. C is row major. I've flipped all array indices to reflect this.
// Fortran indices are 1 based. C is 0 based. 1 has been subtracted from all indices.
//
// The comments indicating the different conditions are incorrect!
// Need to subtract 1 from each of of them! They were kept the same as the original Fortran version for tracability.
//
static void resred(vector<Buffer *> &in, vector<Buffer *> &out)
{
  // Load updated calibration data.
  if ((D_ROW % 5) == 0)
  {
    for (int I = 0; I < 5; ++I)
    {
      for (int ISD = 0; ISD < 166; ++ISD)
      {
        CLUN >> N1[I][ISD];
      }

      for (int ISD = 0; ISD < 166; ++ISD)
      {
        CLUN >> N2[I][ISD];
      }

      for (int J = 0; J < 5; ++J)
      {
        for (int ISD = 0; ISD < 166; ++ISD)
        {
          CLUN >> IRES[I][J][ISD];
        }
      }
    }
  }

  const auto& CUR = *in[0];
  const auto& PRE = *in[1];
  auto& OUT = *out[0];

  OUT[829] = CUR[829];
  OUT[830] = CUR[830];
  OUT[831] = CUR[831];

  static int NSD1 = 0;
  for (int IS = 0; IS < 829; ++IS)
  {
    static int ISD = IS/5;
    static int IDX1 = 0;
    static int IDX2 = 0;
    static short IN1, IN2;
    static short IRES1, IRES3, IRES4;
    static double T, U, RES;

    const auto PREIS = PRE[IS] * prevScale;

    for (int I = 0; I < 5; ++I)
    {
      if (CUR[IS] > N2[I][ISD]) IDX2 = I + 1;
      if (PREIS   > N1[I][ISD]) IDX1 = I + 1;
    }

    if (IDX1 > 4)
    {
      IN1 = N1[4][ISD];
      if (IDX2 > 4)                         // IDX1>5,IDX2>5
      {
        IN2 = N2[4][ISD];
        if (NSD1 != ISD)
        {
          NSD1 = ISD;
          for (int I = 0; I < 2; ++I)
          {
              IRES[5-I][5][ISD] = EXTRAP(N1[3][ISD], IN1, IRES[5-I][3][ISD], IRES[5-I][4][ISD]);
              IRES[5][5-I][ISD] = EXTRAP(N2[3][ISD], IN2, IRES[3][5-I][ISD], IRES[4][5-I][ISD]);
          }

          IRES[5][5][ISD] = (EXTRAP(N2[3][ISD], IN2, IRES[3][5][ISD], IRES[4][5][ISD]) + EXTRAP(N1[3][ISD], IN1, IRES[5][3][ISD], IRES[5][4][ISD]) + 1) / 2;
        }

        IRES1 = IRES[5][4][ISD];
        IRES3 = IRES[4][5][ISD];
        IRES4 = IRES[4][4][ISD];
      }
      else                                  // IDX1>5, IDX2<6
      {
        IRES1 = IRES[IDX2][4][ISD];
        IRES[IDX2][5][ISD] = EXTRAP(N1[3][ISD], N1[4][ISD], IRES[IDX2][3][ISD], IRES[IDX2][4][ISD]);

        if (IDX2 > 0)
        {
          IN2 = N2[IDX2 - 1][ISD];
          IRES3 = EXTRAP(N1[3][ISD], N1[4][ISD], IRES[IDX2 - 1][3][ISD], IRES[IDX2 - 1][4][ISD]);
          IRES4 = IRES[IDX2-1][4][ISD];
        }
        else                                // IDX1=6, IDX2=1
        {
          IRES3 = 4;
          IRES4 = 3;
          IN2 = 0;
        }
      }
    }
    else                                    // IDX1<6
    {
      if (IDX2 > 4)                         // IDX1<6, IDX2=6
      {
        IN2 = N2[IDX2 - 1][ISD];
        if (IDX1 == 1)
        {
          IRES1 = 0;
          IRES3 = IRES[4][0][ISD];
          IRES4 = 0;
          IN1 = 0;
        }
        else                                // 1<IDX1<6, IDX2=6
        {
           IRES1 = EXTRAP(N2[3][ISD], IN2, IRES[3][IDX1 - 1][ISD], IRES[4][IDX1 - 1][ISD]);
           IRES[5][IDX1][ISD] = EXTRAP(N2[3][ISD], N2[4][ISD], IRES[3][IDX1][ISD], IRES[4][IDX1][ISD]);
           IRES3 = IRES[4][IDX1][ISD];
           IRES4 = IRES[4][IDX1 - 1][ISD];
           IN1 = N1[IDX1 - 1][ISD];
        }
      }
      else                                  // IDX1<6, IDX2<6
      {
        if (IDX2 <= 0)
        {
          IN2 = 0;
          if (IDX1 <= 0)                    // IDX1=1, IDX2=1
          {
            IRES1 = 0;
            IRES3 = N1[IDX1][ISD] / 50;
            IRES4 = 0;
            IN1 = 0;
          }
          else                              // IDX1>1, IDX2=1
          {
            IRES1 = IRES[0][IDX1 - 1][ISD];
            IRES3 = N1[IDX1 - 1][ISD] / 50;
            IRES4 = N1[IDX1][ISD] / 50;
            IN1 = N1[IDX1 - 1][ISD];
          }
        }
        else                                // IDX1<6, 1<IDX2<6
        {
          IN2 = N2[IDX2 - 1][ISD];
          if (IDX1 == 0)                    // IDX1=1, 1<IDX2<6
          {
            IRES1 = 0;
            IRES3 = IRES[IDX2][0][ISD];
            IRES4 = 0;
            IN1 = 0;
          }
          else                              // 1<IDX1<6, 1<IDX2<6
          {
            IRES1 = IRES[IDX2][IDX1 - 1][ISD];
            IRES3 = IRES[IDX2 - 1][IDX1][ISD];
            IRES4 = IRES[IDX2 - 1][IDX1 - 1][ISD];
            IN1 = N1[IDX1 - 1][ISD];
          }
        }
      }
    }
    
    T = double(PREIS   - IN1) / double(N1[IDX1][ISD] - IN1);
    U = double(CUR[IS] - IN2) / double(N2[IDX2][ISD] - IN2);
    RES = (1.-T)*(1.-U)*double(IRES4) + T*(1.-U)*double(IRES3) + T*U*double(IRES[IDX2][IDX1][ISD]) + (1.-T)*U*double(IRES1);
    OUT[IS] = CUR[IS] - std::round(RES/16.);
  }

  ++D_ROW;
}