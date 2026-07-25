#ifndef CAN1_H
#define CAN1_H

#include <stdint.h>

#define CAN1_MAX_DLC 8U

/* Snapshot of the most recently accepted CAN1 data frame. */
typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[CAN1_MAX_DLC];
    uint32_t count;
} Can1RxFrame;

/* Updated by CAN1_RX0_IRQHandler when a frame passes the hardware filter. */
extern volatile Can1RxFrame g_can1_rx_latest;

/* Copy a coherent received-frame snapshot. Returns -1 before the first frame. */
int32_t Can1CopyLatestRxFrame(Can1RxFrame *destination);

#endif /* CAN1_H */
