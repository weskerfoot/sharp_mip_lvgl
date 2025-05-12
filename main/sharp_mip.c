#include "sharp_mip.h"
#include <stdbool.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "string.h"
#include "driver/spi_master.h"

// Declare the SPI handle globally
spi_device_handle_t sharp_spi_handle;

#define SHARP_MIP_HEADER              0
#define SHARP_MIP_UPDATE_RAM_FLAG     (1 << 7)
#define SHARP_MIP_COM_INVERSION_FLAG  (1 << 6)
#define SHARP_MIP_CLEAR_SCREEN_FLAG   (1 << 5)

static bool com_output_state = false;

// Accounts for 2 header bytes at the beginning
// 2 bytes for line address and dummy byte per line
// horizontal size is divided by 8 since there are 8 pixels per byte
#define DISP_BUF_SIZE ((SHARP_MIP_VER_RES) * (2 + (SHARP_MIP_HOR_RES / 8)) + 2)
#define BUFIDX(x, y)  ((((x) >> 3) + ((y) * (2 + (SHARP_MIP_HOR_RES >> 3))) + 2))

static uint8_t disp_buf[DISP_BUF_SIZE];

static void
print_byte(uint8_t b) {
  for (int i = 7; i >= 0; i--) {
    printf("%d", b >> i & 1);
  }
}

static void
print_buffer(uint8_t *buf, int x_size, int y_size) {
  for (int y = 0; y < y_size; y++) {
    for (int x = 0; x < x_size; x++) {
      print_byte(buf[x + (y * y_size)]);
    }
    printf("\n");
  }
}

static void
sharp_mip_spi_send(const uint8_t *data, size_t length) {
  if (sharp_spi_handle == NULL) {
      printf("Error: SPI handle not initialized\n");
      return;
  }

  spi_transaction_t t = {
      .length = 8 * length,  // Number of bits to send
      .tx_buffer = data,     // Buffer to send
  };

  esp_err_t ret = spi_device_transmit(sharp_spi_handle, &t);
  if (ret != ESP_OK) {
      printf("Error: SPI transmission failed\n");
  }
}

void sharp_mip_init(void) {
  /* No hardware init needed for SHARP MIP displays */
}

static inline uint8_t
sharp_mip_reverse_byte(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

void
sharp_mip_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (area->y2 < 0 || area->y1 >= SHARP_MIP_VER_RES) {
      printf("Not flushing anything\n");
      lv_disp_flush_ready(disp);
      return;
    }

    px_map += 8; // skip palette

    uint16_t active_y1 = area->y1 < 0 ? 0 : area->y1;
    uint16_t active_y2 = area->y2 >= SHARP_MIP_VER_RES ? SHARP_MIP_VER_RES - 1 : area->y2;

    uint16_t buf_h = active_y2 - active_y1 + 1;
    // uint16_t buf_size = buf_h * (SHARP_MIP_HOR_RES / 8);
    // lv_draw_sw_i1_invert(px_map, buf_size); // not needed?

    for (uint16_t active_y = 0; active_y < buf_h; active_y++) {
      // Copy over pixel data from draw buffer to display buffer
      memcpy(&disp_buf[BUFIDX(0, active_y)], &px_map[active_y * (SHARP_MIP_HOR_RES / 8)], SHARP_MIP_HOR_RES / 8);

      // Set gate addresses and dummy bytes
      disp_buf[BUFIDX(0, active_y) - 1] = sharp_mip_reverse_byte(active_y1 + active_y - 1);
      disp_buf[BUFIDX(0, active_y) - 2] = 0;
    }

    disp_buf[BUFIDX(0, buf_h) - 1] = 0;
    disp_buf[BUFIDX(0, buf_h) - 2] = 0;
    disp_buf[0] = SHARP_MIP_HEADER | SHARP_MIP_UPDATE_RAM_FLAG;

    LV_DRV_DISP_SPI_CS(1);
    sharp_mip_spi_send(disp_buf, DISP_BUF_SIZE);
    LV_DRV_DISP_SPI_CS(0);

    lv_disp_flush_ready(disp);
    sharp_mip_com_inversion();
    sharp_mip_com_inversion();
}

void
sharp_mip_rounder(lv_event_t *e) {
  //printf("Rounder cb being called\n");
  lv_area_t * area = lv_event_get_param(e);
  area->x1 = 0;
  area->x2 = SHARP_MIP_HOR_RES - 1;
}

void sharp_mip_com_inversion(void) {
  uint8_t inversion_header[2] = {0};

  if (com_output_state) {
    com_output_state = false;
  }
  else {
    inversion_header[0] |= SHARP_MIP_COM_INVERSION_FLAG;
    com_output_state = true;
  }

  LV_DRV_DISP_SPI_CS(1);
  sharp_mip_spi_send(inversion_header, 2);
  LV_DRV_DISP_SPI_CS(0);
}
