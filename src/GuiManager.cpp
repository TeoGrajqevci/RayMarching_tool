#include "GuiManager.h"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iostream>

GUIManager::GUIManager() { }
GUIManager::~GUIManager() { }

void GUIManager::newFrame() {
    ImGuiLayer::NewFrame();
}

void GUIManager::renderGUI(GLFWwindow* window, std::vector<Shape>& shapes, std::vector<int>& selectedShapes,
                           float lightDir[3], float lightColor[3], float ambientColor[3],
                           bool& useGradientBackground, char (&hdrFilePath)[512], Texture& hdrTexture, bool& hdrLoaded,
                           float& hdrIntensity, bool& useHdrBackground, bool& useHdrLighting,
                           bool& showGUI, bool& altRenderMode,
                           int& editingShapeIndex, char (&renameBuffer)[128],
                           bool& showHelpPopup, ImVec2& helpButtonPos)
{
    static bool addShapePopupTriggered = false;
    static ImVec2 addShapePopupPos;
    if (!ImGui::GetIO().WantCaptureKeyboard &&
        glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS &&
        (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS))
    {
        if (!addShapePopupTriggered) {
            addShapePopupTriggered = true;
            double mouseX, mouseY;
            glfwGetCursorPos(window, &mouseX, &mouseY);
            addShapePopupPos = ImVec2((float)mouseX, (float)mouseY);
            ImGui::OpenPopup("Add Shape");
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_RELEASE) {
        addShapePopupTriggered = false;
    }
    ImGui::SetNextWindowPos(addShapePopupPos);
    if (ImGui::BeginPopup("Add Shape"))
    {
        if (ImGui::Selectable("Sphere"))
        {
            Shape newShape;
            newShape.type = SHAPE_SPHERE;
            newShape.center[0] = newShape.center[1] = newShape.center[2] = 0.0f;
            newShape.param[0] = 0.5f;
            newShape.param[1] = newShape.param[2] = 0.0f;
            newShape.rotation[0] = newShape.rotation[1] = newShape.rotation[2] = 0.0f;
            newShape.extra = 0.0f;
            newShape.blendOp = BLEND_NONE;
            newShape.smoothness = 0.1f;
            newShape.name = std::to_string(shapes.size());
            newShape.albedo[0] = 1.0f; newShape.albedo[1] = 1.0f; newShape.albedo[2] = 1.0f;
            newShape.metallic = 0.0f;
            newShape.roughness = 0.05f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Box"))
        {
            Shape newShape;
            newShape.type = SHAPE_BOX;
            newShape.center[0] = newShape.center[1] = newShape.center[2] = 0.0f;
            newShape.param[0] = newShape.param[1] = newShape.param[2] = 0.5f;
            newShape.rotation[0] = newShape.rotation[1] = newShape.rotation[2] = 0.0f;
            newShape.extra = 0.0f;
            newShape.blendOp = BLEND_NONE;
            newShape.smoothness = 0.1f;
            newShape.name = std::to_string(shapes.size());
            newShape.albedo[0] = 1.0f; newShape.albedo[1] = 1.0f; newShape.albedo[2] = 1.0f;
            newShape.metallic = 0.0f;
            newShape.roughness = 0.05f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Round Box"))
        {
            Shape newShape;
            newShape.type = SHAPE_ROUNDBOX;
            newShape.center[0] = newShape.center[1] = newShape.center[2] = 0.0f;
            newShape.param[0] = newShape.param[1] = newShape.param[2] = 0.5f;
            newShape.rotation[0] = newShape.rotation[1] = newShape.rotation[2] = 0.0f;
            newShape.extra = 0.1f;
            newShape.blendOp = BLEND_NONE;
            newShape.smoothness = 0.1f;
            newShape.name = std::to_string(shapes.size());
            newShape.albedo[0] = 1.0f; newShape.albedo[1] = 1.0f; newShape.albedo[2] = 1.0f;
            newShape.metallic = 0.0f;
            newShape.roughness = 0.05f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Torus"))
        {
            Shape newShape;
            newShape.type = SHAPE_TORUS;
            newShape.center[0] = newShape.center[1] = newShape.center[2] = 0.0f;
            newShape.param[0] = 0.5f;
            newShape.param[1] = 0.2f;
            newShape.param[2] = 0.0f;
            newShape.rotation[0] = newShape.rotation[1] = newShape.rotation[2] = 0.0f;
            newShape.extra = 0.0f;
            newShape.blendOp = BLEND_NONE;
            newShape.smoothness = 0.1f;
            newShape.name = std::to_string(shapes.size());
            newShape.albedo[0] = 1.0f; newShape.albedo[1] = 1.0f; newShape.albedo[2] = 1.0f;
            newShape.metallic = 0.0f;
            newShape.roughness = 0.05f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Selectable("Cylinder"))
        {
            Shape newShape;
            newShape.type = SHAPE_CYLINDER;
            newShape.center[0] = newShape.center[1] = newShape.center[2] = 0.0f;
            newShape.param[0] = 0.5f;
            newShape.param[1] = 0.5f;
            newShape.param[2] = 0.0f;
            newShape.rotation[0] = newShape.rotation[1] = newShape.rotation[2] = 0.0f;
            newShape.extra = 0.0f;
            newShape.blendOp = BLEND_NONE;
            newShape.smoothness = 0.1f;
            newShape.name = std::to_string(shapes.size());
            newShape.albedo[0] = 1.0f; newShape.albedo[1] = 1.0f; newShape.albedo[2] = 1.0f;
            newShape.metallic = 0.0f;
            newShape.roughness = 0.05f;
            shapes.push_back(newShape);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    int winWidth, winHeight;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    float aspectRatio = static_cast<float>(winWidth) / static_cast<float>(winHeight);

    ImGui::SetNextWindowPos(ImVec2(winWidth - 180, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(180, winHeight - winHeight/1.8), ImGuiCond_Always);
    ImGui::Begin("Shape Directory", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    for (size_t i = 0; i < shapes.size(); i++)
    {
        if (i > 0) ImGui::Separator();

        ImVec4 iconColor;
        switch (shapes[i].type) {
            case SHAPE_SPHERE:   iconColor = ImVec4(0.2f, 0.6f, 0.9f, 1.0f); break;
            case SHAPE_BOX:      iconColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f); break;
            case SHAPE_ROUNDBOX: iconColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f); break;
            case SHAPE_TORUS:    iconColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f); break;
            case SHAPE_CYLINDER: iconColor = ImVec4(0.7f, 0.3f, 0.9f, 1.0f); break;
            default:             iconColor = ImVec4(1,1,1,1); break;
        }
        ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
        ImGui::Bullet();
        ImGui::PopStyleColor();

        if (shapes[i].blendOp != BLEND_NONE) {
            ImGui::SameLine();
            ImVec4 blendColor;
            switch (shapes[i].blendOp) {
                case BLEND_SMOOTH_UNION:        blendColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); break;
                case BLEND_SMOOTH_SUBTRACTION:  blendColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;
                case BLEND_SMOOTH_INTERSECTION: blendColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f); break;
                default:                        blendColor = ImVec4(1,1,1,1); break;
            }
            ImGui::PushStyleColor(ImGuiCol_Text, blendColor);
            ImGui::Bullet();
            ImGui::PopStyleColor();
        }

        std::string shapeTypeName;
        switch (shapes[i].type) {
            case SHAPE_SPHERE:   shapeTypeName = "Sphere "; break;
            case SHAPE_BOX:      shapeTypeName = "Box "; break;
            case SHAPE_ROUNDBOX: shapeTypeName = "Round Box "; break;
            case SHAPE_TORUS:    shapeTypeName = "Torus "; break;
            case SHAPE_CYLINDER: shapeTypeName = "Cylinder "; break;
            default:             shapeTypeName = "Unknown "; break;
        }

        ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        if (std::find(selectedShapes.begin(), selectedShapes.end(), i) != selectedShapes.end())
            node_flags |= ImGuiTreeNodeFlags_Selected;

        if (editingShapeIndex == i) {
            ImGui::SameLine();
            char tempBuffer[128];
            strncpy(tempBuffer, renameBuffer, sizeof(tempBuffer));
            if (ImGui::InputText("##rename", tempBuffer, sizeof(tempBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
            {
                shapes[i].name = tempBuffer;
                editingShapeIndex = -1;
            }
            if (!ImGui::IsItemActive() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1))) {
                editingShapeIndex = -1;
            }
            ImGui::TreeNodeEx((shapeTypeName + shapes[i].name).c_str(), node_flags);
        } else {
            ImGui::TreeNodeEx((shapeTypeName + shapes[i].name).c_str(), node_flags);
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(0)) {
                editingShapeIndex = i;
                strncpy(renameBuffer, shapes[i].name.c_str(), sizeof(renameBuffer));
                renameBuffer[sizeof(renameBuffer)-1] = '\0';
            }
        }
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("DND_SHAPE", &i, sizeof(size_t));
            ImGui::Text("Moving %s", (shapeTypeName + shapes[i].name).c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_SHAPE"))
            {
                size_t srcIndex = *(const size_t*)payload->Data;
                if (srcIndex != i)
                {
                    Shape draggedShape = shapes[srcIndex];
                    shapes.erase(shapes.begin() + srcIndex);
                    shapes.insert(shapes.begin() + i, draggedShape);
                }
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
        {
            editingShapeIndex = i;
            strncpy(renameBuffer, shapes[i].name.c_str(), sizeof(renameBuffer));
            renameBuffer[sizeof(renameBuffer)-1] = '\0';
        }
        if (ImGui::IsItemClicked())
        {
            if (ImGui::GetIO().KeyShift)
            {
                auto it = std::find(selectedShapes.begin(), selectedShapes.end(), i);
                if (it != selectedShapes.end())
                    selectedShapes.erase(it);
                else
                    selectedShapes.push_back(i);
            }
            else
            {
                selectedShapes.clear();
                selectedShapes.push_back(i);
            }
        }
    }
    ImGui::End();

    if (!selectedShapes.empty())
    {
        ImGui::SetNextWindowPos(ImVec2(0, 190), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(290, 150), ImGuiCond_Always);
        ImGui::Begin("Shape Parameters", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        int selIdx = selectedShapes[0];
        Shape &selectedShape = shapes[selIdx];
        if (ImGui::BeginTabBar("ShapeParametersTabs", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Transform"))
            {
                ImGui::DragFloat3("Center", selectedShape.center, 0.01f);
                switch(selectedShape.type)
                {
                    case SHAPE_SPHERE:
                        ImGui::DragFloat("Radius", &selectedShape.param[0], 0.01f);
                        break;
                    case SHAPE_BOX:
                        ImGui::DragFloat3("Half Extents", selectedShape.param, 0.01f);
                        {
                            float rotDeg[3] = { selectedShape.rotation[0] * 57.2958f,
                                                selectedShape.rotation[1] * 57.2958f,
                                                selectedShape.rotation[2] * 57.2958f };
                            if (ImGui::DragFloat3("Rotation", rotDeg, 0.1f))
                            {
                                selectedShape.rotation[0] = rotDeg[0] * 0.0174533f;
                                selectedShape.rotation[1] = rotDeg[1] * 0.0174533f;
                                selectedShape.rotation[2] = rotDeg[2] * 0.0174533f;
                            }
                        }
                        break;
                    case SHAPE_ROUNDBOX:
                        ImGui::DragFloat3("Half Extents", selectedShape.param, 0.01f);
                        ImGui::DragFloat("Roundness", &selectedShape.extra, 0.01f);
                        {
                            float rotDeg[3] = { selectedShape.rotation[0] * 57.2958f,
                                                selectedShape.rotation[1] * 57.2958f,
                                                selectedShape.rotation[2] * 57.2958f };
                            if (ImGui::DragFloat3("Rotation", rotDeg, 0.1f))
                            {
                                selectedShape.rotation[0] = rotDeg[0] * 0.0174533f;
                                selectedShape.rotation[1] = rotDeg[1] * 0.0174533f;
                                selectedShape.rotation[2] = rotDeg[2] * 0.0174533f;
                            }
                        }
                        break;
                    case SHAPE_TORUS:
                        ImGui::DragFloat("Major Radius", &selectedShape.param[0], 0.01f);
                        ImGui::DragFloat("Minor Radius", &selectedShape.param[1], 0.01f);
                        {
                            float rotDeg[3] = { selectedShape.rotation[0] * 57.2958f,
                                                selectedShape.rotation[1] * 57.2958f,
                                                selectedShape.rotation[2] * 57.2958f };
                            if (ImGui::DragFloat3("Rotation", rotDeg, 0.1f))
                            {
                                selectedShape.rotation[0] = rotDeg[0] * 0.0174533f;
                                selectedShape.rotation[1] = rotDeg[1] * 0.0174533f;
                                selectedShape.rotation[2] = rotDeg[2] * 0.0174533f;
                            }
                        }
                        break;
                    case SHAPE_CYLINDER:
                        ImGui::DragFloat("Radius", &selectedShape.param[0], 0.01f);
                        ImGui::DragFloat("Half Height", &selectedShape.param[1], 0.01f);
                        {
                            float rotDeg[3] = { selectedShape.rotation[0] * 57.2958f,
                                                selectedShape.rotation[1] * 57.2958f,
                                                selectedShape.rotation[2] * 57.2958f };
                            if (ImGui::DragFloat3("Rotation", rotDeg, 0.1f))
                            {
                                selectedShape.rotation[0] = rotDeg[0] * 0.0174533f;
                                selectedShape.rotation[1] = rotDeg[1] * 0.0174533f;
                                selectedShape.rotation[2] = rotDeg[2] * 0.0174533f;
                            }
                        }
                        break;
                    default:
                        ImGui::Text("Unknown shape type.");
                        break;
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Material"))
            {
                ImGui::ColorEdit3("Albedo", selectedShape.albedo);
                ImGui::DragFloat("Metallic", &selectedShape.metallic, 0.01f, 0.0f, 1.0f);
                ImGui::DragFloat("Roughness", &selectedShape.roughness, 0.001f, 0.0f, 0.1f);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Blend"))
            {
                const char* blendModeItems[] = { "None", "Smooth Union", "Smooth Subtraction", "Smooth Intersection" };
                ImGui::Combo("Blend Mode", &selectedShape.blendOp, blendModeItems, IM_ARRAYSIZE(blendModeItems));
                ImGui::DragFloat("Smoothness", &selectedShape.smoothness, 0.01f, 0.001f, 1.0f);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(366, 190), ImGuiCond_Always);
    ImGui::Begin("Lighting Parameters", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    if (ImGui::BeginTabBar("LightingParametersTabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Direct Light"))
        {
            ImGui::DragFloat3("Light Direction", lightDir, 0.01f);
            ImGui::ColorEdit3("Light Color", lightColor);
            ImGui::ColorEdit3("Ambient Color", ambientColor);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("HDR Environment"))
        {
            ImGui::Text("HDR Image: %s", hdrLoaded ? hdrFilePath : "None");
            static char inputPath[512] = "";
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
            ImGui::InputTextWithHint("##hdrpath", "Drag & drop HDR/EXR file here or enter path...", inputPath, sizeof(inputPath));
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            if (ImGui::BeginDragDropTarget()) 
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EXTERNAL_FILE"))
                {
                    const char* dropped_path = (const char*)payload->Data;
                    std::string filepath(dropped_path);
                    std::string ext = filepath.substr(filepath.find_last_of(".") + 1);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == "hdr" || ext == "exr") {
                        if (hdrTexture.loadHDR(dropped_path)) {
                            strncpy(hdrFilePath, dropped_path, sizeof(hdrFilePath) - 1);
                            hdrFilePath[sizeof(hdrFilePath) - 1] = '\0';
                            strncpy(inputPath, dropped_path, sizeof(inputPath) - 1);
                            inputPath[sizeof(inputPath) - 1] = '\0';
                            hdrLoaded = true;
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            if (ImGui::Button("Load HDR/EXR"))
            {
                if (strlen(inputPath) > 0) {
                    if (hdrTexture.loadHDR(inputPath)) {
                        strcpy(hdrFilePath, inputPath);
                        hdrLoaded = true;
                    }
                }
            }
            ImGui::Separator();
            ImGui::Checkbox("Use as Background", &useHdrBackground);
            ImGui::Checkbox("Use for Image-Based Lighting", &useHdrLighting);
            ImGui::DragFloat("Intensity", &hdrIntensity, 0.01f, 0.0f, 10.0f);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(380, 0), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowSize(ImVec2(134, 38), ImGuiCond_Always);
    ImGui::Begin("Overlays", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);
    ImGui::Checkbox("hide Overlays", &altRenderMode);
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(-1, winHeight - 45), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowSize(ImVec2(58, 58), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("Help", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
    helpButtonPos = ImGui::GetWindowPos();
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("?", ImVec2(24, 24))) {
        showHelpPopup = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar();

    if (showHelpPopup) {
        ImGui::SetNextWindowPos(ImVec2(helpButtonPos.x + 32, helpButtonPos.y - 420));
        ImGui::SetNextWindowBgAlpha(0.5f);
        if (ImGui::Begin("Key Bindings", &showHelpPopup, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar)) {
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Transform Operations:");
            ImGui::BulletText("Shift + D\nDuplicates selected shapes with ' Cp' suffix");
            ImGui::BulletText("X\nWithout transform: Removes selected shapes\nWith transform: Constrains to X-axis");
            ImGui::BulletText("C\nCenters view on last selected shape");
            ImGui::BulletText("G\nActivates translation mode");
            ImGui::BulletText("R\nActivates rotation mode");
            ImGui::BulletText("S\nActivates scale mode");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Constraints:");
            ImGui::BulletText("X/Y/Z (during transform)\nConstrains to respective axis");
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Interface:");
            ImGui::BulletText("Shift + A\nOpens Add Shape popup at cursor");
            ImGui::BulletText("H\nToggles GUI visibility");
            ImGui::BulletText("ESC\nCancels current transformation");
            ImGui::BulletText("Shift + Click\nToggle multi-selection");
            if (ImGui::IsMouseClicked(0) && !ImGui::IsWindowHovered()) {
                showHelpPopup = false;
            }
            ImGui::End();
        }
    }
}
