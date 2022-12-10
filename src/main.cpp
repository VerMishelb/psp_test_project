#include <pspkernel.h>
#include <pspdebug.h>
#include <stdio.h>

#include "../include/Callbacks.h"
#include "../include/Gu.h"
#include "../include/Input.h"

#include <math.h>

#define PRINT_POS printf("camera.x = %f, camera.y = %f\n", camera.x, camera.y);
#define MOVE_SPEED 5.f

PSP_MODULE_INFO("Test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

bool gRunning = true;


struct Vertex __attribute__((aligned(16))) square_indexed[4] = {
    {0.0f, 0.0f, 0xFF00FFFF, -0.25f, -0.25f, -1.0f}, // 0
    {1.0f, 0.0f, 0xFFFF00FF, -0.25f, 0.25f, -1.0f}, // 1
    {1.0f, 1.0f, 0xFFFFFF00, 0.25f, 0.25f, -1.0f}, // 2
    {0.0f, 1.0f, 0xFF000000, 0.25f, -0.25f, -1.0f} // 3
};

unsigned short __attribute((aligned(16))) indices[6] = {
    0, 1, 2, 2, 3, 0
};

int main() {
    SetupCallbacks();

    printf("\nInitialising Gu and Input\n");
    Gu::init();
    Input::init();

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumOrtho(
        0.f, 480.f, // X
        0.f, 272.f, // Y
        -10.f, 10.f // Z
    );
    
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    // Texture* tex = Gu::loadTexture("c.png", 0, GU_TRUE);
    // Texture* texTiles = Gu::loadTexture("tiles.png", 0, GU_TRUE);
    // Texture* texBg = Gu::loadTexture("starfield5-ci1.png", 0, GU_TRUE);
    Texture* texFont = Gu::loadTexture("font.png", 0, GU_TRUE);
    Sprite* sprite = Gu::createSprite(240.f, 136.f, 164.f, 164.f, texFont);
    // Sprite* spriteBg = Gu::createSprite(240.f, 136.f, 480.f*2, 480.f*2, texBg);

    Camera2D camera = {
        .x = 0,
        .y = 0,
        .rot = 0.f
    };

    TextureAtlas atlas = {.w = 16, .h = 16 };
    Tilemap* tilemap = Gu::createTilemap(atlas, texFont, 16, 16);
    tilemap->x = 144;
    tilemap->y = 16;

    for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < 16; ++x) {
            Tile tile = {
                .x = x,
                .y = y,
                .texture_idx = x + y * 16
            };
            tilemap->tiles[x + y * 16] = tile;
        }
    }

    // Gu::drawText(tilemap, "Help me please");
    Gu::buildTilemap(tilemap);

    while (gRunning) {
        Input::read();

        if (Input::ctrlData.Buttons & PSP_CTRL_LEFT) {
            camera.x -= MOVE_SPEED;
           // PRINT_POS;
        }
        if (Input::ctrlData.Buttons & PSP_CTRL_RIGHT) {
            camera.x += MOVE_SPEED;
         //   PRINT_POS;
        }
        if (Input::ctrlData.Buttons & PSP_CTRL_UP) {
            camera.y += MOVE_SPEED;
           // PRINT_POS;
        }
        if (Input::ctrlData.Buttons & PSP_CTRL_DOWN) {
            camera.y -= MOVE_SPEED;
          //  PRINT_POS;
        }
        if (Input::ctrlData.Buttons & PSP_CTRL_LTRIGGER) {
            camera.rot += 1.f;
           // PRINT_POS;
        }
        if (Input::ctrlData.Buttons & PSP_CTRL_RTRIGGER) {
            camera.rot -= 1.f;
            //PRINT_POS;
        }
        // if (Input::getStickX() > 0) {
        //     sprite->rot += 0.1f;
        // }
        // if (Input::getStickX() < 0) {
        //     sprite->rot -= 0.1f;
        // }


        Gu::startFrame();

        sceGuDisable(GU_DEPTH_TEST);
        sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
        sceGuEnable(GU_BLEND);

        sceGuClearColor(0xFF202020);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT | GU_STENCIL_BUFFER_BIT);

        Gu::applyCamera(&camera);

        // Gu::drawSprite(spriteBg);
        //Gu::drawSprite(sprite);
        Gu::drawTilemap(tilemap);

        Gu::endFrame();
    }

    Gu::destroyTilemap(tilemap);
    Gu::terminate();
    sceKernelExitGame();
    return 0;
}