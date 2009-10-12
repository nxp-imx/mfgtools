#include "windrvr.h"
#if defined(WIN32) && !defined(WINCE)
    #include <process.h>
#endif
#if !defined(VXWORKS)
    #include <malloc.h>
    #if defined(UNIX)
        #include <pthread.h>
        #include <sys/time.h>
        #include <unistd.h>
        #include <errno.h>
    #endif
#endif

#include "utils.h"
#include <stdarg.h>
#include <stdio.h>

#if defined(LINUX)
typedef struct {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    BOOL signaled;
} wd_linux_event_t;
#else
    #include <stdio.h>
    #include <stdarg.h>
    #if defined(WINCE) && WINCE_VER!=410
        #define vsnprintf(a,b,c,d) vsprintf(a,c,d)
    #else
        #define vsnprintf _vsnprintf
    #endif
#endif
// threads functions

typedef struct
{
    HANDLER_FUNC func;
    void *data;
} thread_struct_t;

#if defined(WIN32) && !defined (WINCE)
    static unsigned int WINAPI thread_handler(void *data)
#elif defined(WINCE)
    static unsigned long WINAPI thread_handler(void *data)
#else
    static void *thread_handler(void *data)
#endif
{
    thread_struct_t *t = (thread_struct_t *)data;
    t->func(t->data);
    free(t);
    return 0;
}

DWORD DLLCALLCONV ThreadStart(HANDLE *phThread, void (*pFunc)(void *pData), void *pData)
{
    #if defined(WIN32) || defined(WINCE)
    DWORD dwTmp;
    #endif
    thread_struct_t *t = (thread_struct_t *)malloc(sizeof(thread_struct_t));
    void *ret = NULL;

    t->func = pFunc;
    t->data = pData;
    #if defined(WIN32) && !defined(WINCE)
        ret = (void *)_beginthreadex(NULL, 0x1000, thread_handler,
            (void *)t, 0, (unsigned int *)&dwTmp);
    #elif defined(WINCE)
        ret = (void *)CreateThread(NULL, 0x1000, thread_handler,
            (void *)t, 0, (unsigned long *)&dwTmp);
    #elif defined(VXWORKS)
    {
        int priority, std, task_id = ERROR;
        if (taskPriorityGet(taskIdSelf(), &priority)!=ERROR)
        {
            task_id = taskSpawn(NULL, priority, 0, 0x4000,
                (FUNCPTR)thread_handler, (int)t, 0, 0, 0, 0, 0,
                0, 0, 0, 0);
        }
        if (task_id!=ERROR)
        {
            std = ioTaskStdGet(0, 1);
            ioTaskStdSet(task_id, 0, std);
            ioTaskStdSet(task_id, 1, std);
            ioTaskStdSet(task_id, 2, std);
            ret = (void *)task_id;
        }
    }
    #elif defined(UNIX)
    {
        int err = 0;
        ret = malloc(sizeof(pthread_t));
        if (ret)
            err = pthread_create((pthread_t *)ret, NULL, thread_handler, (PVOID)t);
        if (err)
        {
            free(ret);
            ret = NULL;
        }
    }
    #endif
    *phThread = ret;
    if (!ret)
    {
        free(t);
        return WD_INSUFFICIENT_RESOURCES;
    }
    return WD_STATUS_SUCCESS;
}


#if defined(VXWORKS)
    #define WAIT_FOR_EVER 0
    static void vxTask_wait(int taskId, int waitTime)
    {
        SEM_ID waitSem;

        if (waitTime==WAIT_FOR_EVER)
        {
            /* Loop while task is still alive */
            while (taskIdVerify(taskId)==OK)
                taskDelay(3);
        }
        else
        {
            /* create a dummy semaphore and try to take it for the specified
             * time. */
            waitSem = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
            semTake(waitSem, waitTime);
            semDelete(waitSem);
        }
    }
#endif

void DLLCALLCONV ThreadStop(HANDLE hThread)
{
    #if defined(WIN32)
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    #elif defined(VXWORKS)
        vxTask_wait((int)hThread, WAIT_FOR_EVER);
    #elif defined(UNIX)
        pthread_join(*((pthread_t *)hThread), NULL);
        free(hThread);
    #endif
}

// Synchronization objects

// Auto-reset events
DWORD DLLCALLCONV OsEventCreate(HANDLE *phOsEvent)
{
#if defined(WIN32)
    *phOsEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    return *phOsEvent ? WD_STATUS_SUCCESS : WD_INSUFFICIENT_RESOURCES;
#elif defined(LINUX)
    wd_linux_event_t *linux_event = malloc(sizeof(wd_linux_event_t));
    if (!linux_event)
        return WD_INSUFFICIENT_RESOURCES;
    memset(linux_event, 0, sizeof(wd_linux_event_t));
    pthread_cond_init(&linux_event->cond, NULL); 
    pthread_mutex_init(&linux_event->mutex, NULL);
    *phOsEvent = linux_event;
    return WD_STATUS_SUCCESS;
#else
    return WD_NOT_IMPLEMENTED;
#endif
}

void DLLCALLCONV OsEventClose(HANDLE hOsEvent)
{
#if defined(WIN32)
    if (hOsEvent)
        CloseHandle(hOsEvent);
#elif defined(LINUX)
    wd_linux_event_t *linux_event = (wd_linux_event_t *)hOsEvent;
    pthread_cond_destroy(&linux_event->cond); 
    pthread_mutex_destroy(&linux_event->mutex);
    free(linux_event);
#endif
}

DWORD DLLCALLCONV OsEventWait(HANDLE hOsEvent, DWORD dwSecTimeout)
{
    DWORD rc = WD_STATUS_SUCCESS;
#if defined(WIN32)
    rc = WaitForSingleObject(hOsEvent, dwSecTimeout * 1000);
    switch (rc) 
    {
        case WAIT_OBJECT_0:
            return WD_STATUS_SUCCESS;

        case WAIT_TIMEOUT:
            return WD_TIME_OUT_EXPIRED;
    }
    return WD_SYSTEM_INTERNAL_ERROR;
#elif defined(LINUX)
    struct timeval now;
    struct timespec timeout;
    wd_linux_event_t *linux_event = (wd_linux_event_t *)hOsEvent;

    pthread_mutex_lock(&linux_event->mutex);
    if (!linux_event->signaled)
    {
        gettimeofday(&now, NULL);
        timeout.tv_sec = now.tv_sec + dwSecTimeout;
        timeout.tv_nsec = now.tv_usec;

        rc = pthread_cond_timedwait(&linux_event->cond, &linux_event->mutex, &timeout);
    }
    linux_event->signaled = FALSE;
    pthread_mutex_unlock(&linux_event->mutex);
    return rc==ETIMEDOUT ?  WD_TIME_OUT_EXPIRED : WD_STATUS_SUCCESS;
#endif
}

DWORD DLLCALLCONV OsEventSignal(HANDLE hOsEvent)
{
#if defined(WIN32)
    if (!SetEvent(hOsEvent))
        return WD_SYSTEM_INTERNAL_ERROR;
#elif defined(LINUX)
    wd_linux_event_t *linux_event = (wd_linux_event_t *)hOsEvent;
    pthread_mutex_lock(&linux_event->mutex);
    linux_event->signaled = TRUE;
    pthread_cond_signal(&linux_event->cond);
    pthread_mutex_unlock(&linux_event->mutex);
#endif
    return WD_STATUS_SUCCESS;
}

void DLLCALLCONV PrintDbgMessage(DWORD dwLevel, DWORD dwSection, 
    const char *format, ...)
{
    va_list ap;
    WD_DEBUG_ADD add;
    HANDLE hWD;

    hWD = WD_Open();
    if (hWD==INVALID_HANDLE_VALUE) 
        return;
    BZERO(add);
    add.dwLevel = dwLevel;
    add.dwSection = dwSection;
    va_start(ap, format);
    vsnprintf_s(add.pcBuffer, sizeof(add.pcBuffer), 255, format, ap);
    WD_DebugAdd(hWD, &add);
    va_end(ap);
    WD_Close(hWD);
}

