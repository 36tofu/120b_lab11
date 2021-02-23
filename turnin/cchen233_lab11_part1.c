/* Author:Christopher Chen
 * Partner(s) Name (if applicable):  
 * Lab Section:21
 * Assignment: Lab #11  Exercise #1
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link:https://www.youtube.com/watch?v=OQvUXKNrKJ0
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

void transmit_data(unsigned char data) {
    int i;
    for (i = 0; i < 8 ; ++i) {
   	 // Sets SRCLR to 1 allowing data to be set
   	 // Also clears SRCLK in preparation of sending data
   	 PORTC = 0x08;
   	 // set SER = next bit of data to be sent.
   	 PORTC |= ((data >> i) & 0x01);
   	 // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
   	 PORTC |= 0x02;  
    }
    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    PORTC |= 0x04;
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

/*
const unsigned long periodThreeLEDs = 300;
unsigned long periodSpeaker = 2;
const unsigned long periodCombined = 1;
const unsigned long periodSample = 200;
*/

enum KP_States { KP_SMStart, KP_wait, KP_inc, KP_dec, KP_reset };
int TickFct_KP(int state);

/*
enum SQ_States { SQ_SMStart, SQ_init, SQ_begin, SQ_wait, SQ_check, SQ_match };
int TickFct_detectSQ(int state);


enum IS_States { IS_SMStart, IS_unlock, IS_lock };
int TickFct_IS(int state);


enum OnOff_States { OnOff_SMStart, s_Off, s_On};
int TickFct_OnOff(int state);

enum SP_States { SP_SMStart, SP_s0, SP_s1};
int TickFct_Speaker(int state);

enum FRQ_States { FRQ_SMStart, FRQ_s0, FRQ_inc, FRQ_dec};
int TickFct_FRQ(int state);

*/

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

	unsigned  char tmpA = ~PINA & 0x03;
	static unsigned  char count;
  switch(state) { // Transitions
     case KP_SMStart: // Initial transition
        state = KP_wait;
        break;
     case KP_wait:
	if(tmpA == 1) {
		state = KP_inc;
		if(count < 0xFF) count++;
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
     default:
        state = KP_SMStart;
   } // Transitions
  switch(state) { // Actions
     case KP_SMStart: // Initial transition
        count = 0;
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
  return state;
}
/*
int TickFct_detectSQ(int state) {
  unsigned char recBtn;
  const unsigned char seq[7] = {'#','1','2','3','4','5'};
  static unsigned char i = 0;
  switch(state) { // Transitions
     case SQ_SMStart: // Initial transition
        state = SQ_init;
        break;
     case SQ_init:
        state = SQ_begin;
        break;
     case SQ_begin:
        state = SQ_wait;
        break;
     case SQ_wait:
	if(Q7ucEmpty(btnQ))state = SQ_wait;
	else{
		recBtn = Q7ucPop(&btnQ);
		state = SQ_check;
	}
        break;
     case SQ_check:
	if(recBtn == seq[i])state = SQ_match;
	else state = SQ_begin;
        break;
     case SQ_match:
	if(i!=5){
		i++;
		state = SQ_wait;
	}
	else{
		state = SQ_begin;
		t2unlock = 1;
	}
        break;
     default:
        state = SQ_SMStart;
   } // Transitions

  switch(state) { // state actions
     case SQ_SMStart: // 
        break;
     case SQ_init:
	t2unlock = 0;
        break;
     case SQ_begin:
	i = 0;
        break;
     case SQ_wait:
        break;
     case SQ_check:
	break;
     case SQ_match:
        break;
     default:
        state = SQ_SMStart;
   } // Transitions
  return state;
}


int TickFct_IS(int state) {
  switch(state) { // Transitions
     case IS_SMStart: // Initial transition
        state = IS_unlock;
        break;
     case IS_unlock:
	if(~PINA & 0x01 == 1) state = IS_lock;
	else state = IS_unlock;
        break;
     case IS_lock:
	if(~PINA & 0x01 == 1) state = IS_lock;
	else state = IS_unlock;
        break;
     default:
        state = IS_SMStart;
   } // Transitions
  switch(state) { // State actions
     case IS_unlock:
        break;
     case IS_lock:
	t2unlock = 0;
	break;
     default:
        break;
  } // State actions
  //PORTD = tmpDT1;
  return state;
}


int TickFct_OnOff(int state) {
  unsigned char tmpA = (~PINA & 0x02); 
  switch(state) { // Transitions
     case OnOff_SMStart: // Initial transition
        state = s_Off;
        break;
     case s_Off:
	count = 0;
	if (tmpA == 0x02) state = s_On;
	else state = s_Off;
        break;
     case s_On:
	if (count < MAXCOUNT-1) {
		count++;
		state = s_On;
	}
	else{
		count = 0;
		state = s_Off;
	}
        break;
     default:
        state = OnOff_SMStart;
	break;
   } // Transitions

  switch(state) { // State actions
     case OnOff_SMStart: // Initial transition
        break;
     case s_Off:
	set_PWM(0);
        break;
     case s_On:
	set_PWM(frequency[keySeq[count]]);
        break;
     default:
        break;
  } // State actions
  return state;
}




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

int TickFct_Speaker(int state) {
	unsigned char tmpA = (~PINA & 0x04) >> 2; 
  switch(state) { // Transitions
     case SP_SMStart: // Initial transition
        tmpDT4 = 0; // Initialization behavior
        state = SP_s0;
        break;
     case SP_s0:
	if(tmpA == 0x01)
		state = SP_s1;
	else
		state = SP_s0;
     case SP_s1:
	if(tmpA == 0x01)
        	state = SP_s1;
	else
		state = SP_s0;
        break;
     default:
        state = SP_SMStart;
   } // Transitions

  switch(state) { // State actions
     case SP_s0:
        tmpDT4 = 0x00;
        break;
     case SP_s1:
        tmpDT4 ^= 0x10;
        break;
     default:
        break;
  } // State actions
  return state;
}

int TickFct_FRQ(int state) {
unsigned char tmpA = (~PINA & 0x03);
  switch(state) { // Transitions
     case FRQ_SMStart: // Initial transition
        state = FRQ_s0;
        break;
     case FRQ_s0:
	if(tmpA == 0x00)
		state = FRQ_s0;
	else if(tmpA == 0x01)
		state = FRQ_dec;
	else if(tmpA == 0x02)
		state = FRQ_inc;
	break;
     case FRQ_inc:
	state = FRQ_s0;
        break;
     case FRQ_dec:
	state = FRQ_s0;
        break;
     default:
        state = FRQ_SMStart;
   } // Transitions

  switch(state) { // State actions
     case FRQ_s0:
  	//tasks[3].period = 1;
        break;
     case FRQ_inc:
        periodSpeaker--;
  	tasks[3].period = periodSpeaker;
        break;
     case FRQ_dec:
        periodSpeaker++;
  	tasks[3].period = periodSpeaker;
        break;
     default:
        break;
  } // State actions
  return state;
}


int main(void)
{
	unsigned char x;
	DDRD = 0xFF; PORTD = 0x00; 
	DDRC = 0xF0; PORTC = 0x0F;
	while(1){
		x = GetKeypadKey();
		switch(x) {
			case '\0': PORTD = 0x1F; break;
			case '1': PORTD = 0x01; break;
			case '2': PORTD = 0x02; break;
			case '3': PORTD = 0x03; break;
			case '4': PORTD = 0x04; break;
			case '5': PORTD = 0x05; break;
			case '6': PORTD = 0x06; break;
			case '7': PORTD = 0x07; break;
			case '8': PORTD = 0x08; break;
			case '9': PORTD = 0x09; break;
			case 'A': PORTD = 0x0A; break;
			case 'B': PORTD = 0x0B; break;
			case 'C': PORTD = 0x0C; break;
			case 'D': PORTD = 0x0D; break;
			case '*': PORTD = 0x0E; break;
			case '0': PORTD = 0x00; break;
			case '#': PORTD = 0x0F; break;
			default: PORTD = 0x1B; break;
		}
	}
}
*/
