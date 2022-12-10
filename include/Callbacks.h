#ifndef _Callbacks_h_
#define _Callbacks_h_

#include <pspkernel.h>

int exit_callback(int arg1, int arg2, void* common);

int CallbackThread(SceSize args, void* argp);

int SetupCallbacks(void);

#endif