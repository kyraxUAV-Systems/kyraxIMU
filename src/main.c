/**
 *@file main.c
 *@brief Register Level Programming Simple Blink Project
 *@include main.c
 **/


#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"

#define MODER_WIDTH 2
#define pin5 5

/* FreeRTOS runtime / linker symbols */
uint32_t SystemCoreClock = 16000000UL; /* Required by FreeRTOS port.c for tick timer setup */

void vApplicationIdleHook(void) { } /* required by configUSE_IDLE_HOOK */
void vApplicationTickHook(void) { } /* optional, currently no action */

void vApplicationMallocFailedHook(void) {
    taskDISABLE_INTERRUPTS();
    for(;;) { }
} /* required if configUSE_MALLOC_FAILED_HOOK is enabled or to avoid weak unresolved symbols */

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for(;;) { }
}

/* libc helpers needed by FreeRTOS components (queue/stream buffer etc.).
   Because this is bare-metal and -nostdlib is used, we provide minimal definitions. */
void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}


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
