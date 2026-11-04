#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <btBulletDynamicsCommon.h>
#include <iostream>

#include "Shader.h"
#include "Sky.h"
#include "Ground.h"
#include "Car.h"

static Car* gCar = nullptr;
static bool keys[1024] = { false };

static void keyCB(GLFWwindow*, int key, int, int action, int) {
    if (key < 0 || key >= 1024) return;
    if (action == GLFW_PRESS) keys[key] = true;
    else if (action == GLFW_RELEASE) keys[key] = false;
}

int main() {
    // ---- GLFW ----
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(640, 480, "NFS Mini", nullptr, nullptr);
    if (!win) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSetKeyCallback(win, keyCB);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Glad failed\n"; return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // ---- Bullet ----
    btDefaultCollisionConfiguration* cfg = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* disp = new btCollisionDispatcher(cfg);
    btBroadphaseInterface* bp = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver();
    btDiscreteDynamicsWorld* world = new btDiscreteDynamicsWorld(disp, bp, solver, cfg);
    world->setGravity(btVector3(0, -9.81f, 0));

    // Ground plane
    btCollisionShape* groundShape = new btBoxShape(btVector3(500, 0.5f, 500));
    btTransform groundTr; groundTr.setIdentity();
    groundTr.setOrigin(btVector3(0, -0.5f, 0));
    btDefaultMotionState* groundMS = new btDefaultMotionState(groundTr);
    btRigidBody::btRigidBodyConstructionInfo groundCI(0, groundMS, groundShape, btVector3(0,0,0));
    btRigidBody* groundBody = new btRigidBody(groundCI);
    world->addRigidBody(groundBody);

    // ---- Car ----
    Car car;
    car.initBullet(world, glm::vec3(0, 0.5f, 0));
    car.loadModel("assets/car1.glb");

    // ---- Shaders ----
    // Simple Blinn-Phong for the car
    const char* carVS = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aNormal;
        uniform mat4 mvp;
        uniform mat4 model;
        out vec3 vN;
        out vec3 vWorld;
        void main(){
            vN = mat3(model) * aNormal;
            vWorld = vec3(model * vec4(aPos,1.0));
            gl_Position = mvp * vec4(aPos,1.0);
        }
    )";
    const char* carFS = R"(
        #version 330 core
        in vec3 vN;
        in vec3 vWorld;
        out vec4 FragColor;
        uniform vec3 camPos;
        void main(){
            vec3 N = normalize(vN);
            vec3 L = normalize(vec3(0.4, 1.0, 0.3));
            float d = max(dot(N,L), 0.0);
            vec3 base = vec3(0.85, 0.15, 0.15); // NFS-ish red
            vec3 V = normalize(camPos - vWorld);
            vec3 H = normalize(L+V);
            float spec = pow(max(dot(N,H),0.0), 32.0);
            vec3 col = base * (0.25 + d*0.75) + vec3(1.0)*spec*0.3;
            FragColor = vec4(col, 1.0);
        }
    )";
    Shader carShader(carVS, carFS);

    Sky sky;
    Ground ground;

    // ---- Camera state ----
    glm::vec3 camPos(0, 3, 8);
    float camYaw = 0.0f;

    // ---- Loop ----
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = (float)(now - last);
        last = now;
        if (dt > 0.05f) dt = 0.05f;

        glfwPollEvents();

        // Input
        bool fwd = keys[GLFW_KEY_W] || keys[GLFW_KEY_UP];
        bool back = keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN];
        bool left = keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT];
        bool right = keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT];
        bool hb = keys[GLFW_KEY_SPACE];
        if (keys[GLFW_KEY_ESCAPE]) glfwSetWindowShouldClose(win, 1);

        car.update(dt, fwd, back, left, right, hb);
        world->stepSimulation(dt, 4);

        // Chase camera
        btTransform t;
        car.chassis->getMotionState()->getWorldTransform(t);
        btVector3 p = t.getOrigin();
        btQuaternion q = t.getRotation();
        glm::vec3 carPos(p.x(), p.y(), p.z());
        glm::vec3 fwdDir = glm::normalize(glm::vec3(
            2*(q.x()*q.z() + q.w()*q.y()),
            2*(q.y()*q.z() - q.w()*q.x()),
            1 - 2*(q.x()*q.x() + q.y()*q.y())
        ));
        // Camera position: behind car, slightly up
        glm::vec3 desired = carPos - fwdDir * 7.0f + glm::vec3(0, 3.0f, 0);
        camPos += (desired - camPos) * glm::min(1.0f, 6.0f * dt);
        glm::vec3 target = carPos + fwdDir * 2.0f + glm::vec3(0, 1.0f, 0);

        glm::mat4 view = glm::lookAt(camPos, target, glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(60.0f), 640.0f/480.0f, 0.1f, 1000.0f);
        glm::mat4 vp = proj * view;

        // Render
        glViewport(0, 0, 640, 480);
        glClearColor(0.3f, 0.4f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ground.draw(vp);
        carShader.use();
        carShader.setVec3("camPos", camPos);
        car.draw(vp, carShader.ID);
        sky.draw(view, proj, camPos);

        glfwSwapBuffers(win);
    }

    glfwTerminate();
    return 0;
}
