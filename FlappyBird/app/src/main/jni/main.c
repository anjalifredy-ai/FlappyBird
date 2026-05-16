#include "android_native_app_glue.h"
#include "init.h"
#include "utils.h"
#include "mouse.h"
#include "audio.h"
#include <time.h>
#include <unistd.h>


float DeltaTime = 0.0f;
double g_LastFrameTime = 0.0;

static bool s_AppFocused = true;

static void resetFrameClockAfterResume(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    double t = (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
    g_LastFrameTime = t;
}

static void handleAppCmd(struct android_app* app, int32_t cmd)
{
    switch (cmd)
    {
    case APP_CMD_INIT_WINDOW:
        Init(app);
        break;
    case APP_CMD_TERM_WINDOW:
        Shutdown(true);
        break;
    case APP_CMD_GAINED_FOCUS:
        s_AppFocused = true;
        resetFrameClockAfterResume();
        if (g_Initialized)
            ResumeAudio();
        break;
    case APP_CMD_LOST_FOCUS:
        s_AppFocused = false;
        if (g_Initialized)
            PauseAudio();
        break;
    }
}

int32_t handle_input(struct android_app* app, AInputEvent* event)
{
    int32_t eventType = AInputEvent_getType(event);

    if (eventType == AINPUT_EVENT_TYPE_MOTION) 
    {
        int32_t action = AMotionEvent_getAction(event);
        action &= AMOTION_EVENT_ACTION_MASK;
        int whichsource = action >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        size_t pointerCount = AMotionEvent_getPointerCount(event);
        float x = 0.0f;
        float y = 0.0f;
        bool isDown = false;
        bool isReleased = false;
        bool isMoved = false;
        int index;
    
        for (size_t i = 0; i < pointerCount; ++i)
        {
            x = AMotionEvent_getX(event, i);
            y = AMotionEvent_getY(event, i);
            index = AMotionEvent_getPointerId(event, i);
    
            if (action == AMOTION_EVENT_ACTION_POINTER_DOWN || action == AMOTION_EVENT_ACTION_DOWN)
            {
                int id = index;
                if (action == AMOTION_EVENT_ACTION_POINTER_DOWN && id != whichsource) continue;

                mouse.x = x;
                mouse.y = y;
                mouse.isDown = true;
            }
            else if (action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL)
            {
                int id = index;
                if (action == AMOTION_EVENT_ACTION_POINTER_UP && id != whichsource) continue;
    
                mouse.x = x;
                mouse.y = y;
                mouse.isReleased = true;
            }
            else if (action == AMOTION_EVENT_ACTION_MOVE)
            {
                mouse.isMoved = true;

                mouse.x = x;
                mouse.y = y;
            }
        }

        return 1;
    }
    //else if (eventType == AINPUT_EVENT_TYPE_KEY) //Apparently, this applies to physical keys, including volume control keys.
    //{
    //    return 0;
    //}
    return 0; //Return 0 if you are not processing the event
}

void android_main(struct android_app* state)
{
    state->onInputEvent = handle_input;
    state->onAppCmd = handleAppCmd;

    MouseInit(&mouse);

    while (1)
    {
        int ident;
        int events;
        struct android_poll_source* source;

        // Poll all events until we reach return value TIMEOUT, meaning no events left to process
        while ((ident = ALooper_pollOnce(0, NULL, &events, (void**)&source)) > ALOOPER_POLL_TIMEOUT)
        {
            if (source != NULL) {
                source->process(state, source);
            }

            if (state->destroyRequested != 0) {
                Shutdown(false);
                return;
            }
        }

        if (g_Initialized)
        {
            if (s_AppFocused)
            {
                struct timespec current_timespec;
                clock_gettime(CLOCK_MONOTONIC, &current_timespec);
                double current_time = (double)(current_timespec.tv_sec) + (current_timespec.tv_nsec / 1000000000.0);

                DeltaTime = g_LastFrameTime > 0.0 ? (float)(current_time - g_LastFrameTime) : (1.0f / 60.0f);
                if (DeltaTime > 0.05f) DeltaTime = 0.05f;

                MainLoopStep();
                MouseReset(&mouse);

                g_LastFrameTime = current_time;
            }
            else
            {
                usleep(8000);
            }
        }
    }
}