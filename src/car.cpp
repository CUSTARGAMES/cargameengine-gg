#include "Car.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

void Car::initBullet(btDiscreteDynamicsWorld* world, const glm::vec3& startPos) {
    btCollisionShape* shape = new btBoxShape(btVector3(0.9f, 0.4f, 1.9f));

    btVector3 inertia(0,0,0);
    shape->calculateLocalInertia(900.0f, inertia);

    btTransform tr;
    tr.setIdentity();
    tr.setOrigin(btVector3(startPos.x, startPos.y + 1.0f, startPos.z));

    btDefaultMotionState* ms = new btDefaultMotionState(tr);
    btRigidBody::btRigidBodyConstructionInfo ci(900.0f, ms, shape, inertia);
    chassis = new btRigidBody(ci);
    chassis->setActivationState(DISABLE_DEACTIVATION);
    world->addRigidBody(chassis);

    raycaster = new btDefaultVehicleRaycaster(world);
    vehicle = new btRaycastVehicle(btRaycastVehicle::btVehicleTuning(), chassis, raycaster);
    chassis->setActivationState(DISABLE_DEACTIVATION);
    world->addAction(vehicle);

    btVehicleTuning tuning;
    tuning.m_suspensionStiffness = 40.0f;
    tuning.m_suspensionDamping   = 2.3f;
    tuning.m_suspensionCompression = 4.4f;
    tuning.m_maxSuspensionTravelCm = 25.0f;
    tuning.m_frictionSlip        = 5.0f;   // grip — lower = easier to drift
    vehicle->setCoordinateSystem(0, 1, 2); // y up, z forward

    // Wheel positions (relative to chassis center): x, y, z
    // Front-left, Front-right, Rear-left, Rear-right
    btVector3 wheelDir(0, -1, 0);      // suspension direction (down)
    btVector3 wheelAxle(-1, 0, 0);     // rotation axis (left)
    btScalar halfTrack = 0.85f;
    btScalar wb = 1.4f;                // wheelbase half-length
    btScalar connY = -0.3f;
    btScalar connZ = 0.35f;            // suspension rest length

    vehicle->addWheel(btVector3( halfTrack, connY, wb), wheelDir, wheelAxle, connZ, wheelRadius, tuning, true);
    vehicle->addWheel(btVector3(-halfTrack, connY, wb), wheelDir, wheelAxle, connZ, wheelRadius, tuning, true);
    vehicle->addWheel(btVector3( halfTrack, connY,-wb), wheelDir, wheelAxle, connZ, wheelRadius, tuning, false);
    vehicle->addWheel(btVector3(-halfTrack, connY,-wb), wheelDir, wheelAxle, connZ, wheelRadius, tuning, false);

    for (int i = 0; i < vehicle->getNumWheels(); ++i) {
        btWheelInfo& w = vehicle->getWheelInfo(i);
        w.m_suspensionStiffness = tuning.m_suspensionStiffness;
        w.m_wheelsDampingRelaxation = tuning.m_suspensionDamping;
        w.m_wheelsDampingCompression = tuning.m_suspensionCompression;
        w.m_frictionSlip = tuning.m_frictionSlip;
        w.m_rollInfluence = 0.05f;
    }
}

void Car::update(float dt, bool fwd, bool back, bool left, bool right, bool handbrake) {
    // Steering: smooth interpolation toward target
    float target = 0.0f;
    if (left)  target += steerMax;
    if (right) target -= steerMax;
    currentSteer += (target - currentSteer) * glm::min(1.0f, steerSpeed * dt);

    vehicle->setSteeringValue(currentSteer, 0);
    vehicle->setSteeringValue(currentSteer, 1);

    // Engine force on rear wheels (index 2, 3)
    float force = 0.0f;
    if (fwd)  force = -engineForce;
    if (back) force =  engineForce * 0.6f; // reverse weaker
    vehicle->applyEngineForce(force, 2);
    vehicle->applyEngineForce(force, 3);

    // Brakes
    float brake = handbrake ? 100.0f : (fwd || back ? 0.0f : 3.0f);
    for (int i = 0; i < 4; ++i) {
        vehicle->setBrake(brake, i);
    }
    if (handbrake) {
        // handbrake locks rear wheels → drift
        vehicle->setBrake(100.0f, 2);
        vehicle->setBrake(100.0f, 3);
        vehicle->getWheelInfo(2).m_frictionSlip = 1.2f;
        vehicle->getWheelInfo(3).m_frictionSlip = 1.2f;
    } else {
        vehicle->getWheelInfo(2).m_frictionSlip = 5.0f;
        vehicle->getWheelInfo(3).m_frictionSlip = 5.0f;
    }
}

glm::mat4 Car::getTransform() const {
    btTransform t;
    chassis->getMotionState()->getWorldTransform(t);
    btQuaternion q = t.getRotation();
    btVector3 p = t.getOrigin();
    glm::quat gq(q.w(), q.x(), q.y(), q.z());
    glm::mat4 m = glm::mat4_cast(gq);
    m = glm::translate(m, glm::vec3(p.x(), p.y(), p.z()));
    return m;
}

void Car::loadModel(const std::string& path) {
    Assimp::Importer imp;
    const aiScene* sc = imp.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);
    if (!sc || !sc->mRootNode) {
        std::cerr << "Failed to load car: " << imp.GetErrorString() << "\n";
        return;
    }
    std::vector<float> verts;
    std::vector<unsigned int> idx;

    for (unsigned int m = 0; m < sc->mNumMeshes; ++m) {
        aiMesh* mesh = sc->mMeshes[m];
        unsigned int base = (unsigned int)(verts.size() / 6);
        for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
            aiVector3D v = mesh->mVertices[i];
            aiVector3D n = mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0,1,0);
            verts.insert(verts.end(), { v.x, v.y, v.z, n.x, n.y, n.z });
        }
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            aiFace& face = mesh->mFaces[f];
            for (unsigned int k = 0; k < face.mNumIndices; ++k)
                idx.push_back(base + face.mIndices[k]);
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

void Car::draw(const glm::mat4& vp, unsigned int shaderID) {
    if (!VAO) return;
    glm::mat4 model = getTransform();
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "mvp"), 1, GL_FALSE,
                       glm::value_ptr(vp * model));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE,
                       glm::value_ptr(model));
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}
