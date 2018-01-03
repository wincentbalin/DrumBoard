Touch Demo Drum Board
---------------------

The history of this project began when I got the Capacitive Touch Demo Board at the MSP430 Day 2007. This little device contains a MSP430F2013 microcontroller, a 3V battery, a light emitting diode, a couple of discrete elements and 4 capacitive touch pads.

Some time later I made a music instrument out of it. The only hardware modifications needed were the sound output pins (between the port pin P2.6 = pin 13 of the microcontroller and the ground plane, connected to the VSS pin = pin 14; pin numbers belong to the pinout of the 14-pin TSSOP package). To match the impedance, a 1k resistor was put between the sound output (P2.6) and the audio connector (schematics in the file OneBitSoundAdapter.png in this archive).

The software for the microcontroller was modified.
The music instrument, which I intended to build, would have to be controlled by touching the pads and should be controllable with just four of them. Drum instruments fulfill both criteria.

As the microcontroller on the board does not have a DAC, it was convinient to look into the sources of the One Bit Drum Machine (link below) for inspirations. Coincidentally, a new kind of one bit sound synthesis was invented. Using the program Sonic Visualiser (link below), I looked at the sonogram of the sound I made, trying to beat-box the bass drum. The red line, meaning dominating frequency, was so clear, it almost invited to imitate it. Doing so created a very convincing one-bit bass drum sound.

A snare and a (closed) hi-hat were created similarily. Last sound was a simple on-off switching of the sound output pin. Because the sampling frequency was just 880 Hz, the resulting sound was in the bass range. The instruments were placed in the following layout:

Pad	Sound
-------------
 1	Bass drum
 2	Snare drum
 3	Bass sound
 4	Hi-Hat

The files belonging to the project are described below:


DrumBoard.c		Main firmware file. Modified original firmware source
DrumBoardSynth.c	Synthesizer algorithms. Main firmware file includes this file.
Measure_VLO_SW.s43	Assembler source of frequency measuring routine.
OneBitSoundAdapter.png	Schematics of the impedance adapter.
disclaimer.txt		Texas Instruments disclamer. Also contains the "freeware" license.
Readme.txt		This file.

There is also a video (link below) which shows both the music instrument and the process of it's creation.

Links
-----

Video about the music instrument	http://www.youtube.com/watch?v=DNse-V3BhKM
Article in german about the one bit drum synthesis	http://www.mikrocontroller.net/articles/OneBitSound
Sources and documentation for the original firmware/Schematics for the hardware (if the link does not work, search for keyword slaa363 at the website of Texas Instruments -- http://www.ti.com/)	http://focus.ti.com/general/docs/litabsmultiplefilelist.tsp?literatureNumber=slaa363a
MSP430 Day 2007 presentation slides	http://focus.ti.com/lit/ml/slap131/slap131.pdf
Texas Instruments document about capacitive sensing	http://focus.ti.com/lit/an/slaa379/slaa379.pdf
One Bit Drum Machine	http://web.media.mit.edu/~nvawter/projects/obdm/
Sonic Visualiser	http://www.sonicvisualiser.org/
