#ifndef CSMCamera_h
#define CSMCamera_h
/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Camera.h"
#include "iTime.h"
#include "Target.h"

#include <vector>

#include <QList>
#include <QPointF>
#include <QStringList>

#include "csm/csm.h"
#include "csm/RasterGM.h"

namespace Isis {
  class CSMCamera : public Camera {

    public:
      // constructors
      CSMCamera(Cube &cube);
//      CSMCamera(Cube &cube, QString pluginName, QString modelName, QString stateString);

      //! Destroys the CSMCamera object.
      ~CSMCamera() {};

      /**
       * The CSM camera needs a bogus type for now.
       *
       * @return CameraType Camera::Point
       */
      virtual CameraType GetCameraType() const {
        return Csm;
      }

      /**
       * CK frame ID -  - Instrument Code from spacit run on CK
       *
       * @return @b int The appropriate instrument code for the "Camera-matrix"
       *         Kernel Frame ID
       */
      virtual int CkFrameId() const { return (-1); }

      /**
       * CK Reference ID - J2000
       *
       * @return @b int The appropriate instrument code for the "Camera-matrix"
       *         Kernel Reference ID
       */
      virtual int CkReferenceId() const { return (-1); }

      /**
       *  SPK Center ID - 6 (Saturn)
       *
       * @return @b int The appropriate instrument code for the Spacecraft
       *         Kernel Center ID
       */
      virtual int SpkCenterId() const { return -1; }

      /**
       *  SPK Reference ID - J2000
       *
       * @return @b int The appropriate instrument code for the Spacecraft
       *         Kernel Reference ID
       */
      virtual int SpkReferenceId() const { return (-1); }

      virtual QList<QPointF> PixelIfovOffsets();

      virtual bool SetImage(const double sample, const double line, NaifContextPtr naif) override;

      virtual bool SetGround(NaifContextPtr naif, Latitude latitude, Longitude longitude) override;
      virtual bool SetGround(NaifContextPtr naif, const SurfacePoint &surfacePt) override;
      virtual bool SetUniversalGround(NaifContextPtr naif, const double latitude, const double longitude) override;
      virtual bool SetUniversalGround(NaifContextPtr naif, const double latitude, const double longitude, double radius) override;

      virtual void setTime(const iTime &time, NaifContextPtr naif) override;

      virtual double LineResolution(NaifContextPtr naif) override;
      virtual double SampleResolution(NaifContextPtr naif) override;
      virtual double DetectorResolution(NaifContextPtr naif) override;
      virtual double ObliqueLineResolution(NaifContextPtr naif) override;
      virtual double ObliqueSampleResolution(NaifContextPtr naif) override;
      virtual double ObliqueDetectorResolution(NaifContextPtr naif) override;

      virtual double parentLine() const;
      virtual double parentSample() const;

      virtual void subSpacecraftPoint(double &lat, double &lon, NaifContextPtr naif) override;
      virtual void subSpacecraftPoint(double &lat, double &lon, double line, double sample);
      virtual void subSolarPoint(double &lat, double &lon, NaifContextPtr naif);

      virtual double PhaseAngle(NaifContextPtr naif) const;
      virtual double EmissionAngle(NaifContextPtr naif) const;
      virtual double IncidenceAngle() const;

      virtual SpicePosition *sunPosition() const;
      virtual SpicePosition *instrumentPosition() const;
      virtual SpiceRotation *bodyRotation() const;
      virtual SpiceRotation *instrumentRotation() const;

      virtual void instrumentBodyFixedPosition(double p[3], NaifContextPtr naif) const override;
      virtual void sunPosition(double p[3], NaifContextPtr naif) const override;
      virtual double SolarDistance() const;

      virtual double SlantDistance(NaifContextPtr naif) const;
      virtual double targetCenterDistance(NaifContextPtr naif) const override;

      virtual double RightAscension(NaifContextPtr naif);
      virtual double Declination(NaifContextPtr naif);

      std::vector<int> getParameterIndices(csm::param::Set paramSet) const;
      std::vector<int> getParameterIndices(csm::param::Type paramType) const;
      std::vector<int> getParameterIndices(QStringList paramList) const;
      void applyParameterCorrection(int index, double correction);
      double getParameterCovariance(int index1, int index2);
      QString getParameterName(int index);
      QString getParameterUnits(int index);
      double getParameterValue(int index);

      std::vector<double> getSensorPartials(int index, SurfacePoint groundPoint);
      virtual std::vector<double> GroundPartials(SurfacePoint groundPoint);
      virtual std::vector<double> GroundPartials();

      QString getModelState() const;

    protected:
      void setTarget(Pvl label);

      std::vector<double> sensorPositionBodyFixed() const;
      std::vector<double> sensorPositionBodyFixed(double line, double sample) const;

      virtual void computeSolarLongitude(iTime et, NaifContextPtr naif) override;

    private:
      void init(Cube &cube, QString pluginName, QString modelName, QString stateString);

      csm::RasterGM *m_model; //! CSM sensor model
      iTime m_refTime; //! The reference time that all model image times are relative to

      void isisToCsmPixel(double line, double sample, csm::ImageCoord &csmPixel) const;
      void csmToIsisPixel(csm::ImageCoord csmPixel, double &line, double &sample) const;
      csm::EcefCoord isisToCsmGround(const SurfacePoint &groundPt) const;
      SurfacePoint csmToIsisGround(const csm::EcefCoord &groundPt) const;

      virtual std::vector<double> ImagePartials(SurfacePoint groundPoint);
      virtual std::vector<double> ImagePartials();
  };
};
#endif
