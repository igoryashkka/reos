/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "debug_tools.h"
#include "logger.h"
#include "os_main.h"
#include "os_task.h"
#include "main.h"
#include "cmsis_os2.h"


static void pre_config_rtos(void) {
  logger_init();
  LOG_INFO(LOG_MODULE_SYSTEM, "logger ready");

  os_task_init_all();
  LOG_INFO(LOG_MODULE_SYSTEM, "tasks created");

  /* initialization os kernal */
  osKernelInitialize();
  LOG_INFO(LOG_MODULE_SYSTEM, "cmsis kernel initialized");
}

static void scheduler_rtos_start(void) {
  LOG_INFO(LOG_MODULE_SYSTEM, "starting scheduler");
  /* Start scheduler */
  osKernelStart();
}



void os_main(void) {
  /* initialization configuration */
  pre_config_rtos();
	/* Start scheduler */
	scheduler_rtos_start();
	/* Should never get here*/
  return;
}

void vApplicationMallocFailedHook(void)
{
  debug_panic(DEBUG_PANIC_MALLOC_FAILED, __FILE__, __LINE__, 0, 0);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  debug_panic(DEBUG_PANIC_STACK_OVERFLOW, __FILE__, __LINE__, pcTaskName, xTask);
}