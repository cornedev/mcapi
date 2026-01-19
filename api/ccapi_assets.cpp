#include "api.hpp"

#pragma comment(lib, "winhttp.lib")
namespace ccapi
{

static fs::path datapath = ".ccapi";

// - helpers
static std::string GetOSNativesUrlName(OS os)
{
    switch (os)
    {
        case OS::Windows: return "windows";
        case OS::Linux: return "linux";
        case OS::Macos: return "macos";
    }
    return {};
}

std::string GetOSRuleName(OS os)
{
    switch (os)
    {
        case OS::Windows: return "windows";
        case OS::Linux: return "linux";
        case OS::Macos: return "osx";
    }
    return {};
}

static std::string GetArchSuffix(OS os, Arch arch)
{
    if (arch == Arch::x64)
        return "";

    if (arch == Arch::arm64)
    {
        switch (os)
        {
            case OS::Linux: return "-aarch_64";
            case OS::Macos: return "-arm64";
            case OS::Windows: return "-arm64";
        }
    }
    return "";
}

static bool GetRuleAllow(const json& lib, OS os)
{
    if (!lib.contains("rules"))
        return true;

    bool allowed = false;
    const std::string osname = GetOSRuleName(os);

    for (const auto& rule : lib["rules"])
    {
        bool applies = true;

        if (rule.contains("os"))
        {
            if (!rule["os"].contains("name"))
                applies = false;
            else
                applies = (rule["os"]["name"] == osname);
        }

        if (applies)
            allowed = (rule["action"] == "allow");
    }
    return allowed;
}

// this chooses if the version is modern or not (1.19 +/-)
static bool GetVersionAllow(const std::string& versionid)
{
    int major = 0, minor = 0, patch = 0;
    char dot;

    std::stringstream ss(versionid);
    ss >> major >> dot >> minor;
    if (ss.peek() == '.')
        ss >> dot >> patch;

    return (major > 1) || (major == 1 && minor >= 19);
}

std::optional<std::string> GET(const std::wstring& url, GETmode mode, const std::string& filename, const std::string& folder)
{
    URL_COMPONENTS uc{};
    uc.dwStructSize = sizeof(uc);

    wchar_t host[256]{};
    wchar_t path[2048]{};

    uc.lpszHostName = host;
    uc.dwHostNameLength = _countof(host);
    uc.lpszUrlPath = path;
    uc.dwUrlPathLength = _countof(path);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc))
        return std::nullopt;

    bool https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    INTERNET_PORT port = uc.nPort;
    std::wstring hostName = host;
    std::wstring urlPath = (uc.dwUrlPathLength > 0) ? path : L"/";
    std::ofstream out;
    std::string response;

    if (mode == GETmode::DiskOnly || mode == GETmode::MemoryAndDisk)
    {
        std::string diskfile = filename;

        if (diskfile.empty())
        {
            auto pos = urlPath.find_last_of(L'/');
            if (pos != std::wstring::npos && pos + 1 < urlPath.size())
                diskfile.assign(urlPath.begin() + pos + 1, urlPath.end());
            else
                diskfile = "download.bin";
        }

        if (!folder.empty())
        {
            std::filesystem::create_directories(folder);
            diskfile = folder + "/" + diskfile;
        }

        out.open(diskfile, std::ios::binary);
        if (!out)
            return std::nullopt;
    }

    HINTERNET session = WinHttpOpen(
        L"Mozilla/5.0 (Windows NT 10.0; Win64; x64)",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!session)
        return std::nullopt;

    DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(session, WINHTTP_OPTION_REDIRECT_POLICY,
                     &redirectPolicy, sizeof(redirectPolicy));

    HINTERNET connect = WinHttpConnect(
        session,
        hostName.c_str(),
        port,
        0
    );

    if (!connect)
    {
        WinHttpCloseHandle(session);
        return std::nullopt;
    }

    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET request = WinHttpOpenRequest(
        connect,
        L"GET",
        urlPath.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        flags
    );

    if (!request)
        goto cleanup;

    if (!WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0
    ))
        goto cleanup;

    if (!WinHttpReceiveResponse(request, nullptr))
        goto cleanup;

    for (;;)
    {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available) || available == 0)
            break;

        std::vector<char> buffer(available);
        DWORD read = 0;

        if (!WinHttpReadData(request, buffer.data(), available, &read) || read == 0)
            break;

        if (mode == GETmode::MemoryOnly || mode == GETmode::MemoryAndDisk)
            response.append(buffer.data(), read);

        if (mode == GETmode::DiskOnly || mode == GETmode::MemoryAndDisk)
            out.write(buffer.data(), read);
    }

cleanup:
    if (out.is_open())
        out.close();

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);

    if (mode == GETmode::DiskOnly)
        return std::string{};

    return response;
}

std::string DownloadVersionManifest()
{
    const fs::path manifestpath = datapath;
    return GET(L"https://launchermeta.mojang.com/mc/game/version_manifest.json", GETmode::MemoryAndDisk, "", manifestpath.string()).value_or("");
}

std::optional<std::vector<std::string>> GetVersionsFromManifest(const std::string& manifestjson)
{
    try
    {
        auto j = json::parse(manifestjson);
        if (!j.contains("versions") || !j["versions"].is_array())
            return std::nullopt;

        std::vector<std::string> ids;
        ids.reserve(j["versions"].size());
        for (const auto& versions : j["versions"])
        {
            if (!versions.contains("id") || !versions["id"].is_string())
                continue;

            ids.emplace_back(versions["id"].get<std::string>());
        }
        return ids;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetVersionJsonDownloadUrl(const std::string& manifestjson, const std::string& versionid)
{
    try
    {
        auto j = json::parse(manifestjson);
        for (const auto& entry : j["versions"])
        {
            if (entry["id"] == versionid)
                return entry["url"];
        }
    }
    catch (...)
    {
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<std::string> DownloadVersionJson(const std::string& jsonurl, const std::string& versionid)
{
    if (jsonurl.empty())
        return std::nullopt;

    const fs::path versionpath = datapath / "versions" / versionid;
    const fs::path jsonpath = versionpath / (versionid + ".json");

    if (fs::exists(jsonpath))
    {
        std::ifstream file(jsonpath, std::ios::binary);
        if (!file)
            return std::nullopt;

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::wstring wurl(jsonurl.begin(), jsonurl.end());
    return GET(wurl, GETmode::MemoryAndDisk, versionid + ".json", versionpath.string());
}

std::optional<std::string> GetClientJarDownloadUrl(const std::string& versionjson)
{
    try
    {
        auto j = json::parse(versionjson);
        if (!j.contains("downloads") || 
            !j["downloads"].contains("client") || 
            !j["downloads"]["client"].contains("url"))
        {
            return std::nullopt;
        }
        return j["downloads"]["client"]["url"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> DownloadClientJar(const std::string& clienturl, const std::string& versionid)
{
    if (clienturl.empty())
        return std::nullopt;

    const fs::path clientpath = datapath / "versions" / versionid;
    const fs::path jarpath = clientpath / "client.jar";
    
    if (fs::exists(jarpath) && fs::file_size(jarpath) > 0)
    {
        return std::string{};
    }

    std::wstring wurl(clienturl.begin(), clienturl.end());
    return GET(wurl, GETmode::DiskOnly, "client.jar", clientpath.string());
}

std::optional<std::string> GetAssetIndexJsonDownloadUrl(const std::string& versionjson)
{
    try
    {
        auto j = json::parse(versionjson);
        if (!j.contains("assetIndex") || 
            !j["assetIndex"].contains("url"))
        {
            return std::nullopt;
        }
        return j["assetIndex"]["url"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> DownloadAssetIndexJson(const std::string& indexurl, const std::string& versionid)
{
    if (indexurl.empty())
        return std::nullopt;
    
    const fs::path indexdir  = datapath / versionid / "assets" / "indexes";
    const auto slash = indexurl.find_last_of('/');
    if (slash == std::string::npos)
        return std::nullopt;

    const std::string filename = indexurl.substr(slash + 1);
    const fs::path indexpath = indexdir / filename;

    if (fs::exists(indexpath) && fs::file_size(indexpath) > 0)
    {
        std::ifstream file(indexpath, std::ios::binary);
        if (!file)
            return std::nullopt;

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    std::wstring wurl(indexurl.begin(), indexurl.end());
    return GET(wurl, GETmode::MemoryAndDisk, filename, indexdir.string());
}

std::optional<std::vector<std::pair<std::string, std::string>>> GetLibrariesDownloadUrl(const std::string& versionjson, OS os)
{
    try
    {
        auto j = json::parse(versionjson);
        if (!j.contains("libraries"))
            return std::nullopt;
    
        std::vector<std::pair<std::string, std::string>> urls;
        for (const auto& lib : j["libraries"])
        {
            if (!GetRuleAllow(lib, os))
                continue;

            if (!lib.contains("downloads") || !lib["downloads"].contains("artifact"))
                continue;

            const auto& artifact = lib["downloads"]["artifact"];
            if (!artifact.contains("url") || !artifact.contains("path"))
                continue;

            std::string url = artifact["url"];
            std::string path = artifact["path"];

            if (url.find("natives") != std::string::npos)
                continue;
            
            urls.emplace_back(url, path);
        }
        return urls;
    }
    catch (...)
    {
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<std::vector<std::string>> DownloadLibraries(const std::vector<std::pair<std::string, std::string>>& libraries, const std::string& versionid)
{
    std::vector<std::string> downloaded;
    for (const auto& [url, relpath] : libraries)
    {
        fs::path fullpath = datapath / "libraries" / versionid / relpath;
        fs::create_directories(fullpath.parent_path());
        if (fs::exists(fullpath) && fs::file_size(fullpath) > 0)
        {
            downloaded.push_back(fullpath.string());
            continue;
        }

        std::wstring wurl(url.begin(), url.end());
        std::string filename = fullpath.filename().string();
        std::string folder = fullpath.parent_path().string();

        auto result = GET(wurl, GETmode::DiskOnly, filename, folder);
        if (!result)
        {
            std::cout << "Failed to download library: " << url << "\n";
            return std::nullopt;
        }
        downloaded.push_back(fullpath.string());
    }
    return downloaded;
}

std::optional<std::vector<std::pair<std::string, std::string>>> GetAssetsDownloadUrl(const std::string& assetindexjson)
{
    try
    {
        auto j = json::parse(assetindexjson);
        if (!j.contains("objects"))
            return std::nullopt;

        std::vector<std::pair<std::string, std::string>> urls;
        for (const auto& entry : j["objects"].items())
        {
            const auto& obj = entry.value();
            if (!obj.contains("hash"))
                continue;

            std::string hash = obj["hash"];
            std::string firsttwo = hash.substr(0, 2);
            std::string url = "https://resources.download.minecraft.net/" + firsttwo + "/" + hash;
            std::string path = "assets/objects/" + firsttwo + "/" + hash;
            urls.emplace_back(url, path);
        }
        return urls;
    }
    catch (...)
    {
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<std::vector<std::string>> DownloadAssets(const std::vector<std::pair<std::string, std::string>>& assets, const std::string& versionid)
{
    std::vector<std::string> downloaded;
    for (const auto& [url, relpath] : assets)
    {
        fs::path fullpath = datapath / versionid / relpath;
        fs::create_directories(fullpath.parent_path());
        if (fs::exists(fullpath) && fs::file_size(fullpath) > 0)
        {
            downloaded.push_back(fullpath.string());
            continue;
        }

        std::wstring wurl(url.begin(), url.end());
        std::string filename = fullpath.filename().string();
        std::string folder = fullpath.parent_path().string();

        auto result = GET(wurl, GETmode::DiskOnly, filename, folder);
        if (!result)
        {
            std::cout << "Failed to download asset: " << url << "\n";
            continue;
        }
        downloaded.push_back(fullpath.string());
    }
    return downloaded;
}

std::optional<std::vector<std::pair<std::string, std::string>>> GetLibrariesNatives(const std::string& versionid, const std::string& versionjson, OS os, Arch arch)
{
    try
    {
        if (arch != Arch::x64 && arch != Arch::arm64)
        {
            std::cout << "Unsupported architecture.\n";
            return std::nullopt;
        }
        
        auto j = json::parse(versionjson);
        if (!j.contains("libraries"))
            return std::nullopt;

        const bool modern = GetVersionAllow(versionid);
        if (arch == Arch::arm64 && !modern)
        {
            std::cout << "arm64 is only supported for 1.19 and higher versions.\n";
            return std::nullopt;
        }

        const std::string ruleos = GetOSRuleName(os);
        const std::string artos = GetOSNativesUrlName(os);
        const std::string archsuffix = GetArchSuffix(os, arch);

        std::vector<std::pair<std::string, std::string>> natives;
        for (const auto& lib : j["libraries"])
        {
            if (!GetRuleAllow(lib, os))
                continue;

            if (!lib.contains("downloads"))
                continue;

            const auto& downloads = lib["downloads"];
            
            if (!modern)
            {
                if (!lib.contains("natives") || !downloads.contains("classifiers"))
                    continue;

                const auto& nativesmap = lib["natives"];
                if (!nativesmap.contains(ruleos))
                    continue;

                std::string classifier = nativesmap[ruleos].get<std::string>() + archsuffix;

                const auto& classifiers = downloads["classifiers"];
                if (!classifiers.contains(classifier))
                    continue;

                const auto& native = classifiers[classifier];
                natives.emplace_back(native["url"].get<std::string>(), native["path"].get<std::string>());
            }
            else
            {
                if (!downloads.contains("artifact"))
                    continue;
                
                if (downloads.contains("artifact"))
                {
                    const auto& artifact = downloads["artifact"];
                    std::string path = artifact["path"].get<std::string>();
                    if (path.find("natives-" + artos) != std::string::npos)
                    {
                        natives.emplace_back(artifact["url"].get<std::string>(), path);
                    }
                }
            }
        }
        return natives;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::vector<std::string>> DownloadLibrariesNatives(const std::vector<std::pair<std::string, std::string>>& natives, const std::string& versionid)
{
    std::vector<std::string> downloaded;
    for (const auto& [url, relpath] : natives)
    {
        fs::path fullpath = datapath / "libraries" / versionid / relpath;
        fs::create_directories(fullpath.parent_path());
        if (fs::exists(fullpath) && fs::file_size(fullpath) > 0)
        {
            downloaded.push_back(fullpath.string());
            continue;
        }

        std::wstring wurl(url.begin(), url.end());
        std::string filename = fullpath.filename().string();
        std::string folder = fullpath.parent_path().string();

        auto result = GET(wurl, GETmode::DiskOnly, filename, folder);
        if (!result)
        {
            std::cout << "Failed to download native jar: " << url << "\n";
            return std::nullopt;
        }
        downloaded.push_back(fullpath.string());
    }
    return downloaded;
}

std::optional<std::vector<std::string>> ExtractLibrariesNatives(const std::vector<std::string>& nativesjars, const std::string& versionid, OS os)
{
    std::vector<std::string> extracted;
    fs::create_directories(datapath / "natives" / versionid);

    std::string nativesext;
    switch (os)
    {
        case OS::Windows: nativesext = ".dll"; break;
        case OS::Macos: nativesext = ".dylib"; break;
        case OS::Linux: nativesext = ".so"; break;
        default:
            std::cout << "Unsupported OS.\n";
            return std::nullopt;
    }

    for (const auto& jarpath : nativesjars)
    {
        archive* ar = archive_read_new();
        archive_read_support_filter_all(ar);
        archive_read_support_format_zip(ar);

        if (archive_read_open_filename(ar, jarpath.c_str(), 10240) != ARCHIVE_OK)
        {
            std::cout << "Failed to open native jar: " << jarpath << "\n";
            archive_read_free(ar);
            return std::nullopt;
        }

        archive_entry* entry;
        while (archive_read_next_header(ar, &entry) == ARCHIVE_OK)
        {
            const char* name = archive_entry_pathname(entry);
            if (!name) { archive_read_data_skip(ar); continue; }
            std::string entryName(name);

            if (entryName.rfind("META-INF/", 0) == 0 || archive_entry_filetype(entry) == AE_IFDIR)
            {
                archive_read_data_skip(ar);
                continue;
            }

            if (entryName.size() < nativesext.size() ||
                entryName.substr(entryName.size() - nativesext.size()) != nativesext)
            {
                archive_read_data_skip(ar);
                continue;
            }

            fs::path outpath = datapath / "natives" / versionid / fs::path(entryName).filename();
            if (fs::exists(outpath) && fs::file_size(outpath) > 0)
            {
                extracted.push_back(outpath.string());
                archive_read_data_skip(ar);
                continue;
            }

            std::ofstream out(outpath, std::ios::binary);
            if (!out) { archive_read_free(ar); return std::nullopt; }

            const void* buff;
            size_t size;
            la_int64_t offset;
            while (true)
            {
                int r = archive_read_data_block(ar, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) break;
                if (r != ARCHIVE_OK) { archive_read_free(ar); return std::nullopt; }
                out.write(static_cast<const char*>(buff), size);
            }
            out.close();
            extracted.push_back(outpath.string());
        }
        archive_read_close(ar);
        archive_read_free(ar);
    }
    return extracted;
}

std::optional<std::string> GetClassPath(const std::string& versionjson, const std::vector<std::string>& libraries, const std::string& clientjarpath, OS os)
{
    try
    {
        json j = json::parse(versionjson);
        std::vector<std::string> jars;
        for (const auto& lib : libraries)
        {
            if (lib.empty())
                continue;

            if (lib.find("natives-") != std::string::npos)
                continue;

            fs::path p = fs::absolute(fs::path(lib)).make_preferred();
            if (fs::exists(p))
                jars.push_back(p.string());
        }
        fs::path clientjar = fs::absolute(fs::path(clientjarpath)).make_preferred();
        if (!fs::exists(clientjar))
            return std::nullopt;

        jars.push_back(clientjar.string());
        if (jars.empty())
            return std::nullopt;

        const char sep = (os == OS::Windows) ? ';' : ':';

        std::string classpath;
        for (size_t i = 0; i < jars.size(); ++i)
        {
            classpath += jars[i];
            if (i + 1 < jars.size())
                classpath += sep;
        }
        return classpath;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetLaunchCommand(const std::string& username, const std::string& classpath, const std::string& versionjson, const std::string& versionid)
{
    try
    {
        json j = json::parse(versionjson);
        if (!j.contains("mainClass"))
            return std::nullopt;

        std::string mainClass = j["mainClass"];
        std::string assetIndex = versionid;
        if (j.contains("assets"))
            assetIndex = j["assets"].get<std::string>();

        std::filesystem::path gameDir = std::filesystem::absolute(datapath / "versions" / versionid);
        std::filesystem::path assetsDir = std::filesystem::absolute(datapath / "assets" / versionid);
        std::filesystem::path nativesDir = std::filesystem::absolute(datapath / "natives" / versionid);
        std::filesystem::create_directories(gameDir);
        std::filesystem::create_directories(assetsDir);
        std::filesystem::create_directories(nativesDir);

        std::string cmd;
        cmd.reserve(2048);
        cmd += "-Xmx2G -Xms1G ";
        cmd += "-Djava.library.path=\"" + nativesDir.string() + "\" ";
        cmd += "-cp \"" + classpath + "\" ";
        cmd += mainClass + " ";
        cmd += "--username " + username + " ";
        cmd += "--version " + versionid + " ";
        cmd += "--gameDir \"" + gameDir.string() + "\" ";
        cmd += "--assetsDir \"" + assetsDir.string() + "\" ";
        cmd += "--assetIndex " + assetIndex + " ";
        cmd += "--uuid 00000000-0000-0000-0000-000000000000 ";
        cmd += "--accessToken 0 ";
        cmd += "--userType mojang";
        return cmd;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetServerJarDownloadUrl(const std::string& versionjson)
{
    try
    {
        auto j = json::parse(versionjson);
        if (!j.contains("downloads") || 
            !j["downloads"].contains("server") || 
            !j["downloads"]["server"].contains("url"))
        {
            return std::nullopt;
        }
        return j["downloads"]["server"]["url"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> DownloadServerJar(const std::string& serverurl, const std::string& versionid)
{
    if (serverurl.empty())
        return std::nullopt;

    const fs::path serverdir = datapath / "versions" / versionid / "server";
    const fs::path jarpath   = serverdir / "server.jar";

    if (fs::exists(jarpath) && fs::file_size(jarpath) > 0)
    {
        return jarpath.string();
    }
    fs::create_directories(serverdir);

    std::wstring wurl(serverurl.begin(), serverurl.end());
    return GET(wurl, GETmode::DiskOnly, "server.jar", serverdir.string());
}

}
