/**
 *@file main.c
 *@brief Register Level Programming Simple Blink Project
 *@include main.c
 **/

#include <stdint.h>
#include "stm32f4xx.h"

#define MODER_WIDTH 2
#define pin5 5


void wait_ms(int time){
    for(int i = 0; i < time; i++){
        for(int j = 0; j < 1600; j++);
    }
}

/**
 *@brief Main entry point for blinking project.
 *
 *GPIOA Peripherals are configured to OUTPUT, with LED connected to PA5 being toggled every 100ms
 **/
int main(void){

    //Enable Clock to GPIOD Peripheral by writing 1 to AHB1ENR Bitfield of RCC
    RCC->AHB1ENR |= (1 << 3);
    
    // Reset MODER Bitfield of PD12 to configure PD12 as OUTPUT
    GPIOD->MODER &= ~(3 << (12 * MODER_WIDTH));
    // Write Value 1 to MODER Bitfield PD12 to configure PD12 as OUTPUT
    GPIOD->MODER |=  (1 << (12 * MODER_WIDTH));
    
    for(;;){
        // Toggle PA5
        GPIOD->ODR ^= (1 << 12);
        // Wait for 100ms
        wait_ms(100);
    }
}
