#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "Shader.h"

// Simple UV sphere. Rendered from the inside -> looks like a sky globe.
class Sky {
public:
    unsigned int VAO, VBO, EBO, prog;
    int indexCount;
    Shader* shader;

    Sky() {
        buildSphere(1.0f, 32, 16);
        const char* vs = R"(
            #version 330 core
            layout(location=0) in vec3 aPos;
            layout(location=1) in vec3 aNormal;
            uniform mat4 view;
            uniform mat4 proj;
            out vec3 vDir;
            void main(){
                vDir = aPos;               // direction from center
                vec4 p = proj * view * vec4(aPos,1.0);
                gl_Position = p.xyww;      // force to far plane
            }
        )";
        const char* fs = R"(
            #version 330 core
            in vec3 vDir;
            out vec4 FragColor;
            void main(){
                vec3 d = normalize(vDir);
                float h = clamp(d.y*0.5+0.5, 0.0, 1.0);
                vec3 top = vec3(0.35,0.55,0.95);  // blue sky
                vec3 mid = vec3(0.75,0.85,1.0);   // horizon haze
                vec3 bot = vec3(0.55,0.45,0.35);  // ground bounce
                vec3 col = h > 0.5 ? mix(mid, top, (h-0.5)*2.0)
                                   : mix(bot, mid, h*2.0);
                FragColor = vec4(col, 1.0);
            }
        )";
        shader = new Shader(vs, fs);
    }

    void draw(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos) {
        glDepthFunc(GL_LEQUAL);
        shader->use();
        // Remove translation from view so sky moves with camera (infinite distance)
        glm::mat4 v = glm::mat4(glm::mat3(view));
        shader->setMat4("view", v);
        shader->setMat4("proj", proj);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glDepthFunc(GL_LESS);
    }

private:
    void buildSphere(float r, int segU, int segV) {
        std::vector<float> verts;
        std::vector<unsigned int> idx;
        for (int y = 0; y <= segV; ++y) {
            float v = (float)y / segV;
            float phi = v * 3.14159265f;
            for (int x = 0; x <= segU; ++x) {
                float u = (float)x / segU;
                float theta = u * 2.0f * 3.14159265f;
                float px = r * sinf(phi) * cosf(theta);
                float py = r * cosf(phi);
                float pz = r * sinf(phi) * sinf(theta);
                verts.insert(verts.end(), { px, py, pz, px, py, pz });
            }
        }
        for (int y = 0; y < segV; ++y) {
            for (int x = 0; x < segU; ++x) {
                unsigned int a = y * (segU + 1) + x;
                unsigned int b = a + segU + 1;
                idx.insert(idx.end(), { a, b, a + 1, b, b + 1, a + 1 });
            }
        }
        indexCount = (int)idx.size();

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
};
