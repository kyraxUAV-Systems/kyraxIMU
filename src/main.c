/**
 *@file main.c
 *@brief Register Level Programming Simple Blink Project using FreeRTOS delayUntil
 **/

#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"

#include "spi_driver.h"

#define MODER_WIDTH 2

void wait_ms(int time){
    for(int i = 0; i < time; i++){
        for(int j = 0; j < 1600; j++);
    }
}

int16_t accel_data_x_fused = 0;


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

typedef struct IMU_t{
    uint16_t accel_data_x;
    uint16_t accel_data_y;
    uint16_t accel_data_z;
    uint16_t accel_gyro_x;
    uint16_t accel_gyro_y;
    uint16_t accel_gyro_z;
}IMU_t;

IMU_t imu_frame;

static void TaskCluster_100ms(void *pvParameters) {
    //(void)pvParameters;
    IMU_t * imu = (IMU_t *)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100);

    for(;;) {
        //spi_write_keep_CS_LOW(0x9F00);
        int16_t imu_burst_frame_read_1 = (spi_transfer(0x9F00)); // [7:0] - <MSB: Garbage            | LSB: 0x1F ACCEL_DATA_X1 > 
        int16_t imu_burst_frame_read_2 = (spi_transfer(0x0100)); // [7:0] - <MSB: 0x20 ACCEL_DATA_X0 | LSB: 0x21 ACCEL_DATA_Y1 >
        int16_t imu_burst_frame_read_3 = (spi_transfer(0x0200)); // [7:0] - <MSB: 0x22 ACCEL_DATA_Y0 | LSB: 0x23 ACCEL_DATA_Z1 >
        int16_t imu_burst_frame_read_4 = (spi_transfer(0x0300)); // [7:0] - <MSB: 0x24 ACCEL_DATA_Z0 | LSB: 0x25 GYRO_DATA_X1  >
        int16_t imu_burst_frame_read_5 = (spi_transfer(0x0400)); // [7:0] - <MSB: 0x26 GYRO_DATA_X0  | LSB: 0x27 GYRO_DATA_Y1  >
        int16_t imu_burst_frame_read_6 = (spi_transfer(0x0500)); // [7:0] - <MSB: 0x28 GYRO_DATA_Y0  | LSB: 0x29 GYRO_DATA_Z1  >
        int16_t imu_burst_frame_read_7 = (spi_transfer(0x0600)); // [7:0] - <MSB: 0x2A GYRO_DATA_Z0  | LSB: IGNORE             >

        imu->accel_data_x = (int16_t)((imu_burst_frame_read_1 << 8U) | ((imu_burst_frame_read_2 & 0xFF00) >> 8U));
        imu->accel_data_y = (int16_t)((imu_burst_frame_read_2 << 8U) | ((imu_burst_frame_read_3 & 0xFF00) >> 8U));
        imu->accel_data_z = (int16_t)((imu_burst_frame_read_3 << 8U) | ((imu_burst_frame_read_4 & 0xFF00) >> 8U));
        imu->accel_gyro_x = (int16_t)((imu_burst_frame_read_4 << 8U) | ((imu_burst_frame_read_5 & 0xFF00) >> 8U));
        imu->accel_gyro_y = (int16_t)((imu_burst_frame_read_5 << 8U) | ((imu_burst_frame_read_6 & 0xFF00) >> 8U));
        imu->accel_gyro_z = (int16_t)((imu_burst_frame_read_6 << 8U) | ((imu_burst_frame_read_7 & 0xFF00) >> 8U));
        // Deselect CS
        GPIOA->ODR |= (1U << 4U);

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

    spi_init();

}


/**
 *@brief Main entry point
 **/
int main(void) {

    prvSetupHardware();

//vTaskDelay(pdMS_TO_TICKS(200));

    wait_ms(100);
    // IMU RESET, DEVICE CONFIG 0x11 + 0x11 for RESET
    spi_write((0x11 << 8U) | 0x01);
    wait_ms(2);

    // Set ACC and GYRO Low Noise, PWR_MGMT0 0x4E + 0x0F for Low Noise Mode
    spi_write((0x4E << 8U) | 0x0F);
    wait_ms(60);


    // Create separate blink tasks
    xTaskCreate(TaskCluster_100ms,  "Blink_100ms",  configMINIMAL_STACK_SIZE, (void*)(&imu_frame), tskIDLE_PRIORITY + 3, NULL);
    xTaskCreate(TaskCluster_500ms,  "Blink_500ms",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskCluster_1000ms, "Blink_1000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskCluster_2000ms, "Blink_2000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // Start scheduler
    vTaskStartScheduler();

    for(;;) { } // should never reach here
}
