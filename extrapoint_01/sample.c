/*----------------------------------------------------------------------------
 * Name:    sample.c
 * Purpose: to control led through EINT and joystick buttons
 * Note(s):
 *----------------------------------------------------------------------------
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2017 Politecnico di Torino. All rights reserved.
 *----------------------------------------------------------------------------*/
                  
#include <stdio.h>
#include "LPC17xx.H"                    /* LPC17xx definitions                */
#include "led/led.h"
#include "button_EXINT/button.h"
#include "timer/timer.h"
#include "RIT/RIT.h"
#include "joystick/joystick.h"
#include "elevator/elevator.h"

/*----------------------------------------------------------------------------
  Main Program
 *----------------------------------------------------------------------------*/
int main (void) {
  	

	SystemInit();  												/* System Initialization (i.e., PLL)  */
  led_init();                           /* LED Initialization                 */
  button_init();												/* BUTTON Initialization              */
	joystick_init();											/* Joystick Initialization            */
	initElevator();
	rit_init(0x004C4B40);									/* RIT Initialization 50 msec       	*/
	//enable_RIT();												//RIT è attivato solo quando il joystick potrà essere effettivamente usato
	init_timer(TIMER0,0x0ABA9500,7,0);		//timer di 7.2s per trasporto salita<->discesa
	init_timer(TIMER1,0x59682F00,7,0);		//timer di 60s per inutilizzo/allarme
	init_timer(TIMER3,0x005F5E10,3,0); 		//timer per blink di 2Hz/5Hz durante trasporto/arrivo al piano

	/* 
	Valori dei periodi dei timer in esadecimale
	0x0ABA9500 = 7.2s * 25MHz 
	0x59682F00 = 60s * 25MHz
	0x005F5E10 = (1/(2*2)) * 25MHz
	0x002625A0 = (1/(5*2)) * 25MHz 
	*/
	
	LPC_SC->PCON |= 0x1;									/* power-down	mode										*/
	LPC_SC->PCON &= ~(0x2);						
		
  while (1) {                           /* Loop forever                       */	
		__ASM("wfi");
  }

}
