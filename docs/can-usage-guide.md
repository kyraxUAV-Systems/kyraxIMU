# STM32F407 CAN1 quick reference

The complete training guide is [can-guide.html](can-guide.html). It explains the hardware, clocks, GPIO, timing, transmit mailboxes, hardware filtering, FIFO0 receive interrupts, storage, testing, tools, and troubleshooting.

## Current firmware behavior

- CAN1 uses PB8 as CAN1_RX and PB9 as CAN1_TX, both alternate function 9.
- APB1 is assumed to be 16 MHz. CAN timing is 500 kbit/s with an 87.5% sample point.
- A standard data frame with ID `0x123` and eight bytes is attempted every 250 ms.
- Filter bank 0 accepts only standard **data** frames with ID `0x321` and routes them to FIFO0.
- `CAN1_RX0_IRQHandler` stores the newest accepted frame in `g_can1_rx_latest`.
- Call `Can1CopyLatestRxFrame()` from `inc/can1.h` to obtain a coherent copy in application code.

## Test the receive path

```sh
python3 -m pip install python-can
python3 canalyzer_tool/read_can.py --id 0x123
python3 canalyzer_tool/send_can.py --id 0x321 --data "DE AD BE EF 01 02 03 04"
```

Set the adapter channel and interface to match your USB-CAN device. Both nodes must use 500 kbit/s, have an active transceiver, share ground, and be connected to a correctly terminated CANH/CANL bus.
