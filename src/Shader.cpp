#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

std::string Shader::readFile(const char* filePath) {
    std::ifstream file;
    file.open(filePath);
    if(!file.is_open()){
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return "";
    }
    std::stringstream ss;
    ss << file.rdbuf();
    file.close();
    return ss.str();
}

GLuint Shader::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
         char infoLog[512];
         glGetShaderInfoLog(shader, 512, NULL, infoLog);
         std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vShaderCode);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fShaderCode);

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);
    int success;
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if(!success) {
         char infoLog[512];
         glGetProgramInfoLog(ID, 512, NULL, infoLog);
         std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    uniformLocationCache.clear();
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::setBool(const std::string &name, bool value) const {
    glUniform1i(getUniformLocation(name), static_cast<int>(value));
}
void Shader::setInt(const std::string &name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}
void Shader::setFloat(const std::string &name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}
void Shader::setVec2(const std::string &name, float x, float y) const {
    glUniform2f(getUniformLocation(name), x, y);
}
void Shader::setVec3(const std::string &name, float x, float y, float z) const {
    glUniform3f(getUniformLocation(name), x, y, z);
}
void Shader::setIVec3(const std::string &name, int x, int y, int z) const {
    glUniform3i(getUniformLocation(name), x, y, z);
}
void Shader::setVec4(const std::string &name, float x, float y, float z, float w) const {
    glUniform4f(getUniformLocation(name), x, y, z, w);
}
void Shader::setMat4(const std::string &name, const float* mat) const {
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, mat);
}

void Shader::setIntArray(const std::string& name, const int* values, int count) const {
    if (values == nullptr || count <= 0) {
        return;
    }
    glUniform1iv(getUniformLocation(name), count, values);
}

GLint Shader::getUniformLocation(const std::string& name) const {
    const std::unordered_map<std::string, GLint>::const_iterator it = uniformLocationCache.find(name);
    if (it != uniformLocationCache.end()) {
        return it->second;
    }

    const GLint location = glGetUniformLocation(ID, name.c_str());
    uniformLocationCache[name] = location;
    return location;
}
