#include "../include/Gu.h"
#include <pspkernel.h>
#include <pspdisplay.h>
#include <malloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"
#include "../include/Common.h"

#define FUCKUP printf("Fucked up at line: %d\n", __LINE__)

static unsigned int __attribute__((aligned(64))) list[0x40000];

namespace Gu {
    static unsigned int getMemorySize(unsigned int width, unsigned int height,
        unsigned int psm)
    {
        unsigned int size = width * height;
        switch (psm) {
            case GU_PSM_T4:
                return size / 2;
            case GU_PSM_T8:
                return size;
            case GU_PSM_5650:
            case GU_PSM_5551:
            case GU_PSM_4444:
            case GU_PSM_T16:
                return size * 2;
            case GU_PSM_8888:
            case GU_PSM_T32:
                return size * 4;
            default:
                return 0;
        }
    }

    void* getStaticVramBuffer(unsigned int width, unsigned int height,
        unsigned int psm)
    {
        static unsigned int staticOffset = 0;
        unsigned int memSize = getMemorySize(width, height, psm);
        void* result = (void*)staticOffset;
        staticOffset += memSize;
        return result;
    }

    void* getStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm) {
        void* result = getStaticVramBuffer(width, height, psm);
        return (void*)((unsigned int)result + (unsigned int)sceGeEdramGetAddr());
    }

    void init() {
        void* fbp0 = getStaticVramBuffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
        void* fbp1 = getStaticVramBuffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_8888);
        void* zbp = getStaticVramBuffer(BUFFER_WIDTH, SCREEN_HEIGHT, GU_PSM_4444);

        sceGuInit();

        //Set up buffers
        sceGuStart(GU_DIRECT, list);
        sceGuDrawBuffer(GU_PSM_8888, fbp0, BUFFER_WIDTH);
        sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, fbp1, BUFFER_WIDTH);
        sceGuDepthBuffer(zbp, BUFFER_WIDTH);

        //Set up viewport
        sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
        sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
        sceGuDepthRange(65535, 0);//Use the full buffer for depth testing - buffer is reversed order
        sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        sceGuEnable(GU_SCISSOR_TEST);

        sceGuDepthFunc(GU_GEQUAL); //Depth buffer is reversed, so GEQUAL instead of LEQUAL
        sceGuEnable(GU_DEPTH_TEST); //Enable depth testing

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

    void terminate() {
        sceGuTerm();
    }

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

    void swizzleFast(unsigned char* out, const unsigned char* in, const unsigned int width, const unsigned int height) {
        unsigned int blockx, blocky;
        unsigned int j;

        unsigned int width_blocks = (width / 16);
        unsigned int height_blocks = (height / 8);

        unsigned int src_pitch = (width - 16) / 4;
        unsigned int src_row = width * 8;

        const u8* ysrc = in;
        u32 *dst = (u32*)out;

        for (blocky = 0; blocky < height_blocks; ++blocky) {
            const unsigned char *xsrc = ysrc;
            for (blockx = 0; blockx < width_blocks; ++blockx) {
                const unsigned int *src = (unsigned int*)xsrc;
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
        stbi_set_flip_vertically_on_load(flip);

        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, STBI_rgb_alpha);

        if(!data) {
            FUCKUP;
            return NULL;
        }
        Texture* tex = (Texture*)malloc(sizeof(Texture));
        // FIXME: Allocation could fail
        tex->width = width;
        tex->height = height;
        tex->pW = pow2(width);
        tex->pH = pow2(height);

        size_t size = tex->pH * tex->pW * 4;

        unsigned int *dataBuffer = (unsigned int*)memalign(16, size);
        //FIXME: Allocation could fail -- release resources, return NULL

        copyTextureData(dataBuffer, data, tex->pW, tex->width, tex->height);
        stbi_image_free(data);

        unsigned int* swizzled_pixels = NULL;
        if (vram) {
            swizzled_pixels = (unsigned int *)getStaticVramTexture(tex->pW, tex->pH, GU_PSM_8888);
        } else {
            swizzled_pixels = (unsigned int *)memalign(16, size);
        }
        //FIXME: Allocation could fail -- release resources, return NULL

        swizzleFast((unsigned char*)swizzled_pixels, (const unsigned char*)dataBuffer, tex->pW * 4, tex->pH);
        free(dataBuffer);

        tex->data = swizzled_pixels;
        sceKernelDcacheWritebackInvalidateAll();
        return tex;
    }

    void bindTexture(Texture *tex) {
        if (!tex) {
            FUCKUP;
            return;
        }
        sceGuTexMode(GU_PSM_8888, 0, 0, 1);
        sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
        sceGuTexFilter(GU_NEAREST, GU_NEAREST);
        sceGuTexWrap(GU_REPEAT, GU_REPEAT);
        sceGuTexImage(0, tex->pW, tex->pH, tex->pW, tex->data);
    }

    void copyTextureData(void* dest, const void* src, const int pW, const int width, const int height) {
        for (unsigned int y = 0; y < height; ++y) {
            for (unsigned int x = 0; x < width; x++) {
                ((unsigned int*)dest)[x + y * pW] = ((unsigned int*)src)[x + y * width];
            }
        }
    }

    void applyCamera(const Camera2D* cam) {
        sceGumMatrixMode(GU_VIEW);
        sceGumLoadIdentity();

        ScePspFVector3 v = {cam->x, cam->y, 0};
        //translate
        sceGumTranslate(&v);

        //rotate
        sceGumRotateZ(cam->rot / 180.f * 3.14159f);
        
        //scale

        sceGumMatrixMode(GU_MODEL);
        sceGumLoadIdentity();
    }

    Mesh* createMesh(u32 vcount, u32 index_count) {
        Mesh* mesh = (Mesh*)malloc(sizeof(Mesh));
        if (mesh == 0) {
            printf("Could not allocate memory for a mesh\n");
            return 0;
        }

        mesh->data = memalign(16, sizeof(Vertex) * vcount);
        if (mesh->data == 0) {
            printf("Could not allocate memory for mesh->data\n");
            free(mesh);
            return 0;
        }
        mesh->indices = (u16*)memalign(16, sizeof(u16) * index_count);
        if (mesh->indices == 0) {
            printf("Could not allocate memory for mesh->indices\n");
            free(mesh->data);
            free(mesh);
            return 0;
        }
        mesh->index_count = index_count;
        return mesh;
    }

    void drawMesh(Mesh* mesh) {
        sceGumDrawArray(GU_TRIANGLES, 
            GU_INDEX_16BIT | GU_TEXTURE_32BITF | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
            mesh->index_count, mesh->indices, mesh->data);
    }

    void destroyMesh(Mesh* mesh) {
        free(mesh->data);
        free(mesh->indices);
        free(mesh);
    }

    Vertex createVert(float u, float v, unsigned int colour, float x, float y, float z) {
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
        Sprite* sprite = (Sprite*)malloc(sizeof(Sprite));
        if (sprite == 0) {
            printf("Could not allocate memory for a sprite\n");
            return 0;
        }

        sprite->mesh = createMesh(4, 6);
        if (sprite->mesh == 0) {
            free(sprite);
            printf("Could not create a mesh\n");
            return 0;
        }

        sprite->x = x;
        sprite->y = y;
        sprite->rot = 0.f;
        sprite->sx = sx;
        sprite->sy = sy;
        sprite->layer = 0;
        sprite->mesh->index_count = 6;
        sprite->texture = tex;

        ((Vertex*)sprite->mesh->data)[0] = createVert(0, 0, COLOR_WHITE, -0.25f, -0.25f, 0.f);
        ((Vertex*)sprite->mesh->data)[1] = createVert(0, 1, COLOR_WHITE, -0.25f, 0.25f, 0.f);
        ((Vertex*)sprite->mesh->data)[2] = createVert(1, 1, COLOR_WHITE, 0.25f, 0.25f, 0.f);
        ((Vertex*)sprite->mesh->data)[3] = createVert(1, 0, COLOR_WHITE, 0.25f, -0.25f, 0.f);

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
        sceGumRotateZ(sprite->rot);

        ScePspFVector3 s = {
            .x = sprite->sx,
            .y = sprite->sy,
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
            FUCKUP;
            return;
        }
        float x = (f32)(idx % 16) / atlas->w;
        float y = (f32)(idx / 16) / atlas->h;
        float w = x + 1.f / atlas->w;
        float h = y + 1.f / atlas->h;

        /*
        00
        X0
        */
        buf[0] = x;
        buf[1] = h;

        /*
        X0
        00
        */
        buf[2] = x;
        buf[3] = y;

        /*
        0X
        00
        */
        buf[4] = w;
        buf[5] = y;

        /*
        00
        0X
        */
        buf[6] = w;
        buf[7] = h;
    }

    Tilemap* createTilemap(TextureAtlas atlas, Texture* texture, int sizex, int sizey) {
        Tilemap* tilemap = (Tilemap*)malloc(sizeof(Tilemap));
        if (tilemap == 0) {
            FUCKUP;
            return 0;
        }
        tilemap->tiles = (Tile*)malloc(sizeof(Tile) * sizex * sizey);
        if (tilemap->tiles == 0) {
            FUCKUP;
            free(tilemap);
            return 0;
        }

        tilemap->mesh = createMesh(sizex * sizey * 4, sizex * sizey * 6);
        if (tilemap->mesh == 0) {
            FUCKUP;
            free(tilemap->tiles);
            free(tilemap);
            return 0;
        }

        memset(tilemap->mesh->data, 0, sizeof(Vertex) * sizex * sizey * 4);
        memset(tilemap->mesh->indices, 0, sizeof(u16) * sizex * sizey * 6);
        memset(tilemap->tiles, 0, sizeof(Tile) * sizex * sizey);

        tilemap->atlas = atlas;
        tilemap->texture = texture;
        tilemap->x = 0;
        tilemap->y = 0;
        tilemap->w = sizex;
        tilemap->h = sizey;
        //tilemap->mesh->index_count = tilemap->w * tilemap->h * 6;
        tilemap->scale_x = 16.f;
        tilemap->scale_y = 16.f;

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
        for (int i = 0; i < tilemap->w * tilemap->h; ++i) {
            float buf[8];
            getUvIndex(&tilemap->atlas, buf, tilemap->tiles[i].texture_idx);

            float tx = (float)tilemap->tiles[i].x;
            float ty = (float)tilemap->tiles[i].y;
            float tw = tx + 1.f;
            float th = ty + 1.f;

            ((Vertex*)tilemap->mesh->data)[i * 4 + 0] = createVert(buf[0], buf[1], COLOR_WHITE, tx, ty, 0.f);
            ((Vertex*)tilemap->mesh->data)[i * 4 + 1] = createVert(buf[2], buf[3], COLOR_WHITE, tx, ty, 0.f);
            ((Vertex*)tilemap->mesh->data)[i * 4 + 2] = createVert(buf[4], buf[5], COLOR_WHITE, tx, ty, 0.f);
            ((Vertex*)tilemap->mesh->data)[i * 4 + 3] = createVert(buf[6], buf[7], COLOR_WHITE, tx, ty, 0.f);

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
        for (int i = 0; i < len; i++) {
            char c = str[i];
            Tile tile = {
                .x = i % t->w,
                .y = i / t->w,
                .texture_idx = c
            };
            t->tiles[i] = tile;
        }
    }
}