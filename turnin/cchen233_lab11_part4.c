/* Author:Christopher Chen
 * Partner(s) Name (if applicable):  
 * Lab Section:21
 * Assignment: Lab #11  Exercise #4
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link:https://www.youtube.com/watch?v=HCQlzRLqcMo
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

task tasks[1];

const unsigned char tasksNum = 1;


const unsigned long tasksPeriodGCD = 1;
const unsigned long periodKP = 50;
const unsigned long periodSQ = 50;
const unsigned long periodIS = 100;
const unsigned long periodSample = 200;


enum KP_States { KP_SMStart, KP_wait, KP_inc, KP_dec, KP_reset };
int TickFct_KP(int state);


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


int main() {
 

  DDRA = 0x00; PORTA = 0x03;
  DDRC = 0x0F; PORTC = 0x00;
  unsigned char i=0;
  tasks[i].state = KP_SMStart;
  tasks[i].period = periodKP;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_KP;

  TimerOn();

  
  
  while(1) {
  }
  return 0;
}



int TickFct_KP(int state) {

	unsigned  char tmpA = ~PINA & 0x03;
	static short count;
  switch(state) { // Transitions
     case KP_SMStart: // Initial transition
	count = 0x02FF;
        state = KP_wait;
        break;
     case KP_wait:
	if(tmpA == 1) {
		state = KP_inc;
		if(count < 0xFFFF) count++;
	}
	else if(tmpA == 2) {
		state = KP_dec;
		if(count > 0x00) count--;
	}
	else if(tmpA == 3) state = KP_reset;
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
	state = KP_wait;
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
     case KP_reset:
	count = 0;
	break;
     default:
        state = KP_SMStart;
	break;
	
   } // Transitions
  transmit_data(count);
  //transmit_data(0x1234);
  return state;
}
