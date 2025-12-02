#ifndef _PSEMU_PLUGIN_DEFS_H
#define _PSEMU_PLUGIN_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

// this can't be __stdcall like it was in PSEmu API as too many functions are mixed up
#undef CALLBACK
#define CALLBACK

// header version
#define _PPDK_HEADER_VERSION		3

#define PLUGIN_VERSION				1

// plugin type returned by PSEgetLibType (types can be merged if plugin is multi type!)
#define PSE_LT_CDR					1
#define PSE_LT_GPU					2
#define PSE_LT_SPU					4
#define PSE_LT_PAD					8
#define PSE_LT_NET					16

// DLL function return codes
#define PSE_ERR_SUCCESS				0	// every function in DLL if completed sucessfully should return this value
#define PSE_ERR_FATAL				-1	// undefined error but fatal one, that kills all functionality

// XXX_Init return values
// Those return values apply to all libraries
// currently obsolete - preserved for compatibilty

#define PSE_INIT_ERR_SUCCESS		0	// initialization went OK
#define PSE_INIT_ERR_NOTCONFIGURED	-2	// this driver is not configured
#define PSE_INIT_ERR_NOHARDWARE		-3	// this driver can not operate properly on this hardware or hardware is not detected

/*         GPU PlugIn          */

//  GPU_Test return values

// sucess, everything configured, and went OK.
#define PSE_GPU_ERR_SUCCESS			0

// ERRORS
// this error might be returned as critical error but none of below
#define PSE_GPU_ERR					-20


// this driver is not configured
#define PSE_GPU_ERR_NOTCONFIGURED	PSE_GPU_ERR - 1
// this driver failed Init
#define PSE_GPU_ERR_INIT			PSE_GPU_ERR - 2

// WARNINGS
// this warning might be returned as undefined warning but allowing driver to continue
#define PSE_GPU_WARN				20

//  GPU_Query		- will be implemented soon

typedef struct
{
	unsigned int flags;
	unsigned int status;
 	void*	window;
	unsigned char reserved[100];
} gpuQueryS;

// gpuQueryS.flags
// if driver can operate in both modes it must support GPU_changeMode();
#define PSE_GPU_FLAGS_FULLSCREEN		1	// this driver can operate in fullscreen mode
#define PSE_GPU_FLAGS_WINDOWED			2	// this driver can operate in windowed mode

// gpuQueryS.status
#define PSE_GPU_STATUS_WINDOWWRONG	1	// this driver cannot operate in this windowed mode

//  GPU_Query	End	- will be implemented in v2


/*         CDR PlugIn          */

//	CDR_Test return values

// sucess, everything configured, and went OK.
#define PSE_CDR_ERR_SUCCESS			0

// general failure (error undefined)
#define PSE_CDR_ERR_FAILURE			-1

// ERRORS
#define PSE_CDR_ERR -40
// this driver is not configured
#define PSE_CDR_ERR_NOTCONFIGURED	PSE_CDR_ERR - 0
// if this driver is unable to read data from medium
#define PSE_CDR_ERR_NOREAD			PSE_CDR_ERR - 1

// WARNINGS
#define PSE_CDR_WARN 40
// if this driver emulates lame mode ie. can read only 2048 tracks and sector header is emulated
// this might happen to CDROMS that do not support RAW mode reading - surelly it will kill many games
#define PSE_CDR_WARN_LAMECD			PSE_CDR_WARN + 0




/*         SPU PlugIn          */

// some info retricted (now!)

// sucess, everything configured, and went OK.
#define PSE_SPU_ERR_SUCCESS 0

// ERRORS
// this error might be returned as critical error but none of below
#define PSE_SPU_ERR					-60

// this driver is not configured
#define PSE_SPU_ERR_NOTCONFIGURED	PSE_SPU_ERR - 1
// this driver failed Init
#define PSE_SPU_ERR_INIT			PSE_SPU_ERR - 2


// WARNINGS
// this warning might be returned as undefined warning but allowing driver to continue
#define PSE_SPU_WARN				60




/*         PAD PlugIn          */

/*

  functions that must be exported from PAD Plugin

  long	PADinit(long flags);	// called only once when PSEmu Starts
  void	PADshutdown(void);		// called when PSEmu exits
  long	PADopen(PadInitS *);	// called when PSEmu is running program
  long	PADclose(void);
  long	PADconfigure(void);
  void  PADabout(void);
  long  PADtest(void);			// called from Configure Dialog and after PADopen();
  long	PADquery(void);

  unsigned char PADstartPoll(int);
  unsigned char PADpoll(unsigned char, int *);

*/

// PADquery responses (notice - values ORed)
// PSEmu will use them also in PADinit to tell Plugin which Ports will use
// notice that PSEmu will call PADinit and PADopen only once when they are from
// same plugin

// might be used in port 1
#define PSE_PAD_USE_PORT1			1
// might be used in port 2
#define PSE_PAD_USE_PORT2			2



// No controller
#define PSE_PAD_TYPE_NONE			0
// MOUSE SCPH-1030
#define PSE_PAD_TYPE_MOUSE			1
// NEGCON - 16 button analog controller SLPH-00001
#define PSE_PAD_TYPE_NEGCON			2
// GUN CONTROLLER - gun controller SLPH-00014 from Konami
#define PSE_PAD_TYPE_GUN			3
// STANDARD PAD SCPH-1080, SCPH-1150
#define PSE_PAD_TYPE_STANDARD		4
// ANALOG JOYSTICK SCPH-1110
#define PSE_PAD_TYPE_ANALOGJOY		5
// GUNCON - gun controller SLPH-00034 from Namco
#define PSE_PAD_TYPE_GUNCON			6
// ANALOG CONTROLLER SCPH-1150
#define PSE_PAD_TYPE_ANALOGPAD		7


// sucess, everything configured, and went OK.
#define PSE_PAD_ERR_SUCCESS			0
// general plugin failure (undefined error)
#define PSE_PAD_ERR_FAILURE			-1


// ERRORS
// this error might be returned as critical error but none of below
#define PSE_PAD_ERR					-80
// this driver is not configured
#define PSE_PAD_ERR_NOTCONFIGURED	PSE_PAD_ERR - 1
// this driver failed Init
#define PSE_PAD_ERR_INIT			PSE_PAD_ERR - 2


// WARNINGS
// this warning might be returned as undefined warning but allowing driver to continue
#define PSE_PAD_WARN				80


typedef struct PadDataS
{
	// controller type - fill it withe predefined values above
	unsigned char controllerType;

	unsigned char padding;
	unsigned short saveSize;

	int portMultitap;
	int requestPadIndex;

	// status of buttons - every controller fills this field
	unsigned short buttonStatus;

	// for analog pad fill those next 4 bytes
	// values are analog in range 0-255 where 127 is center position
	unsigned char rightJoyX, rightJoyY, leftJoyX, leftJoyY;

	// for mouse fill those next 2 bytes
	// values are in range -128 - 127
	unsigned char moveX, moveY;

	// Lightgun values
	int absoluteX, absoluteY;

	unsigned char Vib[2];
	unsigned char VibF[2];
	
	struct {
		unsigned char configMode;
		unsigned char padMode; // 0 : digital 1: analog
		unsigned char cmd4dConfig[6];
		unsigned int  lastUseFrame;
		unsigned int  unused;
		unsigned char configModeUsed;
		unsigned char autoAnalogTried;
		unsigned char userToggled;
		unsigned char padding;
	} ds;
	unsigned char multitapLongModeEnabled;
	unsigned char padding2;
	unsigned char txData[34];

	unsigned char reserved[22];
} PadDataS;

/*         NET PlugIn v2       */
/* Added by linuzappz@pcsx.net */

/* Modes bits for NETsendData/NETrecvData */
#define PSE_NET_BLOCKING	0x00000000
#define PSE_NET_NONBLOCKING	0x00000001

/* note: unsupported fields should be zeroed.
 
typedef struct {
	char EmuName[32];
	char CdromID[9];	// ie. 'SCPH12345', no \0 trailing character
	char CdromLabel[11];
	void *psxMem;
	GPUshowScreenPic GPU_showScreenPic;
	GPUdisplayText GPU_displayText;
	PADsetSensitive PAD_setSensitive;
	char GPUpath[256];
	char SPUpath[256];
	char CDRpath[256];
	char MCD1path[256];
	char MCD2path[256];
	char BIOSpath[256];	// 'HLE' for internal bios
	char Unused[1024];
} netInfo;

*/

/*
  basic funcs:

   long NETopen(HWND hWnd)
    opens the connection.
    shall return 0 on success, else -1.
    -1 is also returned if the user selects offline mode.

   long NETclose()
    closes the connection.
    shall return 0 on success, else -1.

   void NETpause()
    this is called when the user paused the emulator.

   void NETresume()
    this is called when the user resumed the emulator.

   long NETqueryPlayer()
    returns player number

   long NETsendPadData(void *pData, int Size)
    this should be called for the first pad only on each side.

   long NETrecvPadData(void *pData, int Pad)
    call this for Pad 1/2 to get the data sent by the above func.

  extended funcs:

   long NETsendData(void *pData, int Size, int Mode)
    sends Size bytes from pData to the other side.

   long NETrecvData(void *pData, int Size, int Mode)
    receives Size bytes from pData to the other side.

   void NETsetInfo(netInfo *info);
    sets the netInfo struct.

   void NETkeypressed(int key) (linux only)
    key is a XK_?? (X11) keycode.
*/

#ifdef __cplusplus
}
#endif
#endif // _PSEMU_PLUGIN_DEFS_H
