
#include "logger.h"

int app_state = 0;

void application_process(void)
{
  LOG_INFO(LOG_MODULE_APP, "Application started");

    if (app_state)
    {
    LOG_ERROR(LOG_MODULE_APP, "Something went wrong");
    }
}

void application_init(void)
{
  /* USER CODE BEGIN application_init */
  /* add your application code here */
  /* USER CODE END application_init */
}   