#pragma once
#include <lvgl.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Rasterize at the requested size without a second font or an offscreen layer. */
void ui_text_scale(lv_obj_t *label, const lv_font_t *source, unsigned zoom);
const lv_font_t *ui_text_source(const lv_font_t *font);
void ui_text_guard(lv_obj_t *label);
void ui_text_guard_tree(lv_obj_t *root);
#ifdef __cplusplus
}
#endif
