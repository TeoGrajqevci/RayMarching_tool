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

} // namespace rmt

#endif // RMT_RENDERING_TEXTURE_2D_H
