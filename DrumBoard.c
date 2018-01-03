/**
 * DrumBoard.c
 *
 * MSP430 Touch Demo Drum Board
 *
 * Main file.
 *
 * Adapted from the original Touch Demo code (by Zack Albus)
 * and One Bit Drum Machine (by Noah Vawter)
 * by Wincent Balin, 2008.
 */

//******************************************************************************
//  MSP430F20x3 Demo - Capacitive Touch Sensing 4-Key Demo
//
//  Description: This demo implements a 4-key capacitive touch detection.
//  The LED indicates the key which is pressed through four different levels of
//  brightness. Key#1 -> 100%, Key#2 -> 75%, Key#3 -> 50%, Key#4 -> 25%.
//  A calibration process is implemented to accommodate for possible variations
//  /* in VLO frequency. Normal operating mode is LPM3. */
//  in VLO frequency. Normal operating mode is LPM0.
//
//  /* ACLK = VLO ~ 12kHz, MCLK = Calibrated 8MHz / 4 = 2MHz, */
//  ACLK = VLO ~ 12kHz, MCLK = 8MHz,
//  SMCLK = Calibrated 8MHz
//
//                MSP430F20x3                   /|
//             -----------------     R=1K    +-+ |
//         /|\|             P2.6|---#####--->| | | SOUND
//          | |                 |            +-+ |
//          --|RST              |               \|
//            |                 |
//            |             P1.0|---->LED
//            |                 |             ####
//            |             P1.2|----+--------#### Sensor#4
//            |                 |    #        ####
//            |                 |    # R=5.1M
//            |                 |    #        ####
//            |             P1.3|----+--------#### Sensor#3
//            |                 |             ####
//            |                 |
//            |                 |             ####
//            |             P1.4|----+--------#### Sensor#2
//            |                 |    #        ####
//            |                 |    # R=5.1M
//            |                 |    #        ####
//            |             P1.5|----+--------#### Sensor#1
//            |                 |             ####
//
//  Capacitative sensor interface:
//  Zack Albus
//  Texas Instruments Inc.
//  June 2007
//  Built with IAR Embedded Workbench Version: 3.42A
//
//  Sound generators:
//  Noah T. Vawter, MIT, 2007-2008
//  
//  The code of generators had to be adapted to the absence of inputs
//  (drum triggers aside).
//
//******************************************************************************
#include  "msp430x20x3.h"

// Define User Configuration values
// Sensor settings
#define Num_Sen     4                       // Defines number of sensors
#define S_4         (0x04)                  // Sensor 4 P1.2
#define S_3         (0x08)                  // Sensor 3 P1.3
#define S_2         (0x10)                  // Sensor 2 P1.4
#define S_1         (0x20)                  // Sensor 1 P1.5

#define LED         (0x01)                  // P1.0


#define min_KEY_lvl         30              // Defines the min key level threshold usable
#define Sample_Rate         20              // Defines #/sec all sensors are sampled
#define DCO_clks_per_sec    8000000         // Number of DCO clocks per second: 2MHz
                                            // Changes with DCO freq selected!
#define DCO_clks_per_sample     (DCO_clks_per_sec/Sample_Rate)/8
                                            // Clocks per sample cycle /8
                                            // /8 is allows integer usage
                                            // will be *8 in the final calcs
#define LED_pulses_per_sample   5           // Defines LED pulses during each sample
                                            // -number of TACCR0 ints per sample


// Global variables for sensing
unsigned int dco_clks_per_vlo;              // Variable used for VLO freq measurement
unsigned int vlo_clks_per_sample;           // Variable that determines vlo clocks per sample

// vlo_clks_per_sample = DCO_clks_per_sample/dco_clks_per_vlo*8
unsigned int vlo_clks_per_LED_pulse;        // Variable that determines vlo clocks per LED pulse

// Misc Globals
unsigned int base_cnt[Num_Sen];
unsigned int meas_cnt[Num_Sen];
int delta_cnt[Num_Sen];
unsigned char key_press[Num_Sen];
char key_pressed, key_loc, no_key, key_loc_old, key_time_out;
char cycles;
unsigned int timer_count;
unsigned int KEY_lvl = 100;                 // Defines the min count for a "key press"

/* Added base level backup, to avoid watchdog restarts. */
unsigned int base_cnt_backup[Num_Sen];

// System Routines
void Init_Timings_Consts(void);             // Use VLO freq to determine TA values
void measure_count(void);                   // Measures each capacitive sensor

extern unsigned int Measure_VLO_SW(void);   // External function to measure
                                            // speed of the VLO
                                            // (implemented in Measure_VLO_SW.s43)
// Include synth routines
#include "DrumBoardSynth.c"

// Main Function
void main(void)
{
  unsigned int i,j;
  int temp;

  // Configure clock system
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
  dco_clks_per_vlo = Measure_VLO_SW();      // Determine VLO freq for usage
  BCSCTL2 |= DIVM_0;                        // MCLK / 1
  BCSCTL1 = CALBC1_8MHZ;                    // Set DCO to 8MHz
  DCOCTL = CALDCO_8MHZ;                     // MCLK = 8MHz
  BCSCTL3 |= LFXT1S_2;                      // LFXT1 = VLO

  // Configure GPIOs
  P1OUT = 0x00;                             // P1.x = 0
  P1DIR = 0xFF;                             // P1.x = output
  P2OUT = 0x00;
  P2DIR = 0xFF;                             // P2.x = output
  P2SEL = 0x00;                             // No XTAL
  
  // Configure timer
  TACTL = TASSEL_2+MC_2;                    // SMCLK, cont mode
  TACCTL0 = CCIE;                           // Allow Capture/Compare0 interrupt

  __enable_interrupt();                     // Enable interrupts

  Init_Timings_Consts();
  measure_count();                          // Establish an initial baseline capacitance

  for (i = 0; i<Num_Sen; i++)
    base_cnt[i] = meas_cnt[i];

  for (i=15; i>0; i--)                      // Repeat and average base measurement
  {
    measure_count();
    for (j = 0; j<Num_Sen; j++)
      base_cnt[j] = (meas_cnt[j]+base_cnt[j])/2;
  }

  /* Backup base levels. */
  for (i = 0; i<Num_Sen; i++)
    base_cnt_backup[i] = base_cnt[i];
  
  no_key = 0;

  // Main loop starts here
  while (1)
  {
    key_pressed = 0;                    // Assume no keys are pressed

    measure_count();                    // Measure all sensors

    for (i = 0; i<Num_Sen; i++)
    {
      delta_cnt[i] =  meas_cnt[i] - base_cnt[i];  // Calculate delta: c_change

      // Handle baseline measurment for a base C decrease
      if (delta_cnt[i] < 0)             // If negative: result decreased
      {                                 // below baseline, i.e. cap decreased
          base_cnt[i] = (base_cnt[i]+meas_cnt[i]) >> 1; // Re-average baseline down quickly
          delta_cnt[i] = 0;             // Zero out delta for position determination
      }

      if (delta_cnt[i] > KEY_lvl)       // Determine if each key is pressed per a preset threshold
      {
        key_press[i] = 1;               // Specific key pressed
        key_pressed = 1;                // Any key pressed
        key_loc = i;
      }
      else
        key_press[i] = 0;
    }

    // Handle baseline measurement for a base C increase
    if (!key_pressed)                   // Only adjust baseline up if no keys are touched
    {
      key_time_out = Sample_Rate*3;     // Reset key time out duration
      for (i = 0; i<Num_Sen; i++)
        base_cnt[i] = base_cnt[i] + 1;  // Adjust baseline up, should be slow to
    }                                   // accomodate for genuine changes in sensor C
    else                                // Key pressed
    {
      if (key_loc_old == key_loc)       // same key pressed?
        key_time_out--;
      else
        key_time_out = Sample_Rate*3;   // Reset key time out duration

      key_loc_old = key_loc;

      if (key_time_out == 0)            // After time-out, re-init base and key level
//        WDTCTL = WDTHOLD;               // FORCE RESET
      {
        for (i = 0; i<Num_Sen; i++)     // Restore base levels
          base_cnt[i] = base_cnt_backup[i];
        KEY_lvl = 100;                  // Reset key level
      }
    }

    // Dynamically adjust key thresholds
    if (key_pressed && (delta_cnt[key_loc]>>2 > KEY_lvl))
    {
      temp = delta_cnt[key_loc]>>2;
      temp = temp + KEY_lvl;
      temp = temp>>1;
      KEY_lvl = temp;                   // Average result
    }
    else if (!key_pressed)
    {
      for (i = 0; i<Num_Sen; i++)
      {
        if(delta_cnt[i] > min_KEY_lvl)
        {
          temp = delta_cnt[i]>>1;       // Means min key level threshold = %50 of delta measured
          temp = temp + KEY_lvl;        // or = min_KEY_lvl/2
          temp = temp>>1;
          KEY_lvl = temp;               // Average result
          break;
        }
      }
    }

    if (key_pressed)                    // Pulse LED and use defined sample rate
    {
      no_key = Sample_Rate*3;           // Reset ~3 sec delay until slower sample rate is used
      cycles = LED_pulses_per_sample;   // Re-set LED pulse counter

      P1OUT |= LED;                     // Turn on LED

      /* Trigger drums. */
      if(key_press[(4-1)] == 1 && key_press_old[(4-1)] == 0)
        drum1Trigger();
      else if(key_press[(4-2)] == 1 && key_press_old[(4-2)] == 0)
        drum2Trigger();
      else if(key_press[(4-3)] == 1 && key_press_old[(4-3)] == 0)
        drum3Trigger();
      else if(key_press[(4-4)] == 1 && key_press_old[(4-4)] == 0)
        drum4Trigger();
    }
    else                                // No key is pressed...
    {
      if (no_key == 0)                  // ...~3 sec timeout expired: no key press in ~3secs...
      {
        cycles = 1;                     // Adjust cycles for only one TA delay
      }
      else                              // ... still within 3 secs of last detected key press...
      {
        no_key--;                       // Decrement delay
        cycles = LED_pulses_per_sample; // Maintain sample_rate SPS, without LED
      }
    }

    // Backup pressed keys
    for(i = 0; i < Num_Sen; i++)
      key_press_old[i] = key_press[i];
    
    LPM0;
  }
} // End Main

// Measure count result (capacitance) of each sensor
// Routine setup for four sensors, not dependent on Num_Sen value!
void measure_count(void)
{
  unsigned char i;
  char active_key;

  for (i = 0; i<Num_Sen; i++)
  {
    active_key = 1 << i+2;                  // define bit location of active key

//******************************************************************************
// Negative cycle
//******************************************************************************
    P1OUT &=~(S_1+S_2+S_3+S_4);             // everything is low
    // Take the active key high to charge the pad
    P1OUT |= active_key;
    // Allow a short time for the hard pull high to really charge the pad
    __no_operation();
    __no_operation();
    __no_operation();
    // Enable interrupts (edge set to low going trigger)
    // set the active key to input (was output high), and start the
    // timed discharge of the pad.
    P1IES |= active_key;                    //-ve edge trigger
    P1IE |= active_key;
    P1DIR &= ~active_key;
    // Take a snaphot of the timer...
    timer_count = TAR;
    LPM0;
    // Return the key to the driven low state, to contribute to the "ground"
    // area around the next key to be scanned.
    P1IE &= ~active_key;                    // disable active key interrupt
    P1OUT &= ~active_key;                   // switch active key to low to discharge the key
    P1DIR |= active_key;                    // switch active key to output low to save power
    meas_cnt[i]= timer_count;
//****************************************************************************
// Positive Cycle
//****************************************************************************
    P1OUT |= (S_1+S_2+S_3+S_4);             // everything is high
    // Take the active key low to discharge the pad
    P1OUT &= ~active_key;
    // Allow a short time for the hard pull low to really discharge the pad
    __no_operation();
    __no_operation();
    __no_operation();
    // Enable interrupts (edge set to high going trigger)
    // set the active key to input (was output low), and start the
    // timed discharge of the pad.
    P1IES &= ~active_key;                   //+ve edge trigger
    P1IE |= active_key;
    P1DIR &= ~active_key;
    // Take a snaphot of the timer...
    timer_count = TAR;
    LPM0;
    // Return the key to the driven low state, to contribute to the "ground"
    // area around the next key to be scanned.
    P1IE &= ~active_key;                    // disable active key interrupt
    P1OUT &= ~active_key;                   // switch active key to low to discharge the key
    P1DIR |= active_key;                    // switch active key to output low to save power
    P1OUT &=~(S_1+S_2+S_3+S_4);             // everything is low
    meas_cnt[i] = (meas_cnt[i] + timer_count) >> 1; // Average the 2 measurements
  }
}

void Init_Timings_Consts(void)
{
//  dco_clks_per_vlo = dco_clks_per_vlo << 1; // x2 for 2MHz DCO
//  dco_clks_per_vlo = dco_clks_per_vlo << 2; // x4 for 4MHz DCO
  dco_clks_per_vlo = dco_clks_per_vlo << 3; // x8 for 8MHz DCO
//  dco_clks_per_vlo = dco_clks_per_vlo << 4; // x16 for 16MHz DCO

  vlo_clks_per_sample = DCO_clks_per_sample/dco_clks_per_vlo*8;
  vlo_clks_per_LED_pulse = vlo_clks_per_sample/LED_pulses_per_sample;
}

// Port 1 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void port_1_interrupt(void)
{
  P1IFG = 0;                                // clear flag
  timer_count = TAR - timer_count;          // find the charge/discharge time
  LPM0_EXIT;                                // Exit LPM0 on reti
}

// Timer_A0 interrupt service routine
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A0 (void)
{
  TACCR0 += (8000000/12000);
  
  if(key_pressed)
  {
    P1OUT |= LED;
//    P2OUT ^= SOUND;

    // Generate sound (monophone)
    if(key_press[4-1])
      drum1();
    else if(key_press[4-2])
      drum2();
    else if(key_press[4-3])
      drum3();
    else if(key_press[4-4])
      drum4();
  }
  else
    P1OUT &= ~LED;
  
  cycles--;
  if (cycles == 0)
    LPM0_EXIT;                              // Exit LPM0 on reti
}

