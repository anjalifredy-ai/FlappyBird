#include "android_native_app_glue.h"
#include <GLES2/gl2.h>
#include "game.h"
#include "utils.h"
#include "texture.h"
#include "audio.h"
#include "init.h"
#include "mouse.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

extern float DeltaTime;

#define SIZE_SPACE_PIPE 3.3f

#define SPACE_BETWEEN_PIPES 5

//buttons
GLuint t_pause;
GLuint t_ok;
GLuint t_menu;
GLuint t_resume;
GLuint t_score;
GLuint t_share;
GLuint t_start;

//sprites
GLuint t_0;
GLuint t_1;
GLuint t_2;
GLuint t_3;
GLuint t_4;
GLuint t_5;
GLuint t_6;
GLuint t_7;
GLuint t_8;
GLuint t_9;

GLuint t_0_small;
GLuint t_1_small;
GLuint t_2_small;
GLuint t_3_small;
GLuint t_4_small;
GLuint t_5_small;
GLuint t_6_small;
GLuint t_7_small;
GLuint t_8_small;
GLuint t_9_small;

GLuint t_background_day;
GLuint t_base;
GLuint t_bronze_medal;
GLuint t_gameover;
GLuint t_gold_medal;
GLuint t_logo;
GLuint t_message;
GLuint t_new;
GLuint t_panel;
GLuint t_pipe_green;
/* pipe-green.png pixel size — used to tile without vertical stretching */
static unsigned s_pipeTexW = 52u;
static unsigned s_pipeTexH = 320u;
GLuint t_platinum_medal;
GLuint t_silver_medal;
GLuint t_sparkle_sheet;
GLuint t_yellowbird_downflap;
GLuint t_yellowbird_midflap;
GLuint t_yellowbird_upflap;

// data
float offsetBase = 0.0f;
int gameSpeed = 0;
int score = 0;
int bestScore = 0;
bool newBestScore = false;

float alpha = 0.0f;
bool fadeOut = false;

enum GameState {
    IDLE,
    FADE_IN,
    FADE_OUT,
    READY_GAME,
    GO_GAME,
    STOP_GAME,
    FADE_OUT_GAMEOVER,
    FALL_BIRD,
    FADE_IN_PANEL
};

enum GameState currentState = IDLE;

typedef struct {
    float x, y;
    float velocity;
    float angle;
    float width;
    float height;
    GLuint currentTexture;
    int frame;
    uint64_t lastFrameTime;
} Bird;

typedef struct {
    float x, y;
    float w, h;
    float offset;
} Pipe;

Bird bird;
Pipe pipes[2];

float ScaleX(float percent)
{
    return (percent / 100.0f) * WindowSizeX;
}

float ScaleY(float percent)
{
    return (percent / 100.0f) * WindowSizeY;
}

float logoY;
float birdY;
float logoVelocity;
float birdVelocity;
uint64_t timeAnimBirdForLogo;
GLuint curTextureAnimBirdForLogo;

GLuint birdTexturesForLogo[3];
int currentFrameForLogo = 0;

float fadeOutAlpha = 255.0f;
float panelY = 0;
GLuint medalTexture = 0;

int Random(int min, int max)
{
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

bool LoadGameTextures(void)
{
    //buttons
    t_pause = LoadTexture("buttons/pause.png");
    t_ok = LoadTexture("buttons/ok.png");
    t_menu = LoadTexture("buttons/menu.png");
    t_resume = LoadTexture("buttons/resume.png");
    t_score = LoadTexture("buttons/score.png");
    t_share = LoadTexture("buttons/share.png");
    t_start = LoadTexture("buttons/start.png");

    //sprites
    t_0 = LoadTexture("sprites/0.png");
    t_1 = LoadTexture("sprites/1.png");
    t_2 = LoadTexture("sprites/2.png");
    t_3 = LoadTexture("sprites/3.png");
    t_4 = LoadTexture("sprites/4.png");
    t_5 = LoadTexture("sprites/5.png");
    t_6 = LoadTexture("sprites/6.png");
    t_7 = LoadTexture("sprites/7.png");
    t_8 = LoadTexture("sprites/8.png");
    t_9 = LoadTexture("sprites/9.png");

    t_0_small = LoadTexture("sprites/0_small.png");
    t_1_small = LoadTexture("sprites/1_small.png");
    t_2_small = LoadTexture("sprites/2_small.png");
    t_3_small = LoadTexture("sprites/3_small.png");
    t_4_small = LoadTexture("sprites/4_small.png");
    t_5_small = LoadTexture("sprites/5_small.png");
    t_6_small = LoadTexture("sprites/6_small.png");
    t_7_small = LoadTexture("sprites/7_small.png");
    t_8_small = LoadTexture("sprites/8_small.png");
    t_9_small = LoadTexture("sprites/9_small.png");

    t_background_day = LoadTexture("sprites/background-day.png");
    t_base = LoadTexture("sprites/base.png");
    t_bronze_medal = LoadTexture("sprites/bronze-medal.png");
    t_gameover = LoadTexture("sprites/gameover.png");
    t_gold_medal = LoadTexture("sprites/gold-medal.png");
    t_logo = LoadTexture("sprites/logo.png");
    t_message = LoadTexture("sprites/message.png");
    t_new = LoadTexture("sprites/new.png");
    t_panel = LoadTexture("sprites/panel.png");
    {
        unsigned pw = 0u, ph = 0u;
        t_pipe_green = LoadTextureSized("sprites/pipe-green.png", &pw, &ph);
        if (t_pipe_green && pw > 0u && ph > 0u)
        {
            s_pipeTexW = pw;
            s_pipeTexH = ph;
        }
    }
    t_platinum_medal = LoadTexture("sprites/platinum-medal.png");
    t_silver_medal = LoadTexture("sprites/silver-medal.png");
    t_sparkle_sheet = LoadTexture("sprites/sparkle-sheet.png");
    t_yellowbird_downflap = LoadTexture("sprites/yellowbird-downflap.png");
    t_yellowbird_midflap = LoadTexture("sprites/yellowbird-midflap.png");
    t_yellowbird_upflap = LoadTexture("sprites/yellowbird-upflap.png");

    return true;
}

static void RefreshDerivedTextureHandles(void)
{
    birdTexturesForLogo[0] = t_yellowbird_downflap;
    birdTexturesForLogo[1] = t_yellowbird_midflap;
    birdTexturesForLogo[2] = t_yellowbird_upflap;

    switch (bird.frame % 3)
    {
    case 0: bird.currentTexture = t_yellowbird_downflap; break;
    case 1: bird.currentTexture = t_yellowbird_midflap; break;
    case 2: bird.currentTexture = t_yellowbird_upflap; break;
    }

    curTextureAnimBirdForLogo = birdTexturesForLogo[currentFrameForLogo % 3];

    if (score >= 40) medalTexture = t_platinum_medal;
    else if (score >= 30) medalTexture = t_gold_medal;
    else if (score >= 20) medalTexture = t_silver_medal;
    else if (score >= 10) medalTexture = t_bronze_medal;
    else medalTexture = 0;
}

bool ResumeGameAfterSurfaceLoss(void)
{
    if (!LoadGameTextures())
        return false;

    gameSpeed = WindowSizeX / 135;
    RefreshDerivedTextureHandles();
    return true;
}

bool InitGame()
{
    if (!LoadGameTextures())
        return false;

    bird.x = ScaleX(18.52f);
    bird.y = ScaleY(20.f);
    bird.velocity = 0.0f;
    bird.angle = 0.0f;
    bird.width = ScaleX(11.11f);
    bird.height = ScaleY(4.17f);
    bird.currentTexture = t_yellowbird_midflap;
    bird.frame = 0;
    bird.lastFrameTime = 0;


    pipes[0].x = ScaleX(100.f);
    pipes[0].y = ScaleY(37.5f);
    pipes[0].w = ScaleX(15.f);
    pipes[0].h = ScaleY(37.5f);
    pipes[0].offset = Random(ScaleY(-SPACE_BETWEEN_PIPES), ScaleY(SPACE_BETWEEN_PIPES));

    pipes[1].x = ScaleX(100.f) + ScaleX(60.f);
    pipes[1].y = ScaleY(37.5f);
    pipes[1].w = ScaleX(15.f);
    pipes[1].h = ScaleY(37.5f);
    pipes[1].offset = Random(ScaleY(-SPACE_BETWEEN_PIPES), ScaleY(SPACE_BETWEEN_PIPES));


    logoY = ScaleY(20.83f);
    birdY = ScaleY(20.83f);
    logoVelocity = 1.1f;
    birdVelocity = 1.1f;

    timeAnimBirdForLogo = getTickCount();
    curTextureAnimBirdForLogo = t_yellowbird_midflap;

    birdTexturesForLogo[0] = t_yellowbird_downflap;
    birdTexturesForLogo[1] = t_yellowbird_midflap;
    birdTexturesForLogo[2] = t_yellowbird_upflap;

    panelY = ScaleY(100.f);

    newBestScore = false;

    //game speed
    gameSpeed = WindowSizeX / 135; // 1080 / 135 = 8

    //load best score
    char filePath[256];
    snprintf(filePath, sizeof(filePath), "%s/save.txt", g_App->activity->internalDataPath);

    FILE* file = fopen(filePath, "r");
    if (file != NULL)
    {
        int loadbestscore;
        fscanf(file, "%d", &loadbestscore);
        fclose(file);

        bestScore = loadbestscore;
    }

    return true;
}

float MoveTowards(float current, float target, float maxDelta)
{
    if (fabs(target - current) <= maxDelta)
    {
        return target;
    }
    return current + (target > current ? maxDelta : -maxDelta);
}

void AnimateBird()
{
    uint64_t currentTime = getTickCount();
    if (currentTime - bird.lastFrameTime > 100)
    {
        bird.lastFrameTime = currentTime;
        bird.frame = (bird.frame + 1) % 3;
        switch (bird.frame)
        {
        case 0: bird.currentTexture = t_yellowbird_downflap; break;
        case 1: bird.currentTexture = t_yellowbird_midflap; break;
        case 2: bird.currentTexture = t_yellowbird_upflap; break;
        }
    }
}

void ApplyGravity()
{
    float dt = DeltaTime * 60.0f;
    bird.velocity += 0.65f * dt;
    bird.y += bird.velocity * dt;

    float targetAngle = bird.velocity > 0 ? 90.0f : -30.0f;
    bird.angle = MoveTowards(bird.angle, targetAngle, 2.0f * dt);

    if (bird.angle > 90.0f) bird.angle = 90.0f;
}

void Jump()
{
    bird.velocity = -13.5f;
    bird.angle = -30.0f;
}

bool CheckCollision()
{
    // detect collision with pipes
    for (int i = 0; i < 2; i++)
    {
        // upper pipe
        float topPipeX = pipes[i].x;
        float topPipeY = pipes[i].y + pipes[i].offset - (bird.height * SIZE_SPACE_PIPE);
        float topPipeWidth = pipes[i].w;
        float topPipeHeight = -(pipes[i].h + pipes[i].offset - (bird.height * SIZE_SPACE_PIPE));

        // lower pipe
        float bottomPipeX = pipes[i].x;
        float bottomPipeY = pipes[i].y + pipes[i].offset;
        float bottomPipeWidth = pipes[i].w;
        float bottomPipeHeight = pipes[i].h - pipes[i].offset;

        // checking collision for upper pipe
        if (bird.x < topPipeX + topPipeWidth &&
            bird.x + bird.width > topPipeX &&
            bird.y < topPipeY &&
            bird.y + bird.height > topPipeY + topPipeHeight)
        {
            return true;
        }

        // checking collision for lower pipe
        if (bird.x < bottomPipeX + bottomPipeWidth &&
            bird.x + bird.width > bottomPipeX &&
            bird.y < bottomPipeY + bottomPipeHeight &&
            bird.y + bird.height > bottomPipeY)
        {
            return true;
        }

        // checking space between pipes
        float gapStartY = topPipeY + topPipeHeight;
        float gapEndY = bottomPipeY;

        if (bird.x < bottomPipeX + bottomPipeWidth &&
            bird.x + bird.width > bottomPipeX &&
            bird.y + bird.height > gapStartY &&
            bird.y < gapEndY)
        {
            // collision not detect for bird
            continue;
        }
    }

    // ground collision
    float baseHeight = ScaleY(75.f);
    if (bird.y + bird.width > baseHeight)
    {
        return true;
    }

    // sky collision
    if (bird.y <= 0)
    {
        return true;
    }

    return false;
}

void RenderBird()
{
    RenderTexturePro(bird.currentTexture, bird.x, bird.y, bird.width, bird.height, bird.angle);
}

void RenderPipes()
{
    const float gap = bird.height * SIZE_SPACE_PIPE;
    const float texRatio = (float)s_pipeTexH / (float)(s_pipeTexW ? s_pipeTexW : 52u);
    const float naturalH = pipes[0].w * texRatio;

    for (int i = 0; i < 2; i++)
    {
        /* lower pipe: tile downward from the gap; UV top→bottom 0…1 like one long repeating strip */
        float H_lo = pipes[i].h - pipes[i].offset;
        float y = pipes[i].y + pipes[i].offset;
        float rem = H_lo;
        while (rem > 0.0001f && naturalH > 0.0001f)
        {
            float slice = rem < naturalH ? rem : naturalH;
            float v1 = slice / naturalH;
            RenderTextureUV(t_pipe_green, pipes[i].x, y, pipes[i].w, slice, 0.f, 0.f, 1.f, v1);
            y += slice;
            rem -= slice;
        }

        /* upper pipe: same tiling as lower but each segment flipped 180° (swap V: rim toward gap) */
        float H_up = pipes[i].h + pipes[i].offset - gap;
        rem = H_up;
        y = pipes[i].y - pipes[i].h;
        while (rem > 0.0001f && naturalH > 0.0001f)
        {
            float slice = rem < naturalH ? rem : naturalH;
            float vf = slice / naturalH;
            RenderTextureUV(t_pipe_green, pipes[i].x, y, pipes[i].w, slice, 0.f, vf, 1.f, 0.f);
            y += slice;
            rem -= slice;
        }
    }
}

void UpdateBirdTextureForLogo()
{
    uint64_t currentTime = getTickCount();
    if (currentTime - timeAnimBirdForLogo > 100)
    {
        timeAnimBirdForLogo = currentTime;
        currentFrameForLogo = (currentFrameForLogo + 1) % 3;
        curTextureAnimBirdForLogo = birdTexturesForLogo[currentFrameForLogo];
    }
}

void RenderScoreLeft(int score, float x, float y, float digitWidth, float digitHeight)
{
    char scoreStr[10];
    sprintf(scoreStr, "%d", score);
    int len = strlen(scoreStr);

    for (int i = 0; i < len; i++)
    {
        int digit = scoreStr[i] - '0';
        GLuint texture = 0;

        switch (digit)
        {
        case 0: texture = t_0; break;
        case 1: texture = t_1; break;
        case 2: texture = t_2; break;
        case 3: texture = t_3; break;
        case 4: texture = t_4; break;
        case 5: texture = t_5; break;
        case 6: texture = t_6; break;
        case 7: texture = t_7; break;
        case 8: texture = t_8; break;
        case 9: texture = t_9; break;
        }

        RenderTexture(texture, x + i * digitWidth, y, digitWidth, digitHeight);
    }
}

void RenderScoreCenter(int score, float x, float y, float digitWidth, float digitHeight)
{
    char scoreStr[10];
    sprintf(scoreStr, "%d", score);
    int len = strlen(scoreStr);
    RenderScoreLeft(score, x - (len - 1) / 2.f * digitWidth, y, digitWidth, digitHeight);
}

void RenderSmallScoreLeft(int score, float x, float y, float digitWidth, float digitHeight)
{
    char scoreStr[10];
    sprintf(scoreStr, "%d", score);
    int len = strlen(scoreStr);

    for (int i = 0; i < len; i++)
    {
        int digit = scoreStr[i] - '0';
        GLuint texture = 0;

        switch (digit)
        {
        case 0: texture = t_0_small; break;
        case 1: texture = t_1_small; break;
        case 2: texture = t_2_small; break;
        case 3: texture = t_3_small; break;
        case 4: texture = t_4_small; break;
        case 5: texture = t_5_small; break;
        case 6: texture = t_6_small; break;
        case 7: texture = t_7_small; break;
        case 8: texture = t_8_small; break;
        case 9: texture = t_9_small; break;
        }

        RenderTexture(texture, x + i * digitWidth, y, digitWidth, digitHeight);
    }
}

void RenderSmallScoreRight(int score, float x, float y, float digitWidth, float digitHeight)
{
    char scoreStr[10];
    sprintf(scoreStr, "%d", score);
    int len = strlen(scoreStr);
    RenderSmallScoreLeft(score, x - len * digitWidth, y, digitWidth, digitHeight);
}

void Render()
{
    float dt = DeltaTime * 60.0f;

    //background
    RenderTexture(t_background_day, 0, 0, ScaleX(100.f), ScaleY(95.83f));

    //cycle base texture
    if (currentState != STOP_GAME && currentState != FADE_OUT_GAMEOVER && currentState != FALL_BIRD && currentState != FADE_IN_PANEL)
    {
        offsetBase -= gameSpeed * dt;
    }

    RenderTexture(t_base, offsetBase, ScaleY(75.f), ScaleX(100.f), ScaleY(25.f));

    if (offsetBase < 0)
    {
        RenderTexture(t_base, ScaleX(100.f) + offsetBase, ScaleY(75.f), ScaleX(100.f), ScaleY(25.f));
    }

    if (offsetBase <= -ScaleX(100.f))
    {
        offsetBase = 0.0f;
    }

    logoY += logoVelocity * dt;
    birdY += birdVelocity * dt;

    if (logoY > ScaleY(20.83f) + 25 || logoY < ScaleY(20.83f) - 25) {
        logoVelocity = -logoVelocity;
    }

    if (birdY > ScaleY(20.83f) + 25 || birdY < ScaleY(20.83f) - 25) {
        birdVelocity = -birdVelocity;
    }

    if (currentState == IDLE || currentState == FADE_IN)
    {
        RenderTexture(t_logo, ScaleX(15.f), logoY, ScaleX(55.56f), ScaleY(5.21f));

        UpdateBirdTextureForLogo();

        RenderTexture(curTextureAnimBirdForLogo, ScaleX(75), birdY, bird.width, bird.height);

        if (ButtonBump(t_start, ScaleX(10.f), ScaleY(65.f), ScaleX(35.f), ScaleY(6.f)))
        {
            //later
            //PlayAudio("audio/click_sound.ogg");
            currentState = FADE_IN;
        }
        
        // button SCORE
        if (ButtonBump(t_score, ScaleX(55.f), ScaleY(65.f), ScaleX(35.f), ScaleY(6.f)))
        {
            //later
            //PlayAudio("audio/click_sound.ogg");
        }
    }
    else if (currentState == FADE_OUT || currentState == READY_GAME) //Ready?
    {
        RenderBird();

        RenderTexture(t_message, ScaleX(10.f), ScaleY(9.f), ScaleX(80.f), ScaleY(50.f));
        if (Button(0, 0, ScaleX(100.f), ScaleY(100.f)))
        {
            currentState = GO_GAME;
        }
    }
    else if (currentState == GO_GAME)
    {
        ApplyGravity();
        AnimateBird();

        for (int i = 0; i < 2; i++)
        {
            pipes[i].x -= gameSpeed * dt;
            if (pipes[i].x < -ScaleX(15.f))
            {
                pipes[i].x = ScaleX(115.f);
                pipes[i].offset = Random(ScaleY(-SPACE_BETWEEN_PIPES), ScaleY(SPACE_BETWEEN_PIPES));
            }

            if (bird.x + (bird.width / 2) >= pipes[i].x + pipes[i].w &&
                bird.x + (bird.width / 2) <= pipes[i].x + pipes[i].w + gameSpeed * dt)
            {
                score++;
                PlayAudio("audio/point.mp3");
            }
        }

        if (CheckCollision())
        {
            currentState = STOP_GAME;
            PlayAudio("audio/hit.mp3");
        }

        if (IsClick(0, 0, ScaleX(100.f), ScaleY(100.f)))
        {
            Jump();
            PlayAudio("audio/wing.mp3");
        }

        RenderPipes();
        RenderBird();

        if (score > 0)
            RenderScoreCenter(score, ScaleX(45.f), ScaleY(7.f), ScaleX(8.f), ScaleY(5.f));
    }
    else if (currentState == STOP_GAME)
    {
        if (score > bestScore)
        {
            bestScore = score;
            newBestScore = true;

            char filePath[256];
            snprintf(filePath, sizeof(filePath), "%s/save.txt", g_App->activity->internalDataPath);

            FILE* file = fopen(filePath, "w");
            if (file != NULL)
            {
                fprintf(file, "%d", bestScore);
                fclose(file);
            }
        }
        currentState = FADE_OUT_GAMEOVER;
    }
    else if (currentState == FADE_OUT_GAMEOVER)
    {
        fadeOutAlpha -= 5.0f * dt;
        if (fadeOutAlpha <= 0)
        {
            fadeOutAlpha = 0.0f;
            currentState = FALL_BIRD;
            PlayAudio("audio/die.mp3");
        }

        RenderPipes();
        RenderBird();

        uint32_t color = 0x00FFFFFF | ((uint32_t)(int)fadeOutAlpha << 24);
        CreateBox(color, 0, 0, ScaleX(100.f), ScaleY(100.f));
    }
    else if (currentState == FALL_BIRD)
    {
        ApplyGravity();
        RenderPipes();
        RenderBird();

        if (bird.y + bird.height >= ScaleY(75.f) - bird.height)
        {
            bird.y = ScaleY(75.f) - bird.height;
            currentState = FADE_IN_PANEL;
        }
    }
    else if (currentState == FADE_IN_PANEL)
    {
        RenderPipes();
        RenderBird();

        panelY = MoveTowards(panelY, ScaleY(30), 20.0f * dt);
        RenderTexture(t_panel, ScaleX(15.f), panelY, ScaleX(70.f), ScaleY(17.5f));

        // Render default score
        RenderSmallScoreRight(score, ScaleX(78), panelY + ScaleY(5), ScaleX(4), ScaleY(3));

        // Render best score
        RenderSmallScoreRight(bestScore, ScaleX(78), panelY + ScaleY(11.5f), ScaleX(4), ScaleY(3));

        if (newBestScore)
        {
            RenderTexture(t_new, ScaleX(56.f), panelY + ScaleY(9.f), ScaleX(10.f), ScaleY(1.8f));
        }


        RenderTexture(t_gameover, ScaleX(17.5f), ScaleY(18.f), ScaleX(65.f), ScaleY(6.f));

        // Render medal
        if (score >= 40) medalTexture = t_platinum_medal;
        else if (score >= 30) medalTexture = t_gold_medal;
        else if (score >= 20) medalTexture = t_silver_medal;
        else if (score >= 10) medalTexture = t_bronze_medal;
        else medalTexture = 0;

        if (medalTexture)
        {
            RenderTexture(medalTexture, ScaleX(22.f), panelY + ScaleY(6.f), ScaleX(15.f), ScaleY(7.f));
        }

        // button OK
        if (ButtonBump(t_ok, ScaleX(10.f), ScaleY(65.f), ScaleX(35.f), ScaleY(6.f)))
        {
            //later
            //PlayAudio("audio/click_sound.ogg");

            //Reset
            currentState = IDLE;
            score = 0;

            bird.x = ScaleX(18.52f);
            bird.y = ScaleY(20.f);
            bird.velocity = 0.0f;
            bird.angle = 0.0f;
            bird.width = ScaleX(11.11f);
            bird.height = ScaleY(4.17f);
            bird.currentTexture = t_yellowbird_midflap;
            bird.frame = 0;
            bird.lastFrameTime = 0;


            pipes[0].x = ScaleX(100.f);
            pipes[0].y = ScaleY(37.5f);
            pipes[0].w = ScaleX(15.f);
            pipes[0].h = ScaleY(37.5f);
            pipes[0].offset = Random(ScaleY(-SPACE_BETWEEN_PIPES), ScaleY(SPACE_BETWEEN_PIPES));

            pipes[1].x = ScaleX(100.f) + ScaleX(60.f);
            pipes[1].y = ScaleY(37.5f);
            pipes[1].w = ScaleX(15.f);
            pipes[1].h = ScaleY(37.5f);
            pipes[1].offset = Random(ScaleY(-SPACE_BETWEEN_PIPES), ScaleY(SPACE_BETWEEN_PIPES));

            panelY = ScaleY(100);

            fadeOutAlpha = 255.0f;

            newBestScore = false;
        }

        // button SHARE
        if (ButtonBump(t_share, ScaleX(55), ScaleY(65), ScaleX(35), ScaleY(6)))
        {
            //later
            //PlayAudio("audio/click_sound.ogg");
        }
    }

    if (currentState == FADE_IN)
    {
        alpha += 5.0f * dt;
        if (alpha >= 255)
        {
            alpha = 255.0f;
            currentState = FADE_OUT;
        }
    }
    else if (currentState == FADE_OUT)
    {
        alpha -= 5.0f * dt;
        if (alpha <= 0)
        {
            alpha = 0.0f;
            currentState = READY_GAME;
        }
    }

    // render black screen
    if (currentState == FADE_IN || currentState == FADE_OUT)
    {
        uint32_t color = 0x00000000 | ((uint32_t)(int)alpha << 24);
        CreateBox(color, 0, 0, ScaleX(100), ScaleY(100));
    }
}

bool ButtonBump(GLuint textureid, float posX, float posY, float width, float height)
{
    bool released = false;

    if (mouse.isReleased)
    {
        if (IsMouseInSquare(mouse.x, mouse.y, posX, posY, width, height))
        {
            released = true;
        }
    }

    if (released) { posY += ScaleY(1); }

    RenderTexture(textureid, posX, posY, width, height);

    return released;
}

bool Button(float posX, float posY, float width, float height)
{
    bool released = false;

    if (mouse.isReleased)
    {
        if (IsMouseInSquare(mouse.x, mouse.y, posX, posY, width, height))
        {
            released = true;
        }
    }

    return released;
}

bool IsClick(float posX, float posY, float width, float height)
{
    bool down = false;

    if (mouse.isDown)
    {
        if (IsMouseInSquare(mouse.x, mouse.y, posX, posY, width, height))
        {
            down = true;
        }
    }

    return down;
}

void ShutdownGame()
{
    DestroyAudioPlayer();
    DestroyAudioEngine();

    // Delete textures
    glDeleteTextures(1, &t_pause);
    glDeleteTextures(1, &t_ok);
    glDeleteTextures(1, &t_menu);
    glDeleteTextures(1, &t_resume);
    glDeleteTextures(1, &t_score);
    glDeleteTextures(1, &t_share);
    glDeleteTextures(1, &t_start);

    glDeleteTextures(1, &t_0);
    glDeleteTextures(1, &t_1);
    glDeleteTextures(1, &t_2);
    glDeleteTextures(1, &t_3);
    glDeleteTextures(1, &t_4);
    glDeleteTextures(1, &t_5);
    glDeleteTextures(1, &t_6);
    glDeleteTextures(1, &t_7);
    glDeleteTextures(1, &t_8);
    glDeleteTextures(1, &t_9);

    glDeleteTextures(1, &t_0_small);
    glDeleteTextures(1, &t_1_small);
    glDeleteTextures(1, &t_2_small);
    glDeleteTextures(1, &t_3_small);
    glDeleteTextures(1, &t_4_small);
    glDeleteTextures(1, &t_5_small);
    glDeleteTextures(1, &t_6_small);
    glDeleteTextures(1, &t_7_small);
    glDeleteTextures(1, &t_8_small);
    glDeleteTextures(1, &t_9_small);

    glDeleteTextures(1, &t_background_day);
    glDeleteTextures(1, &t_base);
    glDeleteTextures(1, &t_bronze_medal);
    glDeleteTextures(1, &t_gameover);
    glDeleteTextures(1, &t_gold_medal);
    glDeleteTextures(1, &t_logo);
    glDeleteTextures(1, &t_message);
    glDeleteTextures(1, &t_new);
    glDeleteTextures(1, &t_panel);
    glDeleteTextures(1, &t_pipe_green);
    glDeleteTextures(1, &t_platinum_medal);
    glDeleteTextures(1, &t_silver_medal);
    glDeleteTextures(1, &t_sparkle_sheet);
    glDeleteTextures(1, &t_yellowbird_downflap);
    glDeleteTextures(1, &t_yellowbird_midflap);
    glDeleteTextures(1, &t_yellowbird_upflap);
}