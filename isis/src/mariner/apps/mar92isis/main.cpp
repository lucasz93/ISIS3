/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include "Cube.h"
#include "FileName.h"
#include "IString.h"
#include "iTime.h"
#include "OriginalLabel.h"
#include "PixelType.h"
#include "ProcessImport.h"
#include "ProcessImportPds.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlFormat.h"
#include "PvlFormatPds.h"

#include <sstream>

#include <QString>

using namespace std;
using namespace Isis;

void UpdateLabels(Cube *cube, const QString &labels);
void TranslateIsis2Labels(FileName &labelFile, Cube *oCube);
QString EbcdicToAscii(unsigned char *header);
QString DaysToDate(int days);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Determine whether input is a raw Mariner 10 image or an Isis 2 cube
  bool isRaw = false;
  FileName inputFile = ui.GetFileName("FROM");
  Pvl label(inputFile.expanded());

  // If the PVL created from the input labels is empty, then input is raw
  if(label.groups() + label.objects() + label.keywords() == 0) {
    isRaw = true;
  }

  // Only support the original Mariner 9 images.
  // I saw some weird size differences between some images which offsets the reseaus positions.
  if(!isRaw) {
    throw IException(IException::User, "Only raw Mariner 9 images are supported", _FILEINFO_);
  }
  
  ProcessImport p;

  // All mariner images from both cameras share this size
  p.SetDimensions(832, 700, 1);
  p.SetFileHeaderBytes(968);
  p.SaveFileHeader();
  p.SetPixelType(UnsignedByte);
  p.SetByteOrder(Lsb);
  p.SetDataSuffixBytes(136);

  p.SetInputFile(ui.GetFileName("FROM"));
  Cube *oCube = p.SetOutputCube("TO");

  p.StartProcess();
  unsigned char *header = (unsigned char *) p.FileHeader();
  QString labels = EbcdicToAscii(header);
  UpdateLabels(oCube, labels);
  p.EndProcess();
}

// Converts labels into standard pvl format and adds necessary
// information not included in original labels
void UpdateLabels(Cube *cube, const QString &labels) {
  // First, we parse out as much valid information as possible from the
  // original labels
  QString key;
  int keyPosition;
  int consumeChars;

  // Mariner 9 header.
  key = "SCMARINER 9";
  keyPosition = labels.indexOf(key);
  if (keyPosition == -1)
    throw IException(IException::User, "Not a Mariner 9 EDR", _FILEINFO_);

  // Which of the two cameras took the image
  key = "***";
  keyPosition = labels.indexOf(key);
  consumeChars = 1;
  QString ccamera(labels.mid(keyPosition + key.length(), consumeChars));
  ccamera = ccamera.trimmed();

  // Year the image was taken
  key = "YR ";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("DAY") - keyPosition - key.length();
  QString yr(labels.mid(keyPosition + key.length(), consumeChars));
  yr = yr.trimmed();

  // Day the image was taken
  key = "DAY ";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("GMT") - keyPosition - key.length();
  QString day(labels.mid(keyPosition + key.length(), consumeChars));
  day = day.trimmed();

  // Greenwich Mean Time
  key = "GMT ";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("DAS TIME") - keyPosition - key.length();
  QString gmt(labels.mid(keyPosition + key.length(), consumeChars));
  gmt = gmt.trimmed();

  // Greenwich Mean Time
  key = "DAS TIME ";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("CPICTURE") - keyPosition - key.length();
  QString das(labels.mid(keyPosition + key.length(), consumeChars));
  das = das.trimmed();

  // Filter number
  key = "FILTER POS ";
  keyPosition = labels.indexOf(key);
  consumeChars = 1;
  QString filterNum(labels.mid(keyPosition + key.length(), consumeChars));
  filterNum = filterNum.trimmed();

  // Center wavelength
  // These were all pulled from the paper "Mariner 9 Television Reconnaissance of Mars and Its Satellites: Preliminary Results".
  // Available here: https://collections.nlm.nih.gov/ext/document/101584906X949/PDF/101584906X949.pdf
  double filterCenter = 0.;
  QString filterName;
  if (ccamera == "A") {
    // I've only even seen these asterix appear on the B camera.
    // Sanity check here.
    if (filterNum == "*")
      throw IException(IException::Programmer, "Filter A, DAS TIME [" + das + "] has an unknown filter", _FILEINFO_);

    int fnum = toInt(filterNum);
    // There seem to be 14 filters listed in the SEDR.
    switch(fnum) {
      // No useful descriptions. All other filters are accounted for. I'm going to assume these are the yellow filter.
      case 0:   // 3 images.
      case 3:   // 2 images.
      case 5:   // 3 images.
      case 6:   // 3 images.
      case 9:   // 1 image.
      case 10:  // 1 image.
      case 12:  // 1 image.
      case 15:  // 1 image.
        filterCenter = 0.560;
        filterName = "Yellow";
        break;

      case 1:
      case 14:
        filterCenter = .477;  // These are both labelled with a description of BLUE in the SEDR.
        filterName = "Blue";
        break;

      case 2:
        filterCenter = .61;   // Orange.
        filterName = "Orange";
        break;

      case 4:
        filterCenter = .545;  // Green.
        filterName = "Green";
        break;
   
      case 8:
        filterCenter = .414;  // Violet.
        filterName = "Violet";
        break;

      // The above paper says Mariner 9 had 3 polarisation filters.
      case 7:                 // Polarization (120 degrees).
      case 11:                // Polarization (0   degrees).
      case 13:                // Polarization (60  degrees).
        filterCenter = .565;
        filterName = "Polarization";
        break;

      default:
        break;
    }
  }
  else {
    // I've only even seen these asterix appear on the B camera.
    // Sanity check here.
    if (filterNum != "*")
      throw IException(IException::Programmer, "Filter B, DAS TIME [" + das + "] has an unknown filter [" + filterNum + "]", _FILEINFO_);

    // The above paper says the B camera only had a single filter.
    filterCenter = .558;
  }

  // Exposure duration
  key = "EXP TIME ";
  keyPosition = labels.indexOf(key);
  consumeChars = labels.indexOf("MSEC") - keyPosition - key.length();
  QString exposure(labels.mid(keyPosition + key.length(), consumeChars));
  exposure = exposure.trimmed();

  // Create the instrument group
  PvlGroup inst("Instrument");
  inst += PvlKeyword("SpacecraftName", "Mariner_9");
  inst += PvlKeyword("InstrumentId", "M9_VIDICON_" + ccamera);

  // Get the date
  int days = toInt(day);
  QString date = DaysToDate(days);

  // Get the time - reformat from HHMMSS to HH:MM:SS.
  QString time = gmt.mid(0, 2) + ":" + gmt.mid(2, 2) + ":" + gmt.mid(4, 2);

  // Construct the Start Time in yyyy-mm-ddThh:mm:ss format
  QString fullTime = date + "T" + time + ".000";
  iTime startTime(fullTime);

  // Create the archive group
  PvlGroup archive("Archive");

  int year = toInt(yr);
  year += 1900;
  QString fullGMT = toString(year) + ":" + day + ":" + time;
  archive += PvlKeyword("GMT", fullGMT);

  // Create the band bin group
  PvlGroup bandBin("BandBin");
  bandBin += PvlKeyword("FilterName", filterName);
  QString number = filterNum;
  bandBin += PvlKeyword("FilterNumber", number);
  bandBin += PvlKeyword("OriginalBand", "1");
  QString center = toString(filterCenter);
  bandBin += PvlKeyword("Center", center);
  bandBin.findKeyword("Center").setUnits("micrometers");

  inst += PvlKeyword("TargetName", "Mars");
    archive += PvlKeyword("Encounter", "Mars");

  // Place start time and exposure duration in intrument group
  inst += PvlKeyword("StartTime", fullTime);
  inst += PvlKeyword("ExposureDuration", exposure, "milliseconds");
  inst += PvlKeyword("ImageNumber", exposure, "milliseconds");

  // Open nominal positions pvl named by QString encounter
  Pvl nomRx("$ISISDATA/mariner9/reseaus/mar9Nominal.pvl");

  // Allocate all keywords within reseaus groups well as the group its self
  PvlGroup rx("Reseaus");
  PvlKeyword line("Line");
  PvlKeyword sample("Sample");
  PvlKeyword type("Type");
  PvlKeyword valid("Valid");
  PvlKeyword templ("Template");
  PvlKeyword status("Status");

  // All cubes will stay this way until indexOfrx is run on them
  status = "Nominal";

  // Kernels group
  PvlGroup kernels("Kernels");
  kernels += PvlKeyword("SpacecraftClock", das);

  // Camera dependent information
  QString camera = "";
  QString camera_number_reseaus = "";
  if(QString("M9_VIDICON_A") == inst["InstrumentId"][0]) {
    templ = "$ISISDATA/mariner9/reseaus/mar9a.template.cub";
    camera = "M9_VIDICON_A_RESEAUS";
    camera_number_reseaus = "M9_VIDICON_A_NUMBER_RESEAUS";
  }
  else {
    templ = "$ISISDATA/mariner9/reseaus/mar9b.template.cub";
    camera = "M9_VIDICON_B_RESEAUS";
    camera_number_reseaus = "M9_VIDICON_B_NUMBER_RESEAUS";
  }

  // Find the correct PvlKeyword corresponding to the camera for nominal positions
  PvlKeyword resnom = nomRx[camera];
  int rescount = nomRx[camera_number_reseaus];

  // This loop goes through the PvlKeyword resnom which contains data
  // in the format: line, sample, type, on each line. There are 111 reseaus for
  // both cameras. To put data away correctly, it must go through a total 333 items,
  // all in one PvlKeyword.
  int i = 0;
  while(i < rescount * 3) {
    line += resnom[i];
    i++;
    sample += resnom[i];
    i++;
    type += resnom[i];
    i++;
    valid += "0";
  }

  // Add all the PvlKeywords to the PvlGroup Reseaus
  rx += line;
  rx += sample;
  rx += type;
  rx += valid;
  rx += templ;
  rx += status;

  // Get the labels and add the updated labels to them
  Pvl *cubeLabels = cube->label();
  cubeLabels->findObject("IsisCube").addGroup(inst);
  cubeLabels->findObject("IsisCube").addGroup(archive);
  cubeLabels->findObject("IsisCube").addGroup(bandBin);
  cubeLabels->findObject("IsisCube").addGroup(kernels);
  cubeLabels->findObject("IsisCube").addGroup(rx);

  PvlObject original("OriginalLabel");
  original += PvlKeyword("Label", labels);
  Pvl olabel;
  olabel.addObject(original);
  OriginalLabel ol(olabel);
  cube->write(ol);
}

// FYI, mariner10 original labels are stored in ebcdic, a competitor with ascii,
// a conversion table is necessary then to get the characters over to ascii. For
// more info: http://en.wikipedia.org/wiki/Extended_Binary_Coded_Decimal_Interchange_Code
//! Converts ebsidic Mariner10 labels to ascii
QString EbcdicToAscii(unsigned char *header) {
  // Table to convert ebcdic to ascii
  unsigned char xlate[] = {
    0x00, 0x01, 0x02, 0x03, 0x9C, 0x09, 0x86, 0x7F, 0x97, 0x8D, 0x8E, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x9D, 0x85, 0x08, 0x87, 0x18, 0x19, 0x92, 0x8F, 0x1C, 0x1D, 0x1E, 0x1F,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x0A, 0x17, 0x1B, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x05, 0x06, 0x07,
    0x90, 0x91, 0x16, 0x93, 0x94, 0x95, 0x96, 0x04, 0x98, 0x99, 0x9A, 0x9B, 0x14, 0x15, 0x9E, 0x1A,
    0x20, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xD5, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,
    0x26, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
    0x2D, 0x2F, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xE5, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
    0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
    0xC3, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
    0xCA, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0,
    0xD1, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0xD2, 0xD3, 0xD4, 0x5B, 0xD6, 0xD7,
    0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0x5D, 0xE6, 0xE7,
    0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED,
    0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3,
    0x5C, 0x9F, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
  };

  // Mariner 10 has 360 bytes of header information
  for(int i = 0; i < 360; i++) {
    header[i] = xlate[header[i]];
  }

  // Put in a end of QString mark and return
  header[215] = 0;
  return QString((const char *)header);
}

// Mariner 10 labels provide the number of days since the beginning of the year
// 1974 in the GMT keyword, but not always a start time.  In order to derive an
// estimated start time, with an actual date attached, a conversion must be
// performed.
QString DaysToDate(int days) {
  int currentMonth = 12;
  int currentDay = 31;
  int currentYear = 1973;
  while(days > 0) {
    // The Mariner 10 mission took place in the years 1973 through 1975,
    // none of which were Leap Years, thus February always had 28 days
    if(currentDay == 28 && currentMonth == 2) {
      currentMonth = 3;
      currentDay = 1;
    }
    else if(currentDay == 30 &&
            (currentMonth == 4 || currentMonth == 6 ||
             currentMonth == 9 || currentMonth == 11)) {
      currentMonth++;
      currentDay = 1;
    }
    else if(currentDay == 31 && currentMonth == 12) {
      currentMonth = 1;
      currentDay = 1;
      currentYear++;
    }
    else if(currentDay == 31) {
      currentMonth++;
      currentDay = 1;
    }
    else {
      currentDay++;
    }
    days--;
  }
  QString year = toString(currentYear);
  QString month = (currentMonth < 10) ? "0" + toString(currentMonth) : toString(currentMonth);
  QString day = (currentDay < 10) ? "0" + toString(currentDay) : toString(currentDay);
  return year + "-" + month + "-" + day;
}
