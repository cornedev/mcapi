#pragma once

#include <string>
#include <vector>
#include <optional>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <filesystem>
#include <functional>
namespace fs = std::filesystem;
#include <archive.h>
#include <archive_entry.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace ccapi
{
    enum class GETmode
    {
        MemoryOnly,
        DiskOnly,
        MemoryAndDisk
    };

    enum class OS
    {
        Windows,
        Linux,
        Macos
    };

    enum class Arch
    {
        x64,
        x32,
        arm64
    };

    #ifdef _WIN32
    using Processhandle = HANDLE;
    #else
    using Processhandle = pid_t;
    #endif

    std::optional<std::string> GET(const std::wstring& url, GETmode mode = GETmode::MemoryOnly, const std::string& filename = "", const std::string& folder = "");
    
    std::string DownloadVersionManifest();
    std::optional<std::vector<std::string>> GetVersionsFromManifest(const std::string& manifestjson);
    std::optional<std::string> GetVersionJsonDownloadUrl(const std::string& manifestjson, const std::string& versionid);
    std::optional<std::string> DownloadVersionJson(const std::string& jsonurl, const std::string& versionid);
    std::optional<std::string> GetClientJarDownloadUrl(const std::string& versionjson);
    std::optional<std::string> DownloadClientJar(const std::string& clienturl, const std::string& versionid);
    std::optional<std::string> GetAssetIndexJsonDownloadUrl(const std::string& versionjson);
    std::optional<std::string> DownloadAssetIndexJson(const std::string& indexurl, const std::string& versionid);
    std::optional<std::vector<std::pair<std::string, std::string>>> GetLibrariesDownloadUrl(const std::string& versionjson, OS os);
    std::optional<std::vector<std::string>> DownloadLibraries(const std::vector<std::pair<std::string, std::string>>& libraries, const std::string& versionid);
    std::optional<std::vector<std::pair<std::string, std::string>>> GetAssetsDownloadUrl(const std::string& assetindexjson);
    std::optional<std::vector<std::string>> DownloadAssets(const std::vector<std::pair<std::string, std::string>>& assets, const std::string& versionid);
    std::optional<std::vector<std::pair<std::string, std::string>>> GetLibrariesNatives(const std::string& versionid, const std::string& versionjson, OS os, Arch arch);
    std::optional<std::vector<std::string>> DownloadLibrariesNatives(const std::vector<std::pair<std::string, std::string>>& natives, const std::string& versionid);
    std::optional<std::vector<std::string>> ExtractLibrariesNatives(const std::vector<std::string>& nativesjars, const std::string& versionid, OS os);
    std::optional<std::string> GetClassPath(const std::string& versionjson, const std::vector<std::string>& libraries, const std::string& clientjarpath, OS os);
    std::optional<std::string> GetLaunchCommand(const std::string& username, const std::string& classpath, const std::string& versionjson, const std::string& versionid);
    std::optional<std::string> GetServerJarDownloadUrl(const std::string& versionjson);
    std::optional<std::string> DownloadServerJar(const std::string& serverurl, const std::string& versionid);

    std::optional<int> GetJavaVersion(const std::string& versionjson);
    std::optional<std::string> GetJavaDownloadUrl(int javaversion, OS os, Arch arch);
    std::optional<std::string> DownloadJava(const std::string& javaurl, const std::string& versionid);

    bool StartProcess(const std::string& javapath, const std::string& args, OS os, Processhandle* process);
    bool StopProcess(Processhandle* process);
}


