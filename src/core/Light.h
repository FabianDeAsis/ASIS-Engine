#pragma once

#include <string>
#include <glm/glm.hpp>
#include "Shader.h"

namespace ASIS {

struct PointLight {
    std::string name;
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    bool enabled;
    bool castShadows;
    float lightSize; // Radio físico de la fuente de luz para PCSS (Contact Hardening)
    float bias;      // Tolerancia contra acné de sombra

    PointLight(const std::string& n = "LuzPuntual",
               glm::vec3 pos = glm::vec3(0.0f, 2.5f, 0.0f),
               glm::vec3 col = glm::vec3(1.0f, 0.95f, 0.85f),
               float inten = 2.0f)
        : name(n),
          position(pos),
          color(col),
          intensity(inten),
          constant(1.0f),
          linear(0.09f),
          quadratic(0.032f),
          enabled(true),
          castShadows(false),
          lightSize(0.25f),
          bias(0.025f)
    {
    }
};

} // namespace ASIS
