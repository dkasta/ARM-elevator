#include "lpc17xx.h"
#include "button.h"
#include "../timer/timer.h"
#include "../led/led.h"
#include "../elevator/elevator.h"
#include "../rit/rit.h"



static uint8_t int0_counter = 0;
static uint8_t key1_counter = 0;
static uint8_t key2_counter = 0;

volatile uint8_t e1=0;
volatile uint8_t e2=0;

static int test;

volatile int starting_floor = 0;
volatile int ending_floor = 0;

volatile int enable_joystick = 0;


void EINT0_IRQHandler(void) // INT0
{
	enable_RIT();	
  NVIC_DisableIRQ(EINT0_IRQn);
  LPC_PINCON->PINSEL4 &= ~(1 << 20);
  int0_counter = 1;
  LPC_SC->EXTINT &= (1 << 0); /* clear pending interrupt         */
}

void EINT1_IRQHandler(void) // KEY1
{
	enable_RIT();	
  NVIC_DisableIRQ(EINT1_IRQn);
  LPC_PINCON->PINSEL4 &= ~(1 << 22);
  key1_counter = 1;
  LPC_SC->EXTINT &= (1 << 1); /* clear pending interrupt         */
}

void EINT2_IRQHandler(void) // KEY2														 */
{
	enable_RIT();	
  NVIC_DisableIRQ(EINT2_IRQn);
  LPC_PINCON->PINSEL4 &= ~(1 << 24);
  key2_counter = 1;
  LPC_SC->EXTINT &= (1 << 2); /* clear pending interrupt         */
}

void Buttons_Debouncing_Handler()
{
  if (int0_counter != 0) //debounce Int0
  {                 
    if (readInt0()) //INT0
    {
      int0_counter++;
			switch(int0_counter){
				case 2: //code here
					break;
				default: 
					break;
      }
    }
    else
    {
      NVIC_EnableIRQ(EINT0_IRQn);							 /* enable Button interrupts			*/
			LPC_PINCON->PINSEL4    |= (1 << 20);     /* External interrupt 1 pin selection */
      int0_counter = 0;
    }
  }
  if (key1_counter != 0)//debounce KEY1
  {
		//utente preme tasto al piano 1, non sapendo dov'è l'ascensore
    if (readKey1()) //KEY1
    {
      key1_counter++;
      switch(key1_counter)
      {
				case 2:
					test = isFree(0); //controllo se l'ascensore è libero	(controllo se i reserved led sono spenti)	
						if (test == 1) {//ascensore libero
							led_on(0); //accendo reserved led del piano 0 
							led_on(2); //accendo reserved led del piano 1
							starting_floor = getElevatorFloor(); //a che piano è l'ascensore?
								if (starting_floor == 0) { //ascensore è al piano 0, deve andare al piano 1
									ending_floor = 1;
									start_timer(TIMER0);//timer di 7.2s per trasporto piano 1 -> piano 0
									start_timer(TIMER3);//timer per 2Hz blink durante trasporto			
								}
								else if (starting_floor == 1) { //ascensore è già al piano 1
									enable_joystick = 1; //ascensore è gia allo stesso piano dell'utente
									reset_timer(TIMER1);
									start_timer(TIMER1);//timer di 60s entro i quali l'utente deve attivare l'ascensore, premendo tasto select
								}
						}		
					break;
				default: 
					break;
      }
    }
    else
    {
      NVIC_EnableIRQ(EINT1_IRQn);							 /* enable Button interrupts			*/
			LPC_PINCON->PINSEL4    |= (1 << 22);     /* External interrupt 1 pin selection */
      key1_counter = 0;
    }
  }
  if (key2_counter != 0)//debounce KEY2
  {
		//utente preme tasto al piano 0, non sapendo dov'è l'ascensore
    if (readKey2()) //KEY2
    {
      key2_counter++;
      switch(key2_counter)
      {
        case 2:
					test = isFree(0); //controllo se l'ascensore è libero	(i reserved led sono spenti)				
						if (test == 1) {//ascensore libero
							led_on(0); //accendo reserved led del piano 0 
							led_on(2); //accendo reserved led del piano 1
							starting_floor = getElevatorFloor(); //a che piano è l'ascensore?
							if (starting_floor == 1) { //ascensore è al piano 1, deve andare al piano 0
									ending_floor = 0;
									start_timer(TIMER0);//timer di 7.2s per spostamento piano 1 -> piano 0
									start_timer(TIMER3);//timer per 2Hz blink durante spostamento			
								}
								else if (starting_floor == 0) { //ascensore è già al piano 0			
									enable_joystick = 1; //ascensore è gia allo stesso piano dell'utente
									reset_timer(TIMER1);
									start_timer(TIMER1);//timer di 60s entro i quali l'utente deve attivare l'ascensore, premendo tasto select
								}
						}
					break;
				default: break;
      }
    }
    else
    {
      NVIC_EnableIRQ(EINT2_IRQn);							 /* enable Button interrupts			*/
			LPC_PINCON->PINSEL4    |= (1 << 24);     /* External interrupt 2 pin selection */
      key2_counter = 0;
    }
  }
}

uint8_t readInt0()
{
  return !(LPC_GPIO2->FIOPIN & (1 << 10));
}

uint8_t readKey1()
{
  return !(LPC_GPIO2->FIOPIN & (1 << 11));
}

uint8_t readKey2()
{
  return !(LPC_GPIO2->FIOPIN & (1 << 12));
}

void button_init(void)
{

  LPC_PINCON->PINSEL4 |= (1 << 20); /* External interrupt 0 pin selection */
  LPC_GPIO2->FIODIR &= ~(1 << 10);  /* PORT2.10 defined as input          */

  LPC_PINCON->PINSEL4 |= (1 << 22); /* External interrupt 0 pin selection */
  LPC_GPIO2->FIODIR &= ~(1 << 11);  /* PORT2.11 defined as input          */

  LPC_PINCON->PINSEL4 |= (1 << 24); /* External interrupt 0 pin selection */
  LPC_GPIO2->FIODIR &= ~(1 << 12);  /* PORT2.12 defined as input          */

  LPC_SC->EXTMODE = 0x7;

  NVIC_EnableIRQ(EINT2_IRQn);      /* enable irq in nvic                 */
  NVIC_SetPriority(EINT2_IRQn, 1); /* priority, the lower the better     */
  NVIC_EnableIRQ(EINT1_IRQn);      /* enable irq in nvic                 */
  NVIC_SetPriority(EINT1_IRQn, 2);
  NVIC_EnableIRQ(EINT0_IRQn);      /* enable irq in nvic                 */
  NVIC_SetPriority(EINT0_IRQn, 3); /* decreasing priority	from EINT2->0	 */
}
