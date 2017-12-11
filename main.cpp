/*
 * SkeinTwister.cpp
 *
 * Created: 12/8/2017 11:49:53 AM
 * Author : Sundra
 */ 

#define F_CPU 1000000ul

#include <avr/io.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/interrupt.h>
#include <avr/power.h>  //unused?
#include "M328P_Serial.h"

#define KNOB	PINC0
#define LED		PORTD6
#define FET		PORTB1
#define PEDAL	PORTD2

#define TOGGLE_LED	(PORTD ^= (1 << LED))
#define START_PWM	5
#define STOP_PWM	100
#define PWM_INCT	5
#define PWM_DELAY	20
#define TIMEOUT 	30000
#define	DEBOUNCE	75
#define MOTOROFF	TCCR1B &= ~(1 << CS10)
#define MOTORON		TCCR1B |= (1 << CS10)
#define PEDALISPRESSED	bit_is_clear(PIND, PEDAL)
#define PEDALISRELEASED	bit_is_set(PIND, PEDAL)
#define OFF 0
#define ON  1
#define PRESSHOLD	0
#define AUTOMATIC	1
#define TAPONOFF	2

static inline void initOutputs (void) {
	// sets the following to outputs
	DDRD |= (1 << LED);
	DDRB |= (1 << FET);			
}

static inline void initFootPedal (void) {
	EICRA |= (1 << ISC01);			// INT0 triggers on falling edge
	EIMSK |= (1 << INT0);			// enables INT0
};

static inline void initADC (void) {
	DIDR0 |= (1 << KNOB);					// disables digital input filter on KNOB
	ADMUX = 0;								// sets reference to AREF (default), KNOB input muxed, right-adjusted result (default)
	ADCSRA = (1 << ADPS1) | (1 << ADPS0);	// pre-scalar of clock/8.  (ADC clock between 50 and 200 kHz for 10 bit res, for F_CPU = 1Mhz, pre-scalar of 5 to 20)
	PRR &= ~(1 << PRADC);					// clears power reduction ADC register
	ADCSRA |= (1 << ADEN);					// enables ADC
	ADCSRA |= (1 << ADSC);					// performs one conversion, throw away to start
	while (ADCSRA & (1 << ADSC)); 			// waits for conversion to be done
}

static inline void initMotorPWM (void) {
	TCCR1A |= (1 << COM1A1) | (1 << WGM10); 	// clears OC1A on match when counting up, sets on down, PWM phase correct ICR1 is TOP
	TCCR1B |= (1 << WGM13);						// PWM phase correct ICR1 is TOP
	ATOMIC_BLOCK(ATOMIC_FORCEON){				// atomic operation for 16-bit register writes
		OCR1A = 0;								// should start as solid off
		ICR1 = STOP_PWM;						// sets TOP to 100, freq = 10 kHz = F_CPU/(2*prescaler*TOP)
	}
	TCCR1B |= (1 << CS10);						// enables the clock with a prescaler of 1
}

static inline void initMillisTimer (void) {
	// 8-bit Timer0 used for Arduino millis, may as well keep compatible
	TCCR0A |= (1 << WGM01);					// sets CTC mode
	OCR0A = 125;							// 125 = 1 ms @ F_CPU = 1MHz
	TIMSK0 |= (1 << OCIE0A);				// enables interrupt
	TCCR0B |= (1 << CS01);					// starts timer with prescalar: F_CPU/8 (125,000), no output waveforms
}

volatile uint32_t milliseconds = 0;
uint32_t millis(void) {
	uint32_t mscopy;
	ATOMIC_BLOCK(ATOMIC_FORCEON) {
		mscopy = milliseconds;
	}
	return mscopy;
}

uint16_t readKnob() {
	uint16_t result = 0;
	ADCSRA |= (1 << ADSC);		// starts conversion
	while (ADCSRA & (1 << ADSC)); // waits for conversion to be done
	result = ADCL;
	result |= (ADCH << 8);
	return result;
}

uint8_t motorOn = OFF;
void motor(uint8_t state) {
	if (!state) {
		MOTOROFF;
		motorOn = OFF;
	}

	else {
		motorOn = ON;
		OCR1A = 0;
		MOTORON;
	}
}

volatile uint8_t pedalPress = 0;
volatile uint32_t pedalTime = 0;
uint8_t checkPedal() {
	EIMSK &= ~(1 << INT0);					// disables INT0 to guarantee clearing pedalPress
	if (pedalPress && (millis() - pedalTime > DEBOUNCE)) {
		pedalPress = 0;
		EIMSK |= (1 << INT0);				// enables INT0
		if (PEDALISPRESSED) return 1;
		else return 0;
	}
	EIMSK |= (1 << INT0);					// enables INT0
	return 0;
}

void softstart(uint8_t runmode)  {
	motor(ON);

	// blips motor to indicate that it won't automatically stop
	if (runmode == TAPONOFF) {
		OCR1A = START_PWM;
		_delay_ms(50);
		OCR1A = 0;
		_delay_ms(100);
	}

	// ramps up speed
	uint8_t i;
	for (i = START_PWM; i < STOP_PWM; i+=PWM_INCT) {
		OCR1A = i;
		_delay_ms(PWM_DELAY);

		// checks for commanded stops
		if (runmode == PRESSHOLD)	{
			if (PEDALISRELEASED) {		// turns off if pedal is released
				motor(OFF);
				break;
			}
		}
		if (runmode == AUTOMATIC || runmode == TAPONOFF) {
			if (checkPedal()) {			// turns off if pedal tapped again
				motor(OFF);
				break;
			}
		}
	}
	// exits loop either at full speed or off
}

// interrupts are automatically cleared upon execution of the ISR

ISR(INT0_vect) {
	pedalTime = millis();
	pedalPress = 1;
}

ISR(TIMER0_COMPA_vect) {
	milliseconds++;
}

/*
Twister operates differently according to mode set via knob.
----------------------------------------------------------------
PRESSHOLD - the motor only spins while the foot pedal is depressed.
TAPONOFF  - one tap starts the motor, one tap stops the motor.
AUTOMATIC - twists for a set length of time (via knob).  
			One tap starts the motor.  
			It automatically stops after the elapsed time.  
			When on, another tap will stop the motor.
*/

int main(void)
{	
	initMillisTimer();
	initOutputs();
	initFootPedal();
	initMotorPWM();
	initADC();
	SerialInit(9600);
	sei();
	
	uint32_t ledBlinkTime = 0;
	
	// Knob and Foot Pedal variables
	uint8_t mode = AUTOMATIC;
	uint16_t knobsetting;
	uint8_t footpedal = 0;
	uint32_t setTime = 0;
	uint32_t startTime = 0;
	uint32_t elapsedTime = 0;

	// Constants for scaling & dealing with knob settings
	const uint16_t mode0setting = 5;		// 3 detents
	const uint16_t mode2setting = 1018;		// 3 detents
	const uint16_t minimumTime = 1500;		// min knob set time in milliseconds
	const uint16_t maximumTime = 11500;		// max knob set time in milliseconds
	const uint16_t rangeTime = maximumTime - minimumTime;
	const uint16_t knoboffset = mode0setting;		// b, in equation y = mx + b where y is knobadj and x is knobsetting
	const uint16_t knobslope = rangeTime / (mode2setting - mode0setting);								// m, in above
	
	uint8_t a = 0;
	
    while (1) 
    {
			
// 		// Reads pot and determines mode
// 		knobsetting = readKnob();
// 		if (knobsetting < mode0setting) mode = PRESSHOLD;
// 		else if (knobsetting > mode2setting) mode = TAPONOFF;
// 		else {
// 			mode = AUTOMATIC;
// 			setTime = (knobsetting - knoboffset) * knobslope + minimumTime;			// scales trim pot setting to milliseconds
// 		}
// 
// 		// Gets the footpedal status and records the time if pressed
// 		footpedal = checkPedal();
// 		if (footpedal) startTime = millis();
// 
// 		// State Machine
// 		switch (mode) {
// 			case PRESSHOLD:
// 				if (footpedal) softstart(mode);				// Turns ON if pedal just pressed
// 				else if (PEDALISRELEASED) motor(OFF);		// Turns OFF is pedal is released
// 				// Otherwise motor stays in last mode (full ON or OFF)
// 				break;
// 
// 			case AUTOMATIC:
// 				// Changes state when foot pedal is pressed
// 				if (footpedal) {
// 					if (motorOn) motor(OFF);	// Turns motor OFF if ON
// 					else softstart(mode);		// Turns motor ON if OFF
// 				}
// 				// Turns motor OFF if enough time has elapsed
// 				elapsedTime = millis() - startTime;
// 				if (elapsedTime > setTime || elapsedTime > TIMEOUT) motor(OFF);			
// 				// Otherwise motor stays in last mode (full ON or OFF)
// 				break;
// 
// 			case TAPONOFF:
// 				// Changes state when foot pedal is pressed
// 				if (footpedal) {
// 					if (motorOn) motor(OFF);	// Turns motor OFF if ON
// 					else softstart(mode);		// Turns motor ON if OFF
// 				}
// 				// Turns motor OFF if the cat pushed the foot pedal
// 				if (millis() - startTime > TIMEOUT) motor(OFF);
// 				// Otherwise motor stays in last mode (full ON or OFF)
// 				break;
// 		}

 		knobsetting = readKnob();
// 		ATOMIC_BLOCK(ATOMIC_FORCEON) {
// 			ICR1 = 0xFFFF;
// 			OCR1A = knobsetting;
// 		}
//		SerialPrint(knobsetting);
		
// 		SerialTX(a);
/*		a++;*/

		SerialPrint(knobsetting);
		SerialPrint("\n");
		_delay_ms(100);
	
		
		
	//	if(checkPedal()) TOGGLE_LED;
		
		// Blinks LED
		if (millis() - ledBlinkTime > 500)	{
			TOGGLE_LED;
			ledBlinkTime = millis();
		}
		
    } // end of while(1)
} // end of main()

