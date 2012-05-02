#include "Isis.h"
#include "IsisDebug.h"
#include "IException.h"
#include "Pvl.h"
#include "ControlNet.h"
#include "ControlNetFilter.h"
#include "ControlNetStatistics.h"
#include "PvlGroup.h"
#include "Progress.h"

using namespace Isis;
using namespace std;

void ReadDefFile(ControlNetFilter & pcNetFilter, Pvl & pvlDefFile);
void (ControlNetFilter::*GetPtr2Filter(const string sFilter)) (const PvlGroup & pvlGrp, bool pbLastFilter);

void IsisMain() {

  try {
    // Process all the inputs first, for errors and to satisfy requirements
    UserInterface &ui = Application::GetUserInterface();
    string sSerialNumFile = ui.GetFileName("FROMLIST");

    // Get the DefFile
    string sDefFile = "";
    string sOutFile = "";
    Pvl pvlDefFile;
    if (ui.WasEntered("DEFFILE")) {
      sDefFile = ui.GetFileName("DEFFILE");
      sOutFile = ui.GetFileName("FLATFILE");
      pvlDefFile = Pvl(sDefFile);

      // Log the DefFile - Cannot log Object... only by Group
      for (int i=0; i<pvlDefFile.Objects(); i++) {
        PvlObject pvlObj = pvlDefFile.Object(i);
        for (int j=0; j<pvlObj.Groups(); j++) {
          Application::Log(pvlObj.Group(j));
        }
      }

      // Verify DefFile comparing with the Template
      Pvl pvlTemplate("$ISIS3DATA/base/templates/cnetstats/cnetstats.def");
      //Pvl pvlTemplate("/home/sprasad/isis3/isis/src/control/apps/cnetstats/cnetstats.def");
      Pvl pvlResults;
      pvlTemplate.ValidatePvl(pvlDefFile, pvlResults);
      if(pvlResults.Objects() != 0 || pvlResults.Groups() != 0 || pvlResults.Keywords() != 0){
        for (int i=0; i<pvlResults.Objects(); i++) {
          PvlObject pvlObj = pvlResults.Object(i);
          for (int j=0; j<pvlObj.Groups(); j++) {
            Application::Log(pvlObj.Group(j));
          }
        }
        string sErrMsg = "Invalid Deffile\n";
        throw IException(IException::User, sErrMsg, _FILEINFO_);
      }
    }

    // Get the Image Stats File
    string sImageFile= "";
    if (ui.WasEntered("CREATE_IMAGE_STATS") && ui.GetBoolean("CREATE_IMAGE_STATS")) {
      sImageFile = ui.GetFileName("IMAGE_STATS_FILE");
    }

    // Get the Point Stats File
    string sPointFile="";
    if (ui.WasEntered("CREATE_POINT_STATS") && ui.GetBoolean("CREATE_POINT_STATS")) {
      sPointFile = ui.GetFileName("POINT_STATS_FILE");
    }

     // Get the original control net internalized
    Progress progress;
    ControlNet cNet(ui.GetFileName("CNET"), &progress);

    Progress statsProgress;
    ControlNetFilter cNetFilter(&cNet, sSerialNumFile, &statsProgress);

    // Log the summary of the input Control Network
    PvlGroup statsGrp;
    cNetFilter.GenerateControlNetStats(statsGrp);
    Application::Log(statsGrp);

    // Run Filters using Deffile
    if (ui.WasEntered("DEFFILE")) {
      cNetFilter.SetOutputFile(sOutFile);
      ReadDefFile(cNetFilter, pvlDefFile);
    }

    // Run Image Stats
    if (ui.WasEntered("CREATE_IMAGE_STATS") && ui.GetBoolean("CREATE_IMAGE_STATS")) {
      cNetFilter.GenerateImageStats();
      cNetFilter.PrintImageStats(sImageFile);
    }

    // Run Point Stats
    if (ui.WasEntered("CREATE_POINT_STATS") && ui.GetBoolean("CREATE_POINT_STATS")) {
      cNetFilter.GeneratePointStats(sPointFile);
    }
  } // REFORMAT THESE ERRORS INTO ISIS TYPES AND RETHROW
  catch(IException &) {
    throw;
  }
}

/**
 * Reads the DefFile having info about the different filters to
 * be used on the Control Network.
 *
 * @author Sharmila Prasad (9/7/2010)
 *
 * @param pcNetFilter
 * @param pvlDefFile
 */
void ReadDefFile(ControlNetFilter & pcNetFilter, Pvl & pvlDefFile)
{
  // prototype to ControlNetFilter member function
  void (ControlNetFilter::*pt2Filter)(const PvlGroup & pvlGrp, bool pbLastFilter);

  // Parse the Groups in Point Object
  PvlObject filtersObj = pvlDefFile.FindObject("Filters", Pvl::Traverse);
  int iNumGroups = filtersObj.Groups();

  for (int i=0; i<iNumGroups; i++) {
    PvlGroup pvlGrp = filtersObj.Group(i);
    // Get the pointer to ControlNetFilter member function based on Group name
    pt2Filter=GetPtr2Filter(pvlGrp.Name());
    if (pt2Filter != NULL) {
      (pcNetFilter.*pt2Filter)(pvlGrp, ((i==(iNumGroups-1)) ? true : false));
    }
  }
}

/**
 * Returns the pointer to filter function based on the Group name string
 *
 * @author Sharmila Prasad (8/11/2010)
 *
 * @return void(ControlNetFilter::*GetPtr2Filter)(const PvlGroup&pvlGrp)
 */
void (ControlNetFilter::*GetPtr2Filter(const string psFilter)) (const PvlGroup & pvlGrp, bool pbLastFilter)
{
  // Point Filters
  if (psFilter == "Point_PixelShift") {
    return &ControlNetFilter::PointPixelShiftFilter;
  }
  if (psFilter == "Point_EditLock")
    return &ControlNetFilter::PointEditLockFilter;

  if (psFilter == "Point_NumMeasuresEditLock") {
    return &ControlNetFilter::PointNumMeasuresEditLockFilter;
  }

  if (psFilter == "Point_ResidualMagnitude"){
    return &ControlNetFilter::PointResMagnitudeFilter;
  }

  if (psFilter == "Point_GoodnessOfFit") {
    return &ControlNetFilter::PointGoodnessOfFitFilter;
  }

  if (psFilter == "Point_IdExpression") {
    return &ControlNetFilter::PointIDFilter;
  }

  if (psFilter == "Point_NumMeasures"){
    return &ControlNetFilter::PointMeasuresFilter;
  }

  if (psFilter == "Point_Properties") {
    return &ControlNetFilter::PointPropertiesFilter;
  }

  if (psFilter == "Point_LatLon") {
    return &ControlNetFilter::PointLatLonFilter;
  }

  if (psFilter == "Point_Distance") {
    return &ControlNetFilter::PointDistanceFilter;
  }

  if (psFilter == "Point_MeasureProperties") {
    return &ControlNetFilter::PointMeasurePropertiesFilter;
  }

  if (psFilter == "Point_CubeNames") {
    return &ControlNetFilter::PointCubeNamesFilter;
  }

  // Cube Filters
  if (psFilter == "Cube_NameExpression") {
    return &ControlNetFilter::CubeNameExpressionFilter;
  }

  if (psFilter == "Cube_NumPoints") {
    return &ControlNetFilter::CubeNumPointsFilter;
  }

  if (psFilter == "Cube_Distance") {
    return &ControlNetFilter::CubeDistanceFilter;
  }

  if (psFilter == "Cube_ConvexHullRatio") {
    return &ControlNetFilter::CubeConvexHullFilter;
  }

  return NULL;
}


