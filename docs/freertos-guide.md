# FreeRTOS guide for this STM32 project

This project already uses FreeRTOS in [src/main.c](../src/main.c) and the kernel configuration is in [thirdparty/FreeRTOS-Kernel/include/FreeRTOSConfig.h](../thirdparty/FreeRTOS-Kernel/include/FreeRTOSConfig.h). The examples below show the common patterns you asked for:

- how to make and register tasks
- how to schedule tasks
- how to use semaphores and mutexes
- how to make tasks run concurrently
- how to make tasks run in a controlled sequence

## 1. How tasks are created

A FreeRTOS task is a C function that runs forever in a loop. You register it with `xTaskCreate()`.

```c
#include "FreeRTOS.h"
#include "task.h"

static void MyTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        /* task work here */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void StartTasks(void)
{
    xTaskCreate(MyTask,
                "MyTask",
                configMINIMAL_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY + 1,
                NULL);

    vTaskStartScheduler();
}
```

### Important parameters

- `MyTask`: the task function
- `"MyTask"`: the task name shown in debugging tools
- `configMINIMAL_STACK_SIZE`: stack size in words
- `NULL`: optional parameter passed to the task
- `tskIDLE_PRIORITY + 1`: task priority
- `NULL`: optional handle to store the task handle

In this repository, the same pattern is already used in [src/main.c](../src/main.c) for the LED blink tasks.

---

## 2. How scheduling works

FreeRTOS schedules tasks based on:

- task priority
- ready state
- time delays
- blocking waits

### Simple periodic delay

Use `vTaskDelay()` when you want a task to wait for a fixed amount of time.

```c
static void PeriodicTask(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        /* do work */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### More accurate periodic timing

Use `vTaskDelayUntil()` when you want the task to run at a fixed cadence, such as every 100 ms.

```c
static void AccuratePeriodicTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(100);

    for (;;) {
        /* do work */
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
```

### Priority rule of thumb

- lower number = lower priority
- higher number = higher priority
- use small differences at first

A common starting point is:

```c
tskIDLE_PRIORITY + 1
tskIDLE_PRIORITY + 2
```

---

## 3. How tasks run concurrently

Concurrency in FreeRTOS means multiple tasks appear to run at the same time because the kernel switches between them. This is preemptive multitasking.

### Example: two tasks doing different work

```c
static void TaskA(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        /* do A work */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void TaskB(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        /* do B work */
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void StartConcurrentTasks(void)
{
    xTaskCreate(TaskA, "TaskA", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(TaskB, "TaskB", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();
}
```

### Why this works

The scheduler runs one task, then switches to the next when:

- the current task blocks
- its time slice ends
- a higher-priority task becomes ready

---

## 4. How to use semaphores

Use semaphores to signal events between tasks.

### Binary semaphore

Use a binary semaphore when one task needs to notify another that something happened.

```c
#include "semphr.h"

static SemaphoreHandle_t xEventSem = NULL;

static void ProducerTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        /* do work */
        xSemaphoreGive(xEventSem);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void ConsumerTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        xSemaphoreTake(xEventSem, portMAX_DELAY);
        /* handle the event */
    }
}
```

### Counting semaphore

Use a counting semaphore when multiple instances of a resource or event can be available.

```c
xEventSem = xSemaphoreCreateCounting(4, 0);
```

### Important note

- `xSemaphoreGive()` signals
- `xSemaphoreTake()` waits for the signal

---

## 5. How to use mutexes

Use a mutex when two or more tasks need to safely access the same shared resource, such as a shared buffer, peripheral, or global variable.

```c
#include "semphr.h"

static SemaphoreHandle_t xSpiMutex = NULL;

static void Task1(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        xSemaphoreTake(xSpiMutex, portMAX_DELAY);
        /* access shared resource safely */
        xSemaphoreGive(xSpiMutex);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void Task2(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        xSemaphoreTake(xSpiMutex, portMAX_DELAY);
        /* access shared resource safely */
        xSemaphoreGive(xSpiMutex);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void CreateMutexExample(void)
{
    xSpiMutex = xSemaphoreCreateMutex();

    xTaskCreate(Task1, "Task1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(Task2, "Task2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);
}
```

### Mutex rule

A mutex should be used for exclusive access. If multiple tasks need to read and write the same data, protect the critical section with a mutex.

---

## 6. How to make tasks run in a sequence

If you want task B to run only after task A finishes, do not let both tasks run freely. Instead, use a signal or queue to pass control from one task to the next.

### Sequence using a binary semaphore

```c
#include "semphr.h"

static SemaphoreHandle_t xStep1Done = NULL;
static SemaphoreHandle_t xStep2Done = NULL;

static void TaskStep1(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        /* do step 1 work */
        xSemaphoreGive(xStep1Done);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void TaskStep2(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        xSemaphoreTake(xStep1Done, portMAX_DELAY);
        /* do step 2 work */
        xSemaphoreGive(xStep2Done);
    }
}

static void TaskStep3(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
        xSemaphoreTake(xStep2Done, portMAX_DELAY);
        /* do step 3 work */
    }
}
```

This creates a pipeline:

1. TaskStep1 finishes its work
2. it signals TaskStep2
3. TaskStep2 runs and signals TaskStep3

### Better pattern for strict sequencing

If you want a very explicit flow, use a queue with an enum or message ID:

```c
typedef enum {
    STEP_START = 0,
    STEP_NEXT = 1,
    STEP_DONE = 2
} StepMessage;
```

Then send the next step command through the queue.

---

## 7. A practical example for this project

If you want to add a new periodic task to this project, start from the pattern already used in [src/main.c](../src/main.c):

```c
static void MyNewTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(200);

    for (;;) {
        /* your work */
        vTaskDelayUntil(&lastWakeTime, period);
    }
}
```

Then register it in `main()`:

```c
xTaskCreate(MyNewTask, "MyNewTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
```

---

## 8. Common mistakes

- forgetting to call `vTaskStartScheduler()`
- using too small a stack size
- blocking a high-priority task forever
- using a mutex for simple one-way signaling instead of a semaphore
- sharing data without protection
- assuming delay-based timing is perfectly accurate

### Good practice

- keep each task focused on one job
- make tasks wait instead of spinning in tight loops
- use mutexes for shared resources
- use semaphores for events and handoffs
- use `vTaskDelayUntil()` for periodic tasks that need accurate timing

---

## 9. Quick decision guide

- Use `xTaskCreate()` to create a task.
- Use `vTaskDelay()` or `vTaskDelayUntil()` to schedule it.
- Use a semaphore for signaling.
- Use a mutex for protecting shared data.
- Use a queue or notification for strict task-to-task sequencing.

If you want, the next step can be to add a concrete example directly into [src/main.c](../src/main.c) for one of these patterns:

1. a blinking task plus a sensor-reading task
2. a producer/consumer task pair
3. a three-step sequenced task chain
