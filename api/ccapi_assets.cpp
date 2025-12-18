#include "api.hpp"

namespace ccapi
{

static size_t curl_write_callback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    auto* pair = static_cast<std::pair<std::string*, std::ofstream*>*>(userdata);
    const size_t total = size * nmemb;

    if (pair->second && pair->second->is_open())
        pair->second->write(static_cast<char*>(ptr), total);

    if (pair->first)
        pair->first->append(static_cast<char*>(ptr), total);

    return total;
}

static std::optional<std::string> GET(const std::wstring& url, GETmode mode = GETmode::MemoryOnly, const std::string& filename = "", const std::string& folder = "")
{
    std::string curlurl(url.begin(), url.end());

    std::ofstream out;
    std::string response;

    if (mode == GETmode::DiskOnly || mode == GETmode::MemoryAndDisk)
    {
        std::string diskfile = filename;

        if (diskfile.empty())
        {
            auto slash = curlurl.find_last_of('/');
            if (slash != std::string::npos && slash + 1 < curlurl.size())
                diskfile = curlurl.substr(slash + 1);

            if (diskfile.empty())
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

    CURL* curl = curl_easy_init();
    if (!curl)
        return std::nullopt;

    std::pair<std::string*, std::ofstream*> userdata{
        (mode == GETmode::MemoryOnly || mode == GETmode::MemoryAndDisk) ? &response : nullptr,
        (mode == GETmode::DiskOnly  || mode == GETmode::MemoryAndDisk) ? &out      : nullptr
    };

    curl_easy_setopt(curl, CURLOPT_URL, curlurl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userdata);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Match launcher behavior
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (out.is_open())
        out.close();

    if (res != CURLE_OK)
        return std::nullopt;

    if (mode == GETmode::DiskOnly)
        return std::string{};

    return response;
}

std::string DownloadVersionManifest()
{
    return GET(L"https://launchermeta.mojang.com/mc/game/version_manifest.json", GETmode::MemoryAndDisk).value_or("");
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

    const fs::path versionpath  = fs::path("versions") / versionid;
    const fs::path jsonpath    = versionpath / (versionid + ".json");

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

    const fs::path clientpath = fs::path("versions") / versionid;
    const fs::path jarpath   = clientpath / "client.jar";
    
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

std::optional<std::string> DownloadAssetIndexJson(const std::string& indexurl)
{
    if (indexurl.empty())
        return std::nullopt;
    
    const fs::path indexdir  = fs::path("assets") / "indexes";
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

std::optional<std::vector<std::pair<std::string, std::string>>> GetLibrariesDownloadUrl(const std::string& versionjson)
{
    try
    {
        auto j = json::parse(versionjson);
        if (!j.contains("libraries"))
            return std::nullopt;
    
        std::vector<std::pair<std::string, std::string>> urls;
        for (const auto& lib : j["libraries"])
        {
            if (!lib.contains("downloads") || !lib["downloads"].contains("artifact"))
                continue;

            const auto& artifact = lib["downloads"]["artifact"];
            if (!artifact.contains("url") || !artifact.contains("path"))
                continue;

            std::string url = artifact["url"];
            std::string path = artifact["path"];
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

std::optional<std::vector<std::string>> DownloadLibraries(const std::vector<std::pair<std::string, std::string>>& libraries)
{
    std::vector<std::string> downloaded;
    for (const auto& [url, relpath] : libraries)
    {
        fs::path fullpath = fs::path("libraries") / relpath;
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
            std::string url ="https://resources.download.minecraft.net/" + firsttwo + "/" + hash;
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

std::optional<std::vector<std::string>> DownloadAssets(const std::vector<std::pair<std::string, std::string>>& assets)
{
    std::vector<std::string> downloaded;
    for (const auto& [url, relpath] : assets)
    {
        fs::path fullpath = relpath;
        fs::create_directories(fullpath.parent_path());
        if (fs::exists(fullpath) && fs::file_size(fullpath) > 0)
        {
            downloaded.push_back(fullpath.string());
            continue;
        }

        std::wstring wurl(url.begin(), url.end());
        std::string filename = fullpath.filename().string();
        std::string folder   = fullpath.parent_path().string();

        auto result = GET(wurl, GETmode::DiskOnly, filename, folder);
        if (!result)
        {
            std::cout << "Failed to download asset: " << url << "\n";
            return std::nullopt;
        }
        downloaded.push_back(fullpath.string());
    }
    return downloaded;
}

std::optional<std::vector<std::pair<std::string, std::string>>> GetLibrariesNatives(const std::string& versionjson)
{
    try
    {
        auto j = json::parse(versionjson);
        if (!j.contains("libraries"))
            return std::nullopt;

        std::vector<std::pair<std::string, std::string>> natives;
        for (const auto& lib : j["libraries"])
        {
            if (!lib.contains("downloads"))
                continue;

            const auto& downloads = lib["downloads"];
            if (!downloads.contains("classifiers"))
                continue;

            const auto& classifiers = downloads["classifiers"];
            if (!classifiers.contains("natives-windows"))
                continue;

            const auto& native = classifiers["natives-windows"];
            if (!native.contains("url") || !native.contains("path"))
                continue;

            natives.emplace_back(
                native["url"].get<std::string>(),
                native["path"].get<std::string>()
            );
        }
        return natives;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::vector<std::string>> ExtractLibrariesNatives(const std::vector<std::pair<std::string, std::string>>& natives)
{
    std::vector<std::string> extracted;
    fs::create_directories("natives");
    for (const auto& [url, relpath] : natives)
    {
        fs::path jarpath = fs::path("libraries") / relpath;
        if (!fs::exists(jarpath))
        {
            std::cout << "Native jar missing: " << jarpath << "\n";
            return std::nullopt;
        }

        int err = 0;
        zip* archive = zip_open(jarpath.string().c_str(), 0, &err);
        if (!archive)
        {
            std::cout << "Failed to open native jar: " << jarpath << "\n";
            return std::nullopt;
        }

        zip_int64_t entries = zip_get_num_entries(archive, 0);
        for (zip_int64_t i = 0; i < entries; ++i)
        {
            const char* name = zip_get_name(archive, i, 0);
            if (!name)
                continue;

            std::string entryName(name);
            if (entryName.size() < 4 || entryName.substr(entryName.size() - 4) != ".dll")
                continue;

            fs::path outpath = fs::path("natives") / fs::path(entryName).filename();
            if (fs::exists(outpath) && fs::file_size(outpath) > 0)
            {
                extracted.push_back(outpath.string());
                continue;
            }

            zip_file* file = zip_fopen_index(archive, i, 0);
            if (!file)
                continue;

            zip_stat_t st{};
            zip_stat_index(archive, i, 0, &st);

            std::vector<char> buffer(st.size);
            zip_fread(file, buffer.data(), buffer.size());
            zip_fclose(file);

            std::ofstream out(outpath, std::ios::binary);
            if (!out)
            {
                zip_close(archive);
                return std::nullopt;
            }

            out.write(buffer.data(), buffer.size());
            out.close();

            extracted.push_back(outpath.string());
        }
        zip_close(archive);
    }
    return extracted;
}

std::optional<std::string> GetClassPath(const std::string& versionjson, const std::vector<std::string>& libraries, const std::string& clientjarpath)
{
    try
    {
        json j = json::parse(versionjson);
        std::vector<std::string> jars;
        for (const auto& lib : libraries)
        {
            if (!lib.empty())
                jars.push_back(lib);
        }
        fs::path clientjar = fs::absolute(fs::path(clientjarpath)).make_preferred();
        if (!fs::exists(clientjar))
        {
            return std::nullopt;
        }
        jars.push_back(clientjar.string());

        std::string classpath;
        for (size_t i = 0; i < jars.size(); ++i)
        {
            classpath += jars[i];
            if (i + 1 < jars.size())
                classpath += ";";
        }
        if (jars.empty())
            return std::nullopt;
        
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

        std::filesystem::path gameDir    = std::filesystem::absolute("versions/" + versionid);
        std::filesystem::path assetsDir  = std::filesystem::absolute("assets");
        std::filesystem::path nativesDir = std::filesystem::absolute("natives");
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

    const fs::path serverdir = fs::path("versions") / versionid / "server";
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
