#include "lvgl.h"
#include "sharp_mip.h"
#include "driver/gpio.h"
#include "esp_freertos_hooks.h"

#define DRAW_BUF_SIZE (SHARP_MIP_VER_RES * (SHARP_MIP_HOR_RES / 8))
#define CANVAS_WIDTH  400
#define CANVAS_HEIGHT 240

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

static uint8_t backbuf[DRAW_BUF_SIZE];
static uint8_t frontbuf[DRAW_BUF_SIZE];

void
run_tick() {
  // Increment LVGL tick by FreeRTOS tick period (usually 1ms)
  lv_tick_inc(portTICK_PERIOD_MS);
}

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
  lv_display_set_flush_cb(disp, sharp_mip_flush);

  lv_display_set_buffers(disp, backbuf, frontbuf, sizeof(backbuf), LV_DISPLAY_RENDER_MODE_PARTIAL);

  LV_DRAW_BUF_DEFINE_STATIC(draw_buf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_COLOR_FORMAT_I1);
  LV_DRAW_BUF_INIT_STATIC(draw_buf);

  lv_obj_t * label1 = lv_label_create(lv_screen_active());
  lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
  lv_label_set_text(label1, "#0000ff Re-color# #ff00ff words# #ff0000 of a# label, align the lines to the center "
                    "and wrap long text automatically.");
  lv_obj_set_width(label1, 150);  /*Set smaller width to make the lines wrap*/
  lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(label1, LV_ALIGN_CENTER, 0, -40);

  lv_obj_t * label2 = lv_label_create(lv_screen_active());
  lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/
  lv_obj_set_width(label2, 150);
  lv_label_set_text(label2, "It is a circularly scrolling text. ");
  lv_obj_align(label2, LV_ALIGN_CENTER, 0, 40);

  esp_err_t tick_ok = esp_register_freertos_tick_hook(run_tick);

  while (1) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(16));  // Call handler every 10ms
    static int count = 0;
    if (++count >= 100) { // Run inversion every 1s
        sharp_mip_com_inversion();
        count = 0;
    }
  }
}
