// ============================================================================
// mxb_api.h  —  MX Bikes (PiBoSo) plugin API contract
//
// Declarations of the data structures and exported callbacks used to exchange
// data with the MX Bikes simulator. This reproduces the published PiBoSo
// plugin API (the same contract documented on the official forum "Output
// Plugins" thread and used by every MXB plugin). Field order and types must
// match the engine's binary layout exactly — the game hands us raw `void*`
// pointers that we reinterpret as these structs.
//
// Only the subset this mod consumes is declared. Optional callbacks the game
// looks up by name (GetProcAddress) but that we do not export are simply
// absent — the game skips them.
// ============================================================================
#pragma once

extern "C" {

// ---------------------------------------------------------------------------
// Bike / event data
// ---------------------------------------------------------------------------

// Delivered to EventInit when a track + bike are loaded.
typedef struct
{
	char  m_szRiderName[100];
	char  m_szBikeID[100];
	char  m_szBikeName[100];
	int   m_iNumberOfGears;
	int   m_iMaxRPM;
	int   m_iLimiter;
	int   m_iShiftRPM;
	float m_fEngineOptTemperature;          /* degrees Celsius */
	float m_afEngineTemperatureAlarm[2];    /* degrees Celsius, lower/upper */
	float m_fMaxFuel;                       /* liters */
	float m_afSuspMaxTravel[2];             /* meters; 0 = front, 1 = rear */
	float m_fSteerLock;                     /* degrees */
	char  m_szCategory[100];
	char  m_szTrackID[100];
	char  m_szTrackName[100];
	float m_fTrackLength;                   /* centerline length, meters */
	int   m_iType;                          /* 1 = testing; 2 = race; 4 = straight rhythm */
	char  m_szServerName[64];
	int   m_iServerType;
	char  m_szGUID[100];
} SPluginsBikeEvent_t;

// Delivered to RunInit when the bike goes on track.
typedef struct
{
	int   m_iSession;
	int   m_iConditions;                    /* 0 = sunny; 1 = cloudy; 2 = rainy */
	float m_fAirTemperature;                /* degrees Celsius */
	char  m_szSetupFileName[100];
} SPluginsBikeSession_t;

// Delivered to RunTelemetry every physics tick (rate = Startup return value).
typedef struct
{
	int   m_iRPM;                           /* engine rpm */
	float m_fEngineTemperature;             /* degrees Celsius */
	float m_fWaterTemperature;              /* degrees Celsius */
	int   m_iGear;                          /* 0 = Neutral */
	float m_fFuel;                          /* liters */
	float m_fSpeedometer;                   /* meters/second */
	float m_fPosX, m_fPosY, m_fPosZ;        /* world position of a chassis reference point (not CG), meters */
	float m_fVelocityX, m_fVelocityY, m_fVelocityZ; /* velocity of CG in world coords, m/s */
	float m_fAccelerationX, m_fAccelerationY, m_fAccelerationZ; /* G, local to chassis, 10ms avg */
	float m_aafRot[3][3];                   /* chassis rotation matrix (incl. lean and wheeling) */
	float m_fYaw, m_fPitch, m_fRoll;        /* degrees, -180 to 180 */
	float m_fYawVelocity, m_fPitchVelocity, m_fRollVelocity; /* degrees / second */
	float m_afSuspLength[2];                /* meters; 0 = front, 1 = rear */
	float m_afSuspVelocity[2];              /* m/s; 0 = front, 1 = rear */
	int   m_iCrashed;                       /* 1 = rider detached from bike */
	float m_fSteer;                         /* degrees; negative = right */
	float m_fThrottle;                      /* 0..1 */
	float m_fFrontBrake;                    /* 0..1 */
	float m_fRearBrake;                     /* 0..1 */
	float m_fClutch;                        /* 0..1; 0 = fully engaged */
	float m_afWheelSpeed[2];                /* m/s; 0 = front, 1 = rear */
	int   m_aiWheelMaterial[2];             /* material index; 0 = not in contact */
	float m_afBrakePressure[2];             /* kPa */
	float m_fSteerTorque;                   /* Nm */
} SPluginsBikeData_t;

typedef struct
{
	int m_iLapNum;
	int m_iInvalid;
	int m_iLapTime;                         /* milliseconds */
	int m_iBest;                            /* 1 = best lap */
} SPluginsBikeLap_t;

typedef struct
{
	int m_iSplit;
	int m_iSplitTime;                       /* milliseconds */
	int m_iBestDiff;                        /* milliseconds, vs best lap */
} SPluginsBikeSplit_t;

// ---------------------------------------------------------------------------
// Drawing primitives (returned from Draw)
// ---------------------------------------------------------------------------

// A textured or solid quad. Corners in normalized screen space:
// (0,0) = top-left, (1,1) = bottom-right (16:9 reference; x may exceed [0,1]
// on ultrawide, y stays 0..1). Listed counter-clockwise.
typedef struct
{
	float         m_aafPos[4][2];
	int           m_iSprite;                /* 1-based index into the registered sprite list; 0 = solid fill with m_ulColor */
	unsigned long m_ulColor;                /* ABGR */
} SPluginQuad_t;

// A text string drawn with a registered bitmap font.
typedef struct
{
	char          m_szString[100];          /* CP1252, NUL-terminated */
	float         m_afPos[2];               /* normalized screen position */
	int           m_iFont;                  /* 1-based index into the registered font list */
	float         m_fSize;
	int           m_iJustify;               /* 0 = left; 1 = center; 2 = right */
	unsigned long m_ulColor;                /* ABGR */
} SPluginString_t;

// ---------------------------------------------------------------------------
// Track centerline (delivered to TrackCenterline)
// ---------------------------------------------------------------------------

typedef struct
{
	int   m_iType;                          /* 0 = straight; 1 = curve */
	float m_fLength;                        /* meters */
	float m_fRadius;                        /* curve radius, meters; < 0 = left curve; 0 = straight */
	float m_fAngle;                         /* start angle in degrees; 0 = north */
	float m_afStart[2];                     /* start position, meters */
	float m_fHeight;                        /* start height, meters */
} SPluginsTrackSegment_t;

// ---------------------------------------------------------------------------
// Exported callbacks (the engine resolves these by name; all but the identity
// trio are optional). Signatures must match exactly.
// ---------------------------------------------------------------------------

__declspec(dllexport) char* GetModID();
__declspec(dllexport) int   GetModDataVersion();
__declspec(dllexport) int   GetInterfaceVersion();

__declspec(dllexport) int   Startup(char* _szSavePath);   /* returns telemetry Hz; -1 unloads */
__declspec(dllexport) void  Shutdown();

__declspec(dllexport) void  EventInit(void* _pData, int _iDataSize);
__declspec(dllexport) void  EventDeinit();
__declspec(dllexport) void  RunInit(void* _pData, int _iDataSize);
__declspec(dllexport) void  RunDeinit();
__declspec(dllexport) void  RunStart();
__declspec(dllexport) void  RunStop();
__declspec(dllexport) void  RunLap(void* _pData, int _iDataSize);
__declspec(dllexport) void  RunSplit(void* _pData, int _iDataSize);

/* _fTime = on-track time (s); _fPos = position on centerline, 0..1 */
__declspec(dllexport) void  RunTelemetry(void* _pData, int _iDataSize, float _fTime, float _fPos);

/* Register sprite/font files (paths relative to the plugins folder), as
   counts + buffers of NUL-separated filenames. */
__declspec(dllexport) int   DrawInit(int* _piNumSprites, char** _pszSpriteName, int* _piNumFonts, char** _pszFontName);

/* _iState: 0 = on track; 1 = spectate; 2 = replay. Fill the quad/string arrays. */
__declspec(dllexport) void  Draw(int _iState, int* _piNumQuads, void** _ppQuad, int* _piNumString, void** _ppString);

__declspec(dllexport) void  TrackCenterline(int _iNumSegments, SPluginsTrackSegment_t* _pasSegment, void* _pRaceData);

} // extern "C"
