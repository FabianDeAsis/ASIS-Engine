#include "DayNightCycle.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <random>

namespace ASIS {

const float PI = 3.14159265358979323846f;

DayNightCycle::DayNightCycle()
    : m_timeOfDay(12.0f),
      m_cycleEnabled(true),
      m_dayDurationSeconds(180.0f),
      m_sunLatitude(35.0f),
      m_sunAzimuth(45.0f),
      m_sunElevation(50.0f),
      m_sunDirection(glm::normalize(glm::vec3(0.5f, 0.7f, 0.5f))),
      m_moonDirection(-m_sunDirection),
      m_sunColor(1.0f, 0.96f, 0.88f),
      m_sunIntensity(1.2f),
      m_ambientColor(0.24f, 0.27f, 0.35f),
      m_ambientIntensity(1.0f),
      m_zenithColor(0.18f, 0.42f, 0.82f),
      m_horizonColor(0.70f, 0.82f, 0.92f),
      m_horizonHeight(0.0f),
      m_starsIntensity(0.0f),
      m_weather(WeatherType::CLEAR),
      m_rainIntensity(0.0f),
      m_wetness(0.0f),
      m_lightningFlash(0.0f),
      m_lightningTimer(0.0f)
{
    recalculateOrbit();
}

void DayNightCycle::setTimeOfDay(float time) {
    m_timeOfDay = std::fmod(time + 24.0f, 24.0f);
    recalculateOrbit();
}

void DayNightCycle::setSunLatitude(float lat) {
    m_sunLatitude = glm::clamp(lat, -85.0f, 85.0f);
    recalculateOrbit();
}

void DayNightCycle::setManualSun(float azimuth, float elevation) {
    m_sunAzimuth = azimuth;
    m_sunElevation = elevation;
    float elevRad = glm::radians(m_sunElevation);
    float azimRad = glm::radians(m_sunAzimuth);

    glm::vec3 sunDir(
        std::cos(elevRad) * std::cos(azimRad),
        std::sin(elevRad),
        std::cos(elevRad) * std::sin(azimRad)
    );
    m_sunDirection = -glm::normalize(sunDir);
    m_moonDirection = -m_sunDirection;
}

void DayNightCycle::setWeather(WeatherType weather) {
    m_weather = weather;
    if (weather == WeatherType::CLEAR) {
        m_rainIntensity = 0.0f;
        m_fogDensity = 0.15f;
        m_fogStart = 40.0f;
        m_fogEnd = 550.0f;
    } else if (weather == WeatherType::PARTLY_CLOUDY) {
        m_rainIntensity = 0.0f;
        m_fogDensity = 0.30f;
        m_fogStart = 30.0f;
        m_fogEnd = 450.0f;
    } else if (weather == WeatherType::OVERCAST) {
        m_rainIntensity = 0.0f;
        m_fogDensity = 0.50f;
        m_fogStart = 20.0f;
        m_fogEnd = 320.0f;
    } else if (weather == WeatherType::RAIN) {
        m_rainIntensity = 0.75f;
        m_fogDensity = 0.72f;
        m_fogStart = 15.0f;
        m_fogEnd = 220.0f;
    } else if (weather == WeatherType::STORM) {
        m_rainIntensity = 1.0f;
        m_fogDensity = 0.88f;
        m_fogStart = 8.0f;
        m_fogEnd = 130.0f;
    }
}

void DayNightCycle::update(float deltaTime) {
    if (m_cycleEnabled && m_dayDurationSeconds > 0.0f) {
        float hoursDelta = (deltaTime / m_dayDurationSeconds) * 24.0f;
        m_timeOfDay = std::fmod(m_timeOfDay + hoursDelta + 24.0f, 24.0f);
        recalculateOrbit();
    }

    // Actualización de humedad PBR (Wetness) en función del clima
    float targetWetness = (m_weather == WeatherType::RAIN || m_weather == WeatherType::STORM) ? 1.0f : 0.0f;
    float wetSpeed = (targetWetness > m_wetness) ? 0.25f : 0.08f;
    m_wetness = glm::mix(m_wetness, targetWetness, std::min(deltaTime * wetSpeed, 1.0f));

    // Relámpagos dinámicos durante tormenta (STORM)
    if (m_weather == WeatherType::STORM) {
        m_lightningTimer -= deltaTime;
        if (m_lightningTimer <= 0.0f) {
            static std::mt19937 rng(42);
            std::uniform_real_distribution<float> dist(4.0f, 10.0f);
            m_lightningTimer = dist(rng);
            m_lightningFlash = 1.0f;
        } else {
            m_lightningFlash = std::max(0.0f, m_lightningFlash - deltaTime * 3.5f);
        }
    } else {
        m_lightningFlash = 0.0f;
    }
}

void DayNightCycle::recalculateOrbit() {
    // 1. Ángulo solar horario: a las 6:00 = 0 rad (horizonte este), 12:00 = PI/2 (cenit), 18:00 = PI (horizonte oeste)
    float solarAngle = (m_timeOfDay / 24.0f) * 2.0f * PI - (PI * 0.5f);

    float latRad = glm::radians(m_sunLatitude);
    float sinElevation = std::sin(solarAngle) * std::cos(latRad);
    m_sunElevation = std::asin(glm::clamp(sinElevation, -1.0f, 1.0f)) * (180.0f / PI);

    // Azimut solar en grados (0..360)
    m_sunAzimuth = std::fmod((m_timeOfDay / 24.0f) * 360.0f + 90.0f, 360.0f);

    // Vector de dirección de rayos solares en coordenadas de mundo
    float elevRad = glm::radians(m_sunElevation);
    float azimRad = glm::radians(m_sunAzimuth);

    glm::vec3 sunDir(
        std::cos(elevRad) * std::cos(azimRad),
        std::sin(elevRad),
        std::cos(elevRad) * std::sin(azimRad)
    );
    m_sunDirection = -glm::normalize(sunDir);
    m_moonDirection = -m_sunDirection;

    // 2. Transiciones de Colorimetría Atmosférica según Elevación Solar
    // A) Pleno Día (Sol > 12°)
    glm::vec3 daySunCol = glm::vec3(1.0f, 0.96f, 0.88f);
    float daySunIntensity = 1.25f;
    glm::vec3 dayAmbient = glm::vec3(0.24f, 0.28f, 0.38f);
    glm::vec3 dayZenith = glm::vec3(0.16f, 0.38f, 0.78f);
    glm::vec3 dayHorizon = glm::vec3(0.68f, 0.78f, 0.88f);

    // B) Amanecer / Atardecer Dorado-Carmesí (Sol entre -3° y 12°)
    glm::vec3 sunsetSunCol = glm::vec3(1.0f, 0.42f, 0.15f);
    float sunsetSunIntensity = 0.95f;
    glm::vec3 sunsetAmbient = glm::vec3(0.28f, 0.16f, 0.18f);
    glm::vec3 sunsetZenith = glm::vec3(0.10f, 0.14f, 0.32f);
    glm::vec3 sunsetHorizon = glm::vec3(0.98f, 0.48f, 0.20f);

    // C) Plena Noche con Luna (Sol < -5°)
    glm::vec3 nightMoonCol = glm::vec3(0.35f, 0.48f, 0.75f); // Tono lunar azul plateado
    float nightMoonIntensity = 0.25f;
    glm::vec3 nightAmbient = glm::vec3(0.045f, 0.065f, 0.12f);
    glm::vec3 nightZenith = glm::vec3(0.012f, 0.022f, 0.055f);
    glm::vec3 nightHorizon = glm::vec3(0.035f, 0.052f, 0.10f);

    if (m_sunElevation >= 12.0f) {
        // Día soleado completo
        m_sunColor = daySunCol;
        m_sunIntensity = daySunIntensity;
        m_ambientColor = dayAmbient;
        m_ambientIntensity = 1.0f;
        m_zenithColor = dayZenith;
        m_horizonColor = dayHorizon;
        m_starsIntensity = 0.0f;
    } else if (m_sunElevation >= -3.0f) {
        // Transición Amanecer / Atardecer hacia Pleno Día
        float t = (m_sunElevation - (-3.0f)) / 15.0f;
        m_sunColor = glm::mix(sunsetSunCol, daySunCol, t);
        m_sunIntensity = glm::mix(sunsetSunIntensity, daySunIntensity, t);
        m_ambientColor = glm::mix(sunsetAmbient, dayAmbient, t);
        m_ambientIntensity = glm::mix(0.75f, 1.0f, t);
        m_zenithColor = glm::mix(sunsetZenith, dayZenith, t);
        m_horizonColor = glm::mix(sunsetHorizon, dayHorizon, t);
        m_starsIntensity = glm::mix(0.35f, 0.0f, t);
    } else if (m_sunElevation >= -9.0f) {
        // Crepúsculo: de Atardecer a Noche profunda
        float t = (m_sunElevation - (-9.0f)) / 6.0f;
        m_sunColor = glm::mix(nightMoonCol, sunsetSunCol, t);
        m_sunIntensity = glm::mix(nightMoonIntensity, sunsetSunIntensity, t);
        m_ambientColor = glm::mix(nightAmbient, sunsetAmbient, t);
        m_ambientIntensity = glm::mix(0.35f, 0.75f, t);
        m_zenithColor = glm::mix(nightZenith, sunsetZenith, t);
        m_horizonColor = glm::mix(nightHorizon, sunsetHorizon, t);
        m_starsIntensity = glm::mix(1.0f, 0.35f, t);
    } else {
        // Noche profunda
        m_sunColor = nightMoonCol;
        m_sunIntensity = nightMoonIntensity;
        m_ambientColor = nightAmbient;
        m_ambientIntensity = 0.35f;
        m_zenithColor = nightZenith;
        m_horizonColor = nightHorizon;
        m_starsIntensity = 1.0f;
    }

    // Influencia del Clima en el Color y la Iluminación
    if (m_weather == WeatherType::OVERCAST || m_weather == WeatherType::RAIN || m_weather == WeatherType::STORM) {
        float cloudFactor = (m_weather == WeatherType::STORM) ? 0.75f : 0.55f;
        glm::vec3 overcastHorizon = glm::vec3(0.35f, 0.38f, 0.42f);
        glm::vec3 overcastZenith = glm::vec3(0.22f, 0.25f, 0.29f);

        m_sunIntensity *= (1.0f - cloudFactor * 0.75f);
        m_horizonColor = glm::mix(m_horizonColor, overcastHorizon, cloudFactor);
        m_zenithColor = glm::mix(m_zenithColor, overcastZenith, cloudFactor);
        m_starsIntensity *= (1.0f - cloudFactor);
    }

    // Destello de relámpago durante tormentas
    if (m_lightningFlash > 0.01f) {
        glm::vec3 flashCol = glm::vec3(0.85f, 0.95f, 1.0f) * (m_lightningFlash * 2.5f);
        m_ambientColor += flashCol;
        m_zenithColor += flashCol * 0.6f;
        m_horizonColor += flashCol * 0.8f;
    }

    if (!m_customFogColor) {
        m_fogColor = m_horizonColor;
    }
}

std::string DayNightCycle::getTimeString() const {
    int totalMinutes = static_cast<int>(m_timeOfDay * 60.0f);
    int hours = (totalMinutes / 60) % 24;
    int minutes = totalMinutes % 60;

    std::ostringstream ss;
    ss << std::setfill('0') << std::setw(2) << hours << ":"
       << std::setfill('0') << std::setw(2) << minutes;
    return ss.str();
}

} // namespace ASIS
