#ifndef RMT_RENDERING_TEXTURE_2D_H
#define RMT_RENDERING_TEXTURE_2D_H

#include <glad/glad.h>

#include <string>

namespace rmt {

class Texture {
public:
    GLuint id;
    std::string filePath;
    bool loaded;
    float intensity;

    Texture();
    bool loadHDR(const char* path);
    void bind(GLenum textureUnit) const;
};

struct MaterialTextureInfo {
    GLuint textureId = 0;
    int width = 0;
    int height = 0;
};

bool isSupportedMaterialTextureFile(const std::string& path);
bool acquireMaterialTexture(const std::string& path, MaterialTextureInfo& outInfo);
void releaseMaterialTextureCache();

} // namespace rmt

#endif // RMT_RENDERING_TEXTURE_2D_H
