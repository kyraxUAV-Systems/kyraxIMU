/**
 *@file main.c
 *@brief Register Level Programming Simple Blink Project using FreeRTOS delayUntil
 **/

#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"

#define MODER_WIDTH 2

/* FreeRTOS runtime / linker symbols */
uint32_t SystemCoreClock = 16000000UL; /* Required by FreeRTOS port.c for tick timer setup */

/* FreeRTOS hooks */
void vApplicationIdleHook(void) { }
void vApplicationTickHook(void) { }

void vApplicationMallocFailedHook(void) {
    taskDISABLE_INTERRUPTS();
    for(;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for(;;) { }
}

/* libc minimal helpers */
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

static void TaskCluster_100ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100);

    for(;;) {
        GPIOD->ODR ^= (1 << 12);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

static void TaskCluster_500ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(500);

    for(;;) {
        GPIOD->ODR ^= (1 << 13);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

static void TaskCluster_1000ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);

    for(;;) {
        GPIOD->ODR ^= (1 << 14);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

static void TaskCluster_2000ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(2000);

    for(;;) {
        GPIOD->ODR ^= (1 << 15);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Hardware setup for GPIOD pins PD12..PD15 */
static void prvSetupHardware(void) {
    RCC->AHB1ENR |= (1 << 3); // enable GPIOD clock

    for (uint32_t pin = 12; pin <= 15; pin++) {
        GPIOD->MODER &= ~(3 << (pin * MODER_WIDTH)); // reset mode
        GPIOD->MODER |=  (1 << (pin * MODER_WIDTH)); // set as output
    }
}

/**
 *@brief Main entry point
 **/
int main(void) {

    prvSetupHardware();

    // Create separate blink tasks
    xTaskCreate(TaskCluster_100ms,  "Blink_100ms",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskCluster_500ms,  "Blink_500ms",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskCluster_1000ms, "Blink_1000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskCluster_2000ms, "Blink_2000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Start scheduler
    vTaskStartScheduler();

    for(;;) { } // should never reach here
}