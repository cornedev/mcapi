#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>
namespace fs = std::filesystem;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "../api/api.hpp"

static std::vector<std::string> console;
static std::mutex consolemutex;
static std::atomic<bool> launching = false;
static std::thread launchthread;
static std::atomic<bool> running = false;
static ccapi::Processhandle process{};

namespace cclauncher
{

    GLuint LoadTexture(const char* filename, int* outwidth, int* outheight)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filename, &width, &height, &channels, 4);
        if (!data)
            return 0;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);

        *outwidth = width;
        *outheight = height;
        return texture;
    }

    void Log(const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(consolemutex);
        console.push_back(msg);
    }

    void LaunchMinecraft(std::string username, std::string versionid, std::string manifestjson, std::string arch, std::string os)
    {
        // download version json.
        auto versionjsonurl = ccapi::GetVersionJsonDownloadUrl(manifestjson, versionid);
        if (!versionjsonurl)
        {
            Log("Version not found in manifest.\n");
            return;
        }
        auto versionurl = *versionjsonurl;

        Log("Downloading version json...\n");
        auto versionjsonopt = ccapi::DownloadVersionJson(versionurl, versionid);
        if (!versionjsonopt)
        {
            Log("Failed to download version json.\n");
            return;
        }
        Log("Version json downloaded.\n");
        auto versionjson = *versionjsonopt;

        // download client jar.
        auto jarurlopt = ccapi::GetClientJarDownloadUrl(versionjson);
        if (!jarurlopt)
        {
            Log("Failed to get client jar url.\n");
            return;
        }
        auto jarurl = *jarurlopt;

        Log("Downloading client jar...\n");
        auto clientjar = ccapi::DownloadClientJar(jarurl, versionid);
        if (!clientjar)
        {
            Log("Failed to download client jar.\n");
            return;
        }
        Log("Client jar downloaded.\n");

        // download asset index.
        auto indexurlopt = ccapi::GetAssetIndexJsonDownloadUrl(versionjson);
        if (!indexurlopt)
        {
            Log("Failed to get asset index URL.\n");
            return;
        }
        auto indexurl = *indexurlopt;

        Log("Downloading Asset index...\n");
        auto assetjsonopt = ccapi::DownloadAssetIndexJson(indexurl, versionid);
        if (!assetjsonopt)
        {
            Log("Failed to download asset index.\n");
            return;
        }
        auto assetjson = *assetjsonopt;
        Log("Asset index downloaded.\n");

        // download assets.
        auto assetsurlopt = ccapi::GetAssetsDownloadUrl(assetjson);
        if (!assetsurlopt)
        {
            Log("Failed to get assets download urls.\n");
            return;
        }

        Log("Downloading assets... (this may take a while)\n");
        auto assets = ccapi::DownloadAssets(*assetsurlopt, versionid);
        if (!assets)
        {
            Log("Failed to download assets.\n");
            return;
        }
        Log("Assets downloaded.\n");

        // conversion.
        ccapi::OS osenum;
        if (os == "windows")
            osenum = ccapi::OS::Windows;
        else if (os == "linux")
            osenum = ccapi::OS::Linux;
        else if (os == "macos")
            osenum = ccapi::OS::Macos;
        else
        {
            Log("Invalid OS.\n");
            return;
        }
        ccapi::Arch archenum;
        if (arch == "x64")
            archenum = ccapi::Arch::x64;
        else if (arch == "x32")
            archenum = ccapi::Arch::x32;
        else if (arch == "arm64")
            archenum = ccapi::Arch::arm64;
        else
        {
            Log("Invalid architecture.\n");
            return;
        }

        // download java.
        auto javaversionopt = ccapi::GetJavaVersion(versionjson);
        if (!javaversionopt)
        {
            Log("Failed to get java version.\n");
            return;
        }
        int javaversion = *javaversionopt;

        auto javaurlopt = ccapi::GetJavaDownloadUrl(javaversion, osenum, archenum);
        if (!javaurlopt)
        {
            Log("Failed to get java download url.\n");
            return;
        }
        auto javaurl = *javaurlopt;

        Log("Downloading java...\n");
        auto javaopt = ccapi::DownloadJava(javaurl, versionid);
        if (!javaopt)
        {
            Log("Failed to download java.\n");
            return;
        }
        Log("Java downloaded.\n");
        auto java = *javaopt;

        // download libraries.
        auto librariesurlopt = ccapi::GetLibrariesDownloadUrl(versionjson, osenum);
        if (!librariesurlopt)
        {
            Log("Failed to get libraries.\n");
            return;
        }

        Log("Downloading libraries...\n");
        auto librariesdownloaded = ccapi::DownloadLibraries(*librariesurlopt, versionid);
        if (!librariesdownloaded)
        {
            Log("Failed to download libraries.\n");
            return;
        }
        Log("Libraries downloaded.\n");

        // extract natives.
        Log("Extracting natives...\n");
        auto nativesurlopt = ccapi::GetLibrariesNatives(versionid, versionjson, osenum, archenum);
        if (!nativesurlopt)
        {
            Log("Failed to get natives urls.\n");
            return;
        }

        Log("Downloading natives...\n");
        auto nativesjarsopt = ccapi::DownloadLibrariesNatives(*nativesurlopt, versionid);
        if (!nativesjarsopt)
        {
            Log("Failed to download native jars.\n");
            return;
        }

        auto nativesextracted = ccapi::ExtractLibrariesNatives(*nativesjarsopt, versionid, osenum);
        if (!nativesextracted)
        {
            Log("Failed to extract natives.\n");
            return;
        }
        Log("Natives extracted.\n");

        // build classpath.
        Log("Building classpath...\n");
        fs::path datapath = ".ccapi";
        auto classpathopt = ccapi::GetClassPath(versionjson, *librariesdownloaded, (datapath / "versions" / versionid / "client.jar").string(), osenum);
        if (!classpathopt)
        {
            Log("Failed to build classpath.\n");
            return;
        }
        auto classpath = *classpathopt;
        Log("Classpath built.\n");

        // build launch command.
        Log("Building launch command...\n");
        auto launchcmdopt = ccapi::GetLaunchCommand(username, classpath, versionjson, versionid, osenum);
        if (!launchcmdopt)
        {
            Log("Failed to build launch command.\n");
            return;
        }
        auto launchcmd = *launchcmdopt;
        Log("Launch command built.\n");

        // launch minecraft.
        std::string javapath;
        switch (osenum)
        {
            case ccapi::OS::Windows:
                javapath = "runtime/" + versionid + "/java/bin/java.exe";
                break;

            case ccapi::OS::Linux:
            case ccapi::OS::Macos:
                javapath = "runtime/" + versionid + "/java/bin/java";
                break;
        }
        bool launched = ccapi::StartProcess(javapath, launchcmd, osenum, &process);
        if (!launched)
        {
            Log("Failed to launch Minecraft.\n");
        }
        else
        {
            running = true;
            Log("Minecraft launched.\n");
        }
    }

    bool ComboBox(const char* id, int* indexcurrent, const char* const* items, int indexcount, bool heightsmall = true)
    {
        ImGui::PushItemWidth(200);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));

        ImVec2 pos  = ImGui::GetCursorScreenPos();
        ImVec2 size(ImGui::CalcItemWidth(), ImGui::GetFrameHeight());
        ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), ImColor(0.227f, 0.216f, 0.212f, 1.0f), 4.0f);

        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.228f, 0.216f, 0.212f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.318f, 0.302f, 0.298f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.228f, 0.216f, 0.212f, 1.0f));

        bool changed = false;
        ImGuiComboFlags flags = ImGuiComboFlags_NoArrowButton;
        if (heightsmall)
        {
            flags |= ImGuiComboFlags_HeightSmall;
        }
        bool opened = ImGui::BeginCombo(id, items[*indexcurrent], flags);
        if (opened)
        {
            for (int i = 0; i < indexcount; ++i)
            {
                bool selected = (*indexcurrent == i);

                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
                if (ImGui::Selectable(items[i], selected))
                {
                    *indexcurrent = i;
                    changed = true;
                }
                ImGui::PopStyleColor();

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopStyleColor(3);

        bool hovered = ImGui::IsItemHovered();
        bool active = ImGui::IsItemActive() || opened;

        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();

        ImGui::PopStyleColor(3);
        ImGui::PopItemWidth();

        ImVec4 outlinecolor =(hovered || active) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.235f, 0.522f, 0.153f, 1.0f);
        ImGui::GetWindowDrawList()->AddRect(min, max, ImColor(outlinecolor), 0.0f, ImDrawFlags_RoundCornersAll, 1.0f);
        return changed;
    }
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}
int main(int, char**)
{
    static std::vector<std::string> versionids;
    static std::vector<const char*> versionitems;
    static int currentversion = 0;

    static const char* archoptions[] = { "x64", "x32", "arm64" };
    static int currentarch = 0;

    static const char* osoptions[] = { "windows", "linux", "macos" };
    static int currentos = 0;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* mainwindow = glfwCreateWindow(900, 900, "ccapi_gui_launcher", nullptr, nullptr);
    if (mainwindow == nullptr) return 1;
    glfwMakeContextCurrent(mainwindow);

    int icon_width, icon_height, icon_channels;
    unsigned char* icon_pixels = stbi_load("gfx/ccapi_gui.png", &icon_width, &icon_height, &icon_channels, 4);
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
    int mainbgwidth, mainbgheight;
    const char* mainbgpath = "gfx/main.png";
    mainbg = cclauncher::LoadTexture(mainbgpath, &mainbgwidth, &mainbgheight);
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
    const char* playbuttonnormalpath = "gfx/playbuttonnormal.png";
    const char* playbuttonhoverpath = "gfx/playbuttonhover.png";
    const char* playbuttonpressedpath = "gfx/playbuttonpressed.png";
    int playbuttonwidth, playbuttonheight;
    playbuttonnormal = cclauncher::LoadTexture(playbuttonnormalpath, &playbuttonwidth, &playbuttonheight);
    playbuttonhover = cclauncher::LoadTexture(playbuttonhoverpath,  &playbuttonwidth, &playbuttonheight);
    playbuttonactive = cclauncher::LoadTexture(playbuttonpressedpath, &playbuttonwidth, &playbuttonheight);
    if (playbuttonnormal && playbuttonhover && playbuttonactive == 0)
    {
        std::cout << "Failed to load texture: " << playbuttonnormalpath << " " << playbuttonhoverpath << " " << playbuttonpressedpath << "\n";
    }
    else
    {
        std::cout << "Loaded texture: " << playbuttonnormalpath << " " << playbuttonhoverpath << " " << playbuttonpressedpath << "\n";
    }

    GLuint stopbuttonnormal = 0;
    GLuint stopbuttonhover  = 0;
    GLuint stopbuttonactive = 0;
    int stopbuttonwidth, stopbuttonheight;
    stopbuttonnormal = cclauncher::LoadTexture("gfx/stopbuttonnormal.png", &stopbuttonwidth, &stopbuttonheight);
    stopbuttonhover = cclauncher::LoadTexture("gfx/stopbuttonhover.png",  &stopbuttonwidth, &stopbuttonheight);
    stopbuttonactive = cclauncher::LoadTexture("gfx/stopbuttonpressed.png", &stopbuttonwidth, &stopbuttonheight);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(mainwindow, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImFont* mainfont = io.Fonts->AddFontFromFileTTF("fonts/arial.ttf", 19.0f);
    ImFont* consolefont = io.Fonts->AddFontFromFileTTF("fonts/arial.ttf", 16.0f);
    IM_ASSERT(consolefont != nullptr);

    char buf[64] = "";
    std::string manifestjson;

    cclauncher::Log("Downloading version manifest...\n");
    manifestjson = ccapi::DownloadVersionManifest();
    cclauncher::Log("Version manifest downloaded.\n");
    if (!manifestjson.empty())
    {
        auto ids = ccapi::GetVersionsFromManifest(manifestjson);
        if (ids && !ids->empty())
        {
            versionids = std::move(*ids);
            cclauncher::Log(std::string("Loaded ") + std::to_string(versionids.size()) + " versions.\n");
        }
    }

    while (!glfwWindowShouldClose(mainwindow))
    {
        if (ImGui::GetIO().WantTextInput || ImGui::IsAnyItemActive())
            glfwPollEvents();
        else
            glfwWaitEventsTimeout(1.0 / 60.0);
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
                              | ImGuiWindowFlags_NoScrollbar
                              | ImGuiWindowFlags_NoBringToFrontOnFocus;
        {
            ImGui::PushFont(mainfont);
            ImGui::Begin("cclauncher", nullptr, main_window_flags);

            // - username inputtext.
            ImGui::SetCursorPos(ImVec2(635, 520));

            ImGui::PushItemWidth(200);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.22745f, 0.21569f, 0.21176f, 1.0f));

            ImGui::InputText("##username", buf, 64);

            ImGui::PopStyleColor();
            ImGui::PopItemWidth();

            ImVec4 outlinecolor =(ImGui::IsItemHovered() || ImGui::IsItemActive()) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.235f, 0.522f, 0.153f, 1.0f);
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(outlinecolor), 0.0f, ImDrawFlags_RoundCornersAll, 1.0f);
            // - end username inputtext.

            // - architecture select combobox.
            ImGui::SetCursorPos(ImVec2(15, 620));
            cclauncher::ComboBox("##archcombo", &currentarch, archoptions, IM_ARRAYSIZE(archoptions));
            // - end architecture select combobox.

            // - os select combobox.
            ImGui::SetCursorPos(ImVec2(15, 715));
            cclauncher::ComboBox("##oscombo", &currentos, osoptions, IM_ARRAYSIZE(osoptions));
            // - end os select combobox.

            // - version select combobox.
            ImGui::SetCursorPos(ImVec2(70, 520));
            versionitems.clear();
            for (const auto& v : versionids)
                versionitems.push_back(v.c_str());
            if (!versionitems.empty())
            {
                cclauncher::ComboBox("##versioncombo", &currentversion, versionitems.data(), static_cast<int>(versionitems.size()), false);
            }
            else
            {
                cclauncher::Log("Versions not yet loaded.");
            }
            // - end version select combobox.

            // - play button
            ImGui::SetCursorPos(ImVec2(331, 495));
            ImGui::InvisibleButton("playbutton", ImVec2(238, 54));
            GLuint playtex = playbuttonnormal;
            if (ImGui::IsItemActive())
            {
                playtex = playbuttonactive;
            }
            else if (ImGui::IsItemHovered())
            {
                playtex = playbuttonhover;
            }
            if (ImGui::IsItemClicked())
            {
                std::string username(buf);
                std::string arch = archoptions[currentarch];
                std::string os = osoptions[currentos];
                if (username.empty())
                {
                    cclauncher::Log("Please enter a username.\n");
                }
                else 
                {
                    if (launching == true)
                    {
                        cclauncher::Log("Minecraft already launching.");
                    }
                    else if (launching == false && !versionids.empty() && currentversion < static_cast<int>(versionids.size()))
                    {
                        std::string versionid = versionids[currentversion];

                        launching = true;
                        launchthread = std::thread([=]()
                        {
                            try
                            {
                                cclauncher::LaunchMinecraft(username, versionid, manifestjson, arch, os);
                            }
                            catch (const std::exception& e)
                            {
                                cclauncher::Log(std::string("Launcher crashed: ") + e.what());
                            }
                            catch (...)
                            {
                                cclauncher::Log("Unknown error.");
                            }
                            launching = false;
                        });
                        launchthread.detach();
                    }
                }
            }
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)playtex, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            // - end play button

            // - stop process button
            ImGui::SetCursorPos(ImVec2(575, 495));
            ImGui::InvisibleButton("stopbutton", ImVec2(54, 54));
            GLuint stoptex = stopbuttonnormal;
            if (ImGui::IsItemActive())
            {
                stoptex = stopbuttonactive;
            }
            else if (ImGui::IsItemHovered())
            {
                stoptex = stopbuttonhover;
            }
            if (ImGui::IsItemClicked())
            {
                if (!ccapi::StopProcess(&process))
                {
                    cclauncher::Log("Failed to stop Minecraft.\n");
                }
                else
                {
                    running = false;
                    cclauncher::Log("Minecraft stopped.\n");
                }
            }
            ImGui::GetWindowDrawList()->AddImage((ImTextureID)(intptr_t)stoptex, ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
            // - end stop process button

            ImGui::End();
            ImGui::PopFont();
        }

        ImGui::SetNextWindowPos(ImVec2(225, 605), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(450, 280), ImGuiCond_Once);
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

            static bool autoscroll = true;
            {
                std::lock_guard<std::mutex> lock(consolemutex);
                if (ImGui::GetScrollY() < ImGui::GetScrollMaxY())
                    autoscroll = false;
                else
                    autoscroll = true;
                for (const auto& line : console)
                {
                    ImGui::TextUnformatted(line.c_str());
                }
                if (autoscroll)
                    ImGui::SetScrollHereY(1.0f);
            }

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
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(mainwindow);
    glfwTerminate();
    return 0;
}
