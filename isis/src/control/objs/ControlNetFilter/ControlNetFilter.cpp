#include "ControlNetFilter.h"

#include "Angle.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlCubeGraphNode.h"
#include "ControlPoint.h"
#include "FileName.h"
#include "iString.h"
#include "Latitude.h"
#include "Longitude.h"
#include "PvlGroup.h"
#include "SerialNumberList.h"
#include "Statistics.h"

// for double precision
#include <limits>
typedef std::numeric_limits< double > dbl;

using namespace std;

#define UNDEFINED_STATUS 2

namespace Isis {

  //! String names for Point Type
  extern string sPointType [];

  //! String values for Boolean
  extern string sBoolean[];

  /**
   * ControlNetFilter Constructor
   *
   * @author Sharmila Prasad (8/27/2010)
   *
   * @param pCNet  - input Control Net
   * @param psSerialNumFile - Corresponding Serial Num List
   * @param pProgress - Progress of the processing
   */
  ControlNetFilter::ControlNetFilter(ControlNet *pCNet, string &psSerialNumFile, Progress *pProgress) :
    ControlNetStatistics(pCNet, psSerialNumFile, pProgress) {
    mSerialNumFilter  = SerialNumberList(psSerialNumFile);
  }

  /**
   * Get the output file and open the output file stream
   *
   * @author Sharmila Prasad (9/7/2010)
   *
   * @param psPrintFile
   * @param pbPvl
   */
  void ControlNetFilter::SetOutputFile(string psPrintFile) {
    Isis::FileName outFile(psPrintFile);
    string outName(outFile.expanded());
    mOstm.open(outName.c_str(), std::ios::out);
    mOstm.precision(dbl::digits10);
  }

  /**
   * ControlNetFilter Destructor
   *
   * @author Sharmila Prasad (8/27/2010)
   */
  ControlNetFilter::~ControlNetFilter() {
    mOstm.close();
  }


  /**
   * Check the filtered point to be editlocked before removing from the
   * current control network.
   *
   * @author Sharmila Prasad (7/22/2011)
   *
   * @param pindex - Control Point index
   */
  void ControlNetFilter::FilterOutPoint(int pindex){
    if ( mCNet->GetPoint(pindex)->IsEditLocked() ) {
      mCNet->GetPoint(pindex)->SetEditLock(false);
    }
    mCNet->DeletePoint(pindex);
  }

  /**
   * Delete the network for an Image given Serial Number for all the
   * Points in the network.If the Measure is locked, then it is unlocked
   * in preparation for deleting. If the Point is locked, it is unlocked
   * and set back to lock when the Measure is deleted.
   *
   * @author Sharmila Prasad (11/3/2011)
   *
   * @param serialNum - Serial Number
   */
  void ControlNetFilter::FilterOutMeasuresBySerialNum(string serialNum){
    QString sn(serialNum.c_str());
    const ControlCubeGraphNode *csn = mCNet->getGraphNode(sn);
    QList< ControlMeasure * > measures = csn->getMeasures();

    foreach(ControlMeasure * measure, measures) {
      bool pointEditFlag = false;
      QString ptId(measure->Parent()->GetId().c_str());
      ControlPoint * point = mCNet->GetPoint(ptId);
      if (point->IsEditLocked()) {
        point->SetEditLock(false);
        pointEditFlag = true;
      }
      ControlMeasure *msr = point->GetMeasure(sn);
      msr->SetEditLock(false);
      point->Delete(serialNum);
      if (pointEditFlag) {
        point->SetEditLock(true);
      }
    }
  }

  /**
   * Print the Standard Point Stats Header into Output File
   *
   * @author Sharmila Prasad (8/31/2010)
   */
  void ControlNetFilter::PointStatsHeader(void) {
    mOstm << "PointID, PointType, PointIgnored, PointEditLocked, TotalMeasures, MeasuresIgnored, MeasuresEditLocked, ";
  }

  /**
   * Print the Standard Point Stats into Output file given the Control Point
   *
   * @author Sharmila Prasad (8/31/2010)
   *
   * @param pcPoint
   */
  void ControlNetFilter::PointStats(const ControlPoint &pcPoint) {
    mOstm << pcPoint.GetId()   << ", " << sPointType[(int)pcPoint.GetType()]
        << ", " << sBoolean[(int)pcPoint.IsIgnored()] << ", "
        << sBoolean[(int)pcPoint.IsEditLocked()] << ", "
        << pcPoint.GetNumMeasures() << ", "
        << pcPoint.GetNumMeasures() - pcPoint.GetNumValidMeasures() << ", "
        << pcPoint.GetNumLockedMeasures() << ", ";
  }

  /**
   * Print Cube's File and Serial Number into the Output File
   *
   * @author Sharmila Prasad (8/31/2010)
   *
   * @param pcMeasure - Measure's Cube and Serial #
   */
  void ControlNetFilter::PrintCubeFileSerialNum(const ControlMeasure &pcMeasure) {
    mOstm << mSerialNumList.FileName(pcMeasure.GetCubeSerialNumber()) << ", ";
    mOstm << pcMeasure.GetCubeSerialNumber();
  }

  /**
   * Print the Standard Cube Stats Header into Output File
   *
   * @author Sharmila Prasad (8/31/2010)
   */
  void ControlNetFilter::CubeStatsHeader(void) {
    mOstm << "FileName, SerialNumber, ImageTotalPoints, ImagePointsIgnored, ImagePointsEditLocked, ImagePointsFixed, ImagePointsConstrained, ImagePointsFree, ImageConvexHullRatio,";
  }

  /**
   * Filter Points by PixelShift
   *
   * @author Sharmila Prasad (7/20/2011)
   *
   * @param pvlGrp - Deffile Input group
   * @param pbLastFilter - Is this the last filter - for printing purposes.
   */
  void ControlNetFilter::PointPixelShiftFilter(const PvlGroup &pvlGrp, bool pbLastFilter){
    double dLesser = Isis::ValidMaximum;
    double dGreater = 0;

    if (pvlGrp.HasKeyword("LessThan")) {
      dLesser = fabs((pvlGrp["LessThan"][0]).ToDouble());
    }

    if (pvlGrp.HasKeyword("GreaterThan")) {
      dGreater = fabs((pvlGrp["GreaterThan"][0]).ToDouble());
    }

    if (dLesser < 0 || dGreater < 0 || dLesser <= dGreater) {
      string sErrMsg = "Invalid Deffile - Check Point_PixelShift Group\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
      return;
    }

    if (pbLastFilter) {
      mOstm << "PointID, PointType, PointIgnored, PointEditLocked, FileName, SerialNumber, PixelShift, MeasureType, MeasureIgnored, MeasureEditLocked, Reference, ";
      mOstm << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();
    for (int i = (iNumPoints - 1); i >= 0; i--) {
      ControlPoint *cPoint = mCNet->GetPoint(i);
      int iNumMeasures = cPoint->GetNumMeasures();
      bool bFilter = true;
      for (int j = 0; j < iNumMeasures; j++){
        const ControlMeasure *measure = cPoint->GetMeasure(j);
        double dPixelShift = measure->GetPixelShift();
        if (dPixelShift <= dLesser  && dPixelShift >= dGreater) {
          bFilter = false;
          break;
        }
      }
      if (bFilter) {
        FilterOutPoint(i);
        continue;
      }

      // Print into output, if it is the last Filter
      if (pbLastFilter) {
        for (int j = 0; j < iNumMeasures; j++) {
          mOstm << cPoint->GetId() << ", " << sPointType[cPoint->GetType()] << ", "
                << sBoolean[cPoint->IsIgnored()] << ", "
                << sBoolean[cPoint->IsEditLocked()] << ", ";

          const ControlMeasure *measure = cPoint->GetMeasure(j);
          PrintCubeFileSerialNum(*measure);
          double dPixelShift = measure->GetPixelShift();
          mOstm << ", " <<  (dPixelShift == Null ? "Null" : iString(dPixelShift)) << ", "
                << measure->GetMeasureTypeString() << ", "
                << sBoolean[measure->IsIgnored()] << ", "
                << sBoolean[measure->IsEditLocked()] << ", "
                << sBoolean[cPoint->GetRefMeasure() == measure]
                << endl;
        }
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter points based on number of measures EditLocked
   *
   * @author Sharmila Prasad (7/20/2011)
   *
   * Header - PointID, Type, Ignore, EditLock, NumMeasures, NumIgnoredMeasures, NumLockedMeasures,
   *
   * @param pvlGrp - Deffile Input group
   * @param pbLastFilter - Is this the last filter - for printing purposes.
   */
  void ControlNetFilter::PointNumMeasuresEditLockFilter(const PvlGroup &pvlGrp, bool pbLastFilter){
    int iLesser  = VALID_MAX2;
    int iGreater = 0;

    if (pvlGrp.HasKeyword("LessThan")) {
      iLesser = pvlGrp["LessThan"][0];
    }

    if (pvlGrp.HasKeyword("GreaterThan")) {
      iGreater = pvlGrp["GreaterThan"][0];
    }

    if (iLesser < 0 || iGreater < 0 || iLesser < iGreater) {
      string sErrMsg = "Invalid Deffile - Check Point_MeasureEditLock Group\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
      return;
    }

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << "FileName, SerialNumber, MeasureType, MeasureIgnored, MeasureEditLocked, Reference" << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();
    for (int i = (iNumPoints - 1); i >= 0; i--) {
      ControlPoint *cPoint = mCNet->GetPoint(i);
      int iNumLockedMsr = cPoint->GetNumLockedMeasures();
      //cerr << cPoint->GetId() << " NumMsrs=" << iNumLockedMsr << endl;
      if (iNumLockedMsr > iLesser || iNumLockedMsr < iGreater) {
        FilterOutPoint(i);
        continue;
      }

      int iNumMeasures = cPoint->GetNumMeasures();
      if (pbLastFilter) {
        for (int j = 0; j < iNumMeasures; j++) {
          const ControlMeasure *cm = cPoint->GetMeasure(j);
          PointStats(*cPoint);
          PrintCubeFileSerialNum(*cm);
          mOstm << ", " << cm->GetMeasureTypeString() << ", "
                << sBoolean[cm->IsIgnored()] << ", "
                << sBoolean[cm->IsEditLocked()] << ", "
                << sBoolean[cm == cPoint->GetRefMeasure()]
                << endl;
        }
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter points based on the point EditLock
   *
   * @author Sharmila Prasad (7/20/2011)
   *
   * Header - PointID, Type, Ignore, EditLock, NumMeasures, NumIgnoredMeasures, NumLockedMeasures,
   *
   * @param pvlGrp - Deffile Input group
   * @param pbLastFilter - Is this the last filter - for printing purposes.
   */
  void ControlNetFilter::PointEditLockFilter(const PvlGroup &pvlGrp, bool pbLastFilter){
    bool editLock = false;

    if (pvlGrp.HasKeyword("EditLock")) {
      if(pvlGrp["EditLock"][0] == "1" || iString(pvlGrp["EditLock"][0]).DownCase() == "true")
        editLock = true;
    }

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();
    for (int i = (iNumPoints - 1); i >= 0; i--) {
      ControlPoint *cPoint = mCNet->GetPoint(i);
      if (cPoint->IsEditLocked() != editLock) {
        FilterOutPoint(i);
        continue;
      }

      if (pbLastFilter) {
        int iNumMeasures = cPoint->GetNumMeasures();
        mOstm << cPoint->GetId() << ", " << sPointType[cPoint->GetType()] << ", "
              << sBoolean[cPoint->IsIgnored()] << ", "
              << sBoolean[cPoint->IsEditLocked()] << ", "
              << iNumMeasures << ", "
              << (iNumMeasures - cPoint->GetNumValidMeasures()) << ", "
              <<  cPoint->GetNumLockedMeasures() << endl;
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filters out the Control Network based on Residual Magnitude Criteria.
   * Group by Points
   *
   * @author Sharmila Prasad (8/11/2010)
   *
   * Header: PointID, Type, Ignore, EditLock, FileName, SerialNum, ResidualMagnitude, MeasureIgnore, MeasureLocked, Reference,
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointResMagnitudeFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    double dLesser  = Isis::ValidMaximum;
    double dGreater = 0;

    if (pvlGrp.HasKeyword("LessThan")) {
      dLesser = fabs((pvlGrp["LessThan"][0]).ToDouble());
    }

    if (pvlGrp.HasKeyword("GreaterThan")) {
      dGreater = fabs((pvlGrp["GreaterThan"][0]).ToDouble());
    }

    if (dLesser < 0 || dGreater < 0 || dLesser < dGreater) {
      string sErrMsg = "Invalid Deffile - Check Point_ResidualMagnitude Group\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
      return;
    }

    if (pbLastFilter) {
      mOstm << "PointID, PointType, PointIgnored, PointEditLocked, FileName, SerialNumber, ResidualMagnitude, MeasureType, MeasureIgnored, MeasureEditLocked, Reference, ";
      mOstm << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();
    for (int i = (iNumPoints - 1); i >= 0; i--) {
      bool bFilter = true;
      ControlPoint *cPoint = mCNet->GetPoint(i);
      int iNumMeasures = cPoint->GetNumMeasures();
      for (int j = 0; j < iNumMeasures; j++) {
        const ControlMeasure *measure = cPoint->GetMeasure(j);
        double dResMag = measure->GetResidualMagnitude();
        if (dResMag <= dLesser &&  dResMag >= dGreater) {
          bFilter = false;
          break;
        }
      }

      if (bFilter) {
        FilterOutPoint(i);
        continue;
      }
      // Print into output, if it is the last Filter
      else if (pbLastFilter) {

        for (int j = 0; j < iNumMeasures; j++) {
          mOstm << cPoint->GetId() << ", " << sPointType[cPoint->GetType()] << ", "
                << sBoolean[cPoint->IsIgnored()] << ", "
                << sBoolean[cPoint->IsEditLocked()] << ", ";

          const ControlMeasure *measure = cPoint->GetMeasure(j);
          PrintCubeFileSerialNum(*measure);
          double dResMag = measure->GetResidualMagnitude();
          mOstm << ", " << (dResMag == Null ? "Null" : iString(dResMag) ) << ", "
                << measure->GetMeasureTypeString() << ", "
                << sBoolean[measure->IsIgnored()] << ", "
                << sBoolean[measure->IsEditLocked()] << ", "
                << sBoolean[cPoint->GetRefMeasure() == measure]
                << endl;
        }
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter PointID based on regular expression
   * Group by Points
   *
   * @author Sharmila Prasad (8/11/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointIDFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    std::vector<string> strTokens;
    iString sPointIDExpr = pvlGrp["Expression"][0];
    iString sSeparator("*");

    string strToken = sPointIDExpr.Token(sSeparator);
    while (strToken != "") {
      strTokens.push_back(strToken);
      if (!sPointIDExpr.size()) {
        break;
      }
      strToken = sPointIDExpr.Token(sSeparator);
    }

    int iTokenSize = (int)strTokens.size();
    int iNumPoints = mCNet->GetNumPoints();
#ifdef _DEBUG_
    odb << "Net Size=" << iNumPoints << endl;
#endif

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << endl;
    }

    for (int i = (iNumPoints - 1); i >= 0; i--) {
      const ControlPoint *cPoint = mCNet->GetPoint(i);
      string sPointID = cPoint->GetId();
      int iPosition = 0;
      for (int j = (iTokenSize - 1); j >= 0; j--) {
        int iLen = strTokens[j].length();
        if (iLen > 0) {
          size_t found = sPointID.find(strTokens[j], iPosition);
          if (found != string::npos) {
            iPosition = found + iLen;
            // End of the expression
            if (pbLastFilter && j == (iTokenSize - 1)) {
              // Log into the output file
              PointStats(*cPoint);
              mOstm << endl;
            }
          }
          else {
            FilterOutPoint(i);
            break;
          }
        }
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filters the Control Network based on the user specified number of
   * measures in a Control Point. Group by Points
   *
   * @author Sharmila Prasad (8/12/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointMeasuresFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    int  iLesser = VALID_MAX2, iGreater = 0;

    if (pvlGrp.HasKeyword("LessThan")) {
      iLesser = pvlGrp["LessThan"][0];
    }

    if (pvlGrp.HasKeyword("GreaterThan")) {
      iGreater = pvlGrp["GreaterThan"][0];
    }

    if (iLesser < 0 || iGreater < 0 || iLesser < iGreater) {
      string sErrMsg = "Invalid Deffile - Check Point_NumMeasures Group\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
      return;
    }

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << "FileName, SerialNum, MeasureType, MeasureIgnore, MeasureEditLock, Reference" << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();

    for (int i = (iNumPoints - 1); i >= 0; i--) {
      const ControlPoint *cPoint = mCNet->GetPoint(i);
      int iNumMeasures = cPoint->GetNumMeasures();
      if (iNumMeasures > iLesser || iNumMeasures < iGreater) {
        FilterOutPoint(i);
        continue;
      }
      if (pbLastFilter) {
        for (int j = 0; j < iNumMeasures; j++) {
          const ControlMeasure *cm = cPoint->GetMeasure(j);
          PointStats(*cPoint);
          PrintCubeFileSerialNum(*cm);
          mOstm << ", " << cm->GetMeasureTypeString() << ", "
                << sBoolean[cm->IsIgnored()] << ", "
                << sBoolean[cm->IsEditLocked()] << ", "
                << sBoolean[cm == cPoint->GetRefMeasure()]
              << endl;
        }
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter the Control Network based on Ignored, Fixed Point Properties
   * Group by Points
   *
   * @author Sharmila Prasad (8/12/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointPropertiesFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    bool bIgnoredFlag = false;
    int iSetIgnoreFlag = 0;
    iString sType = "";
    iString sTemp = "";

    if (pvlGrp.HasKeyword("PointType")) {
      sType = pvlGrp["PointType"][0];
      sType = sType.DownCase(sType);
    }

    if (pvlGrp.HasKeyword("Ignore")) {
      iSetIgnoreFlag = 1;
      sTemp = pvlGrp["Ignore"][0];
      if (sTemp == "1" || sTemp.DownCase() == "true") {
        bIgnoredFlag = true;
      }
    }

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();

    for (int i = (iNumPoints - 1); i >= 0; i--) {
      const ControlPoint *cPoint = mCNet->GetPoint(i);
      bool bPointFound = false;
      bool bIgnored = cPoint->IsIgnored();
      bool bFixed       = (cPoint->GetType() == ControlPoint::Fixed       ? true : false);
      bool bConstrained = (cPoint->GetType() == ControlPoint::Constrained ? true : false);
      bool bFree        = (cPoint->GetType() == ControlPoint::Free        ? true : false);

      if (!iSetIgnoreFlag || bIgnoredFlag == bIgnored) {
        if (sType == "all" || sType=="") {
          bPointFound = true;
        }
        else if (sType == "fixed" && bFixed) {
          bPointFound = true;
        }
        else if (sType == "constrained" && bConstrained) {
          bPointFound = true;
        }
        else if (sType == "free" && bFree) {
          bPointFound = true;
        }
      }

      if(!bPointFound) {
        FilterOutPoint(i);
        continue;
      }

      // Output the Point Stats
      if (pbLastFilter) {
        PointStats(*cPoint);
        mOstm << endl;
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filters the Control Network based on Lat,Lon Range
   * Group by Points
   *
   * @author Sharmila Prasad (8/13/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointLatLonFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    double dMinLat = Isis::ValidMinimum, dMaxLat = Isis::ValidMaximum;
    double dMinLon = Isis::ValidMinimum, dMaxLon = Isis::ValidMaximum;

    if (pvlGrp.HasKeyword("MinLat")) {
      dMinLat = pvlGrp["MinLat"][0];
    }

    if (pvlGrp.HasKeyword("MaxLat")) {
      dMaxLat = pvlGrp["MaxLat"][0];
    }

    if (pvlGrp.HasKeyword("MinLon")) {
      dMinLon = pvlGrp["MinLon"][0];
    }

    if (pvlGrp.HasKeyword("MaxLon")) {
      dMaxLon = pvlGrp["MaxLon"][0];
    }

    if (dMinLat > dMaxLat || dMinLon > dMaxLon) {
      string sErrMsg = "Invalid Deffile - Check Point_LatLon Group\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
      return;
    }

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << "Latitude, Longitude, Radius" << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();

    for (int i = (iNumPoints - 1); i >= 0; i--) {
      const ControlPoint *cPoint = mCNet->GetPoint(i);
      SurfacePoint cPointSurfPt = cPoint->GetAdjustedSurfacePoint();

      if (!cPointSurfPt.Valid()) {
        const ControlMeasure *cm = cPoint->GetRefMeasure();

        string sn = cm->GetCubeSerialNumber();
        string filename = mSerialNumList.FileName(sn);
        Pvl pvl(filename);

        Camera *camera = CameraFactory::Create(pvl);
        if (camera->SetImage(cm->GetSample(), cm->GetLine())) {
          cPointSurfPt.SetSpherical(
            Latitude(camera->UniversalLatitude(), Angle::Degrees),
            Longitude(camera->UniversalLongitude(), Angle::Degrees),
            Distance(camera->LocalRadius()));
        }
      }
      double latitude  = cPointSurfPt.GetLatitude().degrees();
      double longitude = cPointSurfPt.GetLongitude().degrees();

      if ((latitude < dMinLat || latitude > dMaxLat) ||
          (longitude < dMinLon ||longitude > dMaxLon)) {
        FilterOutPoint(i);
        continue;
      }

      if (pbLastFilter) {
        PointStats(*cPoint);
        mOstm << latitude << ", " << longitude << ", " <<
          cPointSurfPt.GetLocalRadius().meters() << endl;
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter points that are within given distance of some other point
   * Group by Points
   *
   * @author Sharmila Prasad (8/13/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointDistanceFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    double dMaxDistance = 0;
    string sUnits = "pixels";

    if (pvlGrp.HasKeyword("MaxDistance")) {
      dMaxDistance = pvlGrp["MaxDistance"][0];
    }

    if (pvlGrp.HasKeyword("Units")) {
      sUnits = pvlGrp["Units"][0];
    }

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << "Point#Distance >>, " << endl;
    }

    bool bMinDistance = false;
    int iNumPoints = mCNet->GetNumPoints();
    for (int i = (iNumPoints - 1); i >= 0; i--) {
      const ControlPoint *cp1 = mCNet->GetPoint(i);
      const ControlMeasure *cp1RefMeasure = cp1->GetRefMeasure();
      SurfacePoint surfacePt1;
      Camera *cam1;

      double dSample1 = Isis::Null, dLine1 = Isis::Null;

      if (sUnits == "meters") {
        surfacePt1 = cp1->GetAdjustedSurfacePoint();

        if (!surfacePt1.Valid()) {
          string sn1 = cp1RefMeasure->GetCubeSerialNumber();
          string filename1 = mSerialNumList.FileName(sn1);
          Pvl pvl1(filename1);
          cam1 = CameraFactory::Create(pvl1);
          if (cam1->SetImage(cp1RefMeasure->GetSample(),
              cp1RefMeasure->GetLine())) {
            surfacePt1.SetSpherical(
              Latitude(cam1->UniversalLatitude(), Angle::Degrees),
              Longitude(cam1->UniversalLongitude(), Angle::Degrees),
              Distance(cam1->LocalRadius())
            );
          }
        }
      }
      else
        // pixels
      {
        dSample1 = cp1RefMeasure->GetSample();
        dLine1 = cp1RefMeasure->GetLine();
      }

      for (int j = (mCNet->GetNumPoints() - 1); j >= 0; j--) {
        if (i == j) {
          continue;
        }
        const ControlPoint *cp2 = mCNet->GetPoint(j);
        const ControlMeasure *cp2RefMeasure = cp2->GetRefMeasure();

        SurfacePoint surfacePt2;
        Camera *cam2;
        double dDist = 0;

        double dSample2 = Isis::Null, dLine2 = Isis::Null;

        if (sUnits == "meters") {
          surfacePt2 = cp2->GetAdjustedSurfacePoint();

          if (!surfacePt2.Valid()) {
            string sn2 = cp2RefMeasure->GetCubeSerialNumber();
            string filename2 = mSerialNumList.FileName(sn2);
            Pvl pvl2(filename2);
            cam2 = CameraFactory::Create(pvl2);

            if (cam2->SetImage(cp2RefMeasure->GetSample(),
                cp2RefMeasure->GetLine())) {
              surfacePt2.SetSpherical(
                Latitude(cam2->UniversalLatitude(), Angle::Degrees),
                Longitude(cam2->UniversalLongitude(), Angle::Degrees),
                Distance(cam2->LocalRadius())
              );
            }
          }

          dDist = surfacePt1.GetDistanceToPoint(surfacePt2,
              surfacePt1.GetLocalRadius()).meters();
        }
        else
          // pixels
        {
          dSample2 = cp2RefMeasure->GetSample();
          dLine2 = cp2RefMeasure->GetLine();

          double dDeltaSamp = dSample1 - dSample2;
          double dDeltaLine = dLine1 - dLine2;
          // use the distance formula for cartesian coordinates
          dDist = sqrt((dDeltaSamp * dDeltaSamp) + (dDeltaLine * dDeltaLine));
        }

        if (dDist <= dMaxDistance) {
          if (pbLastFilter) {
            if (!bMinDistance) {
              PointStats(*cp1);
            }
            mOstm << cp2->GetId() << "#" << dDist << ", ";
          }
          bMinDistance = true;
        }
        else
          continue;
      }
      if (!bMinDistance) {
        FilterOutPoint(i);
      }
      if (pbLastFilter && bMinDistance) {
        mOstm << endl;
      }
      bMinDistance = false;
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter the Points based on the Measures Goodness ofFit value
   * Group by Points
   *
   * Header:
   * PointID, PointType, PointIgnored, PointEditLocked, TotalMeasures, MeasuresIgnored, MeasuresEditLocked,
   * FileName, SerialNumber, GoodnessOfFit, MeasureIgnored, MeasureType, MeasureEditLocked, Reference
   *
   * @author Sharmila Prasad (8/16/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointGoodnessOfFitFilter(const PvlGroup & pvlGrp, bool pbLastFilter){
    double dLesser=Isis::ValidMaximum, dGreater=0;

    if (pvlGrp.HasKeyword("LessThan")){
      dLesser = fabs((pvlGrp["LessThan"][0]).ToDouble());
    }

    if (pvlGrp.HasKeyword("GreaterThan")){
      dGreater = fabs((pvlGrp["GreaterThan"][0]).ToDouble());
    }

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << "FileName, SerialNumber, GoodnessOfFit, MeasureType, MeasureIgnored, MeasureEditLocked, Reference" << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();
    for (int i=(iNumPoints-1); i>=0; i--) {
      ControlPoint *cPoint = mCNet->GetPoint(i);
      int iNumMeasures = cPoint->GetNumMeasures();
      bool bMatchFlag=false;

      for (int j=0; j<iNumMeasures; j++) {
        const ControlMeasure *measure = cPoint->GetMeasure(j);
        double dMsrGFit= measure->GetLogData(ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();
        if (dMsrGFit >= dGreater &&  dMsrGFit <= dLesser) {
          bMatchFlag = true;
          break;
        }
      }

      if (!bMatchFlag) {
        FilterOutPoint(i);
      }
      else {
        // Print into output, if it is the last Filter
        if (pbLastFilter) {
          int iNumMeasures = cPoint->GetNumMeasures();
          int iNumMsIgnored = iNumMeasures - cPoint->GetNumValidMeasures();
          for (int j = 0; j < iNumMeasures; j++) {
            const ControlMeasure *measure = cPoint->GetMeasure(j);
            double dMsrGFit= measure->GetLogData(ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();

            mOstm << cPoint->GetId() << ", " << sPointType[cPoint->GetType()] << ", "
              << sBoolean[cPoint->IsIgnored()] << ", " << sBoolean[cPoint->IsEditLocked()] << ", "
              << iNumMeasures << ", " << iNumMsIgnored << ", " << cPoint->GetNumLockedMeasures() << ", ";
            PrintCubeFileSerialNum(*measure);
            mOstm << ", " <<  (dMsrGFit == Null ? "NA" : iString(dMsrGFit)) << ", "
              << measure->GetMeasureTypeString() << ", "
              << sBoolean[measure->IsIgnored()] << ", "
              << sBoolean[measure->IsEditLocked()] << ", "
              << sBoolean[cPoint->GetRefMeasure() == measure]
              << endl;
          }
        }
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }
  /**
   * Filter the Points which have measures of specified Measure type and Ignored Flag.
   * Group by Points
   *
   * @author Sharmila Prasad (8/13/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointMeasurePropertiesFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    int iIgnoredFlag = -1;
    string sType = "";
    iString isType;

    if (pvlGrp.HasKeyword("Ignore")) {
      iIgnoredFlag = 0;
      if (iString(pvlGrp["Ignore"][0]).DownCase() == "true")
        iIgnoredFlag = 1;
    }

    if (pvlGrp.HasKeyword("MeasureType")) {
      sType = iString(pvlGrp["MeasureType"][0]).DownCase();
    }

    if (pbLastFilter) {
      PointStatsHeader();
      mOstm << "FileName, SerialNumber, MeasureIgnored, MeasureType, MeasureEditLocked, Reference," << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();
    for (int i = (iNumPoints - 1); i >= 0; i--) {
      const ControlPoint *cPoint = mCNet->GetPoint(i);

      int iNumMeasures = cPoint->GetNumMeasures();
      int iNotMeasureType = 0;
      for (int j = 0; j < iNumMeasures; j++) {
        const ControlMeasure *cMeasure = cPoint->GetMeasure(j);
        bool bMeasureIgnored = cMeasure->IsIgnored();
        bool bMeasureFound = false;

        if (iIgnoredFlag == -1 || bMeasureIgnored == iIgnoredFlag) {
          if (sType == "all" || sType == "") {
            bMeasureFound = true;
          }
          else if (sType == "candidate" && cMeasure->GetType() == ControlMeasure::Candidate) {
            bMeasureFound = true;
          }
          else if (sType == "manual" && cMeasure->GetType() == ControlMeasure::Manual) {
            bMeasureFound = true;
          }
          else if (sType == "registeredpixel" && cMeasure->GetType() == ControlMeasure::RegisteredPixel) {
            bMeasureFound = true;
          }
          else if (sType == "registeredsubpixel" && cMeasure->GetType() == ControlMeasure::RegisteredSubPixel) {
            bMeasureFound = true;
          }
        }
        if (bMeasureFound) {
          if (pbLastFilter) {
            PointStats(*cPoint);
            string sn = cMeasure->GetCubeSerialNumber();
            mOstm << mSerialNumList.FileName(sn) << ", " << sn << ","
                << sBoolean[(int) cMeasure->IsIgnored()] << ", "
                << cMeasure->GetMeasureTypeString() << ", "
                << sBoolean[cMeasure->IsEditLocked()] << ", "
                << sBoolean[cPoint->GetRefMeasure() == cMeasure]
                << endl;
          }
        }
        else
          iNotMeasureType++;
      }
      //cerr << cPoint->GetId() << " NumMeasures=" << iNumMeasures << " NotFound=" << iNotMeasureType << endl;
      if (iNotMeasureType == iNumMeasures) {
        FilterOutPoint(i);
        continue;
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter points based on the image serial # - Group by Point
   *
   * @author Sharmila Prasad (8/16/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::PointCubeNamesFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    std::vector <string> sCubeNames;

    // Store the Cubenames from the PvlGroup
    for (int i = 0; i < pvlGrp.Keywords(); i++) {
      sCubeNames.push_back(pvlGrp[i][0]);
    }

    int size = sCubeNames.size();

    if (pbLastFilter) {
      PointStatsHeader();
      CubeStatsHeader();
      mOstm << "ImageMeasureIgnored, ImageMeasureEditLocked, ";
      mOstm << endl;
    }

    int iNumPoints = mCNet->GetNumPoints();
    for (int i = (iNumPoints - 1); i >= 0; i--) {
      const ControlPoint *cPoint = mCNet->GetPoint(i);
      int iNumMeasures = cPoint->GetNumMeasures();
      int iNumNoMatch = 0;
      bool bMatch = false;
      for (int j = 0; j < iNumMeasures; j++) {
        const ControlMeasure *cMeasure = cPoint->GetMeasure(j);
        for (int k = 0; k < size; k++) {
          if (cMeasure->GetCubeSerialNumber() == sCubeNames[k]) {
            bMatch = true;
            break;
          }
        }
        if (!bMatch) {
          iNumNoMatch++;
        }
      }
      if (iNumNoMatch == iNumMeasures) {
        FilterOutPoint(i);
      }
    } //end point loop

    // update the image stats with the changes
    GenerateImageStats();

    // If Last filter print to the output file in the required format
    if (pbLastFilter) {
      iNumPoints = mCNet->GetNumPoints();
      for (int i = 0; i < iNumPoints; i++) {
        const ControlPoint *cPoint = mCNet->GetPoint(i);
        int iNumMeasures = cPoint->GetNumMeasures();
        for (int j = 0; j < iNumMeasures; j++) {
          const ControlMeasure *cMeasure = cPoint->GetMeasure(j);

          // Point Details
          mOstm << cPoint->GetId() << ", " << sPointType[cPoint->GetType()] << ", "
                << sBoolean[cPoint->IsIgnored()] << ", "
                << sBoolean[cPoint->IsEditLocked()] << ", "
                << iNumMeasures << ", "
                << iNumMeasures - cPoint->GetNumValidMeasures() << ", "
                << cPoint->GetNumLockedMeasures() << ", ";

          // Image Details
          string sn = cMeasure->GetCubeSerialNumber();
          vector <double> imgStats = GetImageStatsBySerialNum(sn);
          mOstm << mSerialNumList.FileName(sn)   << ", " << sn << ", "
                << imgStats[imgTotalPoints] << ", " << imgStats[imgIgnoredPoints] << ", "
                << imgStats[imgLockedPoints] << ", " << imgStats[imgFixedPoints] << ", "
                << imgStats[imgConstrainedPoints] << ", " << imgStats[imgFreePoints] << ", "
                << imgStats[imgConvexHullRatio] << ", "
                << sBoolean[cMeasure->IsIgnored()] << ", " << sBoolean[cMeasure->IsEditLocked()] << endl;
        }
      }
    }
  }

  /**
   * Filter Cubes by its ConvexHull Ratio (Ratio = Convex Hull / Image Area).
   * ConvexHull is calculated only for valid Control Points
   *
   * @author Sharmila Prasad (11/2/2011)
   *
   * @param pvlGrp       - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   *
   *  Group = Cube_ConvexHullRatio
   *     LessThan = double
   *     GreaterThan = double
   *  EndGroup
   */
  void ControlNetFilter::CubeConvexHullFilter(const PvlGroup &pvlGrp, bool pbLastFilter){
    double dLesser = Isis::ValidMaximum;
    double dGreater = 0;

    if (pvlGrp.HasKeyword("LessThan")) {
      dLesser = fabs((pvlGrp["LessThan"][0]).ToDouble());
    }

    if (pvlGrp.HasKeyword("GreaterThan")) {
      dGreater = fabs((pvlGrp["GreaterThan"][0]).ToDouble());
    }

    if (dLesser < 0 || dGreater < 0 || dLesser <= dGreater) {
      string sErrMsg = "Invalid Deffile - Check Cube_ConvexHullRatio Group\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
      return;
    }

    if (pbLastFilter) {
      CubeStatsHeader();
      mOstm << endl;
    }

    int iNumCubes  = mSerialNumFilter.Size();

    for (int sn = (iNumCubes - 1); sn >= 0; sn--) {
      string sSerialNum = mSerialNumFilter.SerialNumber(sn);
      vector<double> imgStats = GetImageStatsBySerialNum(sSerialNum);
      double convexHullRatio = imgStats[imgConvexHullRatio];
      if (convexHullRatio < dGreater || convexHullRatio > dLesser){
        FilterOutMeasuresBySerialNum(sSerialNum);
        mSerialNumFilter.Delete(sSerialNum);
      }
      else if (pbLastFilter) {
        mOstm << mSerialNumFilter.FileName(sSerialNum) << ", " << sSerialNum << ", "
              << imgStats[imgTotalPoints]  << ", " << imgStats[imgIgnoredPoints] << ", " << imgStats[imgLockedPoints] << ", "
              << imgStats[imgFixedPoints] << ", " << imgStats[imgConstrainedPoints] << ", " << imgStats[imgFreePoints] << ", "
              <<  imgStats[imgConvexHullRatio]<< endl;
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter Cube names in Control Network by cube name expression
   * Group by Image
   *
   * @author Sharmila Prasad (8/16/2010)
   *
   * @param pvlGrp       - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::CubeNameExpressionFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    iString sCubeExpr("");
    if (pvlGrp.HasKeyword("Expression")) {
      sCubeExpr = iString(pvlGrp["Expression"][0]);
    }

    vector <string> strTokens;
    iString sSeparator("*");
    string strToken = sCubeExpr.Token(sSeparator);
    while (strToken != "") {
      strTokens.push_back(strToken);
      if (!sCubeExpr.size()) {
        break;
      }
      strToken = sCubeExpr.Token(sSeparator);
    }

    int iTokenSize = (int)strTokens.size();
    int iNumCubes  = mSerialNumFilter.Size();

    if (pbLastFilter) {
      CubeStatsHeader();
      mOstm << endl;
    }

    for (int i = (iNumCubes - 1); i >= 0;  i--) {
      string sCubeName = mSerialNumFilter.FileName(i);
      string sSerialNum = mSerialNumFilter.SerialNumber(i);
      int iPosition = 0;
      for (int j = (iTokenSize - 1); j >= 0; j--) {
        int iLen = strTokens[j].length();
        if (iLen > 0) {
          size_t found = sSerialNum.find(strTokens[j], iPosition);
          if (found != string::npos) {
            iPosition = found + iLen;
            // End of the expression - Found
            if (j == (iTokenSize - 1)) {
              break;
            }
          }
          else {
            FilterOutMeasuresBySerialNum(sSerialNum);
            mSerialNumFilter.Delete(sSerialNum);
            break;
          }
        }
      }
    }

    // update the image stats with the changes
    GenerateImageStats();

    if (pbLastFilter) {
      iNumCubes = mSerialNumFilter.Size();
      for (int i = 0; i < iNumCubes; i++) {
        string sSerialNum = mSerialNumFilter.SerialNumber(i);

        mOstm << mSerialNumFilter.FileName(i) << ", " << sSerialNum << ", ";
        vector<double> imgStats = GetImageStatsBySerialNum(sSerialNum);
        mOstm << mSerialNumFilter.FileName(sSerialNum) << ", " << sSerialNum << ", "
              << imgStats[imgTotalPoints]  << ", " << imgStats[imgIgnoredPoints] << ", " << imgStats[imgLockedPoints] << ", "
              << imgStats[imgFixedPoints] << ", " << imgStats[imgConstrainedPoints] << ", " << imgStats[imgFreePoints] << ", "
              << imgStats[imgConvexHullRatio]<< endl;
      }
    }
  }

  /**
   * Filter the cube by the number of points in each cube
   * Group by Image
   *
   * @author Sharmila Prasad (8/16/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::CubeNumPointsFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    int iLessPoints = VALID_MAX2, iGreaterPoints = 0;
    if (pvlGrp.HasKeyword("LessThan")) {
      iLessPoints = pvlGrp["LessThan"][0];
    }
    if (pvlGrp.HasKeyword("GreaterThan")) {
      iGreaterPoints = pvlGrp["GreaterThan"][0];
    }

    if (iLessPoints < 0 || iGreaterPoints < 0 || iLessPoints < iGreaterPoints) {
      string sErrMsg = "Invalid Deffile - Check Cube_NumPoints Group\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
      return;
    }

    if (pbLastFilter) {
      CubeStatsHeader();
      mOstm << endl;
    }

    int iNumCubes  = mSerialNumFilter.Size();

    for (int sn = (iNumCubes - 1); sn >= 0; sn--) {
      string sSerialNum = mSerialNumFilter.SerialNumber(sn);
      vector<double> imgStats = GetImageStatsBySerialNum(sSerialNum);
      double numPoints = imgStats[imgTotalPoints];
      if (numPoints < iGreaterPoints || numPoints > iLessPoints){
        FilterOutMeasuresBySerialNum(sSerialNum);
        mSerialNumFilter.Delete(sSerialNum);
      }
      else if (pbLastFilter) {
        mOstm << mSerialNumFilter.FileName(sSerialNum) << ", " << sSerialNum << ", "
              << imgStats[imgTotalPoints]  << ", " << imgStats[imgIgnoredPoints] << ", " << imgStats[imgLockedPoints] << ", "
              << imgStats[imgFixedPoints] << ", " << imgStats[imgConstrainedPoints] << ", " << imgStats[imgFreePoints] << ", "
              <<  imgStats[imgConvexHullRatio] << endl;
      }
    }

    // update the image stats with the changes
    GenerateImageStats();
  }

  /**
   * Filter by distance between points in Cube
   * Group by Image
   *
   * @author Sharmila Prasad (8/17/2010)
   *
   * @param pvlGrp - Pvl Group containing the filter info
   * @param pbLastFilter - Flag to indicate whether this is the last filter to print the stats
   */
  void ControlNetFilter::CubeDistanceFilter(const PvlGroup &pvlGrp, bool pbLastFilter) {
    double dDistance = 0;
    string sUnits = "pixels";

    if (pvlGrp.HasKeyword("MaxDistance")) {
      dDistance = pvlGrp["MaxDistance"][0];
    }

    if (pvlGrp.HasKeyword("Units")) {
      sUnits = pvlGrp["Units"][0];
    }

    if (dDistance <= 0) {
      string sErrMsg = "Invalid Deffile - Check Cube_Distance Group\n";
      throw IException(IException::User, sErrMsg, _FILEINFO_);
      return;
    }

    if (pbLastFilter) {
      CubeStatsHeader();
      mOstm << "Distance_PointIDs >>, " << endl;
    }

    int iNumCubes = mSerialNumFilter.Size();
    for (int sn = (iNumCubes - 1); sn >= 0; sn--) {
      string sSerialNum = mSerialNumFilter.SerialNumber(sn);
      Pvl pvl(mSerialNumList.FileName(sSerialNum));
      Camera *cam = CameraFactory::Create(pvl);
      double dDist = 0;
      bool bMatchDistance = false;

      std::vector <int> sPointIndex1;
      std::vector <int> sPointIndex2;
      std::vector <double> dPointDistance;

      // Point stats
      int iPointsTotal   = 0;
      int iPointsIgnored = 0;
      int iPointsFixed  = 0;
      int iPointsConstrained = 0;
      int iPointsFree = 0;
      int iPointsLocked  = 0;

      // Reset the vectors
      sPointIndex1.clear();
      sPointIndex2.clear();
      dPointDistance.clear();

      int iNumPoints = mCNet->GetNumPoints();
      for (int i = 0; i < iNumPoints; i++) {
        const ControlPoint *cPoint1 = mCNet->GetPoint(i);
        const ControlMeasure *cMeasure1;
        bool bImageFound = false;
        int iNumMeasures1 = cPoint1->GetNumMeasures();
        for (int j = 0; j < iNumMeasures1; j++) {
          cMeasure1 = cPoint1->GetMeasure(j);
          if (cMeasure1->GetCubeSerialNumber() == sSerialNum) {
            iPointsTotal++;
            if (cPoint1->IsIgnored()) {
              iPointsIgnored++;
            }
            if (cPoint1->GetType() == ControlPoint::Fixed) {
              iPointsFixed++;
            }
            if (cPoint1->GetType() == ControlPoint::Constrained) {
              iPointsConstrained++;
            }
            if (cPoint1->GetType() == ControlPoint::Free) {
              iPointsFree++;
            }
            if (cPoint1->IsEditLocked()) {
              iPointsLocked++;
            }
            bImageFound = true;
            break;
          }
        }
        if (!bImageFound) {
          continue;
        }

        //if(cMeasure1.Sample()==0 && cMeasure1.Line()==0) continue;

        // if the user chooses distance in meters, create camera to find lat/lon for this measure
        double dRadius = 0, dLat1 = 0, dLon1 = 0;
        if (sUnits == "meters") {
          // try to set image using sample/line values
          if (cam->SetImage(cMeasure1->GetSample(), cMeasure1->GetLine())) {
            dRadius = cam->LocalRadius().meters();
            dLat1 = cam->UniversalLatitude();
            dLon1 = cam->UniversalLongitude();
          }
          else
            continue;
        }

        for (int k = (i + 1); k < iNumPoints; k++) {
          const ControlPoint *cPoint2 = mCNet->GetPoint(k);
          int iNumMeasures2 = cPoint2->GetNumMeasures();
          const ControlMeasure *cMeasure2;
          bool bImageFound2 = false;

          for (int j = 0; j < iNumMeasures2; j++) {
            if (cPoint2->GetMeasure(j)->GetCubeSerialNumber() == sSerialNum) {
              cMeasure2 = cPoint2->GetMeasure(j);
              bImageFound2 = true;
              break;
            }
          }
          if (!bImageFound2 ||
              (cMeasure2->GetSample() == 0 && cMeasure2->GetLine() == 0))
            continue;

          if (sUnits == "pixels") {
            double dDeltaSamp = cMeasure1->GetSample() - cMeasure2->GetSample();
            double dDeltaLine = cMeasure1->GetLine() - cMeasure2->GetLine();
            // use the distance formula for cartesian coordinates
            dDist = sqrt((dDeltaSamp * dDeltaSamp) + (dDeltaLine * dDeltaLine));
          }
          else
            // calculate distance in meters
          {
            double dLat2 = 0, dLon2 = 0;
            if (cam->SetImage(cMeasure2->GetSample(), cMeasure2->GetLine())) {
              dLat2 = cam->UniversalLatitude();
              dLon2 = cam->UniversalLongitude();
            }
            else
              continue;

            // Calculate the distance between the two points
            Latitude lat1(dLat1, Angle::Degrees);
            Longitude lon1(dLon1, Angle::Degrees);
            Latitude lat2(dLat2, Angle::Degrees);
            Longitude lon2(dLon2, Angle::Degrees);
            Distance radius(dRadius, Distance::Meters);

            SurfacePoint point1(lat1, lon1, radius);
            SurfacePoint point2(lat2, lon2, radius);

            dDist = point1.GetDistanceToPoint(point1, radius).meters();
          }
          if (!dDist || dDist >= dDistance) {
            continue;
          }
          else {
            bMatchDistance = true;
            sPointIndex1.push_back(i);
            sPointIndex2.push_back(k);
            dPointDistance.push_back(dDist);
            //break;
          }
        }// end Loop Point2
        //if (bMatchDistance) {
        // break;
        //}
      } //end Loop Point1
      if (!bMatchDistance) {
        FilterOutMeasuresBySerialNum(sSerialNum);
        mSerialNumFilter.Delete(sSerialNum);
      }
      else if (pbLastFilter) {
        vector <double> imgStats = GetImageStatsBySerialNum((sSerialNum));
        mOstm << mSerialNumList.FileName(sSerialNum) << ", " << sSerialNum << ", "
              << iPointsTotal << ", " << iPointsIgnored << ", " << iPointsLocked << ", "
              << iPointsFixed << ", " << iPointsConstrained << ", " << iPointsFree << ", "
              << imgStats[ imgConvexHullRatio] << ", ";
        for (int j = 0; j < (int)sPointIndex1.size(); j++) {
          iString sPointIDDist(dPointDistance[j]);
          sPointIDDist += "#";
          sPointIDDist += (*mCNet)[sPointIndex1[j]]->GetId();
          sPointIDDist += "#";
          sPointIDDist += (*mCNet)[sPointIndex2[j]]->GetId();

          mOstm << (string)sPointIDDist << ",";
        }
        mOstm << endl;
      }
      delete(cam);
    } // end cube loop

    // update the image stats with the changes
    GenerateImageStats();
  }
}

