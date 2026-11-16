#pragma once

#include <glm/glm.hpp>
#include <string>

namespace ASIS {

enum class WeatherType {
    CLEAR = 0,          // Despejado
    PARTLY_CLOUDY = 1,  // Parcialmente Nublado
    OVERCAST = 2,       // Nublado
    RAIN = 3,           // Lluvia
    STORM = 4           // Tormenta Eléctrica con Relámpagos
};

/**
 * @brief Ciclo Dinámico Día / Noche de 24 Horas y Clima Atmosférico (Fase 20)
 * 
 * Gestiona el paso continuo del tiempo, órbitas solar y lunar astronómicas,
 * colorimetría atmosférica dinámica (Amanecer, Mediodía, Atardecer, Noche),
 * efecto horizonte global unificado, estrellas procedimentales y clima interactivo.
 */
class DayNightCycle {
public:
    DayNightCycle();

    // Actualizar paso del tiempo y órbitas astronómicas
    void update(float deltaTime);

    // Getters y Setters de Tiempo (0.0h a 24.0h)
    float getTimeOfDay() const { return m_timeOfDay; }
    void setTimeOfDay(float time);

    bool isCycleEnabled() const { return m_cycleEnabled; }
    void setCycleEnabled(bool enabled) { m_cycleEnabled = enabled; }

    float getDayDurationSeconds() const { return m_dayDurationSeconds; }
    void setDayDurationSeconds(float seconds) { m_dayDurationSeconds = (seconds > 1.0f) ? seconds : 1.0f; }

    float getSunLatitude() const { return m_sunLatitude; }
    void setSunLatitude(float lat);
    void setManualSun(float azimuth, float elevation);

    // Salidas calculadas para Iluminación PBR y Sombras Direccionales
    float getSunAzimuth() const { return m_sunAzimuth; }
    float getSunElevation() const { return m_sunElevation; }
    glm::vec3 getSunDirection() const { return m_sunDirection; }
    glm::vec3 getMoonDirection() const { return m_moonDirection; }

    glm::vec3 getSunColor() const { return m_sunColor; }
    float getSunIntensity() const { return m_sunIntensity; }

    glm::vec3 getAmbientColor() const { return m_ambientColor; }
    float getAmbientIntensity() const { return m_ambientIntensity; }

    // Salidas para la Bóveda Celeste Dinámica y Efecto Horizonte Global
    glm::vec3 getZenithColor() const { return m_zenithColor; }
    glm::vec3 getHorizonColor() const { return m_horizonColor; }
    float getHorizonHeight() const { return m_horizonHeight; }
    void setHorizonHeight(float h) { m_horizonHeight = h; }

    float getStarsIntensity() const { return m_starsIntensity; }
    bool isNight() const { return m_sunElevation < 0.0f; }

    // Clima Atmosférico y Lluvia
    WeatherType getWeather() const { return m_weather; }
    void setWeather(WeatherType weather);

    float getWetness() const { return m_wetness; }
    void setWetness(float w) { m_wetness = glm::clamp(w, 0.0f, 1.0f); }

    float getRainIntensity() const { return m_rainIntensity; }
    void setRainIntensity(float r) { m_rainIntensity = glm::clamp(r, 0.0f, 1.0f); }

    float getLightningFlash() const { return m_lightningFlash; }

    // Neblina Atmosférica Global y Volumétrica (Global Fog Controller)
    bool isFogEnabled() const { return m_fogEnabled; }
    void setFogEnabled(bool e) { m_fogEnabled = e; }

    float getFogStart() const { return m_fogStart; }
    void setFogStart(float s) { m_fogStart = (s >= 0.0f) ? s : 0.0f; }

    float getFogEnd() const { return m_fogEnd; }
    void setFogEnd(float e) { m_fogEnd = (e > m_fogStart) ? e : m_fogStart + 1.0f; }

    float getFogDensity() const { return m_fogDensity; }
    void setFogDensity(float d) { m_fogDensity = glm::clamp(d, 0.0f, 1.0f); }

    glm::vec3 getFogColor() const { return m_fogColor; }
    void setFogColor(const glm::vec3& c) { m_fogColor = c; }
    bool isCustomFogColor() const { return m_customFogColor; }
    void setCustomFogColor(bool custom) { m_customFogColor = custom; }

    // Utilidades
    std::string getTimeString() const;

private:
    float m_timeOfDay;           // 0.0f .. 24.0f (12.0 = Mediodía, 0.0 = Medianoche)
    bool m_cycleEnabled;
    float m_dayDurationSeconds;  // Duración real de 1 día (por defecto: 180s = 3 min)
    float m_sunLatitude;         // Inclinación solar (por defecto: 35 grados)

    // Resultados calculados
    float m_sunAzimuth;
    float m_sunElevation;
    glm::vec3 m_sunDirection;
    glm::vec3 m_moonDirection;
    glm::vec3 m_sunColor;
    float m_sunIntensity;
    glm::vec3 m_ambientColor;
    float m_ambientIntensity;
    glm::vec3 m_zenithColor;
    glm::vec3 m_horizonColor;
    float m_horizonHeight;
    float m_starsIntensity;

    // Clima y Relámpagos
    WeatherType m_weather;
    float m_rainIntensity;
    float m_wetness;
    float m_lightningFlash;
    float m_lightningTimer;

    // Neblina Atmosférica Global (Fase 20)
    bool m_fogEnabled = true;
    float m_fogStart = 20.0f;
    float m_fogEnd = 450.0f;
    float m_fogDensity = 0.45f;
    glm::vec3 m_fogColor = glm::vec3(0.70f, 0.78f, 0.88f);
    bool m_customFogColor = false;

    void recalculateOrbit();
};

} // namespace ASIS
