#include "rmt/rendering/Texture2D.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

namespace rmt {

namespace {

struct CachedMaterialTexture {
    GLuint textureId;
    int width;
    int height;

    CachedMaterialTexture() : textureId(0), width(0), height(0) {}
};

std::unordered_map<std::string, CachedMaterialTexture> gMaterialTextureCache;

std::string normalizeTexturePath(const std::string& path) {
    std::string normalized = path;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    return normalized;
}

std::string lowerExtension(const std::string& path) {
    const std::string::size_type dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return std::string();
    }

    std::string ext = path.substr(dot);
    for (std::size_t i = 0; i < ext.size(); ++i) {
        ext[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(ext[i])));
    }
    return ext;
}

bool isSupportedMaterialTextureExtension(const std::string& extension) {
    return extension == ".png" ||
           extension == ".jpg" ||
           extension == ".jpeg" ||
           extension == ".bmp" ||
           extension == ".tga" ||
           extension == ".ppm";
}

bool loadMaterialTextureFromDisk(const std::string& normalizedPath, CachedMaterialTexture& outTexture) {
    int width = 0;
    int height = 0;
    int components = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(normalizedPath.c_str(), &width, &height, &components, STBI_rgb_alpha);
    if (data == nullptr) {
        return false;
    }

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 width,
                 height,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    outTexture.textureId = textureId;
    outTexture.width = width;
    outTexture.height = height;
    return true;
}

} // namespace

Texture::Texture() : id(0), loaded(false), intensity(1.0f) {
    filePath = "";
}

bool Texture::loadHDR(const char* path) {
    if (id != 0) {
        glDeleteTextures(1, &id);
        id = 0;
    }
    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);
    if (!data) {
        std::cerr << "Failed to load HDR image: " << path << std::endl;
        return false;
    }
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, 
                 nrComponents == 3 ? GL_RGB : GL_RGBA, GL_FLOAT, data);
    stbi_image_free(data);
    filePath = path;
    loaded = true;
    return true;
}

void Texture::bind(GLenum textureUnit) const {
    glActiveTexture(textureUnit);
    glBindTexture(GL_TEXTURE_2D, id);
}

bool isSupportedMaterialTextureFile(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    return isSupportedMaterialTextureExtension(lowerExtension(normalizeTexturePath(path)));
}

bool acquireMaterialTexture(const std::string& path, MaterialTextureInfo& outInfo) {
    outInfo.textureId = 0;
    outInfo.width = 0;
    outInfo.height = 0;

    const std::string normalizedPath = normalizeTexturePath(path);
    if (!isSupportedMaterialTextureFile(normalizedPath)) {
        return false;
    }

    std::unordered_map<std::string, CachedMaterialTexture>::iterator it = gMaterialTextureCache.find(normalizedPath);
    if (it != gMaterialTextureCache.end()) {
        if (it->second.textureId == 0) {
            return false;
        }
        outInfo.textureId = it->second.textureId;
        outInfo.width = it->second.width;
        outInfo.height = it->second.height;
        return true;
    }

    CachedMaterialTexture cachedTexture;
    if (!loadMaterialTextureFromDisk(normalizedPath, cachedTexture)) {
        gMaterialTextureCache[normalizedPath] = cachedTexture;
        std::cerr << "Failed to load material texture: " << normalizedPath << std::endl;
        return false;
    }

    gMaterialTextureCache[normalizedPath] = cachedTexture;
    outInfo.textureId = cachedTexture.textureId;
    outInfo.width = cachedTexture.width;
    outInfo.height = cachedTexture.height;
    return true;
}

void releaseMaterialTextureCache() {
    for (std::unordered_map<std::string, CachedMaterialTexture>::iterator it = gMaterialTextureCache.begin();
         it != gMaterialTextureCache.end();
         ++it) {
        if (it->second.textureId != 0) {
            glDeleteTextures(1, &it->second.textureId);
            it->second.textureId = 0;
        }
    }
    gMaterialTextureCache.clear();
}

} // namespace rmt
