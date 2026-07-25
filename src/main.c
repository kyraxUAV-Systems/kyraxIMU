/**
 *@file main.c
 *@brief Register-level STM32F407 CAN1 transmit demo with FreeRTOS blink tasks
 **/

/* Include the fixed-width integer definitions used by the firmware and the CAN helper code. */
#include <stdint.h>
/* Include the size_t definition used by the small libc compatibility helpers. */
#include <stddef.h>
/* Include the STM32F4 CMSIS header that exposes the peripheral register layout. */
#include "stm32f4xx.h"
#include "can1.h"

/* Include the FreeRTOS core definitions used by tasks and scheduling helpers. */
#include "FreeRTOS.h"
/* Include the FreeRTOS task API used to create and manage the firmware tasks. */
#include "task.h"

/* Define the bit width used for the GPIO MODER register field calculations. */
#define MODER_WIDTH 2U
/* Define the default standard CAN identifier used by the transmit task. */
#define CAN_STD_ID  0x123U
/* Define the one standard data-frame identifier that CAN1 accepts and stores. */
#define CAN_RX_STD_ID 0x321U
/* Define the default payload length used by the transmit and receive helpers. */
#define CAN_DLC     CAN1_MAX_DLC

/* Store the latest hardware-filtered frame for use by the rest of the firmware. */
volatile Can1RxFrame g_can1_rx_latest = {0U};
/* Increment before and after an update so readers can obtain a coherent snapshot. */
static volatile uint32_t g_can1_rx_sequence = 0U;

/* Copy the latest received frame without exposing a partially-written ISR update. */
int32_t Can1CopyLatestRxFrame(Can1RxFrame *destination) {
    uint32_t sequence_before;
    uint32_t sequence_after;

    if (destination == NULL) {
        return -1;
    }

    for (;;) {
        sequence_before = g_can1_rx_sequence;
        if ((sequence_before & 1U) != 0U) {
            continue;
        }

        destination->id = g_can1_rx_latest.id;
        destination->dlc = g_can1_rx_latest.dlc;
        for (uint32_t i = 0U; i < CAN_DLC; i++) {
            destination->data[i] = g_can1_rx_latest.data[i];
        }
        destination->count = g_can1_rx_latest.count;
        sequence_after = g_can1_rx_sequence;
        if ((sequence_before == sequence_after) && ((sequence_after & 1U) == 0U)) {
            break;
        }
    }

    return (destination->count == 0U) ? -2 : 0;
}

/* FreeRTOS runtime / linker symbols */
/* Define the system clock frequency required by the FreeRTOS port layer. */
uint32_t SystemCoreClock = 16000000UL; /* Required by FreeRTOS port.c for tick timer setup */

/* FreeRTOS hooks */
/* Provide the idle hook used by FreeRTOS when the scheduler has no work to run. */
void vApplicationIdleHook(void) { }
/* Provide the tick hook used by FreeRTOS for periodic code that runs on every tick. */
void vApplicationTickHook(void) { }

/* Handle memory allocation failure by disabling interrupts and halting execution. */
void vApplicationMallocFailedHook(void) {
    /* Disable interrupts to stop the system from running further after an allocation failure. */
    taskDISABLE_INTERRUPTS();
    /* Enter an infinite loop to halt execution after the failure. */
    for(;;) { }
}

/* Handle a stack overflow by disabling interrupts and halting execution. */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    /* Mark the unused task parameter so the compiler does not warn about it. */
    (void)xTask;
    /* Mark the unused task-name parameter so the compiler does not warn about it. */
    (void)pcTaskName;
    /* Disable interrupts to stop execution after a stack overflow event. */
    taskDISABLE_INTERRUPTS();
    /* Enter an infinite loop to halt execution after the stack overflow. */
    for(;;) { }
}

/* libc minimal helpers */
/* Implement a small memcpy replacement for bare-metal builds that do not have libc support. */
void *memcpy(void *dest, const void *src, size_t n) {
    /* Create a byte pointer for the destination buffer. */
    unsigned char *d = dest;
    /* Create a byte pointer for the source buffer. */
    const unsigned char *s = src;
    /* Copy the requested number of bytes one byte at a time. */
    while (n--) *d++ = *s++;
    /* Return the destination pointer to the caller. */
    return dest;
}

/* Implement a small memset replacement for bare-metal builds that do not have libc support. */
void *memset(void *s, int c, size_t n) {
    /* Create a byte pointer for the destination buffer. */
    unsigned char *p = s;
    /* Fill the requested number of bytes with the specified byte value. */
    while (n--) *p++ = (unsigned char)c;
    /* Return the destination pointer to the caller. */
    return s;
}

/* Blink task toggles PD12 every 100 ms using vTaskDelayUntil */
/* Define the 100 ms LED blink task that toggles PD12. */
static void BlinkTask_100ms(void *pvParameters) {
    /* Mark the unused task parameter so the compiler does not warn about it. */
    (void)pvParameters;
    /* Capture the scheduler tick count so the task can keep a periodic cadence. */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* Convert 100 milliseconds into a FreeRTOS tick count. */
    const TickType_t xPeriod = pdMS_TO_TICKS(100);

    /* Start an infinite loop that toggles the LED at the requested interval. */
    for(;;) {
        /* Toggle the PD12 output bit in the GPIOD output data register. */
        GPIOD->ODR ^= (1 << 12);
        /* Delay until the next wake-up time so the period stays consistent. */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Blink task toggles PD13 every 500 ms using vTaskDelayUntil */
/* Define the 500 ms LED blink task that toggles PD13. */
static void BlinkTask_500ms(void *pvParameters) {
    /* Mark the unused task parameter so the compiler does not warn about it. */
    (void)pvParameters;
    /* Capture the scheduler tick count so the task can keep a periodic cadence. */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* Convert 500 milliseconds into a FreeRTOS tick count. */
    const TickType_t xPeriod = pdMS_TO_TICKS(500);

    /* Start an infinite loop that toggles the LED at the requested interval. */
    for(;;) {
        /* Toggle the PD13 output bit in the GPIOD output data register. */
        GPIOD->ODR ^= (1 << 13);
        /* Delay until the next wake-up time so the period stays consistent. */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Blink task toggles PD14 every 1000 ms using vTaskDelayUntil */
/* Define the 1000 ms LED blink task that toggles PD14. */
static void BlinkTask_1000ms(void *pvParameters) {
    /* Mark the unused task parameter so the compiler does not warn about it. */
    (void)pvParameters;
    /* Capture the scheduler tick count so the task can keep a periodic cadence. */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* Convert 1000 milliseconds into a FreeRTOS tick count. */
    const TickType_t xPeriod = pdMS_TO_TICKS(1000);

    /* Start an infinite loop that toggles the LED at the requested interval. */
    for(;;) {
        /* Toggle the PD14 output bit in the GPIOD output data register. */
        GPIOD->ODR ^= (1 << 14);
        /* Delay until the next wake-up time so the period stays consistent. */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Blink task toggles PD15 every 2000 ms using vTaskDelayUntil */
/* Define the 2000 ms LED blink task that toggles PD15. */
static void BlinkTask_2000ms(void *pvParameters) {
    /* Mark the unused task parameter so the compiler does not warn about it. */
    (void)pvParameters;
    /* Capture the scheduler tick count so the task can keep a periodic cadence. */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* Convert 2000 milliseconds into a FreeRTOS tick count. */
    const TickType_t xPeriod = pdMS_TO_TICKS(2000);

    /* Start an infinite loop that toggles the LED at the requested interval. */
    for(;;) {
        /* Toggle the PD15 output bit in the GPIOD output data register. */
        GPIOD->ODR ^= (1 << 15);
        /* Delay until the next wake-up time so the period stays consistent. */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Transmit a basic CAN frame on mailbox 0. */
/* Define a helper that writes a standard CAN frame into transmit mailbox 0. */
static int32_t prvCanTransmitStdFrame(uint32_t std_id, const uint8_t *payload, uint8_t dlc) {
    /* Allocate temporary variables for the low and high data words of the frame payload. */
    uint32_t tx_l, tx_h;

    /* Reject null payload pointers, invalid standard IDs, or oversized payloads. */
    if ((payload == NULL) || (std_id > 0x7FFU) || (dlc > CAN_DLC)) {
        /* Return an error code when the caller provided invalid data. */
        return -1;
    }

    /* Do not block the periodic task if the previous frame is still pending. */
    if ((CAN1->TSR & CAN_TSR_TME0) == 0U) {
        /* Tell the caller that mailbox 0 is still occupied. */
        return -2;
    }

    /* Pack the first four payload bytes into the low data register word. */
    tx_l = ((uint32_t)payload[0]) |
           /* Pack payload byte 1 into the low data word. */
           ((uint32_t)payload[1] << 8U) |
           /* Pack payload byte 2 into the low data word. */
           ((uint32_t)payload[2] << 16U) |
           /* Pack payload byte 3 into the low data word. */
           ((uint32_t)payload[3] << 24U);
    /* Pack the last four payload bytes into the high data register word. */
    tx_h = ((uint32_t)payload[4]) |
           /* Pack payload byte 5 into the high data word. */
           ((uint32_t)payload[5] << 8U) |
           /* Pack payload byte 6 into the high data word. */
           ((uint32_t)payload[6] << 16U) |
           /* Pack payload byte 7 into the high data word. */
           ((uint32_t)payload[7] << 24U);

    /*
     * Load the complete mailbox before requesting transmission.  TXRQ starts
     * transmission immediately, so it must be the final mailbox write.
     */
    CAN1->sTxMailBox[0].TIR = (std_id << CAN_TI0R_STID_Pos);
    /* Load the mailbox data-length register with the requested DLC. */
    CAN1->sTxMailBox[0].TDTR = dlc;
    /* Load the low payload word into the transmit data low register. */
    CAN1->sTxMailBox[0].TDLR = tx_l;
    /* Load the high payload word into the transmit data high register. */
    CAN1->sTxMailBox[0].TDHR = tx_h;

    /* Request transmission only after ID, DLC, and both data words are valid. */
    CAN1->sTxMailBox[0].TIR |= CAN_TI0R_TXRQ;

    /* Return success to the caller after the mailbox has been armed. */
    return 0;
}

/* Periodic task that sends a basic CAN frame every 250 ms */
/* Define a task that sends a CAN frame every 250 ms. */
static void CanTxTask_250ms(void *pvParameters) {
    /* Mark the unused task parameter so the compiler does not warn about it. */
    (void)pvParameters;
    /* Capture the current tick count so this task can remain periodic. */
    TickType_t xLastWakeTime = xTaskGetTickCount();
    /* Convert 250 milliseconds into a FreeRTOS tick count. */
    const TickType_t xPeriod = pdMS_TO_TICKS(250);
    /* Define a sample 8-byte payload that will be transmitted on the bus. */
    uint8_t payload[CAN_DLC] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};

    /* Loop forever and send a CAN frame on every wake-up. */
    for(;;) {
        /* Toggle the first payload byte so the transmitted data changes slightly over time. */
        payload[0] ^= 0x01U;
        /* Toggle the second payload byte so the transmitted data changes slightly over time. */
        payload[1] ^= 0x02U;
        /* Send the current payload to the CAN mailbox helper. */
        (void)prvCanTransmitStdFrame(CAN_STD_ID, payload, CAN_DLC);
        /* Delay until the next periodic wake-up time. */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* Configure filter bank 0 to accept CAN_RX_STD_ID standard data frames into FIFO0. */
static void prvCanConfigureRxFilter(void) {
    /* Enter filter initialization mode so the filter registers can be reprogrammed. */
    CAN1->FMR |= CAN_FMR_FINIT;
    /* Reserve filter banks 0 through 13 for CAN1; CAN2 is not used in this project. */
    CAN1->FMR = (CAN1->FMR & ~CAN_FMR_CAN2SB) | (14U << CAN_FMR_CAN2SB_Pos);
    /* Disable the filter activation bit for filter bank 0 before reconfiguring it. */
    CAN1->FA1R &= ~CAN_FA1R_FACT0;
    /* Configure filter bank 0 as a mask mode filter rather than a list mode filter. */
    CAN1->FM1R &= ~CAN_FM1R_FBM0;
    /* Configure filter bank 0 as a single 32-bit scale filter. */
    CAN1->FS1R |= CAN_FS1R_FSC0;
    /* Route filter-bank-0 matches to receive FIFO0. */
    CAN1->FFA1R &= ~CAN_FFA1R_FFA0;
    /* Put the required 11-bit identifier into the standard-ID position. */
    CAN1->sFilterRegister[0].FR1 = (CAN_RX_STD_ID << CAN_RI0R_STID_Pos);
    /* Compare all ID bits and require a standard (IDE=0), data (RTR=0) frame. */
    CAN1->sFilterRegister[0].FR2 = CAN_RI0R_STID_Msk | CAN_RI0R_IDE | CAN_RI0R_RTR;
    /* Re-enable the filter bank after programming the filter registers. */
    CAN1->FA1R |= CAN_FA1R_FACT0;
    /* Exit filter initialization mode and allow the filter to become active. */
    CAN1->FMR &= ~CAN_FMR_FINIT;
}

/* Read every pending FIFO0 frame and retain the most recent accepted frame. */
void CAN1_RX0_IRQHandler(void) {
    while ((CAN1->RF0R & CAN_RF0R_FMP0) != 0U) {
        uint32_t rir = CAN1->sFIFOMailBox[0].RIR;
        uint32_t rdtr = CAN1->sFIFOMailBox[0].RDTR;
        uint32_t rdlr = CAN1->sFIFOMailBox[0].RDLR;
        uint32_t rdhr = CAN1->sFIFOMailBox[0].RDHR;
        uint8_t dlc = (uint8_t)(rdtr & 0x0FU);

        if (dlc > CAN_DLC) {
            dlc = CAN_DLC;
        }

        /* Mark the shared frame as being updated before changing any of its fields. */
        g_can1_rx_sequence++;
        g_can1_rx_latest.id = (rir & CAN_RI0R_STID_Msk) >> CAN_RI0R_STID_Pos;
        g_can1_rx_latest.dlc = dlc;
        g_can1_rx_latest.data[0] = (uint8_t)(rdlr & 0xFFU);
        g_can1_rx_latest.data[1] = (uint8_t)((rdlr >> 8U) & 0xFFU);
        g_can1_rx_latest.data[2] = (uint8_t)((rdlr >> 16U) & 0xFFU);
        g_can1_rx_latest.data[3] = (uint8_t)((rdlr >> 24U) & 0xFFU);
        g_can1_rx_latest.data[4] = (uint8_t)(rdhr & 0xFFU);
        g_can1_rx_latest.data[5] = (uint8_t)((rdhr >> 8U) & 0xFFU);
        g_can1_rx_latest.data[6] = (uint8_t)((rdhr >> 16U) & 0xFFU);
        g_can1_rx_latest.data[7] = (uint8_t)((rdhr >> 24U) & 0xFFU);
        g_can1_rx_latest.count++;
        /* Mark the shared frame as stable before application code can copy it. */
        g_can1_rx_sequence++;

        /* Toggle PD15 to make an accepted receive event visible on the board. */
        GPIOD->ODR ^= (1U << 15);
        /* Release the FIFO output mailbox after every register has been copied. */
        CAN1->RF0R |= CAN_RF0R_RFOM0;
    }
}

/* Hardware setup for GPIOD pins PD12..PD15 and CAN1 on PB8/PB9 */
/* Define the hardware initialization routine for the LED outputs and CAN1 peripheral. */
static void prvSetupHardware(void) {
    /* Enable the GPIOB clock used by the CAN1 RX/TX pins and the GPIOD clock used by the LED outputs. */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOBEN; /* enable GPIOD and GPIOB clocks */
    /* Read back the register to ensure the GPIO clocks are active before use. */
    (void)RCC->AHB1ENR;

    /* Configure the LED output pins PD12 through PD15 as push-pull outputs. */
    for (uint32_t pin = 12; pin <= 15; pin++) {
        /* Clear the mode bits for the current LED pin before programming the new mode. */
        GPIOD->MODER &= ~(3U << (pin * MODER_WIDTH)); /* reset mode */
        /* Set the current LED pin to output mode. */
        GPIOD->MODER |=  (1U << (pin * MODER_WIDTH)); /* set as output */
    }

    /* Configure PB8 (CAN1_RX) and PB9 (CAN1_TX) as alternate function pins. */
    GPIOB->MODER &= ~((3U << (8U * MODER_WIDTH)) | (3U << (9U * MODER_WIDTH)));
    /* Select alternate-function mode for PB8 and PB9. */
    GPIOB->MODER |=  ((2U << (8U * MODER_WIDTH)) | (2U << (9U * MODER_WIDTH)));
    /* Keep the CAN pins in push-pull output mode for the alternate-function driver. */
    GPIOB->OTYPER &= ~((1U << 8U) | (1U << 9U));
    /* Set the GPIO speed for PB8 and PB9 to the highest supported level. */
    GPIOB->OSPEEDR &= ~((3U << (8U * MODER_WIDTH)) | (3U << (9U * MODER_WIDTH)));
    GPIOB->OSPEEDR |= ((3U << (8U * MODER_WIDTH)) | (3U << (9U * MODER_WIDTH)));
    /* Clear any pull-up or pull-down settings on the CAN pins. */
    GPIOB->PUPDR &= ~((3U << (8U * MODER_WIDTH)) | (3U << (9U * MODER_WIDTH)));
    /* Select alternate-function 9 for PB8 and PB9, which maps them to CAN1_RX and CAN1_TX. */
    GPIOB->AFR[1] &= ~((0xFU << ((8U - 8U) * 4U)) | (0xFU << ((9U - 8U) * 4U)));
    /* Apply the alternate-function 9 selection to the relevant AFR bits. */
    GPIOB->AFR[1] |= ((0x9U << ((8U - 8U) * 4U)) | (0x9U << ((9U - 8U) * 4U)));

    /* Enable and reset the CAN1 peripheral so its state is known. */
    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    (void)RCC->APB1ENR;
    RCC->APB1RSTR |= RCC_APB1RSTR_CAN1RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_CAN1RST;

    /* Wake bxCAN before requesting initialization mode. */
    CAN1->MCR &= ~CAN_MCR_SLEEP;
    while ((CAN1->MSR & CAN_MSR_SLAK) != 0U) {
        /* Wait until the peripheral acknowledges wake-up. */
        ;
    }

    /* Request initialization mode for CAN1 before reprogramming its timing and filters. */
    CAN1->MCR |= CAN_MCR_INRQ;
    /* Wait until the initialization acknowledge bit shows that CAN1 has entered init mode. */
    while ((CAN1->MSR & CAN_MSR_INAK) == 0U) {
        /* Spin until initialization mode is acknowledged by the peripheral. */
        ;
    }

    /*
     * APB1 is 16 MHz after reset.  bxCAN stores each timing value minus one:
     * BRP field 1 = prescaler 2, TS1 field 12 = 13 TQ, TS2 field 1 = 2 TQ.
     * 16 MHz / (2 * (1 + 13 + 2)) = 500 kbit/s, sample point = 87.5%.
     */
    CAN1->BTR = (1U << CAN_BTR_BRP_Pos) |
                /* Program the time-segment 1 field for the selected bit timing. */
                (12U << CAN_BTR_TS1_Pos) |
                /* Program the time-segment 2 field for the selected bit timing. */
                (1U << CAN_BTR_TS2_Pos);
    /* Program the CAN receive filter bank so standard frames are accepted into FIFO0. */
    prvCanConfigureRxFilter();
    /*
     * Send only one frame attempt per task period.  An analyzer is not a CAN
     * receiver and cannot ACK; disabling automatic retransmission prevents a
     * missing ACK from turning one scheduled frame into repeated traffic.
     */
    CAN1->MCR |= CAN_MCR_NART | CAN_MCR_ABOM | CAN_MCR_TXFP;

    /* Exit initialization mode so the CAN peripheral can start participating on the bus. */
    CAN1->MCR &= ~CAN_MCR_INRQ;
    /* Wait until CAN1 leaves initialization mode and becomes active. */
    while ((CAN1->MSR & CAN_MSR_INAK) != 0U) {
        /* Spin until the peripheral has left initialization mode. */
        ;
    }

    /* Interrupt whenever FIFO0 receives a frame accepted by filter bank 0. */
    CAN1->IER |= CAN_IER_FMPIE0;
    /* The handler only copies data and does not call FreeRTOS APIs. */
    NVIC_SetPriority(CAN1_RX0_IRQn, 6U);
    NVIC_EnableIRQ(CAN1_RX0_IRQn);
}

/**
 *@brief Main entry point
 **/
/* Define the firmware entry point that starts the scheduler and the tasks. */
int main(void) {
    /* Initialize the LED pins and the CAN1 peripheral before creating tasks. */
    prvSetupHardware();

    /* Create separate blink tasks */
    /* Create the 100 ms LED blink task so PD12 toggles at a fast interval. */
    xTaskCreate(BlinkTask_100ms,  "Blink_100ms",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    /* Create the 500 ms LED blink task so PD13 toggles at a medium interval. */
    xTaskCreate(BlinkTask_500ms,  "Blink_500ms",  configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    /* Create the 1000 ms LED blink task so PD14 toggles at a slow interval. */
    xTaskCreate(BlinkTask_1000ms, "Blink_1000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    /* Create the 2000 ms LED blink task so PD15 toggles at a very slow interval. */
    xTaskCreate(BlinkTask_2000ms, "Blink_2000ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    /* Create the periodic CAN transmit task that sends a sample frame every 250 ms. */
    xTaskCreate(CanTxTask_250ms, "CanTx_250ms", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    /* Start scheduler */
    /* Start the FreeRTOS scheduler so the created tasks begin running. */
    vTaskStartScheduler();

    /* Enter a forever loop if the scheduler unexpectedly returns. */
    for(;;) { } /* should never reach here */
}
