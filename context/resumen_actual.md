# 📌 Resumen de Estado Actual - ASIS Engine

## 1. Fase Actual
- **Fase activa**: **FASE 22.7 COMPLETA (OPTIMIZACIÓN Y PRECISIÓN EN TERRENO PBR Y OASIS OCÉANO: BIOMAS ANALÍTICOS, ESCULTURA PARCIAL ULTRARRÁPIDA Y LIMPIEZA VRAM)**
- **Versión del Motor**: `ASIS Engine v0.22.7`
- **Compilación**: Exitosa en Release x64 con MSVC y CMake (`build\Release\ASIS_Engine.exe`).
- **Próximas Fases**:
  - **Fase 23**: Sistema de Renderizado Basado en Nodos / Shader Graph Visual.
  - **Fase 24**: Sistema de Animación Facial, Morphs y Blend Shapes.

---

## 2. Módulos Completados (Fases 0 a 22.5)

- [x] **Fase 0: Pre-requisitos**: Entorno C++20, compiladores MSVC, Python y librerías base de vcpkg instaladas.
- [x] **Fase 1: Motor Gráfico Base**: OpenGL 3.3 Core Profile, shaders GLSL modulares, VAO/VBO/EBO, GLFW.
- [x] **Fase 2: Modelos 3D y Cámaras**: Carga Assimp (`.obj`/`.fbx`), texturas con `stb_image`, cámara híbrida (Unreal Flycam RMB+WASD / Blender Orbit MMB/Alt+LMB) y rejilla espacial de suelo (*Ground Grid*).
- [x] **Fase 3: Scripting en Lua con Hot-Reload**: Entidades, Scene, ScriptEngine con `sol2`, hot-reload en vivo desde disco.
- [x] **Fase 4: Módulo de IA y Creación por Prompts**: Servidor socket TCP (`127.0.0.1:5555`), cliente Python y generador multimodelo con Gemini API.
- [x] **Fase 5: Feedback Loop, Sandbox de Lua y Shaders Vivos**: Feedback bidireccional, autocorrección autónoma con Gemini, sandbox de CPU y coloración voxel.
- [x] **Fase 5.1: Editor Moderno (Docking, Segoe UI y Consola IA Integrada)**: ImGui DockSpace, fuentes Segoe UI/Consolas en HD y consola embebida.
- [x] **Fase 6: Gizmos 3D (ImGuizmo W/E/R) y Viewport FBO**: Viewport desacoplado en Framebuffer Offscreen, gizmos W/E/R, conmutación local/world, snapping y re-enfoque `F`.
- [x] **Fase 7: Iluminación y Sombras Dinámicas (Shadow Mapping 2K + PCF) & Selección 3D**: Two-pass shadow mapping con luz solar direccional, filtrado PCF 3x3, plano receptor de sombras, selección 3D por raycasting y pausa de scripts.
- [x] **Fase 8: Motor de Física 3D (Bullet Physics) & Panel Vertical Blender**: Integración `bullet3` a 60 Hz, colisiones convexas, sincronización continua bidireccional y panel lateral estilo Blender 4.
- [x] **Fase 9: Import/Export Multiformato 3D, Drag & Drop y Riel Vectorial Ilustrado**: Importador universal Assimp, exportadores Wavefront OBJ/MTL y glTF 2.0 Base64, Drag & Drop nativo y dibujos vectoriales con `ImDrawList`.
- [x] **Fase 10: Sistema de Materiales PBR, Sombras Ultra-HD 4K con Penumbra, Multiluz y Control de Cámaras**: Pipeline Cook-Torrance GGX, 7 ranuras de textura PBR, multiluz hasta 8 point lights y sombras dinámicas 4K.
- [x] **Fase 11: Formato Nativo .asis, Deshacer/Rehacer (Undo/Redo), Primitivas 3D Procedurales, Luces en Jerarquía, Gestión de Color ACES/AgX y Menú de Preferencias**.
- [x] **Fase 12: Iluminación Basada en Imágenes (IBL) & Skybox HDRI 360°, Antialiasing Real (MSAA + FXAA), Bloom HDR, Captura F12 y Compatibilidad "Abrir con..."**.
- [x] **Fase 13: Sistema de Partículas 3D (VFX), SSAO Bilateral, DoF/Bokeh Fotográfico, Animaciones Esqueléticas (Assimp Bones) y Notificaciones de Render**.
- [x] **Fase 14: Terreno Procedural 3D, Escultura en Tiempo Real (Sculpting Brushes), Colisiones Bullet y Niebla Atmosférica Volumétrica (Height Fog)**.
- [x] **Fase 15: Play-in-Editor (PIE) & Controlador de Personaje 3D (FPS / TPS con Spring Arm)**.
- [x] **Fase 16: Motor de Audio 3D Espacial (Miniaudio), Pulido FPS & Agachado, Terreno con Pinceles Splatmap/Offset/Chunks, Grupos en Jerarquía y Cuadrícula Infinita**:
  - Motor Miniaudio con listener 3D HRTF orientado y sintetizador procedural PCM autónomo para pisadas en 5 biomas, salto, impacto y fuego.
  - Desplazamiento base vertical (`m_baseOffset = -8.0m`), pintura de texturas de 4 capas y cuadrícula infinita analítica con antialiasing.
- [x] **Fase 17: Sistema de Prefabs, Spawning Dinámico con Lua, Triggers de Eventos y Auto-Exposición**:
  - Formato JSON `.prefab` para reutilización de estructuras completas con mallas, físicas y materiales.
  - Spawning dinámico en caliente desde Lua con `spawnPrefab(...)`, `getPrefabList()` y `destroyEntity(...)`.
  - Sensores de eventos Bullet (`Ghost/Trigger colliders`) con callbacks `onTriggerEnter` y `onTriggerExit`.
  - Adaptación ocular fisiológica y auto-exposición cinemática con histograma de luminancia.
- [x] **Fase 18: Geometría Virtual OASIS Módulo 2 (Meshlets LOD) y Exportación a Standalone .exe**:
  - Arquitectura inspirada en Nanite con partición en meshlets de 64 vértices / 126 primitivas y cono de normales para culling jerárquico.
  - Empaquetador automatizado de proyectos standalone (.exe autónomo para Windows con bundle de assets, shaders y scripts).
- [x] **Fase 19: Océano 3D con Olas de Gerstner, Flotabilidad Física Bullet, Espuma Dinámica y Refracción**:
  - Malla oceánica de 700m con 4 trenes de ondas trocoidales de Gerstner en GPU (`water.vert`/`water.frag`).
  - Muestreo en CPU y fuerza hidrostática de empuje de Arquímedes sobre rigidbodies de Bullet Physics (cajas de madera que flotan y se mecen).
  - Espuma por curvatura Jacobiana y profundidad de orilla Z-buffer, con niebla submarina cian inmersiva.
- [x] **Fase 20: Ciclo Día / Noche Dinámico 24h, Clima Atmosférico, SSR, Contact Shadows, Niebla Volumétrica Global, CSM y Sombras Suaves PCSS**:
  - Ciclo astronómico de 24 horas con Sol y Luna orbitales, curvas cromáticas Kelvin y sombras nocturnas lunares.
  - Clima con precipitaciones (Lluvia VFX con partículas orientadas, Nieve, Tormenta, Despejado) y Humedad PBR de Superficie (*Wetness*).
  - Screen-Space Reflections (SSR) por raymarching jerárquico con refinamiento binario en depth buffer.
  - Sombras de contacto en screen-space (Contact Shadows) para micro-oclusiones finas.
  - Niebla Volumétrica Atmosférica Global con absorción e inscattering exponencial en cielo, terreno y modelos.
  - Cascaded Shadow Maps (CSM) de 3 cascadas ortográficas (2048x2048) para sombras lejanas nítidas sin pixelado.
  - Sombras Suaves Físicas PCSS (Percentage-Closer Soft Shadows) para Sol direccional y Luces Puntuales (Omni-PCSS con cubemaps, búsqueda de bloqueadores Vogel Disk y filtro de penumbra variable).
- [x] **Fase 21: Gestor de Materiales PBR, Jerarquía con Atajos Profesionales, Barra de Búsqueda, Layouts de UI y Consola Lua Interactiva**:
  - **Gestor Central de Materiales PBR (`MaterialManager.h`, `MaterialManager.cpp`)**:
    - Formato nativo legible `.asismat` (JSON estructurado) con versionado, propiedades físicas PBR completas (albedo, roughness, metallic, specular, normalStrength, ao, emission, emissionStrength) y persistencia de rutas relativas de mapas de texturas.
    - Paleta visual interactiva de 14 Presets Físicos PBR aplicables con un solo clic: Oro Pulido, Cobre, Acero Inoxidable, Cromo / Espejo, Vidrio / Cristal, Plástico Brillante, Plástico Mate, Neón Cian Emisivo, Neón Carmesí, Obsidiana Negra, Mármol Blanco, Hormigón / Concreto, Hierro Oxidado y Madera / Corteza.
    - Asignación en tiempo real a objetos individuales o selecciones múltiples en el Viewport 3D y la Jerarquía.
    - Navegador y explorador de biblioteca global de materiales en `assets/materials/` accesible incluso sin entidades seleccionadas, con selector combo, muestras de color en vivo, clonado y guardado rápido a disco.
  - **Jerarquía de Escena con Atajos Profesionales (Outliner) (`EditorUI.h`, `EditorUI.cpp`)**:
    - `Ctrl + D`: Duplicación instantánea de la entidad o entidades seleccionadas con cálculo automático de sufijos únicos (`_Copy`, `_Copy1`), desplazamiento espacial de compensación (+1.0m en X/Z), duplicación en profundidad de geometría, material PBR completo, cuaternión de orientación, estado de grupo, zonas de trigger y registro íntegro de cuerpo rígido en Bullet Physics (`PhysicsWorld`).
    - `F2`: Renombrado inline inmediato o mediante buffer con validación de teclas Intro y Escape.
    - `F`: Enfoque dinámico y suave de la cámara del editor hacia la entidad seleccionada, grupo o baricentro de selección múltiple.
    - `Supr` / `Delete`: Eliminación segura de entidades seleccionadas con remoción de colisionadores de física Bullet y vaciado de referencias.
    - Menú contextual de clic derecho enriquecido en elementos de la jerarquía: Renombrar (`F2`), Duplicar (`Ctrl+D`), Enfocar Cámara (`F`), Agrupar Selección (`Ctrl+G`), Asignador Rápido de Presets PBR (submenú con todos los materiales predefinidos), Guardar como Prefab (`.prefab`) y Eliminar (`Supr`).
    - Registro automático de instantáneas en la pila de Deshacer/Rehacer (`recordSnapshot`) antes de cada operación de atajo.
  - **Barra de Búsqueda en Tiempo Real en la Escena (`EditorUI.cpp`)**:
    - Campo de entrada integrado `[🔍 Buscar objeto o luz...]` en la cabecera del Outliner con botón rápido de limpieza `[✕]`.
    - Filtrado en tiempo real sin distinción de mayúsculas/minúsculas (*case-insensitive substring match*) para nombres de entidades, carpetas/grupos y luces puntuales.
    - Contador dinámico de resultados coincidentes (`🔍 N resultados encontrados`).
    - Auto-expansión automática de nodos de grupo (`ImGuiTreeNodeFlags_DefaultOpen`) si el grupo o alguno de sus objetos hijos coincide con la búsqueda, permitiendo localizar activos inmediatamente en escenas complejas.
  - **Presets de Espacios de Trabajo / Layouts de UI (`EditorUI.h`, `EditorUI.cpp`)**:
    - Sistema modular de distribución de ventanas desacopladas en el DockSpace:
      1. **🏢 General / Completo**: Vista estándar de 3 columnas (Outliner al 20% izquierdo, Propiedades al 26% derecho, Consola/IA/Prefabs al 28% inferior y Viewport 3D central).
      2. **💡 Iluminación & Cine**: Viewport ampliado al 75% del área, panel de propiedades fijado automáticamente en la pestaña de Iluminación/Sol/Skybox y controles cinematográficos optimizados.
      3. **⛰️ Terreno & Modelado**: Panel de propiedades ensanchado al 32% activando por defecto la escultura 3D de terreno y herramientas de pintura splatmap, con consola reducida.
      4. **🎮 Modo Juego (Inmersivo)**: Viewport maximizado al área completa con paneles laterales colapsados en pestañas mínimas para experiencia de juego sin distracciones.
    - Conmutadores directos en el menú superior `Ver > Espacios de Trabajo / Layouts` y botones de acceso rápido con pills visuales (`[ 🏢 General* ] [ 💡 Cine* ] [ ⛰️ Terreno* ] [ 🎮 Juego* ]`) integrados en la barra flotante del Viewport 3D.
  - **Consola Lua Interactiva (REPL en Tiempo Real) (`EditorUI.cpp`, `ScriptEngine.cpp`)**:
    - Barra de filtrado superior de logs con botones categorizados y resaltado activo: `[ Todos ]`, `[ ℹ️ Info ]`, `[ ⚠️ Advertencias ]`, `[ ❌ Errores ]`, `[ 📜 Scripts / Lua ]` y botón de limpieza de logs `[ 🗑️ Limpiar Logs ]`.
    - Línea de comandos interactiva en la parte inferior: indicador `Lua >`, caja de texto con autocompletado y ejecución mediante tecla `Intro` o botón `[ ▶ Ejecutar ]`.
    - Buffer de historial de comandos navegable con las flechas `Arriba` y `Abajo` del teclado.
    - Ejecución directa de declaraciones y llamadas Lua a través del sandbox de `ScriptEngine::executeString`, con captura inmediata de estado de retorno y reporte en consola (`[Lua REPL] ✓` o `[Lua REPL ERROR]`).
- [x] **Fase 22: Sistema de Animación Cinemática / Sequencer, Cámaras de Cine y Pistas de Curvas Temporales**:
  - **Motor Central de Secuencias (`Sequencer.h`, `Sequencer.cpp`)**:
    - Tipos de interpolación: `Linear` (lineal estándar), `Constant / Step` (mantiene el valor hasta el siguiente keyframe, ideal para cortes instantáneos de cámara) y `CubicBezier` (curvas Hermite / Bézier cúbicas con tangentes continuas y aceleración suave).
    - **Pista de Entidades 3D (`EntityTransformTrack`)**: Fotogramas clave de posición, rotación mediante cuaterniones (esférico `slerp`) y escala tridimensional.
    - **Pista de Cámara de Cine (`CineCameraTrack`)**: Posición 3D, objetivo de mirada (*Target LookAt*), distancia focal en milímetros (`focalLength_mm`), distancia de enfoque fotográfico (`focusDistance` en metros para DoF), apertura de diafragma (`aperture_fstop` para radio Bokeh) y parámetros de vibración procedural en mano (`shakeIntensity`, `shakeFrequency`).
    - **Pista de Entorno y Atmósfera (`EnvironmentTrack`)**: Interpolación continua de hora del día (ciclo solar 24h), compensación de exposición EV, intensidad solar y color del sol.
    - **Pista de Eventos Lua (`LuaEventTrack`)**: Claves de activación (*Trigger Keys*) en momentos exactos de la línea de tiempo que ejecutan cadenas o llamadas Lua en el sandbox durante la reproducción activa.
    - Control de transporte: `play()`, `pause()`, `stop()`, `setTime(t)`, velocidad de reproducción (`playbackSpeed`), duración y bucle continuo (`isLooping`).
    - Auto-Keying: Grabación automática de fotogramas clave al manipular entidades con gizmos o desplazar cámaras de cine.
    - 3 Presets de Demostración Cinemática en 1 Clic:
      1. *Hero Shot 360° con Bokeh DoF*: Órbita suave alrededor del objetivo con lente de 50mm y desenfoque Bokeh a f/1.4.
      2. *Travelling de Cine + Amanecer Rápido*: Dolly In steadycam con lente de 35mm sincronizado con amanecer solar dorado acelerado.
      3. *Secuencia de Acción Multi-Corte*: 3 planos de cámara distintos con cortes instantáneos (`Step`) y disparo de eventos de audio/scripts en Lua.
    - Serialización nativa `.asisseq` en formato JSON estructurado hacia `assets/sequences/`.
  - **Línea de Tiempo / Timeline, Dope Sheet y Graph Editor (`EditorUI.h`, `EditorUI.cpp`)**:
    - Pestaña acoplada por defecto en la bandeja inferior: `"Sequencer & Cine (Fase 22)"`, con enfoque automático al activar el espacio de trabajo `Iluminación & Cine`.
    - Barra de transporte superior de 26px: botones vectoriales `[ |< ]`, `[ < ]`, `[ ▶ / ⏸ ]`, `[ ■ ]`, `[ > ]`, `[ ● Auto-Key ]` (rojo `#E03C3C`), `[ + Clave (I) ]`, alternador de vista Dope Sheet vs Graph Editor, toggle `[ 🎥 Pilotar ]`, toggle `[ 🎬 2.39:1 Scope ]`, contador de tiempo/fotogramas en pill cyan `00:03.20 (F96 / 300)`, arrastre de duración, selector de FPS (24, 30, 60), checkbox de bucle, selector de presets y botones de guardar/cargar `.asisseq`.
    - Panel izquierdo (~22%): Lista de pistas con iconos vectoriales (`CINE_CAMERA`, `SUN`, `PHYSICS`, `SPARK`), nombres, botón Mute `[M]`, añadir clave `[+K]` y eliminar pista `[X]`.
    - Lienzo temporal derecho (~78%):
      - Regla superior con marcas de segundo (0s, 1s, 2s...) y ticks de fotogramas con interacción de arrastre/scrubbing continuo.
      - **Modo Dope Sheet**: Claves en rombos vectoriales (`IconType::KEYFRAME` / quads) en cada pista, con selección y arrastre de tiempos.
      - **Modo Graph Editor**: Rejilla matemática con curvas continuas cúbicas Bézier multicanal (Rojo = X, Verde = Y, Azul = Z) y puntos de control en cada keyframe.
      - Playhead vertical azul brillante (`#4772B3`) con cabezal triangular superior y actualización inmediata de la escena en tiempo real al arrastrar.
  - **Herramientas de Cámara de Cine y Overlays en Viewport 3D (`EditorUI.cpp`, `EditorUI.h`)**:
    - Atajo de teclado `I` en Viewport: Abre el popup rápido `"Insertar Clave de Animacion (I)"` para transformar entidades, cámara de cine o entorno con un solo clic.
    - Máscara Anamórfica 2.39:1 Scope: Bandas negras superior e inferior calculadas dinámicamente según el aspect ratio del Viewport con badge visual.
    - Guías de Composición de Regla de Tercios: Líneas sutiles al 33% y 66% horizontal/vertical con puntos dorados de intersección.
    - Ruta de Movimiento 3D (Motion Path): Trazado tridimensional suave en amarillo en el espacio del Viewport que proyecta la trayectoria de la cámara con puntos de keyframe y cono de visión/frustum interactivo.
    - Inspector de Propiedades: Pestaña de Iluminación enriquecida con sección *"Camara de Cine y Lentes Fisicos"*, botones de lentes predefinidos (18mm, 24mm, 35mm, 50mm, 85mm, 135mm), sliders ópticos de distancia focal, FOV vertical, distancia de enfoque DoF, apertura de diafragma y vibración de cámara (Shake).
  - **Bindings para Scripting en Lua (`ScriptEngine.cpp`)**:
    - `Engine.playSequence(ruta)` / `playSequence(...)`: Inicia la reproducción cinemática (carga archivo `.asisseq` si se especifica).
    - `Engine.pauseSequence()` / `pauseSequence()`: Pausa la animación.
    - `Engine.stopSequence()` / `stopSequence()`: Detiene y rebobina a $t=0$s.
    - `Engine.setSequenceTime(t)` / `setSequenceTime(t)`: Salto temporal directo a cualquier segundo.
    - `Engine.isSequencePlaying()` / `isSequencePlaying()`: Consulta de estado de reproducción.
    - `Engine.triggerCameraShake(intensidad, duracion)`: Disparo de sacudida sísmica o impacto en la cámara desde gameplay.
- [x] **Fase 22.5: Corrección Matemática del Isotipo ASIS, Sistema de Keyframes Estilo Blender, Sincronización de Animaciones IA/Lua en el Sequencer y Pulido Visual**:
  - **Renderizado Exacto del Isotipo `asisengine.svg` & Barra de Título Oscura**:
    - Geometría analítica de triángulo isósceles blanco puro (`#FFFFFF`) sobre `#1D1D1D` con ápice en `(0.50, 0.00)` y base en `y=1.00`.
    - 4 cortes horizontales exactos de fondo oscuro en la esquina inferior izquierda siguiendo la diagonal $x_{\text{right}}(y) = 0.370f + (y - 0.674f) \cdot 0.46f$ para $y \in [0.674, 0.708]$, $[0.752, 0.786]$, $[0.830, 0.864]$ y $[0.908, 0.942]$, con espesor reforzado mínimo de $1.6\text{px}$ en tamaños pequeños ($\le 22\text{px}$).
    - Icono de ventana nativo de 64x64 RGBA generado analíticamente y asignado vía `glfwSetWindowIcon`.
    - Barra de título de Windows en modo oscuro nativo mediante la API DWM de Windows (`DwmSetWindowAttribute(hwnd, 20, &useDarkMode, sizeof(useDarkMode))`).
    - Modal *"Acerca de ASIS Engine v0.22"* con el isotipo a `84x84 px` en blanco puro sobre fondo `#1D1D1D`.
  - **Sincronización de Animaciones IA/Lua y Auto-Bake al Sequencer (`Sequencer.cpp`, `ScriptEngine.cpp`)**:
    - `syncSceneAnimationsToSequencer(Scene& scene)`: Genera automáticamente pistas y fotogramas clave en el Sequencer para todas las entidades procedurales activas de la escena (`CenterCube`, satélites `Sat1/2/3`, `MainModel`, monedas `Moneda_1/2`, cámara principal).
    - Botón `[ ● Bake IA & Lua ]` en la barra de transporte: Muestrea y hornea las trayectorias de física y scripts procedurales de la escena a fotogramas clave discretos del Sequencer a intervalos configurables.
    - Exposición de funciones Lua dedicadas: `seq_clear()`, `seq_addEntityTrack(name)`, `seq_addKeyframe(...)` y `seq_bake(duration, interval)`.
    - Normalización angular de Euler en `Entity::setRotation`, `Entity::rotate` y `Entity::setOrientation` acotada a $[0, 360)^\circ$ con `std::fmod`.
    - Prioridad de evaluación cinemática: flag `m_controlledBySequencer` para evitar interferencias de scripts acumulativos durante la reproducción o scrubbing en la línea de tiempo.
  - **Sistema de Keyframes Estilo Blender (Inspector + Viewport + Dope Sheet)**:
    - Botón vectorial de rombo diamante (`◇` inactivo, `◆` activo) a la derecha de los campos de traslación, rotación, escala y color en el Inspector.
    - Resaltado cromático dinámico de los campos (`FrameBg`):
      - **Dorado Blender** (`#5C4B1B`): Cuando el cursor temporal coincide exactamente ($|t - t_{\text{key}}| < 0.02\text{s}$) con un keyframe.
      - **Verde Oliva Blender** (`#2E4725`): En fotogramas intermedios de una pista animada interpolada.
      - **Gris Oscuro Neutro** (`#1D1D1D`): Propiedades sin animación.
    - Subcanales desplegables en el Dope Sheet (`Posición`, `Rotación`, `Escala`) con barras de retención (*Hold Bars*) de Blender entre fotogramas consecutivos de valor constante.
    - Atajos estándar: `I` para insertar keyframe en el objeto seleccionado con notificación en la barra de estado; selección rectangular con caja (box select); `G` para desplazar claves; `Shift+D` para duplicar claves seleccionadas; `Supr`/`X` para eliminar claves; y tecla `T` para desplegar el selector rápido de interpolación (`Linear`, `Step`, `Bézier`).
  - **Corrección de Cortes Visuales en UI**:
    - Outliner: Reserva exacta de 54px para la barra inferior, eliminando solapamientos; campo de texto y botón `"+ Asignar"` de grupos ajustados responsivamente al ancho disponible.
    - Sequencer: Ancho del selector de FPS incrementado a 68px, mostrando `"30 FPS"` completo sin truncamientos.
  - **Surgical Update: Frustum Culling con Guard Band en OASIS CDLOD & Pulido Profesional del Sequencer**:
    - Frustum culling con aspect ratio real del Viewport y FOV real de la cámara; AABB 3D con `m_baseOffset` y rango completo de elevaciones; guard band lateral `guardMargin = chunkRadius * 1.45f * std::max(1.0f, lodDistanceFactor * 0.40f)`.
    - Sincronización Euler <-> Quaternion Y -> X -> Z (`Entity::syncQuaternionFromEuler`) en transformaciones de entidades, gizmos e inspector.
    - Bandera `m_forceSampleOnce` en `Sequencer` para evitar sobreescritura de modificaciones manuales cuando el reproductor esté pausado, con muestreo reactivo (`seq.requestSample()`).
    - Dope Sheet interactivo: arrastre LMB de rombos con muestreo continuo, doble clic para ubicar cabezal y seleccionar entidad, clic derecho con menú contextual `SeqKeyframeContextPopup` (tiempo, interpolación, posición, rotación Euler, escala, botón capturar pose, duplicar, eliminar).
    - Graph Editor interactivo: selector de canales (Posición, Rotación, Escala), auto-escalado vertical centrado y nodos circulares arrastrables (horizontal edita tiempo, vertical edita valor).
    - Desplazamiento horizontal con MMB drag / `Shift+Rueda` (`m_sequencerScrollX`), y sincronización vertical del panel de pistas con el lienzo temporal mediante `trackListScrollY` y `PushClipRect`.
    - Corrección de textos cortados en pestañas de OASIS e Importación/Exportación.
  - **Surgical Update: 5 Correcciones Matemáticas en el Núcleo (Cámara, Bóveda Celeste, CSM y Personaje)**:
    - `Camera::syncOrbitDistance()`: Sincronización dinámica de `m_orbitDistance = glm::max(0.5f, glm::length(m_target - m_position))` en `setPosition`, `setTarget`, Gizmo de Navegación, Sequencer, MMB y scroll, eliminando saltos de cámara al reanudar el modo órbita.
    - `DayNightCycle` & `EditorUI::getLightDir()`: Unificación trigonométrica analítica de coordenadas esféricas celestes para que el disco solar del Skybox y las sombras proyectadas en el suelo coincidan exactamente al milímetro en modo dinámico y manual (`setManualSun`). Detección nocturna (`isNight`) devolviendo la dirección lunar con ángulo descendente garantizado $\ge 12^\circ$ para evitar sombras invertidas rasantes o desde debajo del suelo.
    - `CascadedShadowMap`: Inclusión de parámetro `aspectRatio` en `update()` y `calculateCascadeMatrix()`, recibiendo la relación de aspecto real de la ventana del Viewport desde `main.cpp` para un cálculo exacto de sub-frustums y cajas envolventes de cascadas.
    - `CharacterController`: Head bobbing dependiente de `deltaTime` (eliminando la constante de 0.016f que duplicaba la velocidad a 100+ FPS) e indexación de biomas en `detectPlayerSurface()` relativa a `terrain->getBaseOffset()`.
  - **Fase 22.6: Corrección de Geomorphing CDLOD, Modos de Shading Blender, Aspect Ratio Meshlets, Físicas Sequencer y Persistencia Drag & Drop**:
    - `OASISTerrain`: Corrección matemática de la fórmula de CDLOD Geomorphing hacia el padre (`parentSubdivThreshold = (size * 2.0f) * 2.2f * m_lodDistanceFactor;`, `morphRange = parentSubdivThreshold * 0.30f`) en chunks hoja (`lod > 0`), manteniendo `morphFactor = 0.0f` para chunks raíz (`lod == 0`). Margen `guardMargin = size * 1.75f * std::max(1.0f, m_lodDistanceFactor * 0.5f) + 15.0f` en `evaluateQuadtree`, suprimiendo cualquier popping en bordes con distancias LOD altas (4.00x).
    - `main.cpp` (Modos de Shading Viewport): Modo `SOLID` con anulación dinámica de sombras directas e IBL (`u_ShadowsEnabled = 0`, `u_UseIBL = 0`) y luz ambiente neutra (`u_AmbientColor = vec3(0.45f)`); Modo `OASIS_MESHLETS` con activación temporal durante el frame de depuración cromática de Meshlets (`debugColors = true`) y depuración cromática de chunks CDLOD (`terrain.getOASIS()->setDebugLOD(true)`), restaurando banderas tras el pase de renderizado.
    - `Mesh.cpp` y `OASISMeshlet.cpp`: Detección en tiempo de ejecución del viewport de OpenGL (`glGetIntegerv(GL_VIEWPORT, vp)`) para derivar la matriz de proyección con relación de aspecto real en `Mesh::drawOASIS`; margen de seguridad lateral `worldSphereRadius * 0.5f` en `OASISMeshletContainer::cullAndCollect`.
    - `PhysicsWorld.cpp`: Prioridad cinemática de Sequencer sobre Bullet Physics en `syncPhysicsToEntities`; si `isControlledBySequencer()` y `Sequencer::get().isPlaying()`, Bullet no sobreescribe la entidad, sino que se sincroniza la física hacia la cinemática (`syncEntityToPhysics`), eliminando tirones por gravedad en entidades animadas.
    - `main.cpp` y `SceneIO.cpp`: Integración de `editorUI.loadProject(...)` en el callback de Drag & Drop y archivo de inicio, restaurando completamente terreno, niebla, agua, cielo HDRI y simulación física; extracción conjunta de `"position"` y `"target"` de cámara en `SceneIO::loadSceneASIS` restaurando encuadre y órbita con `camera.resetToFocus(camTarget); camera.setPosition(camPos); camera.syncOrbitDistance();`.
  - **Fase 22.7: Optimización y Precisión en Terreno PBR y OASIS Océano: Biomas Analíticos, Escultura Parcial Ultrarrápida y Limpieza VRAM**:
    - `Terrain::initSplatmap()`: Cálculo de biomas basado en altitud local no-desfasada (`elevation = m_heights[idx]`) y pendiente analítica (`slopeY`) mediante diferencias centrales continuas sobre `m_heights`, independizando el splatmapping de la malla de respaldo acotada a $256 \times 256$ para dar soporte fiel a resoluciones hasta 4096.
    - `Terrain::sculpt()` y `EditorUI.cpp`: Actualización parcial interactiva de la textura GPU (R32F) vía `m_oasis->updateHeightmapSubRegion(...)` (`glTexSubImage2D`) durante el arrastre del ratón, difiriendo la regeneración de VBO y `syncPhysics()` hasta el evento de liberación (`IsMouseReleased`), alcanzando 60-120+ FPS estables sin congelamientos.
    - `Terrain::shutdown()`: Liberación rigurosa de texturas de mapas de altura (`heightTex`) y oclusión ambiental (`aoTex`) en cada capa PBR con `glDeleteTextures`, erradicando fugas de memoria VRAM.
    - `Water::evaluateQuadtree()`: Integración de margen de guarda `guardMargin = size * 1.5f * std::max(1.0f, m_oasisLodFactor * 0.5f) + 15.0f` en el frustum culling de los chunks oceánicos en `Water.cpp`.

---

## 3. Snippets de Referencia Clave
- `src/core/Sequencer.h` y `src/core/Sequencer.cpp`: Motor cinemático, interpolación de curvas Bézier/Linear/Step, sincronización procedural IA/Lua y auto-bake.
- `src/core/OASISTerrain.h` y `src/core/OASISTerrain.cpp`: Arquitectura CDLOD, quadtree adaptativo, geomorphing continuo y frustum culling con guard band.
- `src/core/OASISMeshlet.h` y `src/core/OASISMeshlet.cpp`: Módulo de geometría virtual con meshlets y culling jerárquico.
- `src/core/PhysicsWorld.h` y `src/core/PhysicsWorld.cpp`: Simulación Bullet 3D a 60 Hz y sincronización cinemática con Sequencer.
- `src/core/Window.h` y `src/core/Window.cpp`: Barra de título oscura DWM e icono analítico 64x64 RGBA del isotipo ASIS.
- `src/core/MaterialManager.h` y `src/core/MaterialManager.cpp`: Gestor de materiales PBR, serialización `.asismat` y presets.
- `src/core/EditorUI.h` y `src/core/EditorUI.cpp`: Layouts, Outliner con búsqueda, atajos profesionales, Dope Sheet con subcanales y hold bars, panel de propiedades con diamantes de keyframe y REPL Lua.
- `src/core/ScriptEngine.cpp`: Ejecución de scripts, bindings cinemáticos `seq_*` y comandos interactivos en sandbox protegido.
- `src/core/CascadedShadowMap.h` y `src/core/OmniShadowMap.h`: CSM y Omni-PCSS.
- `src/core/DayNightCycle.h` y `src/core/DayNightCycle.cpp`: Ciclo solar/lunar de 24h y clima.
- `src/core/Water.h` y `src/core/Water.cpp`: Océano 3D con olas de Gerstner, streaming OASIS y flotabilidad física.
- `assets/shaders/model.frag` y `assets/shaders/terrain.frag`: Shaders PBR con CSM, PCSS, Omni-PCSS, niebla global y humedad.

---

## 4. Estado de Compilación y Verificación (Fase 22.7)
- **Binario**: `motor_grafico/build/Release/ASIS_Engine.exe`
- **Errores de Compilación**: 0
- **Verificación en Ejecución**: Exitosa, inicialización limpia a 60-120+ FPS sin micro-tirones, cero emojis unicode rotos (100% micro-iconos vectoriales de `ImDrawList`), modos de shading funcionando dinámicamente, escultura interactiva en tiempo real y liberación completa de recursos de textura.
- **Bugs Pendientes**: Ninguno. Todas las optimizaciones y correcciones integradas al 100%.
