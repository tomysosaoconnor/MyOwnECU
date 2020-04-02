/*This CDI ignition uses TIMER 0 for measuring RPM and TIMER 1 for doing the spark delay*/

/*This ignition CANNOT perform negative advance!*/

/*
Author: Tomás Sosa O'Connor.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdint.h>

#include "setup.h"

void tmrInterruptSetup (void);
void extInterruptSetup (void);

int engrpm = 0;// Engine rpm.
volatile unsigned long engperiod = 500000;// Engine turning period in uS.

volatile unsigned long time_old_us = 0;
volatile unsigned long time_old_ms = 0;

volatile int engE = 1;// Flag.
volatile unsigned char ign_flag=0;// This flag tells if it's moment to set Timer 1 with the "precise_time" value or with the "tmr1_counter" value.

volatile unsigned long tmr1_counter = 0;// The times Timer 1 needs to overflow.
volatile unsigned long tmr1_ovf = 0;// Timer 1 overflows.
volatile unsigned int precise_time = 0;// Extra uS needed before firing, always < 25mS.

volatile unsigned long tmr0_counter = 0;// Timer 0 overflows.
volatile unsigned int rpm_calc_cont = 0;

unsigned int ignadv = 0;// Ignition advance in tenths of a degree.
volatile unsigned long igndelay = 0;// Ignition delay in uS.

const int rpmcal[2][20]=
{
{100, 100, 100, 100, 110, 150, 200, 250, 280, 300, 310, 320, 330, 330, 330, 330, 330, 290, 220, 0},//asociated advance correction.
{400, 600, 800, 1000, 1200, 1400, 1600, 1800, 2000, 2400, 2800, 3200, 3700, 4200, 4800, 5500, 6000, 6500, 7000, 7700}//rpm value.
};

unsigned char select = 0;
void setup()
{
  unsigned long tmr1_counter_inloop = 0;// The times Timer 1 needs to overflow. Each overflow is 25mS.
  unsigned long precise_time_inloop = 0;// Extra uS needed before firing, always < 25mS.
  
  // Sensors - inputs.
  DDRD &= ~(1<<PD2);// External interrupt input, PD2.
  
  // Actuators - outputs.
  DDRB |= (1<<IGN);// IGN pin with distributor.
  
  tmrInterruptSetup ();
  extInterruptSetup ();
}

void loop()
{
  
  for (select = TABLE_X_SIZE; rpmcal[1][select] > engrpm; select--)// Look for lower cell.
    {
      if (select <= 0)// If 0 or not found get out of the loop.
      {
        select = 0;
        goto loop1end;
      }
    }
  loop1end:
  if (engperiod < 2000000)// If the engine is turning faster than 30RPM.
  {engrpm = 60000000/engperiod;}// We calculate rpms.
  else
  {
    engrpm = 0;
  }
  ignadv = rpmcal[0][select];
  igndelay = ((DEF_ADV - ignadv) * engperiod) / 3600;// We calculate the delay time [uS] from the sensor input.
  tmr1_counter_inloop = igndelay / 25000;// Number of Timer 1 overflows to wait. We divide the time we have to wait by the time each Timer 1 overflow takes.
  precise_time_inloop = igndelay - tmr1_counter * 25000;// Our precise time, after the Timer 1 counts, in uS.
  
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)// We make sure no interrupts randomly modify the value of variables shared between the main loop and the ISRs.
  {
    tmr1_counter = tmr1_counter_inloop;
    precise_time = precise_time_inloop;
  }
}




void tmrInterruptSetup (void)
{
  cli();// Stop interrupts.
  
  // Configure timer 1 to CTC mode.
  TCCR1A = 0;// Set entire TCCR1A register to 0.
  TCCR1B = 0;// Same for TCCR1B.
  TCNT1  = 0;// Initialize counter value to 0.
  OCR1A = 49999;// Value in half microseconds of each Timer 1 overflow. It would overflow each 25mS.
  TCCR1B |= (1 << WGM12);// Turn on CTC mode.
  
  TCCR1B |= (1 << CS11);// Set CS11 for 8 prescaler.
  
  TIMSK1 |= (1 << OCIE1A);// Enable timer compare interrupt.
  
  sei();
}

void extInterruptSetup (void)
{
  cli();
  // Interrupt configuration code here.
  bit_set(ISC00,EICRA);// We turn on INT0 external interrupts.
  bit_set(ISC01,EICRA);
  bit_set(INT0,EIMSK);
  bit_clear(INT1,EIMSK);
  sei();
}

//*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* INTERRUPTS *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*//

ISR(INT0_vect)// Our ISR from the rpm sensor pin.
{
  rpm_calc_cont++;
  
  if (engrpm != 0)// If engine should be on/is turning.
  {
    // We begin to wait with our calculated delay.
    TCCR1B = 0x0A;// We turn on timer 1 with its 1/8 prescaler.
  }
  
  // This is important. If you're using tach signal on a 4 cylinder engine (IGN_TYPE=4), program will wait untill 4 pulses came from the input in order to prevent
  //the RPMs being 4 times bigger than what they should be.
  if (rpm_calc_cont >= N_CYL)// >= is for noise immunity.
  {
  engperiod = ((micros() - time_old_us)>0)?(micros() - time_old_us):engperiod;// Timer 0 current value.
  TCNT0 = 0;// We reset Timer 0.
  
  // We calculate engine period.
    engperiod += (1000 * (millis() - time_old_ms));// After doing the 'fast thing', we add up the rest of the Timer 0 overflows.
  
  if (STROKES == 4)
  {
    engperiod = (engperiod >> 1);// engperiod used to be the period for two engine turns, we compensate that.
  }
    
  rpm_calc_cont = 0;
  }
}

ISR(TIMER1_COMPA_vect)// Compare output from timer 1.
{
  
  if (tmr1_counter == tmr1_ovf)
  {
    if (ign_flag == 0)
    {
      // We fire up spark plug.
      bit_set(IGN,PORTB);
      // Then we stop and reset the timer 1 and related.
      TCCR1B = B00001000;
      TCNT1 = 0;
      OCR1A = 49999;// Restore the compare register.
      tmr1_ovf = 0;
      _delay_us(THY_IGN);//
      bit_clear(IGN,PORTB);
    }
    OCR1A = precise_time;
    endofif:
  }
  else if (tmr1_counter > tmr1_ovf)
  {
    tmr1_ovf++;// Number of current Timer 1 overflows.
  }
}
