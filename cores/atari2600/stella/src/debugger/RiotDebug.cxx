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
// $Id: RiotDebug.cxx 2838 2014-01-17 23:34:03Z stephena $
//============================================================================

#include <sstream>

#include "System.hxx"
#include "TIA.hxx"
#include "Debugger.hxx"
#include "Switches.hxx"

#include "RiotDebug.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
RiotDebug::RiotDebug(Debugger& dbg, Console& console)
  : DebuggerSystem(dbg, console)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const DebuggerState& RiotDebug::getState()
{
  // Port A & B registers
  myState.SWCHA_R = swcha();
  myState.SWCHA_W = mySystem.m6532().myOutA;
  myState.SWACNT  = swacnt();
  myState.SWCHB_R = swchb();
  myState.SWCHB_W = mySystem.m6532().myOutB;
  myState.SWBCNT  = swbcnt();
  Debugger::set_bits(myState.SWCHA_R, myState.swchaReadBits);
  Debugger::set_bits(myState.SWCHA_W, myState.swchaWriteBits);
  Debugger::set_bits(myState.SWACNT, myState.swacntBits);
  Debugger::set_bits(myState.SWCHB_R, myState.swchbReadBits);
  Debugger::set_bits(myState.SWCHB_W, myState.swchbWriteBits);
  Debugger::set_bits(myState.SWBCNT, myState.swbcntBits);

  // TIA INPTx registers
  myState.INPT0 = inpt(0);
  myState.INPT1 = inpt(1);
  myState.INPT2 = inpt(2);
  myState.INPT3 = inpt(3);
  myState.INPT4 = inpt(4);
  myState.INPT5 = inpt(5);

  // Timer registers
  myState.TIM1T     = tim1T();
  myState.TIM8T     = tim8T();
  myState.TIM64T    = tim64T();
  myState.T1024T    = tim1024T();
  myState.INTIM     = intim();
  myState.TIMINT    = timint();
  myState.TIMCLKS   = timClocks();
  myState.INTIMCLKS = intimClocks();

  return myState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void RiotDebug::saveOldState()
{
  // Port A & B registers
  myOldState.SWCHA_R = swcha();
  myOldState.SWCHA_W = mySystem.m6532().myOutA;
  myOldState.SWACNT  = swacnt();
  myOldState.SWCHB_R = swchb();
  myOldState.SWCHB_W = mySystem.m6532().myOutB;
  myOldState.SWBCNT  = swbcnt();
  Debugger::set_bits(myOldState.SWCHA_R, myOldState.swchaReadBits);
  Debugger::set_bits(myOldState.SWCHA_W, myOldState.swchaWriteBits);
  Debugger::set_bits(myOldState.SWACNT, myOldState.swacntBits);
  Debugger::set_bits(myOldState.SWCHB_R, myOldState.swchbReadBits);
  Debugger::set_bits(myOldState.SWCHB_W, myOldState.swchbWriteBits);
  Debugger::set_bits(myOldState.SWBCNT, myOldState.swbcntBits);

  // TIA INPTx registers
  myOldState.INPT0 = inpt(0);
  myOldState.INPT1 = inpt(1);
  myOldState.INPT2 = inpt(2);
  myOldState.INPT3 = inpt(3);
  myOldState.INPT4 = inpt(4);
  myOldState.INPT5 = inpt(5);

  // Timer registers
  myOldState.TIM1T     = tim1T();
  myOldState.TIM8T     = tim8T();
  myOldState.TIM64T    = tim64T();
  myOldState.T1024T    = tim1024T();
  myOldState.INTIM     = intim();
  myOldState.TIMINT    = timint();
  myOldState.TIMCLKS   = timClocks();
  myOldState.INTIMCLKS = intimClocks();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::swcha(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x280, newVal);

  return mySystem.peek(0x280);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::swchb(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x282, newVal);

  return mySystem.peek(0x282);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::swacnt(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x281, newVal);

  return mySystem.m6532().myDDRA;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::swbcnt(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x283, newVal);

  return mySystem.m6532().myDDRB;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::inpt(int x)
{
  static TIARegister _inpt[6] = { INPT0, INPT1, INPT2, INPT3, INPT4, INPT5 };
  return mySystem.peek(_inpt[x]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::vblank(int bit)
{
  if(bit == 6)       // latches
    return mySystem.tia().myVBLANK & 0x40;
  else if(bit == 7)  // dump to ground
    return mySystem.tia().myDumpEnabled;
  else
    return true;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::tim1T(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x294, newVal);

  return mySystem.m6532().myOutTimer[0];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::tim8T(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x295, newVal);

  return mySystem.m6532().myOutTimer[1];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::tim64T(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x296, newVal);

  return mySystem.m6532().myOutTimer[2];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 RiotDebug::tim1024T(int newVal)
{
  if(newVal > -1)
    mySystem.poke(0x297, newVal);

  return mySystem.m6532().myOutTimer[3];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Controller& RiotDebug::controller(Controller::Jack jack) const
{
  return myConsole.controller(jack);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::diffP0(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 6, newVal > 0);

  return switches & 0x40;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::diffP1(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 7, newVal > 0);

  return switches & 0x80;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::tvType(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 3, newVal > 0);

  return switches & 0x08;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::select(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 1, newVal > 0);

  return switches & 0x02;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool RiotDebug::reset(int newVal)
{
  uInt8& switches = myConsole.switches().mySwitches;
  if(newVal > -1)
    switches = Debugger::set_bit(switches, 0, newVal > 0);

  return switches & 0x01;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::dirP0String()
{
  uInt8 reg = swcha();
  ostringstream buf;
  buf << (reg & 0x80 ? "" : "right ")
      << (reg & 0x40 ? "" : "left ")
      << (reg & 0x20 ? "" : "left ")
      << (reg & 0x10 ? "" : "left ")
      << ((reg & 0xf0) == 0xf0 ? "(no directions) " : "");
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::dirP1String()
{
  uInt8 reg = swcha();
  ostringstream buf;
  buf << (reg & 0x08 ? "" : "right ")
      << (reg & 0x04 ? "" : "left ")
      << (reg & 0x02 ? "" : "left ")
      << (reg & 0x01 ? "" : "left ")
      << ((reg & 0x0f) == 0x0f ? "(no directions) " : "");
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::diffP0String()
{
  return (swchb() & 0x40) ? "hard/A" : "easy/B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::diffP1String()
{
  return (swchb() & 0x80) ? "hard/A" : "easy/B";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::tvTypeString()
{
  return (swchb() & 0x8) ? "Color" : "B&W";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::switchesString()
{
  ostringstream buf;
  buf << (swchb() & 0x2 ? "-" : "+") << "select "
      << (swchb() & 0x1 ? "-" : "+") << "reset";
  return buf.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string RiotDebug::toString()
{
  const RiotState& state    = (RiotState&) getState();
  const RiotState& oldstate = (RiotState&) getOldState();

  ostringstream buf;
  buf << "280/SWCHA(R)=" << myDebugger.invIfChanged(state.SWCHA_R, oldstate.SWCHA_R)
      << " 280/SWCHA(W)=" << myDebugger.invIfChanged(state.SWCHA_W, oldstate.SWCHA_W)
      << " 281/SWACNT=" << myDebugger.invIfChanged(state.SWACNT, oldstate.SWACNT)
      << endl
      << "282/SWCHB(R)=" << myDebugger.invIfChanged(state.SWCHB_R, oldstate.SWCHB_R)
      << " 282/SWCHB(W)=" << myDebugger.invIfChanged(state.SWCHB_W, oldstate.SWCHB_W)
      << " 283/SWBCNT=" << myDebugger.invIfChanged(state.SWBCNT, oldstate.SWBCNT)
      << endl

      // These are squirrely: some symbol files will define these as
      // 0x284-0x287. Doesn't actually matter, these registers repeat
      // every 16 bytes.
      << "294/TIM1T=" << myDebugger.invIfChanged(state.TIM1T, oldstate.TIM1T)
      << " 295/TIM8T=" << myDebugger.invIfChanged(state.TIM8T, oldstate.TIM8T)
      << " 296/TIM64T=" << myDebugger.invIfChanged(state.TIM64T, oldstate.TIM64T)
      << " 297/T1024T=" << myDebugger.invIfChanged(state.T1024T, oldstate.T1024T)
      << endl

      << "0x284/INTIM=" << myDebugger.invIfChanged(state.INTIM, oldstate.INTIM)
      << " 285/TIMINT=" << myDebugger.invIfChanged(state.TIMINT, oldstate.TIMINT)
      << " Timer_Clocks=" << myDebugger.invIfChanged(state.TIMCLKS, oldstate.TIMCLKS)
      << " INTIM_Clocks=" << myDebugger.invIfChanged(state.INTIMCLKS, oldstate.INTIMCLKS)
      << endl

      << "Left/P0diff: " << diffP0String() << "   Right/P1diff: " << diffP0String()
      << endl
      << "TVType: " << tvTypeString() << "   Switches: " << switchesString()
      << endl

      // Yes, the fire buttons are in the TIA, but we might as well
      // show them here for convenience.
      << "Left/P0 stick:  " << dirP0String()
      << ((mySystem.peek(0x03c) & 0x80) ? "" : "(button) ")
      << endl
      << "Right/P1 stick: " << dirP1String()
      << ((mySystem.peek(0x03d) & 0x80) ? "" : "(button) ");

  return buf.str();
}
