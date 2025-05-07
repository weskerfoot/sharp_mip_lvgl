#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lvgl.h"

#define SHARP_MIP_HEADER              0
#define SHARP_MIP_UPDATE_RAM_FLAG     (1 << 7)
#define SHARP_CS_GPIO 5

#ifndef USE_SHARP_MIP
#define USE_SHARP_MIP 1
#endif

#ifndef SHARP_MIP_SOFT_COM_INVERSION
#define SHARP_MIP_SOFT_COM_INVERSION 1
#endif

// Set your resolution here
#define SHARP_MIP_HOR_RES 400
#define SHARP_MIP_VER_RES 240
#define LV_DRV_DISP_SPI_CS(x) gpio_set_level(SHARP_CS_GPIO, (x) ? 1 : 0)

extern spi_device_handle_t sharp_spi_handle;

void sharp_mip_init(void);
void sharp_mip_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p);
void sharp_mip_set_px(lv_display_t * disp, uint8_t * buf, lv_coord_t buf_w,
                      lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);
void sharp_mip_rounder(lv_event_t * e);

void sharp_mip_com_inversion(void);
