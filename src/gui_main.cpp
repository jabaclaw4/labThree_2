#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/3/glfw3.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "BinaryTree.h"
#include "TreeOperations.h"
#include "TraverseOrder.h"

using IntTree = BinaryTree<int, int>;

static IntTree tree;
static std::map<IntTree::Node*, ImVec2> nodePositions;
static bool treeChanged = true;
static std::string serializeResult = "";
static std::string traversalResult = "";
static int newNodeKey = 10;
static int newNodeVal = 100;
static int deleteKey = 10;
static int reduceResult = 0;

// Расчет позиций узлов для отрисовки
void CalcNodePos(IntTree::Node* node, int depth, int& xCounter) {
    if (!node) return;
    CalcNodePos(node->left, depth + 1, xCounter);
    nodePositions[node] = ImVec2(xCounter * 60.0f, depth * 80.0f);
    xCounter++;
    CalcNodePos(node->right, depth + 1, xCounter);
}

void UpdateLayout() {
    nodePositions.clear();
    int xCounter = 0;
    CalcNodePos(tree.GetRoot(), 0, xCounter);
    treeChanged = false;
}

int main(int, char**) {
    if (!glfwInit()) return -1;
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 800, "Lab 3 - Binary Tree GUI", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    float zoom = 1.0f;
    ImVec2 panOffset(50, 50);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (treeChanged) UpdateLayout();

        // --- Левая панель управления ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(350, 800));
        ImGui::Begin("Operations");

        // Вставка
        ImGui::Text("Insert Node");
        ImGui::InputInt("Key##1", &newNodeKey);
        ImGui::InputInt("Value##1", &newNodeVal);
        if (ImGui::Button("Add", ImVec2(-1, 25))) {
            try { tree.Add(newNodeKey, newNodeVal); treeChanged = true; }
            catch (...) { ImGui::OpenPopup("Error"); }
        }

        // Удаление
        ImGui::Separator();
        ImGui::Text("Remove Node");
        ImGui::InputInt("Key##2", &deleteKey);
        if (ImGui::Button("Remove", ImVec2(-1, 25))) {
            try { tree.Remove(deleteKey); treeChanged = true; }
            catch (...) { ImGui::OpenPopup("Error"); }
        }
        if (ImGui::Button("Clear All", ImVec2(-1, 25))) {
            tree = IntTree(); treeChanged = true; traversalResult = ""; serializeResult = "";
        }

        // Обходы
        ImGui::Separator();
        ImGui::Text("Traversals (Visitor)");
        if (ImGui::Button("PreOrder (KLP)", ImVec2(-1, 25))) {
            traversalResult = "";
            tree.Traverse(TraverseOrder::PreOrder, [&](int k, int) { traversalResult += std::to_string(k) + " "; });
        }
        if (ImGui::Button("InOrder (LKP)", ImVec2(-1, 25))) {
            traversalResult = "";
            tree.Traverse(TraverseOrder::InOrder, [&](int k, int) { traversalResult += std::to_string(k) + " "; });
        }
        if (ImGui::Button("PostOrder (LPK)", ImVec2(-1, 25))) {
            traversalResult = "";
            tree.Traverse(TraverseOrder::PostOrder, [&](int k, int) { traversalResult += std::to_string(k) + " "; });
        }
        ImGui::TextWrapped("Result: %s", traversalResult.c_str());

        // Функции (map, where, reduce)
        ImGui::Separator();
        ImGui::Text("Lab 3 Functions");
        if (ImGui::Button("Map (Key = 2x+1)", ImVec2(-1, 25))) {
            tree = Map<int, int>(tree, [](const int& k) { return 2 * k + 1; });
            treeChanged = true;
        }
        static int whereThreshold = 50;
        ImGui::InputInt("Where key >=", &whereThreshold);
        if (ImGui::Button("Apply Where", ImVec2(-1, 25))) {
            tree = Where<int, int>(tree, [whereThreshold](const int& k, const int&) { return k >= whereThreshold; });
            treeChanged = true;
        }
        if (ImGui::Button("Reduce (Sum keys)", ImVec2(-1, 25))) {
            reduceResult = Reduce<int, int, int>(tree, [](const int& acc, const int& k, const int&) { return acc + k; }, 0);
        }
        ImGui::Text("Sum of keys: %d", reduceResult);

        // Сериализация
        ImGui::Separator();
        ImGui::Text("Serialization");
        if (ImGui::Button("To String {K}(L)[P]", ImVec2(-1, 25))) {
            serializeResult = tree.ToStringByFormat("{К}(Л)[П]");
        }
        ImGui::TextWrapped("Format: %s", serializeResult.c_str());

        ImGui::End();

        // --- Панель рисования ---
        ImGui::SetNextWindowPos(ImVec2(350, 0));
        ImGui::SetNextWindowSize(ImVec2(930, 800));
        ImGui::Begin("Tree Canvas (Scroll to Zoom, Drag to Pan)");

        if (tree.GetCount() > 0) {
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            if (ImGui::IsWindowHovered()) {
                zoom += io.MouseWheel * 0.1f;
                if (zoom < 0.2f) zoom = 0.2f;
                if (zoom > 3.0f) zoom = 3.0f;
                if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    panOffset.x += io.MouseDelta.x;
                    panOffset.y += io.MouseDelta.y;
                }
            }

            ImVec2 origin = ImGui::GetCursorScreenPos();
            ImVec2 zoomedOffset = ImVec2(origin.x + panOffset.x, origin.y + panOffset.y);

            std::function<void(IntTree::Node*)> drawZoom = [&](IntTree::Node* node) {
                if (!node) return;
                ImVec2 pos = ImVec2(zoomedOffset.x + nodePositions[node].x * zoom, zoomedOffset.y + nodePositions[node].y * zoom);
                float radius = 25.0f * zoom;

                if (node->left) {
                    ImVec2 lPos = ImVec2(zoomedOffset.x + nodePositions[node->left].x * zoom, zoomedOffset.y + nodePositions[node->left].y * zoom);
                    drawList->AddLine(ImVec2(pos.x + radius, pos.y + radius), ImVec2(lPos.x + radius, lPos.y + radius), IM_COL32(255, 255, 255, 255), 2.0f * zoom);
                    drawZoom(node->left);
                }
                if (node->right) {
                    ImVec2 rPos = ImVec2(zoomedOffset.x + nodePositions[node->right].x * zoom, zoomedOffset.y + nodePositions[node->right].y * zoom);
                    drawList->AddLine(ImVec2(pos.x + radius, pos.y + radius), ImVec2(rPos.x + radius, rPos.y + radius), IM_COL32(255, 255, 255, 255), 2.0f * zoom);
                    drawZoom(node->right);
                }

                drawList->AddCircleFilled(ImVec2(pos.x + radius, pos.y + radius), radius, IM_COL32(50, 150, 255, 255));
                drawList->AddCircle(ImVec2(pos.x + radius, pos.y + radius), radius, IM_COL32(255, 255, 255, 255), 0, 2.0f * zoom);

                std::string s = std::to_string(node->key);
                ImVec2 textSize = ImGui::CalcTextSize(s.c_str());
                drawList->AddText(ImVec2(pos.x + radius - (textSize.x * zoom) / 2, pos.y + radius - (textSize.y * zoom) / 2), IM_COL32(255, 255, 255, 255), s.c_str());
            };

            drawZoom(tree.GetRoot());
        } else {
            ImGui::Text("Tree is empty. Add nodes from the left panel.");
        }

        ImGui::End();

        // Рендер
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}