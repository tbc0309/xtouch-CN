#ifndef _XLCD_GLOBALS
#define _XLCD_GLOBALS

void xtouch_globals_init()
{
    // No active AMS tray is known until the printer reports one.
    bambuStatus.m_tray_now = 255;
    bambuStatus.m_tray_tar = 255;
    controlMode.inc = 1;
    controlMode.axis = ControlAxisXY;
}

#endif
