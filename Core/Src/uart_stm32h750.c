#include "uart.h"

#include "config_pin.h"
#include "main.h"

#define LOGGER_UART_INSTANCE              BSP_LOG_UART_INSTANCE
#define LOGGER_UART_BAUDRATE              115200U

static int logger_uart_initialized;

static void logger_uart_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio_init.Pin = BSP_LOG_UART_GPIO_PIN;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = BSP_LOG_UART_GPIO_AF;

    HAL_GPIO_Init(BSP_LOG_UART_GPIO_PORT, &gpio_init);
}

static uint32_t logger_uart_get_clock_hz(void)
{
    RCC_PeriphCLKInitTypeDef periph_clk_init = {0};

    periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    periph_clk_init.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&periph_clk_init) != HAL_OK)
    {
        return 0U;
    }

    __HAL_RCC_USART3_CLK_ENABLE();

    return HAL_RCC_GetPCLK1Freq();
}

int uart_init(void)
{
    uint32_t uart_clock_hz;

    if (logger_uart_initialized)
    {
        return 0;
    }

    uart_clock_hz = logger_uart_get_clock_hz();

    if (uart_clock_hz == 0U)
    {
        return -1;
    }

    logger_uart_gpio_init();

    CLEAR_BIT(LOGGER_UART_INSTANCE->CR1, USART_CR1_UE);
    LOGGER_UART_INSTANCE->CR1 = 0U;
    LOGGER_UART_INSTANCE->CR2 = 0U;
    LOGGER_UART_INSTANCE->CR3 = 0U;
    LOGGER_UART_INSTANCE->PRESC = 0U;
    LOGGER_UART_INSTANCE->BRR = uart_clock_hz / LOGGER_UART_BAUDRATE;
    SET_BIT(LOGGER_UART_INSTANCE->CR1, USART_CR1_TE);
    SET_BIT(LOGGER_UART_INSTANCE->CR1, USART_CR1_UE);

    logger_uart_initialized = 1;

    return 0;
}

int uart_write(const uint8_t *data, size_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return 0;
    }

    if ((logger_uart_initialized == 0) && (uart_init() != 0))
    {
        return -1;
    }

    for (size_t index = 0; index < length; index++)
    {
        while ((LOGGER_UART_INSTANCE->ISR & USART_ISR_TXE_TXFNF) == 0U)
        {
        }

        LOGGER_UART_INSTANCE->TDR = data[index];
    }

    return (int)length;
}

int uart_flush(void)
{
    if (logger_uart_initialized == 0)
    {
        return 0;
    }

    while ((LOGGER_UART_INSTANCE->ISR & USART_ISR_TC) == 0U)
    {
    }

    return 0;
}