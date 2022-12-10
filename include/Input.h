#ifndef _Input_h_
#define _Input_h_

#include <pspctrl.h>
#include <psptypes.h>

namespace Input {
    extern SceCtrlData ctrlData;
    extern u8 stickDeadZone;
    void init();
    void read();
    void setStickDeadZone(u8 v);
    u8 getStickX();
    u8 getStickY();
}

#endif