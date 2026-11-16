cmake_minimum_required(VERSION 3.20)
project(ASIS_Engine VERSION 0.16.0 LANGUAGES C CXX)

# Exigir C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paquetes vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(sol2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(imguizmo CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

# Código fuente del núcleo
set(CORE_SOURCES
    src/core/main.cpp
    src/core/Window.h
    src/core/Window.cpp
    src/core/Shader.h
    src/core/Shader.cpp
    src/core/Mesh.h
    src/core/Mesh.cpp
    src/core/Texture.h
    src/core/Texture.cpp
    src/core/Material.h
    src/core/Material.cpp
    src/core/MaterialManager.h
    src/core/MaterialManager.cpp
    src/core/Light.h
    src/core/Camera.h
    src/core/Camera.cpp
    src/core/Model.h
    src/core/Model.cpp
    src/core/Renderer.h
    src/core/Renderer.cpp
    src/core/Entity.h
    src/core/Entity.cpp
    src/core/Scene.h
    src/core/Scene.cpp
    src/core/PhysicsWorld.h
    src/core/PhysicsWorld.cpp
    src/core/ScriptEngine.h
    src/core/ScriptEngine.cpp
    src/core/NetworkServer.h
    src/core/NetworkServer.cpp
    src/core/Framebuffer.h
    src/core/Framebuffer.cpp
    src/core/ShadowMap.h
    src/core/ShadowMap.cpp
    src/core/CascadedShadowMap.h
    src/core/CascadedShadowMap.cpp
    src/core/OmniShadowMap.h
    src/core/OmniShadowMap.cpp
    src/core/SceneIO.h
    src/core/SceneIO.cpp
    src/core/PrefabManager.h
    src/core/PrefabManager.cpp
    src/core/EditorUI.h
    src/core/EditorUI.cpp
    src/core/Skybox.h
    src/core/Skybox.cpp
    src/core/PostProcess.h
    src/core/PostProcess.cpp
    src/core/ParticleSystem.h
    src/core/ParticleSystem.cpp
    src/core/SSAO.h
    src/core/SSAO.cpp
    src/core/Animation.h
    src/core/Animation.cpp
    src/core/Terrain.h
    src/core/Terrain.cpp
    src/core/OASISTerrain.h
    src/core/OASISTerrain.cpp
    src/core/OASISMeshlet.h
    src/core/OASISMeshlet.cpp
    src/core/CharacterController.h
    src/core/CharacterController.cpp
    src/core/AudioEngine.h
    src/core/AudioEngine.cpp
    src/core/Water.h
    src/core/Water.cpp
    src/core/DayNightCycle.h
    src/core/DayNightCycle.cpp
    src/core/Sequencer.h
    src/core/Sequencer.cpp
)

add_executable(ASIS_Engine ${CORE_SOURCES})

target_include_directories(ASIS_Engine PRIVATE
    src/core
    ${LUA_INCLUDE_DIR}
    ${BULLET_INCLUDE_DIR}
    C:/dev/vcpkg/installed/x64-windows/include
)

target_link_libraries(ASIS_Engine PRIVATE
    glfw
    glad::glad
    glm::glm
    assimp::assimp
    sol2::sol2
    imgui::imgui
    imguizmo::imguizmo
    ${LUA_LIBRARIES}
    ${BULLET_LIBRARIES}
    ws2_32
    opengl32
    ZLIB::ZLIB
)

# Copiar assets y scripts al directorio del ejecutable tras la compilación
add_custom_command(TARGET ASIS_Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:ASIS_Engine>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders
    $<TARGET_FILE_DIR:ASIS_Engine>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scripts
    $<TARGET_FILE_DIR:ASIS_Engine>/scripts
    COMMENT "[ASIS Engine] Copiando assets y scripts al directorio de salida..."
)
