#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class Shader {
public:
    unsigned int ID;
    Shader(const char* vSrc, const char* fSrc) {
        unsigned int v = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(v, 1, &vSrc, nullptr);
        glCompileShader(v);
        check(v, "VERTEX");

        unsigned int f = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(f, 1, &fSrc, nullptr);
        glCompileShader(f);
        check(f, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, v);
        glAttachShader(ID, f);
        glLinkProgram(ID);
        checkProgram();

        glDeleteShader(v);
        glDeleteShader(f);
    }
    void use() { glUseProgram(ID); }
    void setMat4(const std::string& n, const glm::mat4& m) {
        glUniformMatrix4fv(glGetUniformLocation(ID, n.c_str()), 1, GL_FALSE, glm::value_ptr(m));
    }
    void setVec3(const std::string& n, const glm::vec3& v) {
        glUniform3fv(glGetUniformLocation(ID, n.c_str()), 1, glm::value_ptr(v));
    }
    void setFloat(const std::string& n, float v) {
        glUniform1f(glGetUniformLocation(ID, n.c_str()), v);
    }
private:
    void check(unsigned int s, const char* t) {
        int ok; char log[1024];
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { glGetShaderInfoLog(s, 1024, nullptr, log); std::cerr << t << " ERR: " << log << "\n"; }
    }
    void checkProgram() {
        int ok; char log[1024];
        glGetProgramiv(ID, GL_LINK_STATUS, &ok);
        if (!ok) { glGetProgramInfoLog(ID, 1024, nullptr, log); std::cerr << "LINK ERR: " << log << "\n"; }
    }
};
