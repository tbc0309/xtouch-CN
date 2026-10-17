/* One native-drawn modal avoids allocating four buttons and four scrolling
 * labels in the constrained LVGL pool. Device changes stay queued in loop(). */
#include "ui.h"
#include "printer_picker.h"
#include "printer_models.h"
#include "source_icons.h"
#include <stdio.h>
#include <string.h>

#ifndef UI_DESIGN_CLIP_AREA
#define UI_DESIGN_CLIP_AREA(ctx) ((ctx)->clip_area)
#endif

static lv_obj_t *overlay;
static bool closing;

static unsigned configured_slots(uint8_t slots[XTOUCH_MAX_PRINTERS])
{
    unsigned count = 0;
    for (unsigned i=0; i<XTOUCH_MAX_PRINTERS; ++i)
        if (xtouch_printer_configured(i)) slots[count++] = i;
    return count;
}

static lv_area_t dialog_area(lv_obj_t *object, unsigned count)
{
    lv_area_t screen; lv_obj_get_coords(object, &screen);
    /* Same three design-size classes as shared confirmation/fault dialogs. */
    int width=count<=1?192:256;
    int height=count<=1?96:count<=2?128:208;
    int x = screen.x1 + (lv_area_get_width(&screen)-width)/2;
    int y = screen.y1 + (lv_area_get_height(&screen)-height)/2;
    return (lv_area_t){x,y,x+width-1,y+height-1};
}

static lv_area_t close_area(lv_area_t panel)
{
    int closeWidth=30*lv_disp_get_ver_res(NULL)*320/(240*lv_disp_get_hor_res(NULL));
    return (lv_area_t){panel.x2-6-closeWidth,panel.y1+6,panel.x2-7,panel.y1+35};

}

static void text(lv_draw_ctx_t *ctx, lv_area_t area, const char *value, uint32_t color, bool center)
{
    lv_draw_label_dsc_t d;lv_draw_label_dsc_init(&d);d.font=LV_FONT_DEFAULT;d.color=lv_color_hex(color);
    ui_layout_draw_text(ctx,&d,area,value,center);

}

static void ellipsize(char *buffer, unsigned capacity, const char *name, int width)
{
    strlcpy(buffer, name, capacity);
    lv_point_t size;
    lv_txt_get_size(&size, buffer, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_EXPAND);
    if (size.x <= width) return;
    size_t n = strlen(buffer);
    while (n) {
        --n;
        while (n && (((unsigned char)buffer[n] & 0xC0) == 0x80)) --n;
        buffer[n] = 0;
        lv_txt_get_size(&size, buffer, LV_FONT_DEFAULT, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_EXPAND);
        if (size.x <= width-16) break;
    }
    strlcat(buffer, "...", capacity);
}

static void draw(lv_event_t *e)
{
    uint8_t slots[XTOUCH_MAX_PRINTERS]; unsigned count = configured_slots(slots);
    lv_obj_t *object = lv_event_get_target(e); lv_draw_ctx_t *ctx = lv_event_get_draw_ctx(e);
    lv_area_t panel = dialog_area(object, count);
    lv_draw_rect_dsc_t r; lv_draw_rect_dsc_init(&r);
    r.bg_color = lv_color_hex(0x444444); r.bg_opa = LV_OPA_COVER; r.radius = 8;
    lv_draw_rect(ctx, &r, &panel);
    text(ctx, (lv_area_t){panel.x1+14,panel.y1+10,panel.x2-40,panel.y1+29}, XTOUCH_TEXT_SELECT_DEVICE, 0xFFFFFF, false);
    /* Same circled-cross icon and red tone as the filament confirmation. */
    lv_area_t close = close_area(panel);
    r.radius=6; r.bg_color=lv_color_hex(0xAA2A00);
    lv_draw_rect(ctx,&r,&close);
    lv_draw_label_dsc_t symbol; lv_draw_label_dsc_init(&symbol);
    symbol.font=&ui_font_xlcdmin; symbol.color=lv_color_white(); symbol.align=LV_TEXT_ALIGN_CENTER;
    ui_layout_draw_text(ctx,&symbol,close,"r",true);

}
static void draw_rows(lv_event_t *e)
{
    uint8_t slots[XTOUCH_MAX_PRINTERS];unsigned count=configured_slots(slots);
    lv_obj_t *object=lv_event_get_target(e);lv_draw_ctx_t *ctx=lv_event_get_draw_ctx(e);
    lv_area_t a;lv_obj_get_coords(object,&a);
    lv_draw_rect_dsc_t r;lv_draw_rect_dsc_init(&r);r.bg_opa=LV_OPA_COVER;
    unsigned first=UI_DESIGN_CLIP_AREA(ctx)->y1>a.y1?(UI_DESIGN_CLIP_AREA(ctx)->y1-a.y1)/40:0;
    for (unsigned i=first; i<count && (int)(a.y1+i*40)<=UI_DESIGN_CLIP_AREA(ctx)->y2; ++i) {
        bool active = slots[i] == xtouch_printer_active();
        int y=a.y1+i*40;
        lv_area_t row = {a.x1,y,a.x2,y+33};
        r.radius = 5; r.bg_color = lv_color_hex(active ? 0x777777 : 0x555555);
        lv_draw_rect(ctx, &r, &row);
        uint8_t source=xtouch_printer_source(slots[i]);
        lv_draw_img_dsc_t image;lv_draw_img_dsc_init(&image);image.recolor=lv_color_hex(xtouch_printer_source_color(slots[i]));image.recolor_opa=LV_OPA_COVER;lv_area_t bounds={row.x1+5,y+5,row.x1+28,y+28};lv_draw_img(ctx,&image,&bounds,source==0?&source_img_0:source==1?&source_img_1:&source_img_2);
        char formatted[100],name[100];
        snprintf(formatted,sizeof(formatted),"[%s] %s",ui_printer_model_short_name(xtouch_printer_model_at(slots[i])),xtouch_printer_name_at(slots[i]));
        ellipsize(name,sizeof(name),formatted,lv_area_get_width(&row)-64);
        text(ctx, (lv_area_t){row.x1+34,y+8,row.x2-30,y+26}, name, xtouch_printer_source_color(slots[i]), false);
        lv_area_t dot = {row.x2-20,y+13,row.x2-13,y+20};
        r.radius = LV_RADIUS_CIRCLE; r.bg_color = lv_color_hex(active ? 0x74D34A : 0xAAAAAA);
        lv_draw_rect(ctx, &r, &dot);
    }
}

void ui_printer_picker_close(void)
{
    if (!overlay || closing) return;
    closing = true;
    lv_obj_del_async(overlay); // Never delete the target of a running input event.
}

static void deleted(lv_event_t *e)
{
    if (lv_event_get_target(e) == overlay) { overlay = NULL; closing = false; }
}

static void click(lv_event_t *e)
{
    lv_indev_t *input = lv_indev_get_act();
    if (!input || closing) return;
    lv_point_t point; lv_indev_get_point(input, &point);
    uint8_t slots[XTOUCH_MAX_PRINTERS]; unsigned count = configured_slots(slots);
    lv_area_t panel = dialog_area(lv_event_get_target(e), count);
    lv_area_t close=close_area(panel);
    if (point.x < panel.x1 || point.x > panel.x2 || point.y < panel.y1 || point.y > panel.y2 ||
        (point.x >= close.x1 && point.x <= close.x2 && point.y >= close.y1 && point.y <= close.y2)) {
        ui_printer_picker_close(); return;
    }

}

static void select_row(lv_event_t *e)
{
    lv_indev_t *input=lv_indev_get_act();if(!input || closing)return;
    lv_point_t point;lv_indev_get_point(input,&point);lv_area_t a;lv_obj_get_coords(lv_event_get_target(e),&a);
    unsigned index=(point.y-a.y1)/40;uint8_t slots[XTOUCH_MAX_PRINTERS];unsigned count=configured_slots(slots);
    if(index<count){xtouch_printer_request(slots[index]);ui_printer_picker_close();}
}

void ui_printer_picker_show(void)
{
    if (overlay) return;
    closing = false;
    overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(overlay);
    lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_50, 0);
    lv_obj_set_style_bg_opa(overlay,200,0);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(overlay, draw, LV_EVENT_DRAW_MAIN_END, NULL);
    lv_obj_add_event_cb(overlay, click, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(overlay, deleted, LV_EVENT_DELETE, NULL);
    uint8_t slots[XTOUCH_MAX_PRINTERS];unsigned count=configured_slots(slots);
    lv_obj_update_layout(overlay);lv_area_t panel=dialog_area(overlay,count);
    lv_obj_t *viewport=lv_obj_create(overlay);lv_obj_remove_style_all(viewport);
    lv_obj_set_pos(viewport,panel.x1+8,panel.y1+40);lv_obj_set_size(viewport,panel.x2-panel.x1-15,(count>4?4:count)*40);
    lv_obj_set_scroll_dir(viewport,LV_DIR_VER);lv_obj_set_scrollbar_mode(viewport,LV_SCROLLBAR_MODE_AUTO);
    lv_obj_clear_flag(viewport,LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_style_width(viewport,3,LV_PART_SCROLLBAR);lv_obj_set_style_bg_color(viewport,lv_color_hex(0xAAAAAA),LV_PART_SCROLLBAR);
    lv_obj_t *content=lv_obj_create(viewport);lv_obj_remove_style_all(content);lv_obj_set_size(content,panel.x2-panel.x1-19,count*40);
    lv_obj_clear_flag(content,LV_OBJ_FLAG_SCROLLABLE);lv_obj_add_flag(content,LV_OBJ_FLAG_CLICKABLE|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_add_event_cb(content,draw_rows,LV_EVENT_DRAW_MAIN_END,NULL);lv_obj_add_event_cb(content,select_row,LV_EVENT_SHORT_CLICKED,NULL);
    for(unsigned i=0;i<count;++i)if(slots[i]==xtouch_printer_active()){lv_obj_update_layout(viewport);lv_obj_scroll_to_y(viewport,i*40,LV_ANIM_OFF);break;}

}
