# STM32F407 CAN1 transmit/receive guide

This document describes the CAN1 implementation that was added to the STM32F407 firmware in this project. The implementation is written in register-level style and is intended to work with the existing FreeRTOS-based firmware skeleton.

## What was implemented

The firmware now provides a basic bxCAN setup for the STM32F407 using CAN1 on the following pins:

- PB8 → CAN1_RX
- PB9 → CAN1_TX

The implementation includes:

1. Hardware setup for CAN1
   - enables the GPIOB and CAN1 clocks
   - configures PB8/PB9 as alternate-function pins for CAN1
   - configures the CAN peripheral for a nominal 500 kbit/s baud rate
   - enables the receive filter path so incoming frames can be accepted

2. CAN transmit support
   - sends a standard CAN frame using mailbox 0
   - uses a small helper function to load the payload into the transmit registers
   - runs from a FreeRTOS task so transmission is scheduled in a deterministic way

3. CAN receive support
   - uses filter bank 0 to accept incoming standard frames into FIFO0
   - polls FIFO0 from a FreeRTOS task
   - stores the latest received frame ID, DLC, data bytes, and receive count

4. FreeRTOS integration
   - the transmit and receive logic are executed from tasks rather than blocking the main loop
   - the scheduler handles task timing and cooperative execution

## Hardware requirements

The STM32F407 CAN peripheral must be connected to a CAN transceiver. The MCU pins alone are not sufficient to drive the bus.

### Required hardware

- STM32F407 board with CAN1 available on PB8/PB9
- A CAN transceiver such as:
  - MCP2551
  - SN65HVD230
  - TJA1050
- A CAN bus with two nodes or a USB-CAN adapter
- Optional: a 120 Ω termination resistor across CANH and CANL on one or both ends of the bus

### Example wiring

Typical connection:

- STM32F407 PB8 → transceiver RX pin
- STM32F407 PB9 → transceiver TX pin
- transceiver VDD/VSS to the board power rails
- transceiver CANH → CAN bus CANH
- transceiver CANL → CAN bus CANL
- GND shared between MCU and transceiver

If you are connecting to a USB-CAN adapter, follow that adapter’s wiring instructions carefully, because some adapters expect 3.3 V logic while others expect 5 V levels.

## CAN configuration details

The default configuration in this implementation is:

- CAN peripheral: CAN1
- Frame type: standard identifier (11-bit ID)
- Default transmit ID: 0x123
- Default payload size: 8 bytes
- Bit timing assumption: 16 MHz system clock, 500 kbit/s

The important timing values are configured in the CAN1 bit timing register:

- BRP = 1
- TS1 = 12
- TS2 = 2

This is a practical starting point for a 500 kbit/s bus when the STM32F407 is running from a 16 MHz clock domain in the project’s current setup.

If your clock configuration differs, you may need to recalculate the bit timing. The most common values to adjust are:

- `CAN_BTR_BRP_Pos`
- `CAN_BTR_TS1_Pos`
- `CAN_BTR_TS2_Pos`

## How to use the transmit functionality

The transport layer is implemented through a helper function named `prvCanTransmitStdFrame()` in the firmware source.

### What the transmit helper does

The helper:

1. checks that the payload pointer is valid
2. checks that the DLC does not exceed 8 bytes
3. waits until transmit mailbox 0 is free
4. writes data into the transmit data registers
5. triggers the transmit request by setting the mailbox request bit

### How to send a CAN frame

The frame is transmitted from a FreeRTOS task named `CanTxTask_250ms`. That task periodically sends a sample frame every 250 ms.

You can modify the transmit behavior in the task body:

- change the frame ID by editing `CAN_STD_ID`
- change the payload bytes in the `payload[]` array
- change the transmit period by modifying the `xPeriod` value

Example of the relevant transmit task behavior:

```c
static void CanTxTask_250ms(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(250);
    uint8_t payload[CAN_DLC] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80};

    for(;;) {
        (void)prvCanTransmitStdFrame(CAN_STD_ID, payload, CAN_DLC);
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}
```

### Notes on sending frames

- Keep the transmit operation short and deterministic.
- Avoid writing large amounts of application logic directly inside the transmit task.
- If you need to send frames from multiple parts of the firmware, use a queue or a small message buffer so that the CAN task is the only place that touches the bxCAN mailbox registers.

## How to use the receive functionality

The receive side is implemented by a task named `CanRxTask_100ms`.

### What the receive task does

On each loop iteration, the task checks FIFO0 for pending frames. If a frame is available:

1. it reads the mailbox contents from the FIFO buffer
2. extracts the frame ID, DLC, and payload bytes
3. stores the values in the global variables:
   - `g_can_rx_id`
   - `g_can_rx_dlc`
   - `g_can_rx_data[]`
   - `g_can_rx_count`
4. releases the FIFO output mailbox so the next frame can be processed

### How to inspect received data

The latest received frame is available through these variables:

```c
extern volatile uint32_t g_can_rx_id;
extern volatile uint8_t g_can_rx_dlc;
extern volatile uint8_t g_can_rx_data[8];
extern volatile uint32_t g_can_rx_count;
```

You can inspect them inside other tasks or from a debug session.

### Receiving frames from another node

To test the receive path:

1. flash the updated firmware onto the STM32F407 board
2. connect the board to another CAN node or a USB-CAN adapter
3. send a frame to the STM32
4. observe the receive task processing the frame and updating the global RX variables

The current filter configuration accepts standard frames into FIFO0. If you want to filter by a specific ID, you should adjust the filter registers in `prvCanConfigureRxFilter()`.

## FreeRTOS integration overview

The CAN functionality is intentionally split into tasks so it fits naturally inside the FreeRTOS execution model.

### Why tasks are used

Using tasks gives you several benefits:

- the main thread stays lightweight and does not become blocked by CAN I/O
- CAN transmit and receive are handled on a periodic schedule
- the scheduler can preempt and switch tasks when needed
- it is easier to keep hardware access deterministic and isolated

### Task structure in this firmware

The firmware currently has:

- blinking tasks for the LEDs
- a CAN transmit task
- a CAN receive task

The CAN tasks are launched in `main()` with `xTaskCreate()`.

### Scheduling recommendations

The scheduling of CAN functionality should follow a few simple rules:

1. Keep the CAN transmit task periodic and short
   - do not put long processing inside the transmit task
   - if a message takes time to prepare, queue it and let the task send it quickly

2. Keep the receive task periodic but not too aggressive
   - polling FIFO0 every 100 ms is a practical compromise for simple demos
   - if you need faster reaction time, reduce the period, but keep in mind that too-frequent polling can consume CPU time

3. Avoid blocking the scheduler unnecessarily
   - busy-wait loops should be kept to a minimum
   - waiting for transmit mailbox availability should be bounded by a small timeout, if you plan to build production-grade code

4. Prefer task-to-task communication
   - use queues or ring buffers if the CAN frame data should be consumed by other tasks
   - this keeps the hardware driver logic isolated from application logic

## Recommended structure for a more robust design

For a production-quality design, the following structure is better than directly accessing the CAN registers from multiple tasks:

1. Create a CAN driver layer
   - encapsulates the register access and hardware configuration
   - provides functions like `can_send_frame()` and `can_receive_frame()`

2. Create a transmit task
   - waits for messages from a queue
   - sends them to the CAN peripheral

3. Create a receive task
   - polls or handles interrupts from the CAN peripheral
   - places received frames into a queue for higher-level tasks

4. Use application tasks for processing
   - sensor tasks, state machines, protocol handlers, or logging tasks consume the queued CAN data

This approach makes the firmware easier to test and maintain.

## How to tune or extend the implementation

### Change the baud rate

If you want a different bus speed, change the bit timing configuration in `prvSetupHardware()`.

### Change the frame ID

Edit `CAN_STD_ID` and the transmit helper call.

### Change the payload

Edit the `payload[]` array in `CanTxTask_250ms()`.

### Change the receive filter

Modify `prvCanConfigureRxFilter()` if you want the peripheral to accept only certain frames.

### Switch from polling to interrupts

This code uses polling from FreeRTOS tasks. If you want lower CPU usage and faster responsiveness, you can move to interrupt-based reception. In that case:

- enable the receive FIFO not-empty interrupt in the CAN interrupt mask register
- implement the CAN interrupt handler
- place a received frame into a queue from the ISR or a deferred task context

## Current limitations

The implementation is intentionally minimal and educational. It demonstrates functional CAN transmit and receive support for the STM32F407 in the context of this FreeRTOS firmware, but it is not yet a full production driver.

The current limitations include:

- no extended-frame support yet
- no interrupt-based receive handling yet
- no robust timeout handling around mailbox availability
- no full filtering by ID mask yet
- no queue-based abstraction between CAN hardware and application logic

## Build and flash steps

Build the firmware with:

```sh
make clean && make
```

Flash it with:

```sh
make flash
```

The generated firmware image will be written to the `debug/` folder.

## Summary

This implementation provides the minimum practical building blocks for using CAN1 on the STM32F407 in a FreeRTOS firmware:

- hardware setup for CAN1 on PB8/PB9
- frame transmission from a task
- frame reception from FIFO0 in a task
- simple state variables for inspecting recent receive activity

That is enough to start experimenting with CAN communication on the board and to build a more complete driver around it later.
