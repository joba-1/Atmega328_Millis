/*
 * Atmega328P_Millis.c
 *
 * Created: 02.04.2020 09:45:06
 * Author : Joachim Banzhaf
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED PB5

volatile static uint32_t _millis = 0;

ISR(TIMER0_COMPA_vect) {
	_millis++;
}

void init_millis() {
	// Configure timer 0 as fast PWM stopping at OCR0A (CTC should work as well)
	TCCR0A |= (1<<WGM00) | (1<<WGM01);

	// Select highest prescaler where 256 steps take more than 1ms for given F_CPU:
	// 1/F_CPU * 2^8 * Prescaler >= 1/1000 s => Prescaler >= F_CPU / 2^8 / 1000
	//        fast PWM     Prescaler = 64 for F_CPU = 16000000
	TCCR0B |= (1<<WGM02) | (1<<CS00) | (1<<CS01);

	// Select ticks after 1ms has passed:
	// 1/F_CPU * ticks * Prescaler = 1/1000s => ticks = F_CPU / Prescaler / 1000
	OCR0A = F_CPU / 64 / 1000 - 1; // -1 because it starts at zero and step from limit to zero counts as well

	// Enable interrupt routine ISR(TIMER0_COMPA_vect)
	TIMSK0 |= (1<<OCIE0A);
	
	// Enable interrupts
	sei();
}

uint32_t millis() {
	cli(); // don't allow ISR to change 32bit _millis while we read it byte by byte
	uint32_t rc = _millis;
	sei();
	return rc;
}

void delay( uint32_t ms ) {
	uint32_t start = millis();
	while( millis() - start < ms );
}


// Structure holding all parameters necessary for a task
typedef struct payload {
	int pin; // pin to toggle
} payload_t;

// Function type of a task to execute with payload parameter
typedef void (*task_t)( payload_t *payload );

// Job defines task to execute with payload parameter at regular intervals
typedef struct job {
	task_t task;
	payload_t *payload;
	uint32_t ms;
	uint32_t start;
} job_t;

// Init job by storing start time
void job_init( job_t *job ) {
	job->start = millis();
}

// Execute task if delay has elapsed
void job_do( job_t *job ) {
	if( millis() - job->start >= job->ms ) {
		job->start += job->ms;
		(*job->task)(job->payload);
	}
}


// Task to do at regular intervals
void toggle( payload_t *payload ) {
	PORTB ^= (1 << payload->pin); // toggle LED pin with XOR
}

int main(void) {
	init_millis();
	
	DDRB |= (1 << LED);  // LED pin as output

	static payload_t p = { LED };
	static job_t job1 = { toggle, &p, 28 };
	static job_t job2 = { toggle, &p, 27 }; // nice breathing effect

	while (1) {
		// You could do multiple jobs like this for different tasks with different intervals...
		job_do(&job1);
		job_do(&job2);
		
		// do something useful here...
	}
}
