#include "stdio.h"
#include "lpc17xx.h"
#include "timer.h"
#include "../common.h"
#include "../led/led.h"
#include "../elevator/elevator.h"
#include "../rit/rit.h"
#include "../adc_dac/adc.h"
#include "../GLCD/GLCD.h" 
#include "../TouchPanel/TouchPanel.h"


uint16_t SinTable[45] = //80% volume
{
    410, 467, 523, 576, 627, 673, 714, 749, 778,
    799, 813, 819, 817, 807, 789, 764, 732, 694, 
    650, 602, 550, 495, 438, 381, 324, 270, 217,
    169, 125, 87 , 55 , 30 , 12 , 2  , 0  , 6  ,   
    20 , 41 , 70 , 105, 146, 193, 243, 297, 353
};

uint16_t SinTable30[45] = //30% volume
{
    154, 175, 196, 216, 235, 252, 268, 281, 292,
    300, 305, 307, 306, 303, 296, 287, 275, 260, 
		244, 226, 206, 186, 164, 143, 122, 101,  81, 
		 63, 	47,  33,  21,  11,   5,   1,   0,   2, 
		  8,  15,  26,  39,  55,  72,  91,  111, 132
};


static uint8_t _t0state=ON;
static uint8_t _t1state=ON;
static uint8_t _t2state=OFF;
static uint8_t _t3state=OFF;

volatile int enable_3s_check = 0;

extern int ending_floor;
extern int starting_floor;
extern int alarm;
extern int enable_joystick;
extern int emergency_mode;

volatile static int i;
extern int controllaTouch;


char stato = 'O';

/*int indice1 = 5;
int indice2 = 5;
*/

int indici[2] ={5,5};

int notaSel = 0; // 0 1 2

int note[8] = {262, 294, 330, 349, 392, 440, 494, 523};
char noteNome[8] = {'C', 'D', 'E', 'F', 'G', 'A', 'B', 'C'};

const int freqs[8]={2120,1890,1684,1592,1417,1263,1125,1062}; 

char s[100];

void TIMER0_IRQHandler(void)
{
	//una volta scaduto il timer per il trasporto,
	//l'ascensore ha raggiunto il piano desiderato. 
	//i reserved leds e il blink a 2Hz si spengon0    
		
		led_off(0);//reserved led piano 0
		led_off(2);//reserved led piano 1
		led_off(7);
	
	if (ending_floor == 1){
		setElevatorFloor(1);//ora l'ascensore è al piano 1
		starting_floor = 1;
		ending_floor = 0;
	}
	else {
		setElevatorFloor(0);//ora l'ascensore è al piano 0
		starting_floor = 0;
		ending_floor = 1;
	}
	
	enable_joystick = 1; //comincio a controllare il joystick
	alarm = 0; //una volta arrivato al piano da cui il soccorso ha chiamato l'ascensore, la modalità emergenza è disattivata
	
	pause_timer(TIMER3); //metto in pausa il blink a 2 Hz (l'ascensore è al piano)
  
	//gestione del timer di 3s con una variabile + RIT
	init_timer(TIMER3, 0x002625A0, 3, 0);		//blink a 5Hz
	start_timer(TIMER3);
	
	enable_3s_check = 1; //attiva controllo nel RIT
	reset_RIT();
	enable_RIT();
 	
	LPC_TIM0->IR = 1; /* clear interrupt flag */
	
	return;
}

void TIMER1_IRQHandler(void) //controllo touch
{
	static int c =  0;

	if (controllaTouch == 1){
				
		getDisplayPoint(&display, Read_Ads7846(), &matrix );
	
		ADC_start_conversion();

    switch(stato) {
        case 'O':
            GUI_Text(0, 160, (uint8_t *) "^^Tieni premuto qui sopra^^" , Green, Black);
				    GUI_Text(0, 176, (uint8_t *) "^per entrare  in manutenzione^" , Green, Black);

						       
            if(display.x <= 240 && display.x > 0){
                c++;
                if(c == 10){	// 1 seconds = 200 times * 500 us
                    stato = 'I';
                    LCD_Clear(Black);
                    c=0;
                }
            }			
            break;       
        case 'I':
            GUI_Text(60, 70, (uint8_t *) "Nota 1:", White, Black);
						sprintf(s,"%d Hz - %c", note[ indici[0] ], noteNome[ indici[0] ]);
						GUI_Text(140, 70, (uint8_t *)  s, White, Black);
            
            GUI_Text(60, 120, (uint8_t *)  "Nota 2:", White, Black);
						sprintf(s,"%d Hz - %c", note[ indici[1] ], noteNome[ indici[1] ]);
						GUI_Text(140, 120, (uint8_t *)  s, White, Black); 									// OLD GUI_Text(140, 70, (uint8_t *)  s, White, Black); //mia modifica GUI_Text(140, 120, (uint8_t *)  s, White, Black);

				
            GUI_Text(60, 216, (uint8_t *)  "              ", Black, White);
						GUI_Text(60, 200, (uint8_t *)  "      OK      ", Black, White);
						GUI_Text(60, 184, (uint8_t *)  "              ", Black, White);
        
            if(display.x <= 240 && display.x > 0){
                if (display.y > 200) {
                    stato = 'E';
                    LCD_Clear(Black);
                } else if (display.y < 80) {
                    stato = 'U';
                } else if (display.y >= 80 && display.y <= 200) {
                    stato = 'D';
                }
            }						
						notaSel = 0;        
            break;
        case 'U':
						notaSel = 1;
            GUI_Text(10, 70, (uint8_t *)  ">>>", Green, Black);
						sprintf(s,"%d Hz - %c", note[ indici[0] ], noteNome[ indici[0] ]);
						GUI_Text(140, 70, (uint8_t *)  s, White, Black);
				
						if(display.x <= 240 && display.x > 0){
                if (display.y > 200) {
                    stato = 'E';
                    LCD_Clear(Black);
                } else if (display.y >= 80 && display.y <=120) {
                    stato = 'D';
										GUI_Text(10, 70, (uint8_t *)  "   ", White, Black); 
                }
            }
						break;
				case 'D':
						notaSel = 2;
            GUI_Text(10, 120, (uint8_t *)  ">>>", Green, Black);
						sprintf(s,"%d Hz - %c", note[ indici[1] ], noteNome[ indici[1] ]);
						GUI_Text(140, 120, (uint8_t *)  s, White, Black);
				
						if(display.x <= 240 && display.x > 0){
                if (display.y > 200) {
                    stato = 'E';
									
                    LCD_Clear(Black);
                } else if (display.y < 80) {
                    stato = 'U';
									
										GUI_Text(10, 120, (uint8_t *)  "   ", White, Black); 
                }
            }
						break;	
				case 'E': //esco dal touch
						controllaTouch = 0;
						LPC_SC->PCON |= 0x1;									/* power-down	mode										*/
						LPC_SC->PCON &= ~(0x2);						
					  break;					
    }
	}
	
	LPC_TIM1->IR = 1; /* clear interrupt flag */
	return;
}

void TIMER2_IRQHandler(void)
{
	static int ticks=0;

	/* DAC management */
	
	if (i%2==0) {
		DAC_convert (SinTable30[ticks] << 6);
	}
		
	//prova = freqs[ indici[ (i+1)%2 ] ];
	
	if(i%4==0){
		init_timer(TIMER2, freqs[ indici[ (i+1)%2 ] ], 3, 0);
		start_timer(TIMER2);
	}
	
	ticks++;
	
	if(ticks==45) 
		ticks=0;
	
	LPC_TIM2->IR = 1; /* clear interrupt flag */
	return;
}

void TIMER3_IRQHandler(void)  //timer per lampeggio durante trasporto/arrivo/emergenza
{	
	if (i%2==0){
		led_on(7);
	}
	else
	{		
		led_off(7);
		//reset_timer(2);	
	}
	i++;
	
	if (i==10000)
		i=0;
	
	LPC_TIM3->IR = 1; /* clear interrupt flag */
	return;
}


void start_timer(uint8_t timer_num)
{
	switch (timer_num)
	{
	case TIMER0:
		pow_timer(TIMER0, ON);
		LPC_TIM0->TCR = 1;
		break;
	case TIMER1:
		pow_timer(TIMER1, ON);
		LPC_TIM1->TCR = 1;
		break;
	case TIMER2:
		pow_timer(TIMER2, ON);
		LPC_TIM2->TCR = 1;
		break;
	case TIMER3:
		pow_timer(TIMER3, ON);
		LPC_TIM3->TCR = 1;
		break;
	default:
		break;
	}
	return;
}

void pause_timer(uint8_t timer_num)
{
	switch (timer_num)
	{
	case TIMER0:
		LPC_TIM0->TCR = 0;
		break;
	case TIMER1:
		LPC_TIM1->TCR = 0;
		break;
	case TIMER2:
		LPC_TIM2->TCR = 0;
		break;
	case TIMER3:
		LPC_TIM3->TCR = 0;
		break;
	}
	return;
}

void reset_timer(uint8_t timer_num)
{
	switch (timer_num)
	{
	case TIMER0:
		LPC_TIM0->TCR |= 0x02;
		break;
	case TIMER1:
		LPC_TIM1->TCR |= 0x02;
		break;
	case TIMER2:
		LPC_TIM2->TCR |= 0x02;
		break;
	case TIMER3:
		LPC_TIM3->TCR |= 0x02;
		break;
	default:
		break;
	}
	return;
}

void init_timer(uint8_t timer_num, uint32_t timer_interval, uint32_t mode, uint8_t priority)
{
	switch (timer_num)
	{
	case TIMER0:
		pow_timer(TIMER0, ON);
		LPC_TIM0->MR0 = timer_interval;
		LPC_TIM0->MCR = mode;
		LPC_TIM0->TC = 0;
		NVIC_EnableIRQ(TIMER0_IRQn);
		NVIC_SetPriority(TIMER0_IRQn, priority);
		break;
	case TIMER1:
		pow_timer(TIMER1, ON);
		LPC_TIM1->MR0 = timer_interval;
		LPC_TIM1->MCR = mode;
		LPC_TIM1->TC = 0;
		NVIC_EnableIRQ(TIMER1_IRQn);
		NVIC_SetPriority(TIMER1_IRQn, priority);
		break;
	case TIMER2:
		pow_timer(TIMER2, ON);
		LPC_TIM2->MR0 = timer_interval;
		LPC_TIM2->MCR = mode;
		LPC_TIM2->TC = 0;
		NVIC_EnableIRQ(TIMER2_IRQn);
		NVIC_SetPriority(TIMER2_IRQn, priority);
		break;
	case TIMER3:
		pow_timer(TIMER3, ON);
		LPC_TIM3->MR0 = timer_interval;
		LPC_TIM3->MCR = mode;
		LPC_TIM3->TC = 0;
		NVIC_EnableIRQ(TIMER3_IRQn);
		NVIC_SetPriority(TIMER3_IRQn, priority);
		break;
	default:
		break;
	}
	return;
}

void pow_timer(uint8_t timer_num, uint8_t state)
{
	switch (timer_num)
	{
	case TIMER0:
		if (state == ON && _t0state==OFF){
			LPC_SC->PCONP |= (1 << 1);
			_t0state=ON;
		}
		if (state == OFF && _t0state==ON){
			LPC_SC->PCONP &= (0xFFFFFFFD);
			_t0state=OFF;
		}
		break;
	case TIMER1:
		if (state == ON && _t1state==OFF){
			LPC_SC->PCONP |= (1 << 2);
			_t1state=ON;
		}
		if (state == OFF && _t1state==ON){
			LPC_SC->PCONP &= (0xFFFFFFFB);
			_t1state=OFF;
		}
		break;
	case TIMER2:
		if (state == ON && _t2state==OFF){
			LPC_SC->PCONP |= (1 << 22);
			_t2state=ON;
		}
		if (state == OFF && _t2state==ON){
			LPC_SC->PCONP &= (0xFFBFFFFF);			
			_t2state=OFF;
		}
		break;
	case TIMER3:
		if (state == ON && _t3state==OFF){
			LPC_SC->PCONP |= (1 << 23);
			_t3state=ON;
		}
		if (state == OFF && _t3state==ON){
			LPC_SC->PCONP &= (0xFF7FFFFF);
			_t3state=OFF;
		}
		break;
	default:
		break;
	}
}
