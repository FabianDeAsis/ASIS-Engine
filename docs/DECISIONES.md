# ⚖️ Registro de Decisiones de Arquitectura (ADR)

Este documento registra las decisiones clave de ingeniería, sus justificaciones y las alternativas descartadas.

---

### DEC-001: Núcleo en C++ (C++20)
- **Decisión**: Usar C++ moderno (estándar C++20) para el motor gráfico y la capa de bajo nivel.
- **Justificación**: Máximo control sobre memoria, compatibilidad total con librerías nativas de gráficos (OpenGL, Vulkan), cero sobrecarga de garbage collection y rendimiento predecible a 60+ FPS.
- **Alternativas consideradas**: Rust, C#.

---

### DEC-002: API Gráfica Inicial: OpenGL Moderno (Core Profile 3.3 / 4.5)
- **Decisión**: Implementar primero con OpenGL Moderno antes de migrar a Vulkan.
- **Justificación**: OpenGL permite una iteración rápida, abstracción directa de shaders (GLSL) y menor código boilerplate inicial.
- **Alternativas consideradas**: Vulkan directo, DirectX 12.

---

### DEC-003: Scripting Interno: Lua 5.4 + sol2
- **Decisión**: Usar Lua como lenguaje de scripting en tiempo de ejecución, enlazado con la librería `sol2` en C++.
- **Justificación**: Ultraligero, rápido, estándar de la industria y permite hot-reload en tiempo real sin reiniciar el motor.
- **Alternativas consideradas**: Python embebido directo.

---

### DEC-004: Módulo de IA: Proceso Python Externo desacoplado con IPC
- **Decisión**: El módulo de IA corre como un proceso Python independiente comunicado por socket local.
- **Justificación**: Aísla el motor de bloqueos de red o errores del LLM; el motor permanece a 60 FPS mientras la IA razona y genera código Lua en segundo plano.
- **Alternativas consideradas**: C++ llamando directamente a APIs HTTP.

---

### DEC-005: Gestión de Dependencias C++: CMake + vcpkg
- **Decisión**: Usar CMake como sistema de construcción principal y `vcpkg` como gestor de paquetes.
- **Justificación**: Automatiza la descarga, compilación y enlazado de librerías (.lib y .dll) en Windows 11 sin intervención manual.

---

### DEC-006: Interfaz, Gizmos y Controles de Cámara (Estilo Blender / Unreal Engine)
- **Decisión**:
  - Adoptar **Dear ImGui** (con soporte de *docking*) y **ImGuizmo** para la interfaz gráfica del editor, paneles flotantes, jerarquía de entidades, inspector de propiedades y gizmos de manipulación 3D (Traslación, Rotación, Escala con atajos W/E/R o G/R/S).
  - Implementar un sistema de cámara híbrido:
    - **Estilo Unreal Engine**: Clic derecho (RMB) presionado + teclas WASD para volar libremente por la escena 3D como en un FPS.
    - **Estilo Blender**: Clic central (MMB) o Alt + LMB para orbitar alrededor del objeto enfocado; Shift + MMB para desplazamiento lateral (Pan); Rueda del ratón para Zoom.
  - Añadir una rejilla de orientación de suelo (*Ground Grid*) con ejes coloreados en el plano XZ para una referencia espacial tridimensional clara.
- **Justificación**: Proporciona el flujo de trabajo estándar e intuitivo que cualquier creador habituado a Unreal o Blender espera encontrar en un motor profesional.

---

### DEC-007: Sandbox de Seguridad y Watchdog de Instrucciones en Lua
- **Decisión**: Restringir las librerías cargadas en el estado de Lua a exclusivamente computación segura (`base`, `math`, `string`, `table`), eliminando `io`, `os`, `package` y `debug`. Además, activar un hook de conteo (`lua_sethook` con límite de 2.5 millones de instrucciones) durante la evaluación de scripts.
- **Justificación**: Previene que código generado por modelos de IA ejecute comandos destructivos en el sistema operativo o congele el hilo de renderizado de OpenGL mediante bucles infinitos (`while true do end`).

---

### DEC-008: Feedback Loop Bidireccional y Auto-Corrección Autónoma (Self-Healing)
- **Decisión**: El servidor TCP de C++ no solo acusa recibo del script, sino que tras la ejecución en el render loop devuelve un paquete de telemetría estructurado (`RESULT:OK` o `RESULT:ERROR:<detalles>`). En Python, la consola detecta los fallos de sintaxis o runtime y consulta a Gemini para generar un parche correctivo y reinyectarlo de forma transparente.
- **Justificación**: Elimina la necesidad de depuración manual por parte del usuario cuando el LLM comete un error de tipado o invoca una API de manera incorrecta.

---

### DEC-009: Tema Oscuro y Paneles Acoplables de Editor UI con Dear ImGui
- **Decisión**: Integrar paneles de Outliner (jerarquía de entidades), Inspector (edición de Transform y Color mediante `DragFloat3` y `ColorEdit3`) y Consola de telemetría interactiva con capacidad de alternar visibilidad mediante la tecla `F1`.
- **Justificación**: Permite visualizar y modificar interactivamente todas las propiedades de las entidades creadas por la IA en tiempo real sin salir del motor.

---

### DEC-010: Docking Modular y Generación de IA Asíncrona Multihilo
- **Decisión**:
  - Habilitar `ImGuiConfigFlags_DockingEnable` con `DockBuilder` para crear un diseño acoplable predeterminado idéntico a los motores AAA (Unreal Engine / Unity).
  - Embeber la consola de prompts de IA directamente dentro de la interfaz del motor, ejecutando la llamada a Gemini mediante un hilo secundario en C++ (`std::thread`).
- **Justificación**:
  - Ofrece máxima flexibilidad espacial al creador para personalizar la posición de sus paneles.
  - Elimina la necesidad de cambiar de ventana a una terminal externa para enviar prompts.
  - El hilo secundario garantiza que el motor continúe renderizando a 60+ FPS sin congelar la ventana mientras el modelo de lenguaje razona y genera el código en la nube.

---

### DEC-011: Renderizado Offscreen con Framebuffer (FBO) y Manipulación Directa 3D con ImGuizmo
- **Decisión**:
  - Migrar el renderizado de la escena 3D y la cuadrícula desde el framebuffer por defecto de GLFW hacia un Framebuffer Object (FBO) desacoplado, exponiendo su textura a Dear ImGui dentro de una ventana acoplable `"Viewport 3D"`.
  - Integrar la biblioteca `ImGuizmo` para renderizar gizmos 3D interactivos en tiempo real directamente sobre el viewport.
  - Asignar los atajos estándar de la industria: `W` para Traslación, `E` para Rotación, `R` para Escala, y conmutadores para espacio de coordenadas (`Local` / `World`) y alineación modular (`Snap`).
  - Desacoplar la interacción de cámara para que solo actúe cuando el cursor se encuentra sobre el Viewport 3D y no se esté arrastrando ningún manipulador de gizmo (`!editorUI.isUsingGizmo()`).
- **Justificación**:
  - Unificar el espacio de renderizado con la interfaz modular de usuario, permitiendo que la escena 3D se redimensione dinámicamente según el tamaño asignado al Viewport.
  - La manipulación directa mediante Gizmos 3D transforma a ASIS Engine de un visor de scripts a un entorno de autoría y edición 3D interactivo completo.
  - Al separar el foco de entrada entre cámara y gizmos, se eliminan los conflictos de navegación y la cámara no se desplaza accidentalmente mientras el usuario transforma objetos.

---

### DEC-012: Pipeline de Sombras Dinámicas en Tiempo Real (Shadow Mapping 2K + PCF)
- **Decisión**:
  - Implementar un pase de renderizado en dos etapas con un Framebuffer Object exclusivo de sombras (`ShadowMap`) de resolución 2048x2048 utilizando `GL_DEPTH_COMPONENT24` y `GL_CLAMP_TO_BORDER` con color blanco.
  - Proyección ortográfica solar (`glm::ortho(-22, 22, -22, 22, 1, 65)`) que abarca la escena completa, con suavizado de penumbra mediante kernel PCF 3x3 y bias dependiente de la pendiente normal (`slope-scaled bias`).
  - Renderizar un plano receptor de sombras a nivel del suelo para que las siluetas tridimensionales se proyecten de forma natural bajo los objetos sin ocultar las líneas de la cuadrícula.
- **Justificación**:
  - Las sombras otorgan dimensionalidad y anclaje espacial a los objetos en el entorno 3D, eliminando la sensación de ingravidez o desconexión visual con el suelo.
  - La resolución 2K combinada con el filtrado PCF produce bordes suaves cinematográficos manteniendo 100+ FPS en tarjetas modernas.

---

### DEC-013: Selección 3D en Viewport mediante Raycasting y Modo Pausa de Scripts
- **Decisión**:
  - Implementar desproyección matemática de coordenadas de pantalla del ratón hacia rayos en el espacio de mundo para detectar intersecciones con los volúmenes de las entidades al hacer clic izquierdo sobre el Viewport 3D.
  - Añadir un interruptor de pausa en la barra de herramientas del editor (y tecla `Espacio`) que congela selectivamente la ejecución de `onUpdate(dt)` de Lua.
- **Justificación**:
  - Permite seleccionar intuitivamente cualquier objeto directamente en el espacio tridimensional sin obligar al usuario a buscar su nombre en una lista de jerarquía.
  - Permite pausar las animaciones programadas para manipular las piezas libremente con los Gizmos 3D sin que el script vuelva a sobreescribir sus posiciones en el siguiente fotograma.

---

### DEC-014: Integración del Motor de Física 3D Bullet Physics
- **Decisión**:
  - Incorporar la biblioteca de física de grado industrial Bullet Physics 3 (`bullet3` v3.25) mediante vcpkg y CMake.
  - Diseñar el subsistema modular `PhysicsWorld` (`PhysicsWorld.h` y `PhysicsWorld.cpp`) encapsulando `btDiscreteDynamicsWorld`, configuración de colisiones, solver de impulsos secuenciales y detección broadphase DBVT.
  - Implementar suelo infinito estático en $Y = 0$, colisionadores orientados (`btBoxShape`), sub-stepping temporal a 60 Hz y sincronización bidireccional continua:
    - Estado de Bullet a matriz de transformación de `Entity` durante la simulación activa.
    - Transformación de Gizmo/Inspector a Bullet (`btTransform`, `clearForces`, `activate(true)`) cuando el usuario edita la posición en el editor.
  - Exponer funciones a Lua 5.4 (`enablePhysics(mass)`, `applyImpulse(fx, fy, fz)`, `Engine.setGravity(gx, gy, gz)`).
- **Justificación**:
  - Bullet es el motor de física de código abierto más probado y optimizado de la industria (utilizado en Blender, juegos AAA y simuladores robóticos).
  - Proporciona colisiones convexas precisas, simulación de cuerpos rígidos con masa e inercia realistas, y desacoplamiento limpio de la lógica de renderizado gráfico.

---

### DEC-015: Ergonomía de Editor Estilo Blender 4: Riel Vertical de Íconos
- **Decisión**:
  - Reemplazar las pestañas horizontales superiores en el panel derecho por una barra de navegación vertical en el extremo izquierdo del panel (`ImGui::BeginChild("IconRail", ImVec2(48, 0))`) con botones cuadrados y etiquetas de categoría (`[Obj]`, `[Luz]`, `[Fis]`, `[Lua]`).
  - Unificar los módulos en un solo panel acoplable `"Propiedades"` con un área de contenido dinámico a la derecha del riel.
- **Justificación**:
  - Sigue la convención estándar de la industria establecida por Blender 4.
  - Ahorra espacio vertical para los controles del inspector, organiza los módulos temáticamente de manera limpia y permite escalar indefinidamente a futuras fases (materiales PBR, post-procesado, partículas, audio) sin saturar la parte superior de la ventana.

---

### DEC-016: Ilustraciones Vectoriales Procedurales en ImDrawList para el Riel de Blender
- **Decisión**:
  - Implementar el renderizado de iconos mediante comandos vectoriales de bajo nivel en `ImDrawList` (`AddQuadFilled`, `AddCircleFilled`, `AddLine`) en lugar de depender de tipografías TTF de iconos o imágenes rasterizadas externas.
  - Diseñar iconografía que emula la identidad visual oficial de Blender 4:
    - **Objeto**: Cubo isométrico 3D tricolor sombreado en naranja Blender.
    - **Luz**: Sol radiante con núcleo y 8 rayos a 45° en amarillo dorado.
    - **Física**: Esfera azul con trayectoria orbital elíptica inclinada y línea de contacto.
    - **Scripts**: Símbolo `< / >` en verde esmeralda.
    - **I/O**: Contenedor 3D con flechas de importación/exportación en cian y púrpura.
- **Justificación**:
  - El dibujo vectorial en `ImDrawList` garantiza nitidez absoluta en cualquier resolución (1080p, 1440p, 4K), no tiene dependencias de carga de archivos externos ni problemas de codificación UTF-8, y ofrece un rendimiento instantáneo (0 allocations).

---

### DEC-017: Arquitectura de Importación y Exportación Multiformato 3D y Drag & Drop
- **Decisión**:
  - Encapsular la lógica de E/S 3D en la clase estática `SceneIO` (`SceneIO.h`, `SceneIO.cpp`).
  - Utilizar Assimp para la importación universal (`.obj`, `.gltf`, `.glb`, `.fbx`, `.stl`, `.ply`, `.dae`, `.3ds`) con cálculo de AABB, escalado proporcional inteligente y alineación al suelo $Y = 0$.
  - Implementar exportadores dedicados en formato estándar Wavefront OBJ + MTL (con preservación de colores RGB y matrices de transformación de mundo) y glTF 2.0 (JSON con buffer Base64 autónomo).
  - Integrar diálogos nativos del Explorador de Windows (`GetOpenFileNameA` / `GetSaveFileNameA`) y arrastrar y soltar (Drag & Drop) mediante `glfwSetDropCallback`.
- **Justificación**:
  - Permite a los usuarios traer sus propios modelos existentes al motor sin conversiones tediosas y exportar las creaciones generadas con IA hacia suites de producción como Blender o motores como Unreal y Unity.

---

### DEC-018: Pipeline de Materiales PBR Físicamente Realistas (Cook-Torrance GGX) y Mapeo de Texturas
- **Decisión**:
  - Adoptar el modelo de iluminación PBR (Physically Based Rendering) **Cook-Torrance Microfacet** en GLSL con distribución normal **Trowbridge-Reitz GGX**, función geométrica **Smith Schlick-GGX** y ecuación de **Fresnel-Schlick**.
  - Soportar parámetros físicos estándar de la industria: Albedo, Roughness, Metallic, Specular, Normal Strength, Emission (con multiplicador de intensidad) y Ambient Occlusion (AO).
  - Gestionar 7 ranuras de textura independientes (Albedo, Normal/Bump, Roughness, Metallic, Specular, Emission, AO) vinculadas a unidades OpenGL 0..6.
  - Para mapas de relieve/normales, calcular la matriz TBN en el fragment shader al vuelo utilizando derivadas de espacio de pantalla (`dFdx`/`dFdy` de la posición de fragmento y coordenadas UV).
  - Mapear automáticamente tanto materiales clásicos OBJ (`aiTextureType_DIFFUSE`) como materiales PBR modernos de glTF (`aiTextureType_BASE_COLOR`, `NORMALS`, `METALNESS`, `ROUGHNESS`), normalizando rutas de archivos en Windows (`\` a `/`).
- **Justificación**:
  - Cumple los estándares visuales de Blender (Principled BSDF) y Unreal Engine (PBR Material System), permitiendo transiciones realistas entre conductores (metales) y aislantes (dieléctricos) con conservación de energía.
  - El cálculo TBN con derivadas evita modificar los formatos de vértices existentes (`Mesh.h`), garantizando compatibilidad con modelos antiguos y mallas generadas proceduralmente.

---

### DEC-020: Formato de Escena Nativo .asis y Arquitectura de Deshacer / Rehacer (Undo/Redo)
- **Decisión**:
  - Crear un formato nativo de escena con extensión `.asis` basado en JSON legible por humanos, serializado a través de `SceneIO::saveSceneASIS` y `SceneIO::loadSceneASIS`.
  - El formato `.asis` almacena el estado completo del motor: versión, cámara 3D, iluminación global y sombras, perfiles de color y mapeo tonal, todas las entidades 3D (con sus primitivas, transformaciones y materiales PBR) y la colección completa de luces puntuales.
  - Implementar un sistema de historial de estados en memoria (`SceneSnapshot`) en `EditorUI` con pilas independientes `m_undoStack` y `m_redoStack` y profundidad configurable (16 a 256 pasos).
  - Capturar instantáneas antes de cualquier operación destructiva o interactiva (Gizmo drag start, borrado de entidades, añadir primitivas, limpiar escena, autoguardado).
- **Justificación**:
  - Proporciona persistencia nativa fiable de escenas creadas tanto manualmente como mediante prompts de IA.
  - Garantiza la seguridad y comodidad del usuario al manipular objetos 3D y experimentar con la física o iluminación sin miedo a perder su trabajo.

---

### DEC-021: Gestión de Color Cine ACES Filmic/AgX y Generación de Primitivas Procedurales
- **Decisión**:
  - Eliminar el doble paso gamma que provocaba colores opacos: linearizar explícitamente lecturas de texturas difusas sRGB (`pow(tex, vec3(2.2))`) y aplicar mapeo tonal profesional en el fragment shader (`model.frag`) con perfiles conmutables: ACES Filmic (Narkowicz), AgX (Blender 4.0), Reinhard y Lineal, junto con controles de exposición, gamma, saturación y contraste.
  - Generar mallas 3D procedurales nativas calculadas analíticamente en `Mesh.cpp`: Esfera UV, Cilindro, Cono, Toroide (Donut), Plano y Cubo.
  - Integrar las luces puntuales directamente en la jerarquía de escena (`Jerarquia de Escena`), haciendo que sean seleccionables y manipulables mediante Gizmos 3D y clics directos sobre sus marcadores visuales en el Viewport.
  - Añadir modal de configuración de preferencias (`Ctrl+,`) para VSync, límite de FPS, MSAA, temas visuales y atajos.
- **Justificación**:
  - Resuelve el defecto estético de colores deslavados, otorgando riqueza cromática y contraste similar a motores de alta gama.
  - El menú de primitivas (`Shift+A`) y la selección de luces en el Viewport dotan a ASIS Engine de la ergonomía esperada en software moderno de modelado y creación 3D como Blender.

---

### DEC-022: Iluminación Basada en Imágenes (IBL) & Skybox HDRI 360°, Antialiasing Real (MSAA + FXAA), Pipeline Post-Procesado Bloom HDR y Captura F12
- **Decisión**:
  - Implementar compatibilidad total con Windows "Abrir con..." fijando el directorio de trabajo del proceso en el arranque (`GetModuleFileNameW` + `std::filesystem::current_path(p.parent_path())`) y parseando `argv[1]` para cargar escenas `.asis`, modelos 3D (`.obj`, `.gltf`, `.fbx`) o mapas `.hdr` automáticamente.
  - Reemplazar el Framebuffer básico por una arquitectura con Multisample FBO (`m_msaaFBO` con `glRenderbufferStorageMultisample` y resolución `glBlitFramebuffer`) hacia una textura 2D en formato de alto rango dinámico `GL_RGBA16F`.
  - Añadir filtro de post-procesamiento FXAA 3.11 para suavizado sub-pixel complementario o de bajo consumo.
  - Implementar iluminación basada en imágenes (IBL) y Skybox HDRI:
    - Carga de imágenes equirrectangulares `.hdr` en coma flotante de 32 bits por canal con `stbi_loadf`.
    - Cielo procedural atmosférico integrado con sol y gradiente horizonte como respaldo automático.
    - Renderizado de fondo de cielo a profundidad infinita (`glDepthFunc(GL_LEQUAL)`) con controles de rotación 360°, exposición y desenfoque de fondo (LOD).
    - Cálculo de reflejos especulares de entorno PBR en metales y materiales lisos en `model.frag` muestreando la textura equirrectangular rotada en $Y$ con nivel de mipmap proporcional a `roughness * 7.0`.
    - Irradiancia difusa ambiental por IBL con Fresnel-Schlick rugoso.
  - Implementar pipeline de Bloom HDR con extracción por umbral, desenfoque gaussiano separable de 5 taps a media resolución (ping-pong FBOs) y adición ponderada.
  - Incorporar captura limpia de viewport en PNG sin pérdidas (`stbi_write_png`) accesible con la tecla `F12` y mediante el menú superior `Render`.
- **Justificación**:
  - Soluciona de raíz el fallo de Windows "Abrir con..." permitiendo asociar la extensión `.asis` directamente con `ASIS_Engine.exe`.
  - Proporciona verdadero suavizado de bordes MSAA (1x a 8x) en el Viewport 3D de Dear ImGui, resolviendo la ineficacia del MSAA en fases previas.
  - La iluminación IBL confiere a cualquier modelo 3D un realismo visual fotográfico inmediato con reflejos metálicos y ambientación coherente.

---

### DEC-023: Sistema de Partículas 3D (VFX), SSAO Bilateral, DoF/Bokeh Fotográfico, Animaciones Esqueléticas y Notificaciones de Render
- **Decisión**:
  - **Sistema de Partículas 3D (VFX Engine)**:
    - Diseñar una arquitectura de partículas instanciadas en GPU (`glDrawArraysInstanced`) con billboarding automático orientado a la cámara (`right` y `up` vectors).
    - Ofrecer preajustes integrados con parámetros calibrados: 🔥 Fuego, ✨ Chispas, 💨 Humo, 🌧️ Lluvia, 🔮 Magia y Emisor Personalizado.
    - Soportar modos de mezcla conmutable: blend aditivo (`GL_ONE, GL_ONE`) para llamas, orbes mágicos y chispas de soldadura vs. blend alfa suave (`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`) para columnas de humo y niebla.
    - Integrar pestaña `[VFX]` en la barra de herramientas vertical de Blender con ilustración vectorial propia de estrella resplandeciente.
  - **Oclusión Ambiental en Espacio de Pantalla (SSAO)**:
    - Extraer la textura de profundidad `GL_DEPTH24_STENCIL8` tras la resolución de multisampling de `m_msaaFBO` hacia `m_fbo` con `glBlitFramebuffer(..., GL_DEPTH_BUFFER_BIT, GL_NEAREST)`.
    - Reconstruir posiciones en espacio de vista mediante la matriz inversa de proyección y normales de superficie mediante derivadas parciales en pantalla (`dFdx` y `dFdy`), evitando crear pases de geometría o G-buffers adicionales.
    - Muestrear un hemisferio tangente de 32 muestras orientadas con decaimiento cuadrático y ruido estocástico 4x4, seguido de desenfoque bilateral para eliminar ruido de alta frecuencia preservando aristas.
  - **Profundidad de Campo Fotográfica (DoF / Bokeh)**:
    - Implementar un paso de post-procesado que calcula el Círculo de Confusión (CoC) a partir de la distancia lineal del fragmento respecto al plano focal.
    - Aplicar un desenfoque Poisson Disc de 16 muestras con ponderación luminosa (`highlight weighting`) para reproducir discos circulares de bokeh fotográfico brillante.
    - Incorporar el botón interactivo de un solo clic `[🎯 Enfocar en Objeto Seleccionado]` que ajusta automáticamente la distancia focal hacia la entidad activa.
  - **Animaciones Esqueléticas y Rigging (Assimp)**:
    - Extender la estructura `Vertex` para registrar hasta 4 influencias de huesos con sus respectivos pesos (`boneIDs[4]`, `weights[4]`).
    - Implementar vertex skinning por hardware en `model.vert` con una matriz uniforme de 100 transformaciones de huesos `u_BoneTransforms[100]`.
    - Crear clases `SkeletalAnimation` y `Animator` para calcular la interpolación de fotogramas clave con slerp cuaterniónico (`glm::slerp`), control de transporte (reproducir, pausar, reset, bucle, velocidad) y barra de tiempo interactiva (scrubber).
  - **Gestión y Visibilidad de Renders (F12)**:
    - Asegurar la creación del directorio `renders/` con rutas absolutas de Windows.
    - Agregar menú directo `Render -> 📂 Abrir Carpeta de Capturas (Renders)...` que invoca Windows Explorer vía `ShellExecuteA`.
    - Mostrar notificación flotante interactiva en pantalla (toast de 6 segundos) con botones `[📂 Abrir Carpeta]` y `[🖼️ Ver Imagen]`.
    - Leer la tecla `GLFW_KEY_F12` directamente en el bucle principal de hardware para garantizar que la captura nunca sea bloqueada por el foco de texto de ImGui.
- **Justificación**:
  - Las partículas 3D transforman a ASIS Engine de un visualizador estático a un verdadero motor gráfico y de juegos capaz de representar efectos visuales dinámicos.
  - SSAO y DoF aumentan de forma dramática el fotorrealismo y la percepción de volumen espacial en cualquier escena.
  - El soporte de mallas esqueléticas animadas abre las puertas a personajes 3D animados traídos desde Blender o Mixamo.
  - El acceso inmediato en 1 clic a los renders guardados resuelve por completo la incertidumbre del usuario respecto a dónde quedaban guardadas sus capturas.

---

### DEC-024: Terreno Procedural 3D con Escultura en Tiempo Real, PBR Splatmapping, Colisiones Físicas Bullet y Niebla Atmosférica Volumétrica (Height Fog) (Fase 14)
- **Decisión**:
  - **Malla de Terreno Dinámica en Memoria y GPU**:
    - Usar una cuadrícula regular de $128 \times 128$ vértices (16,384 vértices, 32,258 triángulos) abarcando 120m x 120m en el plano XZ.
    - Mantener las alturas en un vector contiguo `std::vector<float>` en RAM, sincronizado con la GPU mediante un buffer VBO con bandera de uso `GL_DYNAMIC_DRAW` y llamadas de actualización `glBufferSubData`.
    - Calcular normales de superficie de forma analítica en CPU mediante diferencias centrales discretas:
      $\vec{N} = \text{normalize}\left(\frac{h(x-1, z) - h(x+1, z)}{2 \cdot dx}, 1.0, \frac{h(x, z-1) - h(x, z+1)}{2 \cdot dz}\right)$
  - **Herramientas de Escultura 3D (Sculpting Brushes) y Raycasting**:
    - Desarrollar un trazador de rayos (*Raymarcher*) analítico en CPU que muestrea el heightfield regular en pasos de 0.28m y refina el punto de impacto mediante búsqueda binaria de 10 iteraciones ($< 0.01$ ms de coste por frame).
    - Renderizar un anillo circular tridimensional que amolda su perímetro a las irregularidades del relieve mediante `ImDrawList::AddPolyline`.
    - Ofrecer 5 tipos de pinceles conmutables: Subir (`RAISE`), Bajar (`LOWER`), Suavizado Laplaciano (`SMOOTH`), Aplanar a cota (`FLATTEN`) y Ruido rocoso (`NOISE`), con 4 curvas de caída radial (Suave/Coseno, Esférico, Lineal, Plano).
    - Asignar atajos directos en el Viewport 3D: Clic Izquierdo para aplicar pincel activo, `Ctrl` + Clic Izquierdo para Bajar e invertir, `Shift` + Clic Izquierdo para Suavizar.
  - **Splatmapping Procedural PBR Multicapa en GPU (`terrain.frag`)**:
    - Mezclar 4 capas de material sin requerir texturas externas pesadas mediante el coseno de la pendiente ($\cos \theta = \vec{N} \cdot \vec{Y}$) y la elevación de mundo ($Y$):
      1. *Pasto verde templado*: Zonas bajas horizontales ($Y < 2.0$m).
      2. *Tierra rica y fértil*: Zonas intermedias onduladas ($2.0\text{m} < Y < 12.0\text{m}$).
      3. *Roca de acantilado granítica*: Paredes con inclinación pronunciada ($> 40^\circ$), con micro-estriado mineral procedimental.
      4. *Nieve alpina pura*: Cumbres altas ($Y > 14.0\text{m}$ a $22.0$m), salvo en acantilados verticales donde la nieve no puede posarse.
    - Conectar al pipeline PBR existente: Sol con sombras 4K PCF, hasta 8 luces puntuales e IBL difuso/especular del Skybox HDRI.
  - **Física 3D con Bullet Physics (`btHeightfieldTerrainShape`)**:
    - Construir un cuerpo rígido estático (masa 0.0) vinculado a la forma de colisión `btHeightfieldTerrainShape` de Bullet.
    - Sincronizar automáticamente la forma física tras cada trazo de escultura (`syncPhysics`), permitiendo que esferas, cubos, autos y monos colisionen y rueden de forma natural sobre las colinas y valles modelados.
  - **Niebla Atmosférica Volumétrica (Height Fog - Opción D)**:
    - Integrar la ecuación de atenuación óptica exponencial basada en la altitud de la cámara y del fragmento:
      $\tau = d \times \rho_0 \times \frac{e^{-b(C_y - Y_0)} - e^{-b(P_y - Y_0)}}{b(P_y - C_y)}$
    - Implementar dispersión de luz solar direccional (*Solar Inscattering*), produciendo un resplandor dorado cálido hacia el sol mientras los valles profundos permanecen cubiertos de bruma azulada.
    - Incorporar la niebla de forma unificada tanto en `terrain.frag` como en `model.frag`, de modo que cualquier objeto o modelo 3D en escena se integre armónicamente en la atmósfera.
  - **Integración en UI (Estilo Blender 4)**:
    - Agregar pestaña dedicada `[Terreno]` en el riel de iconos de propiedades con icono vectorial de cordillera montañosa.
    - Organizar controles en 5 módulos limpios: Estado general, Preajustes procedurales (Montañas, Colinas, Cañones, Plano), Pinceles de escultura, Física Bullet y Parámetros atmosféricos de niebla.
- **Justificación**:
  - Un terreno procedural esculpible transforma el motor de un mero visor de objetos flotantes en un entorno de juego inmersivo completo con paisajes expansivos.
  - La niebla volumétrica proporciona una enorme profundidad visual atmosférica y escala cinematográfica.
  - La sincronización bidireccional con Bullet Physics garantiza que la interacción física entre objetos y el terreno funcione de forma orgánica sin configuración adicional.

---

### DEC-025: Play-in-Editor (PIE) & Controlador de Personaje 3D (First Person & Third Person con Spring Arm) (Fase 15)
- **Decisión**:
  - **Controlador Físico de Personaje Dinámico (`CharacterController.h`, `CharacterController.cpp`)**:
    - Implementar el controlador sobre un cuerpo rígido dinámico de Bullet Physics (`btRigidBody`) con forma de cápsula (`btCapsuleShape`, radio 0.38m, altura 1.80m, masa 75kg).
    - Bloquear los tres ejes de rotación angular (`setAngularFactor(btVector3(0,0,0))`) para evitar vuelcos al caminar por rampas o colisionar con cajas y bordes.
    - Desactivar el reposo físico del motor Bullet (`DISABLE_DEACTIVATION`) para mantener el cuerpo permanentemente receptivo a los comandos de teclado.
    - Implementar detección de contacto con el suelo mediante raycasting analítico vertical descendente con filtro personalizado en `PhysicsWorld::rayTest`, ignorando el propio cuerpo de la cápsula para eliminar falsos positivos de auto-colisión.
    - Permitir marcha (WASD), sprint a alta velocidad (`Shift`), agachado/deslizamiento (`Ctrl` o `C`) y salto físico (`Espacio`).
  - **Sistema de Cámaras Dual (FPS & TPS con Brazo Elástico / Spring Arm)**:
    - Ofrecer dos modos de visualización conmutables en tiempo real con la tecla `V`:
      1. *Primera Persona (FPS)*: Cámara situada en la altura de los ojos (1.65m), ideal para explorar construcciones o juegos de disparos y aventura.
      2. *Tercera Persona (TPS)*: Cámara orbitante a 3.5m soportada por un brazo elástico (*Spring Arm*) con desplazamiento lateral sobre el hombro (*shoulder offset* de 0.35m).
    - Prevención de colisiones de cámara: trazado de rayos entre el pivote del hombro y la posición deseada de la cámara. Si impacta contra paredes o terreno, acorta la distancia dinámicamente con un colchón de 0.28m, impidiendo que la cámara atraviese la geometría del mundo.
    - Zoom dinámico de la cámara en tercera persona con la rueda del ratón (*mouse scroll*).
  - **Avatar 3D Procedural en Tercera Persona**:
    - Malla tridimensional geométrica modular (torso, cabeza, visor holográfico y mochila de soporte).
    - Materiales PBR integrados (aleación oscura de grafito con alta reflectividad metálica y casco blanco).
    - Visor holográfico emisivo cian de alta intensidad que activa el pipeline Bloom HDR y brilla en la niebla volumétrica.
    - Proyección de sombras dinámicas reales del avatar sobre el terreno y objetos en el pase de sombras 2K.
  - **Flujo Play-in-Editor (PIE) e Integración con UI**:
    - Transición limpia entre modos `EDITOR` y `PLAYING`: captura del cursor del ratón con `GLFW_CURSOR_DISABLED` al jugar y liberación con `GLFW_CURSOR_NORMAL` al salir.
    - Preservación del estado de la cámara del editor: al detener el juego con `Esc`, `Alt+P` o el botón del Viewport, la cámara regresa exactamente a su posición y ángulo previo de edición.
    - HUD in-game en tiempo real: punto de mira / crosshair central con 4 marcas, banner inferior translúcido con los atajos de teclado y píldora de telemetría de velocidad y suelo.
    - Pestaña dedicada `[Jugador / PIE]` en el riel de iconos estilo Blender 4 con ilustración vectorial de mando de videojuegos (gamepad).
- **Justificación**:
  - Un motor de juego requiere permitir probar inmediatamente las creaciones y mecánicas en primera y tercera persona sin necesidad de exportar ni compilar proyectos externos.
  - El sistema de cápsula física Bullet garantiza interacciones naturales y colisiones realistas tanto con objetos dinámicos como con el terreno esculpido en la Fase 14.
  - El brazo elástico con prevención de oclusiones ofrece una experiencia de cámara suave idéntica a la de motores de la industria como Unreal Engine 5 o Unity.

---

### DEC-026: Motor de Audio 3D Espacial y Sintetizador Procedural Autónomo con Miniaudio (Fase 16)
- **Decisión**:
  - Adoptar la librería `miniaudio` (instalada vía `vcpkg: miniaudio:x64-windows`) como núcleo del sistema de audio tridimensional del motor.
  - Diseñar el `AudioEngine` como un singleton desacoplado con soporte completo de espacialización 3D por atenuación de distancia, paneo y escucha orientada (Listener con posición, forward y up sincronizados con la cámara activa).
  - Incluir un sintetizador procedural autónomo en memoria (PCM a 44,100 Hz, 16 bits) capaz de sintetizar formas de onda realistas sin requerir archivos de audio externos en disco:
    - 5 superficies de pisada (Césped, Tierra, Roca, Nieve, Concreto) combinando tonos modulados y ruidos filtrados por frecuencia.
    - Salto con barrido de frecuencia ascendente y envolvente sinusoidal.
    - Aterrizaje con atenuación percutiva e impacto proporcional a la velocidad de caída.
    - Crepitar térmico continuo para emisores de partículas de fuego en el espacio 3D.
    - Tono ambiental generativo armónico.
- **Justificación**:
  - `miniaudio` es una librería en C de alto rendimiento, de archivo único, sin dependencias externas complejas, compatible con Windows CoreAudio (WASAPI), DirectSound y DirectMusic.
  - Generar audio procedural autónomo garantiza que el motor funcione al 100% de inmediato sin fallos de "archivo de sonido no encontrado" o rutas rotas.

---

### DEC-027: Jerarquía de Escena con Grupos/Carpetas, Corrección de Glifos BMP y Rejilla Infinita Analítica (Fase 16)
- **Decisión**:
  - **Sustitución de Emojis SMP por Símbolos BMP Geométricos**:
    - Reemplazar caracteres unicode > 0xFFFF (`📦`, `🌐`, `🛢️`, etc.) por símbolos estándar del Plano Multilingüe Básico (`■`, `●`, `▲`, `▮`, `◎`, `▦`, `◈`, `★`) acompañados de colores de categoría idénticos a Blender 4.
    - Esto elimina definitivamente el problema de los diamantes con signo de interrogación (``) que Dear ImGui mostraba al carecer de glifos SMP en la fuente predeterminada.
  - **Sistema de Agrupación de Entidades**:
    - Añadir campo `m_group` en `Entity`, serializado en escenas `.asis` y expuesto a la API de Lua.
    - Renderizado en árbol desplegable dentro del Outliner con control de visibilidad grupal en bloque, selección, desagrupación y borrado conjunto.
    - El generador de IA empaqueta automáticamente las piezas de estructuras complejas (Castillos, Autos, Creepers, Torres) en sus respectivos grupos.
  - **Cuadrícula Infinita Analítica Anti-Aliased Estilo Blender 4**:
    - Implementar el algoritmo analítico de McGuire mediante desproyección de rayos NDC sobre el plano $Y=0$.
    - Escritura directa de profundidad calculada en el Z-Buffer (`gl_FragDepth`) para que los objetos y terrenos ocluyan la rejilla correctamente.
    - Derivadas `fwidth` para líneas anti-aliasing continuas y atenuación cuadrática suave hacia el horizonte.
- **Justificación**:
  - Organizar escenas con decenas o cientos de piezas en carpetas lógicas es un requisito fundamental para la usabilidad y escalabilidad de cualquier motor 3D.
  - La cuadrícula analítica infinita reemplaza una rejilla finita limitada por una guía espacial continua, anti-aliased y moderna idéntica a Blender 4.

---

### DEC-028: Prefabs Reutilizables, Spawning Dinámico con Lua y Zonas Trigger (Fase 17)
- **Decisión**:
  - Implementar formato `.prefab` en JSON estructurado para guardar y reutilizar jerarquías con mallas, materiales PBR, colisionadores de física y datos de transformación relativa.
  - Añadir soporte para Spawning dinámico en Lua (`spawnPrefab(...)`, `getPrefabList()`, `destroyEntity(...)`) en tiempo real.
  - Implementar sensores y disparadores de colisión sin respuesta de fuerza en Bullet Physics (`isTrigger`) con despacho de eventos `onTriggerEnter` y `onTriggerExit` a scripts Lua.
  - Integrar auto-exposición y adaptación ocular cinemática con medición de luminancia para transiciones de luz realistas.
- **Justificación**:
  - Permite crear gameplay modular y reactivo sin recompilar el motor.

---

### DEC-029: Geometría Virtual OASIS Módulo 2 y Empaquetado Standalone (Fase 18)
- **Decisión**:
  - Diseñar partición geométrica en *Meshlets* de 64 vértices / 126 primitivas inspirada en Unreal Nanite.
  - Culling jerárquico de cono de normales (*Normal Cone Culling*) y prueba de visibilidad por frustum/bounding boxes.
  - Sistema de exportación y generación de instalables / ejecutables independientes para Windows (.exe standalone) empaquetando assets, shaders y scripts.
- **Justificación**:
  - Renderizado eficiente de millones de polígonos a más de 100 FPS con uso óptimo de ancho de banda y distribución autónoma de proyectos.

---

### DEC-030: Océano 3D con Olas de Gerstner y Flotabilidad Física Bidireccional (Fase 19)
- **Decisión**:
  - Simular agua y océano mediante 4 trenes de ondas trocoidales de Gerstner calculadas en el vertex shader (`water.vert`).
  - Muestreo analítico de la altura del agua en CPU sincronizado exactamente con la función trigonométrica del shader para calcular el empuje de Arquímedes sobre rigidbodies de Bullet Physics.
  - Espuma dinámica calculada por curvatura Jacobiana y detección de orilla mediante Z-buffer (`linearDepth`).
- **Justificación**:
  - Logra agua realista y dinámica sin mallas estáticas planas, con interacción física real donde los objetos flotan y reaccionan al oleaje.

---

### DEC-031: Ciclo Día/Noche 24h, Clima, SSR, Contact Shadows, CSM y PCSS (Fase 20)
- **Decisión**:
  - Implementar ciclo astronómico de 24 horas continuo con Sol y Luna orbitales, temperatura Kelvin adaptativa y sombras nocturnas lunares.
  - Sistema de clima con lluvia procedural VFX y humedad PBR en superficies.
  - Screen-Space Reflections (SSR) y Contact Shadows en screen-space para reflejos y micro-sombras en juntas y ranuras.
  - Cascaded Shadow Maps (CSM) de 3 cascadas ortográficas y Sombras Suaves Físicas PCSS (Percentage-Closer Soft Shadows) para Sol direccional y Luces Puntuales (Omni-PCSS con cubemaps y base ortonormal tangente).
- **Justificación**:
  - Proporciona fidelidad visual AAA contemporánea, penumbras suaves físicamente verosímiles y ambiente inmersivo.

---

### DEC-032: Gestor de Materiales PBR (.asismat), Jerarquía Profesional, Buscador, Layouts y REPL Lua (Fase 21)
- **Decisión**:
  - **MaterialManager y Formato .asismat**: Centralizar la gestión de materiales PBR como activos independientes en disco con biblioteca de 14 presets físicos (Oro, Cobre, Acero, Cromo, Vidrio, Plástico, Neón, Obsidiana, Mármol, Hormigón, etc.) y asignación a selecciones individuales o grupales.
  - **Atajos Profesionales en Outliner**: `Ctrl+D` para duplicación profunda (mallas, materiales, cuaterniones, triggers y físicas Bullet con offset), `F2` para renombrar, `F` para enfocar cámara, `Supr`/`Delete` para eliminar con registro de undo, y menú contextual de clic derecho con submenú de presets.
  - **Buscador en Tiempo Real en la Escena**: Filtro de subcadenas en cabecera de Outliner con conteo de coincidencias y auto-expansión de carpetas/grupos coincidentes.
  - **Presets de Espacios de Trabajo / Layouts de UI**: 4 perfiles configurados (General/Completo, Iluminación & Cine, Terreno & Modelado, Modo Juego Inmersivo) conmutables desde el menú `Ver` y píldoras en la barra flotante del Viewport.
  - **Consola Lua Interactiva (REPL)**: Prompt de comandos `Lua >` con historial navegable (flechas Arriba/Abajo), ejecución protegida vía `ScriptEngine::executeString` y botones superiores de filtrado de logs (Todos, Info, Advertencias, Errores, Scripts).
- **Justificación**:
  - Maximiza la ergonomía y productividad del usuario acercando el flujo de trabajo de ASIS Engine al estándar de las herramientas profesionales de la industria (Blender, Unity, Unreal Engine).

---

### DEC-033: Sistema de Animación Cinemática / Sequencer, Cámaras de Cine y Curvas Temporales (Fase 22)
- **Decisión**:
  - **Arquitectura del Sequencer (`Sequencer.h`, `Sequencer.cpp`)**: Motor desacoplado que interpola en tiempo real transformaciones 3D de entidades (con cuaterniones `slerp`), cámaras de cine completas (posición, LookAt, óptica física en mm, DoF, apertura de diafragma y vibración de cámara en mano), condiciones ambientales 24h y eventos reactivos en Lua mediante modos de interpolación `Linear`, `Constant (Step)` y `CubicBezier` (curvas Hermite / Bézier cúbicas con tangentes continuas).
  - **Persistencia en Formato JSON `.asisseq`**: Guardado y carga estandarizada de secuencias de animación en la carpeta `assets/sequences/`.
  - **Línea de Tiempo y Dope Sheet / Graph Editor en UI**: Pestaña inferior acoplada en el DockSpace con barra de transporte de 26px (`Play`, `Pause`, `Stop`, `Prev`, `Next`, `First`, `Auto-Key`, `+ Clave (I)`), contador de tiempo en pill cyan, panel de pistas con iconos vectoriales de `ImDrawList` y lienzo temporal interactivo con scrubbing en vivo y visualización conmutable de diamantes de clave o curvas continuas Bézier multicanal.
  - **Herramientas de Dirección y Viewport Overlays**: Máscara anamórfica 2.39:1 (Scope), guías de composición de Regla de Tercios, trayectoria tridimensional suave (Motion Path) con marcadores de keyframe y cono de visión/frustum, y controles ópticos de lentes físicos (18mm, 24mm, 35mm, 50mm, 85mm, 135mm).
  - **Bindings en Lua 5.4**: Exposición de funciones `playSequence`, `pauseSequence`, `stopSequence`, `setSequenceTime`, `isSequencePlaying` y `triggerCameraShake` en la tabla `Engine` y globales para control dinámico desde scripts de juego.
- **Justificación**:
  - Proporciona a ASIS Engine capacidades completas de producción cinemática y secuenciación temporal al nivel de herramientas profesionales de la industria como Unreal Sequencer o Blender Timeline, facilitando la creación de cinemáticas, tráileres, cutscenes y mecánicas de cámara en mano sin necesidad de software externo.

---

### DEC-034: Corrección Matemática del Isotipo ASIS, Normalización Angular, Prioridad de Sequencer y Sistema de Keyframes Blender (Fase 22.5)
- **Decisión**:
  - **Renderizado Analítico del Isotipo Oficial y Barra DWM**: Reemplazar aproximaciones previas por la formulación geométrica exacta de `asisengine.svg`: triángulo isósceles blanco puro (`#FFFFFF`) con 4 cortes negativos horizontales hacia la diagonal izquierda, con espesor mínimo garantizado de $1.6\text{px}$ para evitar artefactos de aliasing a baja escala ($\le 22\text{px}$). Generar además un búfer RGBA de 64x64 para el icono de ventana y habilitar la barra de título oscura de Windows vía la API DWM (`DwmSetWindowAttribute`).
  - **Normalización Angular y Prioridad Cinemática sobre Lua**: Implementar en `Entity` la normalización analítica de ángulos de Euler en $[0, 360)^\circ$ vía `std::fmod` para evitar saltos o desbordamientos durante giros continuos. Introducir la bandera de prioridad `m_controlledBySequencer` para que el Sequencer tenga precedencia absoluta sobre scripts acumulativos procedurales durante la reproducción cinemática o el arrastre manual de la línea de tiempo.
  - **Sincronización y Hornado Procedural (Auto-Bake)**: Crear la función `syncSceneAnimationsToSequencer` y la herramienta `bakeScene` que muestrean en tiempo real entidades manipuladas por IA, scripts de Lua o físicas Bullet y las consolidan en pistas y fotogramas clave discretos del Sequencer, exponiendo las APIs correspondientes a Lua (`seq_clear`, `seq_addEntityTrack`, `seq_addKeyframe`, `seq_bake`).
  - **Sistema de Keyframes y Dope Sheet Estilo Blender**: Incorporar botones vectoriales de rombo diamante (`◇` / `◆`) en las propiedades de traslación, rotación, escala y color, con coloración dinámica del fondo de inputs (dorado Blender para claves exactas, verde oliva para valores interpolados), subcanales desplegables en el Dope Sheet con barras de retención constantes (*Hold Bars*), selección rectangular con caja, atajos `I` (insertar clave), `G` (mover claves), `Shift+D` (duplicar), `Supr`/`X` (eliminar) y `T` (menú emergente de interpolación).
- **Justificación**:
  - Garantiza fidelidad visual idéntica a la identidad gráfica corporativa del motor, elimina conflictos de sincronización entre scripts en vivo y la línea de tiempo, y provee una experiencia de usuario familiar, fluida y coherente con el estándar de la industria (Blender 4/5).

---

### DEC-035: Frustum Culling con Guard Band en OASIS CDLOD, Sincronización Euler/Quaternion y Graph Editor Interactivo (Fase 22.5 Patch)
- **Decisión**:
  - **Frustum Culling con Guard Band en Terreno OASIS (`OASISTerrain.cpp`, `Terrain.cpp`)**:
    - Calcular la matriz `ViewProjection` del culling usando el aspect ratio real del Viewport 3D (`m_viewportSize.x / m_viewportSize.y`) en lugar de proporciones fijas, garantizando que el campo de visión lateral de la cámara coincida con el renderizado.
    - Evaluar el AABB 3D completo de cada chunk con el rango de alturas real considerando `m_baseOffset` (`minY = baseOffset - absMaxH - 20.0f`, `maxY = baseOffset + absMaxH + 30.0f`).
    - Añadir margen de seguridad lateral (*Guard Band*) proporcional al radio del chunk y factor LOD: `guardMargin = chunkRadius * 1.45f * std::max(1.0f, m_lodDistanceFactor * 0.40f)` en `Frustum::isBoxVisible(..., guardMargin)`. Evitar culling por cono en nodos de terreno virtual para prevenir desaparición prematura de chunks en los bordes del Viewport al aumentar la distancia LOD.
  - **Sincronización Euler <-> Quaternion y Sequencer Reactivo (`Entity.h`, `Sequencer.h`, `EditorUI.cpp`)**:
    - Implementar `Entity::syncQuaternionFromEuler()` con orden de rotación consistente Y -> X -> Z, ejecutada en `setRotation`, `rotate`, y al manipular gizmos en el Viewport o campos en el Inspector.
    - Bandera `m_forceSampleOnce` en `Sequencer`: Permite que cuando el Sequencer esté pausado no sobreescriba las modificaciones manuales de gizmos o inspector salvo que se reproduzca o se solicite explícitamente con `seq.requestSample()`.
    - Auto-Key reactivo al transformar entidades cuando `animState == 2` o `isAutoKeyEnabled()`.
    - Restringir la llamada a `syncSceneAnimationsToSequencer` a una única vez al iniciar el motor (`!m_initialSequencerSynced`).
  - **Línea de Tiempo Dope Sheet y Graph Editor Interactivo (`EditorUI.cpp`)**:
    - Arrastre interactivo continuo con clic izquierdo (LMB Drag) de los rombos de keyframes con previsualización en vivo (`seq.requestSample()`) y ordenado al soltar (`sortAllKeyframes`).
    - Doble clic en un rombo: ubica el cabezal de tiempo exactamente en el keyframe y selecciona la entidad en el Outliner.
    - Clic derecho en un rombo: despliega menú contextual `SeqKeyframeContextPopup` con edición numérica directa de tiempo, posición, rotación Euler, escala, modos de interpolación, botón "Capturar Pose Actual", duplicación y eliminación.
    - Graph Editor con selector de canales (`Posición`, `Rotación`, `Escala`), auto-escalado vertical centrado según valores mínimos y máximos de la pista, y nodos circulares de control arrastrables bidireccionalmente (horizontal modifica tiempo, vertical modifica valor).
    - Desplazamiento horizontal con MMB drag o `Shift + Rueda` (`m_sequencerScrollX`), y sincronización vertical del panel de pistas con el lienzo temporal mediante `trackListScrollY` y `PushClipRect`.
  - **Corrección de Textos Cortados**:
    - Pestaña OASIS: Títulos y checkboxes completos sin solapamientos, separación de columnas en Debug LOD y Subdivision LOD, y reserva de altura para telemetría.
    - Pestaña Importación/Exportación: Botones expandidos con nombres completos de formatos y extensiones (`.obj + .mtl`, `.gltf 2.0`).
- **Justificación**:
  - Elimina artefactos visuales de popping en bordes de pantalla para terrenos de alta resolución con cualquier relación de aspecto, resuelve discrepancias de rotación entre Euler y cuaterniones, y entrega herramientas completas de edición de curvas y fotogramas clave con ergonomía profesional.

---

### DEC-036: Sincronización de Distancia de Órbita, Coherencia Bóveda/Sombras, CSM Adaptativo y Físicas de Personaje (Fase 22.5 Hotfix)
- **Decisión**:
  - **Sincronización Dinámica de `m_orbitDistance` (`Camera.h`, `Camera.cpp`)**:
    - Centralizar la actualización de la distancia de órbita con `syncOrbitDistance() = glm::max(0.5f, glm::length(m_target - m_position))` en los accesores `setPosition`, `setTarget` y en los eventos de inicio de órbita/zoom.
    - Evita que al regresar de vuelos de cámara, animaciones cinemáticas del Sequencer o clics en el Navigation Gizmo, la cámara salte bruscamente a una distancia de órbita obsoleta.
  - **Unificación de Coordenadas Esféricas Celestes (`DayNightCycle.cpp`, `EditorUI.cpp`)**:
    - Adoptar la misma convención trigonométrica de vector de incidencia de luz `dir = (cos(el)*cos(az), sin(el), cos(el)*sin(az))` y `lightDir = -normalize(dir)` en el cálculo de sombras y en el trazado del disco solar en el Skybox shader.
    - Redireccionar las sombras al vector lunar (`getMoonDirection()`) durante la noche (`isNight()`), restringiendo el vector a un ángulo descendente mínimo equivalente a $12^\circ$ bajo el horizonte (`y <= -sin(12°)`). Impide que las sombras apunten hacia arriba o generen penumbras infinitas rasantes al ocultarse el sol.
  - **Relación de Aspecto Real en Frustums de Cascadas (`CascadedShadowMap.cpp`, `main.cpp`)**:
    - Reemplazar el aspect ratio fijo de 16:9 en `calculateCascadeMatrix()` por el aspect ratio dinámico transmitido por `main.cpp` (`framebuffer.getAspectRatio()` o resolución de ventana). Garantiza que las 3 cajas envolventes de las cascadas cubran exactamente la pirámide de visión visible sin sub-muestreo lateral ni desperdicio de texels.
  - **Frecuencia Invariante de Head Bobbing y Detección de Biomas (`CharacterController.cpp`)**:
    - Escalar el incremento de `m_headBobTimer` directamente con el `deltaTime` del bucle principal en `CharacterController::update()`, garantizando la misma cadencia de oscilación independientemente de si el motor corre a 30, 60 o 144+ FPS.
    - Normalizar las cotas de elevación en `detectPlayerSurface()` restando `terrain->getBaseOffset()`, asegurando que las texturas y efectos de audio de pisadas correspondan fielmente al bioma visual del terreno con cualquier desplazamiento vertical.
- **Justificación**:
  - Garantiza robustez matemática, suavidad de control e integración fotorrealista coherente entre la atmósfera, la iluminación direccional y las físicas de cámara y personaje.

---

### DEC-037: Fórmula de Geomorphing CDLOD, Modos de Shading en Viewport, Aspect Ratio en Meshlets, Prioridad Cinemática en Bullet y Persistencia Drag & Drop (Fase 22.6)
- **Decisión**:
  - **Fórmula de Geomorphing Continuo en Terreno OASIS CDLOD (`OASISTerrain.cpp`)**:
    - Para los chunks hoja (`lod > 0`), el geomorphing se calcula con respecto al umbral de subdivisión del padre (`parentSubdivThreshold = (size * 2.0f) * 2.2f * m_lodDistanceFactor;`), aplicando la transición continua en el último 30% del rango (`morphRange = parentSubdivThreshold * 0.30f`). Para los chunks raíz (`lod == 0`), `morphFactor = 0.0f` al no existir un nivel superior hacia el cual converger.
    - Se amplía el margen de seguridad de Frustum Culling (`guardMargin = size * 1.75f * std::max(1.0f, m_lodDistanceFactor * 0.5f) + 15.0f;`) para erradicar cualquier popping visual en los bordes de la pantalla cuando la distancia LOD se eleva hasta 4.00x.
  - **Conexión de Modos de Shading de Viewport (`main.cpp`)**:
    - Implementación de `ViewportShadingMode::SOLID`: anulación dinámica de sombras (`u_ShadowsEnabled = 0`) y reflexiones IBL (`u_UseIBL = 0`) con iluminación ambiental uniforme neutra (`u_AmbientColor = vec3(0.45f)`), emulando el modo sólido de modelado ágil de Blender.
    - Implementación de `ViewportShadingMode::OASIS_MESHLETS`: forzado temporal durante el frame de depuración cromática de Meshlets (`debugColors = true`) y coloreado de chunks CDLOD (`terrain.getOASIS()->setDebugLOD(true)`), restaurando el estado original al finalizar el pase de renderizado.
  - **Relación de Aspecto Dinámica y Margen de Culling en Meshlets (`Mesh.cpp`, `OASISMeshlet.cpp`)**:
    - En `Mesh::drawOASIS`: consulta en tiempo de ejecución del tamaño real del viewport GL (`glGetIntegerv(GL_VIEWPORT, vp)`) para derivar la matriz de proyección con aspect ratio exacto, evitando culling prematuro en ratios no 16:9.
    - En `OASISMeshletContainer::cullAndCollect`: incorporación de un margen de guarda de radio esférico `worldSphereRadius * 0.5f` en la verificación contra los planos del frustum.
  - **Prioridad Cinemática de Sequencer sobre Simulación Física (`PhysicsWorld.cpp`)**:
    - En `PhysicsWorld::syncPhysicsToEntities`: comprobación de si la entidad posee la bandera `isControlledBySequencer()` activa y el Sequencer se encuentra en reproducción (`Sequencer::get().isPlaying()`). En dicho caso, Bullet no sobreescribe la transformación espacial de la entidad, sino que se sincroniza el RigidBody hacia la posición y rotación impuesta por el Sequencer (`syncEntityToPhysics`), garantizando animación cinemática limpia sin artefactos de gravedad.
  - **Persistencia Completa y Restauración Exacta en Drag & Drop (`main.cpp`, `SceneIO.cpp`)**:
    - En `main.cpp`: sustitución de la carga básica por `editorUI.loadProject(...)`, permitiendo que al arrastrar cualquier archivo `.asis` sobre la ventana se restauren integralmente terreno, cuerpos de agua, cielo HDRI, parámetros atmosféricos y simulación física.
    - En `SceneIO::loadSceneASIS`: deserialización coordinada de `"position"` y `"target"`, invocando `camera.resetToFocus(camTarget); camera.setPosition(camPos); camera.syncOrbitDistance();` para recrear exactamente el encuadre y distancia de órbita almacenados.
- **Justificación**:
  - Elimina distorsiones geométricas en el terreno procedural por geomorphing desfasado, unifica la experiencia visual de sombreado con Blender 5, asegura la integridad cinemática del sequencer contra la gravedad física y consolida la portabilidad de proyectos `.asis`.

---

### DEC-038: Biomas Analíticos por Diferencias Centrales, Sub-actualización de Alturas GPU y Descarga de Texturas PBR (Fase 22.7)
- **Decisión**:
  - **Independencia de Resolución en Splatmap de Biomas (`Terrain.cpp`)**:
    - Calcular el vector normal y la pendiente `slopeY` directamente sobre el array de alturas `m_heights` mediante diferencias centrales analíticas (`(hR - hL) / dx`, `(hU - hD) / dz`), evitando acceder a la malla VBO acotada a $256 \times 256$.
    - Evaluar la altitud de bioma con la coordenada local `m_heights[idx]` sin restar `m_baseOffset`, garantizando correspondencia con los umbrales de nieve ($> 14.0\text{m}$) y tierra ($> 5.5\text{m}$).
  - **Actualización Parcial en Escultura Interactiva (`Terrain.cpp`, `EditorUI.cpp`)**:
    - Sustituir la re-creación total de la textura GPU (R32F) y VBO en cada cuadro de esculpido por llamadas localizadas a `m_oasis->updateHeightmapSubRegion(...)` (`glTexSubImage2D`).
    - Posponer la sincronización física de Bullet y el cómputo de la malla de respaldo hasta la liberación del botón del ratón (`ImGui::IsMouseReleased`), garantizando respuesta instantánea a 60-120+ FPS sin micro-tirones durante el esculpido interactivo.
  - **Gestión Exhaustiva de Memoria VRAM en Capas de Terreno (`Terrain.cpp`)**:
    - Incorporar llamadas explícitas a `glDeleteTextures` para los mapas de altura (`heightTex`) y oclusión ambiental (`aoTex`) en `Terrain::shutdown()`.
  - **Guard Band en Frustum Culling de Superficies Oceánicas (`Water.cpp`)**:
    - Añadir margen de guarda `guardMargin = size * 1.5f * std::max(1.0f, m_oasisLodFactor * 0.5f) + 15.0f` a `frustum.isBoxVisible` en `Water::evaluateQuadtree`, asegurando continuidad visual sin desaparición de crestas de olas en los bordes del campo de visión.
- **Justificación**:
  - Previene caídas drásticas de tasa de cuadros durante la edición de terreno en alta resolución, elimina accesos desfasados de memoria en splatmaps y erradica fugas de memoria VRAM.



