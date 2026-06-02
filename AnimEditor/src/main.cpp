#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ui/EditorUI.h"

static void renderFrame(GLFWwindow* window) {
    ImGui::Render();

    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (!glfwInit()) {
        return 1;
    }

    // OpenGL 3.3 core profile (required on macOS)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "AnimEditor", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::StyleColorsDark();

    // Platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Setup editor UI
    anim::EditorUI editor;
    editor.init();

    ImGuiID mainDockId = 0;
    bool dockBuilt = false;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // DockSpaceOverViewport must be called every frame to maintain
        // the full-viewport invisible host window. On the first frame we
        // build the node tree with DockBuilder; subsequent frames reuse it.
        mainDockId = ImGui::DockSpaceOverViewport(0, nullptr,
            ImGuiDockNodeFlags_PassthruCentralNode);

        if (!dockBuilt) {
            auto* viewport = ImGui::GetMainViewport();

            ImGui::DockBuilderRemoveNode(mainDockId);
            ImGui::DockBuilderAddNode(mainDockId,
                ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
            ImGui::DockBuilderSetNodeSize(mainDockId, viewport->Size);

            ImGuiID topId, bottomId;
            ImGuiID topLeftId, topCenterId, topRightId;
            ImGuiID botLeftId, botRightId;

            ImGui::DockBuilderSplitNode(mainDockId, ImGuiDir_Down, 0.30f,
                                        &bottomId, &topId);
            ImGui::DockBuilderSplitNode(topId, ImGuiDir_Left, 0.20f,
                                        &topLeftId, &topRightId);
            ImGui::DockBuilderSplitNode(topRightId, ImGuiDir_Right, 0.25f,
                                        &topRightId, &topCenterId);
            ImGui::DockBuilderSplitNode(bottomId, ImGuiDir_Left, 0.35f,
                                        &botLeftId, &botRightId);

            ImGui::DockBuilderDockWindow("Files", topLeftId);
            ImGui::DockBuilderDockWindow("Preview", topCenterId);
            ImGui::DockBuilderDockWindow("Properties", topRightId);
            ImGui::DockBuilderDockWindow("Nodes", botLeftId);
            ImGui::DockBuilderDockWindow("Timeline", botRightId);

            ImGui::DockBuilderFinish(mainDockId);
            dockBuilt = true;
        }

        editor.render();
        renderFrame(window);
    }

    editor.shutdown();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
