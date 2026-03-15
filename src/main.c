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


/* Blink task toggles PD12 every 100 ms using FreeRTOS delay. */
static void BlinkTask_100ms(void *pvParameters) {
    (void)pvParameters;
    for(;;) {
        GPIOD->ODR ^= (1 << 12); // toggle LED on PD12
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


/* Blink task toggles PD13 every 500 ms using FreeRTOS delay. */
static void BlinkTask_500ms(void *pvParameters) {
    (void)pvParameters;
    for(;;) {
        GPIOD->ODR ^= (1 << 13); // toggle LED on PD13
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* Blink task toggles PD14 every 1000 ms using FreeRTOS delay. */
static void BlinkTask_1000ms(void *pvParameters) {
    (void)pvParameters;
    for(;;) {
        GPIOD->ODR ^= (1 << 14); // toggle LED on PD14
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Blink task toggles PD15 every 2000 ms using FreeRTOS delay. */
static void BlinkTask_2000ms(void *pvParameters) {
    (void)pvParameters;
    for(;;) {
        GPIOD->ODR ^= (1 << 15); // toggle LED on PD15
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}



static void prvSetupHardware(void)
{
    //Enable Clock to GPIOD Peripheral by writing 1 to AHB1ENR Bitfield of RCC
    RCC->AHB1ENR |= (1 << 3);
    
    // Reset MODER Bitfield of PD12 to configure PD12 as OUTPUT
    GPIOD->MODER &= ~(3 << (12 * MODER_WIDTH));
    // Write Value 1 to MODER Bitfield PD12 to configure PD12 as OUTPUT
    GPIOD->MODER |=  (1 << (12 * MODER_WIDTH));

    // Reset MODER Bitfield of PD12 to configure PD12 as OUTPUT
    GPIOD->MODER &= ~(3 << (13 * MODER_WIDTH));
    // Write Value 1 to MODER Bitfield PD12 to configure PD12 as OUTPUT
    GPIOD->MODER |=  (1 << (13 * MODER_WIDTH));

    // Reset MODER Bitfield of PD12 to configure PD12 as OUTPUT
    GPIOD->MODER &= ~(3 << (14 * MODER_WIDTH));
    // Write Value 1 to MODER Bitfield PD12 to configure PD12 as OUTPUT
    GPIOD->MODER |=  (1 << (14 * MODER_WIDTH));

    // Reset MODER Bitfield of PD12 to configure PD12 as OUTPUT
    GPIOD->MODER &= ~(3 << (15 * MODER_WIDTH));
    // Write Value 1 to MODER Bitfield PD12 to configure PD12 as OUTPUT
    GPIOD->MODER |=  (1 << (15 * MODER_WIDTH));
}

/**
 *@brief Main entry point for blinking project using FreeRTOS.
 *
 *GPIOA Peripherals are configured to OUTPUT, with LED connected to PD12 being toggled every 100ms
 **/
int main(void){

    prvSetupHardware();

    // Create a blinking task, 100ms period via vTaskDelay.
    xTaskCreate(BlinkTask_100ms, "Blink_100ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Create a blinking task, 500ms period via vTaskDelay.
    xTaskCreate(BlinkTask_500ms, "Blink_500ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Create a blinking task, 1000ms period via vTaskDelay.
    xTaskCreate(BlinkTask_1000ms, "Blink_1000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Create a blinking task, 2000ms period via vTaskDelay.
    xTaskCreate(BlinkTask_2000ms, "Blink_2000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Start the FreeRTOS scheduler.
    vTaskStartScheduler();

    // Should never reach here if scheduler starts.
    for(;;) { }
}
