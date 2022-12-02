KPL/FK

MARINER 9 NAIF Name/ID Definitions Kernel
===============================================================================

   This text kernel contains name-to-NAIF ID mappings for MARINER 9
   (M9) mission.


Version and Date
--------------------------------------------------------

   Version 1.0 -- June 4, 2008 -- Boris Semenov, NAIF


References
--------------------------------------------------------

   1. ``SP-424 The Voyage of Mariner 9'', 
      http://history.nasa.gov/SP-424/sp424.htm


M9 NAIF Name-ID Mappings
--------------------------------------------------------

   This section contains name to NAIF ID mappings for the M9 mission.
   Once the contents of this file is loaded into the KERNEL POOL, these
   mappings become available within SPICE, making it possible to use
   names instead of ID code in the high level SPICE routine calls.

   The tables below summarize the mappings; the actual definitions are
   provided after the last summary table.

   Spacecraft and Spacecraft Bus
   -----------------------------

      MARINER-9		-9
      MARINER 9		-9
      MARINER_9		-9
      MARINER9			-9
      M9			-9

      M9_SC_BUS		-9000
      M9_SPACECRAFT_BUS	-9000
      M9_SPACECRAFT		-9000

      M9_INSTRUMENT_PLATFORM	-9000
      M9_PLATFORM		-9000

   Instruments and Sensors Mounted on Spacecraft Bus
   -------------------------------------------------

      M9_IR              -909

      M9_PLASMA_SEA      -9021
      M9_PLASMA_SES      -9022

      M9_CPT_MAIN        -9041
      M9_CPT_LOW_ENERGY  -9042

      M9_EUV_OCCULTATION -9051

      M9_SUN_SENSOR      -9060

      M9_STAR_TRACKER    -9070

   Scan Platform
   -------------

      M9_SCAN_PLATFORM   -990

   Instruments Mounted on Scan Platform
   ------------------------------------

      M9_VIDICON_A       -9110
      A       		  -9110
      M9_VIDICON_B       -9120
      B       		  -9120

      M9_EUV_AIRGLOW     -9152

   Magnetometer Boom and Sensors 
   ----------------------------- 

      M9_MAG_BOOM        -9200
      M9_MAG_INBOARD     -9211 
      M9_MAG_OUTBOARD    -9212

   Solar Arrays
   ------------

      M9_SA+X            -939 
      M9_SA-X            -9320 

   High Gain Antenna
   -----------------

      M9_HGA             -9400

   Low Gain Antenna
   ----------------

      M9_LGA             -9500


   Spacecraft Bus Frame
   -------------------------------------------------------------------------------
   
      \begindata
   
         FRAME_M9_SPACECRAFT     = -9000
         FRAME_-9000_NAME        = 'M9_SPACECRAFT'
         FRAME_-9000_CLASS       = 3
         FRAME_-9000_CLASS_ID    = -9000
         FRAME_-9000_CENTER      = -9
         CK_-9000_SCLK           = -9
         CK_-9000_SPK            = -9
   
      \begintext
   
   Metric Frames
   -------------------------------------------------------------------------------
   
      \begindata
   
         FRAME_VIDICON_A          = -9110
         FRAME_-9110_NAME        = 'VIDICON_A'
         FRAME_-9110_CLASS       = 3
         FRAME_-9110_CLASS_ID    = -9110
         FRAME_-9110_CENTER      = -9
         CK_-9110_SCLK           = -9
         CK_-9110_SPK            = -9
   
         FRAME_VIDICON_B          = -9120
         FRAME_-9120_NAME        = 'VIDICON_B'
         FRAME_-9120_CLASS       = 3
         FRAME_-9120_CLASS_ID    = -9120
         FRAME_-9120_CENTER      = -9
         CK_-9120_SCLK           = -9
         CK_-9120_SPK            = -9
   
      \begintext
  
   The keywords below implement M9 name-ID mappings summarized above.

   \begindata

      NAIF_BODY_NAME += ( 'MARINER-9' )
      NAIF_BODY_CODE += ( -9 )

      NAIF_BODY_NAME += ( 'MARINER 9' )
      NAIF_BODY_CODE += ( -9 )

      NAIF_BODY_NAME += ( 'MARINER9' )
      NAIF_BODY_CODE += ( -9 )

      NAIF_BODY_NAME += ( 'M-9' )
      NAIF_BODY_CODE += ( -9 )

      NAIF_BODY_NAME += ( 'M9' )
      NAIF_BODY_CODE += ( -9 )

      NAIF_BODY_NAME += ( 'M9_SC_BUS' )
      NAIF_BODY_CODE += ( -9000 )

      NAIF_BODY_NAME += ( 'M9_SPACECRAFT_BUS' )
      NAIF_BODY_CODE += ( -9000 )

      NAIF_BODY_NAME += ( 'M9_SPACECRAFT' )
      NAIF_BODY_CODE += ( -9000 )

      NAIF_BODY_NAME += ( 'M9_INSTRUMENT_PLATFORM' )
      NAIF_BODY_CODE += ( -9000 )

      NAIF_BODY_NAME += ( 'M9_PLATFORM' )
      NAIF_BODY_CODE += ( -9000 )

      NAIF_BODY_NAME += ( 'M9_IR' )
      NAIF_BODY_CODE += ( -909 )

      NAIF_BODY_NAME += ( 'M9_PLASMA_SEA' )
      NAIF_BODY_CODE += ( -9021 )

      NAIF_BODY_NAME += ( 'M9_PLASMA_SES' )
      NAIF_BODY_CODE += ( -9022 )

      NAIF_BODY_NAME += ( 'M9_CPT_MAIN' )
      NAIF_BODY_CODE += ( -9041 )

      NAIF_BODY_NAME += ( 'M9_CPT_LOW_ENERGY' )
      NAIF_BODY_CODE += ( -9042 )

      NAIF_BODY_NAME += ( 'M9_EUV_OCCULTATION' )
      NAIF_BODY_CODE += ( -9051 )

      NAIF_BODY_NAME += ( 'M9_SUN_SENSOR' )
      NAIF_BODY_CODE += ( -9060 )

      NAIF_BODY_NAME += ( 'M9_STAR_TRACKER' )
      NAIF_BODY_CODE += ( -9070 )

      NAIF_BODY_NAME += ( 'M9_SCAN_PLATFORM' )
      NAIF_BODY_CODE += ( -990 )

      NAIF_BODY_NAME += ( 'M9_VIDICON_A' )
      NAIF_BODY_CODE += ( -9110 )

      NAIF_BODY_NAME += ( 'A' )
      NAIF_BODY_CODE += ( -9110 )

      NAIF_BODY_NAME += ( 'M9_VIDICON_B' )
      NAIF_BODY_CODE += ( -9120 )

      NAIF_BODY_NAME += ( 'B' )
      NAIF_BODY_CODE += ( -9120 )

      NAIF_BODY_NAME += ( 'M9_EUV_AIRGLOW' )
      NAIF_BODY_CODE += ( -9152 )

      NAIF_BODY_NAME += ( 'M9_MAG_BOOM' )
      NAIF_BODY_CODE += ( -9200 )

      NAIF_BODY_NAME += ( 'M9_MAG_INBOARD' )
      NAIF_BODY_CODE += ( -9211 )

      NAIF_BODY_NAME += ( 'M9_MAG_OUTBOARD' )
      NAIF_BODY_CODE += ( -9212 )

      NAIF_BODY_NAME += ( 'M9_SA+X' )
      NAIF_BODY_CODE += ( -939 )

      NAIF_BODY_NAME += ( 'M9_SA-X' )
      NAIF_BODY_CODE += ( -9320 )

      NAIF_BODY_NAME += ( 'M9_HGA' )
      NAIF_BODY_CODE += ( -9400 )

      NAIF_BODY_NAME += ( 'M9_LGA' )
      NAIF_BODY_CODE += ( -9500 )

   \begintext
