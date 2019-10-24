/*	Author: nmoor004
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab # 7 Exercise # 1
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "io.h"

#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))
          
/*-------------------------------------------------------------------------*/

#define DATA_BUS PORTC		// port connected to pins 7-14 of LCD display
#define CONTROL_BUS PORTD	// port connected to pins 4 and 6 of LCD disp.
#define RS 6			// pin number of uC connected to pin 4 of LCD disp.
#define E 7			// pin number of uC connected to pin 6 of LCD disp.

/*-------------------------------------------------------------------------*/


// Global variables
unsigned char ascii_zero = '0';
unsigned char num_LCD = 0;
unsigned char sum = 5;
const unsigned char WINRAR[] = "WINNER!";

/////////////////////////////////////////////////
////////////////////////////LCD FUNCS///////////
///////////////////////////////////////////////
void LCD_ClearScreen(void) {
   LCD_WriteCommand(0x01);
}

void LCD_init(void) {

    //wait for 100 ms.
	delay_ms(100);
	LCD_WriteCommand(0x38);
	LCD_WriteCommand(0x06);
	LCD_WriteCommand(0x0f);
	LCD_WriteCommand(0x01);
	delay_ms(10);						 
}

void LCD_WriteCommand (unsigned char Command) {
   CLR_BIT(CONTROL_BUS,RS);
   DATA_BUS = Command;
   SET_BIT(CONTROL_BUS,E);
   asm("nop");
   CLR_BIT(CONTROL_BUS,E);
   delay_ms(2); // ClearScreen requires 1.52ms to execute
}

void LCD_WriteData(unsigned char Data) {
   SET_BIT(CONTROL_BUS,RS);
   DATA_BUS = Data;
   SET_BIT(CONTROL_BUS,E);
   asm("nop");
   CLR_BIT(CONTROL_BUS,E);
   delay_ms(1);
}

void LCD_DisplayString( unsigned char column, const unsigned char* string) {
   LCD_ClearScreen();
   unsigned char c = column;
   while(*string) {
      LCD_Cursor(c++);
      LCD_WriteData(*string++);
   }
}

void LCD_Cursor(unsigned char column) {
   if ( column < 17 ) { // 16x1 LCD: column < 9
						// 16x2 LCD: column < 17
      LCD_WriteCommand(0x80 + column - 1);
   } else {
      LCD_WriteCommand(0xB8 + column - 9);	// 16x1 LCD: column - 1
											// 16x2 LCD: column - 9
   }
}

void delay_ms(int miliSec) //for 8 Mhz crystal

{
    int i,j;
    for(i=0;i<miliSec;i++)
    for(j=0;j<775;j++)
  {
   asm("nop");
  }
}


//////////////////////////////////////////////////////////
////////////////TIMER///////////////////////////
//////////////////////////////////////////////////////////


volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C Programmer should clear to 0.
unsigned char global_PINA;
unsigned long _avr_timer_M = 1;	       // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

enum LED_States {LED_Init, Play, WIN, Gain_Point} LED_State;

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;     // bit3 = 0: CTC mode (clear timer on compare)
		          // bit2bit1bit0 = 011: pre-scaler /64
			 // 00001011: 0x0B
			// SO, 8 MHz clock or 8,000,000 / 64 = 125,000 ticks/s
		       // Thus, TCNT1 register will count at 125,000 tick/s

	// AVR output compare register OCR1A.
	OCR1A = 125;    // Timer interrupt will be generated when TCNT1==OCR1A
		       // We want a 1 ms tick. 0.001s * 125,000 ticks/s = 125
		      // So when TCNT1 register equals 125,
		     // 1 ms has passed. Thus, we compare to 125.

	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	// Initialize avr counter
	TCNT1 = 0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	// Enable globla interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; /// bit3bit1bit0 = 000: timer off
}


void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP

	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); 	      // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}

}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}



void Tick_LED() {
	
	switch(LED_State) {
		case LED_Init:
			PORTB = 0x01;
			LED_State = Play;
			LCD_ClearScreen();
			num_LCD = sum + ascii_zero;
			LCD_WriteData(num_LCD);
			break;

		case Play:


			if ((PORTB == 0x02) && (global_PINA == 0x00)) {
				LED_State = Gain_Point;
				sum++;
				num_LCD = sum + ascii_zero;
				LCD_ClearScreen();
				LCD_WriteData(num_LCD);
				break;
			}
			else if ((PORTB != 0x02) && (global_PINA == 0x00)) {
				if (sum != 0) {				
					sum--;
					num_LCD = sum + ascii_zero;
					LCD_ClearScreen();
					LCD_WriteData(num_LCD);
				}
			}

			if (PORTB == 0x02 || PORTB == 0x01) {
				PORTB = PORTB << 1;
			}
			else if (PORTB == 0x04) {
				PORTB = 0x01;
			}
			break;
		case Gain_Point:

			if ((sum != 0x09)) {
				LED_State = Play;

			}
			else if ((sum == 0x09)) {
				LED_State = WIN;
				LCD_ClearScreen();
				LCD_DisplayString(1, WINRAR);
				sum = 0;
			}
			break;
		case WIN: 

			if (PINA == 0x00) {
				LED_State = LED_Init;
			}
			
	}

	switch(LED_State) {

		case LED_Init:
			break;
		case Play:
			break;
		case Gain_Point:
			break;
		case WIN:
			break;
	}



}








int main(void) {
    /* Insert DDR and PORT initializations */
	DDRA = 0x00; PORTA = 0x01;    // input
	DDRB = 0xFF; PORTB = 0x07;   // output
	DDRC = 0xFF; PORTC = 0x00;  // output
	DDRD = 0xFF; PORTD = 0x00; // output
	TimerSet(300); // Timer Set to: 300 ms
	TimerOn();
	LED_State = LED_Init;
	
	unsigned char toggle = 0;
	const unsigned char h[] = "Hello World";
	
    /* Insert your solution below */
	// Init LCD display
	LCD_init();

	// Starting @ pos1 on the LCD, writes Hello World
	LCD_DisplayString(1, h);

    while (1) {
	 
	Tick_LED();
	global_PINA = PINA;
	while (!TimerFlag) {
		if (toggle == 0) {
			if (PINA == 0x00) {
				global_PINA = PINA;
				toggle = 1;
			}
		}
	}; // Wait 1 sec
	toggle = 0;
	TimerFlag = 0;
    }
    return 1;
}
