#include "lpc17xx.h"
#include "rit.h"
#include "../button_EXINT/button.h"
#include "../joystick/joystick.h"
#include "../timer/timer.h"
#include "../led/led.h"
#include "../adc_dac/adc.h"
#include "../elevator/elevator.h"

volatile int timer3s;
int enable_timer60s;
int timer60s;

extern int alarm;
extern int controllaTouch;
extern int freqs[];
extern int indici[];
extern int enable_3s_check;
extern int enable_joystick;

void RIT_IRQHandler(void)
{
	Buttons_Debouncing_Handler();		// Buttons debouncing
	
	if (enable_joystick == 1){			//joystick controllato solo se l'ascensore è al piano dell'utente
		enable_RIT();	
		Joystick_HANDLER();						//joystick management
	}
	
	//implementazione di un timer di 3s usando il RIT
	if (enable_3s_check == 1){
		timer3s += 1;
		//60 è il numero di incrementi da fare per contare 3 secondi, tenendo conto del periodo (50ms) del RIT
		if (timer3s >= 60) {  //fine dei 3 secondi
			timer3s = 0;
			pause_timer(TIMER3);
			init_timer(TIMER3, 0x005F5E10, 3, 0); //reinizializzazione per trasporto con blink a 2Hz
			enable_3s_check = 0; //disattivazione timer dei 3s
			enable_timer60s = 1;
			led_off(7);
			}
		}
	
		
	if (enable_timer60s == 1) {
		timer60s++;	
		if (timer60s >= 1200){
			timer60s = 0;
			enable_timer60s = 0;
			if (alarm == 1) { //utente è fermo tra i piani e dopo 60s non ha premuto piu il joytick quindi scatta il caso d'allarme
				led_on(1);
				led_on(3);
				alarm = 0;
				init_timer(TIMER3,0x002FAF08, 3, 0); //blink a 4Hz per emergenza
				led_on(1); //alarm led piano 0 acceso
				led_on(3); //alarm led piano 1 acceso
				//fermo l'ascensore								
				//gestione speaker, all'inizio con nota standard (440 Hz)
				pause_timer(TIMER2);
				reset_timer(TIMER2);
				init_timer(TIMER2, freqs[ indici[0] ], 3, 0);  //produce la nota
				start_timer(TIMER2);
				start_timer(TIMER3);
				
		}
		else { //ascensore è inutilizzato da 60s e non in allarme
			led_off(0); //reserved led piano 0
			led_off(2); //reserved led piano 1
			led_off(7); //status led del controller
			enable_joystick  = 0; 
		}
	}
	}
	else {
		//azzeramento del timer 60s inutilizzo se l'utente effettua qualche azione
		timer60s = 0;
	}

	LPC_RIT->RICTRL |= 0x1;				// clear interrupt flag
	return;
}

void enable_RIT(void)
{
	LPC_RIT->RICTRL |= (1 << 3);
	return;
}

void disable_RIT(void)
{
	LPC_RIT->RICTRL &= ~(1 << 3);
	return;
}

void reset_RIT(void)
{
	LPC_RIT->RICOUNTER = 0; // Set count value to 0
	return;
}

uint32_t rit_init(uint32_t RITInterval)
{
	LPC_SC->PCLKSEL1 &= ~(3 << 26);
	LPC_SC->PCLKSEL1 |= (1 << 26);		// RIT Clock = CCLK
	LPC_SC->PCONP |= (1 << 16);				// Enable power for RIT
	LPC_RIT->RICOMPVAL = RITInterval; // Set match value
	LPC_RIT->RICTRL = (1 << 1) |			// Enable clear on match
										(1 << 2);				// Enable timer for debug
	LPC_RIT->RICOUNTER = 0;						// Set count value to 0
	NVIC_EnableIRQ(RIT_IRQn);
	return (0);
}
