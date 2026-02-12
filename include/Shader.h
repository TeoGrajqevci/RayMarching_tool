#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>
#include <unordered_map>

class Shader {
public:
    GLuint ID;
    Shader(const char* vertexPath, const char* fragmentPath);
    void use() const;
    // Uniform setters
    void setBool(const std::string &name, bool value) const;  
    void setInt(const std::string &name, int value) const;   
    void setFloat(const std::string &name, float value) const;
    void setVec2(const std::string &name, float x, float y) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setIVec3(const std::string &name, int x, int y, int z) const;
    void setVec4(const std::string &name, float x, float y, float z, float w) const;
    void setMat4(const std::string &name, const float* mat) const;
    void setIntArray(const std::string &name, const int* values, int count) const;
    GLint getUniformLocation(const std::string& name) const;
private:
    std::string readFile(const char* filePath);
    GLuint compileShader(GLenum type, const char* source);
    mutable std::unordered_map<std::string, GLint> uniformLocationCache;
};

#endif
