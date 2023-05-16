#include <stdio.h>
#include "elevator.h"
#include "../led/led.h"

volatile int state; //variabile per il piano a cui si trova l'ascensore

void initElevator(void){
	
	setElevatorFloor(1);
	led_off(7);		//status led del controller
	
	return;
}

void setElevatorFloor(int num){
	
	state = num;
	
	return;
}

int getElevatorFloor(void){

	return state;
}

//controlla se i reserved led sono spenti
int isFree(unsigned int num) {

	return check_on_off(num);
}
