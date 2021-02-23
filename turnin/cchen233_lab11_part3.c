/* Author:Christopher Chen
 * Partner(s) Name (if applicable):  
 * Lab Section:21
 * Assignment: Lab #11  Exercise #3
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link:https://www.youtube.com/watch?v=mCzQx5vLDIs
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

void transmit_data(unsigned char data, unsigned char cs) {
    int i;
    for (i = 0; i < 8 ; ++i) {
   	 // Sets SRCLR to 1 allowing data to be set
   	 // Also clears SRCLK in preparation of sending data
	 if(cs == 0) PORTC = 0x08;
	 else if (cs == 1) PORTC = 0x20;
   	 // set SER = next bit of data to be sent.
   	 PORTC |= ((data >> i) & 0x01);
   	 // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
   	 PORTC |= 0x02;  
    }
    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    if(cs == 0) PORTC |= 0x04;
    else if(cs == 1) PORTC |= 0x10;
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
}


unsigned long _avr_timer_M = 1; //start count from here, down to 0. Dft 1ms
unsigned long _avr_timer_cntcurr = 0; //Current internal count of 1ms ticks

void TimerOn(){
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; //bit 3 = 0: CTC mode (clear timer on compare)
	//AVR output compare register OCR1A
	OCR1A = 125; // Timer interrupt will be generated when TCNT1 == OCR1A
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; //bit1: OCIE1A -- enables compare match interrupt
	//Init avr counter
	TCNT1 = 0;

	_avr_timer_cntcurr = _avr_timer_M;
	//TimerISR will be called every _avr_timer_cntcurr ms
	
	//Enable global interrupts 
	SREG |= 0x80; //0x80: 1000000

}

void TimerOff(){
	TCCR1B = 0x00; //bit3bit1bit0 = 000: timer off
}


ISR(TIMER1_COMPA_vect){
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
			TimerISR();
			_avr_timer_cntcurr = _avr_timer_M;
			}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}




typedef struct task {
  int state; // Current state of the task
  unsigned long period; // Rate at which the task should tick
  unsigned long elapsedTime; // Time since task's previous tick
  int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[5];

const unsigned char tasksNum = 5;


const unsigned long tasksPeriodGCD = 1;
const unsigned long periodKP = 100;
const unsigned long periodKP2 = 100;
const unsigned long periodP1 = 200;
const unsigned long periodP2 = 200;
const unsigned long periodP3 = 200;


enum KP_States { KP_SMStart, KP_wait, KP_inc, KP_dec, KP_reset,KP_waitOn, KP_off };
int TickFct_KP(int state);

enum KP2_States { KP2_SMStart, KP2_wait, KP2_inc, KP2_dec, KP2_reset,KP2_waitOn, KP2_off };
int TickFct_KP2(int state);

enum P1_States { P1_SMStart,P1_init, P1_inc, P1_dec};
int TickFct_P1(int state);

enum P2_States { P2_SMStart,P2_init, P2_inc, P2_dec};
int TickFct_P2(int state);

enum P3_States { P3_SMStart,P3_init, P3_inc, P3_dec};
int TickFct_P3(int state);


void TimerISR() {
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
     if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
     }
     tasks[i].elapsedTime += tasksPeriodGCD;
  }
}

unsigned char mirror(unsigned char lsb){
	unsigned char out;
	out = lsb & 0x0F;
	out = out | (lsb & 0x08) << 1;
	out = out | (lsb & 0x04) << 3;
	out = out | (lsb & 0x02) << 5;
	out = out | (lsb & 0x01) << 7;
	
	return out;
}

unsigned char tmpBUL;
unsigned char t2unlock; //0 = locked    1 = unlocked
unsigned char tmpBT1;
unsigned char keyPressed;
unsigned char p1, p2, p3;



int main() {
 

  DDRA = 0x00; PORTA = 0x0F;
  DDRC = 0x3F; PORTC = 0x00;
  unsigned char i=0;
  tasks[i].state = KP_SMStart;
  tasks[i].period = periodKP;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_KP;
  ++i;
  tasks[i].state = KP2_SMStart;
  tasks[i].period = periodKP2;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_KP2;
  ++i;
  tasks[i].state = P3_SMStart;
  tasks[i].period = periodP3;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_P3;
  ++i; 
  tasks[i].state = P2_SMStart;
  tasks[i].period = periodP2;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_P2;
  ++i;
  tasks[i].state = P1_SMStart;
  tasks[i].period = periodP1;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_P1;
  
  
  /*
  ++i;
  tasks[i].state = SQ_SMStart;
  tasks[i].period = periodSQ;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_detectSQ;
  ++i;
  tasks[i].state = IS_SMStart;
  tasks[i].period = periodIS;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_IS;
  ++i
  tasks[i].state = OnOff_SMStart;
  tasks[i].period = periodSample;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_OnOff;
  ++i;
  //TimerSet(tasksPeriodGCD);
  */

  TimerOn();

  
  
  while(1) {
  }
  return 0;
}



int TickFct_KP(int state) {

	unsigned  char tmpA = ~PINA & 0x03; //0x0C >> 2
	static unsigned  char psl;
	static unsigned char dOut;

  switch(state) { // Transitions
     case KP_SMStart: // Initial transition
        state = KP_wait;
        break;
     case KP_wait:
	if(tmpA == 3) state = KP_reset;
	else if(tmpA == 1) {
		state = KP_inc;
		if(psl < 0x02) psl++;
		else psl = 0x00;
	}
	else if(tmpA == 2) {
		state = KP_dec;
		if(psl > 0x00) psl--;
		else psl = 2;
	}
	else if(tmpA == 0) state = KP_wait;
        break;
     case KP_inc:
	if(tmpA == 1) state = KP_inc;
	else if(tmpA == 3) state = KP_reset;
	else state = KP_wait;
        break;
     case KP_dec:
	if(tmpA == 2) state = KP_dec;
	else if(tmpA == 3) state = KP_reset;
	else state = KP_wait;
        break;
     case KP_reset:
	if(tmpA == 0x03) {
		state = KP_reset;
	}
	else state = KP_off;
	break;
     case KP_off:
	if(tmpA == 0x03) {
		state = KP_waitOn;
	}
	else state = KP_off;
	break;
     case KP_waitOn:
	if(tmpA == 0x00) {
		state = KP_wait;
	}
	else state = KP_waitOn;
	break;
     default:
        //state = KP_reset;
	break;
  } // Transitions
  switch(state) { // Actions
     case KP_SMStart: // Initial transition
        psl = 0;
        break;
     case KP_wait:
        break;
     case KP_inc:
	//psl = 2;
	break;
     case KP_dec:
	//psl = 1;
        break;
     case KP_reset:
	psl = 3;
	break;
     case KP_off:
	psl = 3;
	break;
     case KP_waitOn:
	psl = 0;
	break;
     default:
        state = KP_SMStart;
	break;

   } // Transitions
  switch(psl){
	case 0:
	  dOut = p1;
	  break;
	case 1:
	  dOut = p2;
	  break;
	case 2:
	  dOut = p3;
	  break;
	case 3:
	  dOut = 0;
	  break;
	default:
	  break;
  }
  transmit_data(dOut,1);
  return state;
}


int TickFct_KP2(int state) {

	unsigned  char tmpA = (~PINA & 0x0C) >> 2; //0x0C >> 2
	static unsigned  char psl;
	static unsigned char dOut;

  switch(state) { // Transitions
     case KP2_SMStart: // Initial transition
        state = KP2_wait;
        break;
     case KP2_wait:
	if(tmpA == 3) state = KP2_reset;
	else if(tmpA == 1) {
		state = KP2_inc;
		if(psl < 0x02) psl++;
		else psl = 0x00;
	}
	else if(tmpA == 2) {
		state = KP2_dec;
		if(psl > 0x00) psl--;
		else psl = 2;
	}
	else if(tmpA == 0) state = KP2_wait;
        break;
     case KP2_inc:
	if(tmpA == 1) state = KP2_inc;
	else if(tmpA == 3) state = KP2_reset;
	else state = KP2_wait;
        break;
     case KP2_dec:
	if(tmpA == 2) state = KP2_dec;
	else if(tmpA == 3) state = KP2_reset;
	else state = KP2_wait;
        break;
     case KP2_reset:
	if(tmpA == 0x03) {
		state = KP2_reset;
	}
	else state = KP2_off;
	break;
     case KP2_off:
	if(tmpA == 0x03) {
		state = KP2_waitOn;
	}
	else state = KP2_off;
	break;
     case KP2_waitOn:
	if(tmpA == 0x00) {
		state = KP2_wait;
	}
	else state = KP2_waitOn;
	break;
     default:
        //state = KP2_reset;
	break;
  } // Transitions
  switch(state) { // Actions
     case KP2_SMStart: // Initial transition
        psl = 0;
        break;
     case KP2_wait:
        break;
     case KP2_inc:
	//psl = 2;
	break;
     case KP2_dec:
	//psl = 1;
        break;
     case KP2_reset:
	psl = 3;
	break;
     case KP2_off:
	psl = 3;
	break;
     case KP2_waitOn:
	psl = 0;
	break;
     default:
        state = KP2_SMStart;
	break;

   } // Transitions
  switch(psl){
	case 0:
	  dOut = p1;
	  break;
	case 1:
	  dOut = p2;
	  break;
	case 2:
	  dOut = p3;
	  break;
	case 3:
	  dOut = 0;
	  break;
	default:
	  break;
  }
  transmit_data(dOut,0);
  return state;
}


int TickFct_P1(int state) {
  switch(state) { // Transitions
     case P1_SMStart: // Initial transition
        state = P1_init;
        break;
     case P1_init:
	state = P1_inc;
	break;
     case P1_inc:
	if(p1==0x80) state = P1_dec;
	else state = P1_inc;
        break;
     case P1_dec:
	if(p1==0x01) state = P1_inc;
	else state = P1_dec;
        break;
     default:
        state = P1_SMStart;
	break;
   } // Transitions
  switch(state) { // Actions
     case P1_SMStart: // Initial transition
        break;
     case P1_init:
	p1 = 0x01;
	break;
     case P1_inc:
	p1 = p1 << 1;
        break;
     case P1_dec:
	p1 = p1 >> 1;
        break;
     default:
	break;
   } // Transitions
  //transmit_data(p1);
  return state;
}



int TickFct_P2(int state) {
  static unsigned char base;
  switch(state) { // Transitions
     case P2_SMStart: // Initial transition
        state = P2_init;
        break;
     case P2_init:
	state = P2_inc;
	break;
     case P2_inc:
	if(base==0x0F) state = P2_dec;
	else state = P2_inc;
        break;
     case P2_dec:
	if(base==0x01) state = P2_inc;
	else state = P2_dec;
        break;
     default:
        state = P2_SMStart;
	break;
   } // Transitions
  switch(state) { // Actions
     case P2_SMStart: // Initial transition
        break;
     case P2_init:
	base = 0x01;
	break;
     case P2_inc:
	base = base << 1 | 0x01;
        break;
     case P2_dec:
	base = base >> 1;
        break;
     default:
	break;
   } // Transitions
  p2 = mirror(base);
  //transmit_data(p2);
  return state;
}


int TickFct_P3(int state) {
  static unsigned char base;
  switch(state) { // Transitions
     case P3_SMStart: // Initial transition
        state = P3_init;
        break;
     case P3_init:
	state = P3_inc;
	break;
     case P3_inc:
	if(base==0x0F) state = P3_dec;
	else state = P3_inc;
        break;
     case P3_dec:
	if(base==0x01) state = P3_inc;
	else state = P3_dec;
        break;
     default:
        state = P3_SMStart;
	break;
   } // Transitions
  switch(state) { // Actions
     case P3_SMStart: // Initial transition
        break;
     case P3_init:
	base = 0x01;
	break;
     case P3_inc:
	base++;
        break;
     case P3_dec:
	base--;
        break;
     default:
	break;
   } // Transitions
  p3 = mirror(base);
  //transmit_data(p3);
  return state;
}
/*
int TickFct_Combined(int state) {
  switch(state) { // Transitions
     case C_SMStart: // Initial transition
        state = C_s1;
        break;
     case C_s1:
        state = C_s1;
        break;
     default:
        state = C_SMStart;
   } // Transitions

  switch(state) { // State actions
     case C_SMStart: // Initial transition
     	PORTD = tmpDT1 | tmpDT2 | tmpDT4; 
        break;
     case C_s1:
     	PORTD = tmpDT1 | tmpDT2 | tmpDT4; 
        break; 
     default:
     	PORTD = tmpDT1 | tmpDT2 | tmpDT4; 
        break;
  } // State actions
  return state;
}

*/
