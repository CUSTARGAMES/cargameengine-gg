#pragma once
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <string>

struct Car {
    // ---- Bullet ----
    btRigidBody* chassis = nullptr;
    btRaycastVehicle* vehicle = nullptr;
    btVehicleRaycaster* raycaster = nullptr;

    // ---- Graphics ----
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    int indexCount = 0;
    float wheelRadius = 0.35f;

    // ---- Input ----
    float engineForce = 2200.0f;
    float brakeForce  = 45.0f;
    float steerMax    = 0.55f;
    float steerSpeed  = 2.5f;
    float currentSteer = 0.0f;

    void initBullet(btDiscreteDynamicsWorld* world, const glm::vec3& startPos);
    void loadModel(const std::string& path);
    void update(float dt, bool fwd, bool back, bool left, bool right, bool handbrake);
    void draw(const glm::mat4& vp, unsigned int shaderID);

    glm::mat4 getTransform() const;
};
