#include "filament_actions.h"
#include "printer_models.h"
#include <stdio.h>
#include <string.h>

/* Selection is a UI target, never a change to the printer's loaded feeder. */
static int selected = -1;
static unsigned selected_printer = 255;
static struct ui_filament_request pending;

static bool slot_exists(int tray)
{
    return tray == 254 || (tray >= 0 && tray < XTOUCH_MAX_AMS_SLOTS &&
        (bambuStatus.ams_exist_bits & (1UL << (tray / 4))) &&
        (bambuStatus.tray_exist_bits & (1UL << tray)));
}
int ui_filament_selected(void)
{
    unsigned printer = xtouch_printer_active();
    if (printer != selected_printer) { selected = -1; selected_printer = printer; }
    if (selected >= 0 && !slot_exists(selected)) selected = -1;
    return selected;
}
bool ui_filament_select(int tray)
{
    ui_filament_selected();
    if (tray != -1 && !slot_exists(tray)) return false;
    selected = tray;
    return true;
}
static bool supported_single_nozzle(void)
{
    const char *m = ui_printer_model_code(bambuStatus.printer_type);
    /* Multi-extruder routing needs a separate extruder-aware control path. */
    return m && (!strcmp(m,"C11") || !strcmp(m,"C12") || !strcmp(m,"C13") ||
        !strcmp(m,"BL-P001") || !strcmp(m,"BL-P002") || !strcmp(m,"N1") || !strcmp(m,"N2S"));
}
int ui_filament_temperature(int tray)
{
    unsigned low=0,high=0;
    if (tray==254) {low=bambuStatus.external_nozzle_min;high=bambuStatus.external_nozzle_max;}
    else if (tray>=0 && tray<XTOUCH_MAX_AMS_SLOTS) {low=bambuStatus.ams_nozzle_min[tray];high=bambuStatus.ams_nozzle_max[tray];}
    if (low>=150 && high>=low && high<=350 && (low+high)/2<=300) return (low+high)/2;
    int target=bambuStatus.nozzle_target_temper;
    return target>=150 && target<=300 ? target : 0;
}
bool ui_filament_allowed(unsigned action, int tray)
{
    if (!xtouch_printer_online() || xtouch_printer_switch_pending() || !supported_single_nozzle() || !slot_exists(tray)) return false;
    /* Only validated legacy unit IDs 0..3 are enabled for motion commands. */
    if (tray != 254 && tray >= 16) return false;
    if (bambuStatus.print_status != XTOUCH_PRINT_STATUS_IDLE &&
        bambuStatus.print_status != XTOUCH_PRINT_STATUS_FINISHED &&
        bambuStatus.print_status != XTOUCH_PRINT_STATUS_FAILED) return false;
    bool assist_ready=bambuStatus.ams_status_main==AMS_STATUS_MAIN_ASSIST &&
        bambuStatus.ams_status_sub==0 && bambuStatus.hw_switch_state==1 &&
        bambuStatus.m_tray_now>=0 && bambuStatus.m_tray_now<16 &&
        bambuStatus.m_tray_tar==bambuStatus.m_tray_now;
    /* Loaded AMS normally remains in assist (0x0300), not idle. */
    if (bambuStatus.ams_exist_bits && bambuStatus.ams_status_main!=AMS_STATUS_MAIN_IDLE && !assist_ready) return false;
    if (bambuStatus.tray_reading_bits) return false;
    int current = bambuStatus.m_tray_now;
    if (action == UI_FILAMENT_REREAD) return tray < XTOUCH_MAX_AMS_SLOTS && current == 255;
    if (action == UI_FILAMENT_UNLOAD) return current == tray ||
        (tray == 254 && current == 255 && bambuStatus.hw_switch_state == 1);
    if (action == UI_FILAMENT_LOAD) return ui_filament_temperature(tray)>0 && current != tray &&
        (current == 255 || (current >= 0 && current < 16) || current == 254);
    return false;
}
static void target_name(int tray, char *text, size_t capacity)
{
    if (tray == 254) snprintf(text,capacity,"%s",XTOUCH_TEXT_FILAMENT_EXTERNAL);
    else snprintf(text,capacity,"%c%d",'A'+tray/4,tray%4+1);
}
static void confirm_action(void)
{
    /* Never redirect an open confirmation after device/selection changes. */
    if (pending.printer != xtouch_printer_active() || pending.tray != ui_filament_selected() ||
        !ui_filament_allowed(pending.action,pending.tray)) return;
    lv_msg_send(XTOUCH_COMMAND_FILAMENT_ACTION,&pending);
}
void ui_filament_report_error(uint32_t error)
{
    if (!error) return;
    char prompt[180];
    if (error==84033543UL) snprintf(prompt,sizeof(prompt),"%s",XTOUCH_TEXT_FILAMENT_AUTH_ERROR);
    else snprintf(prompt,sizeof(prompt),XTOUCH_TEXT_FILAMENT_ERROR,(unsigned)error);
    ui_confirmPanel_show(prompt,ui_confirmPanel_NOOP);
}
void ui_filament_request_action(unsigned action)
{
    int tray = ui_filament_selected();
    if (!ui_filament_allowed(action,tray)) return;
    pending = (struct ui_filament_request){xtouch_printer_active(),tray,action};
    char name[24], prompt[180]; target_name(tray,name,sizeof(name));
    const char *format = action == UI_FILAMENT_LOAD ? XTOUCH_TEXT_FILAMENT_LOAD_CONFIRM :
        action == UI_FILAMENT_UNLOAD ? XTOUCH_TEXT_FILAMENT_UNLOAD_CONFIRM : XTOUCH_TEXT_FILAMENT_REREAD_CONFIRM;
    snprintf(prompt,sizeof(prompt),format,name);
    ui_confirmPanel_show(prompt,confirm_action);
}
void ui_filament_refresh(lv_obj_t *component)
{
    int tray = ui_filament_selected();
    char name[24], caption[48];
    if (tray >= 0) target_name(tray,name,sizeof(name)); else name[0]=0;
    const unsigned ids[] = {
        UI_COMP_FILAMENTCOMPONENT_FILAMENTSCREENFILAMENT_FILAMENTSCREENLOAD,
        UI_COMP_FILAMENTCOMPONENT_FILAMENTSCREENFILAMENT_FILAMENTSCREENUNLOAD,
        UI_COMP_FILAMENTCOMPONENT_FILAMENTSCREENFILAMENT_FILAMENTSCREENFILAMENTICON
    };
    const char *labels[] = {XTOUCH_TEXT_LOAD,XTOUCH_TEXT_UN_NLOAD,XTOUCH_TEXT_FILAMENT_REREAD};
    for (unsigned action=0;action<3;++action) {
        lv_obj_t *button = ui_comp_get_child(component,ids[action]);
        bool allowed = ui_filament_allowed(action,tray);
        if (allowed) lv_obj_clear_state(button,LV_STATE_DISABLED); else lv_obj_add_state(button,LV_STATE_DISABLED);
        if (tray < 0) snprintf(caption,sizeof(caption),"%s",action == 2 ? labels[action] : XTOUCH_TEXT_FILAMENT_PICK);
        else snprintf(caption,sizeof(caption),"%s %s",name,action==UI_FILAMENT_LOAD && !ui_filament_temperature(tray) ? XTOUCH_TEXT_FILAMENT_SET_TEMP : labels[action]);
        lv_obj_t *label=action==2 ? lv_obj_get_child(button,0) : button;
        if (strcmp(lv_label_get_text(label),caption)) lv_label_set_text(label,caption);
    }
}
static void refresh_event(lv_event_t *e) { ui_filament_refresh(lv_event_get_target(e)); }
static void reread_click(lv_event_t *e) { ui_filament_request_action(UI_FILAMENT_REREAD); }
void ui_filament_bind(lv_obj_t *component)
{
    lv_obj_t *reread = ui_comp_get_child(component,UI_COMP_FILAMENTCOMPONENT_FILAMENTSCREENFILAMENT_FILAMENTSCREENFILAMENTICON);
    lv_obj_add_flag(reread,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_font(reread,&ui_font_xlcdmin,0);
    lv_obj_set_style_pad_all(reread,0,0);
    lv_obj_set_style_pad_top(reread,1,0);
    lv_obj_set_style_text_color(reread,lv_color_hex(0x777777),LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(reread,lv_color_hex(0x444444),LV_STATE_DISABLED);
    lv_obj_t *caption = lv_label_create(reread);
    lv_obj_set_size(caption,lv_pct(100),18);
    lv_obj_set_pos(caption,0,24);
    lv_obj_set_style_text_font(caption,LV_FONT_DEFAULT,0);
    lv_obj_set_style_text_align(caption,LV_TEXT_ALIGN_CENTER,0);
    lv_label_set_long_mode(caption,LV_LABEL_LONG_CLIP);
    lv_obj_add_event_cb(reread,reread_click,LV_EVENT_CLICKED,NULL);
    const unsigned messages[] = {XTOUCH_ON_AMS,XTOUCH_ON_AMS_BITS,XTOUCH_ON_PRINT_STATUS,XTOUCH_ON_MQTT};
    for (unsigned i=0;i<4;++i) lv_msg_subsribe_obj(messages[i],component,NULL);
    lv_obj_add_event_cb(component,refresh_event,LV_EVENT_MSG_RECEIVED,NULL);
    ui_filament_refresh(component);
}
