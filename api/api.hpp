#pragma once

#include <string>
#include <vector>
#include <optional>

#include <windows.h>
#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <filesystem>
#include <functional>
namespace fs = std::filesystem;
#include <zip.h>
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

    std::string DownloadVersionManifest();
    std::optional<std::string> GetVersionJsonDownloadUrl(const std::string& manifestjson, const std::string& versionid);
    std::optional<std::string> DownloadVersionJson(const std::string& jsonurl, const std::string& versionid);
    std::optional<std::string> GetClientJarDownloadUrl(const std::string& versionjson);
    std::optional<std::string> DownloadClientJar(const std::string& clienturl, const std::string& versionid);
    std::optional<std::string> GetAssetIndexJsonDownloadUrl(const std::string& versionjson);
    std::optional<std::string> DownloadAssetIndexJson(const std::string& indexurl);
    std::optional<std::vector<std::pair<std::string, std::string>>> GetLibrariesDownloadUrl(const std::string& versionjson);
    std::optional<std::vector<std::string>> DownloadLibraries(const std::vector<std::pair<std::string, std::string>>& libraries);
    std::optional<std::vector<std::pair<std::string, std::string>>> GetAssetsDownloadUrl(const std::string& assetindexjson);
    std::optional<std::vector<std::string>> DownloadAssets(const std::vector<std::pair<std::string, std::string>>& assets);
    std::optional<std::vector<std::pair<std::string, std::string>>> GetLibrariesNatives(const std::string& versionjson);
    std::optional<std::vector<std::string>> ExtractLibrariesNatives(const std::vector<std::pair<std::string, std::string>>& natives);
    std::optional<std::string> GetClassPath(const std::string& versionjson, const std::vector<std::string>& librarypaths, const std::string& versionid);
    std::optional<std::string> GetLaunchCommand(const std::string& username, const std::string& classpath, const std::string& versionjson, const std::string& versionid);
    std::optional<std::string> GetServerJarDownloadUrl(const std::string& versionjson);
    std::optional<std::string> DownloadServerJar(const std::string& serverurl, const std::string& versionid);

    bool StartProcess(const std::string& javapath, const std::string& args);
}