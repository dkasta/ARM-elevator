#include "lpc17xx.h"
#include "joystick.h"
#include "../led/led.h"
#include "../timer/timer.h"
#include <stdio.h>
#define N 10

uint8_t center, down, left, right, up;

volatile int alarm = 0;
volatile int change_direction = 0;
volatile unsigned long time;
volatile int enable_elevator = 0;

extern int same_floor;
extern int ending_floor;
extern int enable_joystick;


void JCenter_HANDLER(void)
{   /* Joystick CENTER button pressed	*/
	
	enable_elevator = 1; //l'utente ha premuto select quindi avvia l'ascensore (-> avvia anche controllo del joystick)
	alarm = 1; //utente è in ascensore e non preme UP/DOWN per 60s ->
	reset_timer(TIMER1);
	start_timer(TIMER1); //avvio timer inutilizzo/allarme
}

void JDown_HANDLER(void)
{   /* Joystick DOWN button pressed	*/
			
	reset_timer(TIMER1); 		//reset timer inutilizzo/allarme
	alarm = 0;
	
	if (ending_floor == 0){ //utente non cambia direzione e ripreme DOWN
		start_timer(TIMER0); 
	}
	else {									//utente cambia direzione premendo UP
		LPC_TIM0->TC = time;
		start_timer(TIMER0); //timer0 per trasporto nella direzione opposta
	}
	start_timer(TIMER3);	 //blink a 2hz per trasporto 
	ending_floor = 0;			 //è il piano a cui arriverà se non cambia direzione	
}

/*
void JLeft_HANDLER(void)
{    Joystick LEFT button pressed	
	//code here
}

void JRight_HANDLER(void)
{    Joystick RIGHT button pressed 
	//code here
}
*/

void JUp_HANDLER(void)
{   /* Joystick UP button pressed */
	
	reset_timer(TIMER1); //reset timer inutilizzo/allarme
	alarm = 0;
	
	if (ending_floor == 1){ //utente non cambia direzione e ripreme UP
			start_timer(TIMER0);		
	}
	else {
		LPC_TIM0->TC = time;	
		start_timer(TIMER0); //timer0 per trasporto nella direzione opposta
	}
	start_timer(TIMER3);	//blink a 2hz per trasporto 
	ending_floor = 1;			//il piano a cui arriverà se non cambia direzione	
}

void Joystick_HANDLER(void)
{ /* Main handler for joystick input	*/
	static int center = 0, down = 0, up = 0; 
	//static int left = 0, right = 0;
	if (enable_joystick == 1) { //controllo tasto select solo se l'ascensore è allo stesso piano dell'utente
		if ((LPC_GPIO1->FIOPIN & (1 << 25)) == 0)
		{ /* Joytick SELECT pressed */
			center++;
			if (center == 1)
				JCenter_HANDLER(); //attivazione del movimento dell'ascensore
		}
		else
		{
			center = 0;
		}
	}

	if (enable_elevator == 1) {	//controllo del movimento solo se l'utente ha premuto il tasto select
		if ((LPC_GPIO1->FIOPIN & (1 << 26)) == 0)
		{ /* Joytick DOWN pressed */
			down++;
			if (down == 1)
				JDown_HANDLER();
		}
		else if (down > 0)
		{ //utente ha smesso di premere DOWN
			down = 0;
			led_on(7); //status led si accende quando l'ascensore è fermo tra i piani
			pause_timer(TIMER0); //timer per trasporto va in pausa
			pause_timer(TIMER3);	//blink 2Hz va in pausa
			time = LPC_TIM0->MR0 - LPC_TIM0->TC; //tempo per il ritorno al piano precedente
			start_timer(TIMER1);		//timer per allarme dopo 60s
			alarm = 1;
		}
		/*
		if ((LPC_GPIO1->FIOPIN & (1 << 27)) == 0)
		{  Joytick LEFT pressed
			left++;
			if (left == 1)
				JLeft_HANDLER();
		}
		else
		{
			left = 0;
		}

		if ((LPC_GPIO1->FIOPIN & (1 << 28)) == 0)
		{  Joytick RIGHT pressed 
			right++;
			if (right == 1)
				JRight_HANDLER();
		}
		else
		{
			right = 0;
		}
		*/

		if ((LPC_GPIO1->FIOPIN & (1 << 29)) == 0)
		{ /* Joytick UP pressed */
			up++;
			if (up == 1){
				JUp_HANDLER();
			}
		}
		else if (up > 0)
		{	//utente ha smesso di premere UP
			up = 0;
			led_on(7); //status led si accende quando l'ascensore è fermo tra i piani
			pause_timer(TIMER0); //timer per trasporto va in pausa
			pause_timer(TIMER3);	//blink 2Hz va in pausa
			time = LPC_TIM0->MR0 - LPC_TIM0->TC; //tempo per il ritorno al piano precedente
			start_timer(TIMER1);		//timer per allarme dopo 60s
			alarm = 1;
		}
	}
}

void joystick_init(void)
{
	//enabling center key
	LPC_PINCON->PINSEL3 &= ~(3 << 18); //PIN mode GPIO (00b value per P1.25)
	LPC_GPIO1->FIODIR &= ~(1 << 25);   //P1.25 Input (joysticks on PORT1 defined as Input)

	//enabling down key
	LPC_PINCON->PINSEL3 &= ~(3 << 20); //PIN mode GPIO (00b value per P1.25)
	LPC_GPIO1->FIODIR &= ~(1 << 26);   //P1.25 Input (joysticks on PORT1 defined as Input)

	//enabling left key
/*
	LPC_PINCON->PINSEL3 &= ~(3 << 22); //PIN mode GPIO (00b value per P1.25)
	LPC_GPIO1->FIODIR &= ~(1 << 27);   //P1.25 Input (joysticks on PORT1 defined as Input)
*/
	//enabling right
/*
	LPC_PINCON->PINSEL3 &= ~(3 << 24); //PIN mode GPIO (00b value per P1.25)
	LPC_GPIO1->FIODIR &= ~(1 << 28);   //P1.25 Input (joysticks on PORT1 defined as Input)
*/
	//enabling up key
	LPC_PINCON->PINSEL3 &= ~(3 << 26); //PIN mode GPIO (00b value per P1.25)
	LPC_GPIO1->FIODIR &= ~(1 << 29);   //P1.25 Input (joysticks on PORT1 defined as Input)
}
