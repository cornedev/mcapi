#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
typedef int socket_t;
#endif

#include <QDebug>
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <optional>
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

namespace mcapi
{
    inline fs::path datapath = ".mcapi";
    using argsmap = std::unordered_map<std::string, std::string>;

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

    std::optional<std::string> GET(const std::wstring& url, GETmode mode = GETmode::MemoryOnly, const std::string& filename = "", const std::string& folder = "", const std::vector<std::string>& headers = {});
    std::optional<std::string> POST(const std::wstring& url, const std::string& body, const std::vector<std::string>& headers = {});

    bool GetRuleAllow(const json& lib, OS os);
    std::string GetOSRuleName(OS os);
    
    namespace vanilla
    {
        std::optional<std::string> DownloadVersionManifest();
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
        std::optional<std::string> GetLaunchCommand(const std::string& username, const std::string& classpath, const std::string& versionjson, const std::string& versionid, OS os, const std::string& uuid = "00000000-0000-0000-0000-000000000000", const std::string& accesstoken = "0", const std::string& usertype = "mojang");
        std::optional<std::string> GetServerJarDownloadUrl(const std::string& versionjson);
        std::optional<std::string> DownloadServerJar(const std::string& serverurl, const std::string& versionid);
    }

    namespace fabric
    {
        std::optional<std::string> DownloadVersionMeta();
        std::optional<std::vector<std::string>> GetVersionsFromMeta(const std::string& metajson);
        std::optional<std::string> GetLoaderMetaUrl(const std::string& versionid);
        std::optional<std::string> DownloadLoaderMeta(const std::string& metaurl);
        std::optional<std::string> GetLoaderVersion(const std::string& loaderversionjson);
        std::optional<std::string> GetLoaderJsonDownloadUrl(const std::string& loaderid, const std::string& versionid);
        std::optional<std::string> DownloadLoaderJson(const std::string& jsonurl, const std::string& loaderid, const std::string& versionid);
        std::optional<std::string> GetLoaderJson(const std::string& loaderjson, const std::string& loaderid, const std::string& versionid);
        std::optional<std::vector<std::pair<std::string, std::string>>> GetLoaderLibrariesDownloadUrl(const std::string& mergedjson, OS os);
    }

    namespace auth
    {
        std::optional<std::string> GetMicrosoftLoginUrl();
        std::optional<std::string> StartMicrosoftLoginListener(const std::string& url);
        bool StopMicrosoftLoginListener();
        std::optional<std::string> GetAccessTokenJson(const std::string& code);
        std::optional<std::string> GetAccessTokenFromJson(const std::string& tokenjson);
        std::optional<std::string> GetRefreshTokenFromJson(const std::string& tokenjson);
        std::optional<std::string> GetXboxTokenJson(const std::string& accesstoken);
        std::optional<std::string> GetXboxTokenFromJson(const std::string& xboxjson);
        std::optional<std::string> GetXboxHashFromJson(const std::string& xboxjson);
        std::optional<std::string> GetXstsTokenJson(const std::string& xboxtoken);
        std::optional<std::string> GetXstsTokenFromJson(const std::string& xstsjson);
        std::optional<std::string> GetMinecraftTokenJson(const std::string& xboxhash, const std::string& xststoken);
        std::optional<std::string> GetMinecraftTokenFromJson(const std::string& minecraftjson);
        std::optional<std::string> GetMinecraftProfileJson(const std::string& minecrafttoken);
        std::optional<std::string> GetUsernameFromProfileJson(const std::string& profilejson);
        std::optional<std::string> GetUuidFromProfileJson(const std::string& profilejson);
    }

    std::optional<int> GetJavaVersion(const std::string& versionjson);
    std::optional<std::string> GetJavaDownloadUrl(int javaversion, OS os, Arch arch);
    std::optional<std::string> DownloadJava(const std::string& javaurl, const std::string& versionid);

    bool StartProcess(const std::string& javapath, const std::string& args, OS os, Processhandle* process, bool qt = false);
    bool StopProcess(Processhandle* process);
    bool DetectProcess(Processhandle* process);
}
