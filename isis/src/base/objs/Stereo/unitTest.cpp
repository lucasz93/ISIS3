#include <iostream>
#include <iomanip>

#include "Camera.h"
#include "Cube.h"
#include "IException.h"
#include "Preference.h"
#include "Stereo.h"


using namespace std;
using namespace Isis;

/**
 * unitTest for Stereo class
 * 
 * @author 2012-02-29 Tracie Sucharski
 *  
 * @internal 
 * 
 */
int main(int argc, char *argv[]) {
  Preference::Preferences(true);
  NaifContextLifecycle naif_lifecycle;
  auto naif = NaifContext::acquire();

  try {
    cout << "UnitTest for Stereo" << endl;

    cout << setprecision(9);

    Cube leftCube;
    leftCube.open("$mariner10/testData/0027399_clean_equi.cub");
    Cube rightCube;
    rightCube.open("$mariner10/testData/0166613_clean_equi.cub");

    leftCube.camera()->SetImage(1054.19, 624.194, naif);
    rightCube.camera()->SetImage(1052.19, 624.194, naif);

    double radius, lat, lon, sepang, error;
    Stereo::elevation(naif, *(leftCube.camera()), *(rightCube.camera()),
                      radius, lat, lon, sepang, error);

    cout << "Radius = " << radius << endl;
    cout << "Radius Error = " << error << endl;
    cout << "Separation Angle = " << sepang << endl;
    cout << "Latitude = " << lat << endl;
    cout << "Longitude = " << lon << endl;

    double x, y, z;
    Stereo::spherical(naif, lat, lon, radius, x, y, z);
    cout << "Spherical to Rectangular conversion:" << endl;
    cout << "X = " << x << endl;
    cout << "Y = " << y << endl;
    cout << "Z = " << z << endl;

    double newLat, newLon, newRad;
    Stereo::rectangular(naif, x, y, z, newLat, newLon, newRad);
    cout << "Rectangular to spherical conversion:" << endl;
    cout << "Latitude = " << newLat << endl;
    cout << "Longitude = " << newLon << endl;


  }
  catch (Isis::IException &e) {
    e.print();
  }
}

