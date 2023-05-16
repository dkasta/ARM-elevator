#include "lpc17xx.h"
#include "rit.h"
#include "../button_EXINT/button.h"
#include "../joystick/joystick.h"
#include "../timer/timer.h"
#include "../led/led.h"

volatile int count;
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
		count += 1;
		//60 è il numero di incrementi da fare per contare 3 secondi, tenendo conto del periodo (50ms) del RIT
		if (count >= 60) {  //fine dei 3 secondi
			count = 0;
			led_on(7);				//trascorsi i 3 secondi lo status led si spegne
			pause_timer(TIMER3);
			init_timer(TIMER3, 0x005F5E10, 3, 0); //reinizializzazione per trasporto con blink a 2Hz
			enable_3s_check = 0; //disattivazione timer dei 3s
			reset_timer(TIMER1);
			start_timer(TIMER1); //timer di 60s per inutilizzo
		}
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
