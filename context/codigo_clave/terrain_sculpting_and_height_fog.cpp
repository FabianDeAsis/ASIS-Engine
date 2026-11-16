// ==============================================================================
// Snippet Clave: Terreno Procedural 3D, Escultura en Tiempo Real y Niebla Atmosférica (Fase 14)
// ASIS Engine v0.14 - Arquitectura de Terreno y Volumetric Height Fog
// ==============================================================================

#include "Terrain.h"
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

// 1. Raycasting analítico en CPU para intersección de cursor con el terreno
bool ASIS::Terrain::raycast(const glm::vec3& rayOrigin, const glm::vec3& rayDir, 
                            glm::vec3& outHitPos, glm::vec3& outHitNormal, float maxDist) const {
    float halfSize = m_worldSize * 0.5f;
    float stepSize = 0.28f;
    float currentDist = 0.0f;
    bool hadEnteredBounds = false;

    while (currentDist < maxDist) {
        glm::vec3 rayPos = rayOrigin + rayDir * currentDist;

        if (rayPos.x >= -halfSize && rayPos.x <= halfSize &&
            rayPos.z >= -halfSize && rayPos.z <= halfSize) {
            
            hadEnteredBounds = true;
            float terrainH = getHeightAtWorld(rayPos.x, rayPos.z);

            if (rayPos.y <= terrainH) {
                // Refinamiento binario de 10 pasos para precisión milimétrica instantánea (<0.01 ms)
                float t0 = currentDist - stepSize;
                float t1 = currentDist;
                for (int i = 0; i < 10; ++i) {
                    float tMid = (t0 + t1) * 0.5f;
                    glm::vec3 midPos = rayOrigin + rayDir * tMid;
                    if (midPos.y <= getHeightAtWorld(midPos.x, midPos.z)) {
                        t1 = tMid;
                    } else {
                        t0 = tMid;
                    }
                }
                outHitPos = rayOrigin + rayDir * ((t0 + t1) * 0.5f);
                outHitNormal = getNormalAtWorld(outHitPos.x, outHitPos.z);
                return true;
            }
        } else if (hadEnteredBounds && (rayPos.y < m_minHeight - 2.0f)) {
            break;
        }
        currentDist += stepSize;
    }
    return false;
}

// 2. Integración de Colisión Física 3D con Bullet Physics
void ASIS::Terrain::syncPhysics() {
    if (!m_physicsWorld || !m_physicsEnabled) return;

    if (m_terrainBody) {
        m_physicsWorld->removeRigidBodyDirect(m_terrainBody);
        delete m_terrainBody;
        m_terrainBody = nullptr;
    }
    if (m_motionState) {
        delete m_motionState;
        m_motionState = nullptr;
    }
    if (m_terrainShape) {
        delete m_terrainShape;
        m_terrainShape = nullptr;
    }

    updateMinMaxHeight();

    // Heightfield de Bullet Physics basado en el búfer de alturas continuo
    m_terrainShape = new btHeightfieldTerrainShape(
        m_resolution, m_resolution,
        m_heights.data(),
        1.0f,
        m_minHeight, m_maxHeight,
        1, // Y vertical
        PHY_FLOAT,
        false
    );

    float spacing = m_worldSize / static_cast<float>(m_resolution - 1);
    m_terrainShape->setLocalScaling(btVector3(spacing, 1.0f, spacing));

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(0.0f, (m_minHeight + m_maxHeight) * 0.5f, 0.0f));

    m_motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, m_motionState, m_terrainShape, btVector3(0, 0, 0));
    rbInfo.m_friction = 0.85f;
    rbInfo.m_restitution = 0.20f;

    m_terrainBody = new btRigidBody(rbInfo);
    m_physicsWorld->addRigidBodyDirect(m_terrainBody);
}
