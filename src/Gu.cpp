#include "../include/Gu.h"
#include <pspkernel.h>
#include <pspdisplay.h>
#include <malloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/Debug.h"
#include "../include/stb_image.h"
#include "../include/Common.h"

// Display-list
static unsigned int __attribute__((aligned(64))) list[0x40000];

struct Vertex __attribute__((aligned(16))) square_indexed[4] = {
    {0.0f, 1.0f, 0xFF00FFFF, -0.25f, -0.25f, -1.0f}, // 0
    {1.0f, 1.0f, 0xFFFF00FF, +0.25f, -0.25f, -1.0f}, // 1
    {1.0f, 0.0f, 0xFFFFFF00, +0.25f, +0.25f, -1.0f}, // 2
    {0.0f, 0.0f, 0xFF000000, -0.25f, +0.25f, -1.0f} // 3
};

namespace Gu {
    static unsigned int getMemorySize(
        unsigned int width, unsigned int height, unsigned int psm)
    {
        unsigned int size = width * height;
        switch (psm) {
            case GU_PSM_T4:
                return size >> 1;
            case GU_PSM_T8:
                return size;
            case GU_PSM_5650:
            case GU_PSM_5551:
            case GU_PSM_4444:
            case GU_PSM_T16:
                return size << 1;
            case GU_PSM_8888:
            case GU_PSM_T32:
                return size << 2;
            default:
                return 0;
        }
    }

    void* getStaticVramBuffer(
        unsigned int width, unsigned int height, unsigned int psm)
    {
        static unsigned int staticOffset = 0;
        unsigned int memSize = getMemorySize(width, height, psm);
        void* result = (void*) staticOffset;
        staticOffset += memSize;
        return result;
    }

    void* getStaticVramTexture(
        unsigned int width, unsigned int height, unsigned int psm)
    {
        void* result = getStaticVramBuffer(width, height, psm);
        return (void*) ((unsigned int) result + (unsigned int) sceGeEdramGetAddr());
    }

    void init() {
        VME_PRINT("Initialising Gu and Input\n");
        // fbp stands for "frame buffer pointer", zbp - depth buffer pointer
        void* fbp0 = getStaticVramBuffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
        void* fbp1 = getStaticVramBuffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
        void* zbp = getStaticVramBuffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_4444);

        sceGuInit();

        // Set up buffers
        sceGuStart(GU_DIRECT, list);
        sceGuDrawBuffer(GU_PSM_8888, fbp0, BUFFER_WIDTH);
        sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, fbp1, BUFFER_WIDTH);
        sceGuDepthBuffer(zbp, BUFFER_WIDTH);

        // Set up viewport
        sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
        sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);

        // Use the full buffer for depth testing - buffer is reversed order
        sceGuDepthRange(65535, 0);
        sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        sceGuEnable(GU_SCISSOR_TEST);

        // Depth buffer is reversed, so GEQUAL instead of LEQUAL
        sceGuDepthFunc(GU_GEQUAL);

        // Enable depth testing
        sceGuEnable(GU_DEPTH_TEST);

        sceGuFrontFace(GU_CW);
        sceGuShadeModel(GU_SMOOTH);
        sceGuEnable(GU_CULL_FACE);
        sceGuEnable(GU_TEXTURE_2D);
        sceGuEnable(GU_CLIP_PLANES);

        sceGuFinish();
        sceGuSync(0, 0);

        sceDisplayWaitVblankStart();
        sceGuDisplay(GU_TRUE);
    }

    // Call when destroying graphics context
    void terminate() {
        sceGuTerm();
    }

    // Call in a loop at the beginning of draw calls
    void startFrame() {
        sceGuStart(GU_DIRECT, list);
    }

    void endFrame() {
        sceGuFinish();
        sceGuSync(0, 0);
        sceDisplayWaitVblankStart();
        sceGuSwapBuffers();
    }

    void resetTransform(float x, float y, float z) {
        ScePspFVector3 v = {x, y, z};
        sceGumLoadIdentity();
        sceGumTranslate(&v);
    }

    void swizzleFast(
        unsigned char* out, const unsigned char* in,
        const unsigned int width, const unsigned int height)
    {
        unsigned int blockx, blocky;
        unsigned int j;

        unsigned int width_blocks = (width / 16);
        unsigned int height_blocks = (height / 8);

        unsigned int src_pitch = (width - 16) / 4;
        unsigned int src_row = width * 8;

        const u8* ysrc = in;
        u32 *dst = (u32*) out;

        for (blocky = 0; blocky < height_blocks; ++blocky) {
            const unsigned char *xsrc = ysrc;
            for (blockx = 0; blockx < width_blocks; ++blockx) {
                const unsigned int *src = (unsigned int*) xsrc;
                for (j = 0; j < 8; ++j) {
                    *(dst++) = *(src++);
                    *(dst++) = *(src++);
                    *(dst++) = *(src++);
                    *(dst++) = *(src++);
                    src += src_pitch;
                }
                xsrc += 16;
            }
            ysrc += src_row;
        }
    }

    Texture* loadTexture(const char* filename, const int flip, const int vram) {
        int width, height, nrChannels;
        stbi_set_flip_vertically_on_load(flip); // Revisit in case of touching targas

        unsigned char* data = stbi_load(
            filename, &width, &height, &nrChannels, STBI_rgb_alpha);

        if (!data) {
            DBG_ERROR("Could not allocate image data\n");
            return NULL;
        }

        Texture* tex = (Texture*) malloc(sizeof(Texture));
        if (tex == NULL) {
            DBG_ERROR("Could not allocate Texture struct\n");
            return NULL;
        }

        tex->width = width;
        tex->height = height;
        tex->powW = pow2(width);
        tex->powH = pow2(height);

        size_t size = tex->powH * tex->powW * 4;

        unsigned int *dataBuffer = (unsigned int *) memalign(16, size);
        if (dataBuffer == NULL) {
            free(tex);
            DBG_ERROR("Could not allocate dataBuffer\n");
            return NULL;
        }

        copyTextureData(dataBuffer, data, tex->powW, tex->width, tex->height);
        stbi_image_free(data);

        unsigned int* swizzled_pixels = NULL;
        if (vram) {
            swizzled_pixels = (unsigned int *) getStaticVramTexture(
                tex->powW, tex->powH, GU_PSM_8888);
        } else {
            swizzled_pixels = (unsigned int *) memalign(16, size);
        }
        if (swizzled_pixels == NULL) {
            free(dataBuffer);
            free(tex);
            DBG_ERROR("Could not allocate swizzled_pixels\n");
            return NULL;
        }

        swizzleFast(
            (unsigned char *) swizzled_pixels,
            (const unsigned char *) dataBuffer, tex->powW * 4, tex->powH);
        free(dataBuffer);

        tex->data = swizzled_pixels;
        sceKernelDcacheWritebackInvalidateAll();
        return tex;
    }

    void bindTexture(Texture *tex) {
        if (!tex) {
            DBG_ERROR("Could not allocate Texture struct\n");
            return;
        }
        
        sceGuTexMode(GU_PSM_8888, 0, 0, 1);
        sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
        sceGuTexFilter(GU_NEAREST, GU_NEAREST);
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        sceGuTexImage(0, tex->powW, tex->powH, tex->powW, tex->data);
    }

    void copyTextureData(
        void* dest, const void* src, const int pW, const int width,
        const int height)
    {
        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                ((unsigned int *) dest)[x + y * pW] =
                    ((unsigned int *) src)[x + y * width];
            }
        }
    }

    void applyCamera(const Camera2D* cam) {
        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();

        ScePspFVector3 v = { cam->x, cam->y, 0 };
        //translate
        sceGumTranslate(&v);

        //rotate
        sceGumRotateZ(cam->rotation / 180.f * 3.1415927f);
        
        //scale

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
    }

    Mesh* createMesh(u32 vertex_count, u32 index_count) {
        Mesh* mesh = (Mesh *) malloc(sizeof(Mesh));
        if (mesh == 0) {
            DBG_ERROR("Could not allocate mesh\n");
            return 0;
        }

        mesh->data = memalign(16, sizeof(Vertex) * vertex_count);
        if (mesh->data == 0) {
            DBG_ERROR("Could not allocate mesh->data\n");
            free(mesh);
            return 0;
        }
        mesh->indices = (u16 *) memalign(16, sizeof(u16) * index_count);
        if (mesh->indices == 0) {
            DBG_ERROR("Could not allocate mesh->indices\n");
            free(mesh->data);
            free(mesh);
            return 0;
        }
        mesh->index_count = index_count;
        return mesh;
    }

    void drawMesh(Mesh* mesh) {
        sceGumDrawArray(
            GU_TRIANGLES, 
            GU_INDEX_16BIT | GU_TEXTURE_32BITF | GU_COLOR_8888 |
            GU_VERTEX_32BITF | GU_TRANSFORM_3D,
            mesh->index_count, mesh->indices, mesh->data);
    }

    void destroyMesh(Mesh* mesh) {
        free(mesh->data);
        free(mesh->indices);
        free(mesh);
    }

    Vertex createVert(
        float u, float v, unsigned int colour, float x, float y, float z)
    {
        Vertex vert = {
            .u = u,
            .v = v,
            .colour = colour,
            .x = x,
            .y = y,
            .z = z
        };
        return vert;
    }

    Sprite* createSprite(float x, float y, float sx, float sy, Texture* tex) {
        Sprite* sprite = (Sprite *) malloc(sizeof(Sprite));
        if (sprite == 0) {
            DBG_ERROR("Could not allocate sprite\n");
            return 0;
        }

        sprite->mesh = createMesh(4, 6);
        if (sprite->mesh == 0) {
            free(sprite);
            DBG_ERROR("Could not create mesh\n");
            return 0;
        }

        sprite->x = x;
        sprite->y = y;
        sprite->rotation = 0.f;
        sprite->scale_x = sx;
        sprite->scale_y = sy;
        sprite->layer = 0;
        sprite->mesh->index_count = 6;
        sprite->texture = tex;

        ((Vertex*) sprite->mesh->data)[0] = createVert(
            0, 1, COLOUR_WHITE, -0.5f, -0.5f, 0.f);
        ((Vertex*) sprite->mesh->data)[1] = createVert(
            1, 1, COLOUR_WHITE, +0.5f, -0.5f, 0.f);
        ((Vertex*) sprite->mesh->data)[2] = createVert(
            1, 0, COLOUR_WHITE, +0.5f, +0.5f, 0.f);
        ((Vertex*) sprite->mesh->data)[3] = createVert(
            0, 0, COLOUR_WHITE, -0.5f, +0.5f, 0.f);

        sprite->mesh->indices[0] = 0;
        sprite->mesh->indices[1] = 1;
        sprite->mesh->indices[2] = 2;

        sprite->mesh->indices[3] = 2;
        sprite->mesh->indices[4] = 3;
        sprite->mesh->indices[5] = 0;

        sceKernelDcacheWritebackInvalidateAll();
        return sprite;
    }

    void drawSprite(Sprite* sprite) {
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        ScePspFVector3 v = {
            .x = sprite->x,
            .y = sprite->y,
            .z = sprite->layer
        };

        sceGumTranslate(&v);
        sceGumRotateZ(sprite->rotation);

        ScePspFVector3 s = {
            .x = sprite->scale_x,
            .y = sprite->scale_y,
            .z = 1.f
        };
        sceGumScale(&s);

        bindTexture(sprite->texture);
        drawMesh(sprite->mesh);
    }

    void destroySprite(Sprite* sprite) {
        destroyMesh(sprite->mesh);
        free(sprite);
    }

    void getUvIndex(TextureAtlas* atlas, float* buf, int idx) {
        if (buf == 0) {
            DBG_ERROR("buf is NULL\n");
            return;
        }

        int row = idx / (int) atlas->columns;
        int column = idx % (int) atlas->columns;

        float tileSizeX = 1.f / ((float) atlas->columns);
        float tileSizeY = 1.f / ((float) atlas->rows);

        float x = (f32) column * tileSizeX;
        float y = (f32) row * tileSizeY;
        float w = x + tileSizeX;
        float h = y + tileSizeY;

        // Works fine, must be something else
        // printf("getUvIndex: idx=%i; rc=%i,%i; xywh=%f, %f, %f, %f;\n",
        // idx, row, column,
        // x, y, w, h);

        /*
        X0
        00
        */
        buf[0] = x;
        buf[1] = y;

        /*
        0X
        00
        */
        buf[2] = w;
        buf[3] = y;

        /*
        00
        0X
        */
        buf[4] = w;
        buf[5] = h;

        /*
        00
        X0
        */
        buf[6] = x;
        buf[7] = h;
    }

    Tilemap* createTilemap(
        TextureAtlas atlas, Texture* texture, int columns, int rows)
    {
        Tilemap* tilemap = (Tilemap*) malloc(sizeof(Tilemap));
        if (tilemap == 0) {
            DBG_ERROR("Could not allocate tilemap\n");
            return 0;
        }
        tilemap->tiles = (Tile*) malloc(sizeof(Tile) * columns * rows);
        if (tilemap->tiles == 0) {
            DBG_ERROR("Could not allocate tiles\n");
            free(tilemap);
            return 0;
        }
        tilemap->mesh = createMesh(columns * rows * 4, columns * rows * 6);
        if (tilemap->mesh == 0) {
            DBG_ERROR("Could not create mesh");
            free(tilemap->tiles);
            free(tilemap);
            return 0;
        }

        memset(tilemap->mesh->data, 0, sizeof(Vertex) * columns * rows);
        memset(tilemap->mesh->indices, 0, sizeof(u16) * columns * rows);
        memset(tilemap->tiles, 0, sizeof(Tile) * columns * rows);

        tilemap->x = 0;
        tilemap->y = 0;
        tilemap->scale_x = 16.f;
        tilemap->scale_y = 16.f;
        tilemap->columns = columns;
        tilemap->rows = rows;

        tilemap->atlas = atlas;
        tilemap->texture = texture;
        tilemap->mesh->index_count = tilemap->columns * tilemap->rows * 6;

        return tilemap;
    }

    void destroyTilemap(Tilemap* tilemap) {
        destroyMesh(tilemap->mesh);
        free(tilemap->tiles);
        free(tilemap);
    }

    void drawTilemap(Tilemap* tilemap) {
        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();

        ScePspFVector3 v = {
            .x = tilemap->x,
            .y = tilemap->y,
            .z = 0.f
        };

        sceGumTranslate(&v);

        ScePspFVector3 v1 = {
            .x = tilemap->scale_x,
            .y = tilemap->scale_y,
            .z = 0.f
        };

        sceGumScale(&v1);

        bindTexture(tilemap->texture);
        drawMesh(tilemap->mesh);
    }

    void buildTilemap(Tilemap* tilemap) {
        for (int i = 0; i < tilemap->columns * tilemap->rows; ++i) {
            float buf[8];
            getUvIndex(&tilemap->atlas, buf, tilemap->tiles[i].texture_idx);

            float tx = (float) tilemap->tiles[i].x;
            float ty = (float) tilemap->tiles[i].y;
            float tw = tx + 1.f;
            float th = ty + 1.f;

            ((Vertex*) tilemap->mesh->data)[i * 4 + 0] = createVert(
                buf[0], buf[1], COLOUR_WHITE, tx, ty, 0.f);
            ((Vertex*) tilemap->mesh->data)[i * 4 + 1] = createVert(
                buf[2], buf[3], COLOUR_WHITE, tx, ty, 0.f);
            ((Vertex*) tilemap->mesh->data)[i * 4 + 2] = createVert(
                buf[4], buf[5], COLOUR_WHITE, tx, ty, 0.f);
            ((Vertex*) tilemap->mesh->data)[i * 4 + 3] = createVert(
                buf[6], buf[7], COLOUR_WHITE, tx, ty, 0.f);

            tilemap->mesh->indices[i * 6 + 0] = (i * 4) + 0;
            tilemap->mesh->indices[i * 6 + 1] = (i * 4) + 1;
            tilemap->mesh->indices[i * 6 + 2] = (i * 4) + 2;
            tilemap->mesh->indices[i * 6 + 3] = (i * 4) + 2;
            tilemap->mesh->indices[i * 6 + 4] = (i * 4) + 3;
            tilemap->mesh->indices[i * 6 + 5] = (i * 4) + 0;
        }
        sceKernelDcacheWritebackInvalidateAll();
    }

    void drawText(Tilemap *t, const char *str) {
        int len = strlen(str);
        for (int i = 0; i < len; ++i) {
            char c = str[i];
            Tile tile = {
                .x = i % t->columns,
                .y = i / t->columns,
                .texture_idx = c
            };
            t->tiles[i] = tile;
        }
    }
}
