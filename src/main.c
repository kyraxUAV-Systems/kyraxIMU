/**
 *@file main.c
 *@brief Register-level STM32F407 CAN1 transmit demo with FreeRTOS blink tasks
 **/

#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"

#define MODER_WIDTH 2U
#define CAN_STD_ID  0x123U
#define CAN_DLC     8U

static volatile uint32_t g_can_rx_id = 0U;
static volatile uint8_t g_can_rx_dlc = 0U;
static volatile uint8_t g_can_rx_data[CAN_DLC] = {0U};
static volatile uint32_t g_can_rx_count = 0U;

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

/* Blink task toggles PD12 every 100 ms using vTaskDelayUntil */
static void BlinkTask_100ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100);

    for(;;) {
        GPIOD->ODR ^= (1 << 12);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Blink task toggles PD13 every 500 ms using vTaskDelayUntil */
static void BlinkTask_500ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(500);

    for(;;) {
        GPIOD->ODR ^= (1 << 13);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Blink task toggles PD14 every 1000 ms using vTaskDelayUntil */
static void BlinkTask_1000ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);

    for(;;) {
        GPIOD->ODR ^= (1 << 14);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Blink task toggles PD15 every 2000 ms using vTaskDelayUntil */
static void BlinkTask_2000ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(2000);

    for(;;) {
        GPIOD->ODR ^= (1 << 15);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Transmit a basic CAN frame on mailbox 0 */
static int32_t prvCanTransmitStdFrame(uint32_t std_id, const uint8_t *payload, uint8_t dlc) {
    uint32_t tx_l, tx_h;

    if ((payload == NULL) || (dlc > CAN_DLC)) {
        return -1;
    }

    while ((CAN1->TSR & CAN_TSR_TME0) == 0U) {
        ;
    }

    tx_l = ((uint32_t)payload[0]) |
           ((uint32_t)payload[1] << 8U) |
           ((uint32_t)payload[2] << 16U) |
           ((uint32_t)payload[3] << 24U);
    tx_h = ((uint32_t)payload[4]) |
           ((uint32_t)payload[5] << 8U) |
           ((uint32_t)payload[6] << 16U) |
           ((uint32_t)payload[7] << 24U);

    CAN1->sTxMailBox[0].TIR = (std_id << CAN_TI0R_STID_Pos) | CAN_TI0R_TXRQ;
    CAN1->sTxMailBox[0].TDTR = dlc;
    CAN1->sTxMailBox[0].TDLR = tx_l;
    CAN1->sTxMailBox[0].TDHR = tx_h;

    return 0;
}

/* Periodic task that sends a basic CAN frame every 250 ms */
static void CanTxTask_250ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(250);
    uint8_t payload[CAN_DLC] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};

    for(;;) {
        payload[0] ^= 0x01U;
        payload[1] ^= 0x02U;
        (void)prvCanTransmitStdFrame(CAN_STD_ID, payload, CAN_DLC);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Configure filter bank 0 to accept all standard frames into FIFO0 */
static void prvCanConfigureRxFilter(void) {
    CAN1->FMR |= CAN_FMR_FINIT;
    CAN1->FA1R &= ~CAN_FA1R_FACT0;
    CAN1->FM1R &= ~CAN_FM1R_FBM0;
    CAN1->FS1R |= CAN_FS1R_FSC0;
    CAN1->sFilterRegister[0].FR1 = 0U;
    CAN1->sFilterRegister[0].FR2 = 0U;
    CAN1->FA1R |= CAN_FA1R_FACT0;
    CAN1->FMR &= ~CAN_FMR_FINIT;
}

/* Poll FIFO0 and store the latest received frame */
static void CanRxTask_100ms(void *pvParameters) {
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(100);

    for(;;) {
        while ((CAN1->RF0R & CAN_RF0R_FMP0) != 0U) {
            uint32_t rir = CAN1->sFIFOMailBox[0].RIR;
            uint32_t rdtr = CAN1->sFIFOMailBox[0].RDTR;
            uint8_t dlc = (uint8_t)(rdtr & 0x0FU);
            uint8_t payload[CAN_DLC] = {0U};

            if (dlc > CAN_DLC) {
                dlc = CAN_DLC;
            }

            payload[0] = (uint8_t)(CAN1->sFIFOMailBox[0].RDLR & 0xFFU);
            payload[1] = (uint8_t)((CAN1->sFIFOMailBox[0].RDLR >> 8U) & 0xFFU);
            payload[2] = (uint8_t)((CAN1->sFIFOMailBox[0].RDLR >> 16U) & 0xFFU);
            payload[3] = (uint8_t)((CAN1->sFIFOMailBox[0].RDLR >> 24U) & 0xFFU);
            payload[4] = (uint8_t)(CAN1->sFIFOMailBox[0].RDHR & 0xFFU);
            payload[5] = (uint8_t)((CAN1->sFIFOMailBox[0].RDHR >> 8U) & 0xFFU);
            payload[6] = (uint8_t)((CAN1->sFIFOMailBox[0].RDHR >> 16U) & 0xFFU);
            payload[7] = (uint8_t)((CAN1->sFIFOMailBox[0].RDHR >> 24U) & 0xFFU);

            g_can_rx_id = ((rir & CAN_RI0R_STID_Msk) >> CAN_RI0R_STID_Pos);
            g_can_rx_dlc = dlc;
            for (uint32_t i = 0U; i < dlc; i++) {
                g_can_rx_data[i] = payload[i];
            }
            g_can_rx_count++;

            GPIOD->ODR ^= (1U << 15);
            CAN1->RF0R |= CAN_RF0R_RFOM0;
        }

        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Hardware setup for GPIOD pins PD12..PD15 and CAN1 on PB8/PB9 */
static void prvSetupHardware(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOBEN; /* enable GPIOD and GPIOB clocks */

    for (uint32_t pin = 12; pin <= 15; pin++) {
        GPIOD->MODER &= ~(3U << (pin * MODER_WIDTH)); /* reset mode */
        GPIOD->MODER |=  (1U << (pin * MODER_WIDTH)); /* set as output */
    }

    /* Configure PB8 (CAN1_RX) and PB9 (CAN1_TX) as alternate function */
    GPIOB->MODER &= ~((3U << (8U * MODER_WIDTH)) | (3U << (9U * MODER_WIDTH)));
    GPIOB->MODER |=  ((2U << (8U * MODER_WIDTH)) | (2U << (9U * MODER_WIDTH)));
    GPIOB->OTYPER &= ~((1U << 8U) | (1U << 9U));
    GPIOB->OSPEEDR |= ((3U << (8U * MODER_WIDTH)) | (3U << (9U * MODER_WIDTH)));
    GPIOB->PUPDR &= ~((3U << (8U * MODER_WIDTH)) | (3U << (9U * MODER_WIDTH)));
    GPIOB->AFR[1] &= ~((0xFU << ((8U - 8U) * 4U)) | (0xFU << ((9U - 8U) * 4U)));
    GPIOB->AFR[1] |= ((0x9U << ((8U - 8U) * 4U)) | (0x9U << ((9U - 8U) * 4U)));

    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;

    CAN1->MCR |= CAN_MCR_INRQ;
    while ((CAN1->MSR & CAN_MSR_INAK) == 0U) {
        ;
    }

    /* 16 MHz clock, 500 kbit/s: BRP=1, TS1=12, TS2=2 */
    CAN1->BTR = (1U << CAN_BTR_BRP_Pos) |
                (12U << CAN_BTR_TS1_Pos) |
                (2U << CAN_BTR_TS2_Pos);
    prvCanConfigureRxFilter();
    CAN1->MCR &= ~CAN_MCR_INRQ;
    while ((CAN1->MSR & CAN_MSR_INAK) != 0U) {
        ;
    }

    CAN1->MCR |= CAN_MCR_TXFP;
}

/**
 *@brief Main entry point
 **/
int main(void) {
    prvSetupHardware();

    /* Create separate blink tasks */
    xTaskCreate(BlinkTask_100ms,  "Blink_100ms",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(BlinkTask_500ms,  "Blink_500ms",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(BlinkTask_1000ms, "Blink_1000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(BlinkTask_2000ms, "Blink_2000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(CanTxTask_250ms, "CanTx_250ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(CanRxTask_100ms, "CanRx_100ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    /* Start scheduler */
    vTaskStartScheduler();

    for(;;) { } /* should never reach here */
}