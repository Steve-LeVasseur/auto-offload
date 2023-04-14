/*
 * GradDesign2.cpp
 *
 * Created: 4/11/2023 3:20:37 PM
 * Author : 13305
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>

volatile uint16_t actuator_count = 0;
volatile uint16_t actuator_thresh = 10;
volatile uint8_t motor_thresh = 6;


int main(void)
{
	// Timer 1 config - Timing Motor and Actuator Motion
		//******************************************************************************
	//Configure Timer 1 to generate an ISR every 100ms
	//******************************************************************************
	//No Pin Toggles, CTC Mode
	TCCR1A = (0<<COM1A1)|(0<<COM1A0)|(0<<COM1B1)|(0<<COM1B0)|(0<<WGM11)|(0<<WGM01);
	//CTC Mode, N=64
	TCCR1B = (0<<ICNC1)|(0<<ICES1)|(0<<WGM13)|(1<<WGM12)|(1<<CS12)|(0<<CS11)|(0<<CS10);
	//Enable Channel A Match Interrupt
	TIMSK1 = (0<<ICIE1)|(0<<OCIE1B)|(1<<OCIE1A)|(0<<TOIE1);
	//Setup output compare for channel A
	OCR1A = 62500; //OCR1A = 16Mhz/N*Deltat-1 = 16Mhz/64*.1-1
	
	
	
	
	// Timer 0 config - Outputs PWM signals to bridge
	//**************************************************************************************
	//Configure Timer 0
	//**************************************************************************************
	TCCR0A = (1<<COM0A1)|(0<<COM0A0)|(0<<COM0B1)|(0<<COM0B0)|(1<<WGM01)|(1<<WGM00); //Setup Fast PWM Mode for Channel A
	TCCR0B = (0<<WGM02)|(1<<CS02)|(0<<CS01)|(1<<CS00); //Define Clock Source Prescaler
	OCR0A = 0; //Initialized PWM Duty Cycle to 0 for no motion
	
	
	
	
	
	
	
	// All of PORTD to outputs (BIN1, BIN2, AIN1, AIN2, empty, PWMA, PWMB, Standby)
	DDRD = (1<<PORTD0)|(1<<PORTD1)|(1<<PORTD2)|(1<<PORTD3)|(1<<PORTD4)|(1<<PORTD5)|(1<<PORTD6)|(1<<PORTD7);
	// C5 to input, so it can start the whole process
	DDRC = (1<<PORTC0)|(1<<PORTC1)|(1<<PORTC2)|(1<<PORTC3)|(1<<PORTC4)|(0<<PINC5);
	// DDRC = (1<<PORTC0)|(1<<PORTC1)|(0<<PORTC2)|(0<<PORTC3)|(1<<PORTC4)|(0<<PINC5);
	DDRB = (1<<PORTB0);
	PORTC &= !(1<<PORTC4) & !(1<<PORTC3) & !(1<<PORTC2);
	
	PORTD = 0b00000000; // Sets standby low, so nothing can move
	PORTB = (1<<PORTB0);
    /* Replace with your application code */
	
	sei();
	
    while (1) 
    {
		PORTD ^= (1<<PORTD4);
    }
}

//Interrupt Routine for Timer 1 Match
ISR (TIMER1_COMPA_vect)
{
	if ((PINC&(1<<PINC5)) && !(PORTC&(1<<PORTC4))) // signal given to unload & finished signal not pulled high
	// if (PINC&(1<<PINC5) )
	{
		actuator_count++;
		if (actuator_count > actuator_thresh) // unloading portion has finished
		{
			PORTC |= (1<<PORTC2);
			if (actuator_count > actuator_thresh+motor_thresh) // door has closed after unloading
			{
				if (actuator_count > 2*actuator_thresh) // full process is done ENDS HERE
				{
					PORTC &= ~(1<<PORTC2);
					PORTC &= ~(1<<PORTC3);
					OCR0A = 0;
					PORTD = (0<<PORTD0)|(0<<PORTD1)|(0<<PORTD2)|(0<<PORTD3)|(0<<PORTD5)|(0<<PORTD7);
					PORTB = (0<<PORTB0);
					actuator_count = 0;
					PORTC |= (1<<PORTC4);
				}
				else // door has closed but actuator is still returning
				{
					PORTC &= ~(1<<PORTC3);
					OCR0A = 0;
					PORTB = (1<<PORTB0);
					PORTD = (0<<PORTD0)|(0<<PORTD1)|(0<<PORTD2)|(1<<PORTD3)|(1<<PORTD5)|(1<<PORTD7);
					
				}
			}
			else // door still closing after unloading
			{
				PORTC |= (1<<PORTC3);
				OCR0A = 40000;
				PORTB = (0<<PORTB0);
				PORTD = (0<<PORTD0)|(1<<PORTD1)|(0<<PORTD2)|(0<<PORTD3)|(0<<PORTD5)|(1<<PORTD7);
				
			}
		}
		else // still unloading
		{
			PORTC &= ~(1<<PORTC2);
			if (actuator_count > motor_thresh) // door finished opening, still unloading
			{
				PORTC &= ~(1<<PORTC3);
				OCR0A = 0;
				PORTB = (1<<PORTB0);
				PORTD = (0<<PORTD0)|(0<<PORTD1)|(1<<PORTD2)|(0<<PORTD3)|(1<<PORTD5)|(1<<PORTD7);
				
			}
			else // door has not finished opening, still unloading STARTS HERE
			{
				PORTC |= (1<<PORTC3);
				OCR0A = 40000;
				PORTB = (0<<PORTB0);
				PORTD = (1<<PORTD0)|(0<<PORTD1)|(0<<PORTD2)|(0<<PORTD3)|(0<<PORTD5)|(1<<PORTD7);
				
			}
		}
	}
	// if start signal stops while end signal is high, lower end signal
	else if ((!(PINC&(1<<PINC5))) && (PORTC&(1<<PORTC4))) {PORTC &= ~(1<<PORTC4);}
	else if (PORTC&(1<<PORTC4)){OCR0A = 0;}
	else {
		PORTC |= (1<<PORTC2);
		PORTC &= ~(1<<PORTC3);
		OCR0A = 0;
		PORTD = (0<<PORTD0)|(0<<PORTD1)|(0<<PORTD2)|(0<<PORTD3)|(0<<PORTD5)|(0<<PORTD7);
		
	}
}
