/* Row-addressable printer artwork. See scripts/generate_printer_images.py.
 * Pixels remain in Flash; decoding uses LVGL's single scanline output buffer.
 */
#include "printer_models.h"
#include <string.h>

#if LV_COLOR_DEPTH != 16 || LV_COLOR_16_SWAP
#error Printer artwork requires the configured native RGB565 color layout
#endif

static uint16_t read16(const uint8_t *p) { return p[0] | ((uint16_t)p[1] << 8); }

static const lv_img_dsc_t *packed_image(const void *src)
{
    if (lv_img_src_get_type(src) != LV_IMG_SRC_VARIABLE) return NULL;
    const lv_img_dsc_t *image = src;
    if (image->header.cf != LV_IMG_CF_RAW || !image->data || image->data_size < 10) return NULL;
    const uint8_t *p = image->data;
    if (memcmp(p, "XTP1", 4) || !p[4] || !p[5] || !p[6] || p[6] > 128) return NULL;
    unsigned table_end = 8 + p[6]*2 + (p[5]+1)*2;
    if (table_end > image->data_size || image->header.w != p[4] || image->header.h != p[5]) return NULL;
    return image;
}

static lv_res_t image_info(lv_img_decoder_t *decoder, const void *src, lv_img_header_t *header)
{
    (void)decoder;
    const lv_img_dsc_t *image = packed_image(src);
    if (!image) return LV_RES_INV;
    *header = image->header;
    header->cf = LV_IMG_CF_TRUE_COLOR;
    return LV_RES_OK;
}

static lv_res_t image_open(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc)
{
    if (image_info(decoder, dsc->src, &dsc->header) != LV_RES_OK) return LV_RES_INV;
    dsc->img_data = NULL;
    dsc->user_data = (void *)dsc->src;
    return LV_RES_OK;
}

static lv_res_t image_read(lv_img_decoder_t *decoder, lv_img_decoder_dsc_t *dsc,
                          lv_coord_t x, lv_coord_t y, lv_coord_t len, uint8_t *out)
{
    (void)decoder;
    const lv_img_dsc_t *image = dsc->user_data;
    if (!image || !out || x < 0 || y < 0 || len <= 0 || x+len > image->header.w || y >= image->header.h) return LV_RES_INV;
    const uint8_t *p = image->data, *palette = p+8, *table = palette+p[6]*2;
    unsigned data_start = 8 + p[6]*2 + (p[5]+1)*2;
    unsigned start = data_start + read16(table+y*2), end = data_start + read16(table+(y+1)*2);
    if (end > image->data_size || start > end) return LV_RES_INV;
    unsigned pixel = 0, written = 0;
    while (start < end && pixel < (unsigned)(x+len)) {
        unsigned token = p[start++], count = (token & 127)+1;
        bool repeat = token & 128;
        if (start + (repeat ? 1 : count) > end || pixel+count > image->header.w) return LV_RES_INV;
        for (unsigned i=0; i<count; ++i, ++pixel) {
            unsigned index = p[start+(repeat ? 0 : i)];
            if (index >= p[6]) return LV_RES_INV;
            if (pixel >= (unsigned)x && pixel < (unsigned)(x+len)) {
                *out++ = palette[index*2]; *out++ = palette[index*2+1]; ++written;
            }
        }
        start += repeat ? 1 : count;
    }
    return written == (unsigned)len ? LV_RES_OK : LV_RES_INV;
}

void ui_printer_image_decoder_init(void)
{
    static lv_img_decoder_t *decoder;
    if (decoder) return;
    decoder = lv_img_decoder_create();
    if (!decoder) return;
    lv_img_decoder_set_info_cb(decoder, image_info);
    lv_img_decoder_set_open_cb(decoder, image_open);
    lv_img_decoder_set_read_line_cb(decoder, image_read);
}
