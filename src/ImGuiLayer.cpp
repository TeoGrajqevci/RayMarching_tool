#include "ImGuiLayer.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

void ImGuiLayer::Init(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 2.0f;
    style.GrabRounding      = 2.0f;
    style.ItemSpacing       = ImVec2(8, 4);       // Reduced Y spacing from 8 to 4
    style.WindowPadding     = ImVec2(10, 10);
    style.PopupRounding     = 4.0f;
    style.IndentSpacing     = 12.0f;              // Added: controls tree indentation
    style.ItemInnerSpacing  = ImVec2(4, 2);       // Added: controls spacing between elements

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]           = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    colors[ImGuiCol_Header]             = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_Button]             = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_Tab]                = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    colors[ImGuiCol_Separator]          = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void ImGuiLayer::NewFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::Render() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}