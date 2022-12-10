#include "../include/Callbacks.h"

int exit_callback(int arg1, int arg2, void* common) {
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void* argp) {
    int callbackId = sceKernelCreateCallback("Exit callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(callbackId);
    sceKernelSleepThreadCB();
    return 0;
}

int SetupCallbacks(void) {
    int threadId = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if (threadId >= 0) {
        sceKernelStartThread(threadId, 0, 0);
    }
    return threadId;
}