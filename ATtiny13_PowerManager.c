/*
 * ATtiny13_PowerManager.c
 *
 * Created: 2013-01-09
 * Author: Christian
 * Version: 1.2
 */
#define F_CPU 1200000UL

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "ATtiny13_PowerManager.h"

ISR(PCINT0_vect) {	
	// Check if RPI_PIN is low
	if(!(PINB & _BV(RPI_PIN))) {
		if(raspberryPi == shutdown)
			raspberryPi = poweroff;
	} else {
		if(raspberryPi == on)
			raspberryPi = shutdown;
	}
}

ISR(INT0_vect) {
		// Disable external interrupt INT0
		GIMSK &= ~(1 << INT0);
	
		// power button might have been pressed.
		// Enable timer overflow interrupt and perform debouncing to determine if switch really has been pressed.
		TIMSK0 |= (1 << TOIE0);
}

ISR(TIM0_OVF_vect) {
	++timerOverflowCounter;
	
	// Check if PWRSW_PIN is low (0)
	if(!(PINB & _BV(PWRSW_PIN))) {
		if(++powerButtonCounter > PRESSED_DEBOUNCE_SAMPLES) {
			// Signal that power button has been pressed
			powerButton = pressed;
			
			// Disable Timer0 interrupt
			TIMSK0 &= ~(1 << TOIE0);
			
			// Reset counters
			powerButtonCounter = 0;
			timerOverflowCounter = 0;
		}
	} else {
		powerButtonCounter = 0;
	}
	
	if(timerOverflowCounter >= 65000 && powerButton == released) {
		// Disable Timer0 interrupt since power button has not been pressed
		TIMSK0 &= ~(1 << TOIE0);
		
		// Enable external interrupt INT0
		GIMSK |= (1 << INT0);
		
		powerButtonCounter = 0;
		timerOverflowCounter = 0;
	}
}

int main() {
	/* Initialize global variables */
	raspberryPi = off;
	powerButtonCounter = 0;
	timerOverflowCounter = 0;
	powerButton = released;
	
	/* I/O pins */
	// Set outputs
	DDRB |= (1 << PWRLED_PIN);
	DDRB |= (1 << SHUTDOWN_PIN);
	
	// Activate internal pull up
	PORTB |= (1 << PWRSW_PIN);

	/* Timer0 Interrupt */
	// Set timer prescaler to 1/64th the clock rate
	TCCR0B |= (1 << CS01) | (1 << CS00);
	
	/* Pin Change Interrupt */
	// Enable PCINT0
	PCMSK |= (1 << PCINT0);
	GIMSK |= (1 << PCIE);
	
	/* External Interrupt INT0 */
	// Set INT0 to trigger on Falling Edge
	MCUCR |= (1 << ISC01);
	// Enable external interrupt INT0
	GIMSK |= (1 << INT0);
	
	/* Global Interrupt */
	// Enable global interrupt
	SREG |= (1 << SREG_I);
	
	while(1) {
		if(powerButton == pressed) {
			if(raspberryPi == off) {
				power(true);
				waitUntilPowerButtonReleased(false);
			} else {
				// Invoke poweroff by setting SHUTDOWN_PIN high
				PORTB |= (1 << SHUTDOWN_PIN);
				
				if(waitUntilPowerButtonReleased(true)) {
					power(false);
					waitUntilPowerButtonReleased(false);
				}
			}
			// Enable external interrupt INT0
			GIMSK |= (1 << INT0);
		}
		_delay_ms(10);
		
		// Check if RPi power off criteria is fulfilled
		if(raspberryPi == poweroff) {
			_delay_ms(SHUTDOWN_DELAY);
			power(false);
		}
		_delay_ms(10);
	}
	return 0;
}

/*
 * Power RPi on or off
 * @param on TRUE = power on, FALSE = power off
 */
void power(boolean on) {
	if(on) {
		// Turn on power LED
		PORTB |= (1 << PWRLED_PIN);
		
		// Power on RPi
		DDRB |= (1 << MOSFET_PIN);
		raspberryPi = on;
	} else {		
		// Stop sending shutdown signal
		PORTB &= ~(1 << SHUTDOWN_PIN);
		
		// Turn off Power LED
		PORTB &= ~(1 << PWRLED_PIN);
		
		// Power off RPi
		DDRB &= ~(1 << MOSFET_PIN);
		raspberryPi = off;
	}
}

/*
 * Perform debouncing on the power button and wait until it has been released
 * @param timeout Determines if function should check for hard power off or not
 * @return TRUE if timeout is TRUE and hard power off was detected, otherwise FALSE
 */
boolean waitUntilPowerButtonReleased(boolean timeout) {
	int counter = 0, timeoutCounter = 0;
	
	while(counter < RELEASED_DEBOUNCE_SAMPLES) {
		counter = PINB & _BV(PWRSW_PIN) ? counter + 1 : 0;
		if(timeout && ++timeoutCounter > HARD_POWER_OFF_DELAY) {
			return true;
		}
		_delay_ms(10);
	}
	powerButton = released;
	return false;
}