#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <sstream>
#include <filesystem>
namespace fs = std::filesystem;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

static std::ostringstream console;
static std::streambuf* stdcout = nullptr;
static std::streambuf* stdcerr = nullptr;
int main(int, char**)
{
    static std::vector<std::string> versionids;
    static int currentversion = 0;
    static bool versionsloaded = false;

    stdcout = std::cout.rdbuf(console.rdbuf());
    stdcerr = std::cerr.rdbuf(console.rdbuf());
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* mainwindow = glfwCreateWindow(900, 800, "ccapi launcher", nullptr, nullptr);
    if (mainwindow == nullptr) return 1;
    glfwMakeContextCurrent(mainwindow);

    int icon_width, icon_height, icon_channels;
    unsigned char* icon_pixels = stbi_load("gfx/ccapi.png", &icon_width, &icon_height, &icon_channels, 4);
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

    GLuint mainbg = 0;
    int mainbgwidth = 0;
    int mainbgheight = 0;
    const char* mainbgpath = "gfx/main.png";
    mainbg = cclauncher::LoadBackground(mainbgpath, &mainbgwidth, &mainbgheight);
    if (mainbg == 0)
    {
        std::cout << "Failed to load background: " << mainbgpath << "\n";
    }
    else
    {
        std::cout << "Loaded background: " << mainbgpath << "\n";
    }

    GLuint playbuttonnormal = 0;
    GLuint playbuttonhover  = 0;
    GLuint playbuttonactive = 0;
    int width, height;
    playbuttonnormal = cclauncher::LoadBackground("gfx/buttonnormal.png", &width, &height);
    playbuttonhover  = cclauncher::LoadBackground("gfx/buttonhover.png",  &width, &height);
    playbuttonactive = cclauncher::LoadBackground("gfx/buttonpressed.png", &width, &height);

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

    ImFont* normalfont = io.Fonts->AddFontFromFileTTF("fonts/arial.ttf", 19.0f);
    ImFont* consolefont = io.Fonts->AddFontFromFileTTF("fonts/arial.ttf", 16.0f);
    IM_ASSERT(consolefont != nullptr);

    char buf[64] = "";
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

        ImDrawList* drawlist = ImGui::GetBackgroundDrawList();
        ImVec2 screenpos = ImGui::GetMainViewport()->Pos;
        ImVec2 screensize = ImGui::GetMainViewport()->Size;
        drawlist->AddImage((ImTextureID)(intptr_t)mainbg, screenpos, ImVec2(screenpos.x + screensize.x, screenpos.y + screensize.y));

        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(900, 800), ImGuiCond_Once);
        ImGuiWindowFlags main_window_flags = ImGuiWindowFlags_NoResize
                              | ImGuiWindowFlags_NoSavedSettings
                              | ImGuiWindowFlags_NoDocking
                              | ImGuiWindowFlags_NoMove
                              | ImGuiWindowFlags_NoCollapse
                              | ImGuiWindowFlags_NoTitleBar
                              | ImGuiWindowFlags_NoBackground
                              | ImGuiWindowFlags_NoScrollbar;
        {
            ImGui::PushFont(normalfont);
            ImGui::Begin("cclauncher", nullptr, main_window_flags);

            // - inputtext
            ImGui::SetCursorPos(ImVec2(635, 520));

            ImGui::PushItemWidth(200);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.22745f, 0.21569f, 0.21176f, 1.0f));  // normal

            ImGui::InputText("##username", buf, 64);
            ImVec2 inputmin = ImGui::GetItemRectMin();
            ImVec2 inputmax = ImGui::GetItemRectMax();
            bool inputhovered = ImGui::IsItemHovered();
            bool inputactive  = ImGui::IsItemActive();

            ImGui::PopStyleColor();
            ImGui::PopItemWidth();

            static ImVec4 inputoutlinecurrent(0.235f, 0.522f, 0.153f, 1.0f); 
            ImVec4 inputoutlinetarget;
            if (inputactive || inputhovered)
                inputoutlinetarget = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            else
                inputoutlinetarget = ImVec4(0.235f, 0.522f, 0.153f, 1.0f);

            float inputfadespeed = 10.0f;
            float inputdt = ImGui::GetIO().DeltaTime;

            inputoutlinecurrent.x += (inputoutlinetarget.x - inputoutlinecurrent.x) * inputfadespeed * inputdt;
            inputoutlinecurrent.y += (inputoutlinetarget.y - inputoutlinecurrent.y) * inputfadespeed * inputdt;
            inputoutlinecurrent.z += (inputoutlinetarget.z - inputoutlinecurrent.z) * inputfadespeed * inputdt;
            inputoutlinecurrent.w += (inputoutlinetarget.w - inputoutlinecurrent.w) * inputfadespeed * inputdt;
            ImGui::GetWindowDrawList()->AddRect(inputmin, inputmax, ImColor(inputoutlinecurrent), 0.0f, ImDrawFlags_RoundCornersAll, 1.0f);
            // - end inputtext

            // - combobox
            ImGui::SetCursorPos(ImVec2(70, 520));

            if (!versionsloaded)
            {
                std::string manifestjson = ccapi::DownloadVersionManifest();
                if (!manifestjson.empty())
                {
                    auto ids = ccapi::GetVersionsManifest(manifestjson);
                    if (ids && !ids->empty())
                    {
                        versionids = std::move(*ids);
                        currentversion = 0;
                        versionsloaded = true;
                        std::cout << "Loaded " << versionids.size() << " versions from manifest.\n";
                    }
                    else
                    {
                        std::cout << "Failed to parse version manifest\n";
                    }
                }
            }

            ImGui::PushItemWidth(200);
            ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  ImVec4(0, 0, 0, 0));

            ImVec2 combopos = ImGui::GetCursorScreenPos();
            ImVec2 combosize(ImGui::CalcItemWidth(), ImGui::GetFrameHeight());

            ImGui::GetWindowDrawList()->AddRectFilled(combopos, ImVec2(combopos.x + combosize.x, combopos.y + combosize.y), ImColor(0.227f, 0.216f, 0.212f, 1.0f), 4.0f);

            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.228f, 0.216f, 0.212f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.318f, 0.302f, 0.298f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.228f, 0.216f, 0.212f, 1.0f));

            const char* preview =(!versionids.empty() && currentversion < versionids.size()) ? versionids[currentversion].c_str() : "Loading versions...";
            bool comboopened = ImGui::BeginCombo("##combo", preview, ImGuiComboFlags_NoArrowButton);
            if (comboopened)
            {
                for (int i = 0; i < versionids.size(); i++)
                {
                    bool comboselected = (currentversion == i);

                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));

                    if (ImGui::Selectable(versionids[i].c_str(), comboselected))
                        currentversion = i;

                    ImGui::PopStyleColor();

                    if (comboselected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::PopStyleColor(3);

            bool combohovered = ImGui::IsItemHovered();
            bool comboactive  = ImGui::IsItemActive() || comboopened;
            ImVec2 combomin = ImGui::GetItemRectMin();
            ImVec2 combomax = ImGui::GetItemRectMax();

            ImGui::PopStyleColor(3);
            ImGui::PopItemWidth();

            static ImVec4 combooutlinecurrent(0.235f, 0.522f, 0.153f, 1.0f);
            ImVec4 combooutlinetarget = (combohovered || comboactive) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.235f, 0.522f, 0.153f, 1.0f);

            float combofadespeed = 10.0f;
            float combodt = ImGui::GetIO().DeltaTime;

            combooutlinecurrent.x += (combooutlinetarget.x - combooutlinecurrent.x) * combofadespeed * combodt;
            combooutlinecurrent.y += (combooutlinetarget.y - combooutlinecurrent.y) * combofadespeed * combodt;
            combooutlinecurrent.z += (combooutlinetarget.z - combooutlinecurrent.z) * combofadespeed * combodt;
            combooutlinecurrent.w += (combooutlinetarget.w - combooutlinecurrent.w) * combofadespeed * combodt;
            ImGui::GetWindowDrawList()->AddRect(combomin, combomax, ImColor(combooutlinecurrent), 0.0f, ImDrawFlags_RoundCornersAll, 1.0f);
            // - end combobox

            // - play button
            ImGui::SetCursorPos(ImVec2(331, 495));
            ImGui::InvisibleButton("playbutton", ImVec2(238, 54));
            bool playbuttonhovered = ImGui::IsItemHovered();
            bool playbuttonheld = ImGui::IsItemActive();
            bool playbuttonclicked = ImGui::IsItemClicked();
            GLuint playbuttontex = playbuttonnormal;
            if (playbuttonheld)
            {
                playbuttontex = playbuttonactive;
            }
            else if (playbuttonhovered)
            {
                playbuttontex = playbuttonhover;
            }
            if (playbuttonclicked)
            {
                std::cout << "test\n";
            }

            ImVec2 playbuttonmin = ImGui::GetItemRectMin();
            ImVec2 playbuttonmax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)playbuttontex, playbuttonmin, playbuttonmax);
            // - end play button

            ImGui::End();
            ImGui::PopFont();
        }

        ImGui::SetNextWindowPos(ImVec2(225, 605), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(450, 180), ImGuiCond_Once);
        ImGuiWindowFlags console_window_flags = ImGuiWindowFlags_NoResize
                              | ImGuiWindowFlags_NoSavedSettings
                              | ImGuiWindowFlags_NoDocking
                              | ImGuiWindowFlags_NoMove
                              | ImGuiWindowFlags_NoCollapse
                              | ImGuiWindowFlags_NoTitleBar;
        {
            ImGui::PushFont(consolefont);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.227f, 0.216f, 0.212f, 1.0f));
            ImGui::Begin("console", nullptr, console_window_flags);

            ImGui::BeginChild("consolescroll", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(console.str().c_str());
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();

            ImGui::End();
            ImGui::PopStyleColor();
            ImGui::PopFont();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(mainwindow, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(mainwindow);
    }
    std::cout.rdbuf(stdcout);
    std::cerr.rdbuf(stdcerr);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(mainwindow);
    glfwTerminate();
    return 0;
}
