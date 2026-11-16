# ⚡ ASIS Engine v0.22

**ASIS Engine** es un motor gráfico 3D híbrido de alto rendimiento desarrollado en **C++20** y **OpenGL 3.3 Core**, diseñado con una interfaz de alta densidad visual inspirada en **Blender 5** y potenciado por la tecnología de geometría virtual **OASIS** (*Occlusion and Adaptive Streaming of Infinite Surfaces*).

---

## 🚀 Arquitectura y Subsistemas Principales

* **Arquitectura OASIS (Geometría Virtual & Streaming):**
  * **Módulo 1 (Terreno CDLOD):** Quadtree adaptativo con *Frustum Culling* con *Guard Band*, *Geomorphing* continuo sin costuras y desplazamiento por *Heightmap* `R32F` en GPU.
  * **Módulo 2 (Modelos 3D & Meshlets):** Partición en *clusters* de 64-128 triángulos con *Bounding Spheres* y *Backface Cone Culling* estilo Nanite.
* **Pipeline de Renderizado PBR & Iluminación Dinámica:**
  * Cook-Torrance Microfacet GGX con 7 ranuras de textura y gestor nativo `.asismat` (14 presets físicos).
  * Iluminación Basada en Imágenes (**IBL**) con soporte nativo para mapas equirrectangulares `.hdr` y `.exr` (`tinyexr`).
  * **Cascaded Shadow Maps (CSM)** de 3 cascadas ortográficas + **Sombras Suaves Físicas (PCSS)** y **Omni-PCSS** en luces puntuales.
  * Post-procesado HDR completo: *Screen-Space Reflections* (**SSR**), *Contact Shadows*, **SSAO** bilateral, **DoF / Bokeh** fotográfico, **Bloom HDR**, **FXAA**, **MSAA 4x/8x**, Auto-Exposición por histograma y mapeo tonal **ACES Filmic / AgX**.
* **Entorno Dinámico, Terreno y Océano 3D:**
  * Ciclo astronómico Día / Noche de 24h con clima interactivo (lluvia, tormenta con relámpagos, humedad PBR y neblina volumétrica).
  * Escultura de terreno en tiempo real y pintura *Splatmap* multicapa PBR.
  * Océano 3D con 4 trenes de ondas trocoidales de **Gerstner**, absorción de Beer-Lambert, espuma Jacobiana y flotabilidad física de Arquímedes.
* **Física 3D, Audio Espacial y Gameplay (PIE):**
  * **Bullet Physics 3** a 60 Hz (cuerpos rígidos, colisionadores de terreno, triggers con eventos Lua `onTriggerEnter`/`Exit`).
  * Controlador de personaje en 1ra y 3ra persona con cápsula física y *Spring Arm*.
  * Motor de audio espacial 3D HRTF con **Miniaudio** y síntesis procedural PCM en tiempo real.
* **Sequencer Cinemático (Fase 22.5), Scripting Lua 5.4 & IA:**
  * *Dope Sheet* y *Graph Editor* de curvas Bézier/Hermite, cámaras de cine con lentes físicos (`18mm`–`135mm`), máscara anamórfica `2.39:1 Scope` y *Bake* de animaciones de IA/Lua.
  * Consola interactiva **Lua REPL** con *Hot-Reload* (`sol2`) y servidor IPC (`127.0.0.1:5555`) conectado a **Gemini AI**.

---

## 🛠️ Compilación (Windows x64)

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\ASIS_Engine.exe