// ==============================================================================
// Snippet: Integración de Editor UI (Dear ImGui) estilo Unreal / Blender
// ==============================================================================
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

void setupImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Inicializar backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void renderInspector(Entity* selected) {
    if (!selected) return;

    ImGui::Begin("Inspector");
    ImGui::Text("Entidad: %s", selected->getName().c_str());

    glm::vec3& pos = selected->getPositionRef();
    ImGui::DragFloat3("Posición", &pos[0], 0.05f);

    glm::vec3& rot = selected->getRotationRef();
    ImGui::DragFloat3("Rotación", &rot[0], 1.0f);

    glm::vec3& scale = selected->getScaleRef();
    ImGui::DragFloat3("Escala", &scale[0], 0.05f);

    glm::vec3& col = selected->getColorRef();
    if (ImGui::ColorEdit3("Color", &col[0])) {
        selected->setHasCustomColor(true);
    }
    ImGui::End();
}
