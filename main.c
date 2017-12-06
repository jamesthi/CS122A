/*
 * FinalTesting.c
 *
 * Created: 11/7/2017 6:43:09 PM
 * Author : james
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "usart_ATmega1284.h"

#define F_CPU 8000000UL // assume uC operates at 8MHz

#define DIST_REG DDRB // sensor DDR
#define DIST_PORT PORTB // sensor PORT
#define ECHO_PIN 3 // input to listen for echo *** DEPENDENT ON INT0***
#define TRIG_PIN 3 // output to start sensor polling
#define DIST_TRIGGER 5 // output pin to trigger when object detected

#define UNIT_IN 148 // uS/148 = inches
#define UNIT_CM 58 // uS/58 = centimeters

volatile unsigned short pulse = 0;
volatile char pulse_flag = 0;

/*** Usart stuff ***/

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner Timer ISR model.
unsigned long _avr_timer_M = 1;
unsigned long _avr_timer_cntcurr = 0;

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	// in Free Running Mode, a new conversion will trigger whenever
	// the previous conversion completes.
}

void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff() {
	TCCR1B = 0x00;
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
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
/*** END USART STUFF ***/

ISR(INT1_vect)
{
	//PORTC = 0xFF;
	if (pulse_flag == 1) {
		TCCR3B = 0; // disable counter
		pulse = TCNT3; // store counter memory
		TCNT3 = 0; // reset counter memory
		pulse_flag = 0;
	}
	else if (pulse_flag == 0) {
		TCCR3B |= (1 << CS31); // enable counter
		pulse_flag = 1;
	}
}

void EnableDistance() {
	//PORTC = 0xAA;
	SREG |= 0x80; // enable global interrupts
	//PORTC = 0xAA;

	EICRA |= (1 << ISC10); // set interrupt to trigger on logic change
	EICRA &= ~(1 << ISC11); // set interrupt to trigger on logic change
	
	EIMSK |= (1 << INT1); // enable external interrupt 1 (PD3)
	//PORTC = 0xAA;
	
	// set sensor trigger pin as output
	DIST_REG |= (1 << TRIG_PIN); 
	DIST_PORT &= ~(1 << TRIG_PIN);
	// set sensor echo pin as input, enable pull-up
	DDRD &= ~(1 << ECHO_PIN); 
	PORTD |= (1 << ECHO_PIN);
	// set sensor output pin as output
	DIST_REG |= (1 << DIST_TRIGGER); 
	DIST_PORT &= ~(1 << DIST_TRIGGER);
	//PORTC = 0xFF;
}

// Triggers a new measurement

void TriggerPing() {
	DIST_PORT |= (1 << TRIG_PIN); // set trigger pin high
	_delay_us(15);
	DIST_PORT &= ~(1 << TRIG_PIN); // set trigger pin low
}

// Returns the distance in centimeters
unsigned short PingCM() {
	TriggerPing();
	return pulse/UNIT_CM;
}

// Returns the distance in inches 
unsigned short PingIN() {
	TriggerPing();
	return pulse / UNIT_IN;
}

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}

enum States{Initial, start, idle} state;

unsigned char steps[] = {0xF1, 0xF3, 0xF2, 0xF6, 0xF4, 0xFC, 0xF8, 0xF9};
   unsigned char tmpB = 0xAA;
   unsigned char tmpA = 0x00;	
   int cnt = 0;
   int maxcnt = 4096;
   int counter = 0;
   int toggle = 0;
   int speed = 2;
   int pace = 0;
   int measure = 0;
   short distance;
   int displaytoggle = 1;
	unsigned short x;

void Tick()
{
	switch(state)
	{
		case Initial:
			state = start;
			break;
		case start:
		    if (USART_HasReceived(0))
		    {
			    tmpA = USART_Receive(0);
			    //PORTC = tmpA;
				if (tmpA == 0xAA)
					state = idle;
				else if (tmpA == 0xA1)
					speed = 1;
				else if (tmpA == 0xA2)
					speed = 2;
				else if (tmpA == 0xA3)
					speed = 3;
		    }
			break;
		case idle:
		    if (USART_HasReceived(0))
		    {
			    tmpA = USART_Receive(0);
			    //PORTC = tmpA;
			    if (tmpA == 0xFF)
			    state = start;
		    }
		    break;
	}
	switch(state)
	{
		case Initial:
			break;
		case start:
			if (speed == 1 || (speed == 2 && pace % 2 == 0) || (speed == 3 && pace % 4 == 0))
			{
							PORTA = steps[counter];
							if (toggle == 0)
							{
								if (counter >= 7)
								counter = 0;
								else
								counter++;
								if (cnt >= maxcnt)
								{
									toggle = 1;
									cnt = 0;
								}
						}	
							else
							{
								if (counter <= 0)
								counter = 7;
								else
								counter--;
								if (cnt >= maxcnt)
								{
									toggle = 0;
									cnt = 0;
								}
							}
							cnt++;
			}
			pace++;
			if (pace >= 4)
				pace = 0;
				
			if (measure >= 250 && measure <= 999)
			{
				distance = PingIN();
				char firstdig = ((distance % 100) / 10) + 0x30;
				char seconddig = (distance % 10) + 0x30;
				if (distance > 25) // sensor has bad accuracy over 25, output an underscore meaning nothing in range
				{
					if (USART_IsSendReady(0)){
						USART_Send(0x5F, 0);
					}
					if (USART_HasTransmitted(0))
					USART_Flush(0);
				}
				else
				{
					if (USART_IsSendReady(0)){
						USART_Send(firstdig, 0);
					}
					if (USART_HasTransmitted(0))
					USART_Flush(0);
					
					if (USART_IsSendReady(0)){
						USART_Send(seconddig, 0);
					}
					if (USART_HasTransmitted(0))
					USART_Flush(0);	
				}
				measure = 1000;
			}
			else if (measure >= 1000)
			{
				if (USART_IsSendReady(0)){
					USART_Send(0x20, 0);
				}
				if (USART_HasTransmitted(0))
				USART_Flush(0);

				measure = 0;
			}
			measure++;
			

			break;
		case idle:
			if (measure >= 250 && measure <= 999)
			{
				distance = PingIN();
				char firstdig = ((distance % 100) / 10) + 0x30;
				char seconddig = (distance % 10) + 0x30;
				if (distance > 25) // sensor has bad accuracy over 25, output an underscore meaning nothing in range
				{
					if (USART_IsSendReady(0)){
						USART_Send(0x5F, 0);
					}
					if (USART_HasTransmitted(0))
					USART_Flush(0);
				}
				else
				{
					if (USART_IsSendReady(0)){
						USART_Send(firstdig, 0);
					}
					if (USART_HasTransmitted(0))
					USART_Flush(0);
					
					if (USART_IsSendReady(0)){
						USART_Send(seconddig, 0);
					}
					if (USART_HasTransmitted(0))
					USART_Flush(0);
				}
				measure = 1000;
			}
			else if (measure >= 1000)
			{
				if (USART_IsSendReady(0)){
					USART_Send(0x20, 0);
				}
				if (USART_HasTransmitted(0))
				USART_Flush(0);

				measure = 0;
			}
			measure++;

			x = ADC;
			if (x / 20 > 0xFF)
				PORTC = 0xFF;
			break;
	}
}

int main(void)
{/*
	DDRB = 0x00; // Set portB to input
	PORTB = 0xFF;
	DDRA = 0xFF; // Set portA to output
	PORTA = 0x00;
	DDRD = 0xFA; // Set portD to output
	DDRC = 0xFF; // Set portC to output
	PORTC = 0x00;*/
	DDRC = 0xFF; // Set portC as output
	PORTC = 0x00;
	DDRA = 0xFF;
	PORTA = 0x00;
	EnableDistance();
	
	//int cnt = 0;
	PORTC = 0x00;
	
	// set RXD0 as input
	DDRD &= ~(1 << 0);
	PORTD |= (1 << 0);
	// set TXD0 as output
	DDRD |= (1 << 1);
	PORTD &= ~(1 << 1);
	
	// set temp sens as input
	DDRA &= ~(1 << 6);
	PORTA |= (1 << 6);
	
   TimerSet(2);
   TimerOn();

   initUSART(0);
   USART_Flush(0);
   ADC_init();
	
   /* while (1) 
    {
		_delay_us(1000000.0); 
		//TriggerPing();
		PORTC = PingIN();
		//PORTC = 0xFF;
		
    }*/
	
    while (1)
    {
		

	    //PORTB = 0xFF;*/
	   //x = ADC;
	  // unsigned char test = x/11;
		//PORTC = test;
	   //PORTC = x;
	   while (!TimerFlag);
		TimerFlag = 0;
		Tick();
    }
}

