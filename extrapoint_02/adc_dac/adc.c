/*********************************************************************************************************
**--------------File Info---------------------------------------------------------------------------------
** File name:           IRQ_adc.c
** Last modified Date:  20184-12-30
** Last Version:        V1.00
** Descriptions:        functions to manage A/D interrupts
** Correlated files:    adc.h
**--------------------------------------------------------------------------------------------------------       
*********************************************************************************************************/

#include "lpc17xx.h"
#include "adc.h"
#include "../led/led.h"
#include "../timer/timer.h"

/*----------------------------------------------------------------------------
  A/D IRQ: Executed when A/D Conversion is ready (signal from ADC peripheral)
 *----------------------------------------------------------------------------*/
 
// k = (f)/(f'*(N))= 25Mhz/(f'*45)  f' è la frequenza che voglio

unsigned short AD_current; 
unsigned short AD_last = 0xFF;    		 /* Last converted value               */

/*
262	k=2120	c4
294	k=1890
330	k=1684
349	k=1592
392	k=1417
440	k=1263
494	k=1125
532	k=1062		c5
*/

extern int indici[];
extern int notaSel;

void ADC_IRQHandler(void) {	
	
	//potenziometro è acceso, voglio usare quello per decidere il valore di frequenza successivo
	AD_current = ((LPC_ADC->ADGDR>>4) & 0xFFF);				/* Read Conversion Result             */
	//è il valore che ho letto dal potenziometro e devo usarlo per ottenere un indice da usare per il vettore freqs
				
	AD_current = (AD_current*7/0xFFF);	// ad_current : AD_max = x : 7 		LED_On((AD_current*7/0xFFF));	
	
	if (AD_last != AD_current){
	
	if (notaSel == 1) {
		indici[0] = 7-AD_current;
	} 
	else if (notaSel == 2) {
		indici[1] = 7-AD_current;
	}
		AD_last = AD_current;
	}
	
}

/*----------------------------------------------------------------------------
  Function that initializes ADC
 *----------------------------------------------------------------------------*/
void ADC_init (void) {

  LPC_PINCON->PINSEL3 |=  (3UL<<30);      /* P1.31 is AD0.5                     */

  LPC_SC->PCONP       |=  (1<<12);      /* Enable power to ADC block          */

  LPC_ADC->ADCR        =  (1<< 5) |     /* select AD0.5 pin                   */
                          (4<< 8) |     /* ADC clock is 25MHz/5               */
                          (1<<21);      /* enable ADC                         */ 

  LPC_ADC->ADINTEN     =  (1<< 8);      /* global enable interrupt            */

  NVIC_EnableIRQ(ADC_IRQn);             /* enable ADC Interrupt               */
}

void ADC_start_conversion (void) {
	LPC_ADC->ADCR |=  (1<<24);            /* Start A/D Conversion 				*/
}				 


/*----------------------------------------------------------------------------
  Function that initializes ADC
 *----------------------------------------------------------------------------*/
void DAC_init(void) {
	LPC_PINCON->PINSEL1 |= (1<<21);
	LPC_PINCON->PINSEL1 &= ~(1<<20);
	LPC_GPIO0->FIODIR |= (1<<26);
}

void DAC_convert(int value) {
	LPC_DAC->DACR = value;
}				 

