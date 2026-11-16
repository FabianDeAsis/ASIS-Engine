// ==============================================================================
// Snippet de Referencia: Integración de Framebuffer FBO e ImGuizmo en ASIS Engine
// Archivo de referencia para Fase 6: Renderizado Offscreen y Gizmos 3D W/E/R
// ==============================================================================

#include <glad/glad.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// 1. Renderizado en el Framebuffer Object (FBO) desacoplado
void renderSceneToFBO(ASIS::Framebuffer& fbo, ASIS::Camera& camera, ASIS::Scene& scene, ASIS::Shader& shader) {
    fbo.bind(); // glViewport(0, 0, width, height) y glBindFramebuffer(GL_FRAMEBUFFER, fboId)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = fbo.getAspectRatio();
    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();

    shader.use();
    shader.setMat4("u_Projection", proj);
    shader.setMat4("u_View", view);
    scene.draw(shader);

    fbo.unbind(); // glBindFramebuffer(GL_FRAMEBUFFER, 0)
}

// 2. Proyección de la textura FBO en Dear ImGui y manipulación con ImGuizmo
void renderViewportWithGizmo(ASIS::Framebuffer& fbo, ASIS::Camera& camera, std::shared_ptr<ASIS::Entity> selectedEntity,
                            int gizmoOp, int gizmoMode, bool useSnap, float snapVal[3]) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin("Viewport 3D")) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        fbo.resize((int)avail.x, (int)avail.y);

        ImVec2 screenPos = ImGui::GetCursorScreenPos();

        // OpenGL almacena las texturas invertidas verticalmente (UV: 0,1 -> 1,0)
        ImTextureID texID = (ImTextureID)(intptr_t)fbo.getColorTexture();
        ImGui::Image(texID, avail, ImVec2(0, 1), ImVec2(1, 0));

        if (selectedEntity) {
            ImGuizmo::SetOrthographic(false);
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(screenPos.x, screenPos.y, avail.x, avail.y);

            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 proj = camera.getProjectionMatrix(avail.x / avail.y);
            glm::mat4 model = selectedEntity->getModelMatrix();

            bool manipulated = ImGuizmo::Manipulate(
                glm::value_ptr(view),
                glm::value_ptr(proj),
                (ImGuizmo::OPERATION)gizmoOp,
                (ImGuizmo::MODE)gizmoMode,
                glm::value_ptr(model),
                nullptr,
                useSnap ? snapVal : nullptr
            );

            if (manipulated) {
                float t[3], r[3], s[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), t, r, s);
                selectedEntity->setPosition(t[0], t[1], t[2]);
                selectedEntity->setRotation(r[0], r[1], r[2]);
                selectedEntity->setScale(s[0], s[1], s[2]);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
