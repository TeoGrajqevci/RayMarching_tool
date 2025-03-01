#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h>
#include <string>

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

#endif