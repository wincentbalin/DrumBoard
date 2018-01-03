/*
 * From here on sound generation.
 */

// Sound output pin on P2.6
#define SOUND BIT6

typedef unsigned char u08;
typedef unsigned int u16;

// Exponential function
const u08 powRef[16] =
{
  0x80,0x85,0x8b,0x91, 
  0x98,0x9e,0xa5,0xad, 
  0xb5,0xbd,0xc5,0xce, 
  0xe7,0xe0,0xea,0xf5
};

// input range 0-255
// output: 1+2^(t1/16), nominal range: 1 <=> 65535, actual range: 1 <=> 0xf501(62721)
u16 my_exp(u08 input)
{
  u16 op = powRef[input & 0x0f] << 8;
  return (1 + (op >> (15 - (input >> 4))));
}

// Noise generator
u16 noiseReg = 1,
    noiseReg2 = 1;

u16 updNoise(void)
{
  u16 b1,b2,b3,b4;

  // 16-bit PRNG
  b1 = ((noiseReg&0x8000)>>15);
  b2 = ((noiseReg&0x4000)>>14);
  b3 = ((noiseReg&0x1000)>>12);
  b4 = ((noiseReg&0x0008)>> 4);
  b1 = (b1 ^ b2 ^ b3 ^ b4)&0x1;
  noiseReg <<= 1;
  noiseReg |= b1;

  // 15-bit PRNG
  b1 = ((noiseReg2&0x4000)>>14);
  b2 = ((noiseReg2&0x2000)>>13);
  b1 = (b1 ^ b2)&0x1;
  noiseReg2 <<= 1;
  noiseReg2 |= b1;

  return(noiseReg);
}


/*
  My own drums. BTW., Noah, thank you very much for the functions above.

  Do not forget -- real sampling rate is about 900Hz. Not very much.
*/
#define FS 880

/*
  Bass drum

  The sound is divided between two phases:
  1. Glissando from first frequency to the second
  2. Echo of the drum sound at the second frequency

  At the very beginnig of the first phase the noise
  from the punch should be added.

  While implementing many adjustments were made.
*/
// Freq1
#define D1F1 60
#define D1P1 (FS/D1F1)
#define D1F1DUR (FS/2)
// Freq2
#define D1F2 30
#define D1P2 (FS/D1F2)
#define D1F2DUR (FS/4)
// Length of every frequency in glissando
#define D1FGLDUR (D1F1DUR/(D1F1-D1F2))
u16 d1phase;
u08 d1div;
u08 d1period;

void drum1()
{
  // Calculate period
  if(d1phase == 0)
  {
    // If second part of sound ended, return silently
    if(d1period > D1P2)
    {
      P2OUT &= ~SOUND;
      return;
    }
    else
    {
      if(d1period == D1P2)
        d1phase = D1F2DUR;
      else
        d1phase = D1FGLDUR;
      
      d1period++;
    }
  }
  d1phase--;
  
  // Compute sound
  if(d1div < d1period)
  {
    d1div++;
  }
  else
  {
    d1div = 0;
    P2OUT ^= SOUND;
    
    // Noise at the beginning of sound
    if(d1period == D1P1 && (updNoise() & 0x1))
      P2OUT ^= SOUND;
  }
}

void drum1Trigger()
{
  d1phase = D1FGLDUR;
  d1div = 0;
  d1period = D1P1;
}

/*
  Snare drum

  The sound is divided between two phases:
  1. Noise together with resonance sound of the drum
  2. Just noise
*/
// Resonance frequency of the drum
#define D2FRES 100
#define D2PERIOD (FS/D2FRES)
#define D2PH1DUR (FS/4)
#define D2PH2DUR (FS/4)
u16 d2phase;
u08 d2div;
//u08 d2period;

void drum2()
{
  if(d2phase == 0)
  {
    P2OUT &= ~SOUND;
    return;
  }
  d2phase--;

  // Do some noise
  if(updNoise() & 0x1)
    P2OUT ^= SOUND;

  // Phase 2 is noise-only
  if(d2phase < D2PH2DUR)
    return;
  
  // Phase 1 is with sound
  if(d2div < D2PERIOD)
  {
    d2div++;
  }
  else
  {
    d2div = 0;
    P2OUT ^= SOUND;
  }
}

void drum2Trigger()
{
  d2div = 0;
  d2phase = D2PH1DUR+D2PH2DUR;
}

/*
  Buzz drum.

  Just a buzzer with F ~= 30Hz.
*/
#define D3DIV 30
u08 d3div;

void drum3()
{  
  if(d3div < D3DIV)
  {
    d3div++;
  }
  else
  {
    d3div = 0;
    P2OUT ^= SOUND;
  }
}

void drum3Trigger()
{
  d3div = 0;
}

/*
  Closed hi-hat.

  Essentially a very short sample of noise.
*/
#define D4DUR (FS/10);
u08 d4phase;

void drum4()
{
  // Check duration
  if(d4phase == 0)
  {
    P2OUT &= ~SOUND;
    return;
  }
  d4phase--;

  // Generate noise
  if(updNoise() & 0x1)
    P2OUT |= SOUND;
  else
    P2OUT &= ~SOUND;
}

void drum4Trigger()
{
  d4phase = D4DUR;
}



/* Backup of the state of the keys. */
unsigned char key_press_old[Num_Sen];


