/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "IException.h"
#include "UserInterface.h"
#include "iTime.h"
#include "Camera.h"
#include "NaifStatus.h"
#include "Quaternion.h"

#include <iomanip>

using namespace std;
using namespace Isis;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  Cube cube;
  cube.open(ui.GetCubeName("FROM"), "rw");

  // Check that it is a Mariner9 cube.
  Pvl * labels = cube.label();
  if ("Mariner_9" != (QString)labels->findKeyword("SpacecraftName", Pvl::Traverse)) {
    QString msg = "The cube [" + ui.GetCubeName("FROM") + "] does not appear" +
      " to be a Mariner9 cube";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  QString imageNumberString = labels->findKeyword("ImageNumber", Pvl::Traverse);
  int imageNumber = std::stoi(imageNumberString.toStdString());

  switch (imageNumber)
  {
    // These times are listed as being invalid by the SCLK.
    case  1657867:
    case  1749711:
    case  4940820:
    case  5023168:
    case 10494479:
    case 10721009:
    case 11482149:
    case 11658925:
    case 11836131:
    case 12013129:
    case 12188937:
    case 12364456:
    case 12538710:
    case 12910063:
    case 13165396:
    case 13360390:
    case 13511838:
      break;

    // These times don't seem to work.
    case 12685643:
      break;

    default:
      return;
  }

  Pvl sedr("$mariner9/metadata/sedr.pvl");

  if (!sedr.hasGroup(imageNumberString))
  {
    throw IException(IException::User, imageNumberString + " isn't in the SEDR", _FILEINFO_);
    return;
  }

  const iTime utc((QString)labels->findKeyword("StartTime", Pvl::Traverse));

  const auto& metadata = sedr.findGroup(imageNumberString);

  {
    const auto& pointing = metadata["InstrumentPointing"];

    std::vector<double> c(9);
    for (size_t i = 0; i < c.size(); ++i)
    {
      c[i] = std::stod(pointing[i].toStdString());
    }
    Quaternion quat(c);

    // If the first component is less than zero, multiply the whole quaternion by -1. This
    // matches NAIF.
    if (quat[0] < 0) {
      quat[0] = -1 * quat[0];
      quat[1] = -1 * quat[1];
      quat[2] = -1 * quat[2];
      quat[3] = -1 * quat[3];
    }

    TableField q0("J2000Q0", TableField::Double);
    TableField q1("J2000Q1", TableField::Double);
    TableField q2("J2000Q2", TableField::Double);
    TableField q3("J2000Q3", TableField::Double);
    TableField t("ET", TableField::Double);

    TableRecord record;
    record += q0;
    record += q1;
    record += q2;
    record += q3;
    record += t;

    record[0] = quat[0];
    record[1] = quat[1];
    record[2] = quat[2];
    record[3] = quat[3];
    record[4] = utc.Et();

    Table table("InstrumentPointing", record);
    table += record;

    table.Label() += PvlKeyword("FrameTypeCode");
    table.Label()["FrameTypeCode"].addValue(toString(SpiceRotation::CK));

    cube.camera()->instrumentRotation()->LoadCache(table);
    cube.write(table);
  }

  {
    const auto& position = metadata["InstrumentPosition"];

    std::vector<double> vr(3);
    for (size_t i = 0; i < vr.size(); ++i)
    {
      vr[i] = std::stod(position[i].toStdString());
    }

    TableField x("J2000X", TableField::Double);
    TableField y("J2000Y", TableField::Double);
    TableField z("J2000Z", TableField::Double);
    TableField t("ET", TableField::Double);

    TableRecord record;
    record += x;
    record += y;
    record += z;
    record += t;

    record[0] = vr[0];
    record[1] = vr[1];
    record[2] = vr[2];
    record[3] = utc.Et();

    Table table("InstrumentPosition", record);
    table += record;

    cube.camera()->instrumentPosition()->ReloadCache(table);
    cube.write(table);
  }

  std::cout << "Patched" << std::endl;
}