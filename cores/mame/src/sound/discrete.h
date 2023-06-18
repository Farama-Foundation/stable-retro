#ifndef _discrete_h_
#define _discrete_h_

/***********************************************************************
 *
 *  MAME - Discrete sound system emulation library
 *
 *  Written by Keith Wilkins (mame@esplexo.co.uk)
 *
 *  (c) K.Wilkins 2000
 *
 *  Coding started in November 2000
 *
 *  Additions/bugfix February 2003 - D.Renaud, F.Palazzolo, K.Wilkins
 *
 ***********************************************************************
 *
 * For good free text books on electronic theory check out:
 * http://www.ibiblio.org/obp/electricCircuits/
 *
 ***********************************************************************
 *
 * Unused/Unconnected input nodes should be set to NODE_NC (No Connect)
 *
 * Each node can have many inputs from either constants or other
 * nodes within the system.
 *
 * It should be remembered that the discrete sound system emulation
 * does not do individual device emulation, but instead does a function
 * emulation. So you will need to convert the schematic design into
 * a logic block representation.
 *
 * One node point may feed a number of inputs, for example you could
 * connect the output of a DISCRETE_SINEWAVE to the AMPLITUDE input
 * of another DISCRETE_SINEWAVE to amplitude modulate its output and
 * also connect it to the frequecy input of another to frequency
 * modulate its output, the combinations are endless....
 *
 * Consider the circuit below:
 *
 *  .--------.             .----------.                 .-------.
 *  |        |             |          |                 |       |
 *  | SQUARE |       Enable| SINEWAVE |                 |       |
 *  | WAVE   |-+---------->|  2000Hz  |---------------->|       |
 *  |        | |           |          |                 | ADDER |-->OUT
 *  | NODE01 | |           |  NODE02  |                 |       |
 *  '--------' |           '----------'              .->|       |
 *             |                                     |  |NODE06 |
 *             |  .------.   .------.   .---------.  |  '-------'
 *             |  |      |   |      |   |         |  |       ^
 *             |  | INV  |Ena| ADD  |Ena| SINEWVE |  |       |
 *             '->| ERT  |-->| ER2  |-->| 4000Hz  |--'  .-------.
 *                |      |ble|      |ble|         |     |       |
 *                |NODE03|   |NODE04|   | NODE05  |     | INPUT |
 *                '------'   '------'   '---------'     |       |
 *                                                      |NODE07 |
 *                                                      '-------'
 *
 * This should give you an alternating two tone sound switching
 * between the 2000Hz and 4000Hz sine waves at the frequency of the
 * square wave, with the memory mapped enable signal mapped onto NODE07
 * so discrete_sound_w(NODE_06,1) will enable the sound, and
 * discrete_sound_w(NODE_06,0) will disable the sound.
 *
 *  DISCRETE_SOUND_START(test_interface)
 *      DISCRETE_SQUAREWAVE(NODE_01,1,0.5,1,50,0,0)
 *      DISCRETE_SINEWAVE(NODE_02,NODE_01,2000,10000,0,0)
 *      DISCRETE_INVERT(NODE_03,NODE_01)
 *      DISCRETE_ADDER2(NODE_04,1,NODE_03,1)
 *      DISCRETE_SINEWAVE(NODE_05,NODE_04,4000,10000,0,0)
 *      DISCRETE_ADDER2(NODE_06,NODE_07,NODE_02,NODE_05)
 *      DISCRETE_INPUT(NODE_07,1)
 *      DISCRETE_OUTPUT(NODE_06,100)
 *  DISCRETE_SOUND_END
 *
 * To aid simulation speed it is preferable to use the enable/disable
 * inputs to a block rather than setting the output amplitude to zero
 *
 * Feedback loops are allowed BUT they will always feeback one time
 * step later, the loop over the netlist is only performed once per
 * deltaT so feedback occurs in the next deltaT step. This is not
 * the perfect solution but saves repeatedly traversing the netlist
 * until all nodes have settled.
 *
 * The best way to work out your system is generally to use a pen and
 * paper to draw a logical block diagram like the one above, it helps
 * to understand the system ,map the inputs and outputs and to work
 * out your node numbering scheme.
 *
 * Node numbers NODE_01 to NODE_299 are defined at present.
 *
 ***********************************************************************
 *
 * LIST OF CURRENTLY IMPLEMENTED DISCRETE BLOCKS
 * ---------------------------------------------
 *
 * DISCRETE_SOUND_START(STRUCTURENAME)
 * DISCRETE_SOUND_END
 *
 * DISCRETE_ADJUSTMENT(NODE,ENAB,MIN,MAX,LOGLIN,PORT)
 * DISCRETE_ADJUSTMENTX(NODE,ENAB,MIN,MAX,LOGLIN,PORT,PMIN,PMAX)
 * DISCRETE_CONSTANT(NODE,CONST0)
 * DISCRETE_INPUT(NODE,ADDR,MASK,INIT0)
 * DISCRETE_INPUTX(NODE,ADDR,MASK,GAIN,OFFSET,INIT0)
 * DISCRETE_INPUT_PULSE(NODE,INIT0,ADDR,MASK)
 *
 * DISCRETE_COUNTER(NODE,ENAB,RESET,CLK,BITS,DIR,INIT0,CLKTYPE)
 * DISCRETE_COUNTER_FIX(NODE,ENAB,RESET,FREQ,BITS,DIR,INIT0)
 * DISCRETE_LFSR_NOISE(NODE,ENAB,RESET,FREQ,AMPL,FEED,BIAS,LFSRTB)
 * DISCRETE_NOISE(NODE,ENAB,FREQ,AMP,BIAS)
 * DISCRETE_SAWTOOTHWAVE(NODE,ENAB,FREQ,AMP,BIAS,GRADIENT,PHASE)
 * DISCRETE_SINEWAVE(NODE,ENAB,FREQ,AMP,BIAS,PHASE)
 * DISCRETE_SQUAREWAVE(NODE,ENAB,FREQ,AMP,DUTY,BIAS,PHASE)
 * DISCRETE_SQUAREWFIX(NODE,ENAB,FREQ,AMP,DUTY,BIAS,PHASE)
 * DISCRETE_SQUAREWAVE2(NODE,ENAB,AMPL,T_OFF,T_ON,BIAS,TSHIFT)
 * DISCRETE_TRIANGLEWAVE(NODE,ENAB,FREQ,AMP,BIAS,PHASE)
 *
 * DISCRETE_SCHMITT_OSCILLATOR(NODE,ENAB,INP0,AMPL,TABLE)
 *
 * DISCRETE_ADDER2(NODE,IN0,IN1)
 * DISCRETE_ADDER3(NODE,IN0,IN1,IN2)
 * DISCRETE_ADDER4(NODE,IN0,IN1,IN2,IN3)
 * DISCRETE_CLAMP(NODE,ENAB,IN0,MIN,MAX,CLAMP)
 * DISCRETE_DIVIDE(NODE,ENAB,IN0,IN1)
 * DISCRETE_GAIN(NODE,IN0,GAIN)
 * DISCRETE_INVERT(NODE,IN0)
 * DISCRETE_MULTIPLY(NODE,ENAB,IN0,IN1)
 * DISCRETE_MULTADD(NODE,ENAB,INP0,INP1,INP2)
 * DISCRETE_ONESHOT(NODE,ENAB,TRIG,AMP,WIDTH)
 * DISCRETE_ONESHOTR(NODE,ENAB,TRIG,RESET,AMP,WIDTH)
 * DISCRETE_ONOFF(NODE,IN0,IN1)
 * DISCRETE_RAMP(NODE,ENAB,RAMP,GRAD,MIN,MAX,CLAMP)
 * DISCRETE_SAMPLHOLD(NODE,ENAB,INP0,CLOCK,CLKTYPE)
 * DISCRETE_SWITCH(NODE,ENAB,SWITCH,INP0,INP1)
 * DISCRETE_TRANSFORM2(NODE,ENAB,INP0,INP1,FUNCT)
 * DISCRETE_TRANSFORM3(NODE,ENAB,INP0,INP1,INP2,FUNCT)
 * DISCRETE_TRANSFORM4(NODE,ENAB,INP0,INP1,INP2,INP3,FUNCT)
 * DISCRETE_TRANSFORM5(NODE,ENAB,INP0,INP1,INP2,INP3,INP4,FUNCT)
 *
 * DISCRETE_COMP_ADDER(NODE,ENAB,DATA,TABLE)
 * DISCRETE_DAC_R1(NODE,ENAB,DATA,VDATA,LADDER)
 * DISCRETE_LADDER(NODE,ENAB,IN0,GAIN,LADDER)
 * DISCRETE_MIXER2(NODE,ENAB,IN0,IN1,INFO)
 * DISCRETE_MIXER3(NODE,ENAB,IN0,IN1,IN2,INFO)
 * DISCRETE_MIXER4(NODE,ENAB,IN0,IN1,IN2,IN3,INFO)
 * DISCRETE_MIXER5(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,INFO)
 * DISCRETE_MIXER6(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,INFO)
 * DISCRETE_MIXER7(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,INFO)
 * DISCRETE_MIXER8(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,IN7,INFO)
 * DISCRETE_MIXER(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,IN7,IN8,INFO)
 *
 * DISCRETE_LOGIC_INVERT(NODE,ENAB,INP0)
 * DISCRETE_LOGIC_AND(NODE,ENAB,INP0,INP1)
 * DISCRETE_LOGIC_AND3(NODE,ENAB,INP0,INP1,INP2)
 * DISCRETE_LOGIC_AND4(NODE,ENAB,INP0,INP1,INP2,INP3)
 * DISCRETE_LOGIC_NAND(NODE,ENAB,INP0,INP1)
 * DISCRETE_LOGIC_NAND3(NODE,ENAB,INP0,INP1,INP2)
 * DISCRETE_LOGIC_NAND4(NODE,ENAB,INP0,INP1,INP2,INP3)
 * DISCRETE_LOGIC_OR(NODE,ENAB,INP0,INP1)
 * DISCRETE_LOGIC_OR3(NODE,ENAB,INP0,INP1,INP2)
 * DISCRETE_LOGIC_OR4(NODE,ENAB,INP0,INP1,INP2,INP3)
 * DISCRETE_LOGIC_NOR(NODE,ENAB,INP0,INP1)
 * DISCRETE_LOGIC_NOR3(NODE,ENAB,INP0,INP1,INP2)
 * DISCRETE_LOGIC_NOR4(NODE,ENAB,INP0,INP1,INP2,INP3)
 * DISCRETE_LOGIC_XOR(NODE,ENAB,INP0,INP1)
 * DISCRETE_LOGIC_NXOR(NODE,ENAB,INP0,INP1)
 * DISCRETE_LOGIC_DFLIPFLOP(NODE,RESET,SET,CLK,INP)
 *
 * DISCRETE_CRFILTER(NODE,ENAB,IN0,RVAL,CVAL)
 * DISCRETE_CRFILTER_VREF(NODE,ENAB,IN0,RVAL,CVAL,VREF)
 * DISCRETE_FILTER1(NODE,ENAB,INP0,FREQ,TYPE)
 * DISCRETE_FILTER2(NODE,ENAB,INP0,FREQ,DAMP,TYPE)
 * DISCRETE_RCDISC(NODE,ENAB,IN0,RVAL,CVAL)
 * DISCRETE_RCDISC2(NODE,IN0,RVAL0,IN1,RVAL1,CVAL)
 * DISCRETE_RCFILTER(NODE,ENAB,IN0,RVAL,CVAL)
 * DISCRETE_RCFILTER_VREF(NODE,ENAB,IN0,RVAL,CVAL,VREF)
 *
 * DISCRETE_555_ASTABLE(NODE,RESET,AMPL,R1,R2,C,CTRLV,TYPE)
 * DISCRETE_555_CC(NODE,RESET,VIN,R,C,RBIAS,RGND,RDIS,OPTIONS)
 * DISCRETE_566(NODE,ENAB,VMOD,R,C,OPTIONS)
 *
 * DISCRETE_OUTPUT(OPNODE)
 *
 ***********************************************************************
 =======================================================================
 * from from disc_inp.c
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_ADJUSTMENT - Adjustable constant nodempile time
 *
 *                        .----------.
 *                        |          |
 *                        | ADJUST.. |-------->   Netlist node
 *                        |          |
 *                        '----------'
 *  Declaration syntax
 *
 *     DISCRETE_ADJUSTMENT(name of node,
 *                         enable node or static value,
 *                         static minimum value the node can take,
 *                         static maximum value the node can take,
 *                         log/linear scale 0=Linear !0=Logarithmic,
 *                         input port number of the potentiometer)
 *
 *  Example config line
 *
 *     DISCRETE_ADJUSTMENT(NODE_01,1,0.0,5.0,DISC_LINADJ,0,5)
 *
 *  Define an adjustment slider that takes a 0-100 input from input 
 *  port #5, scaling between 0.0 and 5.0. Adujstment scaling is Linear.
 *
 *      DISC_LOGADJ 1.0
 *      DISC_LINADJ 0.0
 *
 ***********************************************************************
 *
 * DISCRETE_CONSTANT - Single output, fixed at compile time
 *
 *                        .----------.
 *                        |          |
 *                        | CONSTANT |-------->   Netlist node
 *                        |          |
 *                        '----------'
 *  Declaration syntax
 *
 *     DISCRETE_CONSTANT(name of node, constant value)
 *
 *  Example config line
 *
 *     DISCRETE_CONSTANT(NODE_01, 100)
 *
 *  Define a node that has a constant value of 100
 *
 ***********************************************************************
 *
 * DISCRETE INPUT - Single output node, initialised at compile time and
 *                variable via memory based interface
 *
 * DISCRETE_INPUT_PULSE - Same as normal input node but the netlist
 *                        node output returns to INIT after a single
 *                        cycle of sound output. To allow for scenarios
 *                        whereby the register write pulse is used as
 *                        a reset to a system.
 *
 *                            .----------.
 *                      -----\|          |
 *        Memory Mapped ===== | INPUT(A) |---->   Netlist node
 *            Write     -----/|          |
 *                            '----------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_INPUT(name of node, initial value, addr, addrmask)
 *
 *  Example config line
 *
 *     DISCRETE_INPUT(NODE_02, 100,0x0000,0x000f)
 *
 *  Define a memory mapped input node called NODE_02 that has an
 *  initial value of 100. It is memory mapped into location 0x0000,
 *  0x0010, 0x0020 etc all the way to 0x0ff0. The exact size of memory
 *  space is defined by DSS_INPUT_SPACE in file disc_inp.c
 *
 *  The incoming address is first logicalled AND with the the ADDRMASK
 *  and then compared to the address, if there is a match on the write
 *  then the node is written to.
 *
 *  The memory space for discrete sound is 4096 locations (0x1000)
 *  the addr/addrmask values are used to setup a lookup table.
 *
 *  Can be written to with:    discrete_sound_w(0x0000, data);
 *
 ***********************************************************************
 =======================================================================
 * from from disc_wav.c
 * Generic modules
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_COUNTER     - up/down counter clocked externally.
 * DISCRETE_COUNTER_FIX - up/down counter clocked internally.
 *
 *  These counters count up/down from 0 to MAX.  When the enable is
 *  low, the output held at it's last value.  When reset is high,
 *  the reset value is loaded into the output.
 *
 *  Declaration syntax
 *
 *       where:         direction: 0 = down, 1 = up
 *                      clock type: toggle on 0/1
 *
 *     DISCRETE_COUNTER(name of node,
 *                      enable node or static value,
 *                      reset node or static value,
 *                      ext clock node,
 *                      max count static value,
 *                      direction node or static value,
 *                      reset value node or static value,
 *                      clock type static value)
 *
 *     DISCRETE_COUNTER_FIX(name of node,
 *                          enable node or static value,
 *                          reset node or static value,
 *                          clock frequency static value
 *                          max count static value,
 *                          direction node or static value
 *                          reset value node or static value)
 *
 ***********************************************************************
 *
 * DISCRETE_LFSR_NOISE - Noise waveform generator node, generates
 *                       psuedo random digital stream at the requested
 *                       clock frequency. Amplitude is 0/AMPLITUDE.
 *
 *  Declaration syntax
 *
 *     DISCRETE_LFSR_NOISE(name of node,
 *                         enable node or static value,
 *                         reset node or static value,
 *                         frequency node or static value,
 *                         amplitude node or static value,
 *                         forced infeed bit to shift reg,
 *                         bias node or static value,
 *                         LFSR noise descriptor structure)
 *
 *  The diagram below outlines the structure of the LFSR model.
 *
 *         .-------.
 *   FEED  |       |
 *   ----->|  F2   |<--------------------------------------------.
 *         |       |                                             |
 *         '-------'               BS - Bit Select               |
 *             |                   Fx - Programmable Function    |
 *             |        .-------.  PI - Programmable Inversion   |
 *             |        |       |                                |
 *             |  .---- | SR>>1 |<--------.                      |
 *             |  |     |       |         |                      |
 *             V  V     '-------'         |  .----               |
 *           .------.                     +->| BS |--. .------.  |
 *   BITMASK |      |    .-------------.  |  '----'  '-|      |  |
 *   ------->|  F3  |-+->| Shift Reg   |--+            |  F1  |--'
 *           |      | |  '-------------'  |  .----.  .-|      |
 *           '------' |         ^         '->| BS |--' '------'
 *                    |         |            '----'
 *   CLOCK            |     RESET VAL
 *   ---->            |                      .----.  .----.
 *                    '----------------------| BS |--| PI |--->OUTPUT
 *                                           '----'  '----'
 *
 ***********************************************************************
 *
 * DISCRETE_NOISE      - Noise waveform generator node, generates
 *                       random noise of the chosen frequency.
 *
 *                        .------------.
 *                        |            |
 *    ENABLE     -0------>|            |
 *                        |            |
 *    FREQUENCY  -1------>|   NOISE    |---->   Netlist node
 *                        |            |
 *    AMPLITUDE  -2------>|            |
 *                        |            |
 *    BIAS       -3------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_NOISE     (name of node,
 *                         enable node or static value,
 *                         frequency node or static value,
 *                         amplitude node or static value)
 *
 *  Example config line
 *
 *     DISCRETE_NOISE(NODE_03,1,5000,NODE_01,0)
 *
 ***********************************************************************
 *
 * DISCRETE_SAWTOOTHWAVE - Saw tooth shape waveform generator, rapid
 *                         rise and then graduated fall
 *
 *                        .------------.
 *                        |            |
 *    ENABLE     -0------>|            |
 *                        |            |
 *    FREQUENCY  -1------>|            |
 *                        |            |
 *    AMPLITUDE  -2------>|  SAWTOOTH  |----> Netlist Node
 *                        |    WAVE    |
 *    BIAS       -3------>|            |
 *                        |            |
 *    GRADIENT   -4------>|            |
 *                        |            |
 *    PHASE      -5------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_SAWTOOTHWAVE(name of node,
 *                         enable node or static value,
 *                         frequency node or static value,
 *                         amplitude node or static value,
 *                         dc bias value for waveform,
 *                         gradient of wave ==0 //// !=0 \\\\,
 *                         starting phase value in degrees)
 *
 *  Example config line
 *
 *     DISCRETE_SAWTOOTHWAVE(NODE_03,1,5000,NODE_01,0,0,90)
 *
 ***********************************************************************
 *
 * DISCRETE_SINEWAVE   - Sinewave waveform generator node, has four
 *                       input nodes FREQUENCY, AMPLITUDE, ENABLE and
 *                       PHASE, if a node is not connected it will
 *                       default to the initialised value in the macro
 *
 *                        .------------.
 *                        |            |
 *    ENABLE     -0------>|            |
 *                        |            |
 *    FREQUENCY  -1------>|            |
 *                        | SINEWAVE   |---->   Netlist node
 *    AMPLITUDE  -2------>|            |
 *                        |            |
 *    BIAS       -3------>|            |
 *                        |            |
 *    PHASE      -4------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_SINEWAVE  (name of node,
 *                         enable node or static value,
 *                         frequency node or static value,
 *                         amplitude node or static value,
 *                         dc bias value for waveform,
 *                         starting phase value in degrees)
 *
 *  Example config line
 *
 *     DISCRETE_SINEWAVE(NODE_03,NODE_01,NODE_02,10000,5000.0,90)
 *
 ***********************************************************************
 *
 * DISCRETE_SQUAREWAVE - Squarewave waveform generator node.
 * DISCRETE_SQUAREWFIX   Waveform is defined by frequency and duty
 *                       cycle.
 *
 *                        .------------.
 *                        |            |
 *    ENABLE     -0------>|            |
 *                        |            |
 *    FREQUENCY  -1------>|            |
 *                        |            |
 *    AMPLITUDE  -2------>| SQUAREWAVE |---->   Netlist node
 *                        |            |
 *    DUTY CYCLE -3------>|            |
 *                        |            |
 *    BIAS       -4------>|            |
 *                        |            |
 *    PHASE      -5------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_SQUAREWAVE(name of node,
 *                         enable node or static value,
 *                         frequency node or static value,
 *                         amplitude node or static value,
 *                         duty cycle node or static value,
 *                         dc bias value for waveform,
 *                         starting phase value in degrees)
 *
 *  Example config line
 *
 *     DISCRETE_SQUAREWAVE(NODE_03,NODE_01,NODE_02,100,50,0,90)
 *
 * NOTE: DISCRETE_SQUAREWFIX is used the same as DISCRETE_SQUAREWAVE.
 *       BUT... It does not stay in sync when you change the freq or
 *              duty values while enabled.  This should be used only
 *              when these values are stable while the wave is enabled.
 *              It takes up less CPU time then DISCRETE_SQUAREWAVE and
 *              should be used whenever possible.
 *
 ***********************************************************************
 *
 * DISCRETE_SQUAREWAVE2 - Squarewave waveform generator node.
 *                        Waveform is defined by it's off/on time
 *                        periods.
 *
 *                        .------------.
 *                        |            |
 *    ENABLE     -0------>|            |
 *                        |            |
 *    AMPLITUDE  -1------>|            |
 *                        |            |
 *    OFF TIME   -2------>| SQUAREWAVE |---->   Netlist node
 *                        |            |
 *    ON TIME    -3------>|            |
 *                        |            |
 *    BIAS       -4------>|            |
 *                        |            |
 *    TIME SHIFT -5------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_SQUAREWAVE2(name of node,
 *                          enable node or static value,
 *                          amplitude node or static value,
 *                          off time node or static value in seconds,
 *                          on time node or static value in seconds,
 *                          dc bias value for waveform,
 *                          starting phase value in seconds)
 *
 *  Example config line
 *
 *   DISCRETE_SQUAREWAVE2(NODE_03,NODE_01,NODE_02,0.01,0.001,0.0,0.001)
 *
 ***********************************************************************
 *
 * DISCRETE_TRIANGLEW  - Triagular waveform generator, generates
 *                       equal ramp up/down at chosen frequency
 *
 *                        .------------.
 *                        |            |
 *    ENABLE     -0------>|            |
 *                        |            |
 *    FREQUENCY  -1------>|  TRIANGLE  |---->   Netlist node
 *                        |    WAVE    |
 *    AMPLITUDE  -2------>|            |
 *                        |            |
 *    BIAS       -3------>|            |
 *                        |            |
 *    PHASE      -4------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_TRIANGLEWAVE(name of node,
 *                         enable node or static value,
 *                         frequency node or static value,
 *                         amplitude node or static value,
 *                         dc bias value for waveform,
 *                         starting phase value in degrees)
 *
 *  Example config line
 *
 *     DISCRETE_TRIANGLEWAVE(NODE_03,1,5000,NODE_01,0.0,0.0)
 *
 ***********************************************************************
 =======================================================================
 * from from disc_wav.c
 * Component specific modules
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_OP_AMP_OSCILLATOR - Various single power supply op-amp oscillator circuits
 *
 * Note: Set all unused components to 0.
 *
 *  Declaration syntax
 *
 *     DISCRETE_OP_AMP_OSCILLATOR(name of node,
 *                                enable node or static value,
 *                                address of dss_op_amp_osc_context structure)
 *
 *  Types:
 *
 *     DISC_OP_AMP_OSCILLATOR_1 | DISC_OP_AMP_IS_NORTON
 *          Basic Norton Op Amp Oscillator circuit.
 *
 *  vP >-.
 *       |         c
 *       Z     .---||----+---------------------------> DISC_OP_AMP_OSCILLATOR_IS_CAP
 *       Z r1  |         |
 *       Z     |   |\    |
 *       |     |   | \   |            |\   
 *       '-----+---|- \  |     r3     | \  
 *                 |   >-+----ZZZZ----|- \ 
 *                 |+ /               |   >--+-------> DISC_OP_AMP_OSCILLATOR_IS_SQR
 *             .---| /             .--|+ /   |
 *             |   |/        r5    |  | /    |
 *             |      vP >--ZZZZ---+  |/     |
 *             Z                   |         |
 *             Z r2                |   r4    |
 *             Z                   '--ZZZZ---+
 *             |                             |
 *             |                             |
 *             '-----------------------------'
 *
 ***********************************************************************
 *
 * DISCRETE_OP_AMP_VCOn - Various single power supply op-amp VCO circuits
 *                   (n = 1 or 2)
 *
 * Note: Set all unused components to 0.
 *
 *  Declaration syntax
 *
 *     DISCRETE_OP_AMP_VCOn(name of node,
 *                          enable node or static value,
 *                          modulation voltage 1 node or static value,
 *                          modulation voltage 2 node or static value,  [optional]
 *                          address of dss_op_amp_osc_context structure)
 *
 *  Types:
 *
 *     DISC_OP_AMP_OSCILLATOR_VCO_1
 *          Basic Op Amp Voltage Controlled Oscillator circuit.
 *          Note that this circuit has only 1 modulation voltage.
 *          So it is used only with DISCRETE_OP_AMP_VCO1.
 *
 *                               c
 *  .------------------------+---||----+---------------------------> DISC_OP_AMP_OSCILLATOR_IS_CAP
 *  |                        |         |
 *  |                        |   |\    |
 *  |              r1        |   | \   |            |\   
 *  | vMod1 >--+--ZZZZ-------+---|- \  |            | \  
 *  |          |                 |   >-+------------|- \ 
 *  |          |   r2            |+ /               |   >--+-------> DISC_OP_AMP_OSCILLATOR_IS_SQR
 *  Z          '--ZZZZ--+--------| /             .--|+ /   |
 *  Z r6                |        |/        r4    |  | /    |
 *  Z                   Z         vP/2 >--ZZZZ---+  |/     |
 *  |                   Z r5                     |         |
 * .----.               Z                        |   r3    |
 * | En |<--------.     |                        '--ZZZZ---+
 * '----'         |    gnd                                 |
 *    |           |                                        |
 *   gnd          '----------------------------------------'
 *
 *          --------------------------------------------------
 *           
 *     DISC_OP_AMP_OSCILLATOR_VCO_1 | DISC_OP_AMP_IS_NORTON
 *          Basic Norton Op Amp Voltage Controlled Oscillator circuit.
 *
 *                                             .---------------------------> DISC_OP_AMP_OSCILLATOR_IS_CAP
 *                                       c      |
 *               r6                  .---||----+
 *        vP >--ZZZZ---.             |         |         r5    |\
 *                     |             |   |\    |  vP >--ZZZZ-. | \
 *               r7    |   r1        |   | \   |             '-|- \
 *     vMod1 >--ZZZZ---+--ZZZZ-------+---|- \  |     r3        |   >--+-------> DISC_OP_AMP_OSCILLATOR_IS_SQR
 *                     |                 |   >-+----ZZZZ----+--|+ /   |
 *               r8    |   r2    .----.  |+ /               |  | /    |
 *     vMod2 >--ZZZZ---+--ZZZZ---| En |--| /                |  |/     |
 *                               '----'  |/                 |         |
 *                                 ^ ^                      |   r4    |
 *                                 | |                      '--ZZZZ---+
 *                                 | |                                |
 *                Enable >---------' |                                |
 *                                   '--------------------------------'
 *
 ***********************************************************************
 *
 * DISCRETE_SCHMITT_OSCILLATOR - Schmitt Inverter gate oscillator
 *
 *                  rFeedback
 *                .---ZZZ----.                   .--< Amplitude
 *                |          |                   |
 *                |  |\      |      .------.     |
 *           rIn  |  | \     | 0/1  | AND/ |    .-.
 *  INP0 >---ZZZ--+--|S >o---+----->|NAND/ |--->|*|-----> Netlist Node
 *                |  | /            |  OR/ |    '-'
 *                |  |/          .->| NOR  |
 *               ---             |  '------'
 *               --- C           |
 *                |              ^
 *               gnd          Enable
 *
 *  Declaration syntax
 *
 *     DISCRETE_SCHMITT_OSCILLATOR(name of node,
 *                                 enable node or static value,
 *                                 Input 0 node or static value,
 *                                 Amplitude node or static value,
 *                                 address of discrete_schmitt_osc_desc structure)
 *
 *  Input Options:
 *     DISC_SCHMITT_OSC_IN_IS_LOGIC
 *     DISC_SCHMITT_OSC_IN_IS_VOLTAGE
 *
 *  Enable Options:
 *     DISC_SCHMITT_OSC_ENAB_IS_AND
 *     DISC_SCHMITT_OSC_ENAB_IS_NAND
 *     DISC_SCHMITT_OSC_ENAB_IS_OR
 *     DISC_SCHMITT_OSC_ENAB_IS_NOR
 *
 ***********************************************************************
 =======================================================================
 * from from disc_wav.c
 * Not yet implemented
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_ADSR_ENV  - Attack Decay Sustain Release envelope generator
 *
 * Note: Not yet implemented.
 *
 *                        .------------.
 *                        |            |
 *    ENABLE     -0------>|            |
 *                        |    /\__    |
 *    TRIGGER    -1------>|   /    \   |---->   Netlist node
 *                        |    ADSR    |
 *    GAIN       -2------>|    Env     |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_ADSR_ENV  (name of node,
 *                         enable node or static value,
 *                         envelope gain node or static value,
 *                         envelope descriptor struct)
 *
 *  Example config line
 *
 *     DISCRETE_ADSR_ENV(NODE_3,1,NODE_21,1.0,&adsrdesc)
 *
 ***********************************************************************
 =======================================================================
 * from from disc_mth.c
 * Generic modules
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_ADDER      - Node addition function, available in three
 *                       lovelly flavours, ADDER2,ADDER3,ADDER4
 *                       that perform a summation of incoming nodes
 *
 *                        .------------.
 *                        |            |
 *    INPUT0     -0------>|            |
 *                        |            |
 *    INPUT1     -1------>|     |      |
 *                        |    -+-     |---->   Netlist node
 *    INPUT2     -2------>|     |      |
 *                        |            |
 *    INPUT3     -3------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_ADDERx    (name of node,
 *        (x=2/3/4)        enable node or static value,
 *                         input0 node or static value,
 *                         input1 node or static value,
 *                         input2 node or static value,  [optional]
 *                         input3 node or static value)  [optional]
 *
 *  Example config line
 *
 *     DISCRETE_ADDER2(NODE_03,1,NODE_12,-2000)
 *
 *  Always enabled, subtracts 2000 from the output of NODE_12
 *
 ***********************************************************************
 *
 * DISCRETE_CLAMP - Force a signal to stay within bounds MIN/MAX
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------>|            |
 *                        |            |
 *    INP0       -1------>|            |
 *                        |            |
 *    MIN        -2------>|   CLAMP    |---->   Netlist node
 *                        |            |
 *    MAX        -3------>|            |
 *                        |            |
 *    CLAMP      -4------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *        DISCRETE_CLAMP(name of node,
 *                       enable,
 *                       input node,
 *                       minimum node or static value,
 *                       maximum node or static value,
 *                       clamp node or static value when disabled)
 *
 *  Example config line
 *
 *     DISCRETE_CLAMP(NODE_9,NODE_10,NODE_11,2.0,10.0,5.0)
 *
 *  Node10 when not zero will allow clamp to operate forcing the value
 *  on the node output, to be within the MIN/MAX boundard. When enable
 *  is set to zero the node will output the clamp value
 *
 ***********************************************************************
 *
 * DISCRETE_DIVIDE     - Node division function
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------>|            |
 *                        |     o      |
 *    INPUT1     -1------>|    ---     |---->   Netlist node
 *                        |     o      |
 *    INPUT2     -2------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_DIVIDE    (name of node,
 *                         enable node or static value,
 *                         input0 node or static value,
 *                         input1 node or static value)
 *
 *  Example config line
 *
 *     DISCRETE_DIVIDE(NODE_03,1.0,NODE_12,50.0)
 *
 *  Always enabled, divides the input NODE_12 by 50.0. Note that a
 *  divide by zero condition will give a LARGE number output, it
 *  will not stall the machine or simulation. It will also attempt
 *  to write a divide by zero error to the Mame log if enabled.
 *
 ***********************************************************************
 *
 * DISCRETE_LOGIC_INVERT - Logic invertor
 * DISCRETE_LOGIC_AND  - Logic AND gate (3 & 4 input also available)
 * DISCRETE_LOGIC_NAND - Logic NAND gate (3 & 4 input also available)
 * DISCRETE_LOGIC_OR   - Logic OR gate (3 & 4 input also available)
 * DISCRETE_LOGIC_NOR  - Logic NOR gate (3 & 4 input also available)
 * DISCRETE_LOGIC_XOR  - Logic XOR gate
 * DISCRETE_LOGIC_NXOR - Logic NXOR gate
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------>|            |
 *                        |            |
 *    INPUT0     -0------>|            |
 *                        |   LOGIC    |
 *    [INPUT1]   -1------>|  FUNCTION  |---->   Netlist node
 *                        |    !&|^    |
 *    [INPUT2]   -2------>|            |
 *                        |            |
 *    [INPUT3]   -3------>|            |
 *                        |            |
 *    [] - Optional       '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_LOGIC_XXXn(name of node,
 *      (X=INV/AND/etc)    enable node or static value,
 *      (n=Blank/2/3)      input0 node or static value,
 *                         [input1 node or static value],
 *                         [input2 node or static value],
 *                         [input3 node or static value])
 *
 *  Example config lines
 *
 *     DISCRETE_LOGIC_INVERT(NODE_03,1,NODE_12)
 *     DISCRETE_LOGIC_AND(NODE_03,1,NODE_12,NODE_13)
 *     DISCRETE_LOGIC_NOR4(NODE_03,1,NODE_12,NODE_13,NODE_14,NODE_15)
 *
 *  Node output is always either 0.0 or 1.0 any input value !=0.0 is
 *  taken as a logic 1.
 *
 ***********************************************************************
 *
 * DISCRETE_LOGIC_DFLIPFLOP - Standard D-type flip-flop
 *
 *    /SET       -2 ------------.
 *                              v
 *                        .-----o------.
 *                        |            |
 *    INPUT      -4 ----->|            |
 *                        |            |
 *                        |  FLIPFLOP  |---->    Netlist node
 *                        |            |
 *    CLOCK      -3 ----->|            |
 *                        |            |
 *                        '-----o------'
 *                              ^
 *    /RESET     -1 ------------'
 *
 *  Declaration syntax
 *
 *       DISCRETE_LOGIC_DFLIPFLOP(name of node,
 *                                enable node or static value,
 *                                reset node or static value,
 *                                set node or static value,
 *                                clock node,
 *                                input node or static value)
 *
 *  Example config line
 *
 *     DISCRETE_LOGIC_DFLIPFLOP(NODE_7,1,NODE_17,0,NODE_13,1)
 *
 *  A flip-flop that clocks a logic 1 through on the rising edge of
 *  NODE_13. A logic 1 on NODE_17 resets the output to 0.
 *
 ***********************************************************************
 *
 * DISCRETE_GAIN       - Node multiplication function output is equal
 * DISCRETE_MULTIPLY     to INPUT0 * INPUT1
 * DISCRETE_MULTADD      to (INPUT0 * INPUT1) + INPUT 2
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------>|            |
 *                        |            |
 *    INPUT0     -1------>|     \|/    |
 *                        |     -+-    |---->   Netlist node
 *    INPUT1     -2------>|     /|\    |
 *                        |            |
 *    INPUT2     -3------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_MULTIPLY  (name of node,
 *                         enable node or static value,
 *                         input0 node or static value,
 *                         input1 node or static value)
 *
 *     DISCRETE_MULTADD   (name of node,
 *                         enable node or static value,
 *                         input0 node or static value,
 *                         input1 node or static value,
 *                         input2 node or static value)
 *
 *     DISCRETE_GAIN      (name of node,
 *                         input0 node or static value,
 *                         static value for gain)
 *  Example config line
 *
 *     DISCRETE_GAIN(NODE_03,NODE_12,112.0)
 *
 *  Always enabled, multiplies the input NODE_12 by 112.0
 *
 ***********************************************************************
 *
 * DISCRETE_ONESHOT    - Monostable multivibrator, no reset
 * DISCRETE_ONESHOTR   - Monostable multivibrator, with reset
 *
 *  Declaration syntax
 *
 *     DISCRETE_ONESHOT   (name of node,
 *                         trigger node,
 *                         amplitude node or static value,
 *                         width (in seconds) node or static value,
 *                         type of oneshot static value)
 *
 *     DISCRETE_ONESHOTR  (name of node,
 *                         reset node or static value,
 *                         trigger node,
 *                         amplitude node or static value,
 *                         width (in seconds) node or static value,
 *                         type of oneshot static value)
 *
 *  Types:
 *
 *     DISC_ONESHOT_FEDGE    0x00 - trigger on falling edge
 *     DISC_ONESHOT_REDGE    0x01 - trigger on rising edge
 *     DISC_ONESHOT_NORETRIG 0x00 - non-retriggerable
 *     DISC_ONESHOT_RETRIG   0x02 - retriggerable
 *     DISC_OUT_ACTIVE_LOW   0x04 - output active low
 *     DISC_OUT_ACTIVE_HIGH  0x00 - output active high
 *
 *  NOTE: A width of 0 seconds will output a pulse of 1 sample.
 *        This is usefull for a guaranteed minimun pulse, regardless
 *        of the sample rate.
 *
 ***********************************************************************
 *
 * DISCRETE_RAMP - Ramp up/down circuit with clamps & reset
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------>| FREE/CLAMP |
 *                        |            |
 *    RAMP       -1------>| FW/REV     |
 *                        |            |
 *    GRAD       -2------>| Grad/sec   |
 *                        |            |---->   Netlist node
 *    START      -3------>| Start clamp|
 *                        |            |
 *    END        -4------>| End clamp  |
 *                        |            |
 *    CLAMP      -5------>| off clamp  |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *         DISCRETE_RAMP(name of node,
 *                       enable,
 *                       ramp forward/reverse node (or value),
 *                       gradient node (or static value),
 *                       start node or static value,
 *                       end node or static value,
 *                       clamp node or static value when disabled)
 *
 *  Example config line
 *
 *     DISCRETE_RAMP(NODE_9,NODE_10,NODE_11,10.0,-10.0,10.0,0)
 *
 *  Node10 when not zero will allow ramp to operate, when 0 then output
 *  is clamped to clamp value specified. Node11 ramp when 0 change
 *  gradient from start to end. 1 is reverse. Output is clamped to max-
 *  min values. Gradient is specified in change/second.
 *
 ***********************************************************************
 *
 * DISCRETE_SAMPHOLD - Sample & Hold circuit
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------>|            |
 *                        |            |
 *    INP0       -1------>|   SAMPLE   |
 *                        |     &      |----> Netlist node
 *    CLOCK      -2------>|    HOLD    |
 *                        |            |
 *    CLKTYPE    -3------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_SAMPHOLD(name of node,
 *                       enable,
 *                       input node,
 *                       clock node or static value,
 *                       input clock type)
 *
 *  Example config line
 *
 *     DISCRETE_SAMPHOLD(NODE_9,1,NODE_11,NODE_12,DISC_SAMPHOLD_REDGE)
 *
 *  Node9 will sample the input node 11 on the rising edge (REDGE) of
 *  the input clock signal of node 12.
 *
 *   DISC_SAMPHOLD_REDGE  - Rising edge clock
 *   DISC_SAMPHOLD_FEDGE  - Falling edge clock
 *   DISC_SAMPHOLD_HLATCH - Output is latched whilst clock is high
 *   DISC_SAMPHOLD_LLATCH - Output is latched whilst clock is low
 *
 ***********************************************************************
 *
 * DISCRETE_SWITCH     - Node switch function, output node is switched
 *                       by switch input to take one node/contst or
 *                       other. Can be nodes or constants.
 *
 *    SWITCH     -0--------------.
 *                               V
 *                        .------------.
 *                        |      |     |
 *    INPUT0     -1------}|----o       |
 *                        |       .--- |---->   Netlist node
 *    INPUT1     -2------>|----o /     |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_SWITCH    (name of node,
 *                         enable node or static value,
 *                         switch node or static value,
 *                         input0 node or static value,
 *                         input1 node or static value)
 *
 *  Example config line
 *
 *     DISCRETE_SWITCH(NODE_03,1,NODE_10,NODE_90,5.0)
 *
 *  Always enabled, NODE_10 switches output to be either NODE_90 or
 *  constant value 5.0. Switch==0 inp0=output else inp1=output
 *
 ***********************************************************************
 *
 * DISCRETE_TRANSFORMn - Node arithmatic logic (postfix arithmatic)
 *     (n=2,3,4,5)
 *                        .------------.
 *                        |            |
 *    ENAB       -0------>|            |
 *                        |            |
 *    INPUT0     -1------>|            |
 *                        |            |
 *    INPUT1     -2------>|  Postfix   |
 *                        |   stack    |----> Netlist node
 *    INPUT2     -3------>|   maths    |
 *                        |            |
 *    INPUT3     -4------>|            |
 *                        |            |
 *    INPUT4     -5------>|            |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_TRANSFORMn(name of node,
 *                         enable node or static value,
 *                         input0 node or static value,
 *                         input1 node or static value,
 *                         input2 node or static value,  [optional]
 *                         input3 node or static value,  [optional]
 *                         input4 node or static value,  [optional]
 *                         maths string)
 *
 *  Example config line
 *
 *  DISCRETE_TRANSFORM4(NODE_12,1,NODE_22,50.0,120.0,33.33,"01*2+3/")
 *
 *  Arithmetic uses stack based arithmetic similar to Forth, the maths
 *  has 5 registers 0-4 and various arithmetic operations. The math
 *  string is processed from left to right in the following manner:
 *   0 - Push input 0 to stack
 *   1 - Push input 1 to stack
 *   2 - Push input 2 to stack
 *   3 - Push input 3 to stack
 *   4 - Push input 4 to stack
 *   - - Pop two values from stack, subtract and push result to stack
 *   + - Pop two values from stack, add and push result to stack
 *   / - Pop two values from stack, divide and push result to stack
 *   * - Pop two values from stack, multiply and push result to stack
 *   i - Pop one value from stack, multiply -1 and push result to stack
 *   ! - Pop one value from stack, logical invert, push result to stack
 *   = - Pop two values from stack, logical = and push result to stack
 *   > - Pop two values from stack, logical > and push result to stack
 *   < - Pop two values from stack, logical < and push result to stack
 *   & - Pop two values from stack, binary AND and push result to stack
 *   | - Pop two values from stack, binary OR and push result to stack
 *   ^ - Pop two values from stack, binary XOR and push result to stack
 *
 ***********************************************************************
 =======================================================================
 * from from disc_mth.c
 * Component specific modules
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_COMP_ADDER - Selecatable parallel component adder.
 *                       The total netlist out will be the sum of all
 *                       components selected in parallel.
 *                       Set cDefault to 0 if not used.
 *
 *      cDefault  >---xx---.
 *      data&0x01 >---xx---+
 *      data&0x02 >---xx---+
 *      data&0x04 >---xx---+
 *      data&0x08 >---xx---+-----> netlist node
 *      data&0x10 >---xx---+
 *      data&0x20 >---xx---+
 *      data&0x40 >---xx---+
 *      data&0x80 >---xx---'
 *
 *  Declaration syntax
 *
 *     DISCRETE_COMP_ADDER(name of node,
 *                         enable node or static value,
 *                         data node (static value is useless),
 *                      address of discrete_comp_adder_table structure)
 *
 *  Circuit Types:
 *     DISC_COMP_P_CAPACITOR - parallel capacitors
 *     DISC_COMP_P_RESISTOR  - parallel resistors
 *
 ***********************************************************************
 *
 * DISCRETE_DAC_R1 - R1 ladder DAC with cap smoothing and external bias
 *
 *                           rBias
 * data&0x01 >--/\R0/\--+-----/\/\----< vBias
 * data&0x02 >--/\R1/\--|
 * data&0x04 >--/\R2/\--|
 * data&0x08 >--/\R3/\--|
 * data&0x10 >--/\R4/\--|
 * data&0x20 >--/\R5/\--|
 * data&0x40 >--/\R6/\--|
 * data&0x80 >--/\R7/\--+-------------+-----> Netlist node
 *                      |             |
 *                      Z            ---
 *                      Z rGnd       --- cFilter
 *                      |             |
 *                     gnd           gnd
 *
 * NOTES: rBias and vBias are used together.  If not needed they should
 *        be set to 0.  If used, they should both have valid values.
 *        rGnd and cFilter should be 0 if not needed.
 *        A resistor value should be properly set for each resistor
 *        up to the ladder length.  Remember 0 is a short circuit.
 *        The data node is bit mapped to the ladder. valid int 0-255.
 *        TTL logic 0 is actually 0.2V but I use 0V.  The other parts
 *        have a tolerance that more then makes up for this.
 *
 *  Declaration syntax
 *
 *     DISCRETE_DAC_R1(name of node,
 *                     enable node or static value,
 *                     data node (static value is useless),
 *                     vData node or static value (vON),
 *                     address of discrete_dac_r1_ladder structure)
 *
 ***********************************************************************
 *
 * DISCRETE_MIXER - Mixes multiple input signals.
 *
 * Note: Set all unused components to 0.
 *
 *  Declaration syntax
 *
 *     DISCRETE_MIXERx(name of node,
 *      (x = 2 to 8)   enable node or static value,
 *                     input 0 node,
 *                     input 1 node,
 *                     input 2 node,  (if used)
 *                     input 3 node,  (if used)
 *                     input 4 node,  (if used)
 *                     input 5 node,  (if used)
 *                     input 6 node,  (if used)
 *                     input 7 node,  (if used)
 *                     address of discrete_mixer_info structure)
 *
 *  Types:
 *
 *     DISC_MIXER_IS_RESISTOR
 *
 *         r[0]   c[0]
 *  IN0 >--zzzz----||---.
 *                      |
 *         r[1]   c[1]  |
 *  IN1 >--zzzz----||---+--------.
 *   .      .      .    |        |      cAmp
 *   .      .      .    |        Z<------||---------> Netlist Node
 *   .      .      .    |        Z
 *   .     r[7]   c[7]  |        Z rF
 *  IN7 >--zzzz----||---+        |
 *                      |        |
 *                     ---       |
 *                  cF ---       |
 *                      |        |
 *                     gnd      gnd
 *
 *  Note: The variable resistor is used in it's full volume position.
 *        MAME's built in volume is used for adjustment.
 *
 *          --------------------------------------------------
 *
 *     DISC_MIXER_IS_OP_AMP
 *
 *                                    cF
 *                               .----||---.
 *                               |         |
 *         r[0]   c[0]           |    rF   |
 *  IN0 >--zzzz----||---.        +---ZZZZ--+
 *                      |        |         |
 *         r[1]   c[1]  |   rI   |  |\     |
 *  IN1 >--zzzz----||---+--zzzz--+  | \    |
 *   .      .      .    |        '--|- \   |  cAmp
 *   .      .      .    |           |   >--+---||-----> Netlist Node
 *   .      .      .    |        .--|+ /
 *   .     r[7]   c[7]  |        |  | /
 *  IN7 >--zzzz----||---'        |  |/
 *                               |
 *  vRef >-----------------------'
 *
 * Note: rI is not always used and should then be 0.
 *
 ***********************************************************************
 =======================================================================
 * from from disc_flt.c
 * Generic modules
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_FILTER1
 *
 *  Declaration syntax
 *
 *     DISCRETE_FILTER1(name of node,
 *                      enable node or static value,
 *                      input node,
 *                      filter center frequency static value,
 *                      filter type static value)
 *
 ***********************************************************************
 *
 * DISCRETE_FILTER2
 *
 *  Declaration syntax
 *
 *     DISCRETE_FILTER2(name of node,
 *                      enable node or static value,
 *                      input node,
 *                      filter center frequency static value,
 *                      damp static value,
 *                      filter type static value)
 *
 ***********************************************************************
 =======================================================================
 * from from disc_flt.c
 * Component specific modules
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_CRFILTER - Simple single pole CR filter network (vRef = 0)
 * DISCRETE_CRFILTER_VREF - Same but refrenced to vRef not 0V
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------}| CR FILTER  |
 *                        |            |
 *    INPUT1     -1------}| --| |-+--  |
 *                        |   C   |    |----}   Netlist node
 *    RVAL       -2------}|       Z    |
 *                        |       Z R  |
 *    CVAL       -3------}|       |    |
 *                        |      vRef  |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_CRFILTER(name of node,
 *                       enable
 *                       input node (or value)
 *                       resistor value in OHMS
 *                       capacitor value in FARADS)
 *
 *     DISCRETE_CRFILTER_VREF(name of node,
 *                            enable
 *                            input node (or value)
 *                            resistor value in OHMS
 *                            capacitor value in FARADS,
 *                            vRef static value)
 *
 *  Example config line
 *
 *     DISCRETE_CRFILTER(NODE_11,1,NODE_10,100,1e-6)
 *
 *  Defines an always enabled CR filter with a 100R & 1uF network
 *  the input is fed from NODE_10.
 *
 *  This can be also thought of as a high pass filter with a 3dB cutoff
 *  at:
 *                                  1
 *            Fcuttoff =      --------------
 *                            2*Pi*RVAL*CVAL
 *
 *  (3dB cutoff is where the output power has dropped by 3dB ie Half)
 *
 ***********************************************************************
 *
 *  DISCRETE_OP_AMP_FILTER - Various Op Amp Filters.
 *
 * Note: Set all unused components to 0.
 *       vP and vN are bias voltages if needed.
 *       vRef is 0 if Gnd.
 *       NORTON Op Amps are not supported (yet).
 *
 *  Declaration syntax
 *
 *      DISCRETE_OP_AMP_FILTER(name of node,
 *                             enable node or static value,
 *                             input 1 node or static value,
 *                             input 2 node or static value,
 *                             type static value,
 *                             address of discrete_op_amp_filt_info)
 *
 *  Types:
 *
 *     DISC_OP_AMP_FILTER_IS_LOW_PASS_1
 *          First Order Low Pass Filter
 *
 *          rP                  c1
 *   vP >--zzzz--.      .-------||---------.
 *               |      |                  |
 *          r1   |      |       rF         |
 *  IN0 >--zzzz--+      +------zzzz--------+
 *               |      |                  |
 *          r2   |      |           |\     |
 *  IN1 >--zzzz--+------+--------+  | \    |
 *               |               '--|- \   |
 *          rN   |                  |   >--+----------> Netlist Node
 *   vN >--zzzz--'               .--|+ /
 *                               |  | /
 *  vRef >-----------------------'  |/
 *
 *          --------------------------------------------------
 *
 *     DISC_OP_AMP_FILTER_IS_HIGH_PASS_1
 *          First Order High Pass Filter
 *
 *          rP
 *   vP >--zzzz--.
 *               |
 *          r1   |              rF
 *  IN0 >--zzzz--+      .------zzzz--------.
 *               |      |                  |
 *          r2   |  c1  |           |\     |
 *  IN1 >--zzzz--+--||--+--------+  | \    |
 *               |               '--|- \   |
 *          rN   |                  |   >--+----------> Netlist Node
 *   vN >--zzzz--'               .--|+ /
 *                               |  | /
 *  vRef >-----------------------'  |/
 *
 *          --------------------------------------------------
 *
 *     DISC_OP_AMP_FILTER_IS_BAND_PASS_1
 *          First Order Band Pass Filter
 *
 *          rP                  c1
 *   vP >--zzzz--.      .-------||---------.
 *               |      |                  |
 *          r1   |      |       rF         |
 *  IN0 >--zzzz--+      +------zzzz--------+
 *               |      |                  |
 *          r2   |  c2  |           |\     |
 *  IN1 >--zzzz--+--||--+--------+  | \    |
 *               |               '--|- \   |
 *          rN   |                  |   >--+----------> Netlist Node
 *   vN >--zzzz--'               .--|+ /
 *                               |  | /
 *  vRef >-----------------------'  |/
 *
 *          --------------------------------------------------
 *
 *     DISC_OP_AMP_FILTER_IS_BAND_PASS_1M
 *          Single Pole Multiple Feedback Band Pass Filter
 *
 *  Note: This filter does not currently work.
 *
 *          rP             c1
 *   vP >--zzzz--.      .--||----+---------.
 *               |      |        |         |
 *          r1   |      |        z         |
 *  IN0 >--zzzz--+      |        z rF      |
 *               |      |        z         |
 *          r2   |      |  c2    |  |\     |
 *  IN1 >--zzzz--+------+--||----+  | \    |
 *               |               '--|- \   |
 *          rN   |                  |   >--+----------> Netlist Node
 *   vN >--zzzz--'               .--|+ /
 *                               |  | /
 *  vRef >-----------------------'  |/
 *
 * Here is how to calculate the filter info, even though there is no way to use it.
 * This will only be usefull if C1 = C2.
 * rA = 1/(1/rP + 1/r1 +1/r2 + 1/rN)  = all input resistors in parallel.
 * C = c1 = c2
 *   fC = 1/(2 * PI * C * sqrt(rA * rF))
 *    Q = .5 * sqrt(rF / rA)
 * gain = -2 * Q*Q
 *
 ***********************************************************************
 *
 * DISCRETE_RCDISC - Simple single pole RC discharge network
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------>| RC         |
 *                        |            |
 *    INPUT1     -1------>| -ZZZZ-+--  |
 *                        |   R   |    |---->   Netlist node
 *    RVAL       -2------>|      ---   |
 *                        |      ---C  |
 *    CVAL       -3------>|       |    |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_RCFILTER(name of node,
 *                       enable,
 *                       input node (or value),
 *                       resistor value in OHMS,
 *                       capacitor value in FARADS)
 *
 *  Example config line
 *
 *     DISCRETE_RCDISC(NODE_11,NODE_10,10,100,1e-6)
 *
 *  When enabled by NODE_10, C discharges from 10v as indicated by RC
 *  of 100R & 1uF.
 *
 ***********************************************************************
 *
 * DISCRETE_RCDISC2  - Switched input RC discharge network
 *
 *                        .------------.
 *                        |            |
 *    SWITCH     -0------>| IP0 | IP1  |
 *                        |            |
 *    INPUT0     -1------>| -ZZZZ-.    |
 *                        |   R0  |    |
 *    RVAL0      -2------>|       |    |
 *                        |       |    |
 *    INPUT1     -3------>| -ZZZZ-+--  |
 *                        |   R1  |    |---->   Netlist node
 *    RVAL1      -4------>|      ---   |
 *                        |      ---C  |
 *    CVAL       -5------>|       |    |
 *                        |            |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *      DISCRETE_RCDISC2(name of node,
 *                       switch,
 *                       input0 node (or value),
 *                       resistor0 value in OHMS,
 *                       input1 node (or value),
 *                       resistor1 value in OHMS,
 *                       capacitor value in FARADS)
 *
 *  Example config line
 *
 *     DISCRETE_RCDISC(NODE_9,NODE_10,10.0,100,0.0,100,1e-6)
 *
 *  When switched by NODE_10, C charges/discharges from 10v/0v
 *  as dicated by RC0 & RC1 combos respectively
 *  of 100R & 1uF.
 *
 ***********************************************************************
 *
 * DISCRETE_RCFILTER - Simple single pole RC filter network (vRef = 0)
 * DISCRETE_RCFILTER_VREF - Same but refrenced to vRef not 0V
 *
 *                        .------------.
 *                        |            |
 *    ENAB       -0------}| RC FILTER  |
 *                        |            |
 *    INPUT1     -1------}| -ZZZZ-+--  |
 *                        |   R   |    |----}   Netlist node
 *    RVAL       -2------}|      ---   |
 *                        |      ---C  |
 *    CVAL       -3------}|       |    |
 *                        |      vRef  |
 *                        '------------'
 *
 *  Declaration syntax
 *
 *     DISCRETE_RCFILTER(name of node,
 *                       enable
 *                       input node (or value)
 *                       resistor value in OHMS
 *                       capacitor value in FARADS)
 *
 *     DISCRETE_RCFILTER_VREF(name of node,
 *                            enable
 *                            input node (or value)
 *                            resistor value in OHMS
 *                            capacitor value in FARADS,
 *                            vRef static value)
 *
 *  Example config line
 *
 *     DISCRETE_RCFILTER(NODE_11,1,NODE_10,100,1e-6)
 *
 *  Defines an always enabled RC filter with a 100R & 1uF network
 *  the input is fed from NODE_10.
 *
 *  This can be also thought of as a low pass filter with a 3dB cutoff
 *  at:
 *                                  1
 *            Fcuttoff =      --------------
 *                            2*Pi*RVAL*CVAL
 *
 *  (3dB cutoff is where the output power has dropped by 3dB ie Half)
 *
 ***********************************************************************
 =======================================================================
 * from from disc_flt.c
 * Component specific modules
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_555_ASTABLE - NE555 Chip simulation (astable mode)
 *
 *                                v555
 *                                 |
 *                       .---------+
 *                       |         |
 *                       Z         |
 *                    R1 Z     .---------.
 *                       |     |  Vcc    |
 *                       +-----|Discharge|
 *                       |     |         |
 *                       Z     |   555   |
 *                    R2 Z     |      Out|---> Netlist Node
 *                       |     |         |
 *                       +-----|Threshold|
 *                       |     |         |
 *                       +-----|Trigger  |
 *                       |     |         |---< Control Voltage
 *                       |     |  Reset  |
 *                       |     '---------'
 *                      ---         |
 *                    C ---         |
 *                       |          ^
 *                      gnd       Reset
 *
 *  Declaration syntax
 *
 *     DISCRETE_NE555(name of node,
 *                    reset node (or value),
 *                    R1 node (or value) in ohms,
 *                    R2 node (or value) in ohms,
 *                    C node (or value) in farads,
 *                    Control Voltage node (or value),
 *                    &Type structure)
 *
 *  Output Types:
 *     DISC_555_OUT_DC - Output is actual DC.
 *     DISC_555_OUT_AC - A cheat to make the waveform AC.
 *
 *  Waveform Types:
 *     DISC_555_OUT_SQW       - Output is Squarewave.  0 or v555high. 
 *     DISC_555_OUT_CAP       - Output is Timing Capacitor 'C' voltage.
 *     DISC_555_OUT_CAP_CLAMP - During a sample period, the high/low
 *                              threshold may be reached, causing the
 *                              voltage to change direction.  This will
 *                              make varing peaks on the cap out signal.
 *                              This may cause an alaised noise to be
 *                              heard.  Using this clamp option will
 *                              force the output to be the threshold
 *                              voltage during a change.  While not
 *                              affecting the actual cap voltage.  This
 *                              may cause a high freq alaised noise
 *                              that is less noticeable then the noise
 *                              without the option.  Try without CLAMP
 *                              first, as it is more accurate.
 *
 ***********************************************************************
 *
 * DISCRETE_555_CC - Constant Current Controlled 555 Oscillator
 *                   Which works out to a VCO when R is fixed.
 *
 *       vCCsource                       v555
 *           V                            V
 *           |     .----------------------+
 *           |     |                      |
 *           |     |                  .---------.
 *           |     |       rDischarge |  Vcc    |
 *           Z     Z        .---+-----|Discharge|
 *           Z R   Z rBias  |   |     |         |
 *           |     |        |   Z     |   555   |
 *           |     |        |   Z     |      Out|---> Netlist Node
 *         .----.  |        |   |     |         |
 *  Vin >--| CC |--+--------'   +-----|Threshold|
 *         '----'               |     |         |
 *                              +-----|Trigger  |
 *                              |     |         |
 *                 .------+-----'     |  Reset  |
 *                 |      |           '---------'
 *                ---     Z                |
 *                --- C   Z rGnd           |
 *                 |      |                ^
 *                gnd    gnd             Reset
 *
 * Notes: R sets the current and should never be 0 (short).
 *        The current follows the voltage I=Vin/R and charges C.
 *        rBias, rDischarge and rGnd should be 0 if not used.
 *        Reset is active low for the module.
 *
 *        DISC_555_OUT_SQW mode only:
 *        When there is no rDischarge there is a very short discharge
 *        cycle (almost 0s), so the module triggers the output for 1
 *        sample. This does not effect the timing, just the duty cycle.
 *        But frequencies more the half the sample frequency will be
 *        limited to a max of half the sample frequency.
 *        This mode should be used to drive a counter for any real use.
 *        Just like the real thing.
 *
 *  Declaration syntax
 *
 *     DISCRETE_555_CC(name of node,
 *                     reset node or static value,
 *                     Vin node or static value,
 *                     R node or static value,
 *                     C node or static value,
 *                     rBias node or static value,
 *                     rGnd node or static value,
 *                     rDischarge node or static value,
 *                     address of discrete_555_cc_desc structure)
 *
 *  Output Types:
 *     See DISCRETE_555_ASTABLE for description. 
 *
 *  Waveform Types:
 *     See DISCRETE_555_ASTABLE for description. 
 *     Note that DISC_555_OUT_CAP_CLAMP does not work well with
 *     type 0 circuit.
 *
 ***********************************************************************
 *
 * DISCRETE_566 - NE566 VCO simulation.
 *
 *                       vPlus
 *                         V
 *           .-------------+
 *           |             |
 *           |    R    .-------.
 *           '---/\/\--|6  8   |
 *                     |       |
 *   vMod >------------|5   3/4|---------> Netlist Node
 *                     |       |
 *                 .---|7  1   |
 *                 |   '-------'
 *                ---      |
 *                --- C    |
 *                 |       |
 *                vNeg    vNeg
 *
 *  Declaration syntax
 *
 *     DISCRETE_566(name of node,
 *                  enable node or static value,
 *                  R node or static value in ohms,
 *                  C node or static value in Farads,
 *                  vMod node or static value,
 *                  address of discrete_566_desc structure)
 *
 *  Output Types:
 *     DISC_556_OUT_DC - Output is actual DC.
 *     DISC_556_OUT_AC - A cheat to make the waveform AC.
 *
 *  Waveform Types:
 *     DISC_566_OUT_SQUARE   - Pin 3 Square Wave Output
 *     DISC_566_OUT_TRIANGLE - Pin 4 Triangle Wave Output
 *     DISC_566_OUT_LOGIC    - Internal Flip/Flop Output
 *
 ***********************************************************************
 =======================================================================
 * Must be last module.
 =======================================================================
 ***********************************************************************
 *
 * DISCRETE_OUTPUT - Single output node to Mame mixer and output
 *
 *                            .----------.       .
 *                            |          |    .-/|
 *      Netlist node -------->| OUTPUT   |----|  | Sound Output
 *                            |          |    '-\|
 *                            '----------'       '
 *
 *  Declaration syntax
 *
 *     DISCRETE_OUTPUT(name of output node,volume)
 *
 *  Example config line
 *
 *     DISCRETE_OUTPUT(NODE_02,100)
 *
 *  Output stream will be generated from the NODE_02 output stream.
 *
 ************************************************************************/


/*************************************
 *
 *	Core constants
 *
 *************************************/

#define DISCRETE_MAX_NODES				300
#define DISCRETE_MAX_INPUTS				10
#define DISCRETE_MAX_OUTPUTS			16



/*************************************
 *
 *	Node-specific constants
 *
 *************************************/

#define DISC_LOGADJ						1.0
#define DISC_LINADJ						0.0

/* DISCRETE_COMP_ADDER types */
#define DISC_COMP_P_CAPACITOR			0x00
#define DISC_COMP_P_RESISTOR			0x01

/* Function possibilities for the LFSR feedback nodes */
/* 2 inputs, one output                               */
#define DISC_LFSR_XOR					0
#define DISC_LFSR_OR					1
#define DISC_LFSR_AND					2
#define DISC_LFSR_XNOR					3
#define DISC_LFSR_NOR					4
#define DISC_LFSR_NAND					5
#define DISC_LFSR_IN0					6
#define DISC_LFSR_IN1					7
#define DISC_LFSR_NOT_IN0				8
#define DISC_LFSR_NOT_IN1				9
#define DISC_LFSR_REPLACE				10

/* LFSR Flag Bits */
#define DISC_LFSR_FLAG_OUT_INVERT		0x01
#define DISC_LFSR_FLAG_RESET_TYPE_L		0x00
#define DISC_LFSR_FLAG_RESET_TYPE_H		0x02

/* Sample & Hold supported clock types */
#define DISC_SAMPHOLD_REDGE				0
#define DISC_SAMPHOLD_FEDGE				1
#define DISC_SAMPHOLD_HLATCH			2
#define DISC_SAMPHOLD_LLATCH			3

/* Maximum number of resistors in ladder chain */
#define DISC_LADDER_MAXRES				8

/* Filter types */
#define DISC_FILTER_LOWPASS				0
#define DISC_FILTER_HIGHPASS			1
#define DISC_FILTER_BANDPASS			2

/* Mixer types */
#define DISC_MIXER_IS_RESISTOR			0
#define DISC_MIXER_IS_OP_AMP			1

#define DISC_MIXER_IS_OP_AMP_WITH_RI 	2	/* Used only internally.  Use DISC_MIXER_IS_OP_AMP */
#define DISC_MIXER_TYPE_MASK			3	/* Used only internally. */
#define DISC_MIXER_HAS_R_NODE			4	/* Used only internally. */

/* Common Op Amp Flags and values */
#define DISC_OP_AMP_IS_NORTON	0x01
#define OP_AMP_NORTON_VBE		0.5		/* This is the norton junction voltage. Used only internally. */
#define OP_AMP_VP_RAIL_OFFSET	1.5		/* This is how close an op-amp can get to the vP rail. Used only internally. */

/* Op Amp Filter Options */
#define DISC_OP_AMP_FILTER_IS_LOW_PASS_1	0x00
#define DISC_OP_AMP_FILTER_IS_HIGH_PASS_1	0x10
#define DISC_OP_AMP_FILTER_IS_BAND_PASS_1	0x20
#define DISC_OP_AMP_FILTER_IS_BAND_PASS_1M	0x30

#define DISC_OP_AMP_FILTER_TYPE_MASK		0xf0	/* Used only internally. */

/* Op Amp Oscillator Flags */
#define DISC_OP_AMP_OSCILLATOR_1			0x00
#define DISC_OP_AMP_OSCILLATOR_VCO_1		0x80
#define DISC_OP_AMP_OSCILLATOR_OUT_CAP		0x00
#define DISC_OP_AMP_OSCILLATOR_OUT_SQW		0x02

#define DISC_OP_AMP_OSCILLATOR_TYPE_MASK	(0xf0 | DISC_OP_AMP_IS_NORTON)	/* Used only internally. */
#define DISC_OP_AMP_OSCILLATOR_VCO_MASK		0x80	/* Used only internally. */

/* Schmitt Oscillator Options */
#define DISC_SCHMITT_OSC_IN_IS_LOGIC	0x00
#define DISC_SCHMITT_OSC_IN_IS_VOLTAGE	0x01

#define DISC_SCHMITT_OSC_ENAB_IS_AND	0x00
#define DISC_SCHMITT_OSC_ENAB_IS_NAND	0x02
#define DISC_SCHMITT_OSC_ENAB_IS_OR		0x04
#define DISC_SCHMITT_OSC_ENAB_IS_NOR	0x06

#define DISC_SCHMITT_OSC_ENAB_MASK		0x06	/* Bits that define output enable type.
						 						 * Used only internally in module. */

/* 555 Common output flags */
#define DISC_555_OUT_DC					0x00
#define DISC_555_OUT_AC					0x01

#define DISC_555_OUT_SQW				0x00	/* Squarewave */
#define DISC_555_OUT_CAP				0x10	/* Cap charge waveform */
#define DISC_555_OUT_CAP_CLAMP			0x20	/* When outputting the cap voltage, it is forced
												 * to the threshold/trigger levels to help eliminate
												 * alaising. */

#define DISC_555_OUT_MASK				0x30	/* Bits that define output type.
												 * Used only internally in module. */

/* 566 output flags */
#define DISC_566_OUT_DC					0x00
#define DISC_566_OUT_AC					0x01

#define DISC_566_OUT_SQUARE				0x00	/* Squarewave */
#define DISC_566_OUT_TRIANGLE			0x10	/* Triangle waveform */
#define DISC_566_OUT_LOGIC				0x20	/* 0/1 logic output */

#define DISC_566_OUT_MASK				0x30	/* Bits that define output type.
												 * Used only internally in module. */

/* Oneshot types */
#define DISC_ONESHOT_FEDGE				0x00
#define DISC_ONESHOT_REDGE				0x01

#define DISC_ONESHOT_NORETRIG			0x00
#define DISC_ONESHOT_RETRIG				0x02

#define DISC_OUT_ACTIVE_LOW				0x04
#define DISC_OUT_ACTIVE_HIGH			0x00



/*************************************
 *
 *	The discrete sound blocks as
 *	defined in the drivers
 *
 *************************************/

struct discrete_sound_block
{
	int				node;							/* Output node number */
	int				type;							/* see defines below */
	int				active_inputs;					/* Number of active inputs on this node type */
	int				input_node[DISCRETE_MAX_INPUTS];/* input/control nodes */
	double			initial[DISCRETE_MAX_INPUTS];	/* Initial values */
	const void *	custom;							/* Custom function specific initialisation data */
	const char *	name;							/* Node Name */
};



/*************************************
 *
 *	Discrete module definition
 *
 *************************************/

struct node_description;
struct discrete_module
{
	int				type;
	const char *	name;
	size_t			contextsize;
	void (*reset)(struct node_description *node);	/* Called to reset a node after creation or system reset */
	void (*step)(struct node_description *node);	/* Called to execute one time delta of output update */
};



/*************************************
 *
 *	Internal structure of a node
 *
 *************************************/

struct node_description
{
	int				node;							/* The nodes index number in the node list */
	struct discrete_module module;					/* Copy of the nodes module info */
	double			output;							/* The nodes last output value */

	int				active_inputs;					/* Number of active inputs on this node type */
	struct node_description *		
					input_node[DISCRETE_MAX_INPUTS];/* Either pointer to input node OR NULL in which case use the input value */
	double			input[DISCRETE_MAX_INPUTS];		/* Input values for this node */

	void *			context;						/* Contextual information specific to this node type */
	const char *	name;							/* Text name string for identification/debug */
	const void *	custom;							/* Custom function specific initialisation data */
};



/*************************************
 *
 *	Node-specific struct types
 *
 *************************************/

struct discrete_lfsr_desc
{
	int bitlength;
	int reset_value;

	int feedback_bitsel0;
	int feedback_bitsel1;
	int feedback_function0;         /* Combines bitsel0 & bitsel1 */

	int feedback_function1;         /* Combines funct0 & infeed bit */

	int feedback_function2;         /* Combines funct1 & shifted register */
	int feedback_function2_mask;    /* Which bits are affected by function 2 */

	int flags;

	int output_bit;
};


struct discrete_op_amp_osc_info
{
	int		type;
	double	r1;
	double	r2;
	double	r3;
	double	r4;
	double	r5;
	double	r6;
	double	r7;
	double	r8;
	double	c;
	double	vP;		/* Op amp B+ */
};

struct discrete_schmitt_osc_desc
{
	double	rIn;
	double	rFeedback;
	double	c;
	double	trshRise;	/* voltage that triggers the gate input to go high (vGate) on rise */
	double	trshFall;	/* voltage that triggers the gate input to go low (0V) on fall */
	double	vGate;		/* the ouput high voltage of the gate that gets fedback through rFeedback */
	int		options;	/* bitmaped options */
};


struct discrete_comp_adder_table
{
	int		type;
	double	cDefault;				/* Default componet.  0 if not used. */
	int		length;
	double	c[DISC_LADDER_MAXRES];	/* Componet table */
};


struct discrete_dac_r1_ladder
{
	int	ladderLength;		/* 2 to DISC_LADDER_MAXRES.  1 would be useless. */
	double	r[DISC_LADDER_MAXRES];	/* Don't use 0 for valid resistors.  That is a short. */
	double	vBias;			/* Voltage Bias resistor is tied to (0 = not used) */
	double	rBias;			/* Additional resistor tied to vBias (0 = not used) */
	double	rGnd;			/* Resistor tied to ground (0 = not used) */
	double	cFilter;		/* Filtering cap (0 = not used) */
};


#define DISC_MAX_MIXER_INPUTS	8
struct discrete_mixer_desc
{
	int	type;
	int	mixerLength;
	double	r[DISC_MAX_MIXER_INPUTS];	/* static input resistance values.  These are in series with rNode, if used. */
	int	rNode[DISC_MAX_MIXER_INPUTS];	/* variable resistance nodes, if needed.  0 if not used. */
	double	c[DISC_MAX_MIXER_INPUTS];
	double	rI;
	double	rF;
	double	cF;
	double	cAmp;
	double	vRef;
	double	gain;				/* Scale value to get output close to +/- 32767 */
};


struct discrete_op_amp_filt_info
{
	double	r1;
	double	r2;
	double	rP;
	double	rN;
	double	rF;
	double	c1;
	double	c2;
	double	vRef;
	double	vP;
	double	vN;
};


struct discrete_555_astbl_desc
{
	int		options;		/* bit mapped options */
	double	v555;			/* B+ voltage of 555 */
	double	v555high;		/* High output voltage of 555 (Usually v555 - 1.7) */
	double	threshold555;	/* normally 2/3 of v555 */
	double	trigger555;		/* normally 1/3 of v555 */
};


struct discrete_555_cc_desc
{
	int		options;		/* bit mapped options */
	double	v555;			/* B+ voltage of 555 */
	double	v555high;		/* High output voltage of 555 (Usually v555 - 1.7) */
	double	threshold555;	/* normally 2/3 of v555 */
	double	trigger555;		/* normally 1/3 of v555 */
	double	vCCsource;		/* B+ voltage of the Constant Current source */
	double	vCCjunction;	/* The voltage drop of the Constant Current source transitor (0 if Op Amp) */
};


struct discrete_566_desc
{
	int		options;	/* bit mapped options */
	double	vPlus;		/* B+ voltage of 566 */
	double	vNeg;		/* B- voltage of 566 */
};


struct discrete_adsr
{
	double attack_time;  /* All times are in seconds */
	double attack_value;
	double decay_time;
	double decay_value;
	double sustain_time;
	double sustain_value;
	double release_time;
	double release_value;
};



/*************************************
 *
 *	The node numbers themselves
 *
 *************************************/

enum { NODE_00=0x40000000
              , NODE_01, NODE_02, NODE_03, NODE_04, NODE_05, NODE_06, NODE_07, NODE_08, NODE_09,
       NODE_10, NODE_11, NODE_12, NODE_13, NODE_14, NODE_15, NODE_16, NODE_17, NODE_18, NODE_19,
       NODE_20, NODE_21, NODE_22, NODE_23, NODE_24, NODE_25, NODE_26, NODE_27, NODE_28, NODE_29,
       NODE_30, NODE_31, NODE_32, NODE_33, NODE_34, NODE_35, NODE_36, NODE_37, NODE_38, NODE_39,
       NODE_40, NODE_41, NODE_42, NODE_43, NODE_44, NODE_45, NODE_46, NODE_47, NODE_48, NODE_49,
       NODE_50, NODE_51, NODE_52, NODE_53, NODE_54, NODE_55, NODE_56, NODE_57, NODE_58, NODE_59,
       NODE_60, NODE_61, NODE_62, NODE_63, NODE_64, NODE_65, NODE_66, NODE_67, NODE_68, NODE_69,
       NODE_70, NODE_71, NODE_72, NODE_73, NODE_74, NODE_75, NODE_76, NODE_77, NODE_78, NODE_79,
       NODE_80, NODE_81, NODE_82, NODE_83, NODE_84, NODE_85, NODE_86, NODE_87, NODE_88, NODE_89,
       NODE_90, NODE_91, NODE_92, NODE_93, NODE_94, NODE_95, NODE_96, NODE_97, NODE_98, NODE_99,
       NODE_100,NODE_101,NODE_102,NODE_103,NODE_104,NODE_105,NODE_106,NODE_107,NODE_108,NODE_109,
       NODE_110,NODE_111,NODE_112,NODE_113,NODE_114,NODE_115,NODE_116,NODE_117,NODE_118,NODE_119,
       NODE_120,NODE_121,NODE_122,NODE_123,NODE_124,NODE_125,NODE_126,NODE_127,NODE_128,NODE_129,
       NODE_130,NODE_131,NODE_132,NODE_133,NODE_134,NODE_135,NODE_136,NODE_137,NODE_138,NODE_139,
       NODE_140,NODE_141,NODE_142,NODE_143,NODE_144,NODE_145,NODE_146,NODE_147,NODE_148,NODE_149,
       NODE_150,NODE_151,NODE_152,NODE_153,NODE_154,NODE_155,NODE_156,NODE_157,NODE_158,NODE_159,
       NODE_160,NODE_161,NODE_162,NODE_163,NODE_164,NODE_165,NODE_166,NODE_167,NODE_168,NODE_169,
       NODE_170,NODE_171,NODE_172,NODE_173,NODE_174,NODE_175,NODE_176,NODE_177,NODE_178,NODE_179,
       NODE_180,NODE_181,NODE_182,NODE_183,NODE_184,NODE_185,NODE_186,NODE_187,NODE_188,NODE_189,
       NODE_190,NODE_191,NODE_192,NODE_193,NODE_194,NODE_195,NODE_196,NODE_197,NODE_198,NODE_199,
       NODE_200,NODE_201,NODE_202,NODE_203,NODE_204,NODE_205,NODE_206,NODE_207,NODE_208,NODE_209,
       NODE_210,NODE_211,NODE_212,NODE_213,NODE_214,NODE_215,NODE_216,NODE_217,NODE_218,NODE_219,
       NODE_220,NODE_221,NODE_222,NODE_223,NODE_224,NODE_225,NODE_226,NODE_227,NODE_228,NODE_229,
       NODE_230,NODE_231,NODE_232,NODE_233,NODE_234,NODE_235,NODE_236,NODE_237,NODE_238,NODE_239,
       NODE_240,NODE_241,NODE_242,NODE_243,NODE_244,NODE_245,NODE_246,NODE_247,NODE_248,NODE_249,
       NODE_250,NODE_251,NODE_252,NODE_253,NODE_254,NODE_255,NODE_256,NODE_257,NODE_258,NODE_259,
       NODE_260,NODE_261,NODE_262,NODE_263,NODE_264,NODE_265,NODE_266,NODE_267,NODE_268,NODE_269,
       NODE_270,NODE_271,NODE_272,NODE_273,NODE_274,NODE_275,NODE_276,NODE_277,NODE_278,NODE_279,
       NODE_280,NODE_281,NODE_282,NODE_283,NODE_284,NODE_285,NODE_286,NODE_287,NODE_288,NODE_289,
       NODE_290,NODE_291,NODE_292,NODE_293,NODE_294,NODE_295,NODE_296,NODE_297,NODE_298,NODE_299 };

/* Some Pre-defined nodes for convenience */
#define NODE_NC  NODE_00
#define NODE_OP  (NODE_00+(DISCRETE_MAX_NODES))

#define NODE_START	NODE_00
#define NODE_END	NODE_OP



/*************************************
 *
 *	Enumerated values for Node types 
 *	in the simulation
 *
 *		DSS - Discrete Sound Source
 *		DST - Discrete Sound Transform
 *		DSD - Discrete Sound Device
 *		DSO - Discrete Sound Output
 *
 *************************************/

enum
{
	DSS_NULL,			/* Nothing, nill, zippo, only to be used as terminating node */

	/* from disc_inp.c */
	DSS_ADJUSTMENT,		/* Adjustment node */
	DSS_CONSTANT,		/* Constant node */
	DSS_INPUT,			/* Memory mapped input node */
	DSS_INPUT_PULSE,	/* Memory mapped input node, single pulsed version */

	/* from disc_wav.c */
	/* generic modules */
	DSS_COUNTER,		/* External clock Binary Counter */
	DSS_COUNTER_FIX,	/* Fixed frequency Binary Counter */
	DSS_LFSR_NOISE,		/* Cyclic/Resetable LFSR based Noise generator */
	DSS_NOISE,			/* Random Noise generator */
	DSS_SAWTOOTHWAVE,	/* Sawtooth wave generator */
	DSS_SINEWAVE,		/* Sine Wave generator */
	DSS_SQUAREWAVE,		/* Square Wave generator, adjustable frequency based */
	DSS_SQUAREWFIX,		/* Square Wave generator, fixed frequency based (faster) */
	DSS_SQUAREWAVE2,	/* Square Wave generator, time based */
	DSS_TRIANGLEWAVE,	/* Triangle wave generator, frequency based */
	/* Component specific */
	DSS_OP_AMP_OSC,		/* Op Amp Oscillator */
	DSS_SCHMITT_OSC,	/* Schmitt Feedback Oscillator */
	/* Not yet implemented */
	DSS_ADSR,			/* ADSR Envelope generator */

	/* from disc_mth.c */
	/* generic modules */
	DST_ADDER,			/* C = A+B */
	DST_CLAMP,			/* Signal Clamp */
	DST_DIVIDE,			/* Gain Block, C = A/B */
	DST_GAIN,			/* Gain Block, D = (A*B) + C*/
	DST_LOGIC_INV,
	DST_LOGIC_AND,
	DST_LOGIC_NAND,
	DST_LOGIC_OR,
	DST_LOGIC_NOR,
	DST_LOGIC_XOR,
	DST_LOGIC_NXOR,
	DST_LOGIC_DFF,
	DST_ONESHOT,		/* One-shot pulse generator */
	DST_RAMP,			/* Ramp up/down simulation */
	DST_SAMPHOLD,		/* Sample & hold transform */
	DST_SWITCH,			/* C = A or B */
	DST_TRANSFORM,		/* Muliply math functions based on string */
	/* Component specific */
	DST_COMP_ADDER,		/* Selectable Parallel Component Adder */
	DST_DAC_R1,			/* R1 Ladder DAC with cap smoothing */
	DST_MIXER,			/* Final Mixing Stage */
	DST_VCA,			/* IC Voltage controlled  amplifiers */
	DST_VCA_OP_AMP,		/* Op Amp Voltage controlled  amplifier circuits */
/*	DST_DELAY,			 Phase shift/Delay line */

	/* from disc_flt.c */
	/* generic modules */
	DST_FILTER1,		/* 1st Order Filter, Low or High Pass */
	DST_FILTER2,		/* 2nd Order Filter, Low, High, or Band Pass */
	/* Component specific */
	DST_CRFILTER,		/* RC Bypass Filter (High Pass) */
	DST_OP_AMP_FILT,	/* Op Amp filters */
	DST_RCDISC,			/* Simple RC discharge */
	DST_RCDISC2,		/* Switched 2 Input RC discharge */
	DST_RCFILTER,		/* Simple RC Filter network */
	/* For testing - seem to be buggered.  Use versions not ending in N. */
	DST_RCFILTERN,		/* Simple RC Filter network */
	DST_RCDISCN,		/* Simple RC discharge */
	DST_RCDISC2N,		/* Switched 2 Input RC discharge */

	/* from disc_dev.c */
	/* Component specific */
	DSD_555_ASTBL,		/* NE555 Astable Emulation */
	DSD_555_MSTBL,		/* NE555 Monostable Emulation */
	DSD_555_CC,			/* Constant Current 555 circuit (VCO)*/
	DSD_566,			/* NE566 Emulation */

	/* Custom */
/*	DST_CUSTOM,			 whatever you want someday  */

	/* Output Node -- this must be the last entry in this enum! */
	DSO_OUTPUT			/* The final output node */
};



/*************************************
 *
 *	Encapsulation macros for defining 
 *	your simulation
 *
 *************************************/

#define DISCRETE_SOUND_START(STRUCTURENAME) struct discrete_sound_block STRUCTURENAME[] = {
#define DISCRETE_SOUND_END                                              { NODE_00, DSS_NULL        , 0, { NODE_NC }, { 0 } ,NULL  ,"End Marker" }  };

/* from disc_inp.c */
#define DISCRETE_ADJUSTMENT(NODE,ENAB,MIN,MAX,LOGLIN,PORT)              { NODE, DSS_ADJUSTMENT  , 7, { ENAB,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { ENAB,MIN,MAX,LOGLIN,PORT,0   ,100  }, NULL  , NULL  },
#define DISCRETE_ADJUSTMENTX(NODE,ENAB,MIN,MAX,LOGLIN,PORT,PMIN,PMAX)   { NODE, DSS_ADJUSTMENT  , 7, { ENAB,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { ENAB,MIN,MAX,LOGLIN,PORT,PMIN,PMAX }, NULL  , NULL  },
#define DISCRETE_CONSTANT(NODE,CONST)                                   { NODE, DSS_CONSTANT    , 1, { NODE_NC }, { CONST } ,NULL  ,"Constant" },
#define DISCRETE_INPUT(NODE,ADDR,MASK,INIT0)                            { NODE, DSS_INPUT       , 6, { NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { INIT0,ADDR,MASK,1   ,0     ,INIT0 }, NULL, "Input" },
#define DISCRETE_INPUTX(NODE,ADDR,MASK,GAIN,OFFSET,INIT0)               { NODE, DSS_INPUT       , 6, { NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { INIT0,ADDR,MASK,GAIN,OFFSET,INIT0 }, NULL, "InputX" },
#define DISCRETE_INPUT_PULSE(NODE,ADDR,MASK,INIT0)                      { NODE, DSS_INPUT_PULSE , 6, { NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC,NODE_NC }, { INIT0,ADDR,MASK,1   ,0     ,INIT0 }, NULL, "Input Pulse" },

/* from disc_wav.c */
/* generic modules */
#define DISCRETE_COUNTER(NODE,ENAB,RESET,CLK,MAX,DIR,INIT0,CLKTYPE)     { NODE, DSS_COUNTER     , 7, { ENAB,RESET,CLK,NODE_NC,DIR,INIT0,NODE_NC }, { ENAB,RESET,CLK,MAX,DIR,INIT0,CLKTYPE }, NULL, "External clock Binary Counter" },
#define DISCRETE_COUNTER_FIX(NODE,ENAB,RESET,FREQ,MAX,DIR,INIT0)        { NODE, DSS_COUNTER_FIX , 6, { ENAB,RESET,FREQ,NODE_NC,DIR,INIT0 }, { ENAB,RESET,FREQ,MAX,DIR,INIT0 }, NULL, "Fixed Freq Binary Counter" },
#define DISCRETE_LFSR_NOISE(NODE,ENAB,RESET,FREQ,AMPL,FEED,BIAS,LFSRTB) { NODE, DSS_LFSR_NOISE  , 6, { ENAB,RESET,FREQ,AMPL,FEED,BIAS }, { ENAB,RESET,FREQ,AMPL,FEED,BIAS }, LFSRTB, "LFSR Noise Source" },
#define DISCRETE_NOISE(NODE,ENAB,FREQ,AMPL,BIAS)                        { NODE, DSS_NOISE       , 4, { ENAB,FREQ,AMPL,BIAS }, { ENAB,FREQ,AMPL,BIAS }, NULL, "Noise Source" },
#define DISCRETE_SAWTOOTHWAVE(NODE,ENAB,FREQ,AMPL,BIAS,GRAD,PHASE)      { NODE, DSS_SAWTOOTHWAVE, 6, { ENAB,FREQ,AMPL,BIAS,NODE_NC,NODE_NC }, { ENAB,FREQ,AMPL,BIAS,GRAD,PHASE }, NULL, "Saw Tooth Wave" },
#define DISCRETE_SINEWAVE(NODE,ENAB,FREQ,AMPL,BIAS,PHASE)               { NODE, DSS_SINEWAVE    , 5, { ENAB,FREQ,AMPL,BIAS,NODE_NC }, { ENAB,FREQ,AMPL,BIAS,PHASE }, NULL, "Sine Wave" },
#define DISCRETE_SQUAREWAVE(NODE,ENAB,FREQ,AMPL,DUTY,BIAS,PHASE)        { NODE, DSS_SQUAREWAVE  , 6, { ENAB,FREQ,AMPL,DUTY,BIAS,NODE_NC }, { ENAB,FREQ,AMPL,DUTY,BIAS,PHASE }, NULL, "Square Wave" },
#define DISCRETE_SQUAREWFIX(NODE,ENAB,FREQ,AMPL,DUTY,BIAS,PHASE)        { NODE, DSS_SQUAREWFIX  , 6, { ENAB,FREQ,AMPL,DUTY,BIAS,NODE_NC }, { ENAB,FREQ,AMPL,DUTY,BIAS,PHASE }, NULL, "Square Wave Fixed" },
#define DISCRETE_SQUAREWAVE2(NODE,ENAB,AMPL,T_OFF,T_ON,BIAS,TSHIFT)     { NODE, DSS_SQUAREWAVE2 , 6, { ENAB,AMPL,T_OFF,T_ON,BIAS,NODE_NC }, { ENAB,AMPL,T_OFF,T_ON,BIAS,TSHIFT }, NULL, "Square Wave 2" },
#define DISCRETE_TRIANGLEWAVE(NODE,ENAB,FREQ,AMPL,BIAS,PHASE)           { NODE, DSS_TRIANGLEWAVE, 5, { ENAB,FREQ,AMPL,BIAS,NODE_NC }, { ENAB,FREQ,AMPL,BIAS,PHASE }, NULL, "Triangle Wave" },
/* Component specific */
#define DISCRETE_OP_AMP_OSCILLATOR(NODE,ENAB,INFO)                      { NODE, DSS_OP_AMP_OSC  , 1, { ENAB }, { ENAB }, INFO, "Op Amp Oscillator" },
#define DISCRETE_OP_AMP_VCO1(NODE,ENAB,VMOD1,INFO)                      { NODE, DSS_OP_AMP_OSC  , 2, { ENAB,VMOD1 }, { ENAB,VMOD1 }, INFO, "Op Amp VCO 1-vMod" },
#define DISCRETE_OP_AMP_VCO2(NODE,ENAB,VMOD1,VMOD2,INFO)                { NODE, DSS_OP_AMP_OSC  , 3, { ENAB,VMOD1,VMOD2 }, { ENAB,VMOD1,VMOD2 }, INFO, "Op Amp VCO 2-vMod" },
#define DISCRETE_SCHMITT_OSCILLATOR(NODE,ENAB,INP0,AMPL,TABLE)          { NODE, DSS_SCHMITT_OSC , 3, { ENAB,INP0,AMPL }, { ENAB,INP0,AMPL }, TABLE, "Schmitt Feedback Oscillator" },
/* Not yet implemented */
#define DISCRETE_ADSR_ENV(NODE,ENAB,TRIGGER,GAIN,ADSRTB)                { NODE, DSS_ADSR        , 3, { ENAB,TRIGGER,GAIN }, { ENAB,TRIGGER,GAIN }, ADSRTB, "ADSR Env Generator" },

/* from disc_mth.c */
/* generic modules */
#define DISCRETE_ADDER2(NODE,ENAB,INP0,INP1)                            { NODE, DST_ADDER       , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, NULL, "Adder 2 Node" },
#define DISCRETE_ADDER3(NODE,ENAB,INP0,INP1,INP2)                       { NODE, DST_ADDER       , 4, { ENAB,INP0,INP1,INP2 }, { ENAB,INP0,INP1,INP2 }, NULL, "Adder 3 Node" },
#define DISCRETE_ADDER4(NODE,ENAB,INP0,INP1,INP2,INP3)                  { NODE, DST_ADDER       , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, NULL, "Adder 4 Node" },
#define DISCRETE_CLAMP(NODE,ENAB,INP0,MIN,MAX,CLAMP)                    { NODE, DST_CLAMP       , 5, { ENAB,INP0,MIN,MAX,CLAMP }, { ENAB,INP0,MIN,MAX,CLAMP }, NULL, "Signal Clamp" },
#define DISCRETE_DIVIDE(NODE,ENAB,INP0,INP1)                            { NODE, DST_DIVIDE      , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, NULL, "Divider" },
#define DISCRETE_GAIN(NODE,INP0,GAIN)                                   { NODE, DST_GAIN        , 4, { NODE_NC,INP0,NODE_NC,NODE_NC }, { 1,INP0,GAIN,0 }, NULL, "Gain" },
#define DISCRETE_INVERT(NODE,INP0)                                      { NODE, DST_GAIN        , 4, { NODE_NC,INP0,NODE_NC,NODE_NC }, { 1,0,-1,0 }, NULL, "Inverter" },
#define DISCRETE_LOGIC_INVERT(NODE,ENAB,INP0)                           { NODE, DST_LOGIC_INV   , 2, { ENAB,INP0 }, { ENAB,INP0 }, NULL, "Logic Invertor" },
#define DISCRETE_LOGIC_AND(NODE,ENAB,INP0,INP1)                         { NODE, DST_LOGIC_AND   , 5, { ENAB,INP0,INP1,NODE_NC,NODE_NC }, { ENAB,INP0,INP1,1.0,1.0 }, NULL, "Logic AND (2inp)" },
#define DISCRETE_LOGIC_AND3(NODE,ENAB,INP0,INP1,INP2)                   { NODE, DST_LOGIC_AND   , 5, { ENAB,INP0,INP1,INP2,NODE_NC }, { ENAB,INP0,INP1,INP2,1.0 }, NULL, "Logic AND (3inp)" },
#define DISCRETE_LOGIC_AND4(NODE,ENAB,INP0,INP1,INP2,INP3)              { NODE, DST_LOGIC_AND   , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 } ,NULL, "Logic AND (4inp)" },
#define DISCRETE_LOGIC_NAND(NODE,ENAB,INP0,INP1)                        { NODE, DST_LOGIC_NAND  , 5, { ENAB,INP0,INP1,NODE_NC,NODE_NC }, { ENAB,INP0,INP1,1.0,1.0 }, NULL, "Logic NAND (2inp)" },
#define DISCRETE_LOGIC_NAND3(NODE,ENAB,INP0,INP1,INP2)                  { NODE, DST_LOGIC_NAND  , 5, { ENAB,INP0,INP1,INP2,NODE_NC }, { ENAB,INP0,INP1,INP2,1.0 }, NULL, "Logic NAND (3inp)" },
#define DISCRETE_LOGIC_NAND4(NODE,ENAB,INP0,INP1,INP2,INP3)             { NODE, DST_LOGIC_NAND  , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, NULL, "Logic NAND (4inp)" },
#define DISCRETE_LOGIC_OR(NODE,ENAB,INP0,INP1)                          { NODE, DST_LOGIC_OR    , 5, { ENAB,INP0,INP1,NODE_NC,NODE_NC }, { ENAB,INP0,INP1,0.0,0.0 }, NULL, "Logic OR (2inp)" },
#define DISCRETE_LOGIC_OR3(NODE,ENAB,INP0,INP1,INP2)                    { NODE, DST_LOGIC_OR    , 5, { ENAB,INP0,INP1,INP2,NODE_NC }, { ENAB,INP0,INP1,INP2,0.0 }, NULL, "Logic OR (3inp)" },
#define DISCRETE_LOGIC_OR4(NODE,ENAB,INP0,INP1,INP2,INP3)               { NODE, DST_LOGIC_OR    , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, NULL, "Logic OR (4inp)" },
#define DISCRETE_LOGIC_NOR(NODE,ENAB,INP0,INP1)                         { NODE, DST_LOGIC_NOR   , 5, { ENAB,INP0,INP1,NODE_NC,NODE_NC }, { ENAB,INP0,INP1,0.0,0.0 }, NULL, "Logic NOR (2inp)" },
#define DISCRETE_LOGIC_NOR3(NODE,ENAB,INP0,INP1,INP2)                   { NODE, DST_LOGIC_NOR   , 5, { ENAB,INP0,INP1,INP2,NODE_NC }, { ENAB,INP0,INP1,INP2,0.0 }, NULL, "Logic NOR (3inp)" },
#define DISCRETE_LOGIC_NOR4(NODE,ENAB,INP0,INP1,INP2,INP3)              { NODE, DST_LOGIC_NOR   , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, NULL, "Logic NOR (4inp)" },
#define DISCRETE_LOGIC_XOR(NODE,ENAB,INP0,INP1)                         { NODE, DST_LOGIC_XOR   , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, NULL, "Logic XOR (2inp)" },
#define DISCRETE_LOGIC_NXOR(NODE,ENAB,INP0,INP1)                        { NODE, DST_LOGIC_NXOR  , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, NULL, "Logic NXOR (2inp)" },
#define DISCRETE_LOGIC_DFLIPFLOP(NODE,ENAB,RESET,SET,CLK,INP)           { NODE, DST_LOGIC_DFF   , 5, { ENAB,RESET,SET,CLK,INP }, { ENAB,RESET,SET,CLK,INP }, NULL, "Logic DFlipFlop" },
#define DISCRETE_MULTIPLY(NODE,ENAB,INP0,INP1)                          { NODE, DST_GAIN        , 4, { ENAB,INP0,INP1,NODE_NC }, { ENAB,INP0,INP1,0 }, NULL, "Multiplier" },
#define DISCRETE_MULTADD(NODE,ENAB,INP0,INP1,INP2)                      { NODE, DST_GAIN        , 4, { ENAB,INP0,INP1,INP2 }, { ENAB,INP0,INP1,INP2 }, NULL, "Multiply/Add" },
#define DISCRETE_ONESHOT(NODE,TRIG,AMPL,WIDTH,TYPE)                     { NODE, DST_ONESHOT     , 5, { NODE_NC,TRIG,AMPL,WIDTH,NODE_NC }, { 0,TRIG,AMPL,WIDTH,TYPE }, NULL, "One Shot" },
#define DISCRETE_ONESHOTR(NODE,RESET,TRIG,AMPL,WIDTH,TYPE)              { NODE, DST_ONESHOT     , 5, { RESET,TRIG,AMPL,WIDTH,NODE_NC }, { RESET,TRIG,AMPL,WIDTH,TYPE }, NULL, "One Shot Resetable" },
#define DISCRETE_ONOFF(NODE,ENAB,INP0)                                  { NODE, DST_GAIN        , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,0,1,0 }, NULL, "OnOff Switch" },
#define DISCRETE_RAMP(NODE,ENAB,RAMP,GRAD,START,END,CLAMP)              { NODE, DST_RAMP        , 6, { ENAB,RAMP,GRAD,START,END,CLAMP }, { ENAB,RAMP,GRAD,START,END,CLAMP }, NULL, "Ramp Up/Down" },
#define DISCRETE_SAMPLHOLD(NODE,ENAB,INP0,CLOCK,CLKTYPE)                { NODE, DST_SAMPHOLD    , 4, { ENAB,INP0,CLOCK,NODE_NC }, { ENAB,INP0,CLOCK,CLKTYPE }, NULL, "Sample & Hold" },
#define DISCRETE_SWITCH(NODE,ENAB,SWITCH,INP0,INP1)                     { NODE, DST_SWITCH      , 4, { ENAB,SWITCH,INP0,INP1 }, { ENAB,SWITCH,INP0,INP1 }, NULL, "2 Pole Switch" },
#define DISCRETE_TRANSFORM2(NODE,ENAB,INP0,INP1,FUNCT)                  { NODE, DST_TRANSFORM   , 3, { ENAB,INP0,INP1 }, { ENAB,INP0,INP1 }, FUNCT, "Transform 2 Nodes" },
#define DISCRETE_TRANSFORM3(NODE,ENAB,INP0,INP1,INP2,FUNCT)             { NODE, DST_TRANSFORM   , 4, { ENAB,INP0,INP1,INP2 }, { ENAB,INP0,INP1,INP2 }, FUNCT, "Transform 3 Nodes" },
#define DISCRETE_TRANSFORM4(NODE,ENAB,INP0,INP1,INP2,INP3,FUNCT)        { NODE, DST_TRANSFORM   , 5, { ENAB,INP0,INP1,INP2,INP3 }, { ENAB,INP0,INP1,INP2,INP3 }, FUNCT, "Transform 4 Nodes" },
#define DISCRETE_TRANSFORM5(NODE,ENAB,INP0,INP1,INP2,INP3,INP4,FUNCT)   { NODE, DST_TRANSFORM   , 6, { ENAB,INP0,INP1,INP2,INP3,INP4 }, { ENAB,INP0,INP1,INP2,INP3,INP4 }, FUNCT, "Transform 5 Nodes" },
/* Component specific */
#define DISCRETE_COMP_ADDER(NODE,ENAB,DATA,TABLE)                       { NODE, DST_COMP_ADDER  , 2, { ENAB,DATA }, { ENAB,DATA }, TABLE, "Selectable R or C component Adder" },
#define DISCRETE_DAC_R1(NODE,ENAB,DATA,VDATA,LADDER)                    { NODE, DST_DAC_R1      , 3, { ENAB,DATA,VDATA }, { ENAB,DATA,VDATA }, LADDER, "DAC with R1 Ladder" },
#define DISCRETE_MIXER2(NODE,ENAB,IN0,IN1,INFO)                         { NODE, DST_MIXER       , 3, { ENAB,IN0,IN1 }, { ENAB,IN0,IN1 }, INFO, "Final Mixer 2 Stage" },
#define DISCRETE_MIXER3(NODE,ENAB,IN0,IN1,IN2,INFO)                     { NODE, DST_MIXER       , 4, { ENAB,IN0,IN1,IN2 }, { ENAB,IN0,IN1,IN2 }, INFO, "Final Mixer 3 Stage" },
#define DISCRETE_MIXER4(NODE,ENAB,IN0,IN1,IN2,IN3,INFO)                 { NODE, DST_MIXER       , 5, { ENAB,IN0,IN1,IN2,IN3 }, { ENAB,IN0,IN1,IN2,IN3 }, INFO, "Final Mixer 4 Stage" },
#define DISCRETE_MIXER5(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,INFO)             { NODE, DST_MIXER       , 6, { ENAB,IN0,IN1,IN2,IN3,IN4 }, { ENAB,IN0,IN1,IN2,IN3,IN4 }, INFO, "Final Mixer 5 Stage" },
#define DISCRETE_MIXER6(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,INFO)         { NODE, DST_MIXER       , 7, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5 }, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5 }, INFO, "Final Mixer 6 Stage" },
#define DISCRETE_MIXER7(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,INFO)     { NODE, DST_MIXER       , 8, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6 }, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6 }, INFO, "Final Mixer 7 Stage" },
#define DISCRETE_MIXER8(NODE,ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,IN7,INFO) { NODE, DST_MIXER       , 9, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,IN7 }, { ENAB,IN0,IN1,IN2,IN3,IN4,IN5,IN6,IN7 }, INFO, "Final Mixer 8 Stage" },
#define DISCRETE_VCA(NODE,ENAB,IN0,CTRL,TYPE)                           { NODE, DST_VCA         , 4, { ENAB,IN0,CTRL,NODE_NC }, { ENAB,IN0,CTRL,TYPE }, NULL, "VCA IC" },
#define DISCRETE_VCA_OP_AMP(NODE,ENAB,IN0,CTRL,TYPE)                    { NODE, DST_VCA_OP_AMP  , 4, { ENAB,IN0,CTRL,NODE_NC }, { ENAB,IN0,CTRL,TYPE }, NULL, "VCA Op Amp Circuit" },

/* from disc_flt.c */
/* generic modules */
#define DISCRETE_FILTER1(NODE,ENAB,INP0,FREQ,TYPE)                      { NODE, DST_FILTER1     , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,FREQ,TYPE }, NULL, "1st Order Filter" },
#define DISCRETE_FILTER2(NODE,ENAB,INP0,FREQ,DAMP,TYPE)                 { NODE, DST_FILTER2     , 5, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,FREQ,DAMP,TYPE }, NULL, "2nd Order Filter" },
/* Component specific */
#define DISCRETE_CRFILTER(NODE,ENAB,INP0,RVAL,CVAL)                     { NODE, DST_CRFILTER    , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "CR Filter" },
#define DISCRETE_CRFILTER_VREF(NODE,ENAB,INP0,RVAL,CVAL,VREF)           { NODE, DST_CRFILTER    , 5, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL,VREF }, NULL, "CR Filter to VREF" },
#define DISCRETE_OP_AMP_FILTER(NODE,ENAB,INP0,INP1,TYPE,INFO)           { NODE, DST_OP_AMP_FILT , 4, { ENAB,INP0,INP1,NODE_NC }, { ENAB,INP0,INP1,TYPE }, INFO, "Op Amp Filter" },
#define DISCRETE_RCDISC(NODE,ENAB,INP0,RVAL,CVAL)                       { NODE, DST_RCDISC      , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Discharge" },
#define DISCRETE_RCDISC2(NODE,SWITCH,INP0,RVAL0,INP1,RVAL1,CVAL)        { NODE, DST_RCDISC2     , 6, { SWITCH,INP0,NODE_NC,INP1,NODE_NC,NODE_NC }, { SWITCH,INP0,RVAL0,INP1,RVAL1,CVAL }, NULL, "RC Discharge 2" },
#define DISCRETE_RCFILTER(NODE,ENAB,INP0,RVAL,CVAL)                     { NODE, DST_RCFILTER    , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Filter" },
#define DISCRETE_RCFILTER_VREF(NODE,ENAB,INP0,RVAL,CVAL,VREF)           { NODE, DST_RCFILTER    , 5, { ENAB,INP0,NODE_NC,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL,VREF }, NULL, "RC Filter to VREF" },
/* For testing - seem to be buggered.  Use versions not ending in N. */
#define DISCRETE_RCDISCN(NODE,ENAB,INP0,RVAL,CVAL)                      { NODE, DST_RCDISCN     , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Discharge (New Type)" },
#define DISCRETE_RCDISC2N(NODE,SWITCH,INP0,RVAL0,INP1,RVAL1,CVAL)       { NODE, DST_RCDISC2N    , 6, { SWITCH,INP0,NODE_NC,INP1,NODE_NC,NODE_NC }, { SWITCH,INP0,RVAL0,INP1,RVAL1,CVAL }, NULL, "RC Discharge 2 (New Type)" },
#define DISCRETE_RCFILTERN(NODE,ENAB,INP0,RVAL,CVAL)                    { NODE, DST_RCFILTERN   , 4, { ENAB,INP0,NODE_NC,NODE_NC }, { ENAB,INP0,RVAL,CVAL }, NULL, "RC Filter (New Type)" },

/* from disc_dev.c */
/* Component specific */
#define DISCRETE_555_ASTABLE(NODE,RESET,R1,R2,C,CTRLV,OPTIONS)          { NODE, DSD_555_ASTBL   , 5, { RESET,R1,R2,C,CTRLV }, { RESET,R1,R2,C,CTRLV }, OPTIONS, "555 Astable" },
#define DISCRETE_555_MSTABLE(NODE,RESET,TRIG,R,C,OPTIONS)               { NODE, DSD_555_MSTBL   , 4, { RESET,TRIG,R,C }, { RESET,TRIG,R,C }, OPTIONS, "555 Monostable" },
#define DISCRETE_555_CC(NODE,RESET,VIN,R,C,RBIAS,RGND,RDIS,OPTIONS)     { NODE, DSD_555_CC      , 7, { RESET,VIN,R,C,RBIAS,RGND,RDIS }, { RESET,VIN,R,C,RBIAS,RGND,RDIS }, OPTIONS, "555 Constant Current VCO" },
#define DISCRETE_566(NODE,ENAB,VMOD,R,C,OPTIONS)                        { NODE, DSD_566         , 4, { ENAB,VMOD,R,C }, { ENAB,VMOD,R,C }, OPTIONS, "566" },

#define DISCRETE_OUTPUT(OPNODE,VOL)                                     { NODE_OP, DSO_OUTPUT   , 2, { OPNODE,NODE_NC }, {0,VOL }, NULL, "Output Node" },



/*************************************
 *
 *	Interface to the external world
 *
 *************************************/

struct node_description *discrete_find_node(int node);

int discrete_sh_start(const struct MachineSound *msound);
void discrete_sh_stop(void);
void discrete_sh_reset(void);
void discrete_sh_update(void);

WRITE_HANDLER(discrete_sound_w);
READ_HANDLER(discrete_sound_r);

#endif
