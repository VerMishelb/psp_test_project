#ifndef _Gu_H_
#define _Gu_H_

#define BUFFER_WIDTH 512
#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 272

#define COLOUR_WHITE 0xFFFFFFFF

#include <pspgu.h>
#include <pspgum.h>


struct Vertex {
    float u, v;
    unsigned int colour;
    float x, y, z;
};

extern struct Vertex __attribute__((aligned(16))) square_indexed[4];

struct Texture {
    unsigned int width, height; // Original dimensions
    unsigned int powW, powH; // Nearest 2^N
    void* data; // Raw pixel data
};

struct Camera2D {
    float x, y; // Position
    float rotation; // Rotation
};

struct Camera3D {
    float x, y, z; // Position
    float yaw; // Left-right
    float pitch; // Up-down
};

/** @struct Mesh
 * @brief Holds all data for rendering
 * @param data Vertices
 * @param indices Index array
 * @param index_count Amount of indices
*/
struct Mesh {
    void* data;
    u16* indices;
    u32 index_count;
};

//Holds texture, mesh and transformations
struct Sprite {
    float x, y; // Position
    float rotation; // Self rotation
    float scale_x, scale_y; // Scale
    int layer; // Depth

    Mesh* mesh; // Where to draw
    Texture* texture; // What to draw
};

struct TextureAtlas {
    float rows;
    float columns;
};

struct Tile {
    int x, y, texture_idx;
};

struct Tilemap {
    float x, y, scale_x, scale_y;
    int columns, rows;

    TextureAtlas atlas;
    Texture* texture;
    Tile* tiles;
    Mesh* mesh;
};

namespace Gu {
    /** @param width Buffer width
     * @param height Buffer height
     * @param psm Pixel format to use
     * @return VRAM buffer size
     */
    static unsigned int getMemorySize(unsigned int width, unsigned int height,
        unsigned int psm);

    /** @param width Buffer width
     * @param height Buffer height
     * @param psm Pixel format to use
     * @return VRAM buffer pointer relative to 0
     */
    void* getStaticVramBuffer(unsigned int width, unsigned int height,
        unsigned int psm);

    /** @param width Buffer width
     * @param height Buffer height
     * @param psm Pixel format to use
     * @return VRAM buffer pointer physical
     */
    void* getStaticVramTexture(unsigned int width, unsigned int height,
        unsigned int psm);

    void init();

    void terminate();

    void startFrame();

    void endFrame();

    void resetTransform(float x, float y, float z);

    void swizzleFast(unsigned char* out, const unsigned char* in,
        const unsigned int width, const unsigned int height);

    /** @brief Loads an image file and fills up Texture fields for future use
     * @param filename Path to the image file
     * @param flip Flip vertically or not
     * @param vram Load to VRAM or RAM
     */
    Texture* loadTexture(const char *filename, const int flip, const int vram);

    /** @brief Prepares GU for drawing the texture
     * @param tex
     */
    void bindTexture(Texture *tex);

    /** @param dest Where to copy
     * @param src What to copy
     * @param pW Closest pow2 of the texture width
     * @param width Texture width
     * @param height Texture height
     */
    void copyTextureData(void* dest, const void* src, const int pW,
        const int width, const int height);

    void applyCamera(const Camera2D* cam);

    /** @brief Allocates Mesh fields
     * @param vcount Vertex count
     * @param index_count Indices count
    */
    Mesh* createMesh(u32 vcount, u32 index_count);

    void drawMesh(Mesh* mesh);

    /** @brief Free Mesh fields
     * @param mesh Mesh to free
    */
    void destroyMesh(Mesh* mesh);

    Vertex createVert(float u, float v, unsigned int colour, float x, float y,
        float z);

    Sprite* createSprite(float x, float y, float scale_x, float scale_y, Texture* tex);

    void drawSprite(Sprite* sprite);

    void destroySprite(Sprite* sprite);

    void getUvIndex(TextureAtlas* atlas, float* buf, int idx);

    Tilemap* createTilemap(TextureAtlas atlas, Texture* texture, int sizex,
        int sizey);

    void destroyTilemap(Tilemap* tilemap);

    void drawTilemap(Tilemap* tilemap);

    void buildTilemap(Tilemap* tilemap);

    void drawText(Tilemap *t, const char *str);
}

#endif
