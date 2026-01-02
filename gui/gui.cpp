#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <filesystem>
namespace fs = std::filesystem;

#include "../api/api.hpp"

namespace cclauncher
{

    GLuint LoadBackground(const char* filename, int* out_width, int* out_height)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
        if (!data)
            return 0;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            data
        );

        stbi_image_free(data);

        *out_width = width;
        *out_height = height;
        return texture;
    }

}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* mainwindow = glfwCreateWindow(700, 500, "cclauncher", nullptr, nullptr);
    if (mainwindow == nullptr) return 1;
    glfwMakeContextCurrent(mainwindow);

    int icon_width, icon_height, icon_channels;
    unsigned char* icon_pixels = stbi_load("gfx/clilauncher.png", &icon_width, &icon_height, &icon_channels, 4);
    if (icon_pixels)
    {
        GLFWimage images[1];
        images[0].width = icon_width;
        images[0].height = icon_height;
        images[0].pixels = icon_pixels;
        glfwSetWindowIcon(mainwindow, 1, images);
        stbi_image_free(icon_pixels);
    }
    glfwSwapInterval(1);

    GLuint bg_day = 0;
    int bg_day_width = 0;
    int bg_day_height = 0;

    const char* bg_day_path = "gfx/bg_day.png";
    bg_day = cclauncher::LoadBackground(bg_day_path, &bg_day_width, &bg_day_height);
    if (bg_day == 0)
    {
        std::cout << "Failed to load background: " << bg_day_path << "\n";
    }
    std::cout << "Loaded background: " << bg_day_path << "\n";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(mainwindow, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    style.FontSizeBase = 16.0f;
    ImFont* font = io.Fonts->AddFontFromFileTTF("fonts/arial.ttf");
    IM_ASSERT(font != nullptr);

    while (!glfwWindowShouldClose(mainwindow))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(mainwindow, GLFW_ICONIFIED) != 0)
        {
            ImGui_ImplGlfw_Sleep(10);
            continue;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // - launcher gui
        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        ImVec2 screen_pos = ImGui::GetMainViewport()->Pos;
        ImVec2 screen_size = ImGui::GetMainViewport()->Size;

        draw_list->AddImage(
            (ImTextureID)(intptr_t)bg_day,
            screen_pos,
            ImVec2(screen_pos.x + screen_size.x, screen_pos.y + screen_size.y)
        );

        ImGui::SetNextWindowPos(ImVec2(150, 50), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Once);
        ImGuiWindowFlags main_window_flags = ImGuiWindowFlags_NoResize
                              | ImGuiWindowFlags_NoSavedSettings
                              | ImGuiWindowFlags_NoDocking
                              | ImGuiWindowFlags_NoMove
                              | ImGuiWindowFlags_NoCollapse
                              | ImGuiWindowFlags_NoSavedSettings;
        {
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.0f, 0.5451f, 0.2706f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.0f, 0.5451f, 0.2706f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.0f, 0.5451f, 0.2706f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
            ImGui::Begin("cclauncher", nullptr, main_window_flags);

            ImGui::SetCursorPos(ImVec2(150, 50));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5451f, 0.2706f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.6039f, 0.2980f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0118f, 0.4784f, 0.2431f, 1.0f));
            ImGui::Button("test", ImVec2(100, 50));
            ImGui::PopStyleColor(3);

            ImGui::End();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
        }
        // - end launcher gui

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(mainwindow, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(mainwindow);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(mainwindow);
    glfwTerminate();
    return 0;
}
