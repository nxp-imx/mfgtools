#ifndef _WINDRVR_INT_THREAD_H_
#define _WINDRVR_INT_THREAD_H_

#include "windrvr.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus 

typedef void (*INT_HANDLER_FUNC)(PVOID pData);

// OLD prototypes for backward compatibility
// implemented in src/windrvr_int_thread.c
 BOOL InterruptThreadEnable(HANDLE *phThread, HANDLE hWD, WD_INTERRUPT *pInt,
    INT_HANDLER_FUNC func, PVOID pData);
 void InterruptThreadDisable(HANDLE hThread);
// implemented in src/windrvr_events.c
 struct event_handle_t *event_register(HANDLE hWD, WD_EVENT *event,
    void (*func)(WD_EVENT *, void *), void *data);
 void event_unregister(HANDLE hWD, struct event_handle_t *handle);


// New prototypes. Functions return status.
 DWORD DLLCALLCONV InterruptEnable(HANDLE *phThread, HANDLE hWD, WD_INTERRUPT *pInt,
    INT_HANDLER_FUNC func, PVOID pData);

 DWORD DLLCALLCONV InterruptDisable(HANDLE hThread);

#ifdef __cplusplus
}
#endif  // __cplusplus 

#endif

