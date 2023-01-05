#include "../include/Input.h"


namespace Input {
    SceCtrlData ctrlData;
    u8 stickDeadZone;
    
    void init() {
        stickDeadZone = 10;
        sceCtrlSetSamplingCycle(0);
        sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    }

    void read() {
        sceCtrlReadBufferPositive(&ctrlData, 1);
    }

    void setStickDeadZone(u8 v) {
        stickDeadZone = v;
    }

    u8 getStickX() {
        return ((ctrlData.Lx < 127 - stickDeadZone || ctrlData.Lx > 127 + stickDeadZone) ? ctrlData.Lx : 0);
    }
    u8 getStickY() {
        return ((ctrlData.Ly < 127 - stickDeadZone || ctrlData.Ly > 127 + stickDeadZone) ? ctrlData.Ly : 0);
    }
}