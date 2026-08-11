/* Тестова заміна rp_hw_if.c: реальні backends/rp_hw_uart.c та
 * backends/rp_hw_radio.c тягнуть STM32 HAL і на хості не збираються.
 * Захоплює останній надісланий кадр для перевірки у тестах. */
#include "rp_hw_if.h"

#include <string.h>

uint8_t g_mock_last_buf[512]; /* > RP_FRAME_MAX (rp_proto.h), avoids pulling that header in here */
size_t  g_mock_last_len;
int     g_mock_send_count;

void rp_hw_if_init_all(void) {}

int rp_hw_if_select(rp_hw_if_id_t id)
{
    (void)id;
    return 0;
}

int rp_hw_if_send(const uint8_t *data, size_t length)
{
    memcpy(g_mock_last_buf, data, length);
    g_mock_last_len = length;
    g_mock_send_count++;

    return (int)length;
}
