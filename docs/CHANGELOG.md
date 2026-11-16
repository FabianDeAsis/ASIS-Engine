# 📜 Changelog - Motor Gráfico ASIS Engine

Todos los cambios notables de este proyecto serán documentados en este archivo siguiendo el formato:
`[FECHA] [FASE] Descripción del cambio`

---

## [2026-11-15] [FASE 22.7] Optimización y Precisión en Terreno PBR y OASIS Océano: Biomas Analíticos, Escultura Parcial Ultrarrápida y Limpieza VRAM
- **Biomas Analíticos y Normales por Diferencias Centrales (`Terrain.cpp`)**:
  - En `Terrain::initSplatmap()`: corrección de la cota de elevación utilizando la altura local `elevation = m_heights[idx]` sin sustraer `m_baseOffset`, garantizando correspondencia con los umbrales de bioma (Nieve $> 14.0\text{m}$, Tierra $> 5.5\text{m}$).
  - Cálculo de la pendiente vertical (`slopeY`) mediante derivadas parciales continuas y diferencias centrales directamente sobre el grid de alturas (`m_heights`), independizando la generación del splatmap de la malla VBO acotada (soportando resoluciones de 64 hasta 4096 sin lecturas fuera de rango ni desfasajes).
- **Actualización Parcial Ultrarrápida en Escultura de Terreno (`Terrain.cpp`, `EditorUI.cpp`)**:
  - En `Terrain::sculpt()`: reemplazo de la costosa llamada a `updateNormalsAndVBO()` en cada frame por `m_oasis->updateHeightmapSubRegion(...)` con límites `[minX, minZ]` a `[maxX, maxZ]`. Solo se actualiza la sub-región sucia de la textura GPU (R32F) vía `glTexSubImage2D` mientras se arrastra el ratón.
  - La reconstrucción completa de la malla VBO de respaldo y la sincronización con Bullet Physics (`terrain.syncPhysics()`) se difiere elegantemente al soltar el botón izquierdo del ratón (`ImGui::IsMouseReleased`) o cuando OASIS está inactivo, manteniendo 60-120+ FPS estables sin congelamiento durante el esculpido interactivo.
- **Liberación Íntegra de VRAM en Apagado de Terreno (`Terrain.cpp`)**:
  - En `Terrain::shutdown()`: añadido borrado explícito de texturas de desplazamiento (`l.heightTex`) y oclusión ambiental (`l.aoTex`) en cada capa PBR con `glDeleteTextures`, eliminando cualquier fuga residual de recursos en GPU al cerrar o reiniciar la escena.
- **Margen de Frustum Culling con Guard Band en OASIS Océano (`Water.cpp`)**:
  - En `Water::evaluateQuadtree()`: incorporación de margen de guarda `guardMargin = size * 1.5f * std::max(1.0f, m_oasisLodFactor * 0.5f) + 15.0f` en `frustum.isBoxVisible(aabbMin, aabbMax, guardMargin)`, eliminando desapariciones anticipadas de parches de olas y espuma en los bordes del encuadre.

---

## [2026-11-15] [FASE 22.6] Corrección de Geomorphing CDLOD, Modos Shading Blender, Aspect Ratio Meshlets, Físicas Sequencer y Drag & Drop .asis
- **Corrección de Fórmula de Geomorphing y Subdivisión en Terreno OASIS (`OASISTerrain.cpp`)**:
  - Fórmula matemática de CDLOD Geomorphing rectificada para chunks hoja: se evalúa la transición continua en el último 30% del rango con respecto al umbral de subdivisión del padre (`parentSubdivThreshold = (size * 2.0f) * 2.2f * m_lodDistanceFactor;`, `morphRange = parentSubdivThreshold * 0.30f;`). Chunks raíz (`lod == 0`) fijan `morphFactor = 0.0f` al carecer de padre.
  - Ampliación del `guardMargin` en `evaluateQuadtree`: `guardMargin = size * 1.75f * std::max(1.0f, m_lodDistanceFactor * 0.5f) + 15.0f;`, eliminando totalmente artefactos de popping en los bordes del Viewport incluso con `Distancia LOD` en `4.00x`.
- **Conexión de los 4 Modos de Viewport Shading en Pipeline de Renderizado (`main.cpp`)**:
  - `ViewportShadingMode::SOLID`: anulación en tiempo real de sombras directas y reflexiones de entorno IBL con luz ambiente uniforme (`u_AmbientColor = vec3(0.45f)`), brindando una vista ágil de modelado y composición idéntica a Blender.
  - `ViewportShadingMode::OASIS_MESHLETS`: activación temporal de `debugColors = true` en `OASISMeshletConfig` y `setDebugLOD(true)` en `OASISTerrain` durante el cuadro de renderizado, permitiendo inspeccionar clústeres de triángulos y niveles de teselación en ambos pases (Editor y Modo Inmersivo F1).
- **Aspect Ratio Dinámico y Margen Frustum en Meshlets de Geometría Virtual (`Mesh.cpp`, `OASISMeshlet.cpp`)**:
  - En `Mesh::drawOASIS`: lectura del viewport real de OpenGL con `glGetIntegerv(GL_VIEWPORT, vp)` para generar la matriz de proyección de culling, sustituyendo el ratio rígido 16:9.
  - En `OASISMeshletContainer::cullAndCollect`: margen de seguridad de frustum culling ajustado a `worldSphereRadius * 0.5f` para evitar descarte prematuro en giros rasantes.
- **Sincronización Limpia y Prioridad Cinemática entre Sequencer y Bullet Physics (`PhysicsWorld.cpp`)**:
  - En `PhysicsWorld::syncPhysicsToEntities`: si la entidad está marcada como `isControlledBySequencer()` y el Sequencer está activo (`isPlaying()`), se transfiere la cinemática de la entidad hacia el cuerpo de Bullet (`syncEntityToPhysics`), impidiendo que la gravedad física interfiera con la animación de curvas temporales.
- **Persistencia Completa en Drag & Drop y Restauración Exacta de Cámara (`main.cpp`, `SceneIO.cpp`)**:
  - En `main.cpp`: arrastrar archivos `.asis` invoca ahora `editorUI.loadProject(path, scene, camera, terrain, skybox, physicsWorld, scriptEngine, &water)`, restaurando integralmente terreno, niebla, agua, cielo y dinámicas.
  - En `SceneIO::loadSceneASIS`: deserialización de `"position"` y `"target"`, restaurando encuadre con `camera.resetToFocus(camTarget); camera.setPosition(camPos); camera.syncOrbitDistance();`.

---

## [2026-11-15] [FASE 22.5 Hotfix] Correcciones Matemáticas en Núcleo: Sincronización Órbita, Bóveda Celeste, CSM y Personaje
- **Sincronización Continua de `m_orbitDistance` (`Camera.h`, `Camera.cpp`, `EditorUI.h`, `Sequencer.cpp`)**:
  - Implementación de `Camera::syncOrbitDistance()`: calcula `float d = glm::length(m_target - m_position); if (d > 0.05f) m_orbitDistance = glm::max(0.5f, d);`.
  - Invocado de forma automática en `setPosition()`, `setTarget()`, activación de órbita con botón central (MMB) / Alt+LMB, scroll del ratón, Gizmo de Navegación 3D y secuencias cinemáticas de Sequencer (`m_pilotCineCamera`), eliminando saltos bruscos de la cámara al regresar al modo de órbita o zoom.
- **Unificación de Trigonometría Solar y Sombras Lunares Nocturnas (`DayNightCycle.h`, `DayNightCycle.cpp`, `EditorUI.cpp`)**:
  - Unificación analítica de coordenadas esféricas para `sunDir` (`cos(el)*cos(az)`, `sin(el)`, `cos(el)*sin(az)`) y `lightDir = -normalize(sunDir)` en todo el motor gráfico, garantizando correspondencia milimétrica entre el disco solar renderizado por el Skybox y el ángulo de proyección de las sombras en el terreno y modelos.
  - En `EditorUI::getLightDir()`: si el ciclo dinámico está activo y es de noche (`isNight()`), se devuelve la dirección lunar (`getMoonDirection()`) garantizando una inclinación vertical mínima de $12^\circ$ hacia abajo (`y <= -sin(12°)`) para prevenir artefactos rasantes y sombras proyectadas desde debajo del suelo.
  - Implementación de `DayNightCycle::setManualSun(azimuth, elevation)` para sincronizar la bóveda celeste cuando el ciclo automático está deshabilitado pero el usuario ajusta el sol en el inspector.
- **Aspect Ratio Dinámico en Cascaded Shadow Maps (`CascadedShadowMap.h`, `CascadedShadowMap.cpp`, `main.cpp`)**:
  - Parámetro `aspectRatio` integrado en `CascadedShadowMap::update()` y `calculateCascadeMatrix()`.
  - `main.cpp` transmite la relación de aspecto real actual del Viewport (`framebuffer.getAspectRatio()` o resolución de ventana en modo juego) en lugar de un factor fijo de 16:9, permitiendo ajuste estricto de las 3 cascadas a cualquier proporción de pantalla.
- **Head Bobbing y Biomas Físicos en `CharacterController.cpp`**:
  - El contador de bamboleo rítmico de cabeza (`m_headBobTimer`) ahora escala con el `deltaTime` real de la simulación física en `update()`, evitando la aceleración artificial a altas tasas de cuadros por segundo (100+ FPS).
  - En `detectPlayerSurface()`: las alturas para detección de biomas (Nieve $> 14.0\text{m}$, Tierra $> 5.5\text{m}$) se evalúan de forma relativa respecto a `terrain->getBaseOffset()`, asignando los pasos y sonidos de pisadas correctos.

---

## [2026-11-15] [FASE 22.5 Patch] Frustum Culling con Guard Band en OASIS CDLOD, Sincronización Euler/Quaternion y Graph Editor Interactivo
- **Corrección de Frustum Culling en Chunks de Terreno OASIS (`OASISTerrain.h`, `OASISTerrain.cpp`, `Terrain.h`, `Terrain.cpp`, `main.cpp`)**:
  - Matriz ViewProjection del culling configurada con el aspect ratio real del Viewport (`m_viewportSize.x / m_viewportSize.y` o `framebuffer.getAspectRatio()`) y el FOV real actual de la cámara.
  - Evaluación AABB 3D de cada chunk contemplando `m_baseOffset` y el rango completo de elevaciones (`minY = baseOffset - absMaxH - 20.0f`, `maxY = baseOffset + absMaxH + 30.0f`).
  - Margen de seguridad lateral (*Guard Band*) proporcional al tamaño del chunk y al factor LOD: `guardMargin = chunkRadius * 1.45f * std::max(1.0f, m_lodDistanceFactor * 0.40f)` en `Frustum::isBoxVisible(..., guardMargin)`. Eliminado culling cónico en nodos de terreno virtual para prevenir desaparición anticipada de chunks en bordes laterales.
- **Sincronización Euler <-> Quaternion y Sequencer Reactivo (`Entity.h`, `Sequencer.h`, `Sequencer.cpp`, `EditorUI.cpp`)**:
  - `Entity::syncQuaternionFromEuler()` con orden de rotación Y -> X -> Z, ejecutado en `setRotation`, `rotate`, y al manipular gizmos en el Viewport o campos en el Inspector.
  - Bandera `m_forceSampleOnce` en `Sequencer`: Cuando el Sequencer está en pausa, no sobreescribe las modificaciones manuales de gizmos o inspector salvo que se reproduzca o se solicite explícitamente con `seq.requestSample()`.
  - Auto-Key reactivo al transformar entidades cuando `animState == 2` o `isAutoKeyEnabled()`.
  - Sincronización de animaciones procedurales de escena al iniciar ejecutada solo una vez (`!m_initialSequencerSynced`).
- **Línea de Tiempo Dope Sheet y Graph Editor Interactivo (`EditorUI.cpp`)**:
  - Arrastre interactivo con clic izquierdo (LMB Drag) de los rombos de keyframes en el Dope Sheet con actualización continua y muestreo en tiempo real (`seq.requestSample()`), ordenando los fotogramas clave al soltar el botón.
  - Doble clic en rombo de keyframe: ubica el cabezal de tiempo en la clave y selecciona la entidad en el Outliner.
  - Clic derecho en rombo de keyframe: despliega menú contextual `SeqKeyframeContextPopup` con edición numérica directa de tiempo, posición, rotación Euler, escala, modos de interpolación, botón "Capturar Pose Actual", duplicación y eliminación.
  - Graph Editor interactivo: selector de canales (`Posición`, `Rotación`, `Escala`), auto-escalado vertical centrado según los valores de la pista seleccionada, y nodos circulares de control arrastrables con el ratón (horizontal modifica tiempo, vertical modifica valor).
  - Desplazamiento horizontal con MMB drag o `Shift + Rueda` (`m_sequencerScrollX`), y sincronización vertical del panel de pistas con el lienzo temporal mediante `trackListScrollY` y `PushClipRect`.
- **Corrección de Textos Cortados en Pestañas de OASIS e Importación/Exportación (`EditorUI.cpp`)**:
  - Pestaña OASIS: Título `"Sistema OASIS (Geometria Virtual CDLOD)"`, checkbox `"Habilitar OASIS (Streaming y Teselacion)"`, salto de línea antes de `"Debug LOD"` y `"Subdivision LOD"`, y altura ampliada para telemetría.
  - Pestaña Importación/Exportación: Botón expandido `"Importar Modelo 3D..."`, texto descriptivo multilínea para formatos compatibles, `"Exportar Escena a OBJ (.obj + .mtl)"` y `"Exportar Escena a glTF 2.0 (.gltf)"`.

---

## [2026-11-15] [FASE 22.5] Corrección Matemática del Isotipo ASIS, Sistema de Keyframes Estilo Blender, Sincronización de Animaciones IA/Lua en el Sequencer y Pulido Visual
- **Renderizado Exacto del Isotipo Oficial `asisengine.svg` & Barra de Título Oscura de Windows**:
  - Implementación vectorial analítica en `UI::DrawASISLogo`: triángulo isósceles blanco puro (`#FFFFFF`) con ápice en `(0.50, 0.00)`, esquinas de base en `(0.00, 1.00)` y `(1.00, 1.00)`, y 4 cortes trapezoidales de color de fondo (`bgCol`) en la esquina inferior izquierda delimitados por la diagonal $x_{\text{right}}(y) = 0.370f + (y - 0.674f) \cdot 0.46f$.
  - Ajuste de grosor mínimo garantizado de $1.6\text{px}$ para los 4 cortes en tamaños pequeños ($\le 22\text{px}$), eliminando artefactos de subpíxel y asegurando legibilidad nítida en la barra de menú.
  - Generación de icono de ventana RGBA de 64x64 derivado de la misma función matemática y cargado mediante `glfwSetWindowIcon`.
  - Integración con la API de Windows DWM (`DwmSetWindowAttribute` con atributo 20/19) para activar la barra de título oscura del sistema operativo en sintonía con la paleta de color del editor.
  - Ventana modal *"Acerca de ASIS Engine v0.22"* con isotipo a `84x84 px` en blanco puro sobre `#1D1D1D`.
- **Sincronización de Animaciones IA/Lua y Hornado Dinámico (Auto-Bake) al Sequencer (`Sequencer.h`, `Sequencer.cpp`, `ScriptEngine.cpp`)**:
  - Función `syncSceneAnimationsToSequencer(Scene& scene)`: Genera automáticamente pistas y claves en el Sequencer para todas las entidades procedurales de la escena (`CenterCube`, satélites `Sat1/2/3`, `MainModel`, monedas `Moneda_1/2`, cámara principal).
  - Botón interactivo `[ ● Bake IA & Lua ]` en la barra de transporte del Sequencer: Hornea y muestrea en tiempo real el movimiento de rigidbodies con físicas y scripts de Lua a claves discretas de posición y rotación con intervalo regulable.
  - Normalización angular de Euler en `Entity::setRotation`, `Entity::rotate` y `Entity::setOrientation` restringida al rango $[0, 360)^\circ$ con `std::fmod`, evitando saltos o desbordamientos durante giros acumulativos.
  - Bandera de prioridad `m_controlledBySequencer`: Inhibe modificaciones incrementales procedentes de scripts Lua mientras el usuario reproduce o arrastra el cursor en la línea de tiempo.
  - Nuevas APIs de Lua para el Sequencer: `seq_clear()`, `seq_addEntityTrack(name)`, `seq_addKeyframe(...)` y `seq_bake(duration, interval)`.
- **Sistema de Keyframes Estilo Blender (Inspector de Propiedades, Viewport y Dope Sheet)**:
  - Botones vectoriales de rombo diamante (`◇` / `◆`) integrados a la derecha de los campos de traslación, rotación, escala y color en el Inspector.
  - Coloración adaptativa del fondo de inputs (`FrameBg`):
    - **Dorado Blender** (`#5C4B1B`): Cuando la propiedad tiene un fotograma clave exacto en el tiempo actual ($|t - t_{\text{key}}| < 0.02\text{s}$).
    - **Verde Oliva Blender** (`#2E4725`): En instantes intermedios interpolados de una propiedad animada.
    - **Gris Neutro Oscuro** (`#1D1D1D`): Propiedades no animadas.
  - Subcanales desplegables en el Dope Sheet (`Posición`, `Rotación`, `Escala`) con barras horizontales de retención (*Hold Bars*) de Blender entre fotogramas consecutivos de valor idéntico.
  - Atajos de teclado profesionales:
    - Tecla `I` en Viewport: Inserta fotograma clave en la entidad seleccionada mostrando una notificación contextual en la barra de estado.
    - Modo Auto-Key reactivo: Registra automáticamente transformaciones modificadas mediante gizmos o inputs del inspector.
    - Lienzo Dope Sheet: Selección rectangular con caja (box select), tecla `G` para desplazar claves seleccionadas, `Shift+D` para duplicar claves, `Supr`/`X` para eliminar y tecla `T` para menú emergente de interpolación (`Linear`, `Constant/Step`, `Cubic Bézier`).
- **Corrección de Cortes Visuales en UI**:
  - Jerarquía (Outliner): Ajuste de altura del contenedor scrollable a `GetContentRegionAvail().y - 54.0f` para garantizar visibilidad íntegra de la barra inferior de botones (`Añadir`, `Duplicar`, `Eliminar`), y anchura responsive para el campo y botón `"+ Asignar"` de grupos.
  - Sequencer: Incremento del ancho del selector de FPS a `68px` para evitar truncamiento del texto (`"30 FPS"`).

---

## [2026-11-15] [FASE 22] Sistema de Animación Cinemática / Sequencer, Cámaras de Cine y Pistas de Curvas Temporales
- **Motor Central de Secuencias Cinemáticas (`Sequencer.h`, `Sequencer.cpp`)**:
  - Diseñada arquitectura multicapa para animación basada en tiempo real con modos de interpolación: `Linear` (lineal continua), `Constant / Step` (mantiene valor hasta el próximo fotograma, para cortes de cámara) y `CubicBezier` (curvas suaves Hermite / Bézier cúbicas con tangentes automáticas y aceleración suave).
  - Pistas modulares especializadas:
    - **Pistas de Transformación 3D (`EntityTransformTrack`)**: Fotogramas clave de traslación, orientación esférica con cuaterniones (`slerp`) y escala de entidades del motor.
    - **Pistas de Cámara de Cine (`CineCameraTrack`)**: Posición espacial, objetivo de mira (*LookAt Target*), longitud focal física (`focalLength_mm`), distancia de enfoque focal (`focusDistance`), apertura del diafragma (`aperture_fstop`) y oscilación procedural de cámara en mano (*Camera Shake*).
    - **Pistas de Entorno y Atmósfera (`EnvironmentTrack`)**: Curvas de ciclo horario de 24h, compensación de exposición EV, intensidad solar y balance de color del sol.
    - **Pistas de Eventos Lua (`LuaEventTrack`)**: Puntos de activación temporal (*Trigger Keys*) que ejecutan comandos y llamadas dinámicas en el sandbox de Lua durante la reproducción.
  - Soporte de transporte completo: `play()`, `pause()`, `stop()`, `setTime(t)`, velocidad de reproducción regulable (`playbackSpeed`), duración y bucle continuo (`isLooping`).
  - Auto-Keying reactivo: Captura inmediata de fotogramas clave al manipular entidades con gizmos o mover la cámara de cine.
  - 3 Secuencias de Demostración Cinemáticas en 1 Clic:
    1. *Hero Shot 360° con Bokeh DoF*: Órbita suave alrededor del objetivo con lente de 50mm y desenfoque Bokeh a f/1.4.
    2. *Travelling de Cine + Amanecer Rápido*: Dolly In steadycam con lente de 35mm sincronizado con amanecer solar dorado acelerado.
    3. *Secuencia de Acción Multi-Corte*: 3 planos de cámara distintos con cortes instantáneos (`Step`) y disparo de eventos de audio/scripts en Lua.
  - Persistencia y serialización nativa en formato estructurado JSON `.asisseq` en la carpeta `assets/sequences/`.
- **Línea de Tiempo / Timeline, Dope Sheet y Graph Editor (`EditorUI.h`, `EditorUI.cpp`)**:
  - Nueva pestaña acoplada automáticamente en la bandeja inferior: `"Sequencer & Cine (Fase 22)"`, sincronizada con el espacio de trabajo `Iluminación & Cine`.
  - Barra superior de transporte de 26px con micro-iconos vectoriales de `ImDrawList` (cero emojis que generen caracteres `?`): `[ |< ]`, `[ < ]`, `[ ▶ / ⏸ ]`, `[ ■ ]`, `[ > ]`, `[ ● Auto-Key ]` (rojo `#E03C3C`), `[ + Clave (I) ]`, alternador de vista Dope Sheet vs Graph Editor, toggle `[ 🎥 Pilotar ]`, toggle `[ 🎬 2.39:1 Scope ]`, contador de tiempo/fotogramas en pill cyan `00:03.20 (F96 / 300)`, arrastre de duración, selector de FPS (24, 30, 60), checkbox de bucle, selector de presets y botones de guardar/cargar `.asisseq`.
  - Panel lateral de pistas (~22%): Lista con iconos vectoriales (`CINE_CAMERA`, `SUN`, `PHYSICS`, `SPARK`), nombres, botón Mute `[M]`, añadir clave `[+K]` y eliminar pista `[X]`.
  - Lienzo temporal derecho (~78%):
    - Regla de tiempo superior con marcas numéricas en segundos y ticks menores de fotogramas con interacción de arrastre/scrubbing continuo.
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

---

## [2026-11-15] [FASE 21] Gestor de Materiales PBR, Jerarquía con Atajos Profesionales, Barra de Búsqueda, Layouts de UI y Consola Lua Interactiva
- **Gestor Central de Materiales PBR y Biblioteca de Presets Físicos (`MaterialManager.h`, `MaterialManager.cpp`, `EditorUI.cpp`)**:
  - Implementada arquitectura centralizada de gestión de materiales PBR (`MaterialManager` singleton).
  - Formato nativo legible `.asismat` (JSON estructurado) con versionado, propiedades físicas PBR completas (albedo, roughness, metallic, specular, normalStrength, ao, emission, emissionStrength) y persistencia de rutas de texturas relativas y absolutas.
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

---

## [2026-11-15] [FASE 20] Ciclo Día / Noche Dinámico 24h, Clima Atmosférico, SSR, Contact Shadows, Niebla Volumétrica Global, CSM y Sombras Suaves PCSS
- **Ciclo Día / Noche Dinámico de 24 Horas (`DayNightCycle.h`, `DayNightCycle.cpp`, `EditorUI.cpp`, `main.cpp`)**:
  - Simulación astronómica continua con paso de tiempo acelerado configurable (duración de 1 día de 30 a 600 segundos).
  - Cálculo trigonométrico de elevación y acimut del Sol y la Luna basado en latitud geográfica e inclinación solar.
  - Curvas de color y temperatura Kelvin procedurales: amanecer dorado, mediodía blanco intenso, atardecer naranja/magenta y noche azul lunar.
  - Conmutación automática de la fuente de sombras hacia la Luna en horario nocturno con atenuación de penumbra.
- **Clima Atmosférico y Precipitaciones (`DayNightCycle.h`, `ParticleSystem.cpp`, `EditorUI.cpp`)**:
  - 4 estados climáticos: Despejado, Lluvia, Nieve y Tormenta con relámpagos.
  - Emisor de lluvia con partículas 3D orientadas en caída y aceleración por viento.
  - Parámetro dinámico de Humedad PBR de Superficie (*Wetness*): humedece progresivamente el albedo y reduce la rugosidad a 0.05 en el terreno y los modelos según la intensidad de lluvia.
- **Screen-Space Reflections (SSR) (`ssr.frag`, `postprocess.frag`, `EditorUI.cpp`)**:
  - Raymarching jerárquico en espacio de pantalla con interpolación lineal y refinamiento binario en el buffer de profundidad normalizado.
  - Reflexión dinámica de modelos sobre agua y superficies lisas/metálicas con atenuación de bordes (*screen edge fade*).
- **Sombras de Contacto en Espacio de Pantalla (Contact Shadows) (`contact_shadows.frag`, `EditorUI.cpp`)**:
  - Trazado de rayos corto en screen-space a lo largo del vector solar para capturar micro-oclusiones y sombras de contacto fino en grietas y juntas donde los shadow maps convencionales pierden resolución.
- **Niebla Volumétrica Atmosférica Global (`model.frag`, `terrain.frag`, `water.frag`, `skybox.frag`)**:
  - Controlador unificado de niebla volumétrica con absorción e inscattering exponencial que afecta armónicamente tanto al cielo HDRI como a las montañas del terreno y los objetos de la escena.
- **Cascaded Shadow Maps (CSM) de 3 Cascadas (`CascadedShadowMap.h`, `CascadedShadowMap.cpp`, `model.frag`, `terrain.frag`)**:
  - Partición del frustum de cámara en 3 sub-frustums con shadow maps de 2048x2048 independientes, eliminando el pixelado de sombras en distancias lejanas sin comprometer el detalle en primer plano.
- **Sombras Suaves Físicas PCSS (Percentage-Closer Soft Shadows) para Sol y Luces Puntuales (`model.frag`, `terrain.frag`, `Light.h`, `OmniShadowMap.h`)**:
  - Búsqueda de bloqueadores basada en disco de Vogel de 16 muestras para estimar la profundidad media del obstáculo.
  - Estimación física del tamaño de la penumbra: $\text{Penumbra} = \frac{d_{\text{receptor}} - d_{\text{bloqueador}}}{d_{\text{bloqueador}}} \times w_{\text{luz}}$.
  - Filtrado adaptativo de 32 a 64 muestras Vogel dependiente de la penumbra, produciendo sombras nítidas en los puntos de contacto y suaves a medida que la sombra se proyecta a mayor distancia.
  - **Omni-PCSS para Luces Puntuales**: Extensión a mapas de cubos tridimensionales con base ortonormal tangente para simular penumbra física realista en bombillos y antorchas.

---

## [2026-11-14] [FASE 19] Océano 3D con Olas de Gerstner, Flotabilidad Física Bullet, Espuma Dinámica y Refracción
- **Simulación Cinemática de Olas de Gerstner en GPU (`Water.h`, `Water.cpp`, `water.vert`, `water.frag`)**:
  - Malla oceánica de alta densidad (128x128 vértices) que abarca hasta 700m con atenuación hacia el horizonte infinito.
  - Combinación matemática de 4 trenes de ondas de Gerstner con desplazamiento trocoidal lateral y vertical.
  - Recálculo dinámico de normales analíticas y vectores tangentes directamente en el Vertex Shader.
- **Flotabilidad Física Bidireccional en Bullet Physics (`Water.cpp`, `PhysicsWorld.cpp`, `main.cpp`)**:
  - Muestreo analítico de la altura del agua en CPU sincronizado exactamente con la función trigonométrica del shader.
  - Cálculo hidrostático de fuerza de empuje de Arquímedes ($\vec{F}_{\text{boya}} = -\rho V \vec{g}$) sobre cuerpos rígidos sumergidos.
  - Amortiguamiento hidrodinámico lineal y angular para emular resistencia y resistencia viscosa al oleaje.
- **Espuma Dinámica en Crestas y Orillas con Niebla Submarina (`water.frag`)**:
  - Detección de orillas y contacto con relieve costero mediante diferencia de profundidad en el buffer de profundidad (`linearDepth(depth) - linearDepth(gl_FragCoord.z)`).
  - Espuma generada dinámicamente por la curvatura Jacobiana de las crestas de ola.
  - Transición suave con niebla de dispersión cromática cian y absorción lumínica debajo de la superficie del agua.

---

## [2026-11-14] [FASE 18] Arquitectura OASIS Módulo 2 (Virtual Geometry & Meshlets LOD) y Exportación a Standalone .exe
- **Sistema OASIS Virtual Geometry Módulo 2 (`OASISMeshlet.h`, `OASISMeshlet.cpp`, `Model.cpp`, `Terrain.cpp`)**:
  - Partición de geometrías densas en agrupaciones de triángulos conexos (*Meshlets* de 64 vértices / 126 primitivas).
  - Culling jerárquico de cono normal (Normal Cone Culling) y prueba de oclusión por bounding boxes para descartar meshlets que apuntan en sentido opuesto a la cámara.
  - Reducción masiva de consumo de GPU y soporte de millones de polígonos a más de 100 FPS estables.
- **Empaquetado Autónomo y Exportación a Juego Independiente (.exe Standalone)**:
  - Generador de binario standalone empaquetado para distribución en Windows sin dependencias de desarrollo.
  - Empaquetador automático de recursos (`assets/`, `shaders/`, `scripts/`, `prefabs/`) con escena de inicio predeterminada.

---

## [2026-11-14] [FASE 17] Sistema de Prefabs, Spawning Dinámico con Lua, Triggers de Eventos y Auto-Exposición
- **Sistema de Prefabs y Plantillas Reutilizables (`PrefabManager.h`, `PrefabManager.cpp`, `EditorUI.cpp`)**:
  - Formato JSON `.prefab` para almacenamiento de jerarquías complejas, transformaciones relativas, componentes visuales y físicas.
  - Paleta visual de prefabs en el editor con botón de previsualización y spawn en el foco del Viewport 3D.
- **Spawning Dinámico de Entidades desde Lua (`ScriptEngine.cpp`)**:
  - Funciones Lua expuestas: `spawnPrefab(nombre, x, y, z)`, `getPrefabList()`, `destroyEntity(nombre)`.
- **Gameplay Triggers y Sensores de Colisión (`PhysicsWorld.cpp`, `ScriptEngine.cpp`, `Entity.h`)**:
  - Cuerpos de detección fantasma (*Ghost / Trigger sensors*) sin respuesta de fuerza pero con emisión de eventos.
  - Despacho de callbacks en Lua: `onTriggerEnter(triggerName, otherName)` y `onTriggerExit(triggerName, otherName)`.
- **Adaptación Ocular Dinámica y Auto-Exposición Cinemática (`EditorUI.cpp`, `postprocess.frag`)**:
  - Medición de luminancia de escena adaptativa con histograma temporal y velocidad de acomodación pupilar fisiológica suave.

---

## [2026-11-13] [REVISIÓN Y PULIDO FASE 16 - 11 CORRECCIONES CRÍTICAS ANTES DE FASE 17]
- **1. Bloqueo Estricto de Atajos de Edición en Modo de Juego (PIE) (`EditorUI.cpp`)**:
  - Gating de atajos de teclado (`Shift+A` para añadir objeto, `Ctrl+Z`, `Ctrl+Y`, `Ctrl+G`, etc.): ahora se ejecutan exclusivamente en `m_playMode != EnginePlayMode::PLAYING`.
  - Corregido el problema donde correr con `Shift` y moverse hacia la izquierda con `A` disparaba la ventana emergente "Añadir a la Escena".
  - Desactivado por completo el raycasting y la escultura/pintura del terreno 3D durante el modo de juego.
- **2. Solución a Fallos Fatales de Sonido (Crashes de Miniaudio) (`AudioEngine.cpp`)**:
  - Diagnóstico: Se asignaba `ma_sound` como variable local en el stack de las funciones `playFootstep`, `playJump`, `playLand` y `playSound3D`. Al salir de la función, el stack se desasignaba mientras el hilo mezclador de Miniaudio leía el buffer de audio, provocando violación de acceso de memoria (0xC0000005).
  - Solución: Implementado un pool estático de 32 voces persistentes (`m_impl->voices`, `acquireVoice()`, `markVoiceActive()`), garantizando memoria válida y concurrencia segura sin crasheos al saltar, caer al suelo o hacer clic en "Impacto Físico 3D".
- **3. Restauración de Cuadrícula Infinita (Infinite Grid Shader) (`grid.vert`, `grid.frag`, `main.cpp`)**:
  - Corregido el plano cercano de desproyección de McGuire en `grid.vert` de `z = 0.0` (Vulkan/DirectX) a `z = -1.0` (OpenGL NDC estándar).
  - Corregida la función de prueba de profundidad en `main.cpp` (`glDepthFunc(GL_LEQUAL)` y `glDepthMask(GL_FALSE)`).
  - Aumentado el desvanecimiento hacia el horizonte hasta 320 metros y mejorado el contraste de líneas menores (1m) y mayores (10m) con ejes X (rojo) y Z (azul) nítidos.
- **4. Físicas de Salto Más Rápidas y Sin Sensación de Globo (`CharacterController.cpp`)**:
  - Incrementada la fuerza vertical de impulso de salto a 8.2 m/s.
  - Implementada aceleración reactiva adicional de caída (`currentVy -= 18.0f * deltaTime` cuando el avatar desciende), eliminando el tiempo excesivo flotando en el aire.
- **5. Oclusión Ambiental en Espacio de Pantalla (SSAO) Totalmente Operativa (`Framebuffer.cpp`, `ssao.frag`, `ssao_composite.frag`)**:
  - Corregida la lectura del buffer de profundidad en Windows/NVIDIA/Intel configurando `GL_DEPTH_STENCIL_TEXTURE_MODE = GL_DEPTH_COMPONENT` y `GL_TEXTURE_COMPARE_MODE = GL_NONE`.
  - Reemplazada la fórmula de atenuación en `ssao.frag` por la ecuación Crytek estándar con `smoothstep(1.0, 0.0, distZ / u_Radius)`.
  - Modificado el pase compuesto a una mezcla lineal responsiva `mix(1.0, ao, u_Intensity)` para respuesta inmediata en tiempo real con los sliders del menú de gráficos.
- **6. Eliminación y Restauración del Plano de Suelo Base (`EditorUI.h`, `EditorUI.cpp`)**:
  - Menú contextual de clic derecho en el outliner para "Ocultar Suelo" o "Eliminar Suelo Base".
  - Opción "Restaurar Plano de Suelo Base" agregada tanto en la barra inferior del Outliner como en el menú "Añadir a la Escena (Shift+A)".
  - El motor ya no dibuja el plano base si fue eliminado por el usuario (`getShowGroundPlane()`).
- **7. Opciones de Terreno Visibles y Tarjetas de Texturas Splatmap (`EditorUI.cpp`)**:
  - "Resolución, Tamaño y Reconstrucción" y "Segmentación por Trozos (Chunks)" convertidos a `CollapsingHeader` desplegados por defecto.
  - Diseñadas tarjetas visuales para las 4 capas de textura (🌿 Césped [R], 🟫 Tierra [G], 🪨 Roca [B], ❄️ Nieve [A]) con indicadores de color, estado activo y botón de pintura con un clic.
- **8. Eliminación de Inclinación Extraña y Atravesamiento de Cajas (`PhysicsWorld.cpp`, `Entity.h`, `Entity.cpp`)**:
  - Diagnóstico: En `syncPhysicsToEntities`, se usaba `getEulerYPR` asignando erróneamente los ejes (Pitch=Z, Yaw=Y, Roll=X), corrompiendo la orientación del mesh visual respecto al cuerpo rígido de Bullet.
  - Solución: Integrado soporte nativo de orientación por cuaternión (`glm::quat` / `btQuaternion`) en `Entity::getModelMatrix()`.
  - Aumentadas las iteraciones del solver a 20 (`m_numIterations = 20`), margen de colisión (`0.015f`) y activado el almacenamiento en caché de fricción.
- **9. Iconos Vectoriales Nítidos en el Outliner (`EditorUI.h`, `EditorUI.cpp`)**:
  - Sustituidos todos los caracteres unicode/SMP por ilustraciones vectoriales dinámicas en `ImDrawList`: Cubo 3D isométrico sombreado, Esfera sombreada, Cilindro, Cono, Toroide, Plano, Bombillo de luz con destellos, Montañas de terreno y Carpetas de grupo.
  - Eliminados por completo los signos de interrogación en cajas (`?`).
- **10. Corrección de Ruta del Módulo de IA y Diálogos de Archivos (`SceneIO.cpp`, `EditorUI.cpp`)**:
  - Agregado el flag `OFN_NOCHANGEDIR` en `GetOpenFileName` y `GetSaveFileName` para evitar que Windows altere el directorio de trabajo del motor al abrir archivos (como HDRIs o texturas).
  - Implementado un resolutor absoluto de rutas basado en `GetModuleFileNameA` para localizar `ai_cli.py` independientemente del directorio actual.
- **11. Multi-Selección, Grupos, Subgrupos y Renombrado Inline (`EditorUI.h`, `EditorUI.cpp`, `Scene.h`, `Scene.cpp`)**:
  - Soporte de selección múltiple con `Ctrl + Clic` (toggle) y `Shift + Clic` (rango contiguo).
  - Renombrado rápido con tecla `F2`, doble clic o menú contextual.
  - Creación de grupos y subgrupos con la selección actual mediante `Ctrl + G` o botón en barra de acciones.
  - Modal de renombrado de grupos existentes y compatibilidad con jerarquías `Grupo/Subgrupo`.

---

## [2026-11-13] [FASE 16] Audio 3D Espacial (Miniaudio), Pulido FPS & Agachado, Terreno con Pinceles Splatmap/Offset/Chunks, Grupos en Jerarquía y Cuadrícula Infinita
- **Motor de Audio 3D Espacial y Sintetizador Procedural Autónomo (`AudioEngine.h`, `AudioEngine.cpp`)**:
  - Integración nativa con `miniaudio:x64-windows` (backend de baja latencia con soporte multicanal y espacialización 3D por HRTF).
  - Posicionamiento espacial tridimensional del Listener sincronizado en cada frame con la posición (`camera.getPosition()`), vector frontal (`camera.getFront()`) y vector vertical (`camera.getUp()`) tanto en modo Editor como en Play-in-Editor.
  - Sintetizador procedural autónomo en memoria (PCM a 44,100 Hz, 16 bits): genera formas de onda en tiempo real para 5 tipos de pisadas (césped, tierra, roca, nieve, concreto), salto, impacto y aterrizaje físico sin depender de assets externos en disco.
  - Soporte de audio 3D posicional para fuentes ambientales (crepitar dinámico de emisores de fuego al aproximarse el jugador).
  - Controles de volumen independientes en tiempo real: Maestro, Efectos (SFX), Pisadas (Footsteps), Música/Ambiente y botón de Silencio (Mute).
- **Pulido de Jugador en Primera Persona (FPS) y Agachado Físico Suave (`CharacterController.h`, `CharacterController.cpp`)**:
  - Modo de cámara predeterminado cambiado a **Primera Persona (FPS)**.
  - **Agachado Dinámico con Transición Suave**:
    - Altura de vista interpolada con `glm::mix` de 1.65m (de pie) a 0.95m (agachado) a 12x velocidad al presionar `C` o `Ctrl` izquierdo.
    - Escalado físico instantáneo de la cápsula de Bullet (`setLocalScaling(1.0, 0.65, 1.0)`) permitiendo al jugador agacharse y deslizarse por debajo de túneles y obstáculos bajos.
  - **Bamboleo de Cabeza Rítmico (Head Bobbing)**: Modulación armónica sinusoidal y cosenoidal de la cámara al caminar o correr sobre el suelo con atenuación suave al detenerse.
  - **Pisadas Adaptativas Según Superficie del Terreno**: Detección en tiempo real de la elevación y normal del terreno para reproducir pasos de Césped (valle), Tierra (media ladera), Roca (acantilados) o Nieve (cumbres alpinas).
  - Efectos sonoros de impulso de salto y aterrizaje con intensidad proporcional a la velocidad de impacto vertical.
- **Mejoras del Sistema de Terreno 3D (`Terrain.h`, `Terrain.cpp`, `terrain.frag`, `EditorUI.cpp`)**:
  - **Desplazamiento Vertical de Altura Base (`m_baseOffset = -8.0m`)**: El terreno ya no entierra los objetos colocados en el origen $Y=0$, permitiendo que el suelo plano y las primitivas reposen de forma natural.
  - Slider interactivo en la pestaña de Terreno para calibrar el desplazamiento base desde -35m hasta +15m en tiempo real.
  - **Reconstrucción Dinámica de Malla**: Selector de resolución (64x64, 128x128, 256x256, 512x512) y tamaño del mundo (40m a 500m) con botón de reconstrucción inmediata.
  - **Segmentación por Trozos (Chunks / Tiling)**: Controles de partición X/Z (de 1 a 8 chunks) con telemetría de densidad para preparación de streaming y LOD modular.
  - **Sistema de Pintura de Texturas con Pinceles (Splatmap Dinámico)**:
    - Selector de modo de herramienta: `[🏔️ Escultura 3D]` vs `[🎨 Pintar Texturas (Splatmap)]`.
    - 4 capas de pintura independientes: Césped / Hierba (R), Tierra / Sendero (G), Roca / Acantilado (B), Nieve Alpina (A).
    - Modificación directa y re-normalización continua de la textura RGBA8 en GPU con `glTexSubImage2D`.
    - Visualizador de pincel en el Viewport 3D adaptado al color de la capa seleccionada.
    - Botón para restaurar el bioma procedural base en cualquier momento.
- **Jerarquía de Escena (Outliner) - Iconos BMP y Sistema de Grupos (`EditorUI.cpp`, `Entity.h`, `Scene.h`, `Scene.cpp`, `SceneIO.cpp`, `ai_generator.py`)**:
  - **Corrección de Iconos Rotos**: Sustitución de caracteres SMP que generaban diamantes con signo de interrogación (``) por símbolos BMP geométricos limpios (`■`, `●`, `▲`, `▮`, `◎`, `▦`, `◈`, `★`) con colores dedicados idénticos a Blender 4.
  - **Sistema Completo de Grupos / Carpetas**:
    - Campo `m_group` en `Entity`, serializado en archivos `.asis` y expuesto a Lua con `entity:setGroup()` y `entity:getGroup()`.
    - Visualización en árbol desplegable en el Outliner con conteo de elementos y checkbox de visibilidad grupal en bloque.
    - Menú contextual de grupo: Seleccionar Todo, Mostrar/Ocultar, Desagrupar y Eliminar Grupo y Contenido.
    - Asignador rápido de grupo para el objeto seleccionado en la parte inferior del Outliner.
    - El Módulo de IA y las plantillas offline empaquetan automáticamente las piezas de estructuras complejas (Castillo, Auto, Creeper, Torre) en sus grupos correspondientes.
- **Cuadrícula Infinita Anti-Aliased Estilo Blender 4 (`assets/shaders/grid.vert`, `assets/shaders/grid.frag`, `main.cpp`, `EditorUI.cpp`)**:
  - Shader analítico de rejilla infinita con desproyección geométrica de planos cerca/lejos sobre $Y=0$.
  - Escritura exacta de profundidad en Z-Buffer (`gl_FragDepth`) para oclusión correcta con objetos y terreno.
  - Suavizado anti-aliasing con derivadas parciales `fwidth` para líneas menores (1m) y mayores (10m).
  - Ejes cartesianos coloreados (Rojo para eje X, Azul para eje Z) y atenuación cuadrática hacia el horizonte.
  - Botón de alternancia instantánea `[▦ Grid]` en la barra flotante superior del Viewport 3D.
- **Nueva Pestaña de Audio en la Interfaz Blender 4 (`EditorUI.h`, `EditorUI.cpp`)**:
  - Botón con icono vectorial de altavoz púrpura y ondas sonoras en el riel vertical de pestañas.
  - Sliders de volumen maestro, SFX, pisadas, música y ambiente con botones de prueba para cada efecto.
  - Indicadores de diagnóstico del backend Miniaudio.
- **Controlador de Personaje 3D Físico (`CharacterController.h`, `CharacterController.cpp`)**:
  - Simulación física con cuerpo rígido de cápsula Bullet Physics (`btCapsuleShape`, radio 0.38m, altura 1.80m, masa 75kg).
  - Bloqueo total de rotaciones angulares parásitas (`setAngularFactor(0,0,0)`) para estabilidad perfecta al colisionar con pendientes o cajas.
  - Prevención de reposo físico mediante `DISABLE_DEACTIVATION`, garantizando respuesta inmediata a controles.
  - Detección precisa de suelo mediante raycasting vertical analítico filtrado (`PhysicsWorld::rayTest`), excluyendo el propio cuerpo de la cápsula para evitar auto-colisiones.
  - Locomoción omnidireccional fluida con aceleración e inercia:
    - Marcha con WASD (ajustable de 1.0 a 15.0 m/s, predeterminado 4.5 m/s).
    - Sprint a alta velocidad con tecla `Shift` (ajustable de 2.0 a 25.0 m/s, predeterminado 9.0 m/s).
    - Agachado / Deslizamiento con `Ctrl` izquierdo o `C` (ajustable de 0.5 a 6.0 m/s, predeterminado 2.2 m/s).
    - Salto físico Bullet con tecla `Espacio` (impulso vertical instantáneo de 6.2 N).
  - Rotación y orientación de mirada con ratón (Yaw de 360° y Pitch limitado de -85° a +85° para evitar inversiones de cámara).
- **Modos de Cámara Dinámicos (FPS & TPS con Spring Arm)**:
  - **Primera Persona (FPS)**: Cámara posicionada directamente en los ojos del avatar (altura 1.65m), con sincronización angular estricta para control de tipo shooter o aventura en primera persona.
  - **Tercera Persona (TPS) con Brazo Elástico (*Spring Arm*)**:
    - Pivote a la altura de hombros (1.45m) con desplazamiento lateral (*shoulder offset* de 0.35m) para encuadre sobre el hombro estilo AAA.
    - Prevención activa de oclusión mediante raycast contra muros, cajas y relieve del terreno: si un obstáculo interfiere con la visual, la cámara se acorta dinámicamente con un margen de seguridad de 0.28m, evitando atravesar paredes o montañas.
    - Zoom dinámico de la cámara en tercera persona usando la rueda de desplazamiento del ratón (*mouse scroll*).
    - Conmutación en tiempo real entre primera y tercera persona pulsando la tecla `V` o mediante el botón en el Viewport.
- **Avatar 3D Procedural con Materiales PBR y Resplandor Bloom HDR**:
  - Malla tridimensional modular del personaje para tercera persona: torso y extremidades con aleación metálica pizarra (`metallic: 0.90`, `roughness: 0.22`), casco blanco estilizado, mochila de exploración dorsal y visor holográfico.
  - Visor holográfico emisivo cian (`emission: [0.10, 0.95, 1.0]`, `strength: 3.5x`) con resplandor intenso gracias al pipeline Bloom HDR y niebla atmosférica volumétrica.
  - Proyección de sombra dinámica completa del avatar sobre el terreno y objetos de la escena en el pase de Shadow Mapping 2K.
- **Sistema Play-in-Editor (PIE) & Flujo de Edición / Juego (`EditorUI.h`, `EditorUI.cpp`, `main.cpp`)**:
  - Máquina de estados con modos `EDITOR`, `PLAYING` y `PAUSED`.
  - Botón maestro `[ ▶ Jugar (Alt+P) ]` / `[ ⏹ Detener (Esc) ]` en la barra superior del menú, en la barra flotante del Viewport 3D y en el panel de propiedades.
  - Atajos globales `Alt+P` o `F5` para iniciar/detener el juego; tecla `Esc` para detener el juego sin cerrar la ventana principal del motor.
  - Preservación y restauración limpia de la cámara del editor y de los estados de simulación al salir del modo de juego.
  - Captura y bloqueo seguro del cursor del ratón con `GLFW_CURSOR_DISABLED` al jugar; liberación a `GLFW_CURSOR_NORMAL` al detener el juego.
  - Interfaz HUD en pantalla durante el juego:
    - Retícula / Crosshair central con punto y 4 marcas de puntería mediante `ImDrawList`.
    - Banner flotante inferior translúcido con los atajos de teclado (`[WASD] Mover | [Shift] Correr | [Espacio] Saltar | [V] 1ra/3ra Persona | [Esc] Salir`).
    - Telemetría en vivo en la toolbar del Viewport (Velocidad en km/h, estado de contacto con suelo / en el aire, modo de cámara).
- **Integración en la Interfaz de Usuario estilo Blender 4 (`EditorUI.h`, `EditorUI.cpp`)**:
  - Nueva pestaña `[Jugador / PIE]` en el riel vertical de iconos con ilustración vectorial nítida de mando de videojuegos (gamepad con cruceta D-pad, botones de colores y botón Play).
  - Panel completo de propiedades del personaje: sliders de velocidad de marcha, sprint, agachado, fuerza de salto, sensibilidad de ratón, distancia y offset de hombro del Spring Arm.
  - Telemetría en tiempo real: coordenadas de posición tridimensional, yaw/pitch, estado de suelo, y velocidad lineal horizontal.

---

## [2026-11-13] [FASE 14] Terreno Procedural 3D, Escultura en Tiempo Real (Sculpting Brushes), Colisiones Bullet y Niebla Atmosférica Volumétrica (Height Fog)
- **Sistema de Terreno 3D Dinámico Basado en Heightmaps (`Terrain.h`, `Terrain.cpp`, `terrain.vert`, `terrain.frag`)**:
  - Malla de terreno tridimensional en tiempo real de 128x128 vértices (16,384 vértices, 32,258 triángulos) abarcando 120m x 120m de superficie continua.
  - Almacenamiento continuo en array unidimensional `std::vector<float>` con subida dinámica a GPU vía `GL_DYNAMIC_DRAW` y `glBufferSubData`.
  - Recálculo analítico instantáneo de normales de superficie mediante diferencias centrales discretas ($dx = \text{size} / (N-1)$), garantizando sombras suaves e iluminación PBR sin artefactos facetados.
  - Preajustes de generación procedural integrados:
    - 🏔️ **Montañas Alpinas (Alpine Mountains)**: Ruido multifractal con aristas invertidas (`ridged noise`), cumbres afiladas y valles erosionados hasta 28m de altura.
    - 🌄 **Colinas Suaves (Rolling Hills)**: Ondulaciones sinusoidales suaves con 4 octavas de fBm de baja frecuencia hasta 9.5m.
    - 🏜️ **Cañones y Mesetas (Canyons / Plateau)**: Estratificación y terrazas escalonadas con paredes abruptas y cimas planas.
    - 🟩 **Plano Base (Flat Plane)**: Restablecimiento a cota cero uniforme.
  - Generador de ruido fractal procedural personalizable con ajuste interactivo de altura máxima, frecuencia, octavas de detalle, persistencia y generador de semillas aleatorias (`seed`).
- **Herramientas de Escultura 3D en Tiempo Real (Real-Time Sculpting Brushes) (`Terrain.h`, `Terrain.cpp`, `EditorUI.h`, `EditorUI.cpp`)**:
  - Raycasting analítico en CPU de alta velocidad (marcha de rayos con refinamiento binario de 10 pasos) que detecta la posición de impacto del cursor y la normal del terreno en $< 0.01$ ms.
  - Pincel visual circular proyectado en 3D sobre la orografía del relieve dentro del Viewport mediante `ImDrawList::AddPolyline`, adaptando su contorno a la pendiente de las laderas.
  - Modos de pincel interactivos:
    - 🖌️ **Subir (Raise)**: Eleva el terreno suavemente según el perfil de atenuación.
    - 🔻 **Bajar (Lower)**: Desciende el terreno creando depresiones y valles.
    - 🌊 **Suavizar (Smooth)**: Relaja y homogeniza las pendientes promediando alturas vecinas mediante suavizado Laplaciano.
    - 📏 **Aplanar (Flatten)**: Iguala la altura hacia un valor objetivo o la cota del punto de impacto.
    - ⛰️ **Rugosidad (Noise / Detail)**: Inyecta micro-variaciones procedurales para simular formaciones rocosas abruptas.
  - Perfiles de atenuación radial configurables: Suave (Coseno), Esférico, Lineal y Plano.
  - Parámetros dinámicos: Radio del pincel (1.0m a 35.0m), Fuerza/Intensidad (0.1x a 4.0x) y Altura Objetivo.
  - Atajos ergonómicos de teclado idénticos a Blender y Unreal Engine:
    - Clic Izquierdo + Arrastrar: Esculpir con el pincel activo.
    - `Ctrl` + Clic Izquierdo: Invierte a bajar terreno.
    - `Shift` + Clic Izquierdo: Conmuta dinámicamente a suavizado.
- **Splatmapping Procedural PBR Multicapa de Terreno (`terrain.vert`, `terrain.frag`)**:
  - Sombreado de 4 biomas orgánicos interconectados por pendiente ($\cos \theta = \vec{N} \cdot \vec{Y}$) y altitud:
    - **Pasto de Llanura**: Verde templado orgánico en valles y planicies bajas ($Y < 2.0$m).
    - **Tierra Fértil**: Suelo marrón rico en cotas intermedias ($2.0\text{m} < Y < 12.0\text{m}$).
    - **Roca de Acantilado**: Textura granítica con estrías minerales que domina en paredes escarpadas con pendiente $> 40^\circ$ ($\cos \theta < 0.76$).
    - **Nieve Alpina**: Capa de nieve blanca pura en cumbres altas ($Y > 14.0$m a $22.0$m).
  - Micro-ruido textural continuo a múltiples escalas para evitar repeticiones visuales y artefactos de tiling a cualquier distancia.
  - Pipeline de iluminación PBR Cook-Torrance completo: Sol direccional, sombras 4K con filtrado PCF ajustable, hasta 8 luces puntuales, e irradiancia difusa y reflejos especulares IBL del Skybox HDRI.
  - Mapeo tonal seleccionable (ACES Filmic, AgX, Reinhard, Linear), exposición, saturación, contraste y corrección gamma.
- **Colisiones Físicas 3D con Bullet Physics (`PhysicsWorld.h`, `PhysicsWorld.cpp`, `Terrain.h`, `Terrain.cpp`)**:
  - Creación e instanciación de cuerpo rígido estático con forma `btHeightfieldTerrainShape` conectado directamente al búfer de alturas del terreno.
  - Escalado y centrado automático de AABB en Bullet (`setLocalScaling` y centrado en $(Y_{min} + Y_{max}) \times 0.5$).
  - Sincronización instantánea de geometría física tras finalizar cada trazo de escultura o cambio de preset (`syncPhysics`).
  - Control en UI para habilitar/deshabilitar la colisión del terreno y alternar con el plano físico base en $Y = 0$.
- **Niebla Atmosférica Volumétrica (Height Fog - Opción D) (`terrain.frag`, `model.frag`, `Terrain.h`, `EditorUI.h`, `EditorUI.cpp`, `main.cpp`)**:
  - Integración óptica exponencial basada en la altitud de la cámara y del fragmento:
    $\tau = d \times \rho_0 \times \frac{e^{-b(C_y - Y_0)} - e^{-b(P_y - Y_0)}}{b(P_y - C_y)}$
  - Dispersión solar direccional (*Atmospheric Inscattering*): resplandor dorado cálido hacia el sol con decaimiento angular $(\vec{V} \cdot \vec{L}_{\text{sun}})^8$.
  - Implementado tanto en el shader de terreno (`terrain.frag`) como en el shader principal de modelos (`model.frag`), unificando todos los objetos tridimensionales bajo la misma atmósfera.
  - Preajustes rápidos: 🌅 Bruma Matutina, 🌫️ Valle Brumoso Denso, 🌇 Atardecer Dorado y ☀️ Cielo Despejado.
  - Parámetros interactivos: densidad global, altura base (nivel de valle), caída vertical (falloff), color ambiental, color de dispersión solar e intensidad del resplandor solar.
- **Integración en la Interfaz de Usuario estilo Blender 4 (`EditorUI.h`, `EditorUI.cpp`)**:
  - Nueva pestaña `[Terreno]` en el riel vertical de iconos de propiedades con ilustración vectorial propia de cordillera alpina con picos nevados y sol naciente.
  - Paleta completa de escultura, gestión de presets procedurales, controles de colisiones físicas Bullet y panel de ajuste de niebla volumétrica.
  - Corrección de la apertura de archivos por línea de comandos ("Abrir con..."): resolución absoluta previa de rutas antes de fijar el directorio de trabajo del proceso.

## [2026-11-13] [FASE 13] Sistema de Partículas 3D (VFX), Oclusión Ambiental (SSAO), Profundidad de Campo (DoF/Bokeh), Animaciones Esqueléticas (Assimp Bones) y Notificaciones de Render
- **Corrección Integral del Guardado y Visibilidad de Renders / Capturas (`F12`) (`EditorUI.h`, `EditorUI.cpp`, `main.cpp`)**:
  - Resuelto el problema de localización de las capturas de imagen: creación automática y aseguramiento de la carpeta `renders/` con rutas absolutas.
  - Nuevo comando en la barra superior: `Render -> 📂 Abrir Carpeta de Capturas (Renders)...` que invoca el Explorador de Windows (`ShellExecuteA`) directamente sobre el directorio de capturas.
  - Notificación emergente flotante en pantalla (toast de 6 segundos) tras capturar con éxito (`📸 ¡Render Guardado con Éxito!`) con botones interactivos `[📂 Abrir Carpeta]` y `[🖼️ Ver Imagen]` (abre la imagen en el visor predeterminado de Windows).
  - Detección directa a nivel de hardware para la tecla `GLFW_KEY_F12` en el bucle principal de `main.cpp` que garantiza la captura incluso si ImGui tiene elementos de texto enfocados.
- **Sistema de Partículas 3D (VFX Particle Engine) con Renderizado Instanciado en GPU (`ParticleSystem.h`, `ParticleSystem.cpp`, `particle.vert`, `particle.frag`)**:
  - Renderizado billboarding orientado automáticamente hacia la cámara mediante vectores directores `right` y `up` extraídos de la matriz de vista.
  - Instanciación por hardware OpenGL (`glDrawArraysInstanced`) con buffer VBO dinámico para altas tasas de cuadros con miles de partículas activas en pantalla.
  - Preajustes nativos de un clic:
    - 🔥 **Fuego (Fire)**: Partículas calientes ascendentes con gradiente de amarillo-naranja a rojo y desvanecimiento a ceniza.
    - ✨ **Chispas (Sparks)**: Partículas luminosas de alta velocidad con gravedad y dispersión angular.
    - 💨 **Humo (Smoke)**: Columnas densas con expansión de tamaño y mezcla alfa suave.
    - 🌧️ **Lluvia (Rain)**: Gotas veloces descendentes simulando tormenta desde el cielo.
    - 🔮 **Magia (Magic)**: Orbes místicos resplandecientes en espiral con mezcla aditiva.
    - ⚙️ **Emisor Personalizado (Custom)**: Configuración libre de todos los parámetros físicos y cromáticos.
  - Shader procedural de partículas (`particle.frag`) con cálculo de forma circular suave y brillo incandescente con decaimiento cuadrático.
  - Integración en la interfaz:
    - Nuevo botón en el riel de iconos verticales estilo Blender 4 (`[VFX]`) con ilustración vectorial propia de estrella resplandeciente.
    - Menú `Añadir ➕ -> ✨ Sistema de Partículas (VFX)` y modal rápido `Shift+A`.
    - Inspector de emisores: lista interactiva, tasa de emisión, partículas máximas, tiempo de vida, color inicial y final RGBA, tamaño inicial y final, gravedad XYZ, velocidad con dispersión y conmutación entre modo aditivo y alfa.
- **Oclusión Ambiental en Espacio de Pantalla (SSAO) Bilateral (`SSAO.h`, `SSAO.cpp`, `ssao.frag`, `ssao_blur.frag`, `ssao_composite.frag`, `Framebuffer.h`, `Framebuffer.cpp`)**:
  - Reconstrucción de posiciones tridimensionales en espacio de vista a partir de la textura de profundidad de 24 bits (`GL_DEPTH24_STENCIL8`) del FBO de escena.
  - Reconstrucción analítica de normales de superficie en espacio de vista mediante derivadas parciales en pantalla (`dFdx(fragPos)` / `dFdy(fragPos)`), evitando la sobrecarga de un G-buffer de normales desacoplado.
  - Hemisferio estocástico de 32 muestras orientadas según la normal con distribución cuadrática de densidad hacia el centro y rotación aleatoria con mapa de ruido de tangentes 4x4.
  - Filtro bilateral/box blur separable para eliminar el ruido estocástico preservando aristas y esquinas.
  - Shader de composición que atenúa la componente ambiental sin tocar las altas luces solares ni especulares.
  - Controles interactivos en `Render` y `Preferencias (Ctrl+,)`: conmutador SSAO, radio de muestreo (0.1m - 3.0m), sesgo de profundidad (bias), curva/potencia de contraste e intensidad de oclusión.
- **Profundidad de Campo Fotográfica (Depth of Field / Bokeh) (`PostProcess.h`, `PostProcess.cpp`, `dof.frag`)**:
  - Linearización de la profundidad de la escena y cálculo normalizado del Círculo de Confusión (Circle of Confusion / CoC).
  - Desenfoque bokeh mediante kernel Poisson Disc de 16 muestras con ponderación no lineal de luminancia (`highlight weighting`), logrando discos luminosos circulares característicos de lentes fotográficos cinematográficos.
  - Controles interactivos en `Preferencias` y menú `Render`: distancia focal, rango nítido y radio de bokeh en píxeles.
  - Herramienta rápida de un clic `[🎯 Enfocar en Objeto Seleccionado]` que mide la distancia tridimensional exacta entre la cámara y la entidad activa.
- **Animaciones Esqueléticas y Skinned Meshes (Assimp Rigging) (`Animation.h`, `Animation.cpp`, `Mesh.h`, `Mesh.cpp`, `Model.h`, `Model.cpp`, `model.vert`)**:
  - Ampliación de la estructura `Vertex` para registrar hasta 4 influencias de huesos por vértice (`boneIDs[4]`, `weights[4]`) vinculadas a los atributos 4 y 5 de OpenGL (`glVertexAttribIPointer` / `glVertexAttribPointer`).
  - Skinning por hardware en GPU dentro de `model.vert` con soporte de hasta 100 transformaciones de matriz de huesos (`u_BoneTransforms[100]`) y activación selectiva (`u_IsAnimated`).
  - Extracción y decodificación completa de la jerarquía de huesos y canales de animación Assimp (`aiNodeAnim`) para traslación, escala y rotación por cuaterniones con interpolación esférica (`glm::slerp`).
  - Clases `SkeletalAnimation` y `Animator` para control de tiempo de animación, cálculo de matrices locales a hueso y transformación jerárquica padre-hijo.
  - Pestaña dedicada `[Animación]` en el panel de propiedades estilo Blender 4 con icono vectorial de claqueta de cine:
    - Identificación automática de clips de animación y conteo de huesos en la armadura.
    - Controles interactivos de reproducción: `[▶ Reproducir]`, `[⏸ Pausar]`, `[⏹ Detener / Reset]` y selector de bucle (loop).
    - Deslizador de línea de tiempo interactivo (timeline scrubber) para avanzar o retroceder a cualquier fotograma clave (tick).
    - Control de velocidad de reproducción (0.1x a 3.0x).
    - Tabla inspectora de la jerarquía de huesos con ID y nombres.
- **Compatibilidad Total con "Abrir con..." y Argumentos de Arranque en Windows (`main.cpp`, `SceneIO.h`)**:
  - Resuelto el fallo de rutas relativas de Windows cuando se ejecuta el motor desde "Abrir con..." o haciendo doble clic en un archivo:
    - Invocación de `GetModuleFileNameW` al inicio para recuperar la ruta absoluta del ejecutable y fijar el directorio de trabajo del proceso a su carpeta contenedora (`std::filesystem::current_path(p.parent_path())`).
    - Procesamiento de argumentos de línea de comandos `argv[1]` en `main(int argc, char* argv[])`.
    - Detección automática por extensión: carga directa de escenas nativas `.asis` (`SceneIO::loadSceneASIS`), importación de modelos 3D (`SceneIO::importModel`) o carga de mapas `.hdr` en el Skybox.
  - Extensión del sistema de Arrastrar y Soltar (Drag & Drop) en la ventana para soportar directamente archivos de escena `.asis` y mapas `.hdr` sin pasar por menús.
- **Antialiasing Real en el Viewport 3D con Hardware Multisample FBO y FXAA (`Framebuffer.h`, `Framebuffer.cpp`, `PostProcess.h`, `PostProcess.cpp`, `fxaa.frag`)**:
  - Reemplazo del Framebuffer clásico por un sistema de doble FBO con muestreo múltiple en hardware:
    - FBO de Renderizado MSAA (`m_msaaFBO`) con Renderbuffers multirresueltos para color (`glRenderbufferStorageMultisample`) y profundidad/stencil con soporte de 1x (desactivado), 2x, 4x y 8x muestras.
    - Resolución automática vía hardware mediante `glBlitFramebuffer` hacia el FBO de textura 2D en formato de alto rango dinámico `GL_RGBA16F`.
    - Ahora el selector de Antialiasing en `Preferencias (Ctrl+,)` y en la barra de herramientas produce un suavizado nítido y real en los bordes poligonales dentro de los paneles acoplables de Dear ImGui.
  - Implementación de filtro de post-procesamiento **FXAA 3.11** (Fast Approximate Anti-Aliasing):
    - Detección de contraste local de luminancia y sub-pixel blending en espacio de pantalla con cero impacto en la tasa de cuadros por segundo.
- **Iluminación Basada en Imágenes (IBL) y Fondo Skybox HDRI 360° (`Skybox.h`, `Skybox.cpp`, `skybox.vert`, `skybox.frag`, `model.frag`)**:
  - Clase `Skybox` con geometría de cubo de radio 1.0 y proyección esférica equirrectangular (`dirToUV`).
  - Cargador nativo de mapas de entorno HDR de punto flotante de 32 bits por canal (`stbi_loadf`) en formato equirrectangular `.hdr` y soporte para panorámicas 360° LDR (`.png`/`.jpg`).
  - Generador de cielo procedural atmosférico predeterminado en caso de no cargar un archivo HDRI externo: gradiente cielo-horizonte con disco solar radiante y dispersión atmosférica.
  - Renderizado del fondo de cielo en el Viewport 3D en profundidad infinita (`glDepthFunc(GL_LEQUAL)`):
    - Control de visibilidad del cielo (activar/desactivar).
    - Rotación en 360° del mapa de entorno para orientar la luz.
    - Control fino de exposición y brillo ambiental.
    - Control de desenfoque de fondo (LOD / Blur) para simular profundidad de campo fotográfica sin afectar a los objetos de la escena.
  - Integración en el shader PBR (`model.frag`):
    - **Reflexiones especulares de entorno**: muestreo equirrectangular del mapa HDRI en la dirección de reflexión $R = \text{reflect}(-V, N)$ rotada en $Y$, con nivel de detalle (LOD) proporcional a `roughness * 7.0`, logrando metales brillantes como espejos cromados y materiales rugosos con reflejos perfectamente difuminados.
    - **Irradiancia difusa ambiental**: modulación por Fresnel-Schlick con rugosidad (`fresnelSchlickRoughness`), aportando un tinte de color realista del entorno sobre todas las sombras y caras no iluminadas directamente por el sol.
- **Pipeline de Post-Procesado y Bloom HDR (`PostProcess.h`, `PostProcess.cpp`, `bloom_extract.frag`, `bloom_blur.frag`, `bloom_composite.frag`)**:
  - Procesamiento en espacio de color HDR (`GL_RGBA16F`) que conserva intensidades superiores a 1.0 provenientes de fuentes emisivas, materiales reflectantes e iluminación solar directa.
  - Fase 1 (Extracción): shader `bloom_extract.frag` que aísla selectivamente los píxeles cuya luminancia sobrepasa el umbral configurable (Threshold).
  - Fase 2 (Desenfoque Gaussiano): sistema de buffers ping-pong a media resolución ($W/2 \times H/2$) con kernel separable horizontal y vertical de 5 muestras para crear un halo amplio y cinematográfico.
  - Fase 3 (Composición Aditiva): shader `bloom_composite.frag` que suma la escena original con el resplandor de Bloom ponderado por la intensidad del usuario.
- **Captura de Pantalla y Render a Imagen PNG (`F12` / `Render -> Renderizar Imagen a PNG...`) (`PostProcess.cpp`, `EditorUI.cpp`, `main.cpp`)**:
  - Atajo rápido global `F12` y opción en el menú superior `Render`.
  - Captura directa del buffer de color del Viewport 3D limpio (sin gizmos ni elementos de UI superpuestos) con resolución completa.
  - Guardado automático en formato PNG de 24 bits sin pérdidas en la carpeta `renders/` con marca de tiempo precisa (`render_YYYYMMDD_HHMMSS.png`).
- **Nuevos Controles en Interfaz de Usuario (`EditorUI.h`, `EditorUI.cpp`)**:
  - Pestaña de Iluminación (`[Luz]`): Nueva sección de **Skybox e Iluminación de Entorno (HDRI / IBL)** con botón para examinar y cargar archivos `.hdr`, selector de visibilidad, sliders de exposición, rotación 360°, desenfoque de fondo e intensidad de IBL.
  - Menú superior `Render`: Botón de renderizado de imagen `F12`, activación de Bloom y selector de perfiles de Antialiasing.
  - Modal de Preferencias (`Ctrl+,`): Nuevas opciones para conmutar FXAA y configurar el umbral e intensidad de Bloom HDR.

---

## [2026-11-13] [FASE 11] Formato de Escena Nativo .asis, Deshacer/Rehacer (Undo/Redo), Primitivas 3D Procedurales, Luces en Jerarquía y Gestión de Color ACES/AgX
- **Corrección de Texturas/Colores Opacos y Gestión de Color Avanzada (`model.frag`, `EditorUI.h`, `EditorUI.cpp`, `main.cpp`)**:
  - Resuelto el lavado de color y tonalidades pastel opacas causadas por doble corrección gamma: linearización estricta de lecturas de texturas sRGB (`pow(tex.rgb, vec3(2.2))`) previo al cálculo microfacetal Cook-Torrance GGX.
  - Implementación de curvas de Mapeo Tonal profesionales en GLSL:
    - **ACES Filmic (Predeterminado)**: Curva ajustada Narkowicz ACES con rango dinámico cinematográfico, negros profundos y alta saturación de luces.
    - **AgX (Blender 4.0)**: Manejo de sobreexposición sin cambio indeseado de tono (hue shift).
    - **Reinhard**: Compresión tonal fotográfica clásica $L / (1.0 + L)$.
    - **Lineal**: Paso directo sin compresión tonal.
  - Controles interactivos en el editor: Exposición (0.1x .. 5.0x), Gamma (1.0 .. 3.2), Saturación (0.0x .. 2.5x), Contraste (0.5x .. 2.0x) y preajustes rápidos (*Vibrante / Rico*, *Cinemático AgX*, *Neutro / Fiel*).
- **Formato Nativo de Escena ASIS Engine `.asis` (`SceneIO.h`, `SceneIO.cpp`)**:
  - Guardado (`Ctrl+S`, Guardar Como) y apertura (`Ctrl+O`) de escenas completas en formato JSON estructurado `.asis`.
  - Serialización total de:
    - Versión del motor (`asis_engine_version: "0.11"`).
    - Parámetros de Cámara (posición, orientación pitch/yaw, campo de visión FOV, planos near/far, ortográfica).
    - Sol direccional, colores ambientales y parámetros de sombras 4K (resolución, kernel PCF, suavizado de penumbra).
    - Gestión de color (perfil tonal, exposición, gamma, saturación, contraste).
    - Entidades 3D (nombre, tipo de primitiva, transformaciones XYZ, visibilidad, color RGB y todos los valores físicos PBR).
    - Luces puntuales activas en la escena.
  - Sistema de Autoguardado periódico en segundo plano (`autosave.asis`) con intervalo regulable.
- **Sistema Completo de Deshacer y Rehacer (`Undo / Redo`) (`EditorUI.h`, `EditorUI.cpp`)**:
  - Atajos universales estándar `Ctrl+Z` (Deshacer) y `Ctrl+Y` / `Ctrl+Shift+Z` (Rehacer).
  - Pila de instantáneas (`SceneSnapshot` / `EntitySnapshot`) de profundidad configurable (16 a 256 pasos).
  - Captura automática de instantánea previa antes de manipulación con Gizmos, creación de primitivas, limpieza de escena, importaciones o borrado de objetos.
  - Sincronización transparente con el mundo de físicas de Bullet.
- **Generador de Primitivas 3D Procedurales y Menú `Shift+A` (`Mesh.h`, `Mesh.cpp`, `Scene.h`, `Scene.cpp`, `EditorUI.cpp`)**:
  - Incorporadas mallas procedurales nativas de alta resolución calculadas matemáticamente:
    - **Esfera UV (`createSphere`)**: Con anillos de latitud y segmentos de longitud, coordenadas UV y normales analíticas.
    - **Cilindro (`createCylinder`)**: Pared cilíndrica con tapas superior e inferior circulares con normales desacopladas para bordes limpios.
    - **Cono (`createCone`)**: Cúspide y base plana con normales tangenciales.
    - **Toroide / Donut (`createTorus`)**: Anillo circular tridimensional con radio mayor y radio de sección tubular.
    - **Plano (`createPlane`)**: Malla cuadriculada horizontal orientada en $+Y$.
    - **Cubo (`createCube`)**: Bloque poliédrico estándar de 6 caras.
  - Menú emergente contextual `Shift+A` (estilo Blender) y menú de barra superior `Añadir ➕` para instanciar cualquier primitiva o luz en el punto de foco de la cámara con un solo clic.
- **Luces Puntuales en la Jerarquía de Escena (`Jerarquia de Escena`) y Selección 3D (`EditorUI.cpp`)**:
  - Las luces puntuales ahora aparecen directamente listadas en el Outliner con icono `💡`, estado activo y selector.
  - Selección interactiva de luces con Gizmo 3D de traslación acoplado en el Viewport para moverlas intuitivamente en el espacio tridimensional.
  - Marcadores visuales circulares 3D proyectados en pantalla en el Viewport que permiten seleccionar luces haciendo clic izquierdo directamente sobre ellas.
  - Control de visibilidad del Plano de Suelo Base (`▦ Plano de Suelo`) y de la Cuadrícula 3D desde la jerarquía y el menú `Ver`.
- **Ventana Modal de Preferencias y Configuración (`Ctrl+,`) (`EditorUI.h`, `EditorUI.cpp`)**:
  - Acceso desde `Editar -> Preferencias y Configuración...` o mediante el atajo `Ctrl+,`.
  - Pestañas organizadas:
    - **🖥️ Gráficos y Render**: VSync dinámico, Límite de FPS (Ilimitado, 30, 60, 120, 144, 240), Antialiasing MSAA y Curva Tonal predeterminada.
    - **🎨 Tema e Interfaz**: Conmutador de estilos visuales (*Blender 4 Dark*, *Unreal Engine 5*, *OLED Midnight*, *Cyberpunk*, *Personalizado*) con selector de color de acento y velocidad de cámara de vuelo (Flycam).
    - **⌨️ Atajos de Teclado**: Guía completa de comandos y atajos conmutables.
    - **⚙️ Sistema y Deshacer**: Configuración del historial de Undo/Redo y parámetros de autoguardado.

---

## [2026-11-13] [FASE 10] Sistema de Materiales PBR (Cook-Torrance GGX), Sombras Ultra-HD 4K con Penumbra Regulable, Multiluz y Control de Cámaras
- **Pipeline de Materiales PBR Físicamente Realistas (`Material.h`, `Material.cpp`, `model.frag`)**:
  - Implementación completa del modelo de microfacetas **Cook-Torrance GGX** en GLSL con distribución normal Trowbridge-Reitz, función geométrica Smith Schlick-GGX y aproximación de Fresnel de Schlick.
  - Soporte de parámetros físicos PBR:
    - **Albedo / Base Color**: Reflectancia difusa base (RGB) y textura sRGB.
    - **Roughness (Rugosidad)**: Rango microfacetal de 0.04 (espejo pulido) a 1.0 (mate difuso).
    - **Metallic (Metalicidad)**: Rango de 0.0 (dieléctricos como plástico o madera) a 1.0 (metales puros como oro o cromo).
    - **Specular**: Reflectancia especular regulable.
    - **Normal / Bump Mapping**: Perturbación de normales mediante derivadas de espacio de pantalla en GLSL (`dFdx`/`dFdy`) sin requerir tangentes precalculadas en el layout de vértices.
    - **Emission (Emisión)**: Color de resplandor propio y multiplicador de intensidad para superficies auto-iluminadas / neón.
    - **Ambient Occlusion (AO)**: Oclusión ambiental para contraste de micro-sombras en grietas y hendiduras.
  - 7 Ranuras de mapas de texturas en unidades de textura OpenGL `0..6` con conmutadores dinámicos `useMap`.
  - Normal flat estática `(128, 128, 255, 255)` vía `Texture::getDefaultNormal()`.
  - Extracción automática de texturas y materiales Assimp en `Model.cpp` (resolviendo tanto `DIFFUSE` como `BASE_COLOR`, `NORMALS`, `METALNESS`, `ROUGHNESS`, etc.) con normalización de rutas de Windows (`\\` a `/`) y búsqueda recursiva en carpetas `textures/`.
- **Riel Vertical Blender 4: Icono Vectorial de Materiales PBR (`EditorUI.cpp`)**:
  - Incorporado el icono vectorial oficial de materiales de Blender 4: esfera circular rosa/roja (`#E13C5F`) con arco elíptico ecuatorial y brillo especular blanco en la esquina superior izquierda.
  - Panel interactivo de Materiales PBR en Dear ImGui con selectores de color, sliders en tiempo real y botones para cargar o remover texturas individuales mediante el explorador nativo de Windows.
  - Colección de Presets PBR de 1 clic: *Oro Pulido*, *Cobre*, *Cromo / Espejo*, *Plástico Brillante*, *Cerámica Mate* y *Neón Cian Emisivo*.
- **Sombras Dinámicas Ultra-HD (1K / 2K / 4K) con Penumbra Regulable (`ShadowMap.h`, `ShadowMap.cpp`, `model.frag`)**:
  - Soporte dinámico de redimensionamiento de textura de profundidad mediante `ShadowMap::resize(width, height)`.
  - Conmutador en interfaz entre **1K (1024x1024)**, **2K (2048x2048)** y **4K Ultra-HD (4096x4096)**.
  - Filtrado PCF (Percentage Closer Filtering) multi-núcleo seleccionable: **3x3 (9 muestras)**, **5x5 (25 muestras)** y **7x7 (49 muestras)**.
  - Slider de suavizado y dispersión de penumbra (`u_ShadowSoftness`).
- **Sistema de Iluminación Multiluz (Point Lights) (`Light.h`, `Scene.h`, `model.frag`)**:
  - Estructura `PointLight` con posición 3D, color RGB, intensidad y atenuación física realista ($1.0 / (c + l \cdot d + q \cdot d^2)$).
  - Bucle de cálculo PBR para hasta 8 luces puntuales simultáneas en el fragment shader.
  - Gestor de luces en el editor UI con botón `[➕ Añadir Luz Puntual]`, nombres editables, color pickers, sliders de posición e intensidad, y conmutador de activación por luz.
- **Sistema de Cámaras y Lente (`Camera.h`, `Camera.cpp`, `EditorUI.cpp`)**:
  - Campo de visión (FOV) regulable de 15° a 110° con presets rápidos (*Telefoto 30°*, *Normal 45°*, *Gran Angular 75°*, *Ojo de Pez 100°*).
  - Conmutador de Proyección Ortográfica (Isométrica) vs. Proyección Perspectiva estándar.
  - Control de planos de recorte cercano (`Near Plane`) y lejano (`Far Plane`).

---

## [2026-11-13] [FASE 9] Importación y Exportación Multiformato 3D (Assimp, OBJ/MTL, glTF 2.0, Drag & Drop) & Riel Vectorial Ilustrado
- **Ilustraciones Vectoriales Nítidas en el Riel Vertical de Blender 4 (`EditorUI.cpp`)**:
  - Reemplazadas las siglas de texto (`[Obj]`, `[Luz]`, `[Fis]`, `[Lua]`) por **ilustraciones vectoriales dibujadas con `ImDrawList`**:
    - **Objeto**: Cubo isométrico 3D tridimensional con caras sombreadas (superior iluminada, laterales media y oscura) y contorno en naranja Blender (`#E87D3E`).
    - **Iluminación**: Sol radiante con núcleo de luz y 8 rayos exteriores proyectados a 45° en amarillo solar (`#FFDF32`).
    - **Física**: Esfera dinámica en azul celeste con anillo orbital inclinado a -30° y línea de impacto/contacto en el suelo.
    - **Scripts / Lógica**: Par de corchetes angulares de código `< / >` en verde esmeralda neón (`#46E682`).
    - **Importación / Exportación (I/O)**: Contenedor/caja abierta 3D en púrpura con flechas de entrada y salida vertical en cian eléctrico (`#5CE0FF`).
  - Cada icono cuenta con fondos redondeados, indicador activo lateral idéntico a Blender 4, resaltado reactivo al pasar el cursor (`hover`) y tooltips explicativos completos.
- **Módulo de Entrada y Salida 3D (`SceneIO.h`, `SceneIO.cpp`)**:
  - **Importador Universal de Modelos 3D**:
    - Soporte para `.obj`, `.gltf`, `.glb`, `.fbx`, `.stl`, `.ply`, `.dae`, `.3ds` y `.blend` impulsado por Assimp.
    - Normalización inteligente de escala basada en el cuadro delimitador (AABB) de todas las submallas del modelo.
    - Reposo automático sobre la cuadrícula: ajusta la altura $Y$ para que la base del modelo repose firmemente sobre el plano del suelo ($Y = 0.0$).
    - Nombres únicos automáticos en la jerarquía de la escena (`modelo`, `modelo_1`, `modelo_2`).
  - **Exportador Wavefront OBJ + Materiales MTL (`exportSceneToOBJ`)**:
    - Exporta todas las entidades visibles (bloques voxel, mallas, modelos combinados) a un archivo `.obj` con coordenadas de mundo aplicadas mediante su matriz de transformación.
    - Genera archivo complementario `.mtl` con nombres de materiales únicos y colores RGB difusos exactos (`Kd r g b`) para cada objeto.
    - 100% compatible para abrir directamente en **Blender**, **Unreal Engine 5**, **Unity**, **Maya**, **3ds Max** y el **Visor 3D de Windows**.
    - Opción de exportar la escena completa o exportar exclusivamente el objeto seleccionado (`exportEntityToOBJ`).
  - **Exportador glTF 2.0 Estándar (`exportSceneToGLTF`)**:
    - Genera un archivo `.gltf` 2.0 auto-contenido con buffer binario codificado en Base64 para geometrías combinadas (posiciones, normales, coordenadas UV e índices).
    - Compatible con visores glTF web (Three.js, Babylon.js) y herramientas modernas de producción.
  - **Integración con Diálogos Nativos del Explorador de Windows**:
    - Ventanas modulares del explorador de archivos de Windows (`GetOpenFileNameA` y `GetSaveFileNameA`) para explorar carpetas, discos duros o Descargas al importar o guardar.
  - **Arrastrar y Soltar en Tiempo Real (Drag & Drop)**:
    - Integrado `glfwSetDropCallback` en `Window` y `main.cpp`: arrastrar cualquier archivo 3D desde una carpeta de Windows y soltarlo directamente dentro de la ventana del motor lo carga inmediatamente como nueva entidad interactiva.
- **Ampliación de la API de Scripting Lua 5.4**:
  - `Engine.importModel(path, [x, y, z])`: Permite importar modelos 3D externos directamente desde código Lua.
  - `Engine.exportScene(path)`: Exporta la escena completa a disco desde scripts.

---

## [2026-11-13] [FASE 8] Motor de Física 3D en Tiempo Real (Bullet Physics) & Panel de Propiedades Estilo Blender
- **Integración de Bullet Physics 3 (`PhysicsWorld.h`, `PhysicsWorld.cpp`)**:
  - Instalada y enlazada la biblioteca industrial `bullet3` (v3.25) mediante vcpkg (`find_package(Bullet CONFIG REQUIRED)`).
  - Inicialización completa del pipeline de física con:
    - `btDefaultCollisionConfiguration` y `btCollisionDispatcher`.
    - `btDbvtBroadphase` para detección acelerada de colisiones amplias.
    - `btSequentialImpulseConstraintSolver` para resolución precisa de contactos y restricciones.
    - `btDiscreteDynamicsWorld` con gravedad predeterminada en `(0, -9.81 m/s², 0)`.
  - **Suelo Físico Estático Infinito**:
    - Creado cuerpo estático colisionador (`btStaticPlaneShape` / `btBoxShape`) en `Y = -1.0f` con superficie superior en `Y = 0.0f` para recibir todos los objetos dinámicos de la escena.
  - **Componente RigidBody para Entidades (`RigidBodyComponent`)**:
    - Formas de colisión automáticas (`btBoxShape` ajustado a las dimensiones de escala de la entidad).
    - Inercia y masa configurable (`mass > 0` para cuerpos dinámicos que caen y rebotan; `mass = 0` para obstáculos o plataformas estáticas).
    - Parámetros físicos avanzados: restitución (rebote), fricción y amortiguamiento lineal/angular.
  - **Bucle de Simulación de Física a 60 Hz con Sub-stepping**:
    - `stepSimulation(dt, 10, 1.0f / 60.0f)` para una simulación estable e independiente del framerate gráfico.
  - **Sincronización Bidireccional Total**:
    - **Física a Escena**: En cada frame de simulación, la posición y rotación de Bullet actualizan la matriz de transformación de la `Entity`.
    - **Gizmo/Inspector a Física**: Si el usuario manipula una entidad con ImGuizmo o edita coordenadas numéricas en el Inspector, el `btRigidBody` correspondiente se teletransporta suavemente y se despierta (`activate(true)`) limpiando velocidades residuales.
  - **Extensión de la API de Scripting Lua 5.4**:
    - `entity:enablePhysics(mass)`: Activa la simulación física de Bullet para la entidad especificada.
    - `entity:applyImpulse(fx, fy, fz)`: Aplica un vector de fuerza impulsiva instantánea al centro de masa del cuerpo.
    - `Engine.setGravity(gx, gy, gz)`: Permite alterar la aceleración gravitacional global en caliente desde scripts.
- **Rediseño del Panel Lateral Derecho: Riel Vertical de Íconos Estilo Blender 4**:
  - Reemplazadas las pestañas horizontales superiores por un elegante riel vertical de botones/íconos en el costado izquierdo del panel `"Propiedades"`, idéntico a la ergonomía de Blender 4:
    - **`[Obj]` Objeto**: Transformación (Posición, Rotación Euler, Escala), color RGB interactivo, visibilidad, botón de re-enfoque de cámara (`F`) y reset de transformación.
    - **`[Luz]` Iluminación**: Sol direccional, azimut, elevación solar, color, intensidad, luz ambiental, presets horarios (Mediodía, Atardecer Dorado, Estudio) y calidad de sombras (Shadow Bias con PCF 2K).
    - **`[Fis]` Física**: Control global de simulación (pausar/reanudar, conteo de cuerpos activos), vector de gravedad con presets planetarios (*Tierra -9.81*, *Luna -1.62*, *Júpiter -24.79*, *Espacio 0.0*), conmutador de Rigidbody por entidad, tipo de colisionador (Box / Sphere), masa, rebote (restitución), fricción y botones de prueba de impulso físico (*"¡Lanzar hacia arriba!"*).
    - **`[Lua]` Scripts**: Script asignado a la entidad, estado de compilación y recarga en caliente (Hot-Reload).
- **Compatibilidad con Módulo de IA Gemini**:
  - El sistema de generación por IA ahora comprende comandos físicos: prompts como *"una torre de bloques con física que caen y colisionan"* generan código Lua estructurado con `enablePhysics(1.0)` y cuerpos dinámicos instantáneamente.
  - Añadido botón rápido *"Torre Física"* en la barra de IA del Editor.

---

## [2026-11-13] [FASE 7] Iluminación Avanzada y Sombras Dinámicas (Shadow Mapping 2K + PCF) & Selección 3D
- **Sistema de Sombras Dinámicas en Tiempo Real (Shadow Mapping)**:
  - Creada clase `ShadowMap` (`ShadowMap.h` y `ShadowMap.cpp`): Framebuffer dedicado con textura de profundidad 2K (`GL_DEPTH_COMPONENT24`, 2048x2048) y envoltura `GL_CLAMP_TO_BORDER` con color blanco.
  - Pase de sombras en dos etapas (*Two-Pass Rendering*):
    - **Pase 1 (Depth Pass)**: Shaders `shadow_depth.vert` y `shadow_depth.frag` que proyectan la geometría de todas las entidades de la escena en el espacio ortográfico de la luz solar (`LightSpaceMatrix = lightProjection * lightView`).
    - **Pase 2 (Scene Pass)**: `model.vert` y `model.frag` calculan la atenuación de sombra con división de perspectiva y mapeo de profundidad.
  - **Suavizado de Penumbra PCF 3x3 (*Percentage-Closer Filtering*)**: Sombras suaves y realistas sin bordes pixelados ni bandas duras.
  - **Slope-Scaled Bias Dinámico**: Prevención matemática de artefactos visuales (*Shadow Acne* y *Peter Panning*) según el ángulo de incidencia de la luz sobre las normales de la superficie (`bias = max(0.005 * (1.0 - cosTheta), u_ShadowBias)`).
- **Plano de Suelo Receptor de Sombras (*Shadow Catcher Floor*)**:
  - Incorporada función `Mesh::createPlane(float size)` para renderizar un plano de suelo elegante a nivel `Y = -0.002f` que captura las sombras arrojadas por todos los objetos creados por el usuario o por la IA, manteniendo las líneas de la cuadrícula superpuestas limpiamente.
- **Selección Directa 3D por Raycasting (*Mouse Picking*)**:
  - Al hacer clic izquierdo (`LMB`) sobre cualquier objeto dentro del Viewport 3D, el motor dispara un rayo desde la posición de la cámara a través de las coordenadas normalizadas de pantalla (NDC) e intersecta los volúmenes delimitadores de las entidades.
  - La entidad más cercana atravesada por el rayo se selecciona al instante, activando su Gizmo 3D y abriendo sus propiedades en el Inspector sin necesidad de buscarla manualmente en la jerarquía.
- **Pausa de Animaciones / Modo Edición en Scripts**:
  - Botón interactivo en la barra del Viewport `[▶ Reanudar / ⏸ Pausar]` y atajo de teclado con la **Barra Espaciadora**: permite congelar la ejecución de `onUpdate(dt)` en Lua para manipular objetos con los Gizmos sin que el script sobreescriba su posición.
  - Botón *"Resetear Transformación"* en el Inspector para devolver cualquier objeto a sus coordenadas originales.
- **Panel de Control de Iluminación y Sombras en el Editor**:
  - Pestaña acoplable **"Iluminacion y Sombras"** con controles interactivos para:
    - Conmutador en vivo de sombras (activar/desactivar).
    - Sliders de orientación solar: Azimut (-180° a 180°) y Elevación (5° a 89°).
    - Paletas de color y multiplicadores de intensidad para la luz solar y la luz ambiental.
    - Preajustes rápidos con un clic: *Mediodía Cenital*, *Atardecer Dorado*, *Estudio / Diagonal*.
    - Ajuste fino del *Shadow Bias*.

---

## [2026-11-13] [FASE 6] Gizmos 3D de Manipulación Directa (ImGuizmo W/E/R) y Viewport FBO
- **Renderizado Offscreen con Framebuffer Object (FBO)**:
  - Creada clase `Framebuffer` (`Framebuffer.h` y `Framebuffer.cpp`) con textura de color `GL_RGB` y renderbuffer de profundidad/stencil `GL_DEPTH24_STENCIL8`.
  - Soporte de reescalado dinámico según el tamaño de la ventana o pestaña `"Viewport 3D"`.
  - La escena 3D y la rejilla se renderizan exclusivamente dentro del FBO y se proyectan en ImGui vía `ImGui::Image` con inversión vertical de coordenadas UV.
  - Al presionar `F1` para ocultar la UI, el motor conmuta automáticamente al framebuffer por defecto para una vista cinematográfica inmersiva en resolución completa.
- **Gizmos 3D de Manipulación Directa con ImGuizmo**:
  - Integrada la biblioteca `imguizmo` mediante CMake y vcpkg (`find_package(imguizmo CONFIG REQUIRED)`).
  - Al seleccionar cualquier objeto en la "Jerarquía de Escena" o en el "Inspector", se dibuja su gizmo 3D interactivo sobre la entidad.
  - **Atajos de teclado W / E / R** (estándar Unreal Engine / Blender):
    - `W`: Traslación (flechas en ejes X, Y, Z y planos de traslación).
    - `E`: Rotación (aros orbitales de rotación en cada eje).
    - `R`: Escala (cubos de escala por eje y escala uniforme).
  - **Barra de herramientas flotante en el Viewport**:
    - Selectores de operación (Mover, Rotar, Escalar).
    - Selector de espacio de coordenadas: **World** (global) / **Local** (orientación del objeto).
    - Conmutador de alineación a la cuadrícula (**Snap**) con pasos de 0.5 unidades, 15° y 0.25 de escala.
  - Descomposición de matriz directa (`ImGuizmo::DecomposeMatrixToComponents`) para sincronizar en tiempo real la posición, rotación Euler y escala de la `Entity`.
- **Desacoplamiento Ergonómico de Entradas y Estabilidad de Cámara**:
  - Detección precisa de foco y posición sobre el Viewport (`isViewportHovered`, `isViewportFocused`).
  - Cuando se manipula un gizmo 3D (`isUsingGizmo()`), la interacción de cámara se desactiva para evitar conflictos al arrastrar ejes.
  - Filtro de protección contra deltas anómalos o saltos de cursor (`std::abs(offset) > 120.0f`) al entrar o salir del modo de captura de cursor (`GLFW_CURSOR_DISABLED`).
  - Atajo `F` en el Viewport 3D re-enfoca la cámara directamente sobre la entidad seleccionada.
- **Sincronización Total con el Asistente de IA**:
  - Todas las creaciones generadas con IA (autos, castillos, dinosaurios, creepers) son inmediatamente seleccionables en la jerarquía y manipulables en 3D con los nuevos Gizmos.

---

## [2026-11-13] [FASE 5.1] Editor Moderno: Docking Modular, Tipografía Segoe UI y Consola de IA Embebida
- **Sistema de Docking Modular Completo (ImGui DockSpace)**:
  - Activado `ImGuiConfigFlags_DockingEnable` con nodo central transparente (`ImGuiDockNodeFlags_PassthruCentralNode`).
  - Distribución predeterminada inicial estilo Unreal Engine 5:
    - Izquierda: Jerarquía de Escena (Outliner).
    - Derecha: Inspector de Entidad.
    - Abajo: Consola del Motor y Generador de IA tabulados juntos.
    - Centro: Viewport 3D en tiempo real con la escena OpenGL sin obstrucciones.
  - Cada ventana se puede arrastrar, acoplar, dividir, combinar en pestañas o flotar libremente.
- **Tipografía Moderna en Alta Definición (Segoe UI & Consolas)**:
  - Carga nativa de la tipografía oficial de Windows 11 `C:\Windows\Fonts\segoeui.ttf` a 17px con suavizado de bordes (*subpixel antialiasing*) y soporte completo para acentos y caracteres en español (`á, é, í, ó, ú, ñ, ¿, ¡`).
  - Carga de `C:\Windows\Fonts\consola.ttf` a 15px para la terminal de logs y telemetría.
  - Eliminados los caracteres rotos `??` de la interfaz.
- **Consola de IA Embebida Nativamente en el Motor**:
  - Nuevo panel acoplable: **"Generador de IA (Gemini)"** con campo de texto directo y botón *"Generar (Enter)"*.
  - Generación asíncrona mediante subproceso multihilo (`std::thread` y `ai_cli.py`): el motor mantiene 60+ FPS sin congelarse mientras Gemini sintetiza la escena en segundo plano.
  - Botones de presets rápidos con un solo clic: Auto Deportivo, Castillo, Creeper, Barco Pirata, T-Rex, Dominó.

---

## [2026-11-13] [FASE 5] Feedback Loop, Sandbox de Lua, Shaders Vivos y Editor UI (Dear ImGui)
- **Feedback Loop Bidireccional y Auto-Corrección Autónoma (Self-Healing)**:
  - Servidor TCP en C++ ahora envía telemetría de ejecución de vuelta al cliente Python (`RESULT:OK:Entities=N` o `RESULT:ERROR:<detalles>`).
  - Módulo Python (`engine_client.py` y `ai_console.py`) intercepta fallos de sintaxis o runtime y activa `AIGenerator.auto_correct_lua()`.
  - Gemini auto-repara el código erróneo y lo reenvía automáticamente al motor sin intervención del usuario.
- **Sandbox Estricto y Watchdog de Instrucciones en Lua**:
  - Restricción de librerías del sistema en `ScriptEngine.cpp`: aisladas las llamadas a `os`, `io`, `package` y `debug`.
  - Watchdog de CPU (`lua_sethook` con límite de 2.5 millones de instrucciones) para abortar bucles infinitos (`while true do end`) sin congelar el hilo de renderizado.
  - Blindaje del callback `onUpdate(dt)` contra excepciones runtime repetitivas.
- **Coloración Real de Entidades y Shaders**:
  - `model.frag`: Incorporado uniform `u_EntityColor` con modulación de luz solar y tintes RGB reales.
  - `Entity.cpp`: Activada coloración sólida directa para cubos Voxel con color personalizado, eliminando la apariencia gris monocromática.
- **Interfaz Gráfica de Editor (Dear ImGui) estilo Unreal / Blender**:
  - Creada clase `EditorUI` con paleta oscura profesional.
  - **Barra Superior**: Telemetría en tiempo real (FPS, ms por cuadro, total de entidades) y botón "Limpiar Escena".
  - **Outliner / Jerarquía**: Lista interactiva de todas las entidades en escena con selección y eliminación.
  - **Inspector**: Modificación en vivo de Transformación (`DragFloat3` para Posición, Rotación y Escala) y Color (`ColorEdit3`), además de botón para enfocar cámara (`F`).
  - **Consola Integrada**: Registro de logs de `Engine.log` y errores en tiempo real coloreados por severidad.
  - **Atajo F1**: Alternar visibilidad de toda la interfaz del editor para alternar entre modo edición y vista pura.

---

## [2026-11-13] [FASE 4.2] Corrección y Optimización de la Integración con Google Gemini API
- **Resolución de Modelos en Gemini API**:
  - Se identificó que las llamadas a modelos con picos de demanda o no disponibles arrojaban HTTP 503/404 sin detalle en consola, provocando caída silenciosa al generador offline.
  - Se configuró la cascada de modelos activos de alta velocidad: `gemini-3.1-flash-lite` (tiempo de respuesta ~1.8s) y `gemini-3.5-flash-lite` (~1.2s), seguidos por `gemini-flash-lite-latest` y `gemini-3.8-flash`.
  - Se elevó el timeout de solicitud a 35 segundos para permitir la síntesis completa de estructuras Voxel 3D complejas.
  - Se incorporó telemetría clara en la consola indicando el modelo exacto que procesó la solicitud y diagnóstico HTTP ante fallos.
- **Validación en Vivo**:
  - Generación en tiempo real verificada con prompts arbitrarios ("crea un creeper de minecraft", "avion caza con alas y turbinas", "coliseo romano con columnas y gradas").
  - Inyección instantánea al viewport del motor en ejecución vía socket TCP (`127.0.0.1:5555`).

---

## [2026-11-13] [FASE 4.1] Expansión del Generador de IA (Voxel Art Procedural y Soporte de LLM)
- **Generador Semántico Ampliado**: Enriquecido `src/ai_module/ai_generator.py` para construir proceduralmente:
  - **Creeper de Minecraft**: Cabeza, ojos, ceño/boca, torso y 4 patas con animación de caminata.
  - **Pastel de Cumpleaños**: Plato, capas de chocolate, crema de fresa, cobertura y 3 velas con llamas parpadeantes.
  - **Coliseo Romano**: Suelo de arena, gradas interiores, arcos de doble nivel y estandartes.
- **Soporte de LLM en la Nube y Local**:
  - Soporte para **Google Gemini API** y **OpenAI API** vía `src/ai_module/.env`.
  - Soporte automático para modelos locales (LM Studio / Ollama en `http://127.0.0.1:1234/v1`).
- **Scripts Demo de Voxel Art**: Creados `src/scripts/creeper.lua`, `src/scripts/cake.lua`, `src/scripts/coliseum.lua`.
- **Script de Inyección Rápida**: `src/ai_module/send_lua.py` para inyectar scripts al motor en caliente desde el chat o terminal.

---

## [2026-11-13] [FASE 4] Módulo de IA Integrado (Servidor IPC en C++ y Consola Python)
- **Servidor Socket Local (IPC)**: Clase `NetworkServer` en C++ mediante Winsock2 escuchando en `127.0.0.1:5555`.
- **Cliente Python y Generador de IA**: `engine_client.py`, `ai_generator.py`, `ai_console.py`, `run_ai.bat`.
- **Snippets Clave**: `network_ipc_server.cpp` y `python_ai_client.py`.

---

## [2026-11-13] [FASE 3] Integración de Scripting en Lua 5.4 con Hot-Reload y Sistema de Entidades
- **Arquitectura de Entidades y Escena**: Clases `Entity` y `Scene`.
- **Motor de Scripting Lua + sol2**: Clase `ScriptEngine`.
- **Sistema de Hot-Reload en Tiempo Real**: Recarga en vivo de scripts `.lua`.

---

## [2026-11-13] [FASE 2.1] Corrección de Orientación Inicial de Cámara, Captura de Ratón y Texturizado
- **Correcciones de Cámara**: Inicialización y eliminación de saltos con `GLFW_CURSOR_DISABLED`.

---

## [2026-11-13] [FASE 2] Carga de Modelos 3D, Sistema de Cámaras e Iluminación
- **Carga con Assimp**: `.obj`, `.fbx`, normales y UVs.
- **Cámara Híbrida**: Unreal Flycam / Blender Orbit.

---

## [2026-11-13] [FASE 1] El Triángulo en Pantalla (OpenGL Core Profile 3.3)
- **Base Modular**: `Window`, `Shader`, `Mesh`, `Renderer`, `main.cpp`.

---

## [2026-11-13] [FASE 0] Inicialización del Proyecto y Auditoría del Entorno
- **Estructura base y auditoría**: Detección de herramientas en Windows 11.
