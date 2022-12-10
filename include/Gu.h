#ifndef _Gu_H_
#define _Gu_H_

#define BUFFER_WIDTH 512
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

#define COLOR_WHITE 0xFFFFFFFF

#include <pspgu.h>
#include <pspgum.h>

struct Vertex {
    float u, v;
    unsigned int colour;
    float x, y, z;
};

struct Texture {
    unsigned int width, height;
    unsigned int pW, pH;
    void* data;
};

struct Camera2D {
    float x, y;
    float rot;
};

struct Camera3D {
    float x, y, z;
    float yaw, pitch;
};

struct Mesh {
    void* data;
    u16* indices;
    u32 index_count;
};

struct Sprite {
    float x, y, rot, sx, sy;
    int layer;

    Mesh* mesh;
    Texture* texture;
};

struct TextureAtlas {
    float   w, // Number of columns
            h; // Number of rows
};

struct Tile {
    int x, y, texture_idx;
};

struct Tilemap {
    float x, y, scale_x, scale_y;
    int w, h;
    TextureAtlas atlas;
    Texture* texture;
    Tile* tiles;
    Mesh* mesh;
};

namespace Gu {
    static unsigned int getMemorySize(unsigned int width, unsigned int height,
        unsigned int psm);
    void* getStaticVramBuffer(unsigned int width, unsigned int height,
        unsigned int psm);
    void* getStaticVramTexture(unsigned int width, unsigned int height,
        unsigned int psm);
    void init();
    void terminate();
    void startFrame();
    void endFrame();
    void resetTransform(float x, float y, float z);
    void swizzleFast(unsigned char* out, const unsigned char* in,
        const unsigned int width, const unsigned int height);
    Texture* loadTexture(const char* filename, const int flip, const int vram);
    void bindTexture(Texture *tex);
    void copyTextureData(void* dest, const void* src, const int pW,
        const int width, const int height);
    void applyCamera(const Camera2D* cam);
    Mesh* createMesh(u32 vcount, u32 index_count);
    void drawMesh(Mesh* mesh);
    void destroyMesh(Mesh* mesh);
    Vertex createVert(float u, float v, unsigned int colour, float x, float y, float z);
    Sprite* createSprite(float x, float y, float sx, float sy, Texture* tex);
    void drawSprite(Sprite* sprite);
    void destroySprite(Sprite* sprite);
    void getUvIndex(TextureAtlas* atlas, float* buf, int idx);
    Tilemap* createTilemap(TextureAtlas atlas, Texture* texture, int sizex, int sizey);
    void destroyTilemap(Tilemap* tilemap);
    void drawTilemap(Tilemap* tilemap);
    void buildTilemap(Tilemap* tilemap);
    void drawText(Tilemap *t, const char *str);
}

#endif