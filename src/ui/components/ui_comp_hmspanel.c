#include "../ui.h"
#include <stdio.h>
#include "xtouch/hms.h"

static uint32_t activePrintError;
static unsigned errorOwner;

void ui_hmsPanel_show(const char *title, enum XtouchDialogButtonStyle btn_style)
{
    errorOwner=xtouch_printer_active();
    lv_obj_t *titleLabel = ui_comp_get_child(ui_hmsComponent, UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANELCAPTION);
    lv_obj_t *confirmButton = ui_comp_get_child(ui_hmsComponent, UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_CONFIRM_BUTTON);
    lv_obj_t *retryButton = ui_comp_get_child(ui_hmsComponent, UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_RETRY_BUTTON);
    lv_obj_t *doneButton = ui_comp_get_child(ui_hmsComponent, UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_DONE_BUTTON);

    lv_obj_clear_flag(confirmButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(retryButton, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(doneButton, LV_OBJ_FLAG_HIDDEN);

    if (btn_style == CONFIRM_AND_DONE)
    {
        lv_obj_clear_flag(doneButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(retryButton, LV_OBJ_FLAG_HIDDEN);
    }
    else if (btn_style == CONFIRM_AND_RETRY)
    {
        lv_obj_add_flag(doneButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(retryButton, LV_OBJ_FLAG_HIDDEN);
    }
    else if (btn_style == DONE_AND_RETRY)
    {
        lv_obj_clear_flag(retryButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(doneButton, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(retryButton, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(doneButton, LV_OBJ_FLAG_HIDDEN);
    }

    unsigned buttons=1+!lv_obj_has_flag(retryButton,LV_OBJ_FLAG_HIDDEN)+!lv_obj_has_flag(doneButton,LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_max_height(lv_obj_get_parent(titleLabel),buttons==3?64:buttons==2?104:128,0);
    if(title)lv_label_set_text(titleLabel, title);
    lv_obj_scroll_to_y(lv_obj_get_parent(titleLabel),0,LV_ANIM_OFF);

    lv_obj_clear_flag(ui_hmsComponent, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(ui_hmsComponent);
}

void ui_hmsPanel_hide()
{
    lv_obj_add_flag(ui_hmsComponent, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_comp_get_child(ui_hmsComponent,UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANELCAPTION),"");
}

/* Both telemetry sources share one modal. Only print_error owns actions. */
static uint32_t errorRevision;
void ui_hmsPanelComponent_onXTouchHMSError(lv_event_t *e)
{
    uint32_t print=hms_print_error();unsigned count=hms_health_count();
    if(!print&&!count){ui_hmsPanel_hide();return;}
    if(!hms_has_pending()&&lv_obj_has_flag(ui_hmsComponent,LV_OBJ_FLAG_HIDDEN))return;
    // Keep the main-loop stack bounded. LVGL owns one growing label string,
    // and hiding the dialog releases it; no second full-list buffer is kept.
    lv_obj_t *caption=ui_comp_get_child(ui_hmsComponent,UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANELCAPTION);
    char line[32];
    if(print){snprintf(line,sizeof(line),"0x%08X",(unsigned)print);lv_label_set_text(caption,line);}else lv_label_set_text(caption,"");
    for(unsigned i=0;i<count;++i){
        uint64_t code=hms_health_at(i);
        snprintf(line,sizeof(line),"%sHMS %04X-%04X-%04X-%04X",(print||i)?"\n":"",(unsigned)(code>>48),(unsigned)((code>>32)&65535),(unsigned)((code>>16)&65535),(unsigned)(code&65535));
        lv_label_ins_text(caption,LV_LABEL_POS_LAST,line);
    }
    if(hms_health_truncated())lv_label_ins_text(caption,LV_LABEL_POS_LAST,"\n...");
    char key[9];snprintf(key,sizeof(key),"%08X",(unsigned)print);
    unsigned actions=print?xtouch_errors_deviceActions(key,xTouchConfig.xTouchSerialNumber):0;
    activePrintError=print;errorRevision=hms_revision();
    // A newly reported fault cancels any obsolete actuator confirmation.
    ui_confirmPanel_hide();
    ui_hmsPanel_show(NULL,(actions&1)?((actions&2)?DONE_AND_RETRY:CONFIRM_AND_RETRY):((actions&2)?CONFIRM_AND_DONE:ONLY_CONFIRM));
}
static bool current_action(unsigned flag)
{
    if(errorOwner!=xtouch_printer_active()||!xtouch_printer_online()||xtouch_printer_switch_pending()||errorRevision!=hms_revision()||!activePrintError||activePrintError!=hms_print_error())return false;
    if(!flag)return true;
    char key[9];snprintf(key,sizeof(key),"%08X",(unsigned)activePrintError);
    return (xtouch_errors_deviceActions(key,xTouchConfig.xTouchSerialNumber)&flag)!=0;
}
void ui_event_comp_hmsPanel_onDoneClick(lv_event_t *e)
{
    if(lv_event_get_code(e)!=LV_EVENT_CLICKED)return;
    bool allowed=current_action(2);hms_dismiss();ui_hmsPanel_hide();
    if(allowed)lv_msg_send(XTOUCH_COMMAND_AMS_CONTROL,"done");
}
void ui_event_comp_hmsPanel_onConfirmClick(lv_event_t *e)
{
    if(lv_event_get_code(e)!=LV_EVENT_CLICKED)return;
    bool allowed=current_action(0);hms_dismiss();ui_hmsPanel_hide();
    // Closing an HMS-only report is always local. Never truncate a 64-bit HMS.
    if(allowed){
        ClearErrorMessage message;message.print_error=(int)activePrintError;
        snprintf(message.subtask_id,sizeof(message.subtask_id),"%s",bambuStatus.subtask_id_);
        lv_msg_send(XTOUCH_COMMAND_CLEAN_PRINT_ERROR,&message);
    }
}
void ui_event_comp_hmsPanel_onRetryClick(lv_event_t *e)
{
    if(lv_event_get_code(e)!=LV_EVENT_CLICKED)return;
    bool allowed=current_action(1);hms_dismiss();ui_hmsPanel_hide();
    if(allowed)lv_msg_send(XTOUCH_COMMAND_AMS_CONTROL,"resume");
}

lv_obj_t *ui_hmsPanel_create(lv_obj_t *comp_parent)
{

    lv_obj_t *cui_hmsPanel;
    cui_hmsPanel = lv_obj_create(comp_parent);
    lv_obj_set_width(cui_hmsPanel, lv_pct(100));
    lv_obj_set_height(cui_hmsPanel, lv_pct(100));
    lv_obj_set_flex_flow(cui_hmsPanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_hmsPanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(cui_hmsPanel, LV_OBJ_FLAG_FLOATING);                                                                                                                                                                                                       /// Flags
    lv_obj_clear_flag(cui_hmsPanel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_radius(cui_hmsPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_hmsPanel, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_hmsPanel, 200, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_hmsPanel, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(cui_hmsPanel,8,0);

    lv_obj_t *cui_hmsPanelContainer;
    cui_hmsPanelContainer = lv_obj_create(cui_hmsPanel);
    lv_obj_set_width(cui_hmsPanelContainer, lv_pct(100));
    lv_obj_set_height(cui_hmsPanelContainer, LV_SIZE_CONTENT); /// 50
    lv_obj_set_flex_flow(cui_hmsPanelContainer, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_hmsPanelContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cui_hmsPanelContainer, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanelContainer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(cui_hmsPanelContainer, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cui_hmsPanelContainer, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_hmsPanelContainer, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(cui_hmsPanelContainer,12,0);
    lv_obj_set_style_pad_row(cui_hmsPanelContainer,8,0);

    // Scroll the combined list, leaving the action buttons visible.
    lv_obj_t *codeList=lv_obj_create(cui_hmsPanelContainer);
    lv_obj_remove_style_all(codeList);
    lv_obj_set_width(codeList,lv_pct(100));
    lv_obj_set_height(codeList,LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(codeList,128,0);
    lv_obj_set_style_pad_right(codeList,4,0);
    lv_obj_set_style_width(codeList,3,LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(codeList,lv_color_hex(0xAAAAAA),LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(codeList,LV_OPA_70,LV_PART_SCROLLBAR);
    lv_obj_set_scroll_dir(codeList,LV_DIR_VER);
    lv_obj_set_scrollbar_mode(codeList,LV_SCROLLBAR_MODE_AUTO);
    lv_obj_clear_flag(codeList,LV_OBJ_FLAG_SCROLL_CHAIN|LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_t *cui_hmsPanelCaption;
    cui_hmsPanelCaption = lv_label_create(codeList);
    lv_obj_set_width(cui_hmsPanelCaption,lv_pct(100));

    lv_obj_set_height(cui_hmsPanelCaption, LV_SIZE_CONTENT); /// 1
    lv_obj_set_flex_grow(cui_hmsPanelCaption, 0);
    lv_label_set_text(cui_hmsPanelCaption, XTOUCH_TEXT_ARE_YOU_SURE);
    lv_label_set_long_mode(cui_hmsPanelCaption, LV_LABEL_LONG_WRAP);
    lv_obj_clear_flag(cui_hmsPanelCaption, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanelCaption, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_left(cui_hmsPanelCaption, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_hmsPanelCaption, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_hmsPanelCaption, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_hmsPanelCaption, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_hmsPanelRetryButton;
    cui_hmsPanelRetryButton = lv_label_create(cui_hmsPanelContainer);
    lv_obj_set_height(cui_hmsPanelRetryButton, LV_SIZE_CONTENT); /// 1
    lv_obj_set_flex_grow(cui_hmsPanelRetryButton, 1);
    lv_obj_set_flex_flow(cui_hmsPanelRetryButton, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_hmsPanelRetryButton, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_label_set_text(cui_hmsPanelRetryButton, "q");
    lv_obj_add_flag(cui_hmsPanelRetryButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);                                                                                                                                                                      /// Flags
    lv_obj_clear_flag(cui_hmsPanelRetryButton, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanelRetryButton, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_hmsPanelRetryButton, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_hmsPanelRetryButton, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_hmsPanelRetryButton, lv_color_hex(0x2AAA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_hmsPanelRetryButton, lv_color_hex(0x2A5500), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_hmsPanelRetryButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_hmsPanelRetryButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_hmsPanelRetryButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_hmsPanelRetryButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_hmsPanelRetryButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_hmsPanelRetryButtonLabel;
    cui_hmsPanelRetryButtonLabel = lv_label_create(cui_hmsPanelRetryButton);
    lv_obj_set_width(cui_hmsPanelRetryButtonLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_hmsPanelRetryButtonLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_hmsPanelRetryButtonLabel, XTOUCH_TEXT_RETRY);
    lv_obj_clear_flag(cui_hmsPanelRetryButtonLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanelRetryButtonLabel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_hmsPanelRetryButtonLabel, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_hmsPanelDoneButton;
    cui_hmsPanelDoneButton = lv_label_create(cui_hmsPanelContainer);
    lv_obj_set_height(cui_hmsPanelDoneButton, LV_SIZE_CONTENT); /// 1
    lv_obj_set_flex_grow(cui_hmsPanelDoneButton, 1);
    lv_obj_set_flex_flow(cui_hmsPanelDoneButton, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_hmsPanelDoneButton, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_label_set_text(cui_hmsPanelDoneButton, "q");
    lv_obj_add_flag(cui_hmsPanelDoneButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);                                                                                                                                                                      /// Flags
    lv_obj_clear_flag(cui_hmsPanelDoneButton, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanelDoneButton, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_hmsPanelDoneButton, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_hmsPanelDoneButton, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_hmsPanelDoneButton, lv_color_hex(0x2AAA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_hmsPanelDoneButton, lv_color_hex(0x2A5500), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_hmsPanelDoneButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_hmsPanelDoneButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_hmsPanelDoneButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_hmsPanelDoneButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_hmsPanelDoneButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_hmsPanelDoneButtonLabel;
    cui_hmsPanelDoneButtonLabel = lv_label_create(cui_hmsPanelDoneButton);
    lv_obj_set_width(cui_hmsPanelDoneButtonLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_hmsPanelDoneButtonLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_hmsPanelDoneButtonLabel, XTOUCH_TEXT_DONE);
    lv_obj_clear_flag(cui_hmsPanelDoneButtonLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanelDoneButtonLabel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_hmsPanelDoneButtonLabel, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_hmsPanelConfirmButton;
    cui_hmsPanelConfirmButton = lv_label_create(cui_hmsPanelContainer);
    lv_obj_set_height(cui_hmsPanelConfirmButton, LV_SIZE_CONTENT); /// 1
    lv_obj_set_flex_grow(cui_hmsPanelConfirmButton, 1);
    lv_obj_set_flex_flow(cui_hmsPanelConfirmButton, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cui_hmsPanelConfirmButton, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_label_set_text(cui_hmsPanelConfirmButton, "q");
    lv_obj_add_flag(cui_hmsPanelConfirmButton, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);                                                                                                                                                                      /// Flags
    lv_obj_clear_flag(cui_hmsPanelConfirmButton, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanelConfirmButton, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_hmsPanelConfirmButton, &ui_font_xlcdmin, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(cui_hmsPanelConfirmButton, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_hmsPanelConfirmButton, lv_color_hex(0x2AAA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(cui_hmsPanelConfirmButton, lv_color_hex(0x2A5500), LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(cui_hmsPanelConfirmButton, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(cui_hmsPanelConfirmButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(cui_hmsPanelConfirmButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(cui_hmsPanelConfirmButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(cui_hmsPanelConfirmButton, 8, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t *cui_hmsPanelConfirmButtonLabel;
    cui_hmsPanelConfirmButtonLabel = lv_label_create(cui_hmsPanelConfirmButton);
    lv_obj_set_width(cui_hmsPanelConfirmButtonLabel, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(cui_hmsPanelConfirmButtonLabel, LV_SIZE_CONTENT); /// 1
    lv_label_set_text(cui_hmsPanelConfirmButtonLabel, XTOUCH_TEXT_CONFIRM);
    lv_obj_clear_flag(cui_hmsPanelConfirmButtonLabel, LV_OBJ_FLAG_PRESS_LOCK | LV_OBJ_FLAG_CLICK_FOCUSABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_SNAPPABLE | LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLL_CHAIN); /// Flags
    lv_obj_set_scrollbar_mode(cui_hmsPanelConfirmButtonLabel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_text_font(cui_hmsPanelConfirmButtonLabel, LV_FONT_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_t **children = lv_mem_alloc(sizeof(lv_obj_t *) * _UI_COMP_HMSPANEL_NUM);
    children[UI_COMP_HMSPANEL_HMSPANEL] = cui_hmsPanel;
    children[UI_COMP_HMSPANEL_HMSPANELCONTAINER] = cui_hmsPanelContainer;
    children[UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANELCAPTION] = cui_hmsPanelCaption;
    children[UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_RETRY_BUTTON] = cui_hmsPanelRetryButton;
    children[UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_RETRY_BUTTON_LABEL] = cui_hmsPanelRetryButtonLabel;
    children[UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_DONE_BUTTON] = cui_hmsPanelDoneButton;
    children[UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_DONE_BUTTON_LABEL] = cui_hmsPanelDoneButtonLabel;
    children[UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_CONFIRM_BUTTON] = cui_hmsPanelConfirmButton;
    children[UI_COMP_HMSPANEL_HMSPANELCONTAINER_HMSPANEL_CONFIRM_BUTTON_LABEL] = cui_hmsPanelConfirmButtonLabel;

    lv_obj_add_event_cb(cui_hmsPanel, get_component_child_event_cb, LV_EVENT_GET_COMP_CHILD, children);
    lv_obj_add_event_cb(cui_hmsPanel, del_component_child_event_cb, LV_EVENT_DELETE, children);

    lv_obj_add_event_cb(cui_hmsPanelConfirmButton, ui_event_comp_hmsPanel_onConfirmClick, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_hmsPanelRetryButton, ui_event_comp_hmsPanel_onRetryClick, LV_EVENT_ALL, children);
    lv_obj_add_event_cb(cui_hmsPanelDoneButton, ui_event_comp_hmsPanel_onDoneClick, LV_EVENT_ALL, children);

    lv_obj_add_event_cb(cui_hmsPanel, ui_hmsPanelComponent_onXTouchHMSError, LV_EVENT_MSG_RECEIVED, NULL);
    lv_msg_subsribe_obj(XTOUCH_ON_ERROR, cui_hmsPanel, NULL);

    ui_comp_hmsPanel_create_hook(cui_hmsPanel);
    return cui_hmsPanel;
}
