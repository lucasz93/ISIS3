#include <QString>
#include <iostream>

#include "csm/csm.h"
#include "csm/Ellipsoid.h"

#include "CSMCamera.h"
#include "Fixtures.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MockCsmPlugin.h"
#include "Mocks.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "Fixtures.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "gmock/gmock.h"

using namespace Isis;

TEST_F(CSMCameraFixture, SetImage) {
  auto naif = NaifContext::acquire();
  csm::Ellipsoid wgs84;
  EXPECT_CALL(mockModel, imageToRemoteImagingLocus(MatchImageCoord(csm::ImageCoord(4.5, 4.5)), ::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      // looking straight down X-Axis
      .WillOnce(::testing::Return(csm::EcefLocus(wgs84.getSemiMajorRadius() + 50000, 0, 0, -1, 0, 0)));
  EXPECT_CALL(mockModel, getImageTime)
      .Times(1)
      .WillOnce(::testing::Return(10.0));

  EXPECT_TRUE(testCam->SetImage(5, 5, naif));
  EXPECT_EQ(testCam->UniversalLatitude(), 0.0);
  EXPECT_EQ(testCam->UniversalLongitude(), 0.0);
  EXPECT_THAT(testCam->lookDirectionBodyFixed(), ::testing::ElementsAre(-1.0, 0.0, 0.0));

  iTime refTime("2000-01-01T11:58:55.816");
  EXPECT_EQ((refTime + 10.0).Et(), testCam->time().Et());
}


TEST_F(CSMCameraFixture, SetImageNoIntersect) {
  auto naif = NaifContext::acquire();
  csm::Ellipsoid wgs84;
  EXPECT_CALL(mockModel, imageToRemoteImagingLocus(MatchImageCoord(csm::ImageCoord(4.5, 4.5)), ::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      // looking straight down X-Axis
      .WillOnce(::testing::Return(csm::EcefLocus(wgs84.getSemiMajorRadius() + 50000, 0, 0, 0, 1, 0)));

  EXPECT_FALSE(testCam->SetImage(5, 5, naif));
  EXPECT_THAT(testCam->lookDirectionBodyFixed(), ::testing::ElementsAre(0.0, 1.0, 0.0));
}


TEST_F(CSMCameraDemFixture, SetImage) {
  auto naif = NaifContext::acquire();
  EXPECT_CALL(mockModel, imageToRemoteImagingLocus(MatchImageCoord(csm::ImageCoord(4.5, 4.5)), ::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      // looking straight down X-Axis
      .WillOnce(::testing::Return(csm::EcefLocus(demRadius + 50000, 0, 0, -1, 0, 0)));
  EXPECT_CALL(mockModel, computeGroundPartials)
      .WillRepeatedly(::testing::Return(std::vector<double>{1, 2, 3, 4, 5, 6}));
  EXPECT_CALL(mockModel, getImageTime)
      .Times(1)
      .WillOnce(::testing::Return(10.0));

  testCam->SetImage(5, 5, naif);
  EXPECT_EQ(testCam->UniversalLatitude(), 0.0);
  EXPECT_EQ(testCam->UniversalLongitude(), 0.0);
}


TEST_F(CSMCameraFixture, SetGround) {
  auto naif = NaifContext::acquire();
  
  // Define some things to match/return
  csm::Ellipsoid wgs84;
  csm::ImageCoord imagePt(4.5, 4.5);
  csm::EcefCoord groundPt(wgs84.getSemiMajorRadius(), 0, 0);
  csm::EcefLocus imageLocus(wgs84.getSemiMajorRadius() + 50000, 0, 0, -1, 0, 0);

  // Setup expected calls/returns
  EXPECT_CALL(mockModel, groundToImage(MatchEcefCoord(groundPt), ::testing::_, ::testing::_, ::testing::_))
      .Times(4)
      .WillRepeatedly(::testing::Return(imagePt));
  EXPECT_CALL(mockModel, imageToRemoteImagingLocus(MatchImageCoord(imagePt), ::testing::_, ::testing::_, ::testing::_))
      .Times(4)
      .WillRepeatedly(::testing::Return(imageLocus));
  EXPECT_CALL(mockModel, getImageTime)
      .Times(4)
      .WillRepeatedly(::testing::Return(10.0));

  iTime refTime("2000-01-01T11:58:55.816");

  EXPECT_TRUE(testCam->SetGround(naif, Latitude(0.0, Angle::Degrees), Longitude(0.0, Angle::Degrees)));
  EXPECT_EQ(testCam->Line(), 5.0);
  EXPECT_EQ(testCam->Sample(), 5.0);
  EXPECT_EQ((refTime + 10.0).Et(), testCam->time().Et());
  EXPECT_THAT(testCam->lookDirectionBodyFixed(), ::testing::ElementsAre(-1.0, 0.0, 0.0));

  EXPECT_TRUE(testCam->SetGround(naif, SurfacePoint(naif, Latitude(0.0, Angle::Degrees),
                                 Longitude(0.0, Angle::Degrees),
                                 Distance(wgs84.getSemiMajorRadius(), Distance::Meters))));
  EXPECT_EQ(testCam->Line(), 5.0);
  EXPECT_EQ(testCam->Sample(), 5.0);
  EXPECT_EQ((refTime + 10.0).Et(), testCam->time().Et());
  EXPECT_THAT(testCam->lookDirectionBodyFixed(), ::testing::ElementsAre(-1.0, 0.0, 0.0));

  EXPECT_TRUE(testCam->SetUniversalGround(naif, 0.0, 0.0));
  EXPECT_EQ(testCam->Line(), 5.0);
  EXPECT_EQ(testCam->Sample(), 5.0);
  EXPECT_EQ((refTime + 10.0).Et(), testCam->time().Et());
  EXPECT_THAT(testCam->lookDirectionBodyFixed(), ::testing::ElementsAre(-1.0, 0.0, 0.0));

  EXPECT_TRUE(testCam->SetUniversalGround(naif, 0.0, 0.0, wgs84.getSemiMajorRadius()));
  EXPECT_EQ(testCam->Line(), 5.0);
  EXPECT_EQ(testCam->Sample(), 5.0);
  EXPECT_EQ((refTime + 10.0).Et(), testCam->time().Et());
  EXPECT_THAT(testCam->lookDirectionBodyFixed(), ::testing::ElementsAre(-1.0, 0.0, 0.0));
}


TEST_F(CSMCameraDemFixture, SetGround) {
  auto naif = NaifContext::acquire();
  
  // Define some things to match/return
  csm::ImageCoord imagePt(4.5, 4.5);
  csm::EcefCoord groundPt(demRadius, 0, 0);
  csm::EcefLocus imageLocus(demRadius + 50000, 0, 0, -1, 0, 0);

  // Setup expected calls/returns
  EXPECT_CALL(mockModel, groundToImage(MatchEcefCoord(groundPt), ::testing::_, ::testing::_, ::testing::_))
      .Times(4)
      .WillRepeatedly(::testing::Return(imagePt));
  EXPECT_CALL(mockModel, imageToRemoteImagingLocus(MatchImageCoord(imagePt), ::testing::_, ::testing::_, ::testing::_))
      .Times(4)
      .WillRepeatedly(::testing::Return(imageLocus));
  EXPECT_CALL(mockModel, getImageTime)
      .Times(4)
      .WillRepeatedly(::testing::Return(10.0));

  EXPECT_TRUE(testCam->SetGround(naif, Latitude(0.0, Angle::Degrees), Longitude(0.0, Angle::Degrees)));
  EXPECT_EQ(testCam->Line(), 5.0);
  EXPECT_EQ(testCam->Sample(), 5.0);

  EXPECT_TRUE(testCam->SetGround(naif, SurfacePoint(naif, Latitude(0.0, Angle::Degrees),
                                 Longitude(0.0, Angle::Degrees),
                                 Distance(demRadius, Distance::Meters))));
  EXPECT_EQ(testCam->Line(), 5.0);
  EXPECT_EQ(testCam->Sample(), 5.0);

  EXPECT_TRUE(testCam->SetUniversalGround(naif, 0.0, 0.0));
  EXPECT_EQ(testCam->Line(), 5.0);
  EXPECT_EQ(testCam->Sample(), 5.0);

  EXPECT_TRUE(testCam->SetUniversalGround(naif, 0.0, 0.0, demRadius));
  EXPECT_EQ(testCam->Line(), 5.0);
  EXPECT_EQ(testCam->Sample(), 5.0);
}


TEST_F(CSMCameraSetFixture, Resolution) {
  auto naif = NaifContext::acquire();
  
  // Setup to return the ground partials we want
  // The pseudoinverse of:
  // 1 2 3
  // 4 5 6
  //
  // is
  // -17  8
  //  -2  2  *  1/18
  //  13 -4
  EXPECT_CALL(mockModel, computeGroundPartials)
      .Times(6)
      .WillRepeatedly(::testing::Return(std::vector<double>{1, 2, 3, 4, 5, 6}));

  // Use expect near here because the psuedoinverse calculation is only accurate to ~1e-10
  double expectedLineRes = sqrt(17*17 + 2*2 + 13*13)/18;
  double expectedSampRes = sqrt(8*8 + 2*2 + 4*4)/18;
  EXPECT_NEAR(testCam->LineResolution(naif), expectedLineRes, 1e-10);
  EXPECT_NEAR(testCam->ObliqueLineResolution(naif), expectedLineRes, 1e-10);
  EXPECT_NEAR(testCam->SampleResolution(naif), expectedSampRes, 1e-10);
  EXPECT_NEAR(testCam->ObliqueSampleResolution(naif), expectedSampRes, 1e-10);
  EXPECT_NEAR(testCam->DetectorResolution(naif), (expectedLineRes+expectedSampRes) / 2.0, 1e-10);
  EXPECT_NEAR(testCam->ObliqueDetectorResolution(naif), (expectedLineRes+expectedSampRes) / 2.0, 1e-10);
}


TEST_F(CSMCameraSetFixture, InstrumentBodyFixedPosition) {
  auto naif = NaifContext::acquire();
  EXPECT_CALL(mockModel, getSensorPosition(MatchImageCoord(imagePt)))
      .Times(1)
      .WillOnce(::testing::Return(imageLocus.point));

  double position[3];
  testCam->instrumentBodyFixedPosition(position, naif);
  EXPECT_EQ(position[0], (imageLocus.point.x) / 1000.0);
  EXPECT_EQ(position[1], (imageLocus.point.y) / 1000.0);
  EXPECT_EQ(position[2], (imageLocus.point.z) / 1000.0);
}


TEST_F(CSMCameraSetFixture, SubSpacecraftPoint) {
  auto naif = NaifContext::acquire();
  
  EXPECT_CALL(mockModel, getSensorPosition(MatchImageCoord(imagePt)))
      .Times(1)
      .WillOnce(::testing::Return(imageLocus.point));

  double lat, lon;
  testCam->subSpacecraftPoint(lat, lon, naif);
  EXPECT_EQ(lat, 0.0);
  EXPECT_EQ(lon, 0.0);
}


TEST_F(CSMCameraSetFixture, SlantDistance) {
  auto naif = NaifContext::acquire();
  
  EXPECT_CALL(mockModel, getSensorPosition(MatchImageCoord(imagePt)))
      .Times(1)
      .WillOnce(::testing::Return(imageLocus.point));

  double expectedDistance = sqrt(
      pow(imageLocus.point.x - groundPt.x, 2) +
      pow(imageLocus.point.y - groundPt.y, 2) +
      pow(imageLocus.point.z - groundPt.z, 2)) / 1000.0;
  EXPECT_DOUBLE_EQ(testCam->SlantDistance(naif), expectedDistance);
}


TEST_F(CSMCameraSetFixture, TargetCenterDistance) {
  auto naif = NaifContext::acquire();
  
  EXPECT_CALL(mockModel, getSensorPosition(MatchImageCoord(imagePt)))
      .Times(1)
      .WillOnce(::testing::Return(imageLocus.point));

  double expectedDistance = sqrt(
      pow(imageLocus.point.x, 2) +
      pow(imageLocus.point.y, 2) +
      pow(imageLocus.point.z, 2)) / 1000.0;
  EXPECT_DOUBLE_EQ(testCam->targetCenterDistance(naif), expectedDistance);
}


TEST_F(CSMCameraSetFixture, PhaseAngle) {
  auto naif = NaifContext::acquire();
  
  EXPECT_CALL(mockModel, getSensorPosition(MatchImageCoord(imagePt)))
      .Times(1)
      .WillOnce(::testing::Return(csm::EcefCoord(groundPt.x + 50000, groundPt.y, groundPt.z + 50000)));
  EXPECT_CALL(mockModel, getIlluminationDirection(MatchEcefCoord(groundPt)))
      .Times(1)
      .WillOnce(::testing::Return(csm::EcefVector(0.0, 0.0, -1.0)));

  EXPECT_DOUBLE_EQ(testCam->PhaseAngle(naif), 45.0);
}


TEST_F(CSMCameraSetFixture, IncidenceAngle) {
  auto naif = NaifContext::acquire();
  
  EXPECT_CALL(mockModel, getIlluminationDirection(MatchEcefCoord(groundPt)))
      .Times(1)
      .WillOnce(::testing::Return(csm::EcefVector(0.0, 0.0, -1.0)));

  EXPECT_DOUBLE_EQ(testCam->IncidenceAngle(naif), 90.0);
}


TEST_F(CSMCameraSetFixture, EmissionAngle) {
  auto naif = NaifContext::acquire();
  
  EXPECT_CALL(mockModel, getSensorPosition(MatchImageCoord(imagePt)))
      .Times(1)
      .WillOnce(::testing::Return(imageLocus.point));

  EXPECT_DOUBLE_EQ(testCam->EmissionAngle(naif), 0.0);
}


TEST_F(CSMCameraSetFixture, GroundPartials) {
  std::vector<double> expectedPartials = {1, 2, 3, 4, 5, 6};
  EXPECT_CALL(mockModel, computeGroundPartials(MatchEcefCoord(groundPt)))
      .Times(1)
      .WillOnce(::testing::Return(expectedPartials));

  std::vector<double> groundPartials = dynamic_cast<CSMCamera*>(testCam)->GroundPartials();
  ASSERT_EQ(groundPartials.size(), 6);
  EXPECT_EQ(groundPartials[0], expectedPartials[0]);
  EXPECT_EQ(groundPartials[1], expectedPartials[1]);
  EXPECT_EQ(groundPartials[2], expectedPartials[2]);
  EXPECT_EQ(groundPartials[3], expectedPartials[3]);
  EXPECT_EQ(groundPartials[4], expectedPartials[4]);
  EXPECT_EQ(groundPartials[5], expectedPartials[5]);
}


TEST_F(CSMCameraSetFixture, SensorPartials) {
  std::pair<double,double> expectedPartials = {1.23, -5.43};
  EXPECT_CALL(mockModel, computeSensorPartials(1, MatchEcefCoord(groundPt), 0.001, NULL, NULL))
      .Times(1)
      .WillOnce(::testing::Return(expectedPartials));

  std::vector<double> groundPartials =
      dynamic_cast<CSMCamera*>(testCam)->getSensorPartials(1, testCam->GetSurfacePoint());
  ASSERT_EQ(groundPartials.size(), 2);
  EXPECT_EQ(groundPartials[0], expectedPartials.first);
  EXPECT_EQ(groundPartials[1], expectedPartials.second);
}


TEST_F(CSMCameraFixture, getParameterIndicesSet) {
  std::vector<int> paramIndices = {0, 1, 2};
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterType(0))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterType(1))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterType(2))
      .WillRepeatedly(::testing::Return(csm::param::REAL));

  std::vector<int> indices = dynamic_cast<CSMCamera*>(testCam)->getParameterIndices(csm::param::ADJUSTABLE);
  ASSERT_EQ(indices.size(), paramIndices.size());
  for (size_t i = 0; i < paramIndices.size(); i++) {
    EXPECT_EQ(indices[i], paramIndices[i]) << "Error at index " << i;
  }
}


TEST_F(CSMCameraFixture, getParameterIndicesType) {
  std::vector<int> paramIndices = {1, 2};
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterType(0))
      .WillRepeatedly(::testing::Return(csm::param::FIXED));
  EXPECT_CALL(mockModel, getParameterType(1))
      .WillRepeatedly(::testing::Return(csm::param::REAL));
  EXPECT_CALL(mockModel, getParameterType(2))
      .WillRepeatedly(::testing::Return(csm::param::REAL));

  std::vector<int> indices = dynamic_cast<CSMCamera*>(testCam)->getParameterIndices(csm::param::REAL);
  ASSERT_EQ(indices.size(), paramIndices.size());
  for (size_t i = 0; i < paramIndices.size(); i++) {
    EXPECT_EQ(indices[i], paramIndices[i]) << "Error at index " << i;
  }
}


TEST_F(CSMCameraFixture, getParameterIndicesList) {
  std::vector<int> paramIndices = {2, 0};
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterName(0))
      .WillRepeatedly(::testing::Return("Parameter 1"));
  EXPECT_CALL(mockModel, getParameterName(1))
      .WillRepeatedly(::testing::Return("Parameter 2"));
  EXPECT_CALL(mockModel, getParameterName(2))
      .WillRepeatedly(::testing::Return("Parameter 3"));

  QStringList paramList = {"Parameter 3", "Parameter 1"};

  std::vector<int> indices = dynamic_cast<CSMCamera*>(testCam)->getParameterIndices(paramList);
  ASSERT_EQ(indices.size(), paramIndices.size());
  for (size_t i = 0; i < paramIndices.size(); i++) {
    EXPECT_EQ(indices[i], paramIndices[i]) << "Error at index " << i;
  }
}


TEST_F(CSMCameraFixture, getParameterIndicesListComparison) {
  std::vector<int> paramIndices = {2, 0, 1};
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterName(0))
      .WillRepeatedly(::testing::Return("Parameter 1  "));
  EXPECT_CALL(mockModel, getParameterName(1))
      .WillRepeatedly(::testing::Return("  Parameter 2"));
  EXPECT_CALL(mockModel, getParameterName(2))
      .WillRepeatedly(::testing::Return("Parameter 3"));

  QStringList paramList = {"PARAMETER 3", "  Parameter 1", "parameter 2  "};

  std::vector<int> indices = dynamic_cast<CSMCamera*>(testCam)->getParameterIndices(paramList);
  ASSERT_EQ(indices.size(), paramIndices.size());
  for (size_t i = 0; i < paramIndices.size(); i++) {
    EXPECT_EQ(indices[i], paramIndices[i]) << "Error at index " << i;
  }
}


TEST_F(CSMCameraFixture, getParameterIndicesListError) {
  std::vector<int> paramIndices = {3, 1};
  EXPECT_CALL(mockModel, getNumParameters())
      .WillRepeatedly(::testing::Return(3));
  EXPECT_CALL(mockModel, getParameterName(0))
      .WillRepeatedly(::testing::Return("Parameter 1"));
  EXPECT_CALL(mockModel, getParameterName(1))
      .WillRepeatedly(::testing::Return("Parameter 2"));
  EXPECT_CALL(mockModel, getParameterName(2))
      .WillRepeatedly(::testing::Return("Parameter 3"));

  QStringList paramList = {"Parameter 4", "Parameter 1", "Parameter 0"};

  try
  {
    dynamic_cast<CSMCamera*>(testCam)->getParameterIndices(paramList);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Failed to find indices for the following parameters ["
        "Parameter 4,Parameter 0].")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      "Failed to find indices for the following parameters [Parameter 4,Parameter 0].\"";
  }
}


TEST_F(CSMCameraFixture, applyParameterCorrection) {
  EXPECT_CALL(mockModel, getParameterValue(2))
      .Times(1)
      .WillOnce(::testing::Return(0.5));
  EXPECT_CALL(mockModel, setParameterValue(2, 1.5))
      .Times(1);

  dynamic_cast<CSMCamera*>(testCam)->applyParameterCorrection(2, 1.0);
}


TEST_F(CSMCameraFixture, getParameterCovariance) {
  EXPECT_CALL(mockModel, getParameterCovariance(2, 3))
      .Times(1)
      .WillOnce(::testing::Return(0.5));

  EXPECT_EQ(dynamic_cast<CSMCamera*>(testCam)->getParameterCovariance(2, 3), 0.5);
}


TEST_F(CSMCameraFixture, getParameterName) {
  EXPECT_CALL(mockModel, getParameterName(2))
      .Times(1)
      .WillOnce(::testing::Return("Omega Bias"));

  EXPECT_EQ(dynamic_cast<CSMCamera*>(testCam)->getParameterName(2), "Omega Bias");
}


TEST_F(CSMCameraFixture, getParameterValue) {
  EXPECT_CALL(mockModel, getParameterValue(2))
      .Times(1)
      .WillOnce(::testing::Return(0.5));

  EXPECT_DOUBLE_EQ(dynamic_cast<CSMCamera*>(testCam)->getParameterValue(2), 0.5);
}


TEST_F(CSMCameraFixture, getParameterUnits) {
  EXPECT_CALL(mockModel, getParameterUnits(2))
      .Times(1)
      .WillOnce(::testing::Return("m"));

  EXPECT_EQ(dynamic_cast<CSMCamera*>(testCam)->getParameterUnits(2), "m");
}


TEST_F(CSMCameraSetFixture, SerialNumber) {
  QString sn = SerialNumber::Compose(*testCube);
  SerialNumberList snl;

  snl.add(testCube->fileName());
  QString instId = snl.spacecraftInstrumentId(sn);

  EXPECT_PRED_FORMAT2(AssertQStringsEqual, sn, "TestPlatform/TestInstrument/2000-01-01T11:58:55.816");
  EXPECT_TRUE(snl.hasSerialNumber(sn));
  EXPECT_PRED_FORMAT2(AssertQStringsEqual, instId, "TESTPLATFORM/TESTINSTRUMENT");
}


TEST_F(CSMCameraFixture, CameraState) {
  std::string testString = "MockSensorModel\nTestModelState";
  EXPECT_CALL(mockModel, getModelState())
      .Times(1)
      .WillOnce(::testing::Return(testString));

  EXPECT_EQ(dynamic_cast<CSMCamera*>(testCam)->getModelState().toStdString(), testString);
}


TEST_F(CSMCameraFixture, SetTime) {
  try
  {
    auto naif = NaifContext::acquire();
    testCam->setTime(iTime("2000-01-01T11:58:55.816"), naif);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Setting the image time is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Setting the image time is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, SubSolarPoint) {
  try
  {
    auto naif = NaifContext::acquire();
    double lat, lon;
    testCam->subSolarPoint(lat, lon, naif);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Sub solar point is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Sub solar point is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, PixelIfovOffsets) {
  try
  {
    testCam->PixelIfovOffsets();
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Pixel Field of View is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Pixel Field of View is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, SunPosition) {
  try
  {
    double position[3];
    testCam->sunPosition(position);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Sun position is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Sun position is not supported for CSM camera models\"";
  }

  try
  {
    testCam->sunPosition();
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Sun position is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Sun position is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, InstrumentPosition) {
  try
  {
    testCam->instrumentPosition();
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Instrument position is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Instrument position is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, BodyRotation) {
  try
  {
    testCam->bodyRotation();
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Target body orientation is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Target body orientation is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, InstrumentRotation) {
  try
  {
    testCam->instrumentRotation();
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Instrument orientation is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Instrument orientation is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, SolarLongitude) {
  try
  {
    auto naif = NaifContext::acquire();
    testCam->solarLongitude(naif);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Solar longitude is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Solar longitude is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, SolarDistance) {
  try
  {
    testCam->SolarDistance();
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Solar distance is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Solar distance is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, RightAscension) {
  try
  {
    auto naif = NaifContext::acquire();
    testCam->RightAscension(naif);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Right Ascension is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Right Ascension is not supported for CSM camera models\"";
  }
}


TEST_F(CSMCameraFixture, Declination) {
  try
  {
    auto naif = NaifContext::acquire();
    testCam->Declination(naif);
  }
  catch(Isis::IException &e)
  {
    EXPECT_TRUE(e.toString().toLatin1().contains("Declination is not supported "
        "for CSM camera models")) << e.toString().toStdString();
  }
  catch(...)
  {
      FAIL() << "Expected an IException with message \""
      " Declination is not supported for CSM camera models\"";
  }
}
