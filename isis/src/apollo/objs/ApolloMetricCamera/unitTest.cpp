/**
 * @file
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include <iomanip>
#include <iostream>

#include <QList>
#include <QString>

#include "CubeManager.h"
#include "ApolloMetricCamera.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void TestLineSamp(Camera *cam, double samp, double line, NaifContextPtr naif);

int main(int argc, char **argv) {
  Preference::Preferences(true);
  NaifContextLifecycle naif_lifecycle;
  auto naif = NaifContext::acquire();
  cout << "Unit Test for ApolloCamera..." << endl;
  try {
    // These should be lat/lon at center of image. To obtain these numbers for a new cube/camera,
    // set both the known lat and known lon to zero and copy the unit test output "Latitude off by: "
    // and "Longitude off by: " values directly into these variables.
    double knownLat = 12.5300329125960879;
    double knownLon = 67.7259113746637524;

    Cube c(FileName("$apollo15/testData/AS15-M-0533.cropped.cub").expanded(), "r");
    ApolloMetricCamera *cam = (ApolloMetricCamera *) CameraFactory::Create(c);
    cout << "FileName: " << FileName(c.fileName()).name() << endl;
    cout << "CK Frame: " << cam->instrumentRotation()->Frame() << endl << endl;
    cout.setf(std::ios::fixed);
    cout << setprecision(9);

    // Test kernel IDsq
    cout << "Kernel IDs: " << endl;
    cout << "CK Frame ID = " << cam->CkFrameId() << endl;
    cout << "CK Reference ID = " << cam->CkReferenceId() << endl;
    cout << "SPK Target ID = " << cam->SpkTargetId() << endl;
    cout << "SPK Reference ID = " << cam->SpkReferenceId() << endl << endl;

    // Test Shutter Open/Close
    const PvlGroup &inst = c.label()->findGroup("Instrument", Pvl::Traverse);
    QString stime = inst["StartTime"];
    double et; // StartTime keyword is the center exposure time
    naif->str2et_c(stime.toLatin1().data(), &et);
    // approximate 1 tenth of a second since Apollo did not
    double exposureDuration = .1;
    pair <iTime, iTime> shuttertimes = cam->ShutterOpenCloseTimes(et, exposureDuration);
    cout << "Shutter open = " << shuttertimes.first.Et() << endl;
    cout << "Shutter close = " << shuttertimes.second.Et() << endl << endl;

    // Test all four corners to make sure the conversions are right
    cout << "For upper left corner ..." << endl;
    TestLineSamp(cam, 1.0, 1.0, naif);

    cout << "For upper right corner ..." << endl;
    TestLineSamp(cam, cam->Samples(), 1.0, naif);

    cout << "For lower left corner ..." << endl;
    TestLineSamp(cam, 1.0, cam->Lines(), naif);

    cout << "For lower right corner ..." << endl;
    TestLineSamp(cam, cam->Samples(), cam->Lines(), naif);

    double samp = cam->Samples() / 2;
    double line = cam->Lines() / 2;
    cout << "For center pixel position ..." << endl;

    if(!cam->SetImage(samp, line, naif)) {
      cout << "ERROR" << endl;
      return 0;
    }

    if(abs(cam->UniversalLatitude() - knownLat) < 1E-10) {
      cout << "Latitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Latitude off by: " << cam->UniversalLatitude() - knownLat << endl;
    }

    if(abs(cam->UniversalLongitude() - knownLon) < 1E-10) {
      cout << "Longitude OK" << endl;
    }
    else {
      cout << setprecision(16) << "Longitude off by: " << cam->UniversalLongitude() - knownLon << endl;
    }

    // Test name methods
    QList<QString> files;
    files.append("$apollo15/testData/AS15-M-0533.cropped.cub");
    files.append("$apollo16/testData/AS16-M-0533.reduced.cub");
    files.append("$apollo17/testData/AS17-M-0543.reduced.cub");

    cout << endl << endl << "Testing name methods ..." << endl;
    for (int i = 0; i < files.size(); i++) {
      Cube n(files[i], "r");
      ApolloMetricCamera *nCam = (ApolloMetricCamera *) CameraFactory::Create(n);
      cout << "Spacecraft Name Long: " << nCam->spacecraftNameLong() << endl;
      cout << "Spacecraft Name Short: " << nCam->spacecraftNameShort() << endl;
      cout << "Instrument Name Long: " << nCam->instrumentNameLong() << endl;
      cout << "Instrument Name Short: " << nCam->instrumentNameShort() << endl << endl;
    }

    // Test exception
    cout << endl << "Testing exceptions:" << endl << endl;
    Cube test("$hayabusa/testData/st_2530292409_v.cub", "r");
    ApolloMetricCamera mCam(test);
  }
  catch(IException &e) {
    e.print();
  }
}

void TestLineSamp(Camera *cam, double samp, double line, NaifContextPtr naif) {
  bool success = cam->SetImage(samp, line, naif);

  if (success) {
    success = cam->SetUniversalGround(naif, cam->UniversalLatitude(), cam->UniversalLongitude());
  }

  if(success) {
    double deltaSamp = samp - cam->Sample();
    double deltaLine = line - cam->Line();
    if(fabs(deltaSamp) < 0.01) deltaSamp = 0;
    if(fabs(deltaLine) < 0.01) deltaLine = 0;
    cout << "DeltaSample = " << deltaSamp << endl;
    cout << "DeltaLine = " << deltaLine << endl << endl;
  }
  else {
    cout << "DeltaSample = ERROR" << endl;
    cout << "DeltaLine = ERROR" << endl << endl;
  }
}
