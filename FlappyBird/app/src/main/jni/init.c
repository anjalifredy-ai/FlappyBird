#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <android/native_window.h>
#include <android/asset_manager.h>
#include <stdbool.h>
#include "init.h"
#include "utils.h"
#include "texture.h"
#include "shaders.h"
#include "audio.h"
#include "game.h"


bool                 g_Initialized = false;
static bool          s_GameSessionLoaded = false;
EGLDisplay           g_EglDisplay = EGL_NO_DISPLAY;
EGLSurface           g_EglSurface = EGL_NO_SURFACE;
EGLContext           g_EglContext = EGL_NO_CONTEXT;
struct android_app* g_App = NULL;

int32_t WindowSizeX = 0;
int32_t WindowSizeY = 0;

GLuint textureProgram;

GLuint colorProgram;
GLuint gPositionHandle;
GLuint gColorHandle;

//fix by Tempa
const char* vertexShaderTexture =
    "attribute vec4 aPosition;"
    "attribute vec2 aTexCoord;"
    "varying vec2 vTexCoord;"
    "void main()"
    "{"
        "gl_Position = aPosition;"
        "vTexCoord = aTexCoord;"
    "}";

const char* fragmentShaderTexture =
    "precision mediump float;"
    "varying vec2 vTexCoord;"
    "uniform sampler2D uTexture;"
    "void main()"
    "{"
        "vec4 texColor = texture2D(uTexture, vTexCoord);"
        "if (texColor.rgb == vec3(0.0))"
        "{"
            "texColor.a = 0.0;"
        "}"
        "gl_FragColor = texColor;"
    "}";

//by vadim
const char* vertexShaderColor =
    "attribute vec4 a_Position;\n"
    "void main() {\n"
    "    gl_Position = a_Position;\n"
    "}\n";

const char* fragmentShaderColor =
    "precision mediump float;\n"
    "uniform vec4 u_Color;\n"
    "void main() {\n"
    "    gl_FragColor = u_Color;\n"
    "}\n";


void Init(struct android_app* app)
{
    if (g_Initialized)
        return;

    g_App = app;
    bool assetsLoaded = false;

    ANativeWindow_acquire(g_App->window);

    // Initialize EGL
    g_EglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g_EglDisplay == EGL_NO_DISPLAY)
    {
        Log("eglGetDisplay(EGL_DEFAULT_DISPLAY) returned EGL_NO_DISPLAY");
        goto fail_release_window;
    }

    if (eglInitialize(g_EglDisplay, 0, 0) != EGL_TRUE)
    {
        Log("eglInitialize() returned with an error");
        goto fail_terminate_display;
    }

    const EGLint egl_attributes[] = {
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
    EGLint num_configs = 0;
    if (eglChooseConfig(g_EglDisplay, egl_attributes, NULL, 0, &num_configs) != EGL_TRUE)
    {
        Log("eglChooseConfig() returned with an error");
        goto fail_terminate_display;
    }
    if (num_configs == 0)
    {
        Log("eglChooseConfig() returned 0 matching config");
        goto fail_terminate_display;
    }

    EGLConfig egl_config;
    eglChooseConfig(g_EglDisplay, egl_attributes, &egl_config, 1, &num_configs);
    EGLint egl_format;
    eglGetConfigAttrib(g_EglDisplay, egl_config, EGL_NATIVE_VISUAL_ID, &egl_format);
    ANativeWindow_setBuffersGeometry(g_App->window, 0, 0, egl_format);

    const EGLint egl_context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    g_EglContext = eglCreateContext(g_EglDisplay, egl_config, EGL_NO_CONTEXT, egl_context_attributes);
    if (g_EglContext == EGL_NO_CONTEXT)
    {
        Log("eglCreateContext() returned EGL_NO_CONTEXT");
        goto fail_terminate_display;
    }

    g_EglSurface = eglCreateWindowSurface(g_EglDisplay, egl_config, g_App->window, NULL);
    if (g_EglSurface == EGL_NO_SURFACE)
    {
        Log("eglCreateWindowSurface() returned EGL_NO_SURFACE");
        goto fail_destroy_context;
    }

    if (eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext) != EGL_TRUE)
    {
        Log("eglMakeCurrent() returned with an error");
        goto fail_destroy_surface;
    }

    eglSwapInterval(g_EglDisplay, 1);

    // Set window size
    WindowSizeX = ANativeWindow_getWidth(g_App->window);
    WindowSizeY = ANativeWindow_getHeight(g_App->window);

    if (!s_GameSessionLoaded)
    {
        if (!InitGame())
        {
            Log("Game not init!");
            goto fail_destroy_gl_assets;
        }
        s_GameSessionLoaded = true;
    }
    else if (!ResumeGameAfterSurfaceLoss())
    {
        Log("Resume game after surface loss failed");
        s_GameSessionLoaded = false;
        goto fail_destroy_gl_assets;
    }
    assetsLoaded = true;

    CreateAudioEngine();

    // Create shader program
    textureProgram = createProgram(vertexShaderTexture, fragmentShaderTexture);
    InitTextureQuadBuffers();

    colorProgram = createProgram(vertexShaderColor, fragmentShaderColor);
    gPositionHandle = glGetAttribLocation(colorProgram, "a_Position");
    gColorHandle = glGetUniformLocation(colorProgram, "u_Color");


    Log("FlappyBird is loaded!");

    g_Initialized = true;
    return;

fail_destroy_gl_assets:
    if (g_EglDisplay != EGL_NO_DISPLAY && g_EglSurface != EGL_NO_SURFACE && g_EglContext != EGL_NO_CONTEXT &&
        eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext) == EGL_TRUE)
    {
        ShutdownTextureQuadBuffers();
        glDeleteProgram(textureProgram);
        textureProgram = 0;
        glDeleteProgram(colorProgram);
        colorProgram = 0;
        if (assetsLoaded)
            ShutdownGame();
    }
    else if (assetsLoaded)
    {
        DestroyAudioPlayer();
        DestroyAudioEngine();
    }

fail_destroy_surface:
    if (g_EglDisplay != EGL_NO_DISPLAY && g_EglSurface != EGL_NO_SURFACE)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(g_EglDisplay, g_EglSurface);
        g_EglSurface = EGL_NO_SURFACE;
    }

fail_destroy_context:
    if (g_EglDisplay != EGL_NO_DISPLAY && g_EglContext != EGL_NO_CONTEXT)
    {
        eglDestroyContext(g_EglDisplay, g_EglContext);
        g_EglContext = EGL_NO_CONTEXT;
    }

fail_terminate_display:
    if (g_EglDisplay != EGL_NO_DISPLAY)
    {
        eglTerminate(g_EglDisplay);
        g_EglDisplay = EGL_NO_DISPLAY;
    }

fail_release_window:
    ANativeWindow_release(g_App->window);
}

void MainLoopStep()
{
    if (g_EglDisplay == EGL_NO_DISPLAY)
        return;

    // Setup display size (every frame to accommodate for window resizing)
    glViewport(0, 0, (int)WindowSizeX, (int)WindowSizeY);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    Render();

    eglSwapBuffers(g_EglDisplay, g_EglSurface);
}

void Shutdown(bool preserve_game_session)
{
    if (!g_Initialized)
        return;

    if (!preserve_game_session)
        s_GameSessionLoaded = false;

    EGLBoolean ctxOk = EGL_FALSE;
    if (g_EglDisplay != EGL_NO_DISPLAY && g_EglSurface != EGL_NO_SURFACE && g_EglContext != EGL_NO_CONTEXT)
        ctxOk = eglMakeCurrent(g_EglDisplay, g_EglSurface, g_EglSurface, g_EglContext);

    if (ctxOk == EGL_TRUE)
        ShutdownTextureQuadBuffers();

    ShutdownGame();

    if (ctxOk == EGL_TRUE)
    {
        glDeleteProgram(textureProgram);
        glDeleteProgram(colorProgram);
    }

    // Cleanup
    if (g_EglDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(g_EglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (g_EglContext != EGL_NO_CONTEXT)
            eglDestroyContext(g_EglDisplay, g_EglContext);

        if (g_EglSurface != EGL_NO_SURFACE)
            eglDestroySurface(g_EglDisplay, g_EglSurface);

        eglTerminate(g_EglDisplay);
    }

    g_EglDisplay = EGL_NO_DISPLAY;
    g_EglContext = EGL_NO_CONTEXT;
    g_EglSurface = EGL_NO_SURFACE;
    ANativeWindow_release(g_App->window);

    g_Initialized = false;
}