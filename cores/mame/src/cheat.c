/***************************************************************************** 
 *
 *	cheat.c
 *	by Ian Patterson [ianpatt at pacbell dot net]
 *
 *	The cheat engine for MAME. Allows you to search for locations in memory
 *	where gameplay-related values are stored, and change them. In other words,
 *	it lets you cheat.
 *
 *	TODO:
 *		- conflict checking
 *		- look in to adding auto-fire
 *		- bounds checks for relative address cheats
 *
 *	Known Issues:
 *		- signed fields displayed in hex don't accept negative values from
 *		  direct keyboard input
 *
 *****************************************************************************/

/******	Cheat File Specification **********************************************

Type Field:

MSB						 	    LSB
33222222 22221111 11111100 00000000
10987654 32109876 54321098 76543210

									[ type ]
-------- -------- -------- -------x		one-shot cheat
-------- -------- -------- -----xx-		type
											00 =	normal/delay
											01 =	wait for modification
											10 =	ignore if decrementing
											11 =	watch
-------- -------- -------- ---xx---		operation (combined	with operation
										extend bit)
											[extend	= 0]
												00 =	write with mask
												01 =	add/subtract
												10 =	force range
												11 =	set/clear bits (for
														relative address mode)
											[extend	= 1]
												00 =	unused
												01 =	unused
												10 =	unused
												11 =	nothing
-------- -------- -------- xxx-----		parameter
											type ==	00	delay in seconds
														between	operations
											type ==	01	delay after
														modification before
														operation in seconds
											type ==	10	decrement ignore value
											type ==	11	watch options
															display format
												-00 =	hex
												-01 =	decimal
												-10 =	binary
												-11 =	ascii
															show label
												0-- =	no
												1-- =	yes, copy from comment
									[ user-selected	value ]
-------- -------- -------x --------		enable
-------- -------- ------x- --------		displayed value
											0 =	value
											1 =	value + 1
-------- -------- -----x-- --------		minimum	value
											0 =	0
											1 =	1
-------- -------- ----x--- --------		BCD
									[ prefill ]
-------- -------- --xx---- --------		value/enable
											00 =	disable
											01 =	prefill	with 0xFF
											10 =	prefill	with 0x00
											11 =	prefill	with 0x01
									[ link / options ]
-------- -------- -x------ --------		don't add to list (used for commands)
-------- -------- x------- --------		add as extend for previous cheat
-------- -------x -------- --------		enable
-------- ------x- -------- --------		copy previous value
-------- -----x-- -------- --------		operation parameter
											operation == 001	add/subtract
												0 =	add
												1 =	subtract
											operation == 011	set/clear
												0 = set
												1 = clear
-------- ----x--- -------- --------		operation extend bit
-------- --xx---- -------- --------		bytes used
											00 =	1
											01 =	2
											10 =	3
											11 =	4
-------- -x------ -------- --------		endianness
											locations associated with a
											processor
												0 =		same endianness	as
														target processor
												1 =		different endianness
											generic	locations
												0 =		big	endian
												1 =		little endian
-------- x------- -------- --------		restore previous value on disable
									[ location / effective address ]
---xxxxx -------- -------- --------		parameter
											type ==	000	CPU	index
											type ==	001	region offset
														(REGION_xxx)
											type ==	010	CPU	index
											type ==	011	custom cheat type
												00000		comment
												00001		EEPROM
												00010		select
												00011		assign activation key
												00100		enable
												00101		overclock
												...			others?
											type ==	100	address	size, CPU
												---00		8 bit
												---01		16 bit
												---10		24 bit
												---11		32 bit
												xxx--		cpu
xxx----- -------- -------- --------		type
											000	=	standard memory	write
											001	=	memory region
											010	=	write handler mapped memory
											011	=	custom
											100	=	relative address (CPU)
											101	=	cheatscript
											110	=	unused
											111	=	unused

Conversion Table:

MSB								LSB
33222222 22221111 11111100 0000	0000
10987654 32109876 54321098 7654	3210
000xxxxx 00000000 00000000 0000	0000	000
000xxxxx 00000000 00000000 0000	0001	001
000xxxxx 00000000 00000000 0010	0000	002
000xxxxx 00000000 00000000 0100	0000	003
000xxxxx 00000000 00000000 1010	0000	004
000xxxxx 00000000 00000000 0010	0010	005
000xxxxx 00000000 00000000 0100	0010	006
000xxxxx 00000000 00000000 1010	0010	007
000xxxxx 00000000 00000000 0010	0100	008
000xxxxx 00000000 00000000 0100	0100	009
000xxxxx 00000000 00000000 0110	0100	010
000xxxxx 00000000 00000000 1000	0100	011
000xxxxx 00000000 00000000 0010	0011	015
000xxxxx 00000000 00000000 0100	0011	016
000xxxxx 00000000 00000000 1010	0011	017
000xxxxx 00000000 00000000 0000	0000	020	(mask used)
000xxxxx 00000000 00000000 0000	0001	021	(mask used)
000xxxxx 00000000 00000000 0010	0000	022	(mask used)
000xxxxx 00000000 00000000 0100	0000	023	(mask used)
000xxxxx 00000000 00000000 1010	0000	024	(mask used)
000xxxxx 00000000 00000000 0000	0000	040	(mask used)
000xxxxx 00000000 00000000 0000	0001	041	(mask used)
000xxxxx 00000000 00000000 0010	0000	042	(mask used)
000xxxxx 00000000 00000000 0100	0000	043	(mask used)
000xxxxx 00000000 00000000 1010	0000	044	(mask used)
000xxxxx 00000000 00000001 0000	0011	060
000xxxxx 00000000 00000011 0000	0011	061
000xxxxx 00000000 00000101 0000	0011	062
000xxxxx 00000000 00001001 0000	0011	063
000xxxxx 00000000 00001011 0000	0011	064
000xxxxx 00000000 00001101 0000	0011	065
000xxxxx 00000000 00000001 0000	0001	070
000xxxxx 00000000 00000011 0000	0001	071
000xxxxx 00000000 00000101 0000	0001	072
000xxxxx 00000000 00001001 0000	0001	073
000xxxxx 00000000 00001011 0000	0001	074
000xxxxx 00000000 00001101 0000	0001	075
000xxxxx 00000000 00000000 0000	0011	080
000xxxxx 00000000 00000010 0000	0011	081
000xxxxx 00000000 00000100 0000	0011	082
000xxxxx 00000000 00001000 0000	0011	083
000xxxxx 00000000 00001010 0000	0011	084
000xxxxx 00000000 00001100 0000	0011	085
000xxxxx 00000000 00000000 0000	0001	090
000xxxxx 00000000 00000010 0000	0001	091
000xxxxx 00000000 00000100 0000	0001	092
000xxxxx 00000000 00001000 0000	0001	093
000xxxxx 00000000 00001010 0000	0001	094
000xxxxx 00000000 00001100 0000	0001	095
001xxxxx 10000000 00000000 0000	0000	100
001xxxxx 00000000 00000000 0000	0001	101
001xxxxx 10000000 00000000 0000	0000	102
001xxxxx 00000000 00000000 0000	0001	103
010xxxxx 10000000 00000000 0000	0000	110
010xxxxx 00000000 00000000 0000	0001	111
010xxxxx 10000000 00000000 0000	0000	112
010xxxxx 00000000 00000000 0000	0001	113
01100011 00000000 00000000 0000 0001	120
01100011 00000000 00000000 0000 0001	121 (mask used)
01100011 00000000 00000000 0000 0001	122 (mask used)
00000000 00000001 00000000 0000	0000	5xx
000xxxxx 00000000 00000000 0000	0110	998
01100000 00000000 00000000 0000	0000	999

Cheat Format:

:[ drivername ]:[ type ]:[ address ]:[ data ]:[ extended data ]:[ name ]:[ description ]


drivername		string	maximum	8 chars
type			hex		32 bits
address			hex		32 bits
data			hex		32 bits
extended data	hex		32 bits
name			string	maximum	255	chars
description		string	maximum	255	chars

Extended Data Field:

[ force	range ]

0xAABB

AA = minimum value accepted
BB = maximum value accepted

[ add/subtract ]

The	field will store either	the	minimum	or maximum boundary	for	modification,
depending on the operation parameter.

[ write	with mask ]

The	field will store a mask	containing which bits are modified by the
operation. For normal operation, set the mask to 0xFFFFFFFF.
Example	code: data = (data & ~mask)	| (input & mask);

Copy Previous Value:

If this	field is true, the value for this cheat	is determined by taking	the
value read from	the	previous cheat and adding the value	stored in the data
field.

Relative Address:

The	extend data	field will store the the signed	offset to be applied to	the
address	read. Because of this, any operation using the extend data field may
have interesting results. Use the special set/clear bits operations instead of
a masked write.

Select Cheat Type: (01100010 -------0 -------- --------) 0x62000000

May	be used	only as	the	first cheat	of a linked	cheat. In the "Enable/Disable
Cheat" menu, instead of	simple listing On/Off or Set as	the	menu option, the
engine will	list the name fields of	each of	the	subcheats. If the current
selected subcheat is a one-shot	cheat, pressing	Enter will activate	the
currently subcheat.	If the subcheat	is an on/off cheat,	the	currently selected
subcheat (and only that	subcheat) will be activated.

Assign Activation Key: (01100011 -------- -1------ --------) 0x63004000

Assigns an activation key to a cheat. Put the index of the cheat you want to
modify in the address field, then put the key index in the data field.

Example: to set the second cheat in the cheat list to activate when "Q" is
pressed, add this cheat to the file.

:gamename:63004000:00000001:00000010:00000000:

Key Index List:

	A		00	Q		10	6		20	F3		30	[		40	PGDN	50	RCTRL	60
	B		01	R		11	7		21	F4		31	]		41	LEFT	51	LALT	61
	C		02	S		12	8		22	F5		32	ENTER	42	RIGHT	52	RALT	62
	D		03	T		13	9		23	F6		33	:		43	UP		53	SCRLOCK	63
	E		04	U		14	[0]		24	F7		34	'		44	DOWN	54	NUMLOCK	64
	F		05	V		15	[1]		25	F8		35	\		45	[/]		55	CAPSLCK	65
	G		06	W		16	[2]		26	F9		36	\		46	[*]		56	LWIN	66
	H		07	X		17	[3]		27	F10		37	,		47	[-]		57	RWIN	67
	I		08	Y		18	[4]		28	F11		38	.		48	[+]		58	MENU	68
	J		09	Z		19	[5]		29	F12		39	/		49	[DEL]	59
	K		0A	0		1A	[6]		2A	ESC		3A	SPACE	4A	[ENTER]	5A
	L		0B	1		1B	[7]		2B	~		3B	INS		4B	PRTSCR	5B
	M		0C	2		1C	[8]		2C	-		3C	DEL		4C	PAUSE	5C
	N		0D	3		1D	[9]		2D	=		3D	HOME	4D	LSHIFT	5D
	O		0E	4		1E	F1		2E	BACKSP	3E	END		4E	RSHIFT	5E
	P		0F	5		1F	F2		2F	TAB		3F	PGUP	4F	LCTRL	5F

Pre-Enable: (01100100 -------- -1------ --------) 0x64004000

Enables a cheat on startup. Put the index of the cheat you want to enable in
the address field.

Example: to activate the eleventh cheat in the cheat list, add this cheat to
the file:

:gamename:64004000:0000000A:00000000:00000000:

Overclock: (01100101 -------- -1------ --------) 0x65004000

Overclocks a CPU. Put the CPU index you want in the address field, and the
overclocking amount in the data field. Use 16.16 fixed point notation for
the overclocking amount.

Example 1: overclocking CPU #0 by 200%

:gamename:65004000:00000000:00020000:00000000:

Example 2: overclocking CPU #3 by 125%

:gamename:65004000:00000003:00014000:00000000:

To convert a percent to 16.16 fixed point notation, take the percentage as a
decimal value (eg. 65% = .65) and multiply it by 65536. Then, convert the value
to hex.

Cheat Engine Commands:

These special cheat lines are used to set global preferences for the cheat engine. They follow
this format:

:_command:[ data ]

The lower byte of the data field stores the command, and the remaining bytes store data
for the command. Here is a list of the commands:

0x00	disable help boxes (once I add them)
0x01	use old-style cheat search box (now redundant)
0x02	use new-style cheat search box
0x03	don't print labels in new-style search menu
0x04	auto-save cheats on exit

So, if you wanted to use the old-style cheat box, you would add this line to your cheat.dat:

:_command:00000001

Watches:

You can specify options for watches using the data field. Specify fields like this:

MSB								LSB
33222222 22221111 11111100 00000000
10987654 32109876 54321098 76543210
-------- -------- -------- xxxxxxxx		number of elements - 1
-------- -------- xxxxxxxx --------		bytes to skip after each element
-------- xxxxxxxx -------- --------		elements per line
											0 = all on one line
xxxxxxxx -------- -------- --------		signed value to add

So, to make a watch on CPU1 address 0064407F with six elements, skipping three bytes after each element,
showing two elements per line, you would do this:

:gamename:00000006:0064407F:00020305:00000000:

The extend data field is used to position the watch display.

MSB							  LSB
3322222222221111 1111110000000000
1098765432109876 5432109876543210
xxxxxxxxxxxxxxxx ----------------	x pixel offset
---------------- xxxxxxxxxxxxxxxx	y pixel offset

Notes:

- if you want to have a	list of	many on/off	subcheats, include a "None"	option,
or there will be no	way	to disable the cheat
- the engine will display "Press Enter to Activate Cheat" if a one-shot	cheat
is selected

*******************************************************************************/

#include "driver.h"
#include "ui_text.h"
#include "artwork.h"
#include "machine/eeprom.h"
#include <ctype.h>
#include <streams/interface_stream.h>


#define CHEAT_DATABASE_RZIP_FILENAME  "cheat.rzip"
#define CHEAT_DATABASE_FILENAME       "cheat.dat"
#define CHEAT_SAVE_FILENAME           "save_cheat.dat"

#define OSD_READKEY_KLUDGE	1

#define NAME_MAX_LENGTH		255
#define DESC_MAX_LENGTH		255
#define STR_(X) #X
#define STR(X) STR_(X)

/**** Macros *****************************************************************/

/*	easy bitfield extraction and setting */
/*	uses *_Shift, *_ShiftedMask, and *_Mask enums */
#define EXTRACT_FIELD(data, name)				(((data) >> k##name##_Shift) & k##name##_ShiftedMask)
#define SET_FIELD(data, name, in)				(data = (data & ~(k##name##_ShiftedMask << k##name##_Shift)) | (((in) & k##name##_ShiftedMask) << k##name##_Shift))
#define TEST_FIELD(data, name)					((data) & k##name##_Mask)
#define SET_MASK_FIELD(data, name)				((data) |= k##name##_Mask)
#define CLEAR_MASK_FIELD(data, name)			((data) &= ~(k##name##_Mask))
#define TOGGLE_MASK_FIELD(data, name)			((data) ^= k##name##_Mask)

#define DEFINE_BITFIELD_ENUM(name, end, start)	k##name##_Shift = (int)(end), 											\
												k##name##_ShiftedMask = (int)(0xFFFFFFFF >> (32 - (start - end + 1))),	\
												k##name##_Mask = (int)(k##name##_ShiftedMask << k##name##_Shift)

#define kRegionListLength						(REGION_MAX - REGION_INVALID)

/**** Enums ******************************************************************/

enum
{
	DEFINE_BITFIELD_ENUM(OneShot,					0,	0),
	DEFINE_BITFIELD_ENUM(Type,						1,	2),
	DEFINE_BITFIELD_ENUM(Operation,					3,	4),
	DEFINE_BITFIELD_ENUM(TypeParameter,				5,	7),
	DEFINE_BITFIELD_ENUM(UserSelectEnable,			8,	8),
	DEFINE_BITFIELD_ENUM(UserSelectMinimumDisplay,	9,	9),
	DEFINE_BITFIELD_ENUM(UserSelectMinimum,			10,	10),
	DEFINE_BITFIELD_ENUM(UserSelectBCD,				11,	11),
	DEFINE_BITFIELD_ENUM(Prefill,					12,	13),
	DEFINE_BITFIELD_ENUM(RemoveFromList,			14, 14),
	DEFINE_BITFIELD_ENUM(LinkEnable,				16,	16),
	DEFINE_BITFIELD_ENUM(LinkCopyPreviousValue,		17,	17),
	DEFINE_BITFIELD_ENUM(OperationParameter,		18,	18),
	DEFINE_BITFIELD_ENUM(OperationExtend,			19,	19),
	DEFINE_BITFIELD_ENUM(BytesUsed,					20,	21),
	DEFINE_BITFIELD_ENUM(Endianness,				22,	22),
	DEFINE_BITFIELD_ENUM(RestorePreviousValue,		23, 23),
	DEFINE_BITFIELD_ENUM(LocationParameter,			24,	28),
	DEFINE_BITFIELD_ENUM(LocationType,				29,	31),

	DEFINE_BITFIELD_ENUM(Watch_AddValue,			0, 15),
	DEFINE_BITFIELD_ENUM(Watch_Label,				16, 17),
	DEFINE_BITFIELD_ENUM(Watch_DisplayType,			18, 19)
};

enum
{
	kType_NormalOrDelay = 0,
	kType_WaitForModification,
	kType_IgnoreIfDecrementing,
	kType_Watch
};

enum
{
	kOperation_WriteMask = 0,
	kOperation_AddSubtract,
	kOperation_ForceRange,
	kOperation_SetOrClearBits,
	/* kOperation_Unused4, */
	/* kOperation_Unused5, */
	/* kOperation_Unused6, */
	kOperation_None = 7
};

enum
{
	kPrefill_Disable = 0,
	kPrefill_UseFF,
	kPrefill_Use00,
	kPrefill_Use01
};

enum
{
	kLocation_Standard = 0,
	kLocation_MemoryRegion,
	kLocation_HandlerMemory,
	kLocation_Custom,
	kLocation_IndirectIndexed
};

enum
{
	kCustomLocation_Comment = 0,
	kCustomLocation_EEPROM,
	kCustomLocation_Select,
	kCustomLocation_AssignActivationKey,
	kCustomLocation_Enable,
	kCustomLocation_Overclock
};

enum
{
	/* set for wait for modification or ignore if decrementing cheats when */
	/* the targeted value has changed */
	/* cleared after the operation is performed */
	kActionFlag_WasModified =		1 << 0,

	/* set for one shot cheats after the operation is performed */
	kActionFlag_OperationDone =		1 << 1,

	/* set if the extendData field is being used by something other than a mask value */
	kActionFlag_IgnoreMask =		1 << 2,

	/* set if the lastValue field contains valid data and can be restored if needed */
	kActionFlag_LastValueGood =		1 << 3,

	/* set after value changes from prefill value */
	kActionFlag_PrefillDone =		1 << 4,

	/* set after prefill value written */
	kActionFlag_PrefillWritten =	1 << 5,

	kActionFlag_StateMask =			kActionFlag_OperationDone |
									kActionFlag_LastValueGood |
									kActionFlag_PrefillDone |
									kActionFlag_PrefillWritten,
	kActionFlag_InfoMask =			kActionFlag_WasModified |
									kActionFlag_IgnoreMask,
	kActionFlag_PersistentMask =	kActionFlag_LastValueGood
};

enum
{
	/* true when the cheat is active */
	kCheatFlag_Active =					1 << 0,

	/* true if the cheat is entirely one shot */
	kCheatFlag_OneShot =				1 << 1,

	/* true if the cheat is entirely null (ex. a comment) */
	kCheatFlag_Null =					1 << 2,

	/* true if the cheat contains a user-select element */
	kCheatFlag_UserSelect =				1 << 3,

	/* true if the cheat is a select cheat */
	kCheatFlag_Select =					1 << 4,

	/* true if the activation key is being pressed */
	kCheatFlag_ActivationKeyPressed =	1 << 5,

	/* true if the cheat has been assigned an activation key */
	kCheatFlag_HasActivationKey =		1 << 6,

	/* true if the cheat has been edited or is a new cheat */
	kCheatFlag_Dirty =					1 << 7,

	/* masks */
	kCheatFlag_StateMask =			kCheatFlag_Active,
	kCheatFlag_InfoMask =			kCheatFlag_OneShot |
									kCheatFlag_Null |
									kCheatFlag_UserSelect |
									kCheatFlag_Select |
									kCheatFlag_ActivationKeyPressed |
									kCheatFlag_HasActivationKey,
	kCheatFlag_PersistentMask =		kCheatFlag_Active |
									kCheatFlag_HasActivationKey |
									kCheatFlag_ActivationKeyPressed |
									kCheatFlag_Dirty
};

enum
{
	kWatchLabel_None = 0,
	kWatchLabel_Address,
	kWatchLabel_String,

	kWatchLabel_MaxPlusOne
};

enum
{
	kWatchDisplayType_Hex = 0,
	kWatchDisplayType_Decimal,
	kWatchDisplayType_Binary,
	kWatchDisplayType_ASCII,

	kWatchDisplayType_MaxPlusOne
};

enum
{
	kVerticalKeyRepeatRate =		8,
	kHorizontalFastKeyRepeatRate =	5,
	kHorizontalSlowKeyRepeatRate =	8
};

enum
{
	/* true if enabled for search */
	kRegionFlag_Enabled =		1 << 0,

	/* true if the memory region has no mapped memory */
	/* and uses a memory handler */
	kRegionFlag_UsesHandler =	1 << 1
};

enum
{
	kRegionType_CPU = 0,
	kRegionType_Memory
};

enum
{
	kSearchSpeed_Fast = 0,		/* RAM + some banks */
	kSearchSpeed_Medium,		/* RAM + BANKx */
	kSearchSpeed_Slow,			/* all memory areas except ROM, NOP, and custom handlers */
	kSearchSpeed_VerySlow,		/* all memory areas except ROM and NOP */
	kSearchSpeed_AllMemory,		/* entire CPU address space */

	kSearchSpeed_Max = kSearchSpeed_AllMemory
};

enum
{
	kSearchOperand_Current = 0,
	kSearchOperand_Previous,
	kSearchOperand_First,
	kSearchOperand_Value,

	kSearchOperand_Max = kSearchOperand_Value
};

enum
{
	kSearchSize_8Bit = 0,
	kSearchSize_16Bit,
	kSearchSize_32Bit,
	kSearchSize_1Bit,

	kSearchSize_Max = kSearchSize_1Bit
};

enum
{
	kSearchComparison_LessThan = 0,
	kSearchComparison_GreaterThan,
	kSearchComparison_EqualTo,
	kSearchComparison_LessThanOrEqualTo,
	kSearchComparison_GreaterThanOrEqualTo,
	kSearchComparison_NotEqual,
	kSearchComparison_IncreasedBy,
	kSearchComparison_NearTo,

	kSearchComparison_Max = kSearchComparison_NearTo
};

enum
{
	kEnergy_Equals = 0,
	kEnergy_Less,
	kEnergy_Greater,
	kEnergy_LessOrEquals,
	kEnergy_GreaterOrEquals,
	kEnergy_NotEquals,

	kEnergy_Max = kEnergy_NotEquals
};

/**** Structs ****************************************************************/

struct CheatAction
{
	UINT32	type;
	UINT32	address;
	UINT32	data;
	UINT32	extendData;
	UINT32	originalDataField;

	INT32	frameTimer;
	UINT32	lastValue;

	UINT32	flags;

	UINT8	** cachedPointer;
	UINT32	cachedOffset;

	char	* optionalName;
};

typedef struct CheatAction	CheatAction;

struct CheatEntry
{
	char			* name;
	char			* comment;

	INT32			actionListLength;
	CheatAction		* actionList;

	int				activationKey;

	UINT32			flags;
	int				selection;
};

typedef struct CheatEntry	CheatEntry;

struct WatchInfo
{
	UINT32			address;
	UINT8			cpu;
	UINT8			numElements;
	UINT8			elementBytes;
	UINT8			labelType;
	UINT8			displayType;
	UINT8			skip;
	UINT8			elementsPerLine;
	INT8			addValue;
	INT8			addressShift;
	INT8			dataShift;
	UINT32			xor;

	UINT16			x, y;

	CheatEntry *	linkedCheat;

	char			label[256];
};

typedef struct WatchInfo	WatchInfo;

struct SearchRegion
{
	UINT32	address;
	UINT32	length;

	UINT8	targetType;
	UINT8	targetIdx;

	UINT8	flags;

	UINT8	* cachedPointer;
	const struct Memory_WriteAddress
			* writeHandler;

	UINT8	* first;
	UINT8	* last;

	UINT8	* status;

	UINT8	* backupLast;
	UINT8	* backupStatus;

	/* 12345678 - 12345678 BANK31 */
	char	name[32];

	UINT32	numResults;
	UINT32	oldNumResults;
};

typedef struct SearchRegion	SearchRegion;

struct OldSearchOptions
{
	UINT8	energy;
	UINT8	status;
	UINT8	slow;
	UINT32	value;
	UINT32	delta;
};

typedef struct OldSearchOptions	OldSearchOptions;

struct SearchInfo
{
	INT32				regionListLength;
	SearchRegion		* regionList;

	char				* name;

	INT8				bytes;	/* 0 = 1, 1 = 2, 2 = 4, 3 = bit */
	UINT8				swap;
	UINT8				sign;
	INT8				lhs;
	INT8				rhs;
	INT8				comparison;

	UINT8				targetType;	/* cpu/region */
	UINT8				targetIdx;	/* cpu or region index */

	UINT32				value;

	UINT8				searchSpeed;

	UINT32				numResults;
	UINT32				oldNumResults;

	INT32				currentRegionIdx;
	INT32				currentResultsPage;

	UINT8				backupValid;

	OldSearchOptions	oldOptions;
};

typedef struct SearchInfo	SearchInfo;

struct CPUInfo
{
	UINT8	type;
	UINT8	dataBits;
	UINT8	addressBits;
	UINT8	addressCharsNeeded;
	UINT32	addressMask;
	UINT8	endianness;
	UINT8	addressShift;
};

typedef struct CPUInfo	CPUInfo;

struct MenuStringList
{
	const char	** mainList;		/* editable menu item lists*/
	const char	** subList;
	char		* flagList;

	char	** mainStrings;		/* lists of usable strings*/
	char	** subStrings;

	char	* buf;				/* string storage*/

	UINT32	length;				/* number of menu items supported*/
	UINT32	numStrings;			/* number of strings supported*/
	UINT32	mainStringLength;	/* max length of main string*/
	UINT32	subStringLength;	/* max length of sub string*/
};

typedef struct MenuStringList	MenuStringList;

struct MenuItemInfoStruct
{
	UINT32	subcheat;
	UINT32	fieldType;
	UINT32	extraData;
};

typedef struct MenuItemInfoStruct	MenuItemInfoStruct;

/**** Local Globals **********************************************************/

static CheatEntry			* cheatList = NULL;
static INT32				cheatListLength = 0;

static WatchInfo			* watchList = NULL;
static INT32				watchListLength = 0;

static SearchInfo			* searchList = NULL;
static INT32				searchListLength = 0;
static INT32				currentSearchIdx = 0;

static CPUInfo				cpuInfoList[MAX_CPU];
static CPUInfo				regionInfoList[kRegionListLength];

static int					cheatEngineWasActive = 0;
static int					foundCheatDatabase = 0;
static int					cheatsDisabled = 0;
static int					watchesDisabled = 0;

static int					fullMenuPageHeight = 0;

static MenuStringList		menuStrings;

static MenuItemInfoStruct	* menuItemInfo;
static INT32				menuItemInfoLength = 0;

static int					useClassicSearchBox = 1;
static int					dontPrintNewLabels = 0;
static int					autoSaveEnabled = 0;

extern int					uirotcharwidth, uirotcharheight;

static const char *	kCheatNameTemplates[] =
{
	"Infinite Lives",
	"Infinite Lives PL1",
	"Infinite Lives PL2",
	"Infinite Time",
	"Infinite Time PL1",
	"Infinite Time PL2",
	"Invincibility",
	"Invincibility PL1",
	"Invincibility PL2",
	"Infinite Energy",
	"Infinite Energy PL1",
	"Infinite Energy PL2",
	"Select next level",
	"Select current level",
	"Infinite Ammo",
	"Infinite Ammo PL1",
	"Infinite Ammo PL2",
	"Infinite Bombs",
	"Infinite Bombs PL1",
	"Infinite Bombs PL2",
	"Infinite Smart Bombs",
	"Infinite Smart Bombs PL1",
	"Infinite Smart Bombs PL2",
	"Select Score PL1",
	"Select Score PL2",
	"Drain all Energy Now! PL1",
	"Drain all Energy Now! PL2",
	"Watch me for good answer",
	"Infinite",
	"Always have",
	"Get",
	"Lose",
	"Finish this",
	"---> <ENTER> To Edit <---",
	"\0"
};

static CPUInfo rawCPUInfo =
{
	0,			/* type*/
	8,			/* dataBits*/
	8,			/* addressBits*/
	1,			/* addressCharsNeeded*/
	CPU_IS_BE	/* endianness*/
};

static const int kSearchByteIncrementTable[] =
{
	1,
	2,
	4,
	1
};

static const char * kSearchByteNameTable[] =
{
	"1",
	"2",
	"4",
	"Bit"
};

static const int	kSearchByteDigitsTable[] =
{
	2,
	4,
	8,
	1
};

static const int	kSearchByteDecDigitsTable[] =
{
	3,
	5,
	10,
	1
};

static const UINT32 kSearchByteMaskTable[] =
{
	0x000000FF,
	0x0000FFFF,
	0xFFFFFFFF,
	0x00000001
};

static const UINT32	kSearchByteSignBitTable[] =
{
	0x00000080,
	0x00008000,
	0x80000000,
	0x00000000
};

static const UINT32 kSearchByteUnsignedMaskTable[] =
{
	0x0000007F,
	0x00007FFF,
	0x7FFFFFFF,
	0x00000001
};

static const UINT32	kCheatSizeMaskTable[] =
{
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF
};

static const UINT32	kCheatSizeDigitsTable[] =
{
	2,
	4,
	6,
	8
};

static const char * kOperandNameTable[] =
{
	"Current Data",
	"Previous Data",
	"First Data",
	"Value"
};

static const char * kComparisonNameTable[] =
{
	"Less",
	"Greater",
	"Equal",
	"Less Or Equal",
	"Greater Or Equal",
	"Not Equal",
	"Increased By Value",
	"Near To"
};

static const int	kByteConversionTable[] =
{
	kSearchSize_8Bit,
	kSearchSize_16Bit,
	kSearchSize_32Bit,
	kSearchSize_32Bit
};

static const int	kWatchSizeConversionTable[] =
{
	kSearchSize_8Bit,
	kSearchSize_16Bit,
	kSearchSize_32Bit,
	kSearchSize_8Bit
};

static const int	kSearchOperandNeedsInit[] =
{
	0,
	1,
	1,
	0
};

static const int kOldEnergyComparisonTable[] =
{
	kSearchComparison_EqualTo,
	kSearchComparison_LessThan,
	kSearchComparison_GreaterThan,
	kSearchComparison_LessThanOrEqualTo,
	kSearchComparison_GreaterThanOrEqualTo,
	kSearchComparison_NotEqual
};

static const int kOldStatusComparisonTable[] =
{
	kSearchComparison_EqualTo,
	kSearchComparison_NotEqual
};

static const UINT32 kPrefillValueTable[] =
{
	0x00,
	0xFF,
	0x00,
	0x01
};

const char *	kWatchLabelStringList[] =
{
	"None",
	"Address",
	"String"
};

const char *	kWatchDisplayTypeStringList[] =
{
	"Hex",
	"Decimal",
	"Binary",
	"ASCII"
};

/**** Function Prototypes ****************************************************/

static int		ShiftKeyPressed(void);
static int		ControlKeyPressed(void);
static int		AltKeyPressed(void);

static int		UIPressedRepeatThrottle(int code, int baseSpeed);
static int		ReadHexInput(void);

static char *	DoDynamicEditTextField(char * buf);
static void		DoStaticEditTextField(char * buf, int size);
static UINT32	DoEditHexField(UINT32 data);
static UINT32	DoEditHexFieldSigned(UINT32 data, UINT32 mask);
static INT32	DoEditDecField(INT32 data, INT32 min, INT32 max);

static UINT32	DoShift(UINT32 input, INT8 shift);
static UINT32	BCDToDecimal(UINT32 value);
static UINT32	DecimalToBCD(UINT32 value);

static void		RebuildStringTables(void);
static void		RequestStrings(UINT32 length, UINT32 numStrings, UINT32 mainStringLength, UINT32 subStringLength);
static void		InitStringTable(void);
static void		FreeStringTable(void);

static INT32	UserSelectValueMenu(struct mame_bitmap * bitmap, int selection, CheatEntry * entry);
static int		EnableDisableCheatMenu(struct mame_bitmap * bitmap, int selection, int firstTime);
static int		EditCheatMenu(struct mame_bitmap * bitmap, CheatEntry * entry, int selection);
static int		DoSearchMenuClassic(struct mame_bitmap * bitmap, int selection, int startNew);
static int		DoSearchMenu(struct mame_bitmap * bitmap, int selection, int startNew);
static int		AddEditCheatMenu(struct mame_bitmap * bitmap, int selection);
static int		ViewSearchResults(struct mame_bitmap * bitmap, int selection, int firstTime);
static int		ChooseWatch(struct mame_bitmap * bitmap, int selection);
static int		EditWatch(struct mame_bitmap * bitmap, WatchInfo * entry, int selection);
static INT32	DisplayHelp(struct mame_bitmap * bitmap, int selection);
static int		SelectOptions(struct mame_bitmap * bitmap, int selection);
static int		SelectSearchRegions(struct mame_bitmap * bitmap, int selection, SearchInfo * search);
static int		SelectSearch(struct mame_bitmap * bitmap, int selection);

static char *	CreateStringCopy(char * buf);

static void		ResizeCheatList(UINT32 newLength);
static void		ResizeCheatListNoDispose(UINT32 newLength);
static void		AddCheatBefore(UINT32 idx);
static void		DeleteCheatAt(UINT32 idx);
static void		DisposeCheat(CheatEntry * entry);
static CheatEntry *	GetNewCheat(void);

static void		ResizeCheatActionList(CheatEntry * entry, UINT32 newLength);
static void		ResizeCheatActionListNoDispose(CheatEntry * entry, UINT32 newLength);
static void		AddActionBefore(CheatEntry * entry, UINT32 idx);
static void		DeleteActionAt(CheatEntry * entry, UINT32 idx);
static void		DisposeAction(CheatAction * action);

static void		InitWatch(WatchInfo * info, UINT32 idx);
static void		ResizeWatchList(UINT32 newLength);
static void		ResizeWatchListNoDispose(UINT32 newLength);
static void		AddWatchBefore(UINT32 idx);
static void		DeleteWatchAt(UINT32 idx);
static void		DisposeWatch(WatchInfo * watch);
static WatchInfo *	GetUnusedWatch(void);
static void		AddCheatFromWatch(WatchInfo * watch);
static void		SetupCheatFromWatchAsWatch(CheatEntry * entry, WatchInfo * watch);

static void		ResizeSearchList(UINT32 newLength);
static void		ResizeSearchListNoDispose(UINT32 newLength);
static void		AddSearchBefore(UINT32 idx);
static void		DeleteSearchAt(UINT32 idx);
static void		InitSearch(SearchInfo * info);
static void		DisposeSearchRegions(SearchInfo * info);
static void		DisposeSearch(UINT32 idx);
static SearchInfo *	GetCurrentSearch(void);

static void		FillBufferFromRegion(SearchRegion * region, UINT8 * buf);
static UINT32	ReadRegionData(SearchRegion * region, UINT32 offset, UINT8 size, UINT8 swap);
static void		BackupSearch(SearchInfo * info);
static void		RestoreSearchBackup(SearchInfo * info);
static void		BackupRegion(SearchRegion * region);
static void		RestoreRegionBackup(SearchRegion * region);
static void		SetSearchRegionDefaultName(SearchRegion * region);
static void		AllocateSearchRegions(SearchInfo * info);
static void		BuildSearchRegions(SearchInfo * info);

static int		ConvertOldCode(int code, int cpu, int * data, int * extendData);
static int		MatchCommandCheatLine(char * buf);
static void		HandleLocalCommandCheat(UINT32 type, UINT32 address, UINT32 data, UINT32 extendData, char * name, char * description);

static void		LoadCheatDatabase(void);
static void		DisposeCheatDatabase(void);

static void		SaveCheat(CheatEntry * entry);
static void		DoAutoSaveCheats(void);
static void		AddCheatFromResult(SearchInfo * search, SearchRegion * region, UINT32 address);
static void		AddCheatFromFirstResult(SearchInfo * search);
static void		AddWatchFromResult(SearchInfo * search, SearchRegion * region, UINT32 address);

static UINT32	SearchSignExtend(SearchInfo * search, UINT32 value);
static UINT32	ReadSearchOperand(UINT8 type, SearchInfo * search, SearchRegion * region, UINT32 address);
static UINT32	ReadSearchOperandBit(UINT8 type, SearchInfo * search, SearchRegion * region, UINT32 address);
static UINT8	DoSearchComparison(SearchInfo * search, UINT32 lhs, UINT32 rhs);
static UINT32	DoSearchComparisonBit(SearchInfo * search, UINT32 lhs, UINT32 rhs);
/*static UINT8	IsRegionOffsetValid(SearchInfo * search, SearchRegion * region, UINT32 offset);*/

#define IsRegionOffsetValid	IsRegionOffsetValidBit

static UINT8	IsRegionOffsetValidBit(SearchInfo * search, SearchRegion * region, UINT32 offset);
static void		InvalidateRegionOffset(SearchInfo * search, SearchRegion * region, UINT32 offset);
static void		InvalidateRegionOffsetBit(SearchInfo * search, SearchRegion * region, UINT32 offset, UINT32 invalidate);
static void		InvalidateEntireRegion(SearchInfo * search, SearchRegion * region);

static void		InitializeNewSearch(SearchInfo * search);
static void		UpdateSearch(SearchInfo * search);

static void		DoSearch(SearchInfo * search);

static UINT8 **	LookupHandlerMemory(UINT8 cpu, UINT32 address, UINT32 * outRelativeAddress);

static UINT32	DoCPURead(UINT8 cpu, UINT32 address, UINT8 bytes, UINT8 swap);
static UINT32	DoMemoryRead(UINT8 * buf, UINT32 address, UINT8 bytes, UINT8 swap, CPUInfo * info);
static void		DoCPUWrite(UINT32 data, UINT8 cpu, UINT32 address, UINT8 bytes, UINT8 swap);
static void		DoMemoryWrite(UINT32 data, UINT8 * buf, UINT32 address, UINT8 bytes, UINT8 swap, CPUInfo * info);

static UINT8	CPUNeedsSwap(UINT8 cpu);
static UINT8	RegionNeedsSwap(UINT8 region);

static CPUInfo *	GetCPUInfo(UINT8 cpu);
static CPUInfo *	GetRegionCPUInfo(UINT8 region);

static UINT32	SwapAddress(UINT32 address, UINT8 dataSize, CPUInfo * info);

static UINT32	ReadData(CheatAction * action);
static void		WriteData(CheatAction * action, UINT32 data);

static void		WatchCheatEntry(CheatEntry * entry, UINT8 associate);
static void		AddActionWatch(CheatAction * action, CheatEntry * entry);
static void		RemoveAssociatedWatches(CheatEntry * entry);

static void		ResetAction(CheatAction * action);
static void		ActivateCheat(CheatEntry * entry);
static void		DeactivateCheat(CheatEntry * entry);
static void		TempDeactivateCheat(CheatEntry * entry);

static void		DoCheatOperation(CheatAction * action);
static void		DoCheatAction(CheatAction * action);
static void		DoCheatEntry(CheatEntry * entry);

static void		UpdateAllCheatInfo(void);
static void		UpdateCheatInfo(CheatEntry * entry, UINT8 isLoadTime);

static int		IsAddressInRange(CheatAction * action, UINT32 length);

static void		BuildCPUInfoList(void);

/**** Imports ****************************************************************/

/**** Code *******************************************************************/

static int ShiftKeyPressed(void)
{
	return (code_pressed(KEYCODE_LSHIFT) || code_pressed(KEYCODE_RSHIFT));
}

static int ControlKeyPressed(void)
{
	return (code_pressed(KEYCODE_LCONTROL) || code_pressed(KEYCODE_RCONTROL));
}

static int AltKeyPressed(void)
{
	return (code_pressed(KEYCODE_LALT) || code_pressed(KEYCODE_RALT));
}

#if 1

#if OSD_READKEY_KLUDGE

/*	dirty hack until osd_readkey_unicode is supported in MAMEW
	re-implementation of osd_readkey_unicode */
static int ReadKeyAsync(int flush)
{
	int	code;

	if(flush)
	{
		while(code_read_async() != CODE_NONE) ;

		return 0;
	}

	while(1)
	{
		code = code_read_async();

		if(code == CODE_NONE)
		{
			return 0;
		}
		else if((code >= KEYCODE_A) && (code <= KEYCODE_Z))
		{
			if(ShiftKeyPressed())
			{
				return 'A' + (code - KEYCODE_A);
			}
			else
			{
				return 'a' + (code - KEYCODE_A);
			}
		}
		else if((code >= KEYCODE_0) && (code <= KEYCODE_9))
		{
			if(ShiftKeyPressed())
			{
				return ")!@#$%^&*("[code - KEYCODE_0];
			}
			else
			{
				return '0' + (code - KEYCODE_0);
			}
		}
		else if((code >= KEYCODE_0_PAD) && (code <= KEYCODE_0_PAD))
		{
			return '0' + (code - KEYCODE_0_PAD);
		}
		else if(code == KEYCODE_TILDE)
		{
			if(ShiftKeyPressed())
			{
				return '~';
			}
			else
			{
				return '`';
			}
		}
		else if(code == KEYCODE_MINUS)
		{
			if(ShiftKeyPressed())
			{
				return '_';
			}
			else
			{
				return '-';
			}
		}
		else if(code == KEYCODE_EQUALS)
		{
			if(ShiftKeyPressed())
			{
				return '+';
			}
			else
			{
				return '=';
			}
		}
		else if(code == KEYCODE_BACKSPACE)
		{
			return 0x08;
		}
		else if(code == KEYCODE_OPENBRACE)
		{
			if(ShiftKeyPressed())
			{
				return '{';
			}
			else
			{
				return '[';
			}
		}
		else if(code == KEYCODE_CLOSEBRACE)
		{
			if(ShiftKeyPressed())
			{
				return '}';
			}
			else
			{
				return ']';
			}
		}
		else if(code == KEYCODE_COLON)
		{
			if(ShiftKeyPressed())
			{
				return ':';
			}
			else
			{
				return ';';
			}
		}
		else if(code == KEYCODE_QUOTE)
		{
			if(ShiftKeyPressed())
			{
				return '\"';
			}
			else
			{
				return '\'';
			}
		}
		else if(code == KEYCODE_BACKSLASH)
		{
			if(ShiftKeyPressed())
			{
				return '|';
			}
			else
			{
				return '\\';
			}
		}
		else if(code == KEYCODE_COMMA)
		{
			if(ShiftKeyPressed())
			{
				return '<';
			}
			else
			{
				return ',';
			}
		}
		else if(code == KEYCODE_STOP)
		{
			if(ShiftKeyPressed())
			{
				return '>';
			}
			else
			{
				return '.';
			}
		}
		else if(code == KEYCODE_SLASH)
		{
			if(ShiftKeyPressed())
			{
				return '?';
			}
			else
			{
				return '/';
			}
		}
		else if(code == KEYCODE_SLASH_PAD)
		{
			return '/';
		}
		else if(code == KEYCODE_ASTERISK)
		{
			return '*';
		}
		else if(code == KEYCODE_MINUS_PAD)
		{
			return '-';
		}
		else if(code == KEYCODE_PLUS_PAD)
		{
			return '+';
		}
		else if(code == KEYCODE_SPACE)
		{
			return ' ';
		}
	}
}

#define osd_readkey_unicode ReadKeyAsync

#endif

#endif

static int UIPressedRepeatThrottle(int code, int baseSpeed)
{
	static int	lastCode = -1;
	static int	lastSpeed = -1;
	static int	incrementTimer = 0;
	int			pressed = 0;

	const int	kDelayRampTimer = 10;

	if(seq_pressed(input_port_type_seq(code)))
	{
		if(lastCode != code)
		{
			lastCode = code;
			lastSpeed = baseSpeed;
			incrementTimer = kDelayRampTimer * lastSpeed;
		}
		else
		{
			incrementTimer--;

			if(incrementTimer <= 0)
			{
				incrementTimer = kDelayRampTimer * lastSpeed;

				lastSpeed /= 2;
				if(lastSpeed < 1)
					lastSpeed = 1;

				pressed = 1;
			}
		}
	}
	else
	{
		if(lastCode == code)
		{
			lastCode = -1;
		}
	}

	return input_ui_pressed_repeat(code, lastSpeed);
}

static int ReadHexInput(void)
{
	int	i;

	for(i = 0; i < 10; i++)
	{
		if(code_pressed_memory(KEYCODE_0 + i))
		{
			return i;
		}
	}

	for(i = 0; i < 10; i++)
	{
		if(code_pressed_memory(KEYCODE_0_PAD + i))
		{
			return i;
		}
	}

	for(i = 0; i < 6; i++)
	{
		if(code_pressed_memory(KEYCODE_A + i))
		{
			return i + 10;
		}
	}

	return -1;
}

static char * DoDynamicEditTextField(char * buf)
{
	char	code = osd_readkey_unicode(0) & 0xFF;

	if(code == 0x08)
	{
		if(buf)
		{
			UINT32	length = strlen(buf);

			if(length > 0)
			{
				buf[length - 1] = 0;

				if(length > 1)
				{
					buf = realloc(buf, length);
				}
				else
				{
					free(buf);

					buf = NULL;
				}
			}
		}
	}
	else if(isprint(code))
	{
		if(buf)
		{
			UINT32	length = strlen(buf);

			buf = realloc(buf, length + 2);

			buf[length] = code;
			buf[length + 1] = 0;
		}
		else
		{
			buf = malloc(2);

			buf[0] = code;
			buf[1] = 0;
		}
	}

	return buf;
}

static void DoStaticEditTextField(char * buf, int size)
{
	char	code = osd_readkey_unicode(0) & 0xFF;
	UINT32	length;

	if(!buf)
		return;

	length = strlen(buf);

	if(code == 0x08)
	{
		if(length > 0)
		{
			buf[length - 1] = 0;
		}
	}
	else if(isprint(code))
	{
		if(length + 1 < size)
		{
			buf[length] = code;
			buf[length + 1] = 0;
		}
	}
}

static UINT32 DoEditHexField(UINT32 data)
{
	INT8	key;

	key = ReadHexInput();

	if(key != -1)
	{
		data <<= 4;
		data |= key;
	}

	return data;
}

static UINT32 DoEditHexFieldSigned(UINT32 data, UINT32 mask)
{
	INT8	key;
	UINT32	isNegative = data & mask;

	if(isNegative)
		data |= mask;

	key = ReadHexInput();

	if(key != -1)
	{
		if(isNegative)
			data = (~data) + 1;

		data <<= 4;
		data |= key;

		if(isNegative)
			data = (~data) + 1;
	}
	else if(code_pressed_memory(KEYCODE_MINUS))
	{
		data = (~data) + 1;
	}

	return data;
}

static INT32 DoEditDecField(INT32 data, INT32 min, INT32 max)
{
	char	code = osd_readkey_unicode(0) & 0xFF;

	if((code >= '0') && (code <= '9'))
	{
		data *= 10;
		data += (code - '0');
	}
	else if(code == '-')
	{
		data = -data;
	}
	else if(code == 0x08)
	{
		data /= 10;
	}

	if(data < min)
		data = min;
	if(data > max)
		data = max;

	return data;
}

void InitCheat(void)
{
	int	screenWidth, screenHeight;

	artwork_get_screensize(&screenWidth, &screenHeight);

	cheatList =				NULL;
	cheatListLength =		0;

	watchList =				NULL;
	watchListLength =		0;

	searchList =			NULL;
	searchListLength =		0;


	currentSearchIdx =		0;
	foundCheatDatabase =	0;
	cheatsDisabled =		0;
	watchesDisabled =		0;

	useClassicSearchBox =	1;
	dontPrintNewLabels =	0;
	autoSaveEnabled =		0;

	fullMenuPageHeight =	screenHeight / (3 * uirotcharheight / 2) - 1;

	BuildCPUInfoList();

	LoadCheatDatabase();

	ResizeSearchList(1);
	ResizeWatchList(20);

	BuildSearchRegions(GetCurrentSearch());
	AllocateSearchRegions(GetCurrentSearch());

	InitStringTable();
}

void StopCheat(void)
{
	int	i;

	if(autoSaveEnabled)
	{
		DoAutoSaveCheats();
	}

	DisposeCheatDatabase();

	if(watchList)
	{
		for(i = 0; i < watchListLength; i++)
		{
			DisposeWatch(&watchList[i]);
		}

		free(watchList);

		watchList = NULL;
	}

	if(searchList)
	{
		for(i = 0; i < searchListLength; i++)
		{
			DisposeSearch(i);
		}

		free(searchList);

		searchList = NULL;
	}

	FreeStringTable();

	free(menuItemInfo);
	menuItemInfo = NULL;

	cheatListLength =		0;
	watchListLength =		0;
	searchListLength =		0;
	currentSearchIdx =		0;
	cheatEngineWasActive =	0;
	foundCheatDatabase =	0;
	cheatsDisabled =		0;
	watchesDisabled =		0;
	menuItemInfoLength =	0;
	useClassicSearchBox =	1;
	dontPrintNewLabels =	0;
	autoSaveEnabled =		0;
}

int cheat_menu(struct mame_bitmap * bitmap, int selection)
{
	enum
	{
		kMenu_EnableDisable = 0,
		kMenu_AddEdit,
		kMenu_StartSearch,
		kMenu_ContinueSearch,
		kMenu_ViewResults,
		kMenu_RestoreSearch,
		kMenu_ChooseWatch,
		kMenu_DisplayHelp,
		kMenu_Options,
		kMenu_Return,

		kMenu_Max
	};

	const char		* menu_item[kMenu_Max + 1];
	INT32			sel;
	UINT8			total;
	static INT32	submenu_choice = 0;
	static int		firstEntry = 0;

	cheatEngineWasActive = 1;

	total = 0;
	sel = selection - 1;

	if(submenu_choice)
	{
		switch(sel)
		{
			case kMenu_EnableDisable:
				submenu_choice = EnableDisableCheatMenu(bitmap, submenu_choice, firstEntry);
				break;

			case kMenu_AddEdit:
				submenu_choice = AddEditCheatMenu(bitmap, submenu_choice);
				break;

			case kMenu_StartSearch:
				if(useClassicSearchBox)
					submenu_choice = DoSearchMenuClassic(bitmap, submenu_choice, 1);
				else
					submenu_choice = DoSearchMenu(bitmap, submenu_choice, 1);
				break;

			case kMenu_ContinueSearch:
				if(useClassicSearchBox)
					submenu_choice = DoSearchMenuClassic(bitmap, submenu_choice, 0);
				else
					submenu_choice = DoSearchMenu(bitmap, submenu_choice, 0);
				break;

			case kMenu_ViewResults:
				submenu_choice = ViewSearchResults(bitmap, submenu_choice, firstEntry);
				break;

			case kMenu_ChooseWatch:
				submenu_choice = ChooseWatch(bitmap, submenu_choice);
				break;

			case kMenu_DisplayHelp:
				submenu_choice = DisplayHelp(bitmap, submenu_choice);
				break;

			case kMenu_Options:
				submenu_choice = SelectOptions(bitmap, submenu_choice);
				break;

			case kMenu_Return:
				submenu_choice = 0;
				sel = -1;
				break;
		}

		firstEntry = 0;

		if(submenu_choice == -1)
			submenu_choice = 0;

		return sel + 1;
	}

	menu_item[total++] = ui_getstring(UI_enablecheat);
	menu_item[total++] = ui_getstring(UI_addeditcheat);
	menu_item[total++] = ui_getstring(UI_startcheat);
	menu_item[total++] = ui_getstring(UI_continuesearch);
	menu_item[total++] = ui_getstring(UI_viewresults);
	menu_item[total++] = ui_getstring(UI_restoreresults);
	menu_item[total++] = ui_getstring(UI_memorywatch);
	menu_item[total++] = ui_getstring(UI_generalhelp);
	menu_item[total++] = ui_getstring(UI_options);
	menu_item[total++] = ui_getstring(UI_returntomain);
	menu_item[total] = 0;

	ui_displaymenu(bitmap, menu_item, 0, 0, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		if(sel < (total - 1))
			sel++;
		else
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		if(sel > 0)
			sel--;
		else
			sel = total - 1;
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		switch(sel)
		{
			case kMenu_Return:
				submenu_choice = 0;
				sel = -1;
				break;

			case kMenu_RestoreSearch:
			{
				SearchInfo	* search = GetCurrentSearch();

				if(search && search->backupValid)
				{
					RestoreSearchBackup(search);

					usrintf_showmessage_secs(1, "values restored");
				}
				else
				{
					usrintf_showmessage_secs(1, "there are no old values");
				}
			}
			break;

			default:
				firstEntry = 1;
				submenu_choice = 1;
				schedule_full_refresh();
				break;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if((sel == -1) || (sel == -2))
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

static UINT32 DoShift(UINT32 input, INT8 shift)
{
	if(shift > 0)
		return input >> shift;
	else
		return input << -shift;
}

static UINT32 BCDToDecimal(UINT32 value)
{
	UINT32	accumulator = 0;
	UINT32	multiplier = 1;
	int		i;

	for(i = 0; i < 8; i++)
	{
		accumulator += (value & 0xF) * multiplier;

		multiplier *= 10;
		value >>= 4;
	}

	return accumulator;
}

static UINT32 DecimalToBCD(UINT32 value)
{
	UINT32	accumulator = 0;
	UINT32	divisor = 10;
	int		i;

	for(i = 0; i < 8; i++)
	{
		UINT32	temp;

		temp = value % divisor;
		value -= temp;
		temp /= divisor / 10;

		accumulator += temp << (i * 4);

		divisor *= 10;
	}

	return accumulator;
}

static void RebuildStringTables(void)
{
	UINT32	storageNeeded, i;
	char	* traverse;

	storageNeeded =				(menuStrings.mainStringLength + menuStrings.subStringLength) * menuStrings.numStrings;

	menuStrings.mainList =		(const char **)	realloc((char *)	menuStrings.mainList,		sizeof(char *) * menuStrings.length);
	menuStrings.subList =		(const char **)	realloc((char *)	menuStrings.subList,		sizeof(char *) * menuStrings.length);
	menuStrings.flagList =						realloc(			menuStrings.flagList,		sizeof(char)   * menuStrings.length);
	menuStrings.mainStrings =					realloc(			menuStrings.mainStrings,	sizeof(char *) * menuStrings.numStrings);
	menuStrings.subStrings =					realloc(			menuStrings.subStrings,		sizeof(char *) * menuStrings.numStrings);
	menuStrings.buf =							realloc(			menuStrings.buf,			sizeof(char)   * storageNeeded);

	if(	(!menuStrings.mainList && menuStrings.length) ||
		(!menuStrings.subList && menuStrings.length) ||
		(!menuStrings.flagList && menuStrings.length) ||
		(!menuStrings.mainStrings && menuStrings.numStrings) ||
		(!menuStrings.subStrings && menuStrings.numStrings) ||
		(!menuStrings.buf && storageNeeded))
	{
		logerror(	"cheat: memory allocation error\n"
					"	length =			%.8X\n"
					"	numStrings =		%.8X\n"
					"	mainStringLength =	%.8X\n"
					"	subStringLength =	%.8X\n"
					"%.8X %.8X %.8X %.8X %.8X %.8X\n",
					menuStrings.length,
					menuStrings.numStrings,
					menuStrings.mainStringLength,
					menuStrings.subStringLength,

					(int)menuStrings.mainList,
					(int)menuStrings.subList,
					(int)menuStrings.flagList,
					(int)menuStrings.mainStrings,
					(int)menuStrings.subStrings,
					(int)menuStrings.buf);

		exit(1);
	}

	traverse = menuStrings.buf;

	for(i = 0; i < menuStrings.numStrings; i++)
	{
		menuStrings.mainStrings[i] = traverse;
		traverse += menuStrings.mainStringLength;

		menuStrings.subStrings[i] = traverse;
		traverse += menuStrings.subStringLength;
	}
}

static void RequestStrings(UINT32 length, UINT32 numStrings, UINT32 mainStringLength, UINT32 subStringLength)
{
	UINT8	changed = 0;

	if(menuStrings.length < length)
	{
		menuStrings.length = length;

		changed = 1;
	}

	if(menuStrings.numStrings < numStrings)
	{
		menuStrings.numStrings = numStrings;

		changed = 1;
	}

	if(menuStrings.mainStringLength < mainStringLength)
	{
		menuStrings.mainStringLength = mainStringLength;

		changed = 1;
	}

	if(menuStrings.subStringLength < subStringLength)
	{
		menuStrings.subStringLength = subStringLength;

		changed = 1;
	}

	if(changed)
	{
		RebuildStringTables();
	}
}

static void InitStringTable(void)
{
	memset(&menuStrings, 0, sizeof(MenuStringList));
}

static void FreeStringTable(void)
{
	free((char *)menuStrings.mainList);
	free((char *)menuStrings.subList);
	free(menuStrings.flagList);
	free(menuStrings.mainStrings);
	free(menuStrings.subStrings);
	free(menuStrings.buf);

	memset(&menuStrings, 0, sizeof(MenuStringList));
}

static INT32 UserSelectValueMenu(struct mame_bitmap * bitmap, int selection, CheatEntry * entry)
{
	char					buf[2048];
	int						sel;
	CheatAction				* action;
	static INT32			value = -1;
	static int				firstTime = 1;
	int						delta = 0;
	int						displayValue;
	int						keyValue;
	int						forceUpdate = 0;

	sel =		selection - 1;

	action = &entry->actionList[0];

	/* if we're just entering, save the value*/
	if(firstTime)
	{
		UINT32	min = EXTRACT_FIELD(action->type, UserSelectMinimum);
		UINT32	max = action->originalDataField + min;

		value = ReadData(action);

		/* and check for valid BCD values*/
		if(TEST_FIELD(action->type, UserSelectBCD))
		{
			value = BCDToDecimal(value);
			value = DecimalToBCD(value);
		}

		if(value < min)
			value = max;
		if(value > max)
			value = min;

		action->data = value;
		firstTime = 0;
	}

	displayValue = value;

	/* if the minimum display value is one, add one to the display value*/
	if(TEST_FIELD(action->type, UserSelectMinimumDisplay))
	{
		/* bcd -> dec*/
		if(TEST_FIELD(action->type, UserSelectBCD))
		{
			displayValue = BCDToDecimal(displayValue);
		}

		displayValue++;

		/* dec -> bcd*/
		if(TEST_FIELD(action->type, UserSelectBCD))
		{
			displayValue = DecimalToBCD(displayValue);
		}
	}

	/* print it*/
	if(TEST_FIELD(action->type, UserSelectBCD))
	{
		sprintf(buf, "\t%s\n\t%.2X\n", ui_getstring(UI_search_select_value), displayValue);
	}
	else
	{
		sprintf(buf, "\t%s\n\t%.2X (%d)\n", ui_getstring(UI_search_select_value), displayValue, displayValue);
	}

	/* create fake menu strings*/
	strcat(buf, "\t");
	strcat(buf, ui_getstring(UI_lefthilight));
	strcat(buf, " ");
	strcat(buf, ui_getstring(UI_OK));
	strcat(buf, " ");
	strcat(buf, ui_getstring(UI_righthilight));

	/* print fake menu*/
	ui_displaymessagewindow(bitmap, buf);

	/* get user input*/
	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		delta = -1;
	}
	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		delta = 1;
	}

	/* done?*/
	if(input_ui_pressed(IPT_UI_SELECT))
	{
		/* ### redundant?? probably can be removed*/
		if(!firstTime)
		{
			int	i;

			/* copy data field to all user select cheats*/
			for(i = 0; i < entry->actionListLength; i++)
			{
				CheatAction	* traverse = &entry->actionList[0];

				if(TEST_FIELD(traverse->type, UserSelectEnable))
				{
					traverse->data = value;
				}
			}

			/* and activate the cheat*/
			ActivateCheat(entry);
		}

		/* reset and return*/
		firstTime = 1;
		sel = -1;
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
	{
		firstTime = 1;
		sel = -1;
	}

	if(input_ui_pressed(IPT_UI_CONFIGURE))
	{
		firstTime = 1;
		sel = -2;
	}

	/* get a key*/
	keyValue = ReadHexInput();

	/* if we got a key*/
	if(keyValue != -1)
	{
		/* add it*/
		if(TEST_FIELD(action->type, UserSelectBCD))
		{
			if(value < 10)
			{
				value *= 10;
				value &= 0xFF;
				value += keyValue;
			}
		}
		else
		{
			value <<= 4;
			value &= 0xF0;
			value |= keyValue & 0x0F;
		}

		delta = 0;
		forceUpdate = 1;
	}

	/* wrap-around with BCD stuff*/
	/* ### this is a really bad way to do this*/
	if(delta || forceUpdate)
	{
		INT32	min = EXTRACT_FIELD(action->type, UserSelectMinimum);
		INT32	max = action->originalDataField + min;

		if(TEST_FIELD(action->type, UserSelectBCD))
		{
			value = BCDToDecimal(value);
		}

		value += delta;

		if(TEST_FIELD(action->type, UserSelectBCD))
		{
			value = DecimalToBCD(value);
		}

		if(value < min)
			value = max;
		if(value > max)
			value = min;
	}

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

static INT32 CommentMenu(struct mame_bitmap * bitmap, int selection, CheatEntry * entry)
{
	char	buf[2048];
	int		sel;
	const char	* comment;

	if(!entry)
		return 0;

	sel = selection - 1;

	/* create fake menu strings*/
	if(entry->comment && entry->comment[0])
		comment = entry->comment;
	else
		comment = "(none)";

	sprintf(buf, "%s\n\t%s %s %s", comment, ui_getstring(UI_lefthilight), ui_getstring(UI_OK), ui_getstring(UI_righthilight));

	/* print fake menu*/
	ui_displaymessagewindow(bitmap, buf);

	/* done?*/
	if(input_ui_pressed(IPT_UI_SELECT))
	{
		sel = -1;
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
	{
		sel = -1;
	}

	if(input_ui_pressed(IPT_UI_CONFIGURE))
	{
		sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

static int EnableDisableCheatMenu(struct mame_bitmap * bitmap, int selection, int firstTime)
{
	INT32			sel;
	static INT32	submenu_choice = 0;
	static INT32	submenu_id = 0;
	const char		** menu_item;
	const char		** menu_subitem;
	char			* flagBuf;
	INT32			i;
	INT32			total = 0;
	CheatEntry		* entry;

	RequestStrings(cheatListLength + 5, 0, 0, 0);

	menu_item = menuStrings.mainList;
	menu_subitem = menuStrings.subList;
	flagBuf = menuStrings.flagList;

	sel = selection - 1;

	/* If a submenu has been selected, go there */
	if(submenu_choice)
	{
		switch(submenu_id)
		{
			case 1:
				submenu_choice = CommentMenu(bitmap, submenu_choice, &cheatList[sel]);
				break;

			case 2:
				submenu_choice = UserSelectValueMenu(bitmap, submenu_choice, &cheatList[sel]);
				break;

			case 3:
				submenu_choice = EditCheatMenu(bitmap, &cheatList[sel], submenu_choice);
				break;

			default:
				submenu_choice = 0;
		}

		if(submenu_choice == -1)
		{
			submenu_choice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	/* No submenu active, do the watchpoint menu */
	for(i = 0; i < cheatListLength; i++)
	{
		CheatEntry	* traverse = &cheatList[i];

		if(traverse->name)
		{
			menu_item[total] = traverse->name;
		}
		else
		{
			menu_item[total] = "null name";
		}

		menu_subitem[total] = NULL;

		if(traverse->flags & kCheatFlag_Select)
		{
			if((traverse->flags & kCheatFlag_OneShot) && !traverse->selection)
			{
				traverse->selection = 1;
			}

			if(traverse->selection && (traverse->selection < traverse->actionListLength))
			{
				menu_subitem[total] = traverse->actionList[traverse->selection].optionalName;
			}
			else
			{
				menu_subitem[total] = ui_getstring(UI_off);
			}
		}
		else
		{
			/* add submenu options for all cheats that are not comments*/
			if(!(traverse->flags & kCheatFlag_Null))
			{
				if(traverse->flags & kCheatFlag_OneShot)
				{
					menu_subitem[total] = ui_getstring(UI_set);
				}
				else
				{
					if(traverse->flags & kCheatFlag_Active)
					{
						menu_subitem[total] = ui_getstring(UI_on);
					}
					else
					{
						menu_subitem[total] = ui_getstring(UI_off);
					}
				}
			}
		}

		if(traverse->comment && traverse->comment[0])
			flagBuf[total] = 1;
		else
			flagBuf[total] = 0;

		total++;
	}

	if(cheatListLength == 0)
	{
		if(foundCheatDatabase)
		{
			menu_item[total] = "there are no cheats for this game";
			menu_subitem[total] = NULL;
			flagBuf[total] = 0;
			total++;
		}
		else
		{
			menu_item[total] = "cheat database not found";
			menu_subitem[total] = NULL;
			flagBuf[total] = 0;
			total++;

			menu_item[total] = "unzip it and place it in the MAME directory";
			menu_subitem[total] = NULL;
			flagBuf[total] = 0;
			total++;
		}
	}

	menu_item[total] = ui_getstring(UI_returntoprior);
	menu_subitem[total] = NULL;
	flagBuf[total] = 0;
	total++;

	menu_item[total] = 0;	/* terminate array */
	menu_subitem[total] = 0;
	flagBuf[total] = 0;

	if(sel < 0)
		sel = 0;
	if(sel > (total - 1))
		sel = total - 1;

	if(cheatListLength && firstTime)
	{
		while(	(sel < total - 1) &&
				(cheatList[sel].flags & kCheatFlag_Null))
				sel++;
	}

	ui_displaymenu(bitmap, menu_item, menu_subitem, flagBuf, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;

		if(cheatListLength)
		{
			for(i = 0;	(i < fullMenuPageHeight / 2) &&
						(sel < total - 1) &&
						(cheatList[sel].flags & kCheatFlag_Null); i++)
				sel++;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
		{
			sel = total - 1;
		}
		else
		{
			if(cheatListLength)
			{
				for(i = 0;	(i < fullMenuPageHeight / 2) &&
							(sel != total - 1) &&
							(cheatList[sel].flags & kCheatFlag_Null); i++)
				{
					sel--;

					if(sel < 0)
						sel = total - 1;
				}
			}
		}
	}

	if(	(sel >= 0) &&
		(sel < cheatListLength))
		entry = &cheatList[sel];
	else
		entry = NULL;

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		if((sel < (total - 1)) && entry)
		{
			if(entry->flags & kCheatFlag_Select)
			{
				entry->selection--;

				if(entry->flags & kCheatFlag_OneShot)
				{
					if(entry->selection <= 0)
						entry->selection = entry->actionListLength - 1;
				}
				else
				{
					if(entry->selection < 0)
						entry->selection = entry->actionListLength - 1;

					if(entry->selection == 0)
					{
						DeactivateCheat(entry);
					}
					else
					{
						ActivateCheat(entry);
					}
				}
			}
			else
			{
				if(	!(entry->flags & kCheatFlag_Null) &&
					!(entry->flags & kCheatFlag_OneShot))
				{
					int active = entry->flags & kCheatFlag_Active;

					active ^= 0x01;

					/* get the user's selected value if needed */
					if((entry->flags & kCheatFlag_UserSelect) && active)
					{
						submenu_id = 2;
						submenu_choice = 1;
						schedule_full_refresh();
					}
					else
					{
						if(active)
							ActivateCheat(entry);
						else
							DeactivateCheat(entry);
					}
				}
			}
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		if((sel < (total - 1)) && entry)
		{
			if(entry->flags & kCheatFlag_Select)
			{
				entry->selection++;

				if(entry->flags & kCheatFlag_OneShot)
				{
					if(entry->selection >= entry->actionListLength)
					{
						entry->selection = 1;

						if(entry->selection >= entry->actionListLength)
							entry->selection = 0;
					}
				}
				else
				{
					if(entry->selection >= entry->actionListLength)
					{
						entry->selection = 0;

						DeactivateCheat(entry);
					}
					else
					{
						ActivateCheat(entry);
					}
				}
			}
			else
			{
				if(	!(entry->flags & kCheatFlag_Null) &&
					!(entry->flags & kCheatFlag_OneShot))
				{
					int active = entry->flags & kCheatFlag_Active;

					active ^= 0x01;

					/* get the user's selected value if needed */
					if((entry->flags & kCheatFlag_UserSelect) && active)
					{
						submenu_id = 2;
						submenu_choice = 1;
						schedule_full_refresh();
					}
					else
					{
						if(active)
							ActivateCheat(entry);
						else
							DeactivateCheat(entry);
					}
				}
			}
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == (total - 1))
		{
			/* return to prior menu */
			submenu_choice = 0;
			sel = -1;
		}
		else if((sel < (total - 1)) && entry)
		{
			if(ShiftKeyPressed())
			{
				if(cheatList[sel].comment && cheatList[sel].comment[0])
				{
					submenu_id = 1;
					submenu_choice = 1;
					schedule_full_refresh();
				}
				else
				{
					ActivateCheat(&cheatList[sel]);

					if(cheatList[sel].flags & kCheatFlag_OneShot)
						usrintf_showmessage_secs(1, "%s activated", cheatList[sel].name);
				}
			}
			else
			{
				if(entry->flags & kCheatFlag_UserSelect)
				{
					submenu_id = 2;
					submenu_choice = 1;
					schedule_full_refresh();
				}
				else
				{
					ActivateCheat(&cheatList[sel]);

					if(cheatList[sel].flags & kCheatFlag_OneShot)
						usrintf_showmessage_secs(1, "%s activated", cheatList[sel].name);
				}
			}
		}
	}

	if(input_ui_pressed(IPT_UI_WATCH_VALUE))
	{
		WatchCheatEntry(entry, 0);
	}

	if(ShiftKeyPressed())
	{
		if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
		{
			for(i = 0; i < cheatListLength; i++)
				SaveCheat(&cheatList[i]);

			usrintf_showmessage_secs(1, "%d cheats saved", cheatListLength);
		}

		if(input_ui_pressed(IPT_UI_ADD_CHEAT))
		{
			AddCheatBefore(sel);
		}

		if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
		{
			DeleteCheatAt(sel);
		}

		if(input_ui_pressed(IPT_UI_EDIT_CHEAT))
		{
			if(entry)
			{
				submenu_id = 3;
				submenu_choice = 1;
				schedule_full_refresh();
			}
		}
	}
	else
	{
		if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
		{
			SaveCheat(entry);
		}
	}

	/* Cancel pops us up a menu level */
	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;

	/* The UI key takes us all the way back out */
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}
	
	return sel + 1;
}

static int EditCheatMenu(struct mame_bitmap * bitmap, CheatEntry * entry, int selection)
{
	const char *	kTypeNames[] =
	{
		"Normal/Delay",
		"Wait",
		"Ignore Decrement",
		"Watch",
		"Comment",
		"Select"
	};
	
	const char *	kNumbersTable[] =
	{
		"0",	"1",	"2",	"3",	"4",	"5",	"6",	"7",
		"8",	"9",	"10",	"11",	"12",	"13",	"14",	"15",
		"16",	"17",	"18",	"19",	"20",	"21",	"22",	"23",
		"24",	"25",	"26",	"27",	"28",	"29",	"30",	"31"
	};

	const char *	kOperationNames[] =
	{
		"Write",
		"Add/Subtract",
		"Force Range",
		"Set/Clear Bits",
		"Unused (4)",
		"Unused (5)",
		"Unused (6)",
		"Null"
	};

	const char *	kAddSubtractNames[] =
	{
		"Add",
		"Subtract"
	};

	const char *	kSetClearNames[] =
	{
		"Set",
		"Clear"
	};

	const char *	kPrefillNames[] =
	{
		"None",
		"FF",
		"00",
		"01"
	};

	const char *	kEndiannessNames[] =
	{
		"Normal",
		"Swap"
	};

	const char *	kRegionNames[] =
	{
		"CPU1",		"CPU2",		"CPU3",		"CPU4",		"CPU5",		"CPU6",		"CPU7",		"CPU8",
		"GFX1",		"GFX2",		"GFX3",		"GFX4",		"GFX5",		"GFX6",		"GFX7",		"GFX8",
		"PROMS",
		"SOUND1",	"SOUND2",	"SOUND3",	"SOUND4",	"SOUND5",	"SOUND6",	"SOUND7",	"SOUND8",
		"USER1",	"USER2",	"USER3",	"USER4",	"USER5",	"USER6",	"USER7",	"USER8"
	};

	const char *	kLocationNames[] =
	{
		"Normal",
		"Region",
		"Mapped Memory",
		"Custom",
		"Relative Address",
		"Unused (5)",
		"Unused (6)",
		"Unused (7)"
	};

	const char *	kCustomLocationNames[] =
	{
		"Comment",
		"EEPROM",
		"Select",
		"Unused (3)",	"Unused (4)",	"Unused (5)",	"Unused (6)",	"Unused (7)",
		"Unused (8)",	"Unused (9)",	"Unused (10)",	"Unused (11)",	"Unused (12)",
		"Unused (13)",	"Unused (14)",	"Unused (15)",	"Unused (16)",	"Unused (17)",
		"Unused (18)",	"Unused (19)",	"Unused (20)",	"Unused (21)",	"Unused (22)",
		"Unused (23)",	"Unused (24)",	"Unused (25)",	"Unused (26)",	"Unused (27)",
		"Unused (28)",	"Unused (29)",	"Unused (30)",	"Unused (31)"
	};

	const char *	kKeycodeNames[] =
	{
		"A",		"B",		"C",		"D",		"E",		"F",
		"G",		"H",		"I",		"J",		"K",		"L",
		"M",		"N",		"O",		"P",		"Q",		"R",
		"S",		"T",		"U",		"V",		"W",		"X",
		"Y",		"Z",		"0",		"1",		"2",		"3",
		"4",		"5",		"6",		"7",		"8",		"9",
		"[0]",		"[1]",		"[2]",		"[3]",		"[4]",
		"[5]",		"[6]",		"[7]",		"[8]",		"[9]",
		"F1",		"F2",		"F3",		"F4",		"F5",
		"F6",		"F7",		"F8",		"F9",		"F10",
		"F11",		"F12",
		"ESC",		"~",		"-",		"=",		"BACKSPACE",
		"TAB",		"[",		"]",		"ENTER",	":",
		"\'",		"\\",		"\\",		",",		".",
		"/",		"SPACE",	"INS",		"DEL",
		"HOME",		"END",		"PGUP",		"PGDN",		"LEFT",
		"RIGHT",	"UP",		"DOWN",
		"[/]",		"[*]",		"[-]",		"[+]",
		"[DEL]",	"[ENT]",	"PTSCR",	"PAUSE",
		"LSHIFT",	"RSHIFT",	"LCONTROL",	"RCONTROL",
		"LALT",		"RALT",		"SCRLLK",	"NUMLK",	"CAPSLK",
		"LWIN",		"RWIN",		"MENU"
	};

	const char *	kSizeNames[] =
	{
		"8 Bit",
		"16 Bit",
		"24 Bit",
		"32 Bit"
	};

	enum
	{
		kType_Name = 0,					/*	text		name*/
										/*	NOTE:	read from base cheat (for idx == 0)*/
		kType_ExtendName,				/*	text		extraName*/
										/*	NOTE:	read from subcheat for (idx > 0) && (cheat[0].type == Select)*/
		kType_Comment,					/*	text		comment*/
										/*	NOTE:	read from base cheat (for idx == 0)*/
		kType_ActivationKey,			/*	key			activationKey*/
										/*	NOTE:	read from base cheat (for idx == 0)*/
		kType_Type,						/*	select		Type				Normal/Delay - Wait - Ignore Decrement - Watch -*/
										/*									Comment - Select*/
										/*	NOTE: also uses location type field for comment and select*/
		/* if((Type != Comment) && (Type != Select))*/
			/* if(Type != Watch)*/
				kType_OneShot,			/*	select		OneShot				Off - On*/
				kType_RestorePreviousValue,
										/*	select		RestorePreviousValue*/
										/*									Off - On*/
			/* if((Type == Normal/Delay) || (Type == Wait))*/
				kType_Delay,			/*	value		TypeParameter		0 - 7*/
			/* if(Type == Ignore Decrement)*/
				kType_IgnoreDecrementBy,/*	value		TypeParameter		0 - 7*/
			/* if(Type == Watch)*/
				kType_WatchSize,		/*	value		Data				0x01 - 0xFF (stored as 0x00 - 0xFE)*/
										/*	NOTE: value is packed in to 0x000000FF*/
				kType_WatchSkip,		/*	value		Data				0x00 - 0xFF*/
										/*	NOTE: value is packed in to 0x0000FF00*/
				kType_WatchPerLine,		/*	value		Data				0x00 - 0xFF*/
										/*	NOTE: value is packed in to 0x00FF0000*/
				kType_WatchAddValue,	/*	value		Data				-0x80 - 0x7F*/
										/*	NOTE: value is packed in to 0xFF000000*/
				kType_WatchFormat,		/*	select		TypeParameter		Hex - Decimal - Binary - ASCII*/
										/*	NOTE: value is packed in to 0x03*/
				kType_WatchLabel,		/*	select		TypeParameter		Off - On*/
										/*	NOTE: value is packed in to 0x04*/
				/* and set operation to null*/
			/* else*/
				kType_Operation,		/*	select		Operation			Write - Add/Subtract - Force Range - Set/Clear Bits -*/
										/*									Null*/
			/* if((Operation == Write) && (LocationType != Relative Address))*/
				kType_WriteMask,		/*	value		extendData			0x00000000 - 0xFFFFFFFF*/
			/* if(Operation == Add/Subtract)*/
				kType_AddSubtract,		/*	select		OperationParameter	Add - Subtract*/
				/* if(LocationType != Relative Address)*/
					/* if(OperationParameter == Add)*/
						kType_AddMaximum,
										/*	value		extendData			0x00000000 - 0xFFFFFFFF*/
					/* else*/
						kType_SubtractMinimum,
										/*	value		extendData			0x00000000 - 0xFFFFFFFF*/
			/* if((Operation == Force Range) && (LocationType != Relative Address))*/
				kType_RangeMinimum,		/*	value		extendData			0x00 - 0xFF*/
										/*	NOTE: value is packed in to upper byte of extendData (as a word)*/
				kType_RangeMaximum,		/*	value		extendData			0x00 - 0xFF*/
										/*	NOTE: value is packed in to lower byte of extendData (as a word)*/
			/* if(Operation == Set/Clear)*/
				kType_SetClear,			/*	select		OperationParameter	Set - Clear*/
			/* if((Operation != Null) || (Type == Watch))*/
				/* if(Type != Watch)*/
					kType_Data,
					kType_UserSelect,		/*	select		UserSelectEnable	Off - On*/
					/* if(UserSelect == On)*/
						kType_UserSelectMinimumDisp,
											/*	value		UserSelectMinimumDisplay*/
											/*									0 - 1*/
						kType_UserSelectMinimum,
											/*	value		UserSelectMinimum	0 - 1*/
						kType_UserSelectBCD,/*	select		UserSelectBCD		Off - On*/
						kType_Prefill,		/*	select		UserSelectPrefill	None - FF - 00 - 01*/
					/* if(idx > 0)*/
						kType_CopyPrevious,	/*	select		LinkCopyPreviousValue*/
										/*									Off - On*/
				kType_ByteLength,		/*	value		BytesUsed			1 - 4*/
				/* if(bytesUsed > 0)*/
					kType_Endianness,	/*	select		Endianness			Normal - Swap*/
				kType_LocationType,		/*	select		LocationType		Normal - Region - Mapped Memory - EEPROM -*/
										/*									Relative Address*/
										/*	NOTE: also uses LocationParameter for EEPROM type*/
				/* if((LocationType == Normal) || (LocationType == HandlerMemory))*/
					kType_CPU,			/*	value		LocationParameter	0 - 31*/
				/* if(LocationType == Region)*/
					kType_Region,		/*	select		LocationParameter	CPU1 - CPU2 - CPU3 - CPU4 - CPU5 - CPU6 - CPU7 -*/
										/*									CPU8 - GFX1 - GFX2 - GFX3 - GFX4 - GFX5 - GFX6 -*/
										/*									GFX7 - GFX8 - PROMS - SOUND1 - SOUND2 - SOUND3 -*/
										/*									SOUND4 - SOUND5 - SOUND6 - SOUND7 - SOUND8 -*/
										/*									USER1 - USER2 - USER3 - USER4 - USER5 - USER6 -*/
										/*									USER7*/
				/* if(LocationType == RelativeAddress)*/
					kType_PackedCPU,	/*	value		LocationParameter	0 - 7*/
										/*	NOTE: packed in to upper three bits of LocationParameter*/
					kType_PackedSize,	/*	value		LocationParameter	1 - 4*/
										/*	NOTE: packed in to lower two bits of LocationParameter*/
					kType_AddressIndex,	/*	value		extendData			-0x80000000 - 0x7FFFFFFF*/
				kType_Address,

		kType_Return,
		kType_Divider,

		kType_Max
	};

	INT32				sel;
	const char			** menuItem;
	const char			** menuSubItem;
	char				* flagBuf;
	char				** extendDataBuf;		/* FFFFFFFF (-80000000)*/
	char				** addressBuf;			/* FFFFFFFF*/
	char				** dataBuf;				/* 80000000 (-2147483648)*/
	char				** watchSizeBuf;		/* FF*/
	char				** watchSkipBuf;		/* FF*/
	char				** watchPerLineBuf;		/* FF*/
	char				** watchAddValueBuf;	/* FF*/
	INT32				i;
	INT32				total = 0;
	MenuItemInfoStruct	* info = NULL;
	CheatAction			* action = NULL;
	UINT8				isSelect = 0;
	static UINT8		editActive = 0;
	UINT32				increment = 1;
	UINT8				dirty = 0;
	static INT32		currentNameTemplate = 0;

	if(!entry)
		return 0;

	if(menuItemInfoLength < (kType_Max * entry->actionListLength) + 2)
	{
		menuItemInfoLength = (kType_Max * entry->actionListLength) + 2;

		menuItemInfo = realloc(menuItemInfo, menuItemInfoLength * sizeof(MenuItemInfoStruct));
	}

	RequestStrings((kType_Max * entry->actionListLength) + 2, 7 * entry->actionListLength, 24, 0);

	menuItem =			menuStrings.mainList;
	menuSubItem =		menuStrings.subList;
	flagBuf =			menuStrings.flagList;
	extendDataBuf =		&menuStrings.mainStrings[entry->actionListLength * 0];
	addressBuf =		&menuStrings.mainStrings[entry->actionListLength * 1];
	dataBuf =			&menuStrings.mainStrings[entry->actionListLength * 2];
	watchSizeBuf =		&menuStrings.mainStrings[entry->actionListLength * 3];	/* these fields are wasteful*/
	watchSkipBuf =		&menuStrings.mainStrings[entry->actionListLength * 4];	/* but the alternative is even more ugly*/
	watchPerLineBuf =	&menuStrings.mainStrings[entry->actionListLength * 5];
	watchAddValueBuf =	&menuStrings.mainStrings[entry->actionListLength * 6];

	sel = selection - 1;

	memset(flagBuf, 0, (kType_Max * entry->actionListLength) + 2);

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction	* traverse = &entry->actionList[i];

		UINT32		type =					EXTRACT_FIELD(traverse->type, Type);
		UINT32		typeParameter =			EXTRACT_FIELD(traverse->type, TypeParameter);
		UINT32		operation =				EXTRACT_FIELD(traverse->type, Operation) |
											EXTRACT_FIELD(traverse->type, OperationExtend) << 2;
		UINT32		operationParameter =	EXTRACT_FIELD(traverse->type, OperationParameter);
		UINT32		locationType =			EXTRACT_FIELD(traverse->type, LocationType);
		UINT32		locationParameter =		EXTRACT_FIELD(traverse->type, LocationParameter);

		UINT8		wasCommentOrSelect =	0;

		if(isSelect)
		{
			/* do extend name field*/

			menuItemInfo[total].subcheat = i;
			menuItemInfo[total].fieldType = kType_ExtendName;
			menuItem[total] = "Name";

			if(traverse->optionalName)
				menuSubItem[total] = traverse->optionalName;
			else
				menuSubItem[total] = "(none)";

			total++;
		}

		if(i == 0)
		{
			{
				/* do name field*/

				menuItemInfo[total].subcheat = i;
				menuItemInfo[total].fieldType = kType_Name;
				menuItem[total] = "Name";

				if(entry->name)
					menuSubItem[total] = entry->name;
				else
					menuSubItem[total] = "(none)";

				total++;
			}

			{
				/* do comment field*/

				menuItemInfo[total].subcheat = i;
				menuItemInfo[total].fieldType = kType_Comment;
				menuItem[total] = "Comment";

				if(entry->comment)
					menuSubItem[total] = entry->comment;
				else
					menuSubItem[total] = "(none)";

				total++;
			}

			{
				/* do activation key field*/

				menuItemInfo[total].subcheat = i;
				menuItemInfo[total].fieldType = kType_ActivationKey;
				menuItem[total] = "Activation Key";

				if(entry->activationKey < __code_key_first)
					entry->activationKey = __code_key_last;
				if(entry->activationKey > __code_key_last)
					entry->activationKey = __code_key_first;

				if(	(entry->flags & kCheatFlag_HasActivationKey))
				{
					menuSubItem[total] = kKeycodeNames[entry->activationKey - __code_key_first];
				}
				else
				{
					menuSubItem[total] = "(none)";
				}

				total++;
			}

			/* check for select cheat*/

			if(	(locationType == kLocation_Custom) &&
				(locationParameter == kCustomLocation_Select))
				isSelect = 1;
		}

		{
			/* do type field*/

			menuItemInfo[total].subcheat = i;
			menuItemInfo[total].fieldType = kType_Type;
			menuItem[total] = "Type";

			if(locationType == kLocation_Custom)
			{
				if(locationParameter == kCustomLocation_Comment)
				{
					wasCommentOrSelect = 1;

					menuSubItem[total] = kTypeNames[4];
				}
				else if(locationParameter == kCustomLocation_Select)
				{
					wasCommentOrSelect = 1;

					menuSubItem[total] = kTypeNames[5];
				}
				else
				{
					menuSubItem[total] = kTypeNames[type & 3];
				}
			}
			else
			{
				menuSubItem[total] = kTypeNames[type & 3];
			}

			total++;
		}

		if(!wasCommentOrSelect)
		{
			if(type != kType_Watch)
			{
				{
					/* do one shot field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_OneShot;
					menuItem[total] = "One Shot";
					menuSubItem[total] = ui_getstring(TEST_FIELD(traverse->type, OneShot) ? UI_on : UI_off);

					total++;
				}

				{
					/* do restore previous value field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_RestorePreviousValue;
					menuItem[total] = "Restore Previous Value";
					menuSubItem[total] = ui_getstring(TEST_FIELD(traverse->type, RestorePreviousValue) ? UI_on : UI_off);

					total++;
				}
			}

			if((type == kType_NormalOrDelay) || (type == kType_WaitForModification))
			{
				/* do delay field*/

				menuItemInfo[total].subcheat = i;
				menuItemInfo[total].fieldType = kType_Delay;
				menuItem[total] = "Delay";
				menuSubItem[total] = kNumbersTable[typeParameter];

				total++;
			}

			if(type == kType_IgnoreIfDecrementing)
			{
				/* do ignore decrement by field*/

				menuItemInfo[total].subcheat = i;
				menuItemInfo[total].fieldType = kType_IgnoreDecrementBy;
				menuItem[total] = "Ignore Decrement By";
				menuSubItem[total] = kNumbersTable[typeParameter];

				total++;
			}

			if(type == kType_Watch)
			{
				{
					/* do watch size field*/

					sprintf(watchSizeBuf[i], "%d", (traverse->originalDataField & 0xFF) + 1);

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_WatchSize;
					menuItem[total] = "Watch Size";
					menuSubItem[total] = watchSizeBuf[i];

					total++;
				}

				{
					/* do watch skip field*/

					sprintf(watchSkipBuf[i], "%d", (traverse->data >> 8) & 0xFF);

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_WatchSkip;
					menuItem[total] = "Watch Skip";
					menuSubItem[total] = watchSkipBuf[i];

					total++;
				}

				{
					/* do watch per line field*/

					sprintf(watchPerLineBuf[i], "%d", (traverse->data >> 16) & 0xFF);

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_WatchPerLine;
					menuItem[total] = "Watch Per Line";
					menuSubItem[total] = watchPerLineBuf[i];

					total++;
				}

				{
					/* do watch add value field*/

					{
						INT8	temp = (traverse->data >> 24) & 0xFF;

						if(temp < 0)
							sprintf(watchAddValueBuf[i], "-%.2X", -temp);
						else
							sprintf(watchAddValueBuf[i], "%.2X", temp);
					}

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_WatchAddValue;
					menuItem[total] = "Watch Add Value";
					menuSubItem[total] = watchAddValueBuf[i];

					total++;
				}

				{
					/* do watch format field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_WatchFormat;
					menuItem[total] = "Watch Format";
					menuSubItem[total] = kWatchDisplayTypeStringList[(typeParameter >> 0) & 0x03];

					total++;
				}

				{
					/* do watch label field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_WatchLabel;
					menuItem[total] = "Watch Label";
					menuSubItem[total] = ui_getstring(((typeParameter >> 2) & 0x01) ? UI_on : UI_off);

					total++;
				}
			}
			else
			{
				/* do operation field*/

				menuItemInfo[total].subcheat = i;
				menuItemInfo[total].fieldType = kType_Operation;
				menuItem[total] = "Operation";
				menuSubItem[total] = kOperationNames[operation];

				total++;
			}

			if((operation == kOperation_WriteMask) && (locationType != kLocation_IndirectIndexed))
			{
				/* do mask field*/

				int	numChars;

				if(traverse->flags & kActionFlag_IgnoreMask)
				{
					menuItemInfo[total].extraData = 0xFFFFFFFF;
					numChars = 8;
				}
				else
				{
					menuItemInfo[total].extraData = kCheatSizeMaskTable[EXTRACT_FIELD(traverse->type, BytesUsed)];
					numChars = kCheatSizeDigitsTable[EXTRACT_FIELD(traverse->type, BytesUsed)];
				}

				sprintf(extendDataBuf[i], "%.*X", numChars, traverse->extendData);

				menuItemInfo[total].subcheat = i;
				menuItemInfo[total].fieldType = kType_WriteMask;
				menuItem[total] = "Mask";
				menuSubItem[total] = extendDataBuf[i];

				total++;
			}

			if(operation == kOperation_AddSubtract)
			{
				{
					/* do add/subtract field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_AddSubtract;
					menuItem[total] = "Add/Subtract";
					menuSubItem[total] = kAddSubtractNames[operationParameter];

					total++;
				}

				if(locationType != kLocation_IndirectIndexed)
				{
					if(operationParameter)
					{
						/* do subtract minimum field*/

						sprintf(extendDataBuf[i], "%.8X", traverse->extendData);

						menuItemInfo[total].subcheat = i;
						menuItemInfo[total].fieldType = kType_SubtractMinimum;
						menuItem[total] = "Minimum Boundary";
						menuSubItem[total] = extendDataBuf[i];

						total++;
					}
					else
					{
						/* do add maximum field*/

						sprintf(extendDataBuf[i], "%.8X", traverse->extendData);

						menuItemInfo[total].subcheat = i;
						menuItemInfo[total].fieldType = kType_AddMaximum;
						menuItem[total] = "Maximum Boundary";
						menuSubItem[total] = extendDataBuf[i];

						total++;
					}
				}
			}

			if((operation == kOperation_ForceRange) && (locationType != kLocation_IndirectIndexed))
			{
				{
					/* do range minimum field*/

					sprintf(extendDataBuf[i], "%.2X", (traverse->extendData >> 8) & 0xFF);

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_RangeMinimum;
					menuItem[total] = "Range Minimum";
					menuSubItem[total] = extendDataBuf[i];

					total++;
				}

				{
					/* do range maximum field*/

					sprintf(extendDataBuf[i] + 3, "%.2X", (traverse->extendData >> 0) & 0xFF);

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_RangeMaximum;
					menuItem[total] = "Range Maximum";
					menuSubItem[total] = extendDataBuf[i] + 3;

					total++;
				}
			}

			if(operation == kOperation_SetOrClearBits)
			{
				/* do set/clear field*/

				menuItemInfo[total].subcheat = i;
				menuItemInfo[total].fieldType = kType_SetClear;
				menuItem[total] = "Set/Clear";
				menuSubItem[total] = kSetClearNames[operationParameter];

				total++;
			}

			if((operation != kOperation_None) || (type == kType_Watch))
			{
				UINT32	userSelect =		TEST_FIELD(traverse->type, UserSelectEnable);
				UINT32	bytesUsed =			EXTRACT_FIELD(traverse->type, BytesUsed);

				if(type != kType_Watch)
				{
					{
						/* do data field*/

						sprintf(dataBuf[i], "%.*X (%d)", (int)kCheatSizeDigitsTable[bytesUsed], traverse->originalDataField, traverse->originalDataField);

						menuItemInfo[total].subcheat = i;
						menuItemInfo[total].fieldType = kType_Data;
						menuItemInfo[total].extraData = kCheatSizeMaskTable[bytesUsed];
						menuItem[total] = "Data";
						menuSubItem[total] = dataBuf[i];

						total++;
					}

					{
						/* do user select field*/

						menuItemInfo[total].subcheat = i;
						menuItemInfo[total].fieldType = kType_UserSelect;
						menuItem[total] = "User Select";
						menuSubItem[total] = ui_getstring(userSelect ? UI_on : UI_off);

						total++;
					}

					if(userSelect)
					{
						{
							/* do user select minimum displayed value field*/

							menuItemInfo[total].subcheat = i;
							menuItemInfo[total].fieldType = kType_UserSelectMinimumDisp;
							menuItem[total] = "Minimum Displayed Value";
							menuSubItem[total] = kNumbersTable[EXTRACT_FIELD(traverse->type, UserSelectMinimumDisplay)];

							total++;
						}

						{
							/* do user select minimum value field*/

							menuItemInfo[total].subcheat = i;
							menuItemInfo[total].fieldType = kType_UserSelectMinimum;
							menuItem[total] = "Minimum Value";
							menuSubItem[total] = kNumbersTable[EXTRACT_FIELD(traverse->type, UserSelectMinimum)];

							total++;
						}

						{
							/* do user select BCD field*/

							menuItemInfo[total].subcheat = i;
							menuItemInfo[total].fieldType = kType_UserSelectBCD;
							menuItem[total] = "BCD";
							menuSubItem[total] = ui_getstring(TEST_FIELD(traverse->type, UserSelectBCD) ? UI_on : UI_off);

							total++;
						}

						{
							/* do prefill field*/

							menuItemInfo[total].subcheat = i;
							menuItemInfo[total].fieldType = kType_Prefill;
							menuItem[total] = "Prefill";
							menuSubItem[total] = kPrefillNames[EXTRACT_FIELD(traverse->type, Prefill)];

							total++;
						}
					}

					if(i > 0)
					{
						/* do copy previous value field*/

						menuItemInfo[total].subcheat = i;
						menuItemInfo[total].fieldType = kType_CopyPrevious;
						menuItem[total] = "Copy Previous Value";
						menuSubItem[total] = ui_getstring(TEST_FIELD(traverse->type, LinkCopyPreviousValue) ? UI_on : UI_off);

						total++;
					}
				}

				{
					/* do byte length field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_ByteLength;
					menuItem[total] = "Byte Length";
					menuSubItem[total] = kSizeNames[bytesUsed];

					total++;
				}

				if(bytesUsed > 0)
				{
					/* do endianness field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_Endianness;
					menuItem[total] = "Endianness";
					menuSubItem[total] = kEndiannessNames[EXTRACT_FIELD(traverse->type, Endianness)];

					total++;
				}

				{
					/* do location type field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_LocationType;
					menuItem[total] = "Location";

					if(locationType == kLocation_Custom)
						menuSubItem[total] = kCustomLocationNames[locationParameter];
					else
						menuSubItem[total] = kLocationNames[locationType];

					total++;
				}

				if((locationType == kLocation_Standard) || (locationType == kLocation_HandlerMemory))
				{
					/* do cpu field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_CPU;
					menuItem[total] = "CPU";
					menuSubItem[total] = kNumbersTable[locationParameter];

					total++;
				}

				if(locationType == kLocation_MemoryRegion)
				{
					/* do region field*/

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_Region;
					menuItem[total] = "Region";
					menuSubItem[total] = kRegionNames[locationParameter];

					total++;
				}

				if(locationType == kLocation_IndirectIndexed)
				{
					{
						/* do packed CPU field*/

						menuItemInfo[total].subcheat = i;
						menuItemInfo[total].fieldType = kType_PackedCPU;
						menuItem[total] = "CPU";
						menuSubItem[total] = kNumbersTable[(locationParameter >> 2) & 7];

						total++;
					}

					{
						/* do packed size field*/

						menuItemInfo[total].subcheat = i;
						menuItemInfo[total].fieldType = kType_PackedSize;
						menuItem[total] = "Address Size";
						menuSubItem[total] = kNumbersTable[(locationParameter & 3) + 1];

						total++;
					}

					{
						/* do address index field*/

						/* swap if negative*/
						if(traverse->extendData & 0x80000000)
						{
							int	temp = traverse->extendData;

							temp = -temp;

							sprintf(extendDataBuf[i], "-%.8X", temp);
						}
						else
						{
							sprintf(extendDataBuf[i], "%.8X", traverse->extendData);
						}

						menuItemInfo[total].subcheat = i;
						menuItemInfo[total].fieldType = kType_AddressIndex;
						menuItem[total] = "Address Index";
						menuSubItem[total] = extendDataBuf[i];

						total++;
					}
				}

				{
					/* do address field*/

					int	charsToPrint = 8;

					switch(EXTRACT_FIELD(traverse->type, LocationType))
					{
						case kLocation_Standard:
						case kLocation_HandlerMemory:
						{
							CPUInfo	* cpuInfo = &cpuInfoList[EXTRACT_FIELD(traverse->type, LocationParameter)];

							charsToPrint = cpuInfo->addressCharsNeeded;
							menuItemInfo[total].extraData = cpuInfo->addressMask;
						}
						break;

						case kLocation_IndirectIndexed:
						{
							CPUInfo	* cpuInfo = &cpuInfoList[(EXTRACT_FIELD(traverse->type, LocationParameter) >> 2) & 7];

							charsToPrint = cpuInfo->addressCharsNeeded;
							menuItemInfo[total].extraData = cpuInfo->addressMask;
						}
						break;

						default:
							menuItemInfo[total].extraData = 0xFFFFFFFF;
					}

					sprintf(addressBuf[i], "%.*X", charsToPrint, traverse->address);

					menuItemInfo[total].subcheat = i;
					menuItemInfo[total].fieldType = kType_Address;
					menuItem[total] = "Address";
					menuSubItem[total] = addressBuf[i];

					total++;
				}
			}
		}

		if(i < (entry->actionListLength - 1))
		{
			menuItemInfo[total].subcheat = i;
			menuItemInfo[total].fieldType = kType_Divider;
			menuItem[total] = "===";
			menuSubItem[total] = NULL;

			total++;
		}
	}

	menuItemInfo[total].subcheat =	0;
	menuItemInfo[total].fieldType =	kType_Return;
	menuItem[total] =				ui_getstring(UI_returntoprior);
	menuSubItem[total] =			NULL;
	total++;

	menuItemInfo[total].subcheat =	0;
	menuItemInfo[total].fieldType =	kType_Return;
	menuItem[total] =				NULL;
	menuSubItem[total] =			NULL;

	if(sel < 0)
		sel = 0;
	if(sel >= total)
		sel = total - 1;

	info = &menuItemInfo[sel];
	action = &entry->actionList[info->subcheat];

	if(editActive)
		flagBuf[sel] = 1;

	ui_displaymenu(bitmap, menuItem, menuSubItem, flagBuf, sel, 0);

	if(AltKeyPressed())
		increment <<= 4;
	if(ControlKeyPressed())
		increment <<= 8;
	if(ShiftKeyPressed())
		increment <<= 16;

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		editActive = 0;
		dirty = 1;

		switch(info->fieldType)
		{
			case kType_Name:
				currentNameTemplate--;

				if(currentNameTemplate < 0)
				{
					currentNameTemplate = 0;

					while(kCheatNameTemplates[currentNameTemplate + 1][0])
					{
						currentNameTemplate++;
					}
				}

				entry->name = realloc(entry->name, strlen(kCheatNameTemplates[currentNameTemplate]) + 1);
				strcpy(entry->name, kCheatNameTemplates[currentNameTemplate]);
				break;

			case kType_ActivationKey:
				entry->activationKey--;

				if(entry->activationKey < __code_key_first)
					entry->activationKey = __code_key_last;
				if(entry->activationKey > __code_key_last)
					entry->activationKey = __code_key_first;

				entry->flags |= kCheatFlag_HasActivationKey;
				break;

			case kType_Type:
			{
				UINT8	handled = 0;

				CLEAR_MASK_FIELD(action->type, OperationExtend);

				if(EXTRACT_FIELD(action->type, LocationType) == kLocation_Custom)
				{
					UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

					if(locationParameter == kCustomLocation_Comment)
					{
						SET_FIELD(action->type, LocationParameter, 0);
						SET_FIELD(action->type, LocationType, kLocation_Standard);
						SET_FIELD(action->type, Type, kType_Watch);
						SET_FIELD(action->type, Operation, kOperation_None);
						SET_MASK_FIELD(action->type, OperationExtend);

						handled = 1;
					}
					else if(locationParameter == kCustomLocation_Select)
					{
						SET_FIELD(action->type, LocationParameter, kCustomLocation_Comment);
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, Type, 0);

						handled = 1;
					}
				}

				if(!handled)
				{
					UINT32	type = EXTRACT_FIELD(action->type, Type);

					if(type == kType_NormalOrDelay)
					{
						SET_FIELD(action->type, LocationParameter, kCustomLocation_Select);
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, Type, 0);
					}
					else
					{
						SET_FIELD(action->type, Type, type - 1);
					}
				}
			}
			break;

			case kType_OneShot:
				TOGGLE_MASK_FIELD(action->type, OneShot);
				break;

			case kType_RestorePreviousValue:
				TOGGLE_MASK_FIELD(action->type, RestorePreviousValue);
				break;

			case kType_Delay:
			case kType_IgnoreDecrementBy:
			{
				UINT32	delay = (EXTRACT_FIELD(action->type, TypeParameter) - 1) & 7;

				SET_FIELD(action->type, TypeParameter, delay);
			}
			break;

			case kType_WatchSize:
				action->originalDataField = (action->originalDataField & 0xFFFFFF00) | ((action->originalDataField - 0x00000001) & 0x000000FF);
				action->data = action->originalDataField;
				break;

			case kType_WatchSkip:
				action->originalDataField = (action->originalDataField & 0xFFFF00FF) | ((action->originalDataField - 0x00000100) & 0x0000FF00);
				action->data = action->originalDataField;
				break;

			case kType_WatchPerLine:
				action->originalDataField = (action->originalDataField & 0xFF00FFFF) | ((action->originalDataField - 0x00010000) & 0x00FF0000);
				action->data = action->originalDataField;
				break;

			case kType_WatchAddValue:
				action->originalDataField = (action->originalDataField & 0x00FFFFFF) | ((action->originalDataField - 0x01000000) & 0xFF000000);
				action->data = action->originalDataField;
				break;

			case kType_WatchFormat:
			{
				UINT32	typeParameter = EXTRACT_FIELD(action->type, TypeParameter);

				typeParameter = (typeParameter & 0xFFFFFFFC) | ((typeParameter - 0x00000001) & 0x0000003);
				SET_FIELD(action->type, TypeParameter, typeParameter);
			}
			break;

			case kType_WatchLabel:
				SET_FIELD(action->type, TypeParameter, EXTRACT_FIELD(action->type, TypeParameter) ^ 0x00000004);
				break;

			case kType_Operation:
			{
				UINT32	operation = (EXTRACT_FIELD(action->type, Operation) - 1) & 7;

				CLEAR_MASK_FIELD(action->type, OperationExtend);
				SET_FIELD(action->type, Operation, operation);
			}
			break;

			case kType_WriteMask:
				action->extendData -= increment;
				action->extendData &= info->extraData;
				break;

			case kType_RangeMinimum:
				action->extendData = (action->extendData & 0xFFFF00FF) | ((action->extendData - 0x00000100) & 0x0000FF00);
				break;

			case kType_RangeMaximum:
				action->extendData = (action->extendData & 0xFFFFFF00) | ((action->extendData - 0x00000001) & 0x000000FF);
				break;

			case kType_AddressIndex:
			case kType_SubtractMinimum:
			case kType_AddMaximum:
				action->extendData -= increment;
				break;

			case kType_AddSubtract:
			case kType_SetClear:
				TOGGLE_MASK_FIELD(action->type, OperationParameter);
				break;

			case kType_Data:
				action->originalDataField -= increment;
				action->originalDataField &= info->extraData;
				action->data = action->originalDataField;
				break;

			case kType_UserSelect:
				TOGGLE_MASK_FIELD(action->type, UserSelectEnable);
				break;

			case kType_UserSelectMinimumDisp:
				TOGGLE_MASK_FIELD(action->type, UserSelectMinimumDisplay);
				break;

			case kType_UserSelectMinimum:
				TOGGLE_MASK_FIELD(action->type, UserSelectMinimum);
				break;

			case kType_UserSelectBCD:
				TOGGLE_MASK_FIELD(action->type, UserSelectBCD);
				break;

			case kType_Prefill:
				TOGGLE_MASK_FIELD(action->type, Prefill);
				break;

			case kType_CopyPrevious:
				TOGGLE_MASK_FIELD(action->type, LinkCopyPreviousValue);
				break;

			case kType_ByteLength:
			{
				UINT32	length = (EXTRACT_FIELD(action->type, BytesUsed) - 1) & 3;

				SET_FIELD(action->type, BytesUsed, length);
			}
			break;

			case kType_Endianness:
				TOGGLE_MASK_FIELD(action->type, Endianness);
				break;

			case kType_LocationType:
			{
				UINT32	locationType = EXTRACT_FIELD(action->type, LocationType);
				UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				if(locationType == kLocation_Standard)
				{
					SET_FIELD(action->type, LocationType, kLocation_IndirectIndexed);
					SET_FIELD(action->type, LocationParameter, (locationParameter << 2) & 0x1C);
				}
				else if(locationType == kLocation_Custom)
				{
					SET_FIELD(action->type, LocationType, kLocation_HandlerMemory);
					SET_FIELD(action->type, LocationParameter, 0);
				}
				else if(locationType == kLocation_IndirectIndexed)
				{
					SET_FIELD(action->type, LocationType, kLocation_Custom);
					SET_FIELD(action->type, LocationParameter, kCustomLocation_EEPROM);
				}
				else
				{
					locationType--;

					SET_FIELD(action->type, LocationType, locationType);
				}
			}
			break;

			case kType_CPU:
			case kType_Region:
			{
				UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				locationParameter = (locationParameter - 1) & 31;

				SET_FIELD(action->type, LocationParameter, locationParameter);
			}
			break;

			case kType_PackedCPU:
			{
				UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				locationParameter = ((locationParameter - 0x04) & 0x1C) | (locationParameter & 0x03);

				SET_FIELD(action->type, LocationParameter, locationParameter);
			}
			break;

			case kType_PackedSize:
			{
				UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				locationParameter = ((locationParameter - 0x01) & 0x03) | (locationParameter & 0x1C);

				SET_FIELD(action->type, LocationParameter, locationParameter);
			}
			break;

			case kType_Address:
				action->address -= increment;
				action->address &= info->extraData;
				break;

			case kType_Return:
			case kType_Divider:
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		editActive = 0;
		dirty = 1;

		switch(info->fieldType)
		{
			case kType_Name:
				currentNameTemplate++;

				if((currentNameTemplate < 0) || !kCheatNameTemplates[currentNameTemplate][0])
				{
					currentNameTemplate = 0;
				}

				entry->name = realloc(entry->name, strlen(kCheatNameTemplates[currentNameTemplate]) + 1);
				strcpy(entry->name, kCheatNameTemplates[currentNameTemplate]);
				break;

			case kType_ActivationKey:
				entry->activationKey++;

				if(entry->activationKey < __code_key_first)
					entry->activationKey = __code_key_last;
				if(entry->activationKey > __code_key_last)
					entry->activationKey = __code_key_first;

				entry->flags |= kCheatFlag_HasActivationKey;

				break;

			case kType_Type:
			{
				UINT8	handled = 0;

				CLEAR_MASK_FIELD(action->type, OperationExtend);

				if(EXTRACT_FIELD(action->type, LocationType) == kLocation_Custom)
				{
					UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

					if(locationParameter == kCustomLocation_Comment)
					{
						SET_FIELD(action->type, LocationParameter, kCustomLocation_Select);
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, Type, 0);

						handled = 1;
					}
					else if(locationParameter == kCustomLocation_Select)
					{
						SET_FIELD(action->type, LocationParameter, 0);
						SET_FIELD(action->type, LocationType, kLocation_Standard);
						SET_FIELD(action->type, Type, 0);

						handled = 1;
					}
				}

				if(!handled)
				{
					UINT32	type = EXTRACT_FIELD(action->type, Type);

					if(type == kType_Watch)
					{
						SET_FIELD(action->type, LocationParameter, kCustomLocation_Comment);
						SET_FIELD(action->type, LocationType, kLocation_Custom);
						SET_FIELD(action->type, Type, 0);
					}
					else
					{
						SET_FIELD(action->type, Type, type + 1);

						if((type + 1) == kType_Watch)
						{
							SET_FIELD(action->type, Operation, kOperation_None);
							SET_MASK_FIELD(action->type, OperationExtend);
						}
					}
				}
			}
			break;

			case kType_OneShot:
				TOGGLE_MASK_FIELD(action->type, OneShot);
				break;

			case kType_RestorePreviousValue:
				TOGGLE_MASK_FIELD(action->type, RestorePreviousValue);
				break;

			case kType_Delay:
			case kType_IgnoreDecrementBy:
			{
				UINT32	delay = (EXTRACT_FIELD(action->type, TypeParameter) + 1) & 7;

				SET_FIELD(action->type, TypeParameter, delay);
			}
			break;

			case kType_WatchSize:
				action->originalDataField = (action->originalDataField & 0xFFFFFF00) | ((action->originalDataField + 0x00000001) & 0x000000FF);
				action->data = action->originalDataField;
				break;

			case kType_WatchSkip:
				action->originalDataField = (action->originalDataField & 0xFFFF00FF) | ((action->originalDataField + 0x00000100) & 0x0000FF00);
				action->data = action->originalDataField;
				break;

			case kType_WatchPerLine:
				action->originalDataField = (action->originalDataField & 0xFF00FFFF) | ((action->originalDataField + 0x00010000) & 0x00FF0000);
				action->data = action->originalDataField;
				break;

			case kType_WatchAddValue:
				action->originalDataField = (action->originalDataField & 0x00FFFFFF) | ((action->originalDataField + 0x01000000) & 0xFF000000);
				action->data = action->originalDataField;
				break;

			case kType_WatchFormat:
			{
				UINT32	typeParameter = EXTRACT_FIELD(action->type, TypeParameter);

				typeParameter = (typeParameter & 0xFFFFFFFC) | ((typeParameter + 0x00000001) & 0x0000003);
				SET_FIELD(action->type, TypeParameter, typeParameter);
			}
			break;

			case kType_WatchLabel:
				SET_FIELD(action->type, TypeParameter, EXTRACT_FIELD(action->type, TypeParameter) ^ 0x00000004);
				break;

			case kType_Operation:
			{
				UINT32	operation = (EXTRACT_FIELD(action->type, Operation) + 1) & 7;

				CLEAR_MASK_FIELD(action->type, OperationExtend);
				SET_FIELD(action->type, Operation, operation);
			}
			break;

			case kType_WriteMask:
				action->extendData += increment;
				action->extendData &= info->extraData;
				break;

			case kType_RangeMinimum:
				action->extendData = (action->extendData & 0xFFFF00FF) | ((action->extendData + 0x00000100) & 0x0000FF00);
				break;

			case kType_RangeMaximum:
				action->extendData = (action->extendData & 0xFFFFFF00) | ((action->extendData + 0x00000001) & 0x000000FF);
				break;

			case kType_AddressIndex:
			case kType_SubtractMinimum:
			case kType_AddMaximum:
				action->extendData += increment;
				break;

			case kType_AddSubtract:
			case kType_SetClear:
				TOGGLE_MASK_FIELD(action->type, OperationParameter);
				break;

			case kType_Data:
				action->originalDataField += increment;
				action->originalDataField &= info->extraData;
				action->data = action->originalDataField;
				break;

			case kType_UserSelect:
				TOGGLE_MASK_FIELD(action->type, UserSelectEnable);
				break;

			case kType_UserSelectMinimumDisp:
				TOGGLE_MASK_FIELD(action->type, UserSelectMinimumDisplay);
				break;

			case kType_UserSelectMinimum:
				TOGGLE_MASK_FIELD(action->type, UserSelectMinimum);
				break;

			case kType_UserSelectBCD:
				TOGGLE_MASK_FIELD(action->type, UserSelectBCD);
				break;

			case kType_Prefill:
				TOGGLE_MASK_FIELD(action->type, Prefill);
				break;

			case kType_CopyPrevious:
				TOGGLE_MASK_FIELD(action->type, LinkCopyPreviousValue);
				break;

			case kType_ByteLength:
			{
				UINT32	length = (EXTRACT_FIELD(action->type, BytesUsed) + 1) & 3;

				SET_FIELD(action->type, BytesUsed, length);
			}
			break;

			case kType_Endianness:
				TOGGLE_MASK_FIELD(action->type, Endianness);
				break;

			case kType_LocationType:
			{
				UINT32	locationType = EXTRACT_FIELD(action->type, LocationType);
				UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				if(locationType == kLocation_IndirectIndexed)
				{
					SET_FIELD(action->type, LocationType, kLocation_Standard);
					SET_FIELD(action->type, LocationParameter, (locationParameter >> 2) & 7);
				}
				else if(locationType == kLocation_Custom)
				{
					SET_FIELD(action->type, LocationType, kLocation_IndirectIndexed);
					SET_FIELD(action->type, LocationParameter, 0);
				}
				else if(locationType == kLocation_HandlerMemory)
				{
					SET_FIELD(action->type, LocationType, kLocation_Custom);
					SET_FIELD(action->type, LocationParameter, kCustomLocation_EEPROM);
				}
				else
				{
					locationType++;

					SET_FIELD(action->type, LocationType, locationType);
				}
			}
			break;

			case kType_CPU:
			case kType_Region:
			{
				UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				locationParameter = (locationParameter + 1) & 31;

				SET_FIELD(action->type, LocationParameter, locationParameter);
			}
			break;

			case kType_PackedCPU:
			{
				UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				locationParameter = ((locationParameter + 0x04) & 0x1C) | (locationParameter & 0x03);

				SET_FIELD(action->type, LocationParameter, locationParameter);
			}
			break;

			case kType_PackedSize:
			{
				UINT32	locationParameter = EXTRACT_FIELD(action->type, LocationParameter);

				locationParameter = ((locationParameter + 0x01) & 0x03) | (locationParameter & 0x1C);

				SET_FIELD(action->type, LocationParameter, locationParameter);
			}
			break;

			case kType_Address:
				action->address += increment;
				action->address &= info->extraData;
				break;

			case kType_Return:
			case kType_Divider:
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;

		editActive = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;

		editActive = 0;
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(editActive)
		{
			editActive = 0;
		}
		else
		{
			switch(info->fieldType)
			{
				case kType_Name:
				case kType_ExtendName:
				case kType_Comment:
				case kType_ActivationKey:
				case kType_WatchSize:
				case kType_WatchSkip:
				case kType_WatchPerLine:
				case kType_WatchAddValue:
				case kType_WriteMask:
				case kType_AddMaximum:
				case kType_SubtractMinimum:
				case kType_RangeMinimum:
				case kType_RangeMaximum:
				case kType_Data:
				case kType_Address:
					osd_readkey_unicode(1);
					dirty = 1;
					editActive = 1;
					break;

				case kType_Return:
					sel = -1;
					break;
			}
		}
	}

	if(editActive)
	{
		/* do edit text*/

		dirty = 1;

		switch(info->fieldType)
		{
			case kType_Name:
				entry->name = DoDynamicEditTextField(entry->name);
				break;

			case kType_ExtendName:
				action->optionalName = DoDynamicEditTextField(action->optionalName);
				break;

			case kType_Comment:
				entry->comment = DoDynamicEditTextField(entry->comment);
				break;

			case kType_ActivationKey:
			{
				if(input_ui_pressed(IPT_UI_CANCEL))
				{
					entry->activationKey = 0;
					entry->flags &= ~kCheatFlag_HasActivationKey;

					editActive = 0;
				}
				else
				{
					int	code = code_read_async();

					if(code == KEYCODE_ESC)
					{
						entry->activationKey = 0;
						entry->flags &= ~kCheatFlag_HasActivationKey;

						editActive = 0;
					}
					else if(	(code != CODE_NONE) &&
								!input_ui_pressed(IPT_UI_SELECT))
					{
						entry->activationKey = code;
						entry->flags |= kCheatFlag_HasActivationKey;

						editActive = 0;
					}
				}
			}
			break;

			case kType_WatchSize:
			{
				UINT32	temp = (action->originalDataField >> 0) & 0xFF;

				temp++;
				temp = DoEditHexField(temp) & 0xFF;
				temp--;

				action->originalDataField = (action->originalDataField & 0xFFFFFF00) | ((temp << 0) & 0x000000FF);
				action->data = action->originalDataField;
			}
			break;

			case kType_WatchSkip:
			{
				UINT32	temp = (action->originalDataField >> 8) & 0xFF;

				temp = DoEditHexField(temp) & 0xFF;

				action->originalDataField = (action->originalDataField & 0xFFFF00FF) | ((temp << 8) & 0x0000FF00);
				action->data = action->originalDataField;
			}
			break;

			case kType_WatchPerLine:
			{
				UINT32	temp = (action->originalDataField >> 16) & 0xFF;

				temp = DoEditHexField(temp) & 0xFF;

				action->originalDataField = (action->originalDataField & 0xFF00FFFF) | ((temp << 16) & 0x00FF0000);
				action->data = action->originalDataField;
			}
			break;

			case kType_WatchAddValue:
			{
				UINT32	temp = (action->originalDataField >> 24) & 0xFF;

				temp = DoEditHexFieldSigned(temp, 0xFFFFFF80) & 0xFF;

				action->originalDataField = (action->originalDataField & 0x00FFFFFF) | ((temp << 24) & 0xFF000000);
				action->data = action->originalDataField;
			}
			break;

			case kType_WriteMask:
				action->extendData = DoEditHexField(action->extendData);
				action->extendData &= info->extraData;
				break;

			case kType_AddMaximum:
			case kType_SubtractMinimum:
				action->extendData = DoEditHexField(action->extendData);
				break;

			case kType_RangeMinimum:
			{
				UINT32	temp;

				temp = (action->extendData >> 8) & 0xFF;

				temp = DoEditHexField(temp) & 0xFF;

				action->extendData = (action->extendData & 0x00FF) | ((temp << 8) & 0xFF00);
			}
			break;

			case kType_RangeMaximum:
			{
				UINT32	temp;

				temp = action->extendData & 0xFF;

				temp = DoEditHexField(temp) & 0xFF;

				action->extendData = (action->extendData & 0xFF00) | (temp & 0x00FF);
			}
			break;

			case kType_Data:
				action->originalDataField = DoEditHexField(action->originalDataField);
				action->originalDataField &= info->extraData;
				action->data = action->originalDataField;
				break;

			case kType_Address:
				action->address = DoEditHexField(action->address);
				action->address &= info->extraData;
				break;
		}

		if(input_ui_pressed(IPT_UI_CANCEL))
		{
			editActive = 0;
		}
	}
	else
	{
		if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
		{
			SaveCheat(entry);
		}

		if(input_ui_pressed(IPT_UI_WATCH_VALUE))
		{
			WatchCheatEntry(entry, 0);
		}

		if(input_ui_pressed(IPT_UI_ADD_CHEAT))
		{
			AddActionBefore(entry, info->subcheat);
		}

		if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
		{
			DeleteActionAt(entry, info->subcheat);
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
	{
		sel = -1;
		editActive = 0;
	}

	if(input_ui_pressed(IPT_UI_CONFIGURE))
	{
		sel = -2;
		editActive = 0;
	}

	if(	(sel == -1) ||
		(sel == -2))
	{
		editActive = 0;
		dirty = 1;
		schedule_full_refresh();
	}

	if(dirty)
	{
		UpdateCheatInfo(entry, 0);

		entry->flags |= kCheatFlag_Dirty;
	}

	return sel + 1;
}

static int DoSearchMenuClassic(struct mame_bitmap * bitmap, int selection, int startNew)
{
	const char * energyStrings[] =
	{
		"Equal",
		"Less",
		"Greater",
		"Less or Equal",
		"Greater or Equal",
		"Not Equal"
	};

	const char * bitStrings[] =
	{
		"Equal",
		"Not Equal"
	};

	enum
	{
		kMenu_CPU = 0,
		kMenu_Value,
		kMenu_Time,
		kMenu_Energy,
		kMenu_Bit,
		kMenu_Slow,
		kMenu_Return,

		kMenu_Max
	};

	INT32			sel = selection - 1;
	const char		* menu_item[kMenu_Max + 2] = { 0 };
	const char		* menu_subitem[kMenu_Max + 2] = { 0 };
	/*char			flagBuf[kMenu_Max + 2] = { 0 };*/
	char			valueBuffer[60];
	char			valueSignedBuffer[60];
	char			cpuBuffer[20];
	INT32			total = kMenu_Max;
	static int		lastPos = 0;

	SearchInfo		* search = GetCurrentSearch();

	/*static UINT8	editActive = 0;*/
	UINT32			increment = 1;
	UINT8			doSearch = 0;
	UINT8			willHaveResults = 0;

	sel = lastPos;

	sprintf(cpuBuffer, "%d", search->targetIdx);
	menu_item[kMenu_CPU] =			ui_getstring(UI_cpu);
	menu_subitem[kMenu_CPU] =		cpuBuffer;

	if(search->sign && (search->oldOptions.value & kSearchByteSignBitTable[search->bytes]))
	{
		UINT32	tempValue;

		tempValue = ~search->oldOptions.value + 1;
		tempValue &= kSearchByteUnsignedMaskTable[search->bytes];

		sprintf(valueBuffer, "-%.*X (-%d)", kSearchByteDigitsTable[search->bytes], tempValue, tempValue);
	}
	else
	{
		sprintf(valueBuffer, "%.*X (%d)", kSearchByteDigitsTable[search->bytes], search->oldOptions.value & kSearchByteMaskTable[search->bytes], search->oldOptions.value & kSearchByteMaskTable[search->bytes]);
	}

	menu_item[kMenu_Value] =		ui_getstring(UI_search_lives);
	menu_subitem[kMenu_Value] =		valueBuffer;

	menu_item[kMenu_Time] =			ui_getstring(UI_search_timers);
	menu_subitem[kMenu_Time] =		NULL;

	menu_item[kMenu_Energy] =		ui_getstring(UI_search_energy);
	menu_subitem[kMenu_Energy] =	NULL;

	menu_item[kMenu_Bit] =			ui_getstring(UI_search_status);
	menu_subitem[kMenu_Bit] =		NULL;

	menu_item[kMenu_Slow] =			ui_getstring(UI_search_slow);
	menu_subitem[kMenu_Slow] =		NULL;

	menu_item[kMenu_Return] =		ui_getstring(UI_returntoprior);
	menu_subitem[kMenu_Return] =	NULL;

	menu_item[kMenu_Max] =			NULL;
	menu_subitem[kMenu_Max] =		NULL;

	if(!startNew)
	{
		if(search->oldOptions.delta & kSearchByteSignBitTable[search->bytes])
		{
			UINT32	tempValue;

			tempValue = ~search->oldOptions.delta + 1;
			tempValue &= kSearchByteUnsignedMaskTable[search->bytes];

			sprintf(valueSignedBuffer, "-%.*X (-%d)", kSearchByteDigitsTable[search->bytes], tempValue, tempValue);
		}
		else
		{
			sprintf(valueSignedBuffer, "%.*X (%d)", kSearchByteDigitsTable[search->bytes], search->oldOptions.delta & kSearchByteMaskTable[search->bytes], search->oldOptions.delta & kSearchByteMaskTable[search->bytes]);
		}

		menu_subitem[kMenu_Time] =		valueSignedBuffer;
		menu_subitem[kMenu_Energy] =	energyStrings[search->oldOptions.energy];
		menu_subitem[kMenu_Bit] =		bitStrings[search->oldOptions.status];
		menu_subitem[kMenu_Slow] =		bitStrings[search->oldOptions.slow];
	}

	ui_displaymenu(bitmap, menu_item, menu_subitem, 0, sel, 0);

	if(AltKeyPressed())
		increment <<= 4;
	if(ControlKeyPressed())
		increment <<= 8;
	if(ShiftKeyPressed())
		increment <<= 16;

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case kMenu_CPU:
				if(search->targetIdx > 0)
				{
					search->targetIdx--;

					BuildSearchRegions(search);
					AllocateSearchRegions(search);
				}
				break;

			case kMenu_Value:
				search->oldOptions.value -= increment;

				search->oldOptions.value &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_Time:
				search->oldOptions.delta -= increment;

				search->oldOptions.delta &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_Energy:
				if(search->oldOptions.energy < kEnergy_Max)
					search->oldOptions.energy++;
				else
					search->oldOptions.energy = kEnergy_Equals;
				break;

			case kMenu_Bit:
				search->oldOptions.status ^= 1;
				break;

			case kMenu_Slow:
				search->oldOptions.slow ^= 1;
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case kMenu_CPU:
				if(search->targetIdx < cpu_gettotalcpu() - 1)
				{
					search->targetIdx++;

					BuildSearchRegions(search);
					AllocateSearchRegions(search);
				}
				break;

			case kMenu_Value:
				search->oldOptions.value += increment;

				search->oldOptions.value &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_Time:
				search->oldOptions.delta += increment;

				search->oldOptions.delta &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_Energy:
				if(search->oldOptions.energy > kEnergy_Equals)
					search->oldOptions.energy--;
				else
					search->oldOptions.energy = kEnergy_Max;
				break;

			case kMenu_Bit:
				search->oldOptions.status ^= 1;
				break;

			case kMenu_Slow:
				search->oldOptions.slow ^= 1;
				break;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		switch(sel)
		{
			case kMenu_Value:
				search->bytes =			kSearchSize_8Bit;
				search->lhs =			kSearchOperand_Current;
				search->rhs =			kSearchOperand_Value;
				search->comparison =	kSearchComparison_NearTo;
				search->value =			search->oldOptions.value;

				doSearch = 1;
				willHaveResults = 1;
				break;

			case kMenu_Time:
				search->bytes =			kSearchSize_8Bit;
				search->lhs =			kSearchOperand_Current;
				search->rhs =			kSearchOperand_Previous;
				search->comparison =	kSearchComparison_IncreasedBy;
				search->value =			search->oldOptions.delta;

				doSearch = 1;
				break;

			case kMenu_Energy:
				search->bytes =			kSearchSize_8Bit;
				search->lhs =			kSearchOperand_Current;
				search->rhs =			kSearchOperand_Previous;
				search->comparison =	kOldEnergyComparisonTable[search->oldOptions.energy];

				doSearch = 1;
				break;

			case kMenu_Bit:
				search->bytes =			kSearchSize_1Bit;
				search->lhs =			kSearchOperand_Current;
				search->rhs =			kSearchOperand_Previous;
				search->comparison =	kOldStatusComparisonTable[search->oldOptions.status];

				doSearch = 1;
				break;

			case kMenu_Slow:
				search->bytes =			kSearchSize_8Bit;
				search->lhs =			kSearchOperand_Current;
				search->rhs =			kSearchOperand_First;
				search->comparison =	kOldStatusComparisonTable[search->oldOptions.slow];

				doSearch = 1;
				break;

			case kMenu_Return:
				sel = -1;
				break;
		}
	}

	if(doSearch)
	{
		if(startNew)
		{
			InitializeNewSearch(search);
		}

		if(	(!kSearchOperandNeedsInit[search->lhs] && !kSearchOperandNeedsInit[search->rhs]) ||
			willHaveResults ||
			!startNew)
		{
			BackupSearch(search);

			DoSearch(search);
		}

		UpdateSearch(search);

		if(willHaveResults || !startNew)
			usrintf_showmessage("%d results found", search->numResults);
		else
			usrintf_showmessage("saved all memory regions");

		if(search->numResults == 1)
		{
			AddCheatFromFirstResult(search);

			usrintf_showmessage("1 result found, added to list");
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(sel == kMenu_Value)
	{
		search->oldOptions.value = DoEditHexField(search->oldOptions.value);

		search->oldOptions.value &= kSearchByteMaskTable[search->bytes];
	}
	else if(sel == kMenu_Time)
	{
		search->oldOptions.delta = DoEditHexField(search->oldOptions.delta);

		search->oldOptions.delta &= kSearchByteMaskTable[search->bytes];
	}

	if(	(sel == -1) ||
		(sel == -2))
		schedule_full_refresh();
	else
		lastPos = sel;

	return sel + 1;
}

static int DoSearchMenu(struct mame_bitmap * bitmap, int selection, int startNew)
{
	enum
	{
		kMenu_LHS,
		kMenu_Comparison,
		kMenu_RHS,
		kMenu_Value,

		kMenu_Divider,

		kMenu_Size,
		kMenu_Swap,
		kMenu_Sign,
		kMenu_CPU,
		kMenu_Name,

		kMenu_Divider2,

		kMenu_DoSearch,
		kMenu_SaveMemory,
		kMenu_Return,

		kMenu_Max
	};

	INT32			sel = selection - 1;
	static INT32	submenuChoice = 0;
	const char		* menu_item[kMenu_Max + 2] =	{ 0 };
	const char		* menu_subitem[kMenu_Max + 2] =	{ 0 };
	char			flagBuf[kMenu_Max + 2] = { 0 };
	char			valueBuffer[20];
	char			cpuBuffer[20];
	SearchInfo		* search = GetCurrentSearch();
	INT32			total = 0;
	UINT32			increment = 1;
	static UINT8	editActive = 0;
	static int		lastSel = 0;

	sel = lastSel;

	if(	(search->sign || search->comparison == kSearchComparison_IncreasedBy) &&
		(search->value & kSearchByteSignBitTable[search->bytes]))
	{
		UINT32	tempValue;

		tempValue = ~search->value + 1;
		tempValue &= kSearchByteUnsignedMaskTable[search->bytes];

		sprintf(valueBuffer, "-%.*X", kSearchByteDigitsTable[search->bytes], tempValue);
	}
	else
	{
		sprintf(valueBuffer, "%.*X", kSearchByteDigitsTable[search->bytes], search->value & kSearchByteMaskTable[search->bytes]);
	}

	if(dontPrintNewLabels)
	{
		menu_item[total] = kOperandNameTable[search->lhs];
		menu_subitem[total] = NULL;
		total++;

		menu_item[total] = kComparisonNameTable[search->comparison];
		menu_subitem[total] = NULL;
		total++;

		menu_item[total] = kOperandNameTable[search->rhs];
		menu_subitem[total] = NULL;
		total++;

		menu_item[total] = valueBuffer;
		menu_subitem[total] = NULL;
		total++;
	}
	else
	{
		menu_item[total] = "LHS";
		menu_subitem[total] = kOperandNameTable[search->lhs];
		total++;

		menu_item[total] = "Comparison";
		menu_subitem[total] = kComparisonNameTable[search->comparison];
		total++;

		menu_item[total] = "RHS";
		menu_subitem[total] = kOperandNameTable[search->rhs];
		total++;

		menu_item[total] = "Value";
		menu_subitem[total] = valueBuffer;
		total++;
	}

	menu_item[total] = "---";
	menu_subitem[total] = NULL;
	total++;

	menu_item[total] = "Size";
	menu_subitem[total] = kSearchByteNameTable[search->bytes];
	total++;

	menu_item[total] = "Swap";
	menu_subitem[total] = ui_getstring(search->swap ? UI_on : UI_off);
	total++;

	menu_item[total] = "Signed";
	menu_subitem[total] = ui_getstring(search->sign ? UI_on : UI_off);
	total++;

	sprintf(cpuBuffer, "%d", search->targetIdx);
	menu_item[total] = "CPU";
	menu_subitem[total] = cpuBuffer;
	total++;

	menu_item[total] = "Name";
	if(search->name)
		menu_subitem[total] = search->name;
	else
		menu_subitem[total] = "(none)";
	total++;

	menu_item[total] = "---";
	menu_subitem[total] = NULL;
	total++;

	menu_item[total] = "Do Search";
	menu_subitem[total] = NULL;
	total++;

	menu_item[total] = "Save Memory";
	menu_subitem[total] = NULL;
	total++;

	menu_item[total] = ui_getstring(UI_returntoprior);
	menu_subitem[total] = NULL;
	total++;

	menu_item[total] = NULL;
	menu_subitem[total] = NULL;

	if(sel < 0)
		sel = 0;
	if(sel >= total)
		sel = total - 1;

	if(editActive)
		flagBuf[sel] = 1;

	ui_displaymenu(bitmap, menu_item, menu_subitem, flagBuf, sel, 0);

	if(AltKeyPressed())
		increment <<= 4;
	if(ControlKeyPressed())
		increment <<= 8;
	if(ShiftKeyPressed())
		increment <<= 16;

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case kMenu_Value:
				search->value -= increment;

				search->value &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_LHS:
				search->lhs--;

				if(search->lhs < kSearchOperand_Current)
					search->lhs = kSearchOperand_Max;
				break;

			case kMenu_RHS:
				search->rhs--;

				if(search->rhs < kSearchOperand_Current)
					search->rhs = kSearchOperand_Max;
				break;

			case kMenu_Comparison:
				search->comparison--;

				if(search->comparison < kSearchComparison_LessThan)
					search->comparison = kSearchComparison_Max;
				break;

			case kMenu_Size:
				search->bytes--;

				if(search->bytes < kSearchSize_8Bit)
					search->bytes = kSearchSize_Max;
				break;

			case kMenu_Swap:
				search->swap ^= 1;
				break;

			case kMenu_Sign:
				search->sign ^= 1;
				break;

			case kMenu_CPU:
				if(search->targetIdx > 0)
				{
					search->targetIdx--;

					BuildSearchRegions(search);
					AllocateSearchRegions(search);
				}
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalFastKeyRepeatRate))
	{
		switch(sel)
		{
			case kMenu_Value:
				search->value += increment;

				search->value &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_Size:
				search->bytes++;

				if(search->bytes > kSearchSize_Max)
					search->bytes = kSearchSize_8Bit;
				break;

			case kMenu_LHS:
				search->lhs++;

				if(search->lhs > kSearchOperand_Max)
					search->lhs = kSearchOperand_Current;
				break;

			case kMenu_RHS:
				search->rhs++;

				if(search->rhs > kSearchOperand_Max)
					search->rhs = kSearchOperand_Current;
				break;

			case kMenu_Comparison:
				search->comparison++;

				if(search->comparison > kSearchComparison_Max)
					search->comparison = kSearchComparison_LessThan;
				break;

			case kMenu_Swap:
				search->swap ^= 1;
				break;

			case kMenu_Sign:
				search->sign ^= 1;
				break;

			case kMenu_CPU:
				if(search->targetIdx < cpu_gettotalcpu() - 1)
				{
					search->targetIdx++;

					BuildSearchRegions(search);
					AllocateSearchRegions(search);
				}
				break;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(editActive)
		{
			editActive = 0;
		}
		else
		{
			switch(sel)
			{
				case kMenu_Value:
				case kMenu_Name:
					editActive = 1;
					break;

				case kMenu_Return:
					submenuChoice = 0;
					sel = -1;
					break;

				case kMenu_DoSearch:
					if(startNew)
					{
						InitializeNewSearch(search);
					}

					if(	(!kSearchOperandNeedsInit[search->lhs] && !kSearchOperandNeedsInit[search->rhs]) ||
						!startNew)
					{
						BackupSearch(search);

						DoSearch(search);
					}

					UpdateSearch(search);

					usrintf_showmessage("%d results found", search->numResults);

					if(search->numResults == 1)
					{
						AddCheatFromFirstResult(search);

						usrintf_showmessage("1 result found, added to list");
					}
					break;

				case kMenu_SaveMemory:
					if(startNew)
					{
						InitializeNewSearch(search);
					}

					UpdateSearch(search);

					usrintf_showmessage("saved all memory regions");
					break;
			}
		}
	}

	if(editActive)
	{
		switch(sel)
		{
			case kMenu_Value:
				search->value = DoEditHexField(search->value);

				search->value &= kSearchByteMaskTable[search->bytes];
				break;

			case kMenu_Name:
				search->name = DoDynamicEditTextField(search->name);
				break;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(	(sel == -1) ||
		(sel == -2))
		schedule_full_refresh();
	else
		lastSel = sel;

	return sel + 1;
}

static int AddEditCheatMenu(struct mame_bitmap * bitmap, int selection)
{
	INT32			sel;
	static INT32	submenuChoice = 0;
	static INT32	submenuCheat = 0;
	const char		** menu_item;
	INT32			i;
	INT32			total = 0;
	CheatEntry		* entry;

	sel = selection - 1;

	RequestStrings(cheatListLength + 2, 0, 0, 0);

	menu_item = menuStrings.mainList;

	if(submenuChoice)
	{
		submenuChoice = EditCheatMenu(bitmap, &cheatList[submenuCheat], submenuChoice);

		if(submenuChoice == -1)
		{
			submenuChoice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	for(i = 0; i < cheatListLength; i++)
	{
		CheatEntry	* traverse = &cheatList[i];

		if(traverse->name)
			menu_item[total] = traverse->name;
		else
			menu_item[total] = "(none)";

		total++;
	}

	menu_item[total] = ui_getstring(UI_returntoprior);
	total++;

	menu_item[total] = NULL;

	if(sel < 0)
		sel = 0;
	if(sel >= total)
		sel = total - 1;

	ui_displaymenu(bitmap, menu_item, NULL, NULL, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(sel < (total - 1))
		entry = &cheatList[sel];
	else
		entry = NULL;

	if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			for(i = 0; i < cheatListLength; i++)
				SaveCheat(&cheatList[i]);

			usrintf_showmessage_secs(1, "%d cheats saved", cheatListLength);
		}
		else
		{
			SaveCheat(entry);
		}
	}

	if(input_ui_pressed(IPT_UI_ADD_CHEAT))
	{
		AddCheatBefore(sel);
	}

	if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
	{
		DeleteCheatAt(sel);
	}

	if(input_ui_pressed(IPT_UI_WATCH_VALUE))
	{
		WatchCheatEntry(entry, 0);
	}

	if(input_ui_pressed(IPT_UI_EDIT_CHEAT))
	{
		if(sel < (total - 1))
		{
			submenuCheat = sel;
			submenuChoice = 1;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == (total - 1))
		{
			submenuChoice = 0;
			sel = -1;
		}
		else if(sel < (total - 1))
		{
			submenuCheat = sel;
			submenuChoice = 1;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(	(sel == -1) ||
		(sel == -2))
		schedule_full_refresh();

	return sel + 1;
}

static int ViewSearchResults(struct mame_bitmap * bitmap, int selection, int firstTime)
{
	enum
	{
		kMenu_Header = 0,
		kMenu_FirstResult,

		kMaxResultsPerPage = 100
	};

	INT32			sel;
	const char		** menu_item;
	char			** buf;
	char			* header;
	INT32			total = 0;
	SearchInfo		* search = GetCurrentSearch();
	SearchRegion	* region;
	INT32			numPages;
	INT32			resultsPerPage;
	INT32			i;
	UINT32			traverse;
	UINT8			hadResults = 0;
	INT32			numToSkip;
	UINT32			resultsFound = 0;
	UINT32			selectedAddress = 0;
	UINT32			selectedOffset = 0;
	UINT8			selectedAddressGood = 0;
	int				goToNextPage = 0;
	int				goToPrevPage = 0;

	RequestStrings(kMaxResultsPerPage + 3, kMaxResultsPerPage + 3, 80, 0);

	menu_item = menuStrings.mainList;
	buf = &menuStrings.mainStrings[1];
	header = menuStrings.mainStrings[0];

	sel = selection - 1;

	if(firstTime)
	{
		search->currentRegionIdx = 0;
		search->currentResultsPage = 0;

		for(traverse = 0; traverse < search->regionListLength; traverse++)
		{
			region = &search->regionList[traverse];

			if(region->numResults)
			{
				search->currentRegionIdx = traverse;
				break;
			}
		}
	}

	if(search->currentRegionIdx >= search->regionListLength)
		search->currentRegionIdx = search->regionListLength - 1;
	if(search->currentRegionIdx < 0)
		search->currentRegionIdx = 0;

	region = &search->regionList[search->currentRegionIdx];

	resultsPerPage = fullMenuPageHeight - 3;

	if(resultsPerPage <= 0)
		resultsPerPage = 1;

	if(region->flags & kRegionFlag_Enabled)
		numPages = (region->numResults / kSearchByteIncrementTable[search->bytes] + resultsPerPage - 1) / resultsPerPage;
	else
		numPages = 0;

	if(search->currentResultsPage >= numPages)
		search->currentResultsPage = numPages - 1;
	if(search->currentResultsPage < 0)
		search->currentResultsPage = 0;

	numToSkip = resultsPerPage * search->currentResultsPage;

	sprintf(header, "%s %d/%d", region->name, search->currentResultsPage + 1, numPages);

	menu_item[total] = header;
	total++;

	traverse = 0;

	if(	(region->length < kSearchByteIncrementTable[search->bytes]) ||
		!(region->flags & kRegionFlag_Enabled))
	{
		/* no results...*/
	}
	else
	{
		for(i = 0; (i < resultsPerPage) && (traverse < region->length) && (resultsFound < region->numResults);)
		{
			while(	!IsRegionOffsetValid(search, region, traverse) &&
					(traverse < region->length))
			{
				traverse += kSearchByteIncrementTable[search->bytes];
			}

			if(traverse < region->length)
			{
				if(numToSkip > 0)
				{
					numToSkip--;
				}
				else
				{
					if(total == sel)
					{
						selectedAddress =		region->address + traverse;
						selectedOffset =		traverse;
						selectedAddressGood =	1;
					}

					sprintf(	buf[total],
								"%.8X (%.*X %.*X %.*X)",
								region->address + traverse,
								kSearchByteDigitsTable[search->bytes],
								ReadSearchOperand(kSearchOperand_First, search, region, region->address + traverse),
								kSearchByteDigitsTable[search->bytes],
								ReadSearchOperand(kSearchOperand_Previous, search, region, region->address + traverse),
								kSearchByteDigitsTable[search->bytes],
								ReadSearchOperand(kSearchOperand_Current, search, region, region->address + traverse));

					menu_item[total] = buf[total];
					total++;

					i++;
				}

				traverse += kSearchByteIncrementTable[search->bytes];
				resultsFound++;
				hadResults = 1;
			}
		}
	}

	if(!hadResults)
	{
		if(search->numResults)
			menu_item[total] = "no results for this region";
		else
			menu_item[total] = "no results found";

		total++;
	}

	menu_item[total] = ui_getstring(UI_returntoprior);
	total++;

	menu_item[total] = NULL;

	if(sel < 0)
		sel = 0;
	if(sel > (total - 1))
		sel = total - 1;

	ui_displaymenu(bitmap, menu_item, NULL, NULL, sel, 0);

	if(code_pressed_memory(KEYCODE_END))
	{
		search->currentResultsPage = numPages - 1;
	}

	if(code_pressed_memory(KEYCODE_HOME))
	{
		search->currentResultsPage = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
		{
			goToNextPage = 1;

			sel = 0;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
		{
			goToPrevPage = 1;

			sel = total - 1;
		}
	}

	if(goToNextPage)
	{
		search->currentResultsPage++;

		if(search->currentResultsPage >= numPages)
		{
			search->currentResultsPage = 0;
			search->currentRegionIdx++;

			if(search->currentRegionIdx >= search->regionListLength)
			{
				search->currentRegionIdx = 0;
			}

			for(traverse = search->currentRegionIdx; traverse < search->regionListLength; traverse++)
			{
				if(search->regionList[traverse].numResults)
				{
					search->currentRegionIdx = traverse;
					break;
				}
			}
		}
	}

	if(goToPrevPage)
	{
		search->currentResultsPage--;

		if(search->currentResultsPage < 0)
		{
			search->currentResultsPage = 0;
			search->currentRegionIdx--;

			if(search->currentRegionIdx < 0)
			{
				search->currentRegionIdx = search->regionListLength - 1;
			}

			for(i = search->currentRegionIdx; i >= 0; i--)
			{
				if(search->regionList[i].numResults)
				{
					search->currentRegionIdx = i;
					break;
				}
			}

			{
				SearchRegion	* newRegion = &search->regionList[search->currentRegionIdx];
				UINT32			nextNumPages = (newRegion->numResults / kSearchByteIncrementTable[search->bytes] + resultsPerPage - 1) / resultsPerPage;

				if(nextNumPages <= 0)
					nextNumPages = 1;

				search->currentResultsPage = nextNumPages - 1;
			}
		}
	}

	if(selectedAddressGood)
	{
		if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
		{
			/**/
		}

		if(input_ui_pressed(IPT_UI_WATCH_VALUE))
		{
			AddWatchFromResult(search, region, selectedAddress);
		}

		if(input_ui_pressed(IPT_UI_ADD_CHEAT))
		{
			AddCheatFromResult(search, region, selectedAddress);
		}

		if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
		{
			InvalidateRegionOffset(search, region, selectedOffset);
			search->numResults--;
			region->numResults--;
		}
	}

	if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			if(region && search)
			{
				InvalidateEntireRegion(search, region);

				usrintf_showmessage_secs(1, "region invalidated - %d results remain", search->numResults);
			}
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == total - 1)
		{
			sel = -1;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(	(sel == -1) ||
		(sel == -2))
		schedule_full_refresh();

	return sel + 1;
}

static int ChooseWatch(struct mame_bitmap * bitmap, int selection)
{
	INT32			sel;
	static INT32	submenuChoice = 0;
	static INT32	submenuWatch = 0;
	WatchInfo		* watch;
	const char		** menuItem;
	char			** buf;
	INT32			total = 0;
	int				i;

	RequestStrings(watchListLength + 2, watchListLength, 30, 0);

	menuItem = menuStrings.mainList;
	buf = menuStrings.mainStrings;

	sel = selection - 1;

	if(submenuChoice)
	{
		submenuChoice = EditWatch(bitmap, &watchList[submenuWatch], submenuChoice);

		if(submenuChoice == -1)
		{
			submenuChoice = 0;
			sel = -2;
		}

		return sel + 1;
	}

	for(i = 0; i < watchListLength; i++)
	{
		WatchInfo	* traverse = &watchList[i];

		sprintf(buf[i], "%d:%.*X (%d)", traverse->cpu, cpuInfoList[traverse->cpu].addressCharsNeeded, traverse->address, traverse->numElements);

		menuItem[total] = buf[i];
		total++;
	}

	menuItem[total] = ui_getstring(UI_returntoprior);
	total++;

	menuItem[total] = NULL;

	if(sel < 0)
		sel = 0;
	if(sel >= total)
		sel = total - 1;

	if(sel < watchListLength)
		watch = &watchList[sel];
	else
		watch = NULL;

	ui_displaymenu(bitmap, menuItem, NULL, NULL, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(ShiftKeyPressed())
	{
		if(input_ui_pressed(IPT_UI_ADD_CHEAT))
		{
			AddWatchBefore(sel);
		}

		if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
		{
			DeleteWatchAt(sel);
		}
	}
	else
	{
		if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
		{
			if(watch)
			{
				CheatEntry	entry;

				memset(&entry, 0, sizeof(CheatEntry));

				SetupCheatFromWatchAsWatch(&entry, watch);
				SaveCheat(&entry);
				DisposeCheat(&entry);
			}
		}

		if(input_ui_pressed(IPT_UI_ADD_CHEAT))
		{
			if(watch)
			{
				if(ShiftKeyPressed())
				{
					CheatEntry	* entry = GetNewCheat();

					DisposeCheat(entry);
					SetupCheatFromWatchAsWatch(entry, watch);
				}
				else
				{
					AddCheatFromWatch(watch);
				}
			}
		}

		if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
		{
			if(ControlKeyPressed())
			{
				for(i = 0; i < watchListLength; i++)
				{
					watchList[i].numElements = 0;
				}
			}
			else
			{
				if(watch)
				{
					watch->numElements = 0;
				}
			}
		}
	}

	if(input_ui_pressed(IPT_UI_EDIT_CHEAT))
	{
		if(sel < (total - 1))
		{
			submenuWatch = sel;
			submenuChoice = 1;
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel == (total - 1))
		{
			submenuChoice = 0;
			sel = -1;
		}
		else if(sel < (total - 1))
		{
			submenuWatch = sel;
			submenuChoice = 1;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(	(sel == -1) ||
		(sel == -2))
		schedule_full_refresh();

	return sel + 1;
}

static int EditWatch(struct mame_bitmap * bitmap, WatchInfo * entry, int selection)
{
	enum
	{
		kMenu_Address = 0,
		kMenu_CPU,
		kMenu_NumElements,
		kMenu_ElementSize,
		kMenu_LabelType,
		kMenu_TextLabel,
		kMenu_DisplayType,
		kMenu_XPosition,
		kMenu_YPosition,
		kMenu_Skip,
		kMenu_ElementsPerLine,
		kMenu_AddValue,
		kMenu_AddressShift,
		kMenu_DataShift,
		kMenu_XOR,

		kMenu_Return
	};

	const char *	kWatchSizeStringList[] =
	{
		"8 Bit",
		"16 Bit",
		"32 Bit"
	};

	INT32			sel;
	const char		** menuItem;
	const char		** menuSubItem;
	char			** buf;
	char			* flagBuf;
	INT32			total = 0;
	UINT32			increment = 1;
	static UINT8	editActive = 0;

	if(!entry)
		return 0;

	RequestStrings(kMenu_Return + 2, kMenu_Return, 0, 20);

	menuItem = menuStrings.mainList;
	menuSubItem = menuStrings.subList;
	buf = menuStrings.subStrings;
	flagBuf = menuStrings.flagList;

	memset(flagBuf, 0, kMenu_Return + 2);

	sel = selection - 1;

	sprintf(buf[total], "%.*X", cpuInfoList[entry->cpu].addressCharsNeeded, entry->address >> entry->addressShift);
	menuItem[total] = "Address";
	menuSubItem[total] = buf[total];
	total++;

	sprintf(buf[total], "%d", entry->cpu);
	menuItem[total] = "CPU";
	menuSubItem[total] = buf[total];
	total++;

	sprintf(buf[total], "%d", entry->numElements);
	menuItem[total] = "Length";
	menuSubItem[total] = buf[total];
	total++;

	menuItem[total] = "Element Size";
	menuSubItem[total] = kWatchSizeStringList[entry->elementBytes];
	total++;

	menuItem[total] = "Label Type";
	menuSubItem[total] = kWatchLabelStringList[entry->labelType];
	total++;

	menuItem[total] = "Text Label";
	if(entry->label[0])
	{
		menuSubItem[total] = entry->label;
	}
	else
	{
		menuSubItem[total] = "(none)";
	}
	total++;

	menuItem[total] = "Display Type";
	menuSubItem[total] = kWatchDisplayTypeStringList[entry->displayType];
	total++;

	sprintf(buf[total], "%d", entry->x);
	menuItem[total] = "X";
	menuSubItem[total] = buf[total];
	total++;

	sprintf(buf[total], "%d", entry->y);
	menuItem[total] = "Y";
	menuSubItem[total] = buf[total];
	total++;

	sprintf(buf[total], "%d", entry->skip);
	menuItem[total] = "Skip Bytes";
	menuSubItem[total] = buf[total];
	total++;

	sprintf(buf[total], "%d", entry->elementsPerLine);
	menuItem[total] = "Elements Per Line";
	menuSubItem[total] = buf[total];
	total++;

	{
		if(entry->addValue < 0)
			sprintf(buf[total], "-%.2X", -entry->addValue);
		else
			sprintf(buf[total], "%.2X", entry->addValue);

		menuItem[total] = "Add Value";
		menuSubItem[total] = buf[total];
		total++;
	}

	sprintf(buf[total], "%d", entry->addressShift);
	menuItem[total] = "Address Shift";
	menuSubItem[total] = buf[total];
	total++;

	sprintf(buf[total], "%d", entry->dataShift);
	menuItem[total] = "Data Shift";
	menuSubItem[total] = buf[total];
	total++;

	sprintf(buf[total], "%.*X", kSearchByteDigitsTable[kWatchSizeConversionTable[entry->elementBytes]], entry->xor);
	menuItem[total] = "XOR";
	menuSubItem[total] = buf[total];
	total++;

	menuItem[total] = ui_getstring(UI_returntoprior);
	menuSubItem[total] = NULL;
	total++;

	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	if(sel < 0)
		sel = 0;
	if(sel >= total)
		sel = total - 1;

	if(editActive)
		flagBuf[sel] = 1;

	ui_displaymenu(bitmap, menuItem, menuSubItem, flagBuf, sel, 0);

	if(AltKeyPressed())
		increment <<= 4;
	if(ControlKeyPressed())
		increment <<= 8;
	if(ShiftKeyPressed())
		increment <<= 16;

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		editActive = 0;

		switch(sel)
		{
			case kMenu_Address:
				entry->address = DoShift(entry->address, entry->addressShift);
				entry->address -= increment;
				entry->address = DoShift(entry->address, -entry->addressShift);
				entry->address &= cpuInfoList[entry->cpu].addressMask;
				break;

			case kMenu_CPU:
				entry->cpu--;

				if(entry->cpu >= cpu_gettotalcpu())
					entry->cpu = cpu_gettotalcpu() - 1;

				entry->address &= cpuInfoList[entry->cpu].addressMask;
				break;

			case kMenu_NumElements:
				if(entry->numElements > 0)
					entry->numElements--;
				else
					entry->numElements = 0;
				break;

			case kMenu_ElementSize:
				if(entry->elementBytes > 0)
					entry->elementBytes--;
				else
					entry->elementBytes = 0;

				entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
				break;

			case kMenu_LabelType:
				if(entry->labelType > 0)
					entry->labelType--;
				else
					entry->labelType = 0;
				break;

			case kMenu_TextLabel:
				break;

			case kMenu_DisplayType:
				if(entry->displayType > 0)
					entry->displayType--;
				else
					entry->displayType = 0;
				break;

			case kMenu_XPosition:
				entry->x--;
				break;

			case kMenu_YPosition:
				entry->y--;
				break;

			case kMenu_Skip:
				if(entry->skip > 0)
					entry->skip--;
				break;

			case kMenu_ElementsPerLine:
				if(entry->elementsPerLine > 0)
					entry->elementsPerLine--;
				break;

			case kMenu_AddValue:
				entry->addValue = (entry->addValue - 1) & 0xFF;
				break;

			case kMenu_AddressShift:
				if(entry->addressShift > -31)
					entry->addressShift--;
				else
					entry->addressShift = 31;
				break;

			case kMenu_DataShift:
				if(entry->dataShift > -31)
					entry->dataShift--;
				else
					entry->dataShift = 31;
				break;

			case kMenu_XOR:
				entry->xor -= increment;
				entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		editActive = 0;

		switch(sel)
		{
			case kMenu_Address:
				entry->address = DoShift(entry->address, entry->addressShift);
				entry->address += increment;
				entry->address = DoShift(entry->address, -entry->addressShift);
				entry->address &= cpuInfoList[entry->cpu].addressMask;
				break;

			case kMenu_CPU:
				entry->cpu++;

				if(entry->cpu >= cpu_gettotalcpu())
					entry->cpu = 0;

				entry->address &= cpuInfoList[entry->cpu].addressMask;
				break;

			case kMenu_NumElements:
				entry->numElements++;
				break;

			case kMenu_ElementSize:
				if(entry->elementBytes < kSearchSize_32Bit)
					entry->elementBytes++;
				else
					entry->elementBytes = kSearchSize_32Bit;

				entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
				break;

			case kMenu_LabelType:
				if(entry->labelType < kWatchLabel_MaxPlusOne - 1)
					entry->labelType++;
				else
					entry->labelType = kWatchLabel_MaxPlusOne - 1;
				break;

			case kMenu_TextLabel:
				break;

			case kMenu_DisplayType:
				if(entry->displayType < kWatchDisplayType_MaxPlusOne - 1)
					entry->displayType++;
				else
					entry->displayType = kWatchDisplayType_MaxPlusOne - 1;
				break;

			case kMenu_XPosition:
				entry->x += increment;
				break;

			case kMenu_YPosition:
				entry->y += increment;
				break;

			case kMenu_Skip:
				entry->skip++;
				break;

			case kMenu_ElementsPerLine:
				entry->elementsPerLine++;
				break;

			case kMenu_AddValue:
				entry->addValue = (entry->addValue + 1) & 0xFF;
				break;

			case kMenu_AddressShift:
				if(entry->addressShift < 31)
					entry->addressShift++;
				else
					entry->addressShift = -31;
				break;

			case kMenu_DataShift:
				if(entry->dataShift < 31)
					entry->dataShift++;
				else
					entry->dataShift = -31;
				break;

			case kMenu_XOR:
				entry->xor += increment;
				entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;

		editActive = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;

		editActive = 0;
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(editActive)
		{
			editActive = 0;
		}
		else
		{
			switch(sel)
			{
				case kMenu_Return:
					sel = -1;
					break;

				case kMenu_Address:
				case kMenu_CPU:
				case kMenu_NumElements:
				case kMenu_TextLabel:
				case kMenu_XPosition:
				case kMenu_YPosition:
				case kMenu_AddValue:
				case kMenu_AddressShift:
				case kMenu_DataShift:
				case kMenu_XOR:
					osd_readkey_unicode(1);
					editActive = 1;
			}
		}
	}

	if(editActive)
	{
		switch(sel)
		{
			case kMenu_Address:
				entry->address = DoShift(entry->address, entry->addressShift);
				entry->address = DoEditHexField(entry->address);
				entry->address = DoShift(entry->address, -entry->addressShift);
				entry->address &= cpuInfoList[entry->cpu].addressMask;
				break;

			case kMenu_CPU:
				entry->cpu = DoEditDecField(entry->cpu, 0, cpu_gettotalcpu() - 1);
				entry->address &= cpuInfoList[entry->cpu].addressMask;
				break;

			case kMenu_NumElements:
				entry->numElements = DoEditDecField(entry->numElements, 0, 99);
				break;

			case kMenu_TextLabel:
				DoStaticEditTextField(entry->label, 255);
				break;

			case kMenu_XPosition:
				entry->x = DoEditDecField(entry->x, -1000, 1000);
				break;

			case kMenu_YPosition:
				entry->y = DoEditDecField(entry->y, -1000, 1000);
				break;

			case kMenu_AddValue:
				entry->addValue = DoEditHexFieldSigned(entry->addValue, 0xFFFFFF80) & 0xFF;
				break;

			case kMenu_AddressShift:
				entry->addressShift = DoEditDecField(entry->addressShift, -31, 31);
				break;

			case kMenu_DataShift:
				entry->dataShift = DoEditDecField(entry->dataShift, -31, 31);
				break;

			case kMenu_XOR:
				entry->xor = DoEditHexField(entry->xor);
				entry->xor &= kSearchByteMaskTable[kWatchSizeConversionTable[entry->elementBytes]];
				break;
		}

		if(input_ui_pressed(IPT_UI_CANCEL))
			editActive = 0;
	}
	else
	{
		if(input_ui_pressed(IPT_UI_ADD_CHEAT))
		{
			AddCheatFromWatch(entry);
		}

		if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
		{
			entry->numElements = 0;
		}

		if(input_ui_pressed(IPT_UI_SAVE_CHEAT))
		{
			CheatEntry	tempEntry;

			memset(&tempEntry, 0, sizeof(CheatEntry));

			SetupCheatFromWatchAsWatch(&tempEntry, entry);
			SaveCheat(&tempEntry);
			DisposeCheat(&tempEntry);
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if(	(sel == -1) ||
		(sel == -2))
		schedule_full_refresh();

	return sel + 1;
}

static int SelectSearchRegions(struct mame_bitmap * bitmap, int selection, SearchInfo * search)
{
	const char	* kSearchSpeedList[] =
	{
		"Fast",
		"Medium",
		"Slow",
		"Very Slow",
		"All Memory"
	};

	INT32			sel;
	const char		** menuItem;
	const char		** menuSubItem;
	INT32			i;
	INT32			total = 0;
	SearchRegion	* region;

	if(!search)
		return 0;

	sel = selection - 1;

	RequestStrings(search->regionListLength + 3, 0, 0, 0);

	menuItem =		menuStrings.mainList;
	menuSubItem =	menuStrings.subList;

	for(i = 0; i < search->regionListLength; i++)
	{
		SearchRegion	* traverse = &search->regionList[i];

		menuItem[total] = traverse->name;
		menuSubItem[total] = ui_getstring((traverse->flags & kRegionFlag_Enabled) ? UI_on : UI_off);
		total++;
	}

	menuItem[total] = "Search Speed";
	menuSubItem[total] = kSearchSpeedList[search->searchSpeed];
	total++;

	menuItem[total] = ui_getstring(UI_returntoprior);
	menuSubItem[total] = NULL;
	total++;

	menuItem[total] = NULL;
	menuSubItem[total] = NULL;

	if(sel < 0)
		sel = 0;
	if(sel > (total - 1))
		sel = total - 1;

	if(sel < search->regionListLength)
		region = &search->regionList[sel];
	else
		region = NULL;

	ui_displaymenu(bitmap, menuItem, menuSubItem, NULL, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		if(sel < search->regionListLength)
		{
			search->regionList[sel].flags ^= kRegionFlag_Enabled;

			AllocateSearchRegions(search);
		}

		if(sel == search->regionListLength)	/* set search speed*/
		{
			if(search->searchSpeed > kSearchSpeed_Fast)
				search->searchSpeed--;
			else
				search->searchSpeed = kSearchSpeed_Max;

			BuildSearchRegions(search);
			AllocateSearchRegions(search);
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		if(sel < search->regionListLength)
		{
			search->regionList[sel].flags ^= kRegionFlag_Enabled;

			AllocateSearchRegions(search);
		}

		if(sel == search->regionListLength)	/* set search speed*/
		{
			if(search->searchSpeed < kSearchSpeed_Max)
				search->searchSpeed++;
			else
				search->searchSpeed = kSearchSpeed_Fast;

			BuildSearchRegions(search);
			AllocateSearchRegions(search);
		}
	}

	if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			if(region && search)
			{
				InvalidateEntireRegion(search, region);

				usrintf_showmessage_secs(1, "region invalidated - %d results remain", search->numResults);
			}
		}
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel >= total - 1)
		{
			sel = -1;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if((sel == -1) || (sel == -2))
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

static int SelectSearch(struct mame_bitmap * bitmap, int selection)
{
	INT32			sel;
	const char		** menuItem;
	char			** buf;
	INT32			i;
	INT32			total = 0;

	sel = selection - 1;

	RequestStrings(searchListLength + 2, searchListLength, 300, 0);

	menuItem =	menuStrings.mainList;
	buf =		menuStrings.mainStrings;

	for(i = 0; i < searchListLength; i++)
	{
		SearchInfo	* info = &searchList[i];

		if(i == currentSearchIdx)
		{
			if(info->name)
			{
				sprintf(buf[total], "[#%d: %s]", i, info->name);
			}
			else
			{
				sprintf(buf[total], "[#%d]", i);
			}
		}
		else
		{
			if(info->name)
			{
				sprintf(buf[total], "#%d: %s", i, info->name);
			}
			else
			{
				sprintf(buf[total], "#%d", i);
			}
		}

		menuItem[total] = buf[total];
		total++;
	}

	menuItem[total] = ui_getstring(UI_returntoprior);
	total++;

	menuItem[total] = NULL;

	if(sel < 0)
		sel = 0;
	if(sel > (total - 1))
		sel = total - 1;

	ui_displaymenu(bitmap, menuItem, NULL, NULL, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		sel++;

		if(sel >= total)
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		sel--;

		if(sel < 0)
			sel = total - 1;
	}

	if(input_ui_pressed(IPT_UI_ADD_CHEAT))
	{
		AddSearchBefore(sel);

		BuildSearchRegions(&searchList[sel]);
		AllocateSearchRegions(&searchList[sel]);
	}

	if(input_ui_pressed(IPT_UI_DELETE_CHEAT))
	{
		if(searchListLength > 1)
		{
			DeleteSearchAt(sel);
		}
	}

	if(input_ui_pressed(IPT_UI_EDIT_CHEAT))
	{
		if(sel < total - 1)
			currentSearchIdx = sel;
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		if(sel >= total - 1)
		{
			sel = -1;
		}
		else
		{
			currentSearchIdx = sel;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if((sel == -1) || (sel == -2))
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

static INT32 DisplayHelp(struct mame_bitmap * bitmap, int selection)
{
	char	buf[2048];
	int		sel;

	sel = selection - 1;

	sprintf(buf,	"\tPlease Go To\n"
					"\thttp://cheat.retrogames.com/faq.htm\n"
					"\tFor Documentation\n"
					"\t%s %s %s", ui_getstring(UI_lefthilight), ui_getstring(UI_OK), ui_getstring(UI_righthilight));

	/* print fake menu*/
	ui_displaymessagewindow(bitmap, buf);

	/* done?*/
	if(input_ui_pressed(IPT_UI_SELECT))
	{
		sel = -1;
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
	{
		sel = -1;
	}

	if(input_ui_pressed(IPT_UI_CONFIGURE))
	{
		sel = -2;
	}

	if (sel == -1 || sel == -2)
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

static int SelectOptions(struct mame_bitmap * bitmap, int selection)
{
	enum
	{
		kMenu_SelectSearchRegions = 0,
		kMenu_SelectSearch,
		kMenu_ReloadCheatDatabase,
		kMenu_SearchDialogStyle,
		kMenu_ShowSearchLabels,
		kMenu_AutoSaveCheats,
		kMenu_Return,

		kMenu_Max
	};

	INT32			sel;
	static INT32	submenuChoice = 0;
	const char		* menuItem[kMenu_Max + 1];
	const char		* menuSubItem[kMenu_Max + 1];
	int				total = 0;

	sel = selection - 1;

	if(submenuChoice)
	{
		switch(sel)
		{
			case kMenu_SelectSearchRegions:
				submenuChoice = SelectSearchRegions(bitmap, submenuChoice, GetCurrentSearch());
				break;

			case kMenu_SelectSearch:
				submenuChoice = SelectSearch(bitmap, submenuChoice);
				break;

			default:
				submenuChoice = 0;
				sel = -1;
				break;
		}

		if(submenuChoice == -1)
			submenuChoice = 0;

		return sel + 1;
	}

	menuItem[total] =		ui_getstring(UI_search_select_memory_areas);
	menuSubItem[total] =	NULL;
	total++;

	menuItem[total] =		"Select Search";
	menuSubItem[total] =	NULL;
	total++;

	menuItem[total] =		ui_getstring(UI_reloaddatabase);
	menuSubItem[total] =	NULL;
	total++;

	menuItem[total] =		"Search Dialog Style";
	menuSubItem[total] =	useClassicSearchBox ? "Classic" : "Advanced";
	total++;

	menuItem[total] =		"Show Search Labels";
	menuSubItem[total] =	ui_getstring(dontPrintNewLabels ? UI_off : UI_on);
	total++;

	menuItem[total] =		"Auto Save Cheats";
	menuSubItem[total] =	ui_getstring(autoSaveEnabled ? UI_on : UI_off);
	total++;

	menuItem[total] =		ui_getstring(UI_returntoprior);
	menuSubItem[total] =	NULL;
	total++;

	menuItem[total] =		NULL;
	menuSubItem[total] =	NULL;

	ui_displaymenu(bitmap, menuItem, menuSubItem, NULL, sel, 0);

	if(UIPressedRepeatThrottle(IPT_UI_RIGHT, kHorizontalSlowKeyRepeatRate))
	{
		switch(sel)
		{
			case kMenu_SearchDialogStyle:
				useClassicSearchBox ^= 1;
				break;

			case kMenu_ShowSearchLabels:
				dontPrintNewLabels ^= 1;
				break;

			case kMenu_AutoSaveCheats:
				autoSaveEnabled ^= 1;
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_LEFT, kHorizontalSlowKeyRepeatRate))
	{
		switch(sel)
		{
			case kMenu_SearchDialogStyle:
				useClassicSearchBox ^= 1;
				break;

			case kMenu_ShowSearchLabels:
				dontPrintNewLabels ^= 1;
				break;

			case kMenu_AutoSaveCheats:
				autoSaveEnabled ^= 1;
				break;
		}
	}

	if(UIPressedRepeatThrottle(IPT_UI_DOWN, kVerticalKeyRepeatRate))
	{
		if(sel < (kMenu_Max - 1))
			sel++;
		else
			sel = 0;
	}

	if(UIPressedRepeatThrottle(IPT_UI_UP, kVerticalKeyRepeatRate))
	{
		if(sel > 0)
			sel--;
		else
			sel = kMenu_Max - 1;
	}

	if(input_ui_pressed(IPT_UI_SELECT))
	{
		switch(sel)
		{
			case kMenu_Return:
				submenuChoice = 0;
				sel = -1;
				break;

			case kMenu_ReloadCheatDatabase:
				DisposeCheatDatabase();
				LoadCheatDatabase();

				usrintf_showmessage_secs(1, "cheat database reloaded");
				break;

			case kMenu_SelectSearchRegions:
			case kMenu_SelectSearch:
				submenuChoice = 1;
				schedule_full_refresh();
				break;
		}
	}

	if(input_ui_pressed(IPT_UI_CANCEL))
		sel = -1;
	if(input_ui_pressed(IPT_UI_CONFIGURE))
		sel = -2;

	if((sel == -1) || (sel == -2))
	{
		schedule_full_refresh();
	}

	return sel + 1;
}

void DoCheat(struct mame_bitmap * bitmap)
{
	int	i;

	if(input_ui_pressed(IPT_UI_TOGGLE_CHEAT))
	{
		if(ShiftKeyPressed())
		{
			watchesDisabled ^= 1;

			usrintf_showmessage_secs(1, "%s %s", ui_getstring(UI_watchpoints), watchesDisabled ? ui_getstring (UI_off) : ui_getstring (UI_on));
		}
		else
		{
			cheatsDisabled ^= 1;

			usrintf_showmessage_secs(1, "%s %s", ui_getstring(UI_cheats), cheatsDisabled ? ui_getstring (UI_off) : ui_getstring (UI_on));

			if(cheatsDisabled)
			{
				for(i = 0; i < cheatListLength; i++)
				{
					TempDeactivateCheat(&cheatList[i]);
				}
			}
		}
	}

	DisplayWatches(bitmap);

	if(cheatsDisabled)
		return;

	for(i = 0; i < cheatListLength; i++)
	{
		DoCheatEntry(&cheatList[i]);
	}
}

UINT32 PrintBinary(char * buf, UINT32 data, UINT32 mask)
{
	UINT32	traverse = 0x80000000;
	UINT32	written = 0;

	while(traverse)
	{
		if(mask & traverse)
		{
			*buf++ = (data & traverse) ? '1' : '0';
			written++;
		}

		traverse >>= 1;
	}

	*buf++ = 0;

	return written;
}

UINT32 PrintASCII(char * buf, UINT32 data, UINT8 size)
{
	switch(size)
	{
		case kSearchSize_8Bit:
		case kSearchSize_1Bit:
		default:
			buf[0] = (data >> 0) & 0xFF;
			buf[1] = 0;

			return 1;

		case kSearchSize_16Bit:
			buf[0] = (data >> 8) & 0xFF;
			buf[1] = (data >> 0) & 0xFF;
			buf[2] = 0;

			return 2;

		case kSearchSize_32Bit:
			buf[0] = (data >> 24) & 0xFF;
			buf[1] = (data >> 16) & 0xFF;
			buf[2] = (data >>  8) & 0xFF;
			buf[3] = (data >>  0) & 0xFF;
			buf[4] = 0;

			return 4;
	}

	buf[0] = 0;
	return 0;
}

void DisplayWatches(struct mame_bitmap * bitmap)
{
	int		i;

	if(watchesDisabled)
		return;

	for(i = 0; i < watchListLength; i++)
	{
		int			j;
		WatchInfo	* info = &watchList[i];
		char		buf[1024];
		UINT32		address = info->address;
		int			xOffset = 0, yOffset = 0;
		int			numChars;
		int			lineElements = 0;

		if(info->numElements)
		{
			switch(info->labelType)
			{
				case kWatchLabel_Address:
					numChars = sprintf(buf, "%.8X: ", info->address);

					ui_text(bitmap, buf, xOffset * uirotcharwidth + info->x, yOffset * uirotcharheight + info->y);
					xOffset += numChars;
					break;

				case kWatchLabel_String:
					numChars = sprintf(buf, "%s: ", info->label);

					ui_text(bitmap, buf, xOffset * uirotcharwidth + info->x, yOffset * uirotcharheight + info->y);
					xOffset += numChars;
					break;
			}

			for(j = 0; j < info->numElements; j++)
			{
				UINT32	data;

				data = (DoCPURead(info->cpu, address, kSearchByteIncrementTable[info->elementBytes], CPUNeedsSwap(info->cpu)) + info->addValue) & kSearchByteMaskTable[info->elementBytes];
				data = DoShift(data, info->dataShift);
				data ^= info->xor;

				if(	(lineElements >= info->elementsPerLine) &&
					info->elementsPerLine)
				{
					lineElements = 0;

					xOffset = 0;
					yOffset++;
				}

				switch(info->displayType)
				{
					case kWatchDisplayType_Hex:
						numChars = sprintf(buf, "%.*X", kSearchByteDigitsTable[info->elementBytes], data);

						ui_text(bitmap, buf, xOffset * uirotcharwidth + info->x, yOffset * uirotcharheight + info->y);
						xOffset += numChars;
						xOffset++;
						break;

					case kWatchDisplayType_Decimal:
						numChars = sprintf(buf, "%.*d", kSearchByteDecDigitsTable[info->elementBytes], data);

						ui_text(bitmap, buf, xOffset * uirotcharwidth + info->x, yOffset * uirotcharheight + info->y);
						xOffset += numChars;
						xOffset++;
						break;

					case kWatchDisplayType_Binary:
						numChars = PrintBinary(buf, data, kSearchByteMaskTable[info->elementBytes]);

						ui_text(bitmap, buf, xOffset * uirotcharwidth + info->x, yOffset * uirotcharheight + info->y);
						xOffset += numChars;
						xOffset++;
						break;

					case kWatchDisplayType_ASCII:
						numChars = PrintASCII(buf, data, info->elementBytes);

						ui_text(bitmap, buf, xOffset * uirotcharwidth + info->x, yOffset * uirotcharheight + info->y);
						xOffset += numChars;
						break;
				}

				address += kSearchByteIncrementTable[info->elementBytes] + info->skip;
				lineElements++;
			}
		}
	}
}

static char * CreateStringCopy(char * buf)
{
	char	* temp = NULL;

	if(buf)
	{
		UINT32	length = strlen(buf) + 1;

		temp = malloc(length);

		if(temp)
		{
			memcpy(temp, buf, length);
		}
	}

	return temp;
}

static void ResizeCheatList(UINT32 newLength)
{
	if(newLength != cheatListLength)
	{
		if(newLength < cheatListLength)
		{
			int	i;

			for(i = newLength; i < cheatListLength; i++)
				DisposeCheat(&cheatList[i]);
		}

		cheatList = realloc(cheatList, newLength * sizeof(CheatEntry));
		if(!cheatList && (newLength != 0))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "ResizeCheatList: out of memory resizing cheat list\n");
			usrintf_showmessage_secs(2, "out of memory while loading cheat database");

			cheatListLength = 0;

			return;
		}

		if(newLength > cheatListLength)
		{
			int	i;

			memset(&cheatList[cheatListLength], 0, (newLength - cheatListLength) * sizeof(CheatEntry));

			for(i = cheatListLength; i < newLength; i++)
			{
				cheatList[i].flags |= kCheatFlag_Dirty;
			}
		}

		cheatListLength = newLength;
	}
}

static void ResizeCheatListNoDispose(UINT32 newLength)
{
	if(newLength != cheatListLength)
	{
		cheatList = realloc(cheatList, newLength * sizeof(CheatEntry));
		if(!cheatList && (newLength != 0))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "ResizeCheatListNoDispose: out of memory resizing cheat list\n");
			usrintf_showmessage_secs(2, "out of memory while loading cheat database");

			cheatListLength = 0;

			return;
		}

		if(newLength > cheatListLength)
		{
			int	i;

			memset(&cheatList[cheatListLength], 0, (newLength - cheatListLength) * sizeof(CheatEntry));

			for(i = cheatListLength; i < newLength; i++)
			{
				cheatList[i].flags |= kCheatFlag_Dirty;
			}
		}

		cheatListLength = newLength;
	}
}

static void AddCheatBefore(UINT32 idx)
{
	ResizeCheatList(cheatListLength + 1);

	if(idx < (cheatListLength - 1))
		memmove(&cheatList[idx + 1], &cheatList[idx], sizeof(CheatEntry) * (cheatListLength - 1 - idx));

	if(idx >= cheatListLength)
		idx = cheatListLength - 1;

	memset(&cheatList[idx], 0, sizeof(CheatEntry));
	cheatList[idx].flags |= kCheatFlag_Dirty;

	ResizeCheatActionList(&cheatList[idx], 1);
}

static void DeleteCheatAt(UINT32 idx)
{
	if(idx >= cheatListLength)
		return;

	DisposeCheat(&cheatList[idx]);

	if(idx < (cheatListLength - 1))
	{
		memmove(&cheatList[idx], &cheatList[idx + 1], sizeof(CheatEntry) * (cheatListLength - 1 - idx));
	}

	ResizeCheatListNoDispose(cheatListLength - 1);
}

static void DisposeCheat(CheatEntry * entry)
{
	if(entry)
	{
		int	i;

		free(entry->name);
		free(entry->comment);

		for(i = 0; i < entry->actionListLength; i++)
		{
			CheatAction	* action = &entry->actionList[i];

			DisposeAction(action);
		}

		free(entry->actionList);

		memset(entry, 0, sizeof(CheatEntry));
	}
}

static CheatEntry *	GetNewCheat(void)
{
	AddCheatBefore(cheatListLength);

	return &cheatList[cheatListLength - 1];
}

static void ResizeCheatActionList(CheatEntry * entry, UINT32 newLength)
{
	if(newLength != entry->actionListLength)
	{
		if(newLength < entry->actionListLength)
		{
			int	i;

			for(i = newLength; i < entry->actionListLength; i++)
				DisposeAction(&entry->actionList[i]);
		}

		entry->actionList = realloc(entry->actionList, newLength * sizeof(CheatAction));
		if(!entry->actionList && (newLength != 0))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "ResizeCheatActionList: out of memory resizing cheat action list\n");
			usrintf_showmessage_secs(2, "out of memory while loading cheat database");

			entry->actionListLength = 0;

			return;
		}

		if(newLength > entry->actionListLength)
		{
			memset(&entry->actionList[entry->actionListLength], 0, (newLength - entry->actionListLength) * sizeof(CheatAction));
		}

		entry->actionListLength = newLength;
	}
}

static void ResizeCheatActionListNoDispose(CheatEntry * entry, UINT32 newLength)
{
	if(newLength != entry->actionListLength)
	{
		entry->actionList = realloc(entry->actionList, newLength * sizeof(CheatAction));
		if(!entry->actionList && (newLength != 0))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "ResizeCheatActionList: out of memory resizing cheat action list\n");
			usrintf_showmessage_secs(2, "out of memory while loading cheat database");

			entry->actionListLength = 0;

			return;
		}

		if(newLength > entry->actionListLength)
		{
			memset(&entry->actionList[entry->actionListLength], 0, (newLength - entry->actionListLength) * sizeof(CheatAction));
		}

		entry->actionListLength = newLength;
	}
}

static void AddActionBefore(CheatEntry * entry, UINT32 idx)
{
	ResizeCheatActionList(entry, entry->actionListLength + 1);

	if(idx < (entry->actionListLength - 1))
		memmove(&entry->actionList[idx + 1], &entry->actionList[idx], sizeof(CheatAction) * (entry->actionListLength - 1 - idx));

	if(idx >= entry->actionListLength)
		idx = entry->actionListLength - 1;

	memset(&entry->actionList[idx], 0, sizeof(CheatAction));
}

static void DeleteActionAt(CheatEntry * entry, UINT32 idx)
{
	if(idx >= entry->actionListLength)
		return;

	DisposeAction(&entry->actionList[idx]);

	if(idx < (entry->actionListLength - 1))
	{
		memmove(&entry->actionList[idx], &entry->actionList[idx + 1], sizeof(CheatAction) * (entry->actionListLength - 1 - idx));
	}

	ResizeCheatActionListNoDispose(entry, entry->actionListLength - 1);
}

static void DisposeAction(CheatAction * action)
{
	if(action)
	{
		free(action->optionalName);

		memset(action, 0, sizeof(CheatAction));
	}
}

static void InitWatch(WatchInfo * info, UINT32 idx)
{
	if(idx > 0)
		info->y = watchList[idx - 1].y + uirotcharheight;
	else
		info->y = 0;
}

static void ResizeWatchList(UINT32 newLength)
{
	if(newLength != watchListLength)
	{
		if(newLength < watchListLength)
		{
			int	i;

			for(i = newLength; i < watchListLength; i++)
				DisposeWatch(&watchList[i]);
		}

		watchList = realloc(watchList, newLength * sizeof(WatchInfo));
		if(!watchList && (newLength != 0))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "ResizeWatchList: out of memory resizing watch list\n");
			usrintf_showmessage_secs(2, "out of memory while adding watch");

			watchListLength = 0;

			return;
		}

		if(newLength > watchListLength)
		{
			int	i;

			memset(&watchList[watchListLength], 0, (newLength - watchListLength) * sizeof(WatchInfo));

			for(i = watchListLength; i < newLength; i++)
			{
				InitWatch(&watchList[i], i);
			}
		}

		watchListLength = newLength;
	}
}

static void ResizeWatchListNoDispose(UINT32 newLength)
{
	if(newLength != watchListLength)
	{
		watchList = realloc(watchList, newLength * sizeof(WatchInfo));
		if(!watchList && (newLength != 0))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "ResizeWatchList: out of memory resizing watch list\n");
			usrintf_showmessage_secs(2, "out of memory while adding watch");

			watchListLength = 0;

			return;
		}

		if(newLength > watchListLength)
		{
			int	i;

			memset(&watchList[watchListLength], 0, (newLength - watchListLength) * sizeof(WatchInfo));

			for(i = watchListLength; i < newLength; i++)
			{
				InitWatch(&watchList[i], i);
			}
		}

		watchListLength = newLength;
	}
}

static void AddWatchBefore(UINT32 idx)
{
	ResizeWatchList(watchListLength + 1);

	if(idx < (watchListLength - 1))
		memmove(&watchList[idx + 1], &watchList[idx], sizeof(WatchInfo) * (watchListLength - 1 - idx));

	if(idx >= watchListLength)
		idx = watchListLength - 1;

	memset(&watchList[idx], 0, sizeof(WatchInfo));

	InitWatch(&watchList[idx], idx);
}

static void DeleteWatchAt(UINT32 idx)
{
	if(idx >= watchListLength)
		return;

	DisposeWatch(&watchList[idx]);

	if(idx < (watchListLength - 1))
	{
		memmove(&watchList[idx], &watchList[idx + 1], sizeof(WatchInfo) * (watchListLength - 1 - idx));
	}

	ResizeWatchListNoDispose(watchListLength - 1);
}

static void DisposeWatch(WatchInfo * watch)
{
	if(watch)
	{
		memset(watch, 0, sizeof(WatchInfo));
	}
}

static WatchInfo * GetUnusedWatch(void)
{
	int			i;
	WatchInfo	* info;
	WatchInfo	* theWatch = NULL;

	for(i = 0; i < watchListLength; i++)
	{
		info = &watchList[i];

		if(info->numElements == 0)
		{
			theWatch = info;

			break;
		}
	}

	if(!theWatch)
	{
		AddWatchBefore(watchListLength);

		theWatch = &watchList[watchListLength - 1];
	}

	return theWatch;
}

static void AddCheatFromWatch(WatchInfo * watch)
{
	if(watch)
	{
		CheatEntry	* entry = GetNewCheat();
		CheatAction	* action = &entry->actionList[0];
		char		tempString[1024];
		int			tempStringLength;
		UINT32		data = DoCPURead(watch->cpu, watch->address, kSearchByteIncrementTable[watch->elementBytes], 0);

		tempStringLength = sprintf(tempString, "%.8X (%d) = %.*X", watch->address, watch->cpu, kSearchByteDigitsTable[watch->elementBytes], data);

		entry->name = realloc(entry->name, tempStringLength + 1);
		memcpy(entry->name, tempString, tempStringLength + 1);

		SET_FIELD(action->type, LocationParameter, watch->cpu);
		SET_FIELD(action->type, BytesUsed, kSearchByteIncrementTable[watch->elementBytes] - 1);
		action->address = watch->address;
		action->data = data;
		action->extendData = 0xFFFFFFFF;
		action->originalDataField = data;

		UpdateCheatInfo(entry, 0);
	}
}

static void SetupCheatFromWatchAsWatch(CheatEntry * entry, WatchInfo * watch)
{
	if(watch && entry && watch->numElements)
	{
		CheatAction	* action;
		char		tempString[1024];
		int			tempStringLength;

		DisposeCheat(entry);
		ResizeCheatActionList(entry, 1);

		action = &entry->actionList[0];

		tempStringLength = sprintf(tempString, "Watch %.8X (%d)", watch->address, watch->cpu);

		entry->name = realloc(entry->name, tempStringLength + 1);
		memcpy(entry->name, tempString, tempStringLength + 1);

		action->type = 0;
		SET_FIELD(action->type, LocationParameter, watch->cpu);
		SET_FIELD(action->type, Type, kType_Watch);
		SET_FIELD(action->type, BytesUsed, kSearchByteIncrementTable[watch->elementBytes] - 1);
		SET_FIELD(action->type, TypeParameter, watch->displayType | ((watch->labelType == kWatchLabel_String) ? 0x04 : 0));

		action->address = watch->address;
		action->data  =	((watch->numElements - 1) & 0xFF) |
						((watch->skip & 0xFF) << 8) |
						((watch->elementsPerLine & 0xFF) << 16) |
						((watch->addValue & 0xFF) << 24);
		action->originalDataField = action->data;
		action->extendData = 0xFFFFFFFF;

		tempStringLength = strlen(watch->label);
		entry->comment = realloc(entry->comment, tempStringLength + 1);
		memcpy(entry->comment, watch->label, tempStringLength + 1);

		UpdateCheatInfo(entry, 0);
	}
}

static void ResizeSearchList(UINT32 newLength)
{
	if(newLength != searchListLength)
	{
		if(newLength < searchListLength)
		{
			int	i;

			for(i = newLength; i < searchListLength; i++)
				DisposeSearch(i);
		}

		searchList = realloc(searchList, newLength * sizeof(SearchInfo));
		if(!searchList && (newLength != 0))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "ResizeSearchList: out of memory resizing search list\n");
			usrintf_showmessage_secs(2, "out of memory while adding search");

			searchListLength = 0;

			return;
		}

		if(newLength > searchListLength)
		{
			int	i;

			memset(&searchList[searchListLength], 0, (newLength - searchListLength) * sizeof(SearchInfo));

			for(i = searchListLength; i < newLength; i++)
			{
				InitSearch(&searchList[i]);
			}
		}

		searchListLength = newLength;
	}
}

static void ResizeSearchListNoDispose(UINT32 newLength)
{
	if(newLength != searchListLength)
	{
		searchList = realloc(searchList, newLength * sizeof(SearchInfo));
		if(!searchList && (newLength != 0))
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "ResizeSearchList: out of memory resizing search list\n");
			usrintf_showmessage_secs(2, "out of memory while adding search");

			searchListLength = 0;

			return;
		}

		if(newLength > searchListLength)
		{
			memset(&searchList[searchListLength], 0, (newLength - searchListLength) * sizeof(SearchInfo));
		}

		searchListLength = newLength;
	}
}

static void AddSearchBefore(UINT32 idx)
{
	ResizeSearchListNoDispose(searchListLength + 1);

	if(idx < (searchListLength - 1))
		memmove(&searchList[idx + 1], &searchList[idx], sizeof(SearchInfo) * (searchListLength - 1 - idx));

	if(idx >= searchListLength)
		idx = searchListLength - 1;

	memset(&searchList[idx], 0, sizeof(SearchInfo));
	InitSearch(&searchList[idx]);
}

static void DeleteSearchAt(UINT32 idx)
{
	if(idx >= searchListLength)
		return;

	DisposeSearch(idx);

	if(idx < (searchListLength - 1))
	{
		memmove(&searchList[idx], &searchList[idx + 1], sizeof(SearchInfo) * (searchListLength - 1 - idx));
	}

	ResizeSearchListNoDispose(searchListLength - 1);
}

static void InitSearch(SearchInfo * info)
{
	if(info)
	{
		info->searchSpeed = kSearchSpeed_Medium;
	}
}

static void DisposeSearchRegions(SearchInfo * info)
{
	if(info->regionList)
	{
		int	i;

		for(i = 0; i < info->regionListLength; i++)
		{
			SearchRegion	* region = &info->regionList[i];

			free(region->first);
			free(region->last);
			free(region->status);
			free(region->backupLast);
			free(region->backupStatus);
		}

		free(info->regionList);

		info->regionList = NULL;
	}

	info->regionListLength = 0;
}

static void DisposeSearch(UINT32 idx)
{
	SearchInfo	* info;

	if(idx >= searchListLength)
		return;

	info = &searchList[idx];

	DisposeSearchRegions(info);

	free(info->name);
	info->name = NULL;
}

static SearchInfo *	GetCurrentSearch(void)
{
	if(currentSearchIdx >= searchListLength)
		currentSearchIdx = searchListLength - 1;
	if(currentSearchIdx < 0)
		currentSearchIdx = 0;

	return &searchList[currentSearchIdx];
}

static void FillBufferFromRegion(SearchRegion * region, UINT8 * buf)
{
	UINT32	offset;

	/* ### optimize if needed*/

	for(offset = 0; offset < region->length; offset++)
	{
		buf[offset] = ReadRegionData(region, offset, 1, 0);
	}
}

static UINT32 ReadRegionData(SearchRegion * region, UINT32 offset, UINT8 size, UINT8 swap)
{
	UINT32	address = region->address + offset;

	switch(region->targetType)
	{
		case kRegionType_CPU:
			return DoCPURead(region->targetIdx, address, size, CPUNeedsSwap(region->targetIdx) ^ swap);

		case kRegionType_Memory:
			if(region->cachedPointer)
				return DoMemoryRead(region->cachedPointer, address, size, swap, &rawCPUInfo);
			else
				return 0;
	}

	return 0;
}

static void BackupSearch(SearchInfo * info)
{
	int	i;

	for(i = 0; i < info->regionListLength; i++)
		BackupRegion(&info->regionList[i]);

	info->oldNumResults = info->numResults;
	info->backupValid = 1;
}

static void RestoreSearchBackup(SearchInfo * info)
{
	int	i;

	if(!info->backupValid)
		return;

	for(i = 0; i < info->regionListLength; i++)
		RestoreRegionBackup(&info->regionList[i]);

	info->numResults = info->oldNumResults;
	info->backupValid = 0;
}

static void BackupRegion(SearchRegion * region)
{
	if(region->flags & kRegionFlag_Enabled)
	{
		memcpy(region->backupLast,		region->last,	region->length);
		memcpy(region->backupStatus,	region->status,	region->length);
		region->oldNumResults =			region->numResults;
	}
}

static void RestoreRegionBackup(SearchRegion * region)
{
	if(region->flags & kRegionFlag_Enabled)
	{
		memcpy(region->last,	region->backupLast,		region->length);
		memcpy(region->status,	region->backupStatus,	region->length);
		region->numResults =	region->oldNumResults;
	}
}

static UINT8 DefaultEnableRegion(SearchRegion * region, SearchInfo * info)
{
	mem_write_handler	handler = region->writeHandler->handler;
	UINT32				handlerAddress = (UINT32)handler;

	switch(info->searchSpeed)
	{
		case kSearchSpeed_Fast:

#if HAS_SH2
			if(Machine->drv->cpu[0].cpu_type == CPU_SH2)
			{
				if(	(info->targetType == kRegionType_CPU) &&
					(info->targetIdx == 0) &&
					(region->address == 0x06000000))
					return 1;

				return 0;
			}
#endif

			if(	(handler == MWA_RAM) &&
				(!region->writeHandler->base))
				return 1;

				/* for neogeo, search bank one*/
				if(	(options.content_flags[CONTENT_NEOGEO]) &&
					(info->targetType == kRegionType_CPU) &&
					(info->targetIdx == 0) &&
					(handler == MWA_BANK1))
					return 1;

#if HAS_TMS34010

			/* for exterminator, search bank one*/
			if(	(Machine->drv->cpu[1].cpu_type == CPU_TMS34010) &&
				(info->targetType == kRegionType_CPU) &&
				(info->targetIdx == 1) &&
				(handler == MWA_BANK1))
				return 1;

			/* for smashtv, search bank two*/
			if(	(Machine->drv->cpu[0].cpu_type == CPU_TMS34010) &&
				(info->targetType == kRegionType_CPU) &&
				(info->targetIdx == 0) &&
				(handler == MWA_BANK2))
				return 1;

#endif

			return 0;

		case kSearchSpeed_Medium:
			if(	(handlerAddress >= ((UINT32)MWA_BANK1)) &&
				(handlerAddress <= ((UINT32)MWA_BANK24)))
				return 1;

			if(handler == MWA_RAM)
				return 1;

			return 0;

		case kSearchSpeed_Slow:
			if(	(handler == MWA_NOP) ||
				(handler == MWA_ROM))
				return 0;

			if(	(handlerAddress > STATIC_COUNT) &&
				(!region->writeHandler->base))
				return 0;

			return 1;

		case kSearchSpeed_VerySlow:
			if(	(handler == MWA_NOP) ||
				(handler == MWA_ROM))
				return 0;

			return 1;
	}

	return 0;
}

static void SetSearchRegionDefaultName(SearchRegion * region)
{
	switch(region->targetType)
	{
		case kRegionType_CPU:
		{
			char	desc[16];

			if(region->writeHandler)
			{
				mem_write_handler	handler = region->writeHandler->handler;
				UINT32				handlerAddress = (UINT32)handler;

				if(	(handlerAddress >= ((UINT32)MWA_BANK1)) &&
					(handlerAddress <= ((UINT32)MWA_BANK24)))
				{
					sprintf(desc, "BANK%.2d", (handlerAddress - ((UINT32)MWA_BANK1)) + 1);
				}
				else
				{
					switch(handlerAddress)
					{
						case (UINT32)MWA_NOP:		strcpy(desc, "NOP   ");	break;
						case (UINT32)MWA_RAM:		strcpy(desc, "RAM   ");	break;
						case (UINT32)MWA_ROM:		strcpy(desc, "ROM   ");	break;
						case (UINT32)MWA_RAMROM:	strcpy(desc, "RAMROM");	break;
						default:					strcpy(desc, "CUSTOM");	break;
					}
				}
			}
			else
			{
				sprintf(desc, "CPU%.2d ", region->targetIdx);
			}

			sprintf(region->name,	"%.*X-%.*X %s",
									cpuInfoList[region->targetIdx].addressCharsNeeded,
									region->address,
									cpuInfoList[region->targetIdx].addressCharsNeeded,
									region->address + region->length - 1,
									desc);
		}
		break;

		case kRegionType_Memory:
			sprintf(region->name, "%.8X-%.8X MEMORY", region->address, region->address + region->length - 1);
			break;

		default:
			sprintf(region->name, "UNKNOWN");
			break;
	}
}

static void AllocateSearchRegions(SearchInfo * info)
{
	int	i;

	info->backupValid = 0;
	info->numResults = 0;

	for(i = 0; i < info->regionListLength; i++)
	{
		SearchRegion	* region;

		region = &info->regionList[i];

		region->numResults = 0;

		free(region->first);
		free(region->last);
		free(region->status);
		free(region->backupLast);
		free(region->backupStatus);

		if(region->flags & kRegionFlag_Enabled)
		{
			region->first =			malloc(region->length);
			region->last =			malloc(region->length);
			region->status =		malloc(region->length);
			region->backupLast =	malloc(region->length);
			region->backupStatus =	malloc(region->length);

			if(	!region->first ||
				!region->last ||
				!region->status ||
				!region->backupLast ||
				!region->backupStatus)
			{
				free(region->first);
				free(region->last);
				free(region->status);
				free(region->backupLast);
				free(region->backupStatus);

				region->first =			NULL;
				region->last =			NULL;
				region->status =		NULL;
				region->backupLast =	NULL;
				region->backupStatus =	NULL;

				region->flags &= ~kRegionFlag_Enabled;
			}
		}
		else
		{
			region->first =			NULL;
			region->last =			NULL;
			region->status =		NULL;
			region->backupLast =	NULL;
			region->backupStatus =	NULL;
		}
	}
}

static void BuildSearchRegions(SearchInfo * info)
{
	info->comparison = kSearchComparison_EqualTo;

	DisposeSearchRegions(info);

	switch(info->targetType)
	{
		case kRegionType_CPU:
		{
			if(info->searchSpeed == kSearchSpeed_AllMemory)
			{
				UINT32			length = cpuInfoList[info->targetIdx].addressMask + 1;
				SearchRegion	* region;

				info->regionList = calloc(sizeof(SearchRegion), 1);
				info->regionListLength = 1;
				region = info->regionList;

				region->address = 0;
				region->length = length;

				region->targetIdx = info->targetIdx;
				region->targetType = info->targetType;
				region->writeHandler = NULL;

				region->first = NULL;
				region->last = NULL;
				region->status = NULL;

				region->backupLast = NULL;
				region->backupStatus = NULL;

				region->flags = kRegionFlag_Enabled;

				SetSearchRegionDefaultName(region);
			}
			else if(info->targetIdx < cpu_gettotalcpu())
			{
				const struct Memory_WriteAddress	* mwa = NULL;
				SearchRegion						* traverse;
				int									count = 0;

				mwa = Machine->drv->cpu[info->targetIdx].memory_write;

				while(!IS_MEMPORT_END(mwa))
				{
					if(!IS_MEMPORT_MARKER(mwa))
					{
						count++;
					}

					mwa++;
				}

				info->regionList = calloc(sizeof(SearchRegion), count);
				info->regionListLength = count;
				traverse = info->regionList;

				mwa = Machine->drv->cpu[info->targetIdx].memory_write;

				while(!IS_MEMPORT_END(mwa))
				{
					if(!IS_MEMPORT_MARKER(mwa))
					{
						UINT32	length = (mwa->end - mwa->start) + 1;

						traverse->address = mwa->start;
						traverse->length = length;

						traverse->targetIdx = info->targetIdx;
						traverse->targetType = info->targetType;
						traverse->writeHandler = mwa;

						traverse->first = NULL;
						traverse->last = NULL;
						traverse->status = NULL;

						traverse->backupLast = NULL;
						traverse->backupStatus = NULL;

						traverse->flags = DefaultEnableRegion(traverse, info) ? kRegionFlag_Enabled : 0;

						SetSearchRegionDefaultName(traverse);

						traverse++;
					}

					mwa++;
				}
			}
		}
		break;

		case kRegionType_Memory:
			break;
	}
}

static int ConvertOldCode(int code, int cpu, int * data, int * extendData)
{
	enum
	{
		kCustomField_None =					0,
		kCustomField_DontApplyCPUField =	1 << 0,
		kCustomField_SetBit =				1 << 1,
		kCustomField_ClearBit =				1 << 2,
		kCustomField_SubtractOne =			1 << 3,

		kCustomField_BitMask =				kCustomField_SetBit |
											kCustomField_ClearBit,
		kCustomField_End =					0xFF
	};

	struct ConversionTable
	{
		int		oldCode;
		UINT32	newCode;
		UINT8	customField;
	};

	struct ConversionTable	kConversionTable[] =
	{
		{	0,		0x00000000,	kCustomField_None },
		{	1,		0x00000001,	kCustomField_None },
		{	2,		0x00000020,	kCustomField_None },
		{	3,		0x00000040,	kCustomField_None },
		{	4,		0x000000A0,	kCustomField_None },
		{	5,		0x00000022,	kCustomField_None },
		{	6,		0x00000042,	kCustomField_None },
		{	7,		0x000000A2,	kCustomField_None },
		{	8,		0x00000024,	kCustomField_None },
		{	9,		0x00000044,	kCustomField_None },
		{	10,		0x00000064,	kCustomField_None },
		{	11,		0x00000084,	kCustomField_None },
		{	15,		0x00000023,	kCustomField_None },
		{	16,		0x00000043,	kCustomField_None },
		{	17,		0x000000A3,	kCustomField_None },
		{	20,		0x00000000,	kCustomField_SetBit },
		{	21,		0x00000001,	kCustomField_SetBit },
		{	22,		0x00000020,	kCustomField_SetBit },
		{	23,		0x00000040,	kCustomField_SetBit },
		{	24,		0x000000A0,	kCustomField_SetBit },
		{	40,		0x00000000,	kCustomField_ClearBit },
		{	41,		0x00000001,	kCustomField_ClearBit },
		{	42,		0x00000020,	kCustomField_ClearBit },
		{	43,		0x00000040,	kCustomField_ClearBit },
		{	44,		0x000000A0,	kCustomField_ClearBit },
		{	60,		0x00000103,	kCustomField_None },
		{	61,		0x00000303,	kCustomField_None },
		{	62,		0x00000503,	kCustomField_SubtractOne },
		{	63,		0x00000903,	kCustomField_None },
		{	64,		0x00000B03,	kCustomField_None },
		{	65,		0x00000D03,	kCustomField_SubtractOne },
		{	70,		0x00000101,	kCustomField_None },
		{	71,		0x00000301,	kCustomField_None },
		{	72,		0x00000501,	kCustomField_SubtractOne },
		{	73,		0x00000901,	kCustomField_None },
		{	74,		0x00000B01,	kCustomField_None },
		{	75,		0x00000D01,	kCustomField_SubtractOne },
		{	80,		0x00000102,	kCustomField_None },
		{	81,		0x00000302,	kCustomField_None },
		{	82,		0x00000502,	kCustomField_SubtractOne },
		{	83,		0x00000902,	kCustomField_None },
		{	84,		0x00000B02,	kCustomField_None },
		{	85,		0x00000D02,	kCustomField_SubtractOne },
		{	90,		0x00000100,	kCustomField_None },
		{	91,		0x00000300,	kCustomField_None },
		{	92,		0x00000500,	kCustomField_SubtractOne },
		{	93,		0x00000900,	kCustomField_None },
		{	94,		0x00000B00,	kCustomField_None },
		{	95,		0x00000D00,	kCustomField_SubtractOne },
		{	100,	0x20800000,	kCustomField_None },
		{	101,	0x20000001,	kCustomField_None },
		{	102,	0x20800000,	kCustomField_None },
		{	103,	0x20000001,	kCustomField_None },
		{	110,	0x40800000,	kCustomField_None },
		{	111,	0x40000001,	kCustomField_None },
		{	112,	0x40800000,	kCustomField_None },
		{	113,	0x40000001,	kCustomField_None },
		{	120,	0x63000001,	kCustomField_None },
		{	121,	0x63000001,	kCustomField_DontApplyCPUField | kCustomField_SetBit },
		{	122,	0x63000001,	kCustomField_DontApplyCPUField | kCustomField_ClearBit },
		{	998,	0x00000006,	kCustomField_None },
		{	999,	0x60000000,	kCustomField_DontApplyCPUField },
		{	-1,		0x00000000,	kCustomField_End }
	};

	struct ConversionTable	* traverse = kConversionTable;
	UINT32					newCode;
	UINT8					linkCheat = 0;

	/* convert link cheats*/
	if((code >= 500) && (code <= 699))
	{
		linkCheat = 1;

		code -= 500;
	}

	/* look up code*/
	while(traverse->oldCode >= 0)
	{
		if(code == traverse->oldCode)
		{
			goto foundCode;
		}

		traverse++;
	}

	log_cb(RETRO_LOG_ERROR, LOGPRE "ConvertOldCode: %d not found\n", code);

	/* not found*/
	*extendData = 0;
	return 0;

	foundCode:

	newCode = traverse->newCode;

	/* add in the CPU field*/
	if(!(traverse->customField & kCustomField_DontApplyCPUField))
	{
		newCode = (newCode & ~0x1F000000) | ((cpu << 24) & 0x1F000000);
	}

	/* hack-ish, subtract one from data field for x5 user select*/
	if(traverse->customField & kCustomField_SubtractOne)
		(*data)--;	/* yaay for C operator precedence*/

	/*	set up the extend data*/
	if(traverse->customField & kCustomField_BitMask)
	{
		*extendData = *data;
	}
	else
	{
		*extendData = 0xFFFFFFFF;
	}

	if(traverse->customField & kCustomField_ClearBit)
	{
		*data = 0;
	}

	if(linkCheat)
	{
		SET_MASK_FIELD(newCode, LinkEnable);
	}

	return newCode;
}

static int MatchCommandCheatLine(char * buf)
{
	int	argumentsMatched;
	unsigned int	data;

	argumentsMatched = sscanf(buf, ":_command:%X", &data);

	if(argumentsMatched == 1)
	{
		switch(data & 0xFF)
		{
			case 0x00:	/* disable help boxes*/
				break;

			case 0x01:	/* use old-style search*/
				useClassicSearchBox = 1;
				break;

			case 0x02:	/* use new-style search*/
				useClassicSearchBox = 0;
				break;

			case 0x03:	/* don't print labels in new-style search menu*/
				dontPrintNewLabels = 1;
				break;

			case 0x04:	/* enable auto-save*/
				autoSaveEnabled = 1;
				break;
		}

		return 1;
	}

	return 0;
}

static void HandleLocalCommandCheat(UINT32 type, UINT32 address, UINT32 data, UINT32 extendData, char * name, char * description)
{
	switch(EXTRACT_FIELD(type, LocationType))
	{
		case kLocation_Custom:
			switch(EXTRACT_FIELD(type, LocationParameter))
			{
				case kCustomLocation_AssignActivationKey:
				{
					if(address < cheatListLength)
					{
						CheatEntry	* entry = &cheatList[address];

						entry->activationKey = data;
						entry->flags |= kCheatFlag_HasActivationKey;
					}
				}
				break;

				case kCustomLocation_Enable:
				{
					if(address < cheatListLength)
					{
						CheatEntry	* entry = &cheatList[address];

						ActivateCheat(entry);
					}
				}
				break;

				case kCustomLocation_Overclock:
				{
					if(address < cpu_gettotalcpu())
					{
						double	overclock = data;

						overclock /= 65536.0;

						timer_set_overclock(address, overclock);
					}
				}
				break;
			}
			break;
	}
}

static void LoadCheatDatabase()
{
	intfstream_t	* in_file  = NULL;
	intfstream_t	* out_file = NULL;
	char 		cheat_directory[PATH_MAX_LENGTH];
	char 		cheat_path[PATH_MAX_LENGTH];
	char		formatString[256];
	char		oldFormatString[256];
	char		buf[2048];
	int			recordNames = 0;

	cheat_directory[0] = '\0';
	cheat_path[0]      = '\0';

	/* Open existing cheat.rzip */
	osd_get_path(FILETYPE_CHEAT, cheat_directory);
	snprintf(cheat_path, PATH_MAX_LENGTH, "%s%c%s", cheat_directory, PATH_DEFAULT_SLASH_C(), CHEAT_DATABASE_RZIP_FILENAME);
	in_file = intfstream_open_rzip_file(cheat_path, RETRO_VFS_FILE_ACCESS_READ);

	if(!in_file)
	{
		/* Try to open cheat.dat */
		cheat_path[0] = '\0';
		snprintf(cheat_path, PATH_MAX_LENGTH, "%s%c%s", cheat_directory, PATH_DEFAULT_SLASH_C(), CHEAT_DATABASE_FILENAME);
		in_file = intfstream_open_file(cheat_path, RETRO_VFS_FILE_ACCESS_READ, RETRO_VFS_FILE_ACCESS_HINT_NONE);

		if(!in_file)
			goto end;

		/* Create new cheat.rzip file */
		cheat_path[0] = '\0';
		snprintf(cheat_path, PATH_MAX_LENGTH, "%s%c%s", cheat_directory, PATH_DEFAULT_SLASH_C(), CHEAT_DATABASE_RZIP_FILENAME);
		out_file = intfstream_open_rzip_file(cheat_path, RETRO_VFS_FILE_ACCESS_WRITE);

		if(!out_file)
		{
			log_cb(RETRO_LOG_ERROR, LOGPRE "Failed to create cheat.rzip\n");
			goto end;
		}

		/* Compression loop */
		for(;;)
		{
			int64_t data_read  = intfstream_read(in_file, buf, sizeof(buf));
			int64_t data_write = 0;

			if (data_read < 0)
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "Failed to read from cheat.dat\n");
				goto end;
			}
	
			if (data_read == 0)
			{
				/* Finished, close cheat.rzip and rewind input file */
				intfstream_close(out_file);
				free(out_file);
				out_file = NULL;

				intfstream_rewind(in_file);
				break;
			}

			data_write = intfstream_write(out_file, buf, data_read);

			if (data_write != data_read)
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "Failed to write to cheat.rzip\n");
				goto end;
			}
		}
	}

	foundCheatDatabase = 1;

	/* make the format strings*/
	sprintf(formatString, ":%s:%s", Machine->gamedrv->name, "%x:%x:%x:%x:%" STR(NAME_MAX_LENGTH) "[^:\n\r]:%" STR(DESC_MAX_LENGTH) "[^:\n\r]");
	sprintf(oldFormatString, "%s:%s", Machine->gamedrv->name, "%d:%x:%x:%d:%" STR(NAME_MAX_LENGTH) "[^:\n\r]:%" STR(DESC_MAX_LENGTH) "[^:\n\r]");

	while(intfstream_gets(in_file, buf, 2048))
	{
		int			type;
		int			address;
		int			data;
		int			extendData;
		char		name[NAME_MAX_LENGTH+1];
		char		description[DESC_MAX_LENGTH+1];

		int			argumentsMatched;

		CheatEntry	* entry;
		CheatAction	* action;

		if(MatchCommandCheatLine(buf))
			continue;

		name[0] = 0;
		description[0] = 0;

		argumentsMatched = sscanf(buf, formatString, &type, &address, &data, &extendData, name, description);

		if(argumentsMatched < 4)
		{
			int	oldCPU;
			int	oldCode;

			argumentsMatched = sscanf(buf, oldFormatString, &oldCPU, &address, &data, &oldCode, name, description);
			if(argumentsMatched < 4)
			{
				continue;
			}
			else
			{
				/* convert the old code to the new format*/
				type = ConvertOldCode(oldCode, oldCPU, &data, &extendData);
			}
		}

		/*logerror("cheat: processing %s\n", buf);*/

		if(TEST_FIELD(type, RemoveFromList))
		{
			/*logerror("cheat: cheat line removed\n", buf);*/

			HandleLocalCommandCheat(type, address, data, extendData, name, description);
		}
		else
		{
			if(TEST_FIELD(type, LinkEnable))
			{
				if(cheatListLength == 0)
				{
					log_cb(RETRO_LOG_ERROR, LOGPRE "LoadCheatDatabase: first cheat found was link cheat; bailing\n");
					goto end;
				}

				/*logerror("cheat: doing link cheat\n");*/

				entry = &cheatList[cheatListLength - 1];
			}
			else
			{
				/* go to the next cheat*/
				ResizeCheatList(cheatListLength + 1);

				/*logerror("cheat: doing normal cheat\n");*/

				if(cheatListLength == 0)
				{
					log_cb(RETRO_LOG_ERROR, LOGPRE "LoadCheatDatabase: cheat list resize failed; bailing\n");
					goto end;
				}

				entry = &cheatList[cheatListLength - 1];

				entry->name = CreateStringCopy(name);

				/* copy the description if we got it*/
				if(argumentsMatched == 6)
				{
					entry->comment = CreateStringCopy(description);
				}

				recordNames = 0;

				if(	(EXTRACT_FIELD(type, LocationType) == kLocation_Custom) &&
					(EXTRACT_FIELD(type, LocationParameter) == kCustomLocation_Select))
				{
					recordNames = 1;
				}
			}

			ResizeCheatActionList(&cheatList[cheatListLength - 1], entry->actionListLength + 1);

			if(entry->actionListLength == 0)
			{
				log_cb(RETRO_LOG_ERROR, LOGPRE "LoadCheatDatabase: action list resize failed; bailing\n");
				goto end;
			}

			action = &entry->actionList[entry->actionListLength - 1];

			action->type = type;
			action->address = address;
			action->data = data;
			action->originalDataField = data;
			action->extendData = extendData;

			if(recordNames)
			{
				action->optionalName = CreateStringCopy(name);
			}
		}
	}

end:

	if(in_file)
	{
		intfstream_close(in_file);
		free(in_file);
	}

	if(out_file)
	{
		intfstream_close(out_file);
		free(out_file);
	}

	UpdateAllCheatInfo();
}

static void DisposeCheatDatabase(void)
{
	int	i;

	if(cheatList)
	{
		for(i = 0; i < cheatListLength; i++)
		{
			DisposeCheat(&cheatList[i]);
		}

		free(cheatList);

		cheatList = NULL;
		cheatListLength = 0;
	}
}

static void SaveCheat(CheatEntry * entry)
{
	mame_file * theFile;
	UINT32	i;
	char	buf[4096];

	if(!entry || !entry->actionList)
		return;

	theFile = mame_fopen(NULL, CHEAT_SAVE_FILENAME, FILETYPE_CHEAT, 1);

	if(!theFile)
		return;

	mame_fseek(theFile, 0, SEEK_END);

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction	* action = &entry->actionList[i];
		char		* name = entry->name;
		UINT32		type = action->type;
		char		* bufTraverse = buf;
		int			addressLength = 8;

		if(i != 0)
		{
			SET_MASK_FIELD(type, LinkEnable);

			if(entry->flags & kCheatFlag_Select)
			{
				name = action->optionalName;
			}
		}

		switch(EXTRACT_FIELD(type, LocationType))
		{
			case kLocation_Standard:
			case kLocation_HandlerMemory:
				addressLength = cpuInfoList[EXTRACT_FIELD(type, LocationParameter)].addressCharsNeeded;
				break;

			case kLocation_IndirectIndexed:
				addressLength = cpuInfoList[(EXTRACT_FIELD(type, LocationParameter) >> 2) & 0x7].addressCharsNeeded;
				break;

			case kLocation_MemoryRegion:
			{
				int	idx = EXTRACT_FIELD(type, LocationParameter) + REGION_CPU1 - REGION_INVALID;

				if(idx < kRegionListLength)
					addressLength = regionInfoList[idx].addressCharsNeeded;
			}
			break;
		}

		bufTraverse += sprintf(bufTraverse, ":%s:%.8X:%.*X:%.8X:%.8X", Machine->gamedrv->name, type, addressLength, action->address, action->originalDataField, action->extendData);

		if(name)
		{
			bufTraverse += sprintf(bufTraverse, ":%s", name);

			if((i == 0) && (entry->comment))
				bufTraverse += sprintf(bufTraverse, ":%s", entry->comment);
		}
		else
		{
			if((i == 0) && (entry->comment))
				bufTraverse += sprintf(bufTraverse, ":(none):%s", entry->comment);
		}

		bufTraverse += sprintf(bufTraverse, "\n");

		mame_fwrite(theFile, buf, strlen(buf));
	}

	mame_fclose(theFile);

	entry->flags &= ~kCheatFlag_Dirty;
}

static void DoAutoSaveCheats(void)
{
	int	i;

	for(i = 0; i < cheatListLength; i++)
	{
		CheatEntry	* entry = &cheatList[i];

		if(entry->flags & kCheatFlag_Dirty)
		{
			SaveCheat(entry);
		}
	}
}

static void AddCheatFromResult(SearchInfo * search, SearchRegion * region, UINT32 address)
{
	if(region->targetType == kRegionType_CPU)
	{
		CheatEntry	* entry = GetNewCheat();
		CheatAction	* action = &entry->actionList[0];
		char		tempString[1024];
		int			tempStringLength;
		UINT32		data = ReadSearchOperand(kSearchOperand_First, search, region, address);

		tempStringLength = sprintf(tempString, "%.8X (%d) = %.*X", address, region->targetIdx, kSearchByteDigitsTable[search->bytes], data);

		entry->name = realloc(entry->name, tempStringLength + 1);
		memcpy(entry->name, tempString, tempStringLength + 1);

		SET_FIELD(action->type, LocationParameter, region->targetIdx);
		SET_FIELD(action->type, BytesUsed, kSearchByteIncrementTable[search->bytes] - 1);
		action->address = address;
		action->data = data;
		action->extendData = 0xFFFFFFFF;
		action->originalDataField = data;

		UpdateCheatInfo(entry, 0);
	}
}

static void AddCheatFromFirstResult(SearchInfo * search)
{
	int	i;

	for(i = 0; i < search->regionListLength; i++)
	{
		SearchRegion	* region = &search->regionList[i];

		if(region->numResults)
		{
			UINT32	traverse;

			for(traverse = 0; traverse < region->length; traverse++)
			{
				UINT32	address = region->address + traverse;

				if(IsRegionOffsetValid(search, region, traverse))
				{
					AddCheatFromResult(search, region, address);

					return;
				}
			}
		}
	}
}

static void AddWatchFromResult(SearchInfo * search, SearchRegion * region, UINT32 address)
{
	if(region->targetType == kRegionType_CPU)
	{
		WatchInfo	* info = GetUnusedWatch();

		info->address =			address;
		info->cpu =				region->targetIdx;
		info->numElements =		1;
		info->elementBytes =	kWatchSizeConversionTable[search->bytes];
		info->labelType =		kWatchLabel_None;
		info->displayType =		kWatchDisplayType_Hex;
		info->skip =			0;
		info->elementsPerLine =	0;
		info->addValue =		0;

		info->linkedCheat =		NULL;

		info->label[0] =		0;
	}
}

static UINT32 SearchSignExtend(SearchInfo * search, UINT32 value)
{
	if(search->sign)
	{
		if(value & kSearchByteSignBitTable[search->bytes])
		{
			value |= ~kSearchByteUnsignedMaskTable[search->bytes];
		}
	}

	return value;
}

static UINT32 ReadSearchOperand(UINT8 type, SearchInfo * search, SearchRegion * region, UINT32 address)
{
	UINT32	value = 0;

	switch(type)
	{
		case kSearchOperand_Current:
			value = ReadRegionData(region, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap);
			break;

		case kSearchOperand_Previous:
			value = DoMemoryRead(region->last, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap, NULL);
			break;

		case kSearchOperand_First:
			value = DoMemoryRead(region->first, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap, NULL);
			break;

		case kSearchOperand_Value:
			value = search->value;
			break;
	}

	value = SearchSignExtend(search, value);

	return value;
}

static UINT32 ReadSearchOperandBit(UINT8 type, SearchInfo * search, SearchRegion * region, UINT32 address)
{
	UINT32	value = 0;

	switch(type)
	{
		case kSearchOperand_Current:
			value = ReadRegionData(region, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap);
			break;

		case kSearchOperand_Previous:
			value = DoMemoryRead(region->last, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap, NULL);
			break;

		case kSearchOperand_First:
			value = DoMemoryRead(region->first, address - region->address, kSearchByteIncrementTable[search->bytes], search->swap, NULL);
			break;

		case kSearchOperand_Value:
			if(search->value)
				value = 0xFFFFFFFF;
			else
				value = 0x00000000;
			break;
	}

	value = SearchSignExtend(search, value);

	return value;
}

static UINT8 DoSearchComparison(SearchInfo * search, UINT32 lhs, UINT32 rhs)
{
	INT32	svalue;

	if(search->sign)
	{
		INT32	slhs, srhs;

		slhs = lhs;
		srhs = rhs;

		switch(search->comparison)
		{
			case kSearchComparison_LessThan:
				return slhs < srhs;

			case kSearchComparison_GreaterThan:
				return slhs > srhs;

			case kSearchComparison_EqualTo:
				return slhs == srhs;

			case kSearchComparison_LessThanOrEqualTo:
				return slhs <= srhs;

			case kSearchComparison_GreaterThanOrEqualTo:
				return slhs >= srhs;

			case kSearchComparison_NotEqual:
				return slhs != srhs;

			case kSearchComparison_IncreasedBy:
				svalue = search->value;
				if(search->value & kSearchByteSignBitTable[search->bytes])
					svalue |= ~kSearchByteUnsignedMaskTable[search->bytes];

				return slhs == (srhs + svalue);

			case kSearchComparison_NearTo:
				return (slhs == srhs) || ((slhs + 1) == srhs);
		}
	}
	else
	{
		switch(search->comparison)
		{
			case kSearchComparison_LessThan:
				return lhs < rhs;

			case kSearchComparison_GreaterThan:
				return lhs > rhs;

			case kSearchComparison_EqualTo:
				return lhs == rhs;

			case kSearchComparison_LessThanOrEqualTo:
				return lhs <= rhs;

			case kSearchComparison_GreaterThanOrEqualTo:
				return lhs >= rhs;

			case kSearchComparison_NotEqual:
				return lhs != rhs;

			case kSearchComparison_IncreasedBy:
				svalue = search->value;
				if(search->value & kSearchByteSignBitTable[search->bytes])
					svalue |= ~kSearchByteUnsignedMaskTable[search->bytes];

				return lhs == (rhs + svalue);

			case kSearchComparison_NearTo:
				return (lhs == rhs) || ((lhs + 1) == rhs);
		}
	}

	return 0;
}

static UINT32 DoSearchComparisonBit(SearchInfo * search, UINT32 lhs, UINT32 rhs)
{
	switch(search->comparison)
	{
		case kSearchComparison_LessThan:
		case kSearchComparison_NotEqual:
		case kSearchComparison_GreaterThan:
		case kSearchComparison_LessThanOrEqualTo:
		case kSearchComparison_GreaterThanOrEqualTo:
		case kSearchComparison_IncreasedBy:
			return lhs ^ rhs;

		case kSearchComparison_EqualTo:
		case kSearchComparison_NearTo:
			return ~(lhs ^ rhs);
	}

	return 0;
}

/*
static UINT8 IsRegionOffsetValid(SearchInfo * search, SearchRegion * region, UINT32 offset)
{
	switch(kSearchByteIncrementTable[search->bytes])
	{
		case 1:
			return *((UINT8  *)&region->status[offset]) == 0xFF;
			break;

		case 2:
			return *((UINT16 *)&region->status[offset]) == 0xFFFF;
			break;

		case 4:
			return *((UINT32 *)&region->status[offset]) == 0xFFFFFFFF;
			break;
	}

	return 0;
}
*/

static UINT8 IsRegionOffsetValidBit(SearchInfo * search, SearchRegion * region, UINT32 offset)
{
	switch(kSearchByteIncrementTable[search->bytes])
	{
		case 1:
			return *((UINT8  *)&region->status[offset]) != 0;
			break;

		case 2:
			return *((UINT16 *)&region->status[offset]) != 0;
			break;

		case 4:
			return *((UINT32 *)&region->status[offset]) != 0;
			break;
	}

	return 0;
}

static void InvalidateRegionOffset(SearchInfo * search, SearchRegion * region, UINT32 offset)
{
	switch(kSearchByteIncrementTable[search->bytes])
	{
		case 1:
			*((UINT8  *)&region->status[offset]) = 0;
			break;

		case 2:
			*((UINT16 *)&region->status[offset]) = 0;
			break;

		case 4:
			*((UINT32 *)&region->status[offset]) = 0;
			break;
	}
}

static void InvalidateRegionOffsetBit(SearchInfo * search, SearchRegion * region, UINT32 offset, UINT32 invalidate)
{
	switch(kSearchByteIncrementTable[search->bytes])
	{
		case 1:
			*((UINT8  *)&region->status[offset]) &= ~invalidate;
			break;

		case 2:
			*((UINT16 *)&region->status[offset]) &= ~invalidate;
			break;

		case 4:
			*((UINT32 *)&region->status[offset]) &= ~invalidate;
			break;
	}
}

static void InvalidateEntireRegion(SearchInfo * search, SearchRegion * region)
{
	memset(region->status, 0, region->length);

	search->numResults -= region->numResults;
	region->numResults = 0;
}

static void InitializeNewSearch(SearchInfo * search)
{
	int	i;

	search->numResults = 0;

	for(i = 0; i < search->regionListLength; i++)
	{
		SearchRegion	* region = &search->regionList[i];

		if(region->flags & kRegionFlag_Enabled)
		{
			region->numResults = 0;

			memset(region->status, 0xFF, region->length);

			FillBufferFromRegion(region, region->first);

			memcpy(region->last, region->first, region->length);
		}
	}
}

static void UpdateSearch(SearchInfo * search)
{
	int	i;

	for(i = 0; i < search->regionListLength; i++)
	{
		SearchRegion	* region = &search->regionList[i];

		if(region->flags & kRegionFlag_Enabled)
		{
			FillBufferFromRegion(region, region->last);
		}
	}
}

static void DoSearch(SearchInfo * search)
{
	int	i, j;

	search->numResults = 0;

	if(search->bytes == kSearchSize_1Bit)
	{
		for(i = 0; i < search->regionListLength; i++)
		{
			SearchRegion	* region = &search->regionList[i];
			UINT32			lastAddress = region->length - kSearchByteIncrementTable[search->bytes] + 1;
			UINT32			increment = kSearchByteIncrementTable[search->bytes];

			region->numResults = 0;

			if(	(region->length < kSearchByteIncrementTable[search->bytes]) ||
				!region->flags & kRegionFlag_Enabled)
			{
				continue;
			}

			for(j = 0; j < lastAddress; j += increment)
			{
				UINT32	address;
				UINT32	lhs, rhs;

				address = region->address + j;

				if(IsRegionOffsetValidBit(search, region, j))
				{
					UINT32	validBits;

					lhs = ReadSearchOperandBit(search->lhs, search, region, address);
					rhs = ReadSearchOperandBit(search->rhs, search, region, address);

					validBits = DoSearchComparisonBit(search, lhs, rhs);

					InvalidateRegionOffsetBit(search, region, j, ~validBits);

					if(IsRegionOffsetValidBit(search, region, j))
					{
						search->numResults++;
						region->numResults++;
					}
				}
			}
		}
	}
	else
	{
		for(i = 0; i < search->regionListLength; i++)
		{
			SearchRegion	* region = &search->regionList[i];
			UINT32			lastAddress = region->length - kSearchByteIncrementTable[search->bytes] + 1;
			UINT32			increment = kSearchByteIncrementTable[search->bytes];

			region->numResults = 0;

			if(	(region->length < kSearchByteIncrementTable[search->bytes]) ||
				!region->flags & kRegionFlag_Enabled)
			{
				continue;
			}

			for(j = 0; j < lastAddress; j += increment)
			{
				UINT32	address;
				UINT32	lhs, rhs;

				address = region->address + j;

				if(IsRegionOffsetValid(search, region, j))
				{
					lhs = ReadSearchOperand(search->lhs, search, region, address);
					rhs = ReadSearchOperand(search->rhs, search, region, address);

					if(!DoSearchComparison(search, lhs, rhs))
					{
						InvalidateRegionOffset(search, region, j);
					}
					else
					{
						search->numResults++;
						region->numResults++;
					}
				}
			}
		}
	}
}

static UINT8 ** LookupHandlerMemory(UINT8 cpu, UINT32 address, UINT32 * outRelativeAddress)
{
	const struct Memory_WriteAddress	* mwa = Machine->drv->cpu[cpu].memory_write;

	while(!IS_MEMPORT_END(mwa))
	{
		if(!IS_MEMPORT_MARKER(mwa))
		{
			if(	(address >= mwa->start) &&
				(address <= mwa->end))
			{
				if(outRelativeAddress)
					*outRelativeAddress = address - mwa->start;

				return mwa->base;
			}
		}

		mwa++;
	}

	return NULL;
}

static UINT32 DoCPURead(UINT8 cpu, UINT32 address, UINT8 bytes, UINT8 swap)
{
	switch(bytes)
	{
		case 1:
				return	(cpunum_read_byte(cpu, address + 0) <<  0);

		case 2:
			if(swap)
			{
				return	(cpunum_read_byte(cpu, address + 0) <<  0) |
						(cpunum_read_byte(cpu, address + 1) <<  8);
			}
			else
			{
				return	(cpunum_read_byte(cpu, address + 0) <<  8) |
						(cpunum_read_byte(cpu, address + 1) <<  0);
			}
			break;

		case 3:
			if(swap)
			{
				return	(cpunum_read_byte(cpu, address + 0) <<  0) |
						(cpunum_read_byte(cpu, address + 1) <<  8) |
						(cpunum_read_byte(cpu, address + 2) << 16);
			}
			else
			{
				return	(cpunum_read_byte(cpu, address + 0) << 16) |
						(cpunum_read_byte(cpu, address + 1) <<  8) |
						(cpunum_read_byte(cpu, address + 2) <<  0);
			}
			break;

		case 4:
			if(swap)
			{
				return	(cpunum_read_byte(cpu, address + 0) <<  0) |
						(cpunum_read_byte(cpu, address + 1) <<  8) |
						(cpunum_read_byte(cpu, address + 2) << 16) |
						(cpunum_read_byte(cpu, address + 3) << 24);
			}
			else
			{
				return	(cpunum_read_byte(cpu, address + 0) << 24) |
						(cpunum_read_byte(cpu, address + 1) << 16) |
						(cpunum_read_byte(cpu, address + 2) <<  8) |
						(cpunum_read_byte(cpu, address + 3) <<  0);
			}
			break;
	}

	return 0;
}

static UINT32 DoMemoryRead(UINT8 * buf, UINT32 address, UINT8 bytes, UINT8 swap, CPUInfo * info)
{
	UINT32	data = 0;

	if(!info)
	{
		switch(bytes)
		{
			case 1:
				data = buf[address];
				break;

			case 2:
				data = *((UINT16 *)&buf[address]);

				if(swap)
				{
					data =	((data >> 8) & 0x00FF) |
							((data << 8) & 0xFF00);
				}
				break;

			case 4:
				data = *((UINT32 *)&buf[address]);

				if(swap)
				{
					data =	((data >> 24) & 0x000000FF) |
							((data >>  8) & 0x0000FF00) |
							((data <<  8) & 0x00FF0000) |
							((data << 24) & 0xFF000000);
				}
				break;

			default:
				info = &rawCPUInfo;
				goto generic;
		}

		return data;
	}

generic:

	if(swap)
	{
		UINT32	i;

		for(i = 0; i < bytes; i++)
			data |= buf[SwapAddress(address + i, bytes, info)] << (i * 8);
	}
	else
	{
		UINT32	i;

		for(i = 0; i < bytes; i++)
			data |= buf[SwapAddress(address + i, bytes, info)] << ((bytes - i - 1) * 8);
	}

	return data;
}

static void DoCPUWrite(UINT32 data, UINT8 cpu, UINT32 address, UINT8 bytes, UINT8 swap)
{
	switch(bytes)
	{
		case 1:
				cpunum_write_byte(cpu, address + 0, data & 0xFF);
			break;

		case 2:
			if(swap)
			{
				cpunum_write_byte(cpu, address + 0, (data >> 0) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >> 8) & 0xFF);
			}
			else
			{
				cpunum_write_byte(cpu, address + 0, (data >> 8) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >> 0) & 0xFF);
			}
			break;

		case 3:
			if(swap)
			{
				cpunum_write_byte(cpu, address + 0, (data >>  0) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >>  8) & 0xFF);
				cpunum_write_byte(cpu, address + 2, (data >> 16) & 0xFF);
			}
			else
			{
				cpunum_write_byte(cpu, address + 0, (data >> 16) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >>  8) & 0xFF);
				cpunum_write_byte(cpu, address + 2, (data >>  0) & 0xFF);
			}
			break;

		case 4:
			if(swap)
			{
				cpunum_write_byte(cpu, address + 0, (data >>  0) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >>  8) & 0xFF);
				cpunum_write_byte(cpu, address + 2, (data >> 16) & 0xFF);
				cpunum_write_byte(cpu, address + 3, (data >> 24) & 0xFF);
			}
			else
			{
				cpunum_write_byte(cpu, address + 0, (data >> 24) & 0xFF);
				cpunum_write_byte(cpu, address + 1, (data >> 16) & 0xFF);
				cpunum_write_byte(cpu, address + 2, (data >>  8) & 0xFF);
				cpunum_write_byte(cpu, address + 3, (data >>  0) & 0xFF);
			}
			break;

		default:
			log_cb(RETRO_LOG_ERROR, LOGPRE "DoCPUWrite: bad size (%d)\n", bytes);
			break;
	}
}

static void DoMemoryWrite(UINT32 data, UINT8 * buf, UINT32 address, UINT8 bytes, UINT8 swap, CPUInfo * info)
{
	if(!info)
	{
		switch(bytes)
		{
			case 1:
				buf[address] = data;
				break;

			case 2:
				if(swap)
				{
					data =	((data >> 8) & 0x00FF) |
							((data << 8) & 0xFF00);
				}

				*((UINT16 *)&buf[address]) = data;
				break;

			case 4:
				if(swap)
				{
					data =	((data >> 24) & 0x000000FF) |
							((data >>  8) & 0x0000FF00) |
							((data <<  8) & 0x00FF0000) |
							((data << 24) & 0xFF000000);
				}

				*((UINT32 *)&buf[address]) = data;
				break;

			default:
				info = &rawCPUInfo;
				goto generic;
		}

		return;
	}

generic:

	if(swap)
	{
		UINT32	i;

		for(i = 0; i < bytes; i++)
			buf[SwapAddress(address + i, bytes, info)] = data >> (i * 8);
	}
	else
	{
		UINT32	i;

		for(i = 0; i < bytes; i++)
			buf[SwapAddress(address + i, bytes, info)] = data >> ((bytes - i - 1) * 8);
	}
}

static UINT8 CPUNeedsSwap(UINT8 cpu)
{
	return cpuInfoList[cpu].endianness ^ 1;
}

static UINT8 RegionNeedsSwap(UINT8 region)
{
	CPUInfo	* temp = GetRegionCPUInfo(region);

	if(temp)
		return temp->endianness ^ 1;

	return 0;
}

static CPUInfo * GetCPUInfo(UINT8 cpu)
{
	return &cpuInfoList[cpu];
}

static CPUInfo * GetRegionCPUInfo(UINT8 region)
{
	if(	(region >= REGION_INVALID) &&
		(region < REGION_MAX))
		return &regionInfoList[region - REGION_INVALID];

	return NULL;
}

static UINT32 SwapAddress(UINT32 address, UINT8 dataSize, CPUInfo * info)
{
	switch(info->dataBits)
	{
		case 16:
			if(info->endianness == CPU_IS_BE)
				return BYTE_XOR_BE(address);
			else
				return BYTE_XOR_LE(address);

		case 32:
			if(info->endianness == CPU_IS_BE)
				return BYTE4_XOR_BE(address);
			else
				return BYTE4_XOR_LE(address);
	}

	return address;
}

static UINT32 ReadData(CheatAction * action)
{
	UINT8	parameter = EXTRACT_FIELD(action->type, LocationParameter);
	UINT8	bytes = EXTRACT_FIELD(action->type, BytesUsed) + 1;
	UINT8	swapBytes = EXTRACT_FIELD(action->type, Endianness);

	switch(EXTRACT_FIELD(action->type, LocationType))
	{
		case kLocation_Standard:
		{
			return DoCPURead(parameter, action->address, bytes, CPUNeedsSwap(parameter) ^ swapBytes);
		}
		break;

		case kLocation_MemoryRegion:
		{
			int		region = REGION_CPU1 + parameter;
			UINT8	* buf = memory_region(region);

			if(buf)
			{
				if(IsAddressInRange(action, memory_region_length(region)))
				{
					return DoMemoryRead(buf, action->address, bytes, RegionNeedsSwap(region) ^ swapBytes, GetRegionCPUInfo(region));
				}
			}
		}
		break;

		case kLocation_HandlerMemory:
		{
			UINT32	relativeAddress;
			UINT8	** buf;

			if(!action->cachedPointer)
			{
				action->cachedPointer = LookupHandlerMemory(parameter, action->address, &action->cachedOffset);
			}

			buf = action->cachedPointer;
			relativeAddress = action->cachedOffset;

			if(buf && *buf)
			{
				return DoMemoryRead(*buf, relativeAddress, bytes, CPUNeedsSwap(parameter) ^ swapBytes, GetCPUInfo(parameter));
			}
		}
		break;

		case kLocation_IndirectIndexed:
		{
			UINT32	address;
			INT32	offset = action->extendData;
			UINT8	cpu = (parameter >> 2) & 0x7;
			UINT8	addressBytes = (parameter & 0x3) + 1;
			CPUInfo	* info = GetCPUInfo(cpu);

			address = DoCPURead(cpu, action->address, addressBytes, CPUNeedsSwap(parameter) ^ swapBytes);
			if(info)
				address = DoShift(address, info->addressShift);
			address += offset;

			return DoCPURead(cpu, address, bytes, CPUNeedsSwap(parameter) ^ swapBytes);
		}
		break;

		case kLocation_Custom:
		{
			switch(parameter)
			{
				case kCustomLocation_Comment:
					break;

				case kCustomLocation_EEPROM:
				{
					int		length;
					UINT8	* buf;

					buf = EEPROM_get_data_pointer(&length);

					if(IsAddressInRange(action, length))
						return DoMemoryRead(buf, action->address, bytes, swapBytes, &rawCPUInfo);
				}
				break;
			}
		}
		break;

		default:
			break;
	}

	return 0;
}

static void WriteData(CheatAction * action, UINT32 data)
{
	UINT8	parameter = EXTRACT_FIELD(action->type, LocationParameter);
	UINT8	bytes = EXTRACT_FIELD(action->type, BytesUsed) + 1;
	UINT8	swapBytes = EXTRACT_FIELD(action->type, Endianness);

	switch(EXTRACT_FIELD(action->type, LocationType))
	{
		case kLocation_Standard:
		{
			DoCPUWrite(data, parameter, action->address, bytes, CPUNeedsSwap(parameter) ^ swapBytes);
		}
		break;

		case kLocation_MemoryRegion:
		{
			int		region = REGION_CPU1 + parameter;
			UINT8	* buf = memory_region(region);

			if(buf)
			{
				if(IsAddressInRange(action, memory_region_length(region)))
				{
					DoMemoryWrite(data, buf, action->address, bytes, RegionNeedsSwap(region) ^ swapBytes, GetRegionCPUInfo(region));
				}
			}
		}
		break;

		case kLocation_HandlerMemory:
		{
			UINT32	relativeAddress;
			UINT8	** buf;

			if(!action->cachedPointer)
			{
				action->cachedPointer = LookupHandlerMemory(parameter, action->address, &action->cachedOffset);
			}

			buf = action->cachedPointer;
			relativeAddress = action->cachedOffset;

			if(buf && *buf)
			{
				DoMemoryWrite(data, *buf, relativeAddress, bytes, CPUNeedsSwap(parameter) ^ swapBytes, GetCPUInfo(parameter));
			}
		}
		break;

		case kLocation_IndirectIndexed:
		{
			UINT32	address;
			INT32	offset = action->extendData;
			UINT8	cpu = (parameter >> 2) & 0x7;
			UINT8	addressBytes = (parameter & 0x3) + 1;
			CPUInfo	* info = GetCPUInfo(cpu);

			address = DoCPURead(cpu, action->address, addressBytes, CPUNeedsSwap(cpu) ^ swapBytes);
			if(info)
				address = DoShift(address, info->addressShift);
			address += offset;

			DoCPUWrite(data, cpu, address, bytes, CPUNeedsSwap(cpu) ^ swapBytes);
		}
		break;

		case kLocation_Custom:
		{
			switch(parameter)
			{
				case kCustomLocation_Comment:
					break;

				case kCustomLocation_EEPROM:
				{
					int		length;
					UINT8	* buf;

					buf = EEPROM_get_data_pointer(&length);

					if(IsAddressInRange(action, length))
						DoMemoryWrite(data, buf, action->address, bytes, swapBytes, &rawCPUInfo);
				}
				break;
			}
		}
		break;

		default:
			break;
	}
}

static void WatchCheatEntry(CheatEntry * entry, UINT8 associate)
{
	UINT32		i;
	CheatEntry	* associateEntry = NULL;

	if(!associate)
    return;
  
  associateEntry = entry;

	for(i = 0; i < entry->actionListLength; i++)
	{
		AddActionWatch(&entry->actionList[i], associateEntry);
	}
}

static void AddActionWatch(CheatAction * action, CheatEntry * entry)
{
	if(EXTRACT_FIELD(action->type, LocationType) == kLocation_Standard)
	{
		WatchInfo	* info = GetUnusedWatch();

		info->address =			action->address;
		info->cpu =				EXTRACT_FIELD(action->type, LocationParameter);
		info->displayType =		kWatchDisplayType_Hex;
		info->elementBytes =	kByteConversionTable[EXTRACT_FIELD(action->type, BytesUsed)];
		info->label[0] =		0;
		info->labelType =		kWatchLabel_None;
		info->linkedCheat =		entry;
		info->numElements =		1;
		info->skip =			0;
		info->linkedCheat =		entry;

		if(EXTRACT_FIELD(action->type, Type) == kType_Watch)
		{
			UINT32	typeParameter = EXTRACT_FIELD(action->type, TypeParameter);

			info->numElements = (action->data & 0xFF) + 1;

			info->skip = (action->data >> 8) & 0xFF;
			info->elementsPerLine = (action->data >> 16) & 0xFF;
			info->addValue = (action->data >> 24) & 0xFF;
			if(info->addValue & 0x80)
				info->addValue |= ~0xFF;

			if(action->extendData != 0xFFFFFFFF)
			{
				info->x += (action->extendData >> 16) & 0xFFFF;
				info->y += (action->extendData >>  0) & 0xFFFF;
			}

			if(	(typeParameter & 0x04) &&
				(entry->comment) &&
				(strlen(entry->comment) < 256))
			{
				info->labelType = kWatchLabel_String;
				strcpy(info->label, entry->comment);
			}

			info->displayType = typeParameter & 0x03;
		}
	}
}

static void RemoveAssociatedWatches(CheatEntry * entry)
{
	int	i;

	for(i = watchListLength - 1; i >= 0; i--)
	{
		WatchInfo	* info = &watchList[i];

		if(info->linkedCheat == entry)
			DeleteWatchAt(i);
	}
}

static void ResetAction(CheatAction * action)
{
	action->frameTimer = 0;
	action->lastValue = ReadData(action);
	action->flags &= ~kActionFlag_StateMask;
	action->flags |= kActionFlag_LastValueGood;
}

static void ActivateCheat(CheatEntry * entry)
{
	int	i;

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction	* action = &entry->actionList[i];

		ResetAction(action);

		if(EXTRACT_FIELD(action->type, Type) == kType_Watch)
			AddActionWatch(action, entry);
	}

	entry->flags |= kCheatFlag_Active;
}

static void DeactivateCheat(CheatEntry * entry)
{
	int	i;

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction	* action = &entry->actionList[i];

		if(	EXTRACT_FIELD(action->type, RestorePreviousValue) &&
			(action->flags & kActionFlag_LastValueGood))
		{
			WriteData(action, action->lastValue);

			action->flags &= ~kActionFlag_LastValueGood;
		}
	}

	RemoveAssociatedWatches(entry);

	entry->flags &= ~kCheatFlag_StateMask;
}

static void TempDeactivateCheat(CheatEntry * entry)
{
	if(entry->flags & kCheatFlag_Active)
	{
		int	i;

		for(i = 0; i < entry->actionListLength; i++)
		{
			CheatAction	* action = &entry->actionList[i];

			if(	EXTRACT_FIELD(action->type, RestorePreviousValue) &&
				(action->flags & kActionFlag_LastValueGood))
			{
				WriteData(action, action->lastValue);
			}
		}
	}
}

static void DoCheatOperation(CheatAction * action)
{
	UINT8	operation =	EXTRACT_FIELD(action->type, Operation) |
						(EXTRACT_FIELD(action->type, OperationExtend) << 2);

	switch(operation)
	{
		case kOperation_WriteMask:
		{
			UINT32	temp;

			if(action->flags & kActionFlag_IgnoreMask)
			{
				WriteData(action, action->data);
			}
			else
			{
				temp = ReadData(action);

				temp = (action->data & action->extendData) | (temp & ~action->extendData);

				WriteData(action, temp);
			}
		}
		break;

		case kOperation_AddSubtract:
		{
			INT32	temp, bound;

			if(action->flags & kActionFlag_IgnoreMask)
				return;

			temp = ReadData(action);

			/* OperationParameter field stores add/subtract*/
			if(TEST_FIELD(action->type, OperationParameter))
			{
				/* subtract*/

				bound = action->extendData + action->data;

				if(temp > bound)
					temp -= action->data;
			}
			else
			{
				/* add*/

				bound = action->extendData - action->data;

				if(temp < bound)
					temp += action->data;
			}

			WriteData(action, temp);
		}
		break;

		case kOperation_ForceRange:
		{
			UINT32	temp;

			if(action->flags & kActionFlag_IgnoreMask)
				return;

			temp = ReadData(action);

			if(	(temp < ((action->extendData >> 8) & 0xFF)) ||
				(temp > ((action->extendData >> 0) & 0xFF)))
			{
				temp = action->data;

				WriteData(action, temp);
			}
		}
		break;

		case kOperation_SetOrClearBits:
		{
			UINT32	temp;

			temp = ReadData(action);

			if(TEST_FIELD(action->type, OperationParameter))
			{
				/* clear*/

				temp &= ~action->data;
			}
			else
			{
				/* set*/

				temp |= action->data;
			}

			WriteData(action, temp);
		}
		break;

		case kOperation_None:
			break;

		default:
			break;
	}
}

static void DoCheatAction(CheatAction * action)
{
	UINT8	parameter = EXTRACT_FIELD(action->type, TypeParameter);

	if(action->flags & kActionFlag_OperationDone)
		return;

	if(	TEST_FIELD(action->type, Prefill) &&
		(!(action->flags & kActionFlag_PrefillDone)))
	{
		UINT32	prefillValue = kPrefillValueTable[EXTRACT_FIELD(action->type, Prefill)];

		if(!(action->flags & kActionFlag_PrefillWritten))
		{
			WriteData(action, prefillValue);

			action->flags |= kActionFlag_PrefillWritten;

			return;
		}
		else
		{
			if(ReadData(action) == prefillValue)
				return;

			action->flags |= kActionFlag_PrefillDone;
		}
	}

	switch(EXTRACT_FIELD(action->type, Type))
	{
		case kType_NormalOrDelay:
		{
			if(action->frameTimer >= (parameter * Machine->drv->frames_per_second))
			{
				action->frameTimer = 0;

				DoCheatOperation(action);

				if(TEST_FIELD(action->type, OneShot))
				{
					action->flags |= kActionFlag_OperationDone;
				}
			}
			else
			{
				action->frameTimer++;
			}
		}
		break;

		case kType_WaitForModification:
		{
			if(action->flags & kActionFlag_WasModified)
			{
				if(action->frameTimer <= 0)
				{
					DoCheatOperation(action);

					action->flags &= ~kActionFlag_WasModified;

					if(TEST_FIELD(action->type, OneShot))
					{
						action->flags |= kActionFlag_OperationDone;
					}
				}
				else
				{
					action->frameTimer--;
				}

				action->lastValue = ReadData(action);
			}
			else
			{
				UINT8	currentValue = ReadData(action);

				if(currentValue != action->lastValue)
				{
					action->frameTimer = parameter * Machine->drv->frames_per_second;

					action->flags |= kActionFlag_WasModified;
				}

				action->lastValue = currentValue;
			}
		}
		break;

		case kType_IgnoreIfDecrementing:
		{
			UINT8	currentValue = ReadData(action);

			if(currentValue != (action->lastValue - parameter))
			{
				DoCheatOperation(action);

				if(TEST_FIELD(action->type, OneShot))
				{
					action->flags |= kActionFlag_OperationDone;
				}
			}

			action->lastValue = currentValue;
		}
		break;

		case kType_Watch:
		default:
			break;
	}
}

static void DoCheatEntry(CheatEntry * entry)
{
	int	i;

	/* special handling for select cheats*/
	if(entry->flags & kCheatFlag_Select)
	{
		if(entry->flags & kCheatFlag_HasActivationKey)
		{
			if(code_pressed(entry->activationKey))
			{
				if(!(entry->flags & kCheatFlag_ActivationKeyPressed))
				{
					entry->selection++;

					if(entry->flags & kCheatFlag_OneShot)
					{
						if(entry->selection >= entry->actionListLength)
						{
							entry->selection = 1;

							if(entry->selection >= entry->actionListLength)
								entry->selection = 0;
						}
					}
					else
					{
						if(entry->selection >= entry->actionListLength)
						{
							entry->selection = 0;

							DeactivateCheat(entry);
						}
						else
						{
							ActivateCheat(entry);
						}
					}

					entry->flags |= kCheatFlag_ActivationKeyPressed;
				}
			}
			else
			{
				entry->flags &= ~kCheatFlag_ActivationKeyPressed;
			}
		}

		/* if a subcheat is selected and it's a legal index, handle it*/
		if(entry->selection && (entry->selection < entry->actionListLength))
		{
			DoCheatAction(&entry->actionList[entry->selection]);
		}
	}
	else
	{
		if(	(entry->flags & kCheatFlag_HasActivationKey) &&
			!(entry->flags & kCheatFlag_UserSelect))
		{
			if(code_pressed(entry->activationKey))
			{
				if(!(entry->flags & kCheatFlag_ActivationKeyPressed))
				{
					if(entry->flags & kCheatFlag_OneShot)
					{
						ActivateCheat(entry);
					}
					else
					{
						if(entry->flags & kCheatFlag_Active)
						{
							DeactivateCheat(entry);
						}
						else
						{
							ActivateCheat(entry);
						}
					}

					entry->flags |= kCheatFlag_ActivationKeyPressed;
				}
			}
			else
			{
				entry->flags &= ~kCheatFlag_ActivationKeyPressed;
			}
		}

		if(!(entry->flags & kCheatFlag_Active))
			return;

		/* update all actions*/
		for(i = 0; i < entry->actionListLength; i++)
		{
			DoCheatAction(&entry->actionList[i]);
		}

		/* if all actions are done, deactivate the cheat*/
		{
			UINT8	done = 1;

			for(i = 0; (i < entry->actionListLength) && done; i++)
				if(!(entry->actionList[i].flags & kActionFlag_OperationDone))
					done = 0;

			if(done)
			{
				DeactivateCheat(entry);
			}
		}
	}
}

static void UpdateAllCheatInfo(void)
{
	int	i;

	for(i = 0; i < cheatListLength; i++)
	{
		UpdateCheatInfo(&cheatList[i], 1);
	}
}

static void UpdateCheatInfo(CheatEntry * entry, UINT8 isLoadTime)
{
	int		isOneShot =	1;
	int		isNull =	1;
	int		flags =		0;
	int		i;

	flags = entry->flags & kCheatFlag_PersistentMask;

	if(	(EXTRACT_FIELD(entry->actionList[0].type, LocationType) == kLocation_Custom) &&
		(EXTRACT_FIELD(entry->actionList[0].type, LocationParameter) == kCustomLocation_Select))
		flags |= kCheatFlag_Select;

	for(i = 0; i < entry->actionListLength; i++)
	{
		CheatAction	* action =		&entry->actionList[i];
		/*int			isActionNull =	0;*/
		/*UINT32		size;*/
		UINT32		operation;
		UINT32		actionFlags = action->flags & kActionFlag_PersistentMask;

		/*size = EXTRACT_FIELD(action->type, BytesUsed);*/
		operation = EXTRACT_FIELD(action->type, Operation) | EXTRACT_FIELD(action->type, OperationExtend) << 2;

		if(	(EXTRACT_FIELD(action->type, LocationType) == kLocation_Custom) &&
			(EXTRACT_FIELD(action->type, LocationParameter) == kCustomLocation_Comment))
		{
			/*isActionNull = 1;*/
            isNull = 1;
		}
		else
		{
			isNull = 0;
		}

		if(!TEST_FIELD(action->type, OneShot))
			isOneShot = 0;

		if(TEST_FIELD(action->type, UserSelectEnable))
			flags |= kCheatFlag_UserSelect;

		if(EXTRACT_FIELD(action->type, LocationType) == kLocation_IndirectIndexed)
		{
			actionFlags |= kActionFlag_IgnoreMask;
		}
		else
		{
			if(isLoadTime)
			{
				/* check for mask == 0, fix*/
				if(	(operation == kOperation_WriteMask) &&
					(action->extendData == 0))
				{
					action->extendData = ~0;
				}
			}
		}

		action->flags = actionFlags;
	}

	if(isOneShot)
		flags |= kCheatFlag_OneShot;
	if(isNull)
		flags |= kCheatFlag_Null;

	entry->flags = (flags & kCheatFlag_InfoMask) | (entry->flags & ~kCheatFlag_InfoMask);

	if(isLoadTime)
		entry->flags &= ~kCheatFlag_Dirty;
}

static int IsAddressInRange(CheatAction * action, UINT32 length)
{
	UINT8	bytes = EXTRACT_FIELD(action->type, BytesUsed) + 1;

	return ((action->address + bytes) <= length);
}

static void BuildCPUInfoList(void)
{
	int	i;

	/* do regions*/
	{
		const struct RomModule *	traverse = rom_first_region(Machine->gamedrv);

		memset(regionInfoList, 0, sizeof(CPUInfo) * kRegionListLength);

		while(traverse)
		{
			if(ROMENTRY_ISREGION(traverse))
			{
				UINT8	regionType = ROMREGION_GETTYPE(traverse);

				/* non-cpu region?*/
				if(	(regionType >= REGION_GFX1) &&
					(regionType <= REGION_USER8))
				{
					CPUInfo	* info = &regionInfoList[regionType - REGION_INVALID];
					UINT32	length = memory_region_length(regionType);
					int		bitState = 0;

					info->type = regionType;
					info->dataBits = ROMREGION_GETWIDTH(traverse);

					info->addressBits = 0;
					info->addressMask = length;

					/* build address mask*/
					for(i = 0; i < 32; i++)
					{
						UINT32	mask = 1 << (31 - i);

						if(bitState)
						{
							info->addressMask |= mask;
						}
						else
						{
							if(info->addressMask & mask)
							{
								info->addressBits = 32 - i;
								bitState = 1;
							}
						}
					}

					info->addressCharsNeeded = info->addressBits >> 2;
					if(info->addressBits & 3)
						info->addressCharsNeeded++;

					info->endianness = ROMREGION_ISBIGENDIAN(traverse);
				}
			}

			traverse = rom_next_region(traverse);
		}
	}

	/* do CPUs*/
	{
		memset(cpuInfoList, 0, sizeof(CPUInfo) * MAX_CPU);

		for(i = 0; i < cpu_gettotalcpu(); i++)
		{
			CPUInfo	* info = &cpuInfoList[i];
			CPUInfo	* regionInfo = &regionInfoList[REGION_CPU1 + i - REGION_INVALID];

			int		type = Machine->drv->cpu[i].cpu_type;

			info->type = type;
			info->dataBits = cputype_databus_width(type);
			info->addressBits = cputype_address_bits(type);
			info->addressMask = 0xFFFFFFFF >> (32 - cputype_address_bits(type));

			info->addressCharsNeeded = info->addressBits >> 2;
			if(info->addressBits & 0x3)
				info->addressCharsNeeded++;

			info->endianness = (cputype_endianess(type) == CPU_IS_BE);

			switch(type)
			{
#if HAS_TMS34010
				case CPU_TMS34010:
					info->addressShift = 3;
					break;
#endif
#if HAS_TMS34020
				case HAS_TMS34020:
					info->addressShift = 3;
					break;
#endif
				default:
					info->addressShift = 0;
					break;
			}

			/* copy to region list*/
			memcpy(regionInfo, info, sizeof(CPUInfo));
		}
	}
}
