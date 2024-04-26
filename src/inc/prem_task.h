#ifndef __PREM_TASK_H__
#define __PREM_TASK_H__

#include <FreeRTOS.h>
#include <task.h>

struct premtask_parameters
{
    TickType_t tickPeriod;
    uint64_t data_size;
    void *data;
    void *pvParameters;
};

// TODO Make documentation
BaseType_t xTaskPREMCreate(TaskFunction_t pxTaskCode,
                           const char *const pcName,
                           const configSTACK_DEPTH_TYPE uxStackDepth,
                           const struct premtask_parameters premtask_parameters,
                           UBaseType_t uxPriority,
                           TaskHandle_t *const pxCreatedTask);

#endif