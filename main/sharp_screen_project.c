#include "lvgl.h"
#include "sharp_mip.h"
#include "driver/gpio.h"

#define DRAW_BUF_SIZE (SHARP_MIP_VER_RES * (SHARP_MIP_HOR_RES / 8))

esp_err_t
sharp_mip_spi_init() {
  esp_err_t ret;

  spi_bus_config_t buscfg = {
      .miso_io_num = -1,
      .mosi_io_num = 23,
      .sclk_io_num = 18,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 84000
  };

  spi_device_interface_config_t devcfg = {
      .clock_speed_hz = 1000000, // Adjust this to your needs
      .mode = 0,                 // SPI mode
      .spics_io_num = 5, // Chip select pin
      .queue_size = 7,           // Number of transactions
      .pre_cb = NULL,            // Callback function if needed
  };

  ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK) {
      return ret;
  }

  ret = spi_bus_add_device(SPI2_HOST, &devcfg, &sharp_spi_handle);
  if (ret != ESP_OK) {
      return ret;
  }

  return ESP_OK;
}

static uint8_t buf[DRAW_BUF_SIZE];

void app_main(void) {
  lv_init();
  static lv_display_t *disp;
  esp_err_t ret = sharp_mip_spi_init();

  if (ret != ESP_OK) {
      printf("SPI initialization failed!");
      return;
  }

    // Configure the GPIO pin
  gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << SHARP_CS_GPIO),
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
  };
  gpio_config(&io_conf);

  disp = lv_display_create(SHARP_MIP_HOR_RES, SHARP_MIP_VER_RES);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_I1);
  lv_display_add_event_cb(disp, sharp_mip_rounder, LV_EVENT_INVALIDATE_AREA, disp);
  lv_display_set_flush_cb(disp, sharp_mip_flush);  // your flush function

  lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_obj_t *scr = lv_screen_active();

  lv_obj_t *rect = lv_obj_create(scr);

  lv_obj_set_size(rect, 80, 80);
  lv_obj_set_pos(rect, 80, 130);

  lv_obj_set_style_bg_color(rect, lv_color_black(), 0);
  lv_obj_set_style_border_color(rect, lv_color_white(), 0);
  lv_obj_set_style_border_width(rect, 2, 0);

  lv_obj_t *label = lv_label_create(scr);

  lv_label_set_text(label, "Hello, world!");
  lv_obj_center(label);

  lv_obj_t *shaft = lv_obj_create(scr);
  lv_obj_set_size(shaft, 20, 80); // Narrow and tall
  lv_obj_set_pos(shaft, 100, 30);
  lv_obj_set_style_radius(shaft, LV_RADIUS_CIRCLE, 0); // Fully rounded corners
  lv_obj_set_style_bg_opa(shaft, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(shaft, lv_color_black(), 0);

  lv_obj_t *cover = lv_obj_create(scr);
  lv_obj_set_size(cover, 20, 20);
  lv_obj_set_pos(cover, 100, 90);
  lv_obj_set_style_bg_opa(cover, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(cover, lv_color_white(), 0);  // "erase" bottom rounding
  lv_obj_set_style_border_width(cover, 0, 0);
  lv_obj_clear_flag(cover, LV_OBJ_FLAG_CLICKABLE); // no interaction

  lv_obj_t *left_circle = lv_obj_create(scr);
  lv_obj_set_size(left_circle, 20, 20);
  lv_obj_set_pos(left_circle, 85, 90);
  lv_obj_set_style_radius(left_circle, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(left_circle, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(left_circle, lv_color_black(), 0);

  lv_obj_t *right_circle = lv_obj_create(scr);
  lv_obj_set_size(right_circle, 20, 20);
  lv_obj_set_pos(right_circle, 115, 90);
  lv_obj_set_style_radius(right_circle, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(right_circle, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(right_circle, lv_color_black(), 0);

  while (1) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(10));
    sharp_mip_com_inversion();
  }
}

