//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2014 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: TIASnd.hxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#ifndef TIASOUND_HXX
#define TIASOUND_HXX

#include "bspf.hxx"

/**
  This class implements a fairly accurate emulation of the TIA sound
  hardware.  This class uses code/ideas from z26 and MESS.

  Currently, the sound generation routines work at 31400Hz only.
  Resampling can be done by passing in a different output frequency.

  @author  Bradford W. Mott, Stephen Anthony, z26 and MESS teams
  @version $Id: TIASnd.hxx 2838 2014-01-17 23:34:03Z stephena $
*/
class TIASound
{
  public:
    /**
      Create a new TIA Sound object using the specified output frequency
    */
    TIASound(Int32 outputFrequency = 31400);

    /**
      Destructor
    */
    virtual ~TIASound();

  public:
    /**
      Reset the sound emulation to its power-on state
    */
    void reset();

    /**
      Set the frequency output samples should be generated at
    */
    void outputFrequency(Int32 freq);

    /**
      Selects the number of audio channels per sample.  There are two factors
      to consider: hardware capability and desired mixing.

      @param hardware  The number of channels supported by the sound system
      @param stereo    Whether to output the internal sound signals into 1
                       or 2 channels

      @return  Status of the channel configuration used
    */
    string channels(uInt32 hardware, bool stereo);

  public:
    /**
      Sets the specified sound register to the given value

      @param address Register address
      @param value Value to store in the register
    */
    void set(uInt16 address, uInt8 value);

    /**
      Gets the specified sound register's value

      @param address Register address
    */
    uInt8 get(uInt16 address) const;

    /**
      Create sound samples based on the current sound register settings
      in the specified buffer. NOTE: If channels is set to stereo then
      the buffer will need to be twice as long as the number of samples.

      @param buffer The location to store generated samples
      @param samples The number of samples to generate
    */
    void process(Int16* buffer, uInt32 samples);

    /**
      Set the volume of the samples created (0-100)
    */
    void volume(uInt32 percent);

  private:
    void polyInit(uInt8* poly, int size, int f0, int f1);

  private:
    // Definitions for AUDCx (15, 16)
    enum AUDCxRegister
    {
      SET_TO_1    = 0x00,  // 0000
      POLY4       = 0x01,  // 0001
      DIV31_POLY4 = 0x02,  // 0010
      POLY5_POLY4 = 0x03,  // 0011
      PURE        = 0x04,  // 0100
      PURE2       = 0x05,  // 0101
      DIV31_PURE  = 0x06,  // 0110
      POLY5_2     = 0x07,  // 0111
      POLY9       = 0x08,  // 1000
      POLY5       = 0x09,  // 1001
      DIV31_POLY5 = 0x0a,  // 1010
      POLY5_POLY5 = 0x0b,  // 1011
      DIV3_PURE   = 0x0c,  // 1100
      DIV3_PURE2  = 0x0d,  // 1101
      DIV93_PURE  = 0x0e,  // 1110
      POLY5_DIV3  = 0x0f   // 1111
    };

    enum {
      POLY4_SIZE = 0x000f,
      POLY5_SIZE = 0x001f,
      POLY9_SIZE = 0x01ff,
      DIV3_MASK  = 0x0c,
      AUDV_SHIFT = 10     // shift 2 positions for AUDV,
                          // then another 8 for 16-bit sound
    };

    enum ChannelMode {
      Hardware2Mono,    // mono sampling with 2 hardware channels
      Hardware2Stereo,  // stereo sampling with 2 hardware channels
      Hardware1         // mono/stereo sampling with only 1 hardware channel
    };

  private:
    // Structures to hold the 6 tia sound control bytes
    uInt8 myAUDC[2];    // AUDCx (15, 16)
    uInt8 myAUDF[2];    // AUDFx (17, 18)
    Int16 myAUDV[2];    // AUDVx (19, 1A)

    Int16 myVolume[2];  // Last output volume for each channel

    uInt8 myP4[2];      // Position pointer for the 4-bit POLY array
    uInt8 myP5[2];      // Position pointer for the 5-bit POLY array
    uInt16 myP9[2];     // Position pointer for the 9-bit POLY array

    uInt8 myDivNCnt[2]; // Divide by n counter. one for each channel
    uInt8 myDivNMax[2]; // Divide by n maximum, one for each channel
    uInt8 myDiv3Cnt[2]; // Div 3 counter, used for POLY5_DIV3 mode

    ChannelMode myChannelMode;
    Int32  myOutputFrequency;
    Int32  myOutputCounter;
    uInt32 myVolumePercentage;

    /*
      Initialize the bit patterns for the polynomials (at runtime).

      The 4bit and 5bit patterns are the identical ones used in the tia chip.
      Though the patterns could be packed with 8 bits per byte, using only a
      single bit per byte keeps the math simple, which is important for
      efficient processing.
    */
    uInt8 Bit4[POLY4_SIZE];
    uInt8 Bit5[POLY5_SIZE];
    uInt8 Bit9[POLY9_SIZE];

    /*
      The 'Div by 31' counter is treated as another polynomial because of
      the way it operates.  It does not have a 50% duty cycle, but instead
      has a 13:18 ratio (of course, 13+18 = 31).  This could also be
      implemented by using counters.
    */
    static const uInt8 Div31[POLY5_SIZE];
};

#endif
