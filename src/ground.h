#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Shader.h"

class Ground {
public:
    unsigned int VAO, VBO, prog;
    Shader* shader;
    float halfSize = 500.0f; // 1km x 1km

    Ground() {
        float verts[] = {
            -halfSize, 0, -halfSize,
             halfSize, 0, -halfSize,
             halfSize, 0,  halfSize,
            -halfSize, 0,  halfSize,
        };
        unsigned int idx[] = { 0,1,2, 0,2,3 };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        unsigned int EBO;
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        const char* vs = R"(
            #version 330 core
            layout(location=0) in vec3 aPos;
            uniform mat4 mvp;
            out vec3 wpos;
            void main(){
                wpos = aPos;
                gl_Position = mvp * vec4(aPos,1.0);
            }
        )";
        const char* fs = R"(
            #version 330 core
            in vec3 wpos;
            out vec4 FragColor;
            void main(){
                // 2-unit checker squares
                vec2 g = floor(wpos.xz / 2.0);
                float c = mod(g.x + g.y, 2.0);
                vec3 col = mix(vec3(0.15), vec3(0.85), c);
                // distance fade to hide horizon line
                float d = length(wpos.xz);
                col = mix(col, vec3(0.55,0.6,0.7), smoothstep(150.0, 400.0, d));
                FragColor = vec4(col, 1.0);
            }
        )";
        shader = new Shader(vs, fs);
    }

    void draw(const glm::mat4& vp) {
        shader->use();
        shader->setMat4("mvp", vp);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
};
