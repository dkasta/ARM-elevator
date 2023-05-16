#include "lpc17xx.h"
#include "timer.h"
#include "../common.h"
#include "../led/led.h"
#include "../elevator/elevator.h"
#include "../rit/rit.h"

static uint8_t _t0state=ON;
static uint8_t _t1state=ON;
static uint8_t _t2state=OFF;
static uint8_t _t3state=OFF;

volatile int enable_3s_check = 0;

extern int ending_floor;
extern int starting_floor;
extern int alarm;
extern int enable_joystick;


void TIMER0_IRQHandler(void)
{
	//una volta scaduto il timer per il trasporto,
	//l'ascensore ha raggiunto il piano desiderato. 
	//i reserved leds e il blink a 2Hz si spengono
	
	led_off(0);//reserved led piano 0
	led_off(2);//reserved led piano 1
	
	if (ending_floor == 1){
		setElevatorFloor(1);//ora l'ascensore è al piano 1
		starting_floor = 1;
		ending_floor = 0;
	}
	if (ending_floor == 0){
		setElevatorFloor(0);//ora l'ascensore è al piano 0
		starting_floor = 0;
		ending_floor = 1;
	}
	
	enable_joystick = 1; //comincio a controllare il joystick
	
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

void TIMER1_IRQHandler(void) //timer di 60s per inutilizzo/allarme
{
	if (alarm == 1) { //utente è fermo tra i piani e dopo 60s non ha premuto piu il joytick quindi scatta il caso d'allarme
		led_on(1);
		led_on(3);
	}
	else { //ascensore è inutilizzato da 60s
		led_off(0); //reserved led piano 0
		led_off(2); //reserved led piano 1
		led_off(7); //status led del controller
		enable_joystick  = 0; 
	}
	
	LPC_TIM1->IR = 1; /* clear interrupt flag */
	return;
}

void TIMER2_IRQHandler(void)
{
	LPC_TIM2->IR = 1; /* clear interrupt flag */
	return;
}

void TIMER3_IRQHandler(void)  //timer per lampeggio durante trasporto/arrivo
{	
	static int i = 0;
	
	if (i%2==0)
		led_on(7);
	else 
		led_off(7);
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
