#include "gmock/gmock.h"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

#include <QString>

#include <nlohmann/json.hpp>

#include "Angle.h"
#include "FileName.h"
#include "IException.h"
#include "Preference.h"
#include "SpiceRotation.h"
#include "Table.h"
#include "TestUtilities.h"

// Declarations for bindings for Naif Spicelib routines that do not have
// a wrapper
extern int bodeul_(integer *body, doublereal *et, doublereal *ra,
                   doublereal *dec, doublereal *w, doublereal *lamda);

using json = nlohmann::json;
using namespace std;
using namespace Isis;

// Old unit test set output precision to 8 digits.
double testTolerance = 1e-8;

// Test case is taken from moc red wide angle image ab102401
// sn = MGS/561812335:32/MOC-WA/RED
//
// This is written as a fixture to ensure that the test kernels get unloaded
// regardless of how the test finishes.
class SpiceRotationKernels : public ::testing::Test {
  protected:
    vector<QString> kernels;
    double startTime;
    double endTime;
    int frameCode;
    int targetCode;

  void SetUp() {
    auto naif = NaifContext::acquire();
    
    startTime = -69382819.0;
    endTime = -69382512.0;
    frameCode = -94031;
    targetCode = 499;

    QString dir = FileName("$ISISTESTDATA/isis/src/base/unitTestData/kernels").expanded() + "/";
    kernels.clear();
    kernels.push_back(dir + "naif0007.tls");
    kernels.push_back(dir + "MGS_SCLKSCET.00045.tsc");
    kernels.push_back(dir + "moc13.ti");
    kernels.push_back(dir + "moc.bc");
    kernels.push_back(dir + "moc.bsp");
    kernels.push_back(dir + "de405.bsp");
    kernels.push_back(dir + "pck00009.tpc");
    kernels.push_back(dir + "mocSpiceRotationUnitTest.ti");
    kernels.push_back(dir + "ROS_V29.TF");
    kernels.push_back(dir + "CATT_DV_145_02_______00216.BC");
    for (QString& kernel : kernels) {
      naif->furnsh_c(kernel.toLatin1().data());
    }
  }

  void TearDown() {
    auto naif = NaifContext::acquire();
    
    for (QString& kernel : kernels) {
      naif->unload_c(kernel.toLatin1().data());
    }
  }
};

class SpiceRotationIsd : public ::testing::Test {
  protected:
    json isd;
    json isdAv;
    json isdConst;

  void SetUp() {
    isd = {{"ck_table_start_time"    , 0.0},
           {"ck_table_end_time"      , 3.0},
           {"ck_table_original_size" , 4},
           {"ephemeris_times"      , {0.0, 1.0, 2.0, 3.0}},
           {"time_dependent_frames" , {-94031, 10014, 1}},
           {"quaternions"         , {{0.0, 0.0, 0.0, 1.0},
                                     {-1.0 / sqrt(2), 0.0, 0.0, 1.0 / sqrt(2)},
                                     {0.0, 1.0 / sqrt(2), 1.0 / sqrt(2), 0.0},
                                     {-0.5, -0.5, 0.5, 0.5}}}};
    isdAv = isd;
    isdAv["angular_velocities"] = {{-Isis::PI / 2, 0.0, 0.0},
                                   {0.0, Isis::PI, 0.0},
                                   {0.0, 0.0, Isis::PI / 2},
                                   {0.0, 0.0, Isis::PI / 2}};
    isdConst = isd;
    isdConst["time_dependent_frames"] = {-94030, 10014, 1};
    isdConst["constant_frames"] = {-94031, -94030};
    isdConst["constant_rotation"] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0};
  }
};

TEST_F(SpiceRotationKernels, FromSpice) {
  auto naif = NaifContext::acquire();
  SpiceRotation rot(frameCode);

  // Start time
  rot.SetEphemerisTime(startTime, naif);
  EXPECT_DOUBLE_EQ(rot.EphemerisTime(), startTime);

  vector<double> startCJ = rot.Matrix(naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, startCJ,
                      (vector<double>{-0.87506927,  0.25477955, -0.41151081,
                                       0.011442263, 0.86088548, 0.50867009,
                                       0.48386242,  0.44041295, -0.75624969}),
                      testTolerance);

  ASSERT_TRUE(rot.HasAngularVelocity());
  vector<double> startAV = rot.AngularVelocity();
  EXPECT_PRED_FORMAT3(AssertVectorsNear, startAV,
                      (vector<double>{-1.3817139e-05, -0.0011493844, -0.00067443921}),
                      testTolerance);

  // Middle time
  rot.SetEphemerisTime(startTime + (4 * (endTime - startTime) / 9), naif);

  vector<double> midCJ = rot.Matrix(naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, midCJ,
                      (vector<double>{-0.77359018,  0.32985508, -0.54106734,
                                       0.010977931, 0.86068895,  0.50901279,
                                       0.63359113,  0.38782749, -0.66944164}),
                      testTolerance);

  ASSERT_TRUE(rot.HasAngularVelocity());
  vector<double> midAV = rot.AngularVelocity();
  EXPECT_PRED_FORMAT3(AssertVectorsNear, midAV,
                      (vector<double>{-1.4107831e-05, -0.0011349124, -0.0006662493}),
                      testTolerance);

  // End time
  rot.SetEphemerisTime(endTime, naif);

  vector<double> endCJ = rot.Matrix(naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, endCJ,
                      (vector<double>{-0.61729588,  0.4060182,  -0.67386573,
                                       0.010223693, 0.86060645,  0.50916796,
                                       0.78666465,  0.30741789, -0.53539982}),
                      testTolerance);

  EXPECT_TRUE(rot.HasAngularVelocity());
  vector<double> endAV = rot.AngularVelocity();
  EXPECT_PRED_FORMAT3(AssertVectorsNear, endAV,
                      (vector<double>{-1.2932496e-05, -0.0010747293, -0.00063276804}),
                      testTolerance);


  // Cache it
  rot.LoadCache(startTime, endTime, 10, naif);

  // Check start again
  rot.SetEphemerisTime(startTime, naif);

  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif), startCJ, testTolerance);

  ASSERT_TRUE(rot.HasAngularVelocity());
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.AngularVelocity(), startAV, testTolerance);

  // Check middle again
  rot.SetEphemerisTime(startTime + (4 * (endTime - startTime) / 9), naif);

  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif), midCJ, testTolerance);

  ASSERT_TRUE(rot.HasAngularVelocity());
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.AngularVelocity(), midAV, testTolerance);

  // Check end again
  rot.SetEphemerisTime(endTime, naif);

  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif), endCJ, testTolerance);

  ASSERT_TRUE(rot.HasAngularVelocity());
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.AngularVelocity(), endAV, testTolerance);


  // Fit polynomial
  rot.SetPolynomial(naif);

  // Check start again
  rot.SetEphemerisTime(startTime, naif);

  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif),
                      (vector<double>{-0.87506744, 0.25462094, -0.41161286,
                                      0.011738947, 0.86135321,  0.5078709,
                                      0.48385863,  0.43958939, -0.75673113}),
                      testTolerance);

  ASSERT_TRUE(rot.HasAngularVelocity());
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.AngularVelocity(),
                      (vector<double>{3.9588092e-05, -0.0011571406, -0.00066422493}),
                      testTolerance);

  // Check middle again
  rot.SetEphemerisTime(startTime + (4 * (endTime - startTime) / 9), naif);

  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif),
                      (vector<double>{-0.77358897,  0.32991801, -0.54103069,
                                       0.010878267, 0.86056939,  0.50921703,
                                       0.63359432,  0.3880392,  -0.66931593}),
                      testTolerance);

  ASSERT_TRUE(rot.HasAngularVelocity());
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.AngularVelocity(),
                      (vector<double>{-2.8366393e-05, -0.0011306014, -0.00067058131}),
                      testTolerance);

  // Check end again
  rot.SetEphemerisTime(endTime, naif);

  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif),
                      (vector<double>{-0.61722064,   0.40639527, -0.67370733,
                                       0.0096837405, 0.86013226,  0.50997914,
                                       0.78673052,   0.30824564, -0.53482681}),
                      testTolerance);

  ASSERT_TRUE(rot.HasAngularVelocity());
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.AngularVelocity(),
                      (vector<double>{3.8816777e-05, -0.0010934565, -0.00061098396}),
                      testTolerance);
}


TEST_F(SpiceRotationKernels, Nadir) {
  auto naif = NaifContext::acquire();
  SpiceRotation rot(frameCode, targetCode);

  rot.SetEphemerisTime(startTime, naif);

  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif),
                      (vector<double>{-0.87397636,  0.25584047, -0.41317186,
                                       0.011529483, 0.86087973,  0.50867786,
                                       0.48583166,  0.43980876, -0.75533824}),
                      testTolerance);
}


TEST_F(SpiceRotationKernels, Pck) {
  auto naif = NaifContext::acquire();
  SpiceRotation ioRot(10023); // Use IO because it has nutation/precession
  ioRot.LoadCache(-15839262.24291, -15839262.24291, 1, naif);

  EXPECT_EQ(ioRot.getFrameType(), SpiceRotation::PCK);

  // These are angles so we can't use vector comparison
  vector<Angle> poleRa = ioRot.poleRaCoefs();
  EXPECT_EQ(poleRa.size(), 3);
  EXPECT_NEAR(poleRa[0].degrees(), 268.05, testTolerance);
  EXPECT_NEAR(poleRa[1].degrees(), -0.009, testTolerance);
  EXPECT_NEAR(poleRa[2].degrees(), 0.0, testTolerance);

  vector<Angle> poleDec = ioRot.poleDecCoefs();
  EXPECT_EQ(poleDec.size(), 3);
  EXPECT_NEAR(poleDec[0].degrees(), 64.5, testTolerance);
  EXPECT_NEAR(poleDec[1].degrees(), 0.003, testTolerance);
  EXPECT_NEAR(poleDec[2].degrees(), 0.0, testTolerance);

  vector<Angle> prMer = ioRot.pmCoefs();
  EXPECT_EQ(prMer.size(), 3);
  EXPECT_NEAR(prMer[0].degrees(), 200.39, testTolerance);
  EXPECT_NEAR(prMer[1].degrees(), 203.4889538, testTolerance);
  EXPECT_NEAR(prMer[2].degrees(), 0.0, testTolerance);

  vector<Angle> sysNutPrec0 = ioRot.sysNutPrecConstants();
  EXPECT_EQ(sysNutPrec0.size(), 15);
  EXPECT_NEAR(sysNutPrec0[0].degrees(), 73.32, testTolerance);
  EXPECT_NEAR(sysNutPrec0[1].degrees(), 24.62, testTolerance);
  EXPECT_NEAR(sysNutPrec0[2].degrees(), 283.9, testTolerance);
  EXPECT_NEAR(sysNutPrec0[3].degrees(), 355.8, testTolerance);
  EXPECT_NEAR(sysNutPrec0[4].degrees(), 119.9, testTolerance);
  EXPECT_NEAR(sysNutPrec0[5].degrees(), 229.8, testTolerance);
  EXPECT_NEAR(sysNutPrec0[6].degrees(), 352.25, testTolerance);
  EXPECT_NEAR(sysNutPrec0[7].degrees(), 113.35, testTolerance);
  EXPECT_NEAR(sysNutPrec0[8].degrees(), 146.64, testTolerance);
  EXPECT_NEAR(sysNutPrec0[9].degrees(), 49.24, testTolerance);
  EXPECT_NEAR(sysNutPrec0[10].degrees(), 99.360714, testTolerance);
  EXPECT_NEAR(sysNutPrec0[11].degrees(), 175.895369, testTolerance);
  EXPECT_NEAR(sysNutPrec0[12].degrees(), 300.323162, testTolerance);
  EXPECT_NEAR(sysNutPrec0[13].degrees(), 114.012305, testTolerance);
  EXPECT_NEAR(sysNutPrec0[14].degrees(), 49.511251, testTolerance);

  vector<Angle> sysNutPrec1 = ioRot.sysNutPrecCoefs();
  EXPECT_EQ(sysNutPrec1.size(), 15);
  EXPECT_NEAR(sysNutPrec1[0].degrees(), 91472.9, testTolerance);
  EXPECT_NEAR(sysNutPrec1[1].degrees(), 45137.2, testTolerance);
  EXPECT_NEAR(sysNutPrec1[2].degrees(), 4850.7, testTolerance);
  EXPECT_NEAR(sysNutPrec1[3].degrees(), 1191.3, testTolerance);
  EXPECT_NEAR(sysNutPrec1[4].degrees(), 262.1, testTolerance);
  EXPECT_NEAR(sysNutPrec1[5].degrees(), 64.3, testTolerance);
  EXPECT_NEAR(sysNutPrec1[6].degrees(), 2382.6, testTolerance);
  EXPECT_NEAR(sysNutPrec1[7].degrees(), 6070, testTolerance);
  EXPECT_NEAR(sysNutPrec1[8].degrees(), 182945.8, testTolerance);
  EXPECT_NEAR(sysNutPrec1[9].degrees(), 90274.4, testTolerance);
  EXPECT_NEAR(sysNutPrec1[10].degrees(), 4850.4046, testTolerance);
  EXPECT_NEAR(sysNutPrec1[11].degrees(), 1191.9605, testTolerance);
  EXPECT_NEAR(sysNutPrec1[12].degrees(), 262.5475, testTolerance);
  EXPECT_NEAR(sysNutPrec1[13].degrees(), 6070.2476, testTolerance);
  EXPECT_NEAR(sysNutPrec1[14].degrees(), 64.3, testTolerance);

  // These are doubles so we can use vector comparison
  EXPECT_PRED_FORMAT3(AssertVectorsNear, ioRot.poleRaNutPrecCoefs(),
                      (vector<double>{0.0, 0.0, 0.094, 0.024, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0}),
                      testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, ioRot.poleDecNutPrecCoefs(),
                      (vector<double>{0.0, 0.0, 0.04, 0.011, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0}),
                      testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, ioRot.pmNutPrecCoefs(),
                      (vector<double>{0.0, 0.0, -0.085, -0.022, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0,
                                      0.0, 0.0, 0.0, 0.0, 0.0}),
                      testTolerance);
}


TEST_F(SpiceRotationIsd, FromALE) {
  auto naif = NaifContext::acquire();
  // Test with just a time dependent rotation
  SpiceRotation aleQuatRot(-94031);
  aleQuatRot.LoadCache(isd, naif);

  EXPECT_EQ(aleQuatRot.getFrameType(), SpiceRotation::CK);
  EXPECT_TRUE(aleQuatRot.IsCached());
  EXPECT_FALSE(aleQuatRot.HasAngularVelocity());

  vector<int> timeDepChain = aleQuatRot.TimeFrameChain();
  EXPECT_EQ(timeDepChain.size(), 3);
  EXPECT_EQ(timeDepChain[0], -94031);
  EXPECT_EQ(timeDepChain[1], 10014);
  EXPECT_EQ(timeDepChain[2], 1);

  aleQuatRot.SetEphemerisTime(0.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatRot.Matrix(naif),
                      (vector<double>{-1.0,  0.0, 0.0,
                                       0.0, -1.0, 0.0,
                                       0.0,  0.0, 1.0}),
                      testTolerance);

  aleQuatRot.SetEphemerisTime(1.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatRot.Matrix(naif),
                      (vector<double>{0.0, 1.0, 0.0,
                                     -1.0, 0.0, 0.0,
                                      0.0, 0.0, 1.0}),
                      testTolerance);

  aleQuatRot.SetEphemerisTime(2.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatRot.Matrix(naif),
                      (vector<double>{0.0, 1.0,  0.0,
                                      1.0, 0.0,  0.0,
                                      0.0, 0.0, -1.0}),
                      testTolerance);

  aleQuatRot.SetEphemerisTime(3.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatRot.Matrix(naif),
                      (vector<double>{0.0, 0.0, -1.0,
                                     -1.0, 0.0,  0.0,
                                      0.0, 1.0,  0.0}),
                      testTolerance);


  // Test with angular velocity
  SpiceRotation aleQuatAVRot(-94031);
  aleQuatAVRot.LoadCache(isdAv, naif);

  ASSERT_TRUE(aleQuatAVRot.HasAngularVelocity());

  aleQuatAVRot.SetEphemerisTime(0.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatAVRot.AngularVelocity(),
                      (vector<double>{-Isis::PI / 2.0, 0.0,  0.0}), testTolerance);

  aleQuatAVRot.SetEphemerisTime(1.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatAVRot.AngularVelocity(),
                      (vector<double>{0.0, Isis::PI, 0.0}), testTolerance);

  aleQuatAVRot.SetEphemerisTime(2.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatAVRot.AngularVelocity(),
                      (vector<double>{0.0, 0.0, Isis::PI / 2.0}), testTolerance);;

  aleQuatAVRot.SetEphemerisTime(3.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatAVRot.AngularVelocity(),
                      (vector<double>{0.0, 0.0, Isis::PI / 2.0}), testTolerance);


  // Test with a constant rotation
  SpiceRotation aleQuatConstRot(-94031);
  aleQuatConstRot.LoadCache(isdConst, naif);

  vector<int> constChain = aleQuatConstRot.ConstantFrameChain();
  EXPECT_EQ(constChain.size(), 2);
  EXPECT_EQ(constChain[0], -94031);
  EXPECT_EQ(constChain[1], -94030);

  aleQuatConstRot.SetEphemerisTime(0.0, naif);
  aleQuatRot.SetEphemerisTime(0.0, naif);
  vector<double> oldCJ = aleQuatRot.Matrix(naif);
  // The constant rotation should swap Y and Z
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatConstRot.Matrix(naif),
                     (vector<double>{oldCJ[0], oldCJ[1], oldCJ[2],
                                     oldCJ[6], oldCJ[7], oldCJ[8],
                                     oldCJ[3], oldCJ[4], oldCJ[5]}),
                     testTolerance);

  aleQuatConstRot.SetEphemerisTime(1.0, naif);
  aleQuatRot.SetEphemerisTime(1.0, naif);
  oldCJ = aleQuatRot.Matrix(naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatConstRot.Matrix(naif),
                     (vector<double>{oldCJ[0], oldCJ[1], oldCJ[2],
                                     oldCJ[6], oldCJ[7], oldCJ[8],
                                     oldCJ[3], oldCJ[4], oldCJ[5]}),
                     testTolerance);

  aleQuatConstRot.SetEphemerisTime(2.0, naif);
  aleQuatRot.SetEphemerisTime(2.0, naif);
  oldCJ = aleQuatRot.Matrix(naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatConstRot.Matrix(naif),
                     (vector<double>{oldCJ[0], oldCJ[1], oldCJ[2],
                                     oldCJ[6], oldCJ[7], oldCJ[8],
                                     oldCJ[3], oldCJ[4], oldCJ[5]}),
                     testTolerance);

  aleQuatConstRot.SetEphemerisTime(3.0, naif);
  aleQuatRot.SetEphemerisTime(3.0, naif);
  oldCJ = aleQuatRot.Matrix(naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, aleQuatConstRot.Matrix(naif),
                     (vector<double>{oldCJ[0], oldCJ[1], oldCJ[2],
                                     oldCJ[6], oldCJ[7], oldCJ[8],
                                     oldCJ[3], oldCJ[4], oldCJ[5]}),
                     testTolerance);
}


TEST_F(SpiceRotationIsd, Cache) {
  auto naif = NaifContext::acquire();
  SpiceRotation rot(-94031);
  rot.LoadCache(isd, naif);
  Table rotTable = rot.Cache("TestCache", naif);

  SpiceRotation newRot(-94031);
  newRot.LoadCache(rotTable, naif);

  rot.SetEphemerisTime(0.0, naif);
  newRot.SetEphemerisTime(0.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif), newRot.Matrix(naif), testTolerance);

  rot.SetEphemerisTime(1.0, naif);
  newRot.SetEphemerisTime(1.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif), newRot.Matrix(naif), testTolerance);

  rot.SetEphemerisTime(2.0, naif);
  newRot.SetEphemerisTime(2.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif), newRot.Matrix(naif), testTolerance);

  rot.SetEphemerisTime(3.0, naif);
  newRot.SetEphemerisTime(3.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif), newRot.Matrix(naif), testTolerance);
}


TEST_F(SpiceRotationIsd, LineCache) {
  auto naif = NaifContext::acquire();
  SpiceRotation polyRot(-94031);
  polyRot.LoadCache(isd, naif);
  polyRot.ComputeBaseTime();
  polyRot.SetPolynomialDegree(naif, 1);
  // The base time is set to 1.5, and the time scale is set to 1.5 so these
  // coefficients are scaled accordingly. The unscaled equations are:
  // angle1 = -pi/2 + pi/2 * t
  // angle2 = -pi   + pi/2 * t
  // angle3 =  pi   - pi/2 * t
  //
  // Note ISIS defaults to ZXZ rotation axis order
  vector<double> angle1Coeffs = {Isis::PI / 4.0, 3.0 * Isis::PI / 4.0};
  vector<double> angle2Coeffs = {-Isis::PI / 4.0, 3.0 * Isis::PI / 4.0};
  vector<double> angle3Coeffs = {Isis::PI / 4.0, -3.0 * Isis::PI / 4.0};
  polyRot.SetPolynomial(naif, angle1Coeffs, angle2Coeffs, angle3Coeffs, SpiceRotation::PolyFunction);

  // LineCache converts the SpiceRotation from a polynomial into a cache so save off these now
  polyRot.SetEphemerisTime(0.0, naif);
  vector<double> cj0 = polyRot.Matrix(naif);

  polyRot.SetEphemerisTime(1.0, naif);
  vector<double> cj1 = polyRot.Matrix(naif);

  polyRot.SetEphemerisTime(2.0, naif);
  vector<double> cj2 = polyRot.Matrix(naif);

  polyRot.SetEphemerisTime(3.0, naif);
  vector<double> cj3 = polyRot.Matrix(naif);

  Table rotTable = polyRot.LineCache("TestCache", naif);
  SpiceRotation newRot(-94031);
  newRot.LoadCache(rotTable, naif);

  polyRot.SetEphemerisTime(0.0, naif);
  newRot.SetEphemerisTime(0.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, polyRot.Matrix(naif), cj0, testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, newRot.Matrix(naif), cj0, testTolerance);

  polyRot.SetEphemerisTime(1.0, naif);
  newRot.SetEphemerisTime(1.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, polyRot.Matrix(naif), cj1, testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, newRot.Matrix(naif), cj1, testTolerance);

  polyRot.SetEphemerisTime(2.0, naif);
  newRot.SetEphemerisTime(2.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, polyRot.Matrix(naif), cj2, testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, newRot.Matrix(naif), cj2, testTolerance);

  polyRot.SetEphemerisTime(3.0, naif);
  newRot.SetEphemerisTime(3.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, polyRot.Matrix(naif), cj3, testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, newRot.Matrix(naif), cj3, testTolerance);
}

TEST_F(SpiceRotationIsd, PolyCache) {
  auto naif = NaifContext::acquire();
  SpiceRotation polyRot(-94031);
  polyRot.LoadCache(isd, naif);
  polyRot.ComputeBaseTime();
  polyRot.SetPolynomialDegree(naif, 1);
  // The base time is set to 1.5, and the time scale is set to 1.5 so these
  // coefficients are scaled accordingly. The unscaled equations are:
  // angle1 = -pi/2 + pi/2 * t
  // angle2 = -pi   + pi/2 * t
  // angle3 =  pi   - pi/2 * t
  //
  // Note ISIS defaults to ZXZ rotation axis order
  vector<double> angle1Coeffs = {Isis::PI / 4.0, 3.0 * Isis::PI / 4.0};
  vector<double> angle2Coeffs = {-Isis::PI / 4.0, 3.0 * Isis::PI / 4.0};
  vector<double> angle3Coeffs = {Isis::PI / 4.0, -3.0 * Isis::PI / 4.0};
  polyRot.SetPolynomial(naif, angle1Coeffs, angle2Coeffs, angle3Coeffs, SpiceRotation::PolyFunction);

  Table rotTable = polyRot.Cache("TestCache", naif);
  SpiceRotation newRot(-94031);
  newRot.LoadCache(rotTable, naif);

  EXPECT_EQ(polyRot.GetSource(), newRot.GetSource());
  EXPECT_NEAR(polyRot.GetBaseTime(), newRot.GetBaseTime(), testTolerance);
  EXPECT_NEAR(polyRot.GetTimeScale(), newRot.GetTimeScale(), testTolerance);

  polyRot.SetEphemerisTime(0.0, naif);
  newRot.SetEphemerisTime(0.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, polyRot.Matrix(naif), newRot.Matrix(naif), testTolerance);

  polyRot.SetEphemerisTime(1.0, naif);
  newRot.SetEphemerisTime(1.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, polyRot.Matrix(naif), newRot.Matrix(naif), testTolerance);

  polyRot.SetEphemerisTime(2.0, naif);
  newRot.SetEphemerisTime(2.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, polyRot.Matrix(naif), newRot.Matrix(naif), testTolerance);

  polyRot.SetEphemerisTime(3.0, naif);
  newRot.SetEphemerisTime(3.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, polyRot.Matrix(naif), newRot.Matrix(naif), testTolerance);
}


TEST_F(SpiceRotationIsd, PolyOverCache) {
  auto naif = NaifContext::acquire();
  SpiceRotation rot(-94031);
  rot.LoadCache(isd, naif);
  rot.ComputeBaseTime();
  rot.SetPolynomialDegree(naif, 1);
  // The base time is set to 1.5, and the time scale is set to 1.5 so these
  // coefficients are scaled to be -90 at 0, 0 at 1, 90 at 2, and 180 at 3.
  vector<double> angle1Coeffs = {Isis::PI / 4, 3 * Isis::PI / 4};
  vector<double> angle2Coeffs = {0.0, 0.0};
  vector<double> angle3Coeffs = {0.0, 0.0};
  rot.SetPolynomial(naif, angle1Coeffs, angle2Coeffs, angle3Coeffs, SpiceRotation::PolyFunctionOverSpice);

  rot.SetEphemerisTime(0.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif),
                      (vector<double>{0.0, 1.0, 0.0,
                                     -1.0, 0.0, 0.0,
                                      0.0, 0.0, 1.0}),
                      testTolerance);

  rot.SetEphemerisTime(1.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif),
                      (vector<double>{0.0, 1.0, 0.0,
                                     -1.0, 0.0, 0.0,
                                      0.0, 0.0, 1.0}),
                      testTolerance);

  rot.SetEphemerisTime(2.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif),
                      (vector<double>{-1.0, 0.0,  0.0,
                                       0.0, 1.0,  0.0,
                                       0.0, 0.0, -1.0}),
                      testTolerance);

  rot.SetEphemerisTime(3.0, naif);
  EXPECT_PRED_FORMAT3(AssertVectorsNear, rot.Matrix(naif),
                      (vector<double>{0.0,  0.0, -1.0,
                                      1.0,  0.0,  0.0,
                                      0.0, -1.0,  0.0}),
                      testTolerance);
}


TEST_F(SpiceRotationIsd, VectorRotation) {
  auto naif = NaifContext::acquire();
  SpiceRotation rot(-94031);
  rot.LoadCache(isd, naif);

  vector<double> unitX = {1.0, 0.0, 0.0};
  vector<double> unitY = {0.0, 1.0, 0.0};
  vector<double> unitZ = {0.0, 0.0, 1.0};

  rot.SetEphemerisTime(1.0, naif);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.J2000Vector(unitX, naif),
                      unitY, testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.J2000Vector(unitY, naif),
                      (vector<double>{-1.0, 0.0, 0.0}), testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.J2000Vector(unitZ, naif),
                      unitZ, testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ReferenceVector(unitX, naif),
                      (vector<double>{0.0, -1.0, 0.0}), testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ReferenceVector(unitY, naif),
                      unitX, testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ReferenceVector(unitZ, naif),
                      unitZ, testTolerance);
}


TEST_F(SpiceRotationIsd, PolynomialPartials) {
  auto naif = NaifContext::acquire();
  SpiceRotation rot(-94031);
  rot.LoadCache(isd, naif);
  rot.ComputeBaseTime();
  rot.SetPolynomialDegree(naif, 1);
  // The base time is set to 1.5, and the time scale is set to 1.5 so these
  // coefficients are scaled accordingly. The unscaled equations are:
  // angle1 = -pi/2 + pi/2 * t
  // angle2 = -pi   + pi/2 * t
  // angle3 =  pi   - pi/2 * t
  //
  // Note ISIS defaults to ZXZ rotation axis order
  vector<double> angle1Coeffs = {Isis::PI / 4.0, 3.0 * Isis::PI / 4.0};
  vector<double> angle2Coeffs = {-Isis::PI / 4.0, 3.0 * Isis::PI / 4.0};
  vector<double> angle3Coeffs = {Isis::PI / 4.0, -3.0 * Isis::PI / 4.0};
  rot.SetPolynomial(naif, angle1Coeffs, angle2Coeffs, angle3Coeffs, SpiceRotation::PolyFunction);

  // At t = 1.0, the angles are:
  // angle1 = 0.0
  // angle2 = -pi/2
  // angle3 = pi/2
  rot.SetEphemerisTime(1.0, naif);

  // Test each unit vector which should map to the columns of the jacobian for
  // ToReferencePartial and the rows of the Jacobian for ToJ2000Partial.
  //
  // For the linear coefficient the Jacobian is multiplied by scaled_t = -1 / 3.
  vector<double> unitX = {1.0, 0.0, 0.0};
  vector<double> unitY = {0.0, 1.0, 0.0};
  vector<double> unitZ = {0.0, 0.0, 1.0};

  // Partials wrt angle 1
  // Jacobian matrix is
  //  0  0  0
  //  0 -1  0
  // -1  0  0
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitX, SpiceRotation::WRT_RightAscension, 0, naif),
                      (vector<double>{0.0, 0.0, -1.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitX, SpiceRotation::WRT_RightAscension, 1, naif),
                      (vector<double>{0.0, 0.0, 1.0 / 3.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitX, SpiceRotation::WRT_RightAscension, 0, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitX, SpiceRotation::WRT_RightAscension, 1, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitY, SpiceRotation::WRT_RightAscension, 0, naif),
                      (vector<double>{0.0, -1.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitY, SpiceRotation::WRT_RightAscension, 1, naif),
                      (vector<double>{0.0, 1.0 / 3.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitY, SpiceRotation::WRT_RightAscension, 0, naif),
                      (vector<double>{0.0, -1.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitY, SpiceRotation::WRT_RightAscension, 1, naif),
                      (vector<double>{0.0, 1.0 / 3.0, 0.0}), testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitZ, SpiceRotation::WRT_RightAscension, 0, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitZ, SpiceRotation::WRT_RightAscension, 1, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitZ, SpiceRotation::WRT_RightAscension, 0, naif),
                      (vector<double>{-1.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitZ, SpiceRotation::WRT_RightAscension, 1, naif),
                      (vector<double>{1.0 / 3.0, 0.0, 0.0}), testTolerance);


  // Partials wrt angle 2
  // Jacobian matrix is
  //  0 -1  0
  //  0  0  0
  //  0  0 -1
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitX, SpiceRotation::WRT_Declination, 0, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitX, SpiceRotation::WRT_Declination, 1, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitX, SpiceRotation::WRT_Declination, 0, naif),
                      (vector<double>{0.0, -1.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitX, SpiceRotation::WRT_Declination, 1, naif),
                      (vector<double>{0.0, 1.0 / 3.0, 0.0}), testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitY, SpiceRotation::WRT_Declination, 0, naif),
                      (vector<double>{-1.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitY, SpiceRotation::WRT_Declination, 1, naif),
                      (vector<double>{1.0 / 3.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitY, SpiceRotation::WRT_Declination, 0, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitY, SpiceRotation::WRT_Declination, 1, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitZ, SpiceRotation::WRT_Declination, 0, naif),
                      (vector<double>{0.0, 0.0, -1.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitZ, SpiceRotation::WRT_Declination, 1, naif),
                      (vector<double>{0.0, 0.0, 1.0 / 3.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitZ, SpiceRotation::WRT_Declination, 0, naif),
                      (vector<double>{0.0, 0.0, -1.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitZ, SpiceRotation::WRT_Declination, 1, naif),
                      (vector<double>{0.0, 0.0, 1.0 / 3.0}), testTolerance);


  // Partials wrt angle 3
  // Jacobian matrix is
  // -1  0  0
  //  0  0  1
  //  0  0  0
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitX, SpiceRotation::WRT_Twist, 0, naif),
                      (vector<double>{-1.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitX, SpiceRotation::WRT_Twist, 1, naif),
                      (vector<double>{1.0 / 3.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitX, SpiceRotation::WRT_Twist, 0, naif),
                      (vector<double>{-1.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitX, SpiceRotation::WRT_Twist, 1, naif),
                      (vector<double>{1.0 / 3.0, 0.0, 0.0}), testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitY, SpiceRotation::WRT_Twist, 0, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitY, SpiceRotation::WRT_Twist, 1, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitY, SpiceRotation::WRT_Twist, 0, naif),
                      (vector<double>{0.0, 0.0, 1.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitY, SpiceRotation::WRT_Twist, 1, naif),
                      (vector<double>{0.0, 0.0, -1.0 / 3.0}), testTolerance);

  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitZ, SpiceRotation::WRT_Twist, 0, naif),
                      (vector<double>{0.0, 1.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.ToReferencePartial(unitZ, SpiceRotation::WRT_Twist, 1, naif),
                      (vector<double>{0.0, -1.0 / 3.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitZ, SpiceRotation::WRT_Twist, 0, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
  EXPECT_PRED_FORMAT3(AssertVectorsNear,
                      rot.toJ2000Partial(unitZ, SpiceRotation::WRT_Twist, 1, naif),
                      (vector<double>{0.0, 0.0, 0.0}), testTolerance);
}

TEST(SpiceRotation, WrapAngle) {
  auto naif = NaifContext::acquire();
  SpiceRotation rot(-94031);

  EXPECT_NEAR(rot.WrapAngle(Isis::PI / 6.0, 4.0 * Isis::PI / 3.0, naif),
              -2.0 * Isis::PI / 3.0, testTolerance);
  EXPECT_NEAR(rot.WrapAngle(Isis::PI / 6.0, -1.0 * Isis::PI / 18.0, naif),
              -1.0 * Isis::PI / 18.0, testTolerance);
  EXPECT_NEAR(rot.WrapAngle(Isis::PI / 6.0, -1.0 * Isis::PI, naif),
              Isis::PI, testTolerance);
  EXPECT_NEAR(rot.WrapAngle(Isis::PI / 6.0, Isis::PI / 2.0, naif),
              Isis::PI / 2.0, testTolerance);
}
