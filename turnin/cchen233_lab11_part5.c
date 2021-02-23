
/*	Author: lab
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: PORTB = tmpBT1;Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

void transmit_data(unsigned short data) {
    int i;
    for (i = 0; i < 16 ; ++i) {
   	 // Sets SRCLR to 1 allowing data to be set
   	 // Also clears SRCLK in preparation of sending data
   	 PORTC = 0x08;
   	 // set SER = next bit of data to be sent.
   	 PORTC |= ((data >> i) & 0x01);
   	 // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
   	 PORTC |= 0x02;  
   	 if(i >= 8) PORTC |= 0x04;  
    }
    PORTC &= 0xFD;  
    PORTC |= 0x02;  
    PORTC &= 0xFB;  
    PORTC |= 0x04;  
    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    //PORTC |= 0x04;
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

task tasks[3];

const unsigned char tasksNum = 3;


const unsigned long tasksPeriodGCD = 1;
const unsigned long periodKP = 50;
const unsigned long periodP1 = 300;
const unsigned long periodCHK = 50;

enum KP_States { KP_SMStart, KP_wait, KP_inc, KP_dec, KP_reset };
int TickFct_KP(int state);

enum P1_States { P1_SMStart,P1_init, P1_inc, P1_dec};
int TickFct_P1(int state);

enum CHK_States { CHK_SMStart,CHK_PLAY, CHK_BOOM};
int TickFct_CHK(int state);

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


unsigned char tmpBUL;
unsigned char t2unlock; //0 = locked    1 = unlocked
unsigned char tmpBT1;
unsigned char keyPressed;
unsigned short alien;
unsigned short user;

int main() {
 

  DDRA = 0x00; PORTA = 0x07;
  DDRC = 0x0F; PORTC = 0x00;
  unsigned char i=0;

  tasks[i].state = P1_SMStart;
  tasks[i].period = periodP1;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_P1;
  ++i;
  tasks[i].state = KP_SMStart;
  tasks[i].period = periodKP;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_KP;
  ++i;
  tasks[i].state = CHK_SMStart;
  tasks[i].period = periodCHK;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_CHK;
  TimerOn();

  
  
  while(1) {
  }
  return 0;
}



int TickFct_KP(int state) {

	unsigned  char tmpA = ~PINA & 0x07;
  switch(state) { // Transitions
     case KP_SMStart: // Initial transition
	user = 0x0080;
        state = KP_wait;
        break;
     case KP_wait:
	if(tmpA == 1) {
		state = KP_inc; 
		if(user != 0x8000) user = user << 1;
		else user = 0x8000;
	}
	else if(tmpA == 5) {
		state = KP_inc;
		if(user <= 0x0400) user = user << 4;
		else user = 0x8000;
	}
	else if(tmpA == 2) {
		state = KP_dec;
		if(user != 0x0001) user = user >> 1;
		else user = 0x0001;
	}
	else if(tmpA == 6) {
		state = KP_dec;
		if(user >= 0x0010) user = user >> 4;
		else user = 0x0001;
	}
	else if(tmpA == 0) state = KP_wait;
        break;
     case KP_inc:
	if(tmpA == 1 || tmpA == 5 || tmpA == 4) state = KP_inc;
	else state = KP_wait;
        break;
     case KP_dec:
	if(tmpA == 2 || tmpA == 6 || tmpA == 4) state = KP_dec;
	else state = KP_wait;
        break;
     default:
        state = KP_SMStart;
	break;
   } // Transitions
  switch(state) { // Actions
     case KP_SMStart: // Initial transition
        break;
     case KP_wait:
        break;
     case KP_inc:
	break;
     case KP_dec:
        break;
     default:
        state = KP_SMStart;
	break;
	
   } // Transitions
  transmit_data(user);
  //transmit_data(0x1234);
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
	if(alien==0x8000) state = P1_dec;
	else state = P1_inc;
        break;
     case P1_dec:
	if(alien==0x01) state = P1_inc;
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
	alien = 0x01;
	break;
     case P1_inc:
	alien = alien << 1;
        break;
     case P1_dec:
	alien = alien >> 1;
        break;
     default:
	break;
   } // Transitions
  transmit_data(alien);
  return state;
}


int TickFct_CHK(int state) {
	static unsigned short dOut;
	static unsigned char i,j;
  switch(state) { // Transitions
     case CHK_SMStart: // Initial transition
        state = CHK_PLAY;
        break;
     case CHK_PLAY:
	if(user != alien) state = CHK_PLAY;
	else {
		i = 0;
		j = 0;
		dOut = 0xFFFF;
		state = CHK_BOOM;
	}
        break;
     case CHK_BOOM:
	if(j == 6) {
		state = CHK_PLAY;
	}
        break;
     default:
        state = CHK_SMStart;
	break;
   } // Transitions
  switch(state) { // Actions
     case CHK_SMStart: // Initial transition
        break;
     case CHK_PLAY:
	dOut = alien | user;
	break;
     case CHK_BOOM:
	if(i <= 3) i++;
	else{
		i = 0;
		j++;
		dOut = ~dOut;
	}
        break;
     default:
	break;
   } // Transitions
  transmit_data(dOut);
  return state;
}
