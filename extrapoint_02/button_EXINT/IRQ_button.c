#include "lpc17xx.h"
#include "button.h"
#include "../timer/timer.h"
#include "../led/led.h"
#include "../elevator/elevator.h"
#include "../rit/rit.h"

static uint8_t int0_counter = 0;
static uint8_t key1_counter = 0;
static uint8_t key2_counter = 0;

extern int indici[];
extern int freqs[];
extern int enable_timer60s;
extern int time;

volatile int alarm = 0;
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
			enable_timer60s = 0;
			
			if(int0_counter < 40 && int0_counter >= 2 && alarm == 0){ //ascensore non era in allarme, pressione < 2s -> accidentale 
					//ignoro
			}
			else if(int0_counter == 2 && alarm == 1){ //ascensore era già in allarme, pressione del tasto, l'allarme si spegne
					alarm = 0;
					pause_timer(TIMER2);	//speaker silenzioso
					reset_timer(TIMER2);
					led_off(1); //alarm led piano 0 spento
					led_off(3); //alarm led piano 1 spento

					pause_timer(TIMER3); //blink a 4Hz della modalità emergenza è messo in pausa
					init_timer(TIMER3, 0x005F5E10, 3, 0); //blink standard a 2Hz per trasporto 
			}
			else if (int0_counter >= 40 && alarm == 0){	//ascensore entra in allarme dopo pressione prolungata per 2s
					alarm = 1;
					init_timer(TIMER3,0x002FAF08, 3, 0); //blink a 4Hz per emergenza
								
					led_on(1); //alarm led piano 0 acceso
					led_on(3); //alarm led piano 1 acceso
					
					//fermo l'ascensore
					pause_timer(TIMER0);
								
					//gestione speaker, all'inizio con nota standard (440 Hz)
					pause_timer(TIMER2);
					reset_timer(TIMER2);
					init_timer(TIMER2, freqs[ indici[0] ], 3, 0);  //produce la nota
					start_timer(TIMER2);
					start_timer(TIMER3);
				}
			else if (int0_counter >= 40 && alarm == 1) {
				//mantengo comportamento di allarme
			}
			else if (int0_counter >=2 && alarm == 1) { //ascensore era in allarme, c'è ripressione del tasto INT0 -> l'allarme si spegne
					alarm = 0;
					led_off(1); //alarm led piano 0 spento
					led_off(3); //alarm led piano 1 spento
					start_timer(TIMER0); //riparte simulazione dello spostamento
					pause_timer(TIMER3); //blink a 4Hz della modalità emergenza è messo in pausa
					init_timer(TIMER3, 0x005F5E10, 3, 0); //blink standard a 2Hz per trasporto 
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
			enable_timer60s = 0;
      switch(key1_counter)
      {
				case 2:
					if (alarm == 1) {//gestisco separatamente il caso di emergenza, utente arriva a salvare dal piano 1
						pause_timer(TIMER2);		//speaker silenziato
						led_off(7);
						led_off(1); //alarm led piano 0 spento
						led_off(3); //alarm led piano 1 spento
						pause_timer(TIMER3); //blink a 4Hz della modalità emergenza è messo in pausa
							if (ending_floor == 0){//utente stava andando DOWN ed e richiamato dal piano 1, prende direzione UP
								time = LPC_TIM0->MR0 - LPC_TIM0->TC; //tempo per il ritorno al piano precedente
								LPC_TIM0->TC = time;	
							}
							ending_floor = 1;			//il piano a cui arriverà
							starting_floor = 0;
							start_timer(TIMER0); //timer0 per trasporto nella direzione opposta
							init_timer(TIMER3, 0x005F5E10, 3, 0); //blink standard a 2Hz per trasporto 
							start_timer(TIMER3);
					}
					else { 
							if (isFree(0) == 1) {//controllo se l'ascensore è libero (controllo se i reserved led sono spenti), se 1 ascensore libero
								led_on(0); //accendo reserved led del piano 0 
								led_on(2); //accendo reserved led del piano 1
								starting_floor = getElevatorFloor(); //a che piano è l'ascensore?
									if (starting_floor == 0) { //ascensore è al piano 0, deve andare al piano 1
										ending_floor = 1;
										
										init_timer(TIMER0, 0x0ABA9500, 7, 0);
										start_timer(TIMER0);//timer di 7.2s per trasporto piano 1 -> piano 0
										
										init_timer(TIMER3, 0x005F5E10, 3, 0); 	//timer per blink di 2Hz/5Hz durante trasporto/arrivo al piano
										start_timer(TIMER3);
										
									}
									else if (starting_floor == 1) { //ascensore è già al piano 1
										enable_joystick = 1; //ascensore è gia allo stesso piano dell'utente
										enable_timer60s = 1;//nel RIT conterò 60s passati i quali l'utente deve attivare l'ascensore, premendo tasto select
									}
							}		
						break;
					default: 
						break;
					}
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
		//utente preme tasto al piano 0
    if (readKey2()) //KEY2
    {
      key2_counter++;
			enable_timer60s = 0;
      switch(key2_counter)
      {
        case 2:
					if (alarm == 1) {//gestisco separatamente il caso di emergenza, utente arriva a salvare dal piano 1
						pause_timer(TIMER2);	//speaker sileziato
						led_off(7);
						led_off(1); //alarm led piano 0 spento
						led_off(3); //alarm led piano 1 spento
						pause_timer(TIMER3); //blink a 4Hz della modalità emergenza è messo in pausa
						
						if (ending_floor == 1) {//utente stava andando UP ed è richiamato dal piano 0, prende direzione DOWN
							time = LPC_TIM0->MR0 - LPC_TIM0->TC; //tempo per il ritorno al piano opposto
							LPC_TIM0->TC = time;	
						}
						starting_floor = 1; 
						ending_floor = 0;			//il piano a cui arriverà
						start_timer(TIMER0); //timer0 per trasporto
						init_timer(TIMER3, 0x005F5E10, 3, 0); //blink standard a 2Hz per trasporto 
						start_timer(TIMER3);
					
						}
					else {
							if (isFree(0) == 1) {//controllo se l'ascensore è libero	(i reserved led sono spenti), se 1 ascensore libero
								led_on(0); //accendo reserved led del piano 0 
								led_on(2); //accendo reserved led del piano 1
								starting_floor = getElevatorFloor(); //a che piano è l'ascensore?
								if (starting_floor == 1) { //ascensore è al piano 1, deve andare al piano 0
										ending_floor = 0;
										init_timer(TIMER0, 0x0ABA9500, 7, 0);
										start_timer(TIMER0);//timer di 7.2s per spostamento piano 1 -> piano 0
									
										init_timer(TIMER3, 0x005F5E10, 3, 0);//timer per 2Hz blink durante spostamento	
										start_timer(TIMER3);		
									}
								else if (starting_floor == 0) { //ascensore è già al piano 0			
									enable_joystick = 1; //ascensore è gia allo stesso piano dell'utente
									enable_timer60s = 1;//nel RIT conterò 60s passati i quali l'utente deve attivare l'ascensore, premendo tasto select
									}
							}
					break;
				default: 
					break;
					}
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

