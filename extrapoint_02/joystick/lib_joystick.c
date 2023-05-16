#include "lpc17xx.h"
#include "joystick.h"
#include "../led/led.h"
#include "../timer/timer.h"
#include <stdio.h>
#define N 10
#include "../GLCD/GLCD.h"
#include "../TouchPanel/TouchPanel.h"

uint8_t center, down, left, right, up;

extern int controllaTouch;
extern int alarm;
volatile int change_direction = 0;
volatile unsigned long time;
volatile int enable_elevator = 0;

extern int same_floor;
extern int ending_floor;
extern int starting_floor;

extern int enable_joystick;

extern int timer60s;
extern int enable_timer60s;

extern int stato;


void JCenter_HANDLER(void)
{   /* Joystick CENTER button pressed	*/
	
	enable_elevator = 1; //l'utente ha premuto select quindi avvia l'ascensore (-> avvia anche controllo del joystick)
	alarm = 1; //utente è in ascensore e non preme UP/DOWN per 60s -> scatenerà caso allarme
	enable_timer60s = 0;
	
}

void JDown_HANDLER(void)
{   /* Joystick DOWN button pressed	*/
			
	enable_timer60s = 0; 		//reset timer inutilizzo/allarme
	alarm = 0;
	
	if (ending_floor == 0){ //utente non cambia direzione e ripreme DOWN
		start_timer(TIMER0); 
	}
	else {									//utente stava andando UP e poi preme DOWN
		LPC_TIM0->TC = time;
		start_timer(TIMER0); //timer0 per trasporto nella direzione opposta
		ending_floor = 0;
		starting_floor = 1;
	}
	start_timer(TIMER3);	 //blink a 2hz per trasporto 

}

/*
void JLeft_HANDLER(void)
{    Joystick LEFT button pressed	
	//code here
}
*/

void JRight_HANDLER(void)
{    //Joystick RIGHT button pressed

	LCD_Initialization();
	TP_Init();
	TouchPanel_Calibrate();
	LCD_Clear(Black);
	controllaTouch = 1;
	init_timer(TIMER1, 0x4E2, 3, 0);		//timer per touch
	start_timer(TIMER1);
	stato = 'O';
	
}


void JUp_HANDLER(void)
{   /* Joystick UP button pressed */
	
	enable_timer60s = 0; //reset timer inutilizzo/allarme
	alarm = 0;
	
	if (ending_floor == 1){ //utente non cambia direzione e ripreme UP
			start_timer(TIMER0);		
	}
	else {									//utente stava andando DOWN e poi preme UP
		LPC_TIM0->TC = time;	
		start_timer(TIMER0); //timer0 per trasporto nella direzione opposta
		ending_floor = 1;
		starting_floor = 0;
		
	}
	start_timer(TIMER3);	//blink a 2hz per trasporto 

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
			
			pause_timer(TIMER0); //timer per trasporto va in pausa
			pause_timer(TIMER3);	//blink 2Hz va in pausa
			time = LPC_TIM0->MR0 - LPC_TIM0->TC; //tempo per il ritorno al piano precedente
			if(time == 0)
				led_off(7); //status led si accende quando l'ascensore è fermo tra i piani
			else led_on(7);
			
			enable_timer60s = 1;		//timer per allarme dopo 60s
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
*/
		if ((LPC_GPIO1->FIOPIN & (1 << 28)) == 0)
		{  //Joytick RIGHT pressed 
			right++;
			if (right >= 40)
				JRight_HANDLER();
		}
		else
		{
			right = 0;
		}
		
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
		
			pause_timer(TIMER0); //timer per trasporto va in pausa
			pause_timer(TIMER3);	//blink 2Hz va in pausa
			time = LPC_TIM0->MR0 - LPC_TIM0->TC; //tempo per il ritorno al piano precedente
			
			if(time == 0)
				led_off(7); //status led si accende quando l'ascensore è fermo tra i piani
			else led_on(7);
				
			enable_timer60s = 1;		//timer per allarme dopo 60s
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

	LPC_PINCON->PINSEL3 &= ~(3 << 24); //PIN mode GPIO (00b value per P1.25)
	LPC_GPIO1->FIODIR &= ~(1 << 28);   //P1.25 Input (joysticks on PORT1 defined as Input)

	//enabling up key
	LPC_PINCON->PINSEL3 &= ~(3 << 26); //PIN mode GPIO (00b value per P1.25)
	LPC_GPIO1->FIODIR &= ~(1 << 29);   //P1.25 Input (joysticks on PORT1 defined as Input)
}
