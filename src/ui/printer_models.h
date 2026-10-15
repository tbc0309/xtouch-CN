#pragma once
#include <lvgl.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void ui_printer_image_decoder_init(void);
const char *ui_printer_model_short_name(const char *value);
const char *ui_printer_model_code(const char *value);
const lv_img_dsc_t *ui_printer_model_image(const char *value);
bool ui_printer_model_chamber_heater(const char *value);
#ifdef __cplusplus
}
#endif
