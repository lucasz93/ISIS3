/**
 * @file
 * $Revision: 1.6 $
 * $Date: 2010/06/29 18:16:39 $
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

#include "Mariner10Camera.h"

#include <iostream>
#include <iomanip>

#include "NaifContext.h"

#include <QString>

#include "CameraDetectorMap.h"
#include "CameraFocalPlaneMap.h"
#include "CameraGroundMap.h"
#include "CameraSkyMap.h"
#include "IString.h"
#include "iTime.h"
#include "FileName.h"
#include "NaifContext.h"
#include "Pvl.h"
#include "ReseauDistortionMap.h"

using namespace std;
namespace Isis {
  /**
   * Creates a Mariner10 Camera Model
   *
   * @param lab Pvl label from a Mariner 10 image.
   *
   * @throw iException::User - "File does not appear to be a Mariner 10 image.
   *        Invalid InstrumentId."
   * @throw iException::Programmer - "Unable to create distortion map."
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Added NAIF error check.
   *
   */
  Mariner10Camera::Mariner10Camera(Cube &cube) : FramingCamera(cube) {
    auto naif = NaifContext::acquire();

    naif->CheckErrors();
    
    m_spacecraftNameLong = "Mariner 10";
    m_spacecraftNameShort = "Mariner10";

    //  Turn off the aberration corrections for instrument position object
    instrumentPosition()->SetAberrationCorrection("NONE");
    instrumentRotation()->SetFrame(-76000);

    // Set camera parameters
    SetFocalLength(naif);
    SetPixelPitch(naif);

    Pvl &lab = *cube.label();
    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    // Get utc start time
    QString stime = inst["StartTime"];

    iTime startTime;
    startTime.setUtc((QString)inst["StartTime"]);
    setTime(startTime, naif);

    // Setup detector map
    new CameraDetectorMap(this);

    // Setup focal plane map, and detector origin
    CameraFocalPlaneMap *focalMap = new CameraFocalPlaneMap(naif, this, naifIkCode());

    QString ikernKey = "INS" + toString((int)naifIkCode()) + "_BORESIGHT_SAMPLE";
    double sampleBoresight = getDouble(naif, ikernKey);
    ikernKey = "INS" + toString((int)naifIkCode()) + "_BORESIGHT_LINE";
    double lineBoresight = getDouble(naif, ikernKey);

    focalMap->SetDetectorOrigin(sampleBoresight, lineBoresight);

    // Setup distortion map which is dependent on encounter, use start time
    // MOON:  1973-11-08T03:16:26.350
    QString spacecraft = (QString)inst["SpacecraftName"];
    QString instId = (QString)inst["InstrumentId"];
    QString cam;
    if(instId == "M10_VIDICON_A") {
      cam = "a";
      m_instrumentNameLong = "Mariner 10 Vidicon A";
      m_instrumentNameShort = "VIDICON A";
    }
    else if(instId == "M10_VIDICON_B") {
      cam = "b";
      m_instrumentNameLong = "Mariner 10 Vidicon B";
      m_instrumentNameShort = "VIDICON B";
    }
    else {
      QString msg = "File does not appear to be a Mariner10 image. InstrumentId ["
          + instId + "] is invalid Mariner 10 value.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    QString fname = FileName("$mariner10/reseaus/mar10" + cam
                             + "MasterReseaus.pvl").expanded();

    try {
      new ReseauDistortionMap(this, lab, fname);
    }
    catch(IException &e) {
      string msg = "Unable to create distortion map.";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }

    // Setup the ground and sky map
    new CameraGroundMap(this);
    new CameraSkyMap(this);

    LoadCache(naif);
    naif->CheckErrors();
  }

  
  /**
   * Returns the shutter open and close times.  The user should pass in the
   * ExposureDuration keyword value, converted from milliseconds to seconds, and
   * the StartTime keyword value, converted to ephemeris time. The StartTime
   * keyword value from the labels represents the shutter center time of the
   * observation. To find the shutter open and close times, half of the exposure
   * duration is subtracted from and added to the input time parameter,
   * respectively. This method overrides the FramingCamera class method.
   *
   * @param exposureDuration ExposureDuration keyword value from the labels,
   *                         converted to seconds.
   * @param time The StartTime keyword value from the labels, converted to
   *             ephemeris time.
   *
   * @return @b pair < @b iTime, @b iTime > The first value is the shutter
   *         open time and the second is the shutter close time.
   *
   * @author 2011-05-03 Jeannie Walldren
   * @internal
   *   @history 2011-05-03 Jeannie Walldren - Original version.
   */
  pair<iTime, iTime> Mariner10Camera::ShutterOpenCloseTimes(double time,
                                                            double exposureDuration) {
    pair<iTime, iTime> shuttertimes;
    // To get shutter start (open) time, subtract half exposure duration
    shuttertimes.first = time - (exposureDuration / 2.0);
    // To get shutter end (close) time, add half exposure duration
    shuttertimes.second = time + (exposureDuration / 2.0);
    return shuttertimes;
  }
}


/**
 * This is the function that is called in order to instantiate a Mariner10Camera
 * object.
 *
 * @param lab Cube labels
 *
 * @return Isis::Camera* Mariner10Camera
 */
extern "C" Isis::Camera *Mariner10CameraPlugin(Isis::Cube &cube) {
  return new Isis::Mariner10Camera(cube);
}
