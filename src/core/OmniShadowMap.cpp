#include "OmniShadowMap.h"
#include <glm/gtc/matrix_transform.hpp>

namespace ASIS {

OmniShadowMap::OmniShadowMap()
    : m_fbo(0),
      m_depthCubemap(0),
      m_resolution(1024),
      m_nearPlane(0.1f),
      m_farPlane(45.0f)
{
}

OmniShadowMap::~OmniShadowMap() {
    destroy();
}

bool OmniShadowMap::init(unsigned int resolution) {
    destroy();
    m_resolution = resolution;

    glGenFramebuffers(1, &m_fbo);
    glGenTextures(1, &m_depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthCubemap);

    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F,
                     m_resolution, m_resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[OmniShadowMap ERROR] Framebuffer status no es completo: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return true;
}

void OmniShadowMap::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_resolution, m_resolution);
}

void OmniShadowMap::bindFace(unsigned int faceIndex) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, m_depthCubemap, 0);
    glViewport(0, 0, m_resolution, m_resolution);
}

void OmniShadowMap::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OmniShadowMap::bindTexture(unsigned int unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthCubemap);
}

glm::mat4 OmniShadowMap::getShadowProjection() const {
    return glm::perspective(glm::radians(90.0f), 1.0f, m_nearPlane, m_farPlane);
}

glm::mat4 OmniShadowMap::getShadowView(unsigned int faceIndex, const glm::vec3& lightPos) const {
    switch (faceIndex) {
    case 0: // +X
        return glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    case 1: // -X
        return glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    case 2: // +Y
        return glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    case 3: // -Y
        return glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    case 4: // +Z
        return glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    case 5: // -Z
        return glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    default:
        return glm::mat4(1.0f);
    }
}

std::vector<glm::mat4> OmniShadowMap::calculateShadowMatrices(const glm::vec3& lightPos) const {
    glm::mat4 shadowProj = getShadowProjection();
    std::vector<glm::mat4> shadowTransforms;
    shadowTransforms.reserve(6);
    for (unsigned int i = 0; i < 6; ++i) {
        shadowTransforms.push_back(shadowProj * getShadowView(i, lightPos));
    }
    return shadowTransforms;
}

void OmniShadowMap::destroy() {
    if (m_depthCubemap) {
        glDeleteTextures(1, &m_depthCubemap);
        m_depthCubemap = 0;
    }
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}

} // namespace ASIS
