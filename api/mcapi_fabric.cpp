#include "api.hpp"

namespace mcapi
{

namespace fabric
{

json GetMergedJson(json parent, json child)
{
    for (auto& [key, value] : child.items())
    {
        if (parent.contains(key))
        {
            if (value.is_array() && parent[key].is_array())
            {
                for (auto& v : value)
                    parent[key].push_back(v);
            }
            else if (value.is_object() && parent[key].is_object())
            {
                parent[key] = GetMergedJson(parent[key], value);
            }
            else
            {
                parent[key] = value;
            }
        }
        else
        {
            parent[key] = value;
        }
    }
    return parent;
}

std::string DownloadVersionMeta()
{
    const fs::path metapath = datapath;
    return GET(L"https://meta.fabricmc.net/v2/versions/game", GETmode::MemoryAndDisk, "version_meta.json", metapath.string()).value_or("");
}

std::optional<std::vector<std::string>> GetVersionsFromMeta(const std::string& metajson)
{
    try
    {
        auto j = json::parse(metajson);
        if (!j.is_array())
            return std::nullopt;

        std::vector<std::string> ids;
        ids.reserve(j.size());
        for (const auto& entry : j)
        {
            if (!entry.contains("version") || !entry["version"].is_string())
                continue;

            ids.emplace_back(entry["version"].get<std::string>());
        }
        return ids;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetLoaderMetaUrl(const std::string& versionid)
{
    return "https://meta.fabricmc.net/v2/versions/loader/" + versionid;
}

std::optional<std::string> DownloadLoaderMeta(const std::string& metaurl)
{
    if (metaurl.empty())
        return std::nullopt;

    const fs::path metapath = datapath / "loader_meta.json";
    const fs::path metadiskpath = datapath;

    if (fs::exists(metapath))
    {
        std::ifstream file(metapath, std::ios::binary);
        if (!file)
            return std::nullopt;

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::wstring wurl(metaurl.begin(), metaurl.end());
    return GET(wurl, GETmode::MemoryAndDisk, "loader_meta.json", metadiskpath.string());
}

std::optional<std::string> GetLoaderVersion(const std::string& loadermetajson)
{
    try
    {
        auto j = json::parse(loadermetajson);
        if (!j.is_array() || j.empty())
            return std::nullopt;

        const auto& loader = j.at(0);
        if (!loader.contains("loader") || 
            !loader["loader"].contains("version") ||
            !loader["loader"]["version"].is_string())
        {
            return std::nullopt;
        }
        return loader["loader"]["version"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetLoaderJsonDownloadUrl(const std::string& loaderid, const std::string& versionid)
{
    return "https://meta.fabricmc.net/v2/versions/loader/" + versionid + "/" + loaderid + "/profile/json";
}

std::optional<std::string> DownloadLoaderJson(const std::string& jsonurl, const std::string& loaderid, const std::string& versionid)
{
    if (jsonurl.empty())
        return std::nullopt;

    const fs::path versionpath = datapath / "versions";
    const fs::path jsonpath = versionpath / (versionid + "-fabric-loader-" + loaderid);
    const fs::path jsondiskpath = versionpath / (versionid + "-fabric-loader-" + loaderid) / (versionid + "-fabric-loader-" + loaderid + "json");

    if (fs::exists(jsondiskpath))
    {
        std::ifstream file(jsondiskpath, std::ios::binary);
        if (!file)
            return std::nullopt;

        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::wstring wurl(jsonurl.begin(), jsonurl.end());
    return GET(wurl, GETmode::MemoryAndDisk, versionid + "-fabric-loader-" + loaderid + ".json", jsonpath.string());
}

std::optional<std::string> GetLoaderJson(const std::string& loaderjson, const std::string& loaderid, const std::string& versionid)
{
    try
    {
        auto j = json::parse(loaderjson);
        if (!j.contains("inheritsFrom"))
            return j.dump();

        std::string parentid = j["inheritsFrom"];

        auto manifest = mcapi::vanilla::DownloadVersionManifest();
        if (manifest.empty())
        {
            return std::nullopt;
        }

        auto parenturl = mcapi::vanilla::GetVersionJsonDownloadUrl(manifest, parentid);
        if (!parenturl)
        {
            return std::nullopt;
        }

        std::string parentjsonid = versionid + "-vanilla-loader";
        auto parentjson = mcapi::vanilla::DownloadVersionJson(parenturl.value(), parentjsonid);
        if (!parentjson)
        {
            return std::nullopt;
        }

        json parent = json::parse(parentjson.value());
        json loaderjson = GetMergedJson(parent, j);
        std::string diskjson = loaderjson.dump(4);

        const fs::path vanillajsonpath = datapath / "versions" / (versionid + "-vanilla-loader")  / (versionid + "-vanilla-loader.json");
        const std::string diskpath = ".mcapi/versions/" + versionid + "-fabric-loader-" + loaderid + "/" + versionid + "-fabric-loader-" + loaderid + ".json";
        if (!diskpath.empty()) 
        {
            std::ofstream out(diskpath, std::ios::trunc);
            if (out.is_open()) 
            {
                out << diskjson;
                out.close();
            }
        }
        if (fs::exists(vanillajsonpath)) 
        {
            const fs::path vanillajsonfolderpath = datapath / "versions" / (versionid + "-vanilla-loader");
            fs::remove_all(vanillajsonfolderpath);
        }
        return diskjson;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::vector<std::pair<std::string, std::string>>> GetLoaderLibrariesDownloadUrl(const std::string& mergedjson, OS os)
{
    try
    {
        auto j = json::parse(mergedjson);
        if (!j.contains("libraries"))
            return std::nullopt;
    
        std::vector<std::pair<std::string, std::string>> urls;
        for (const auto& lib : j["libraries"])
        {
            if (!GetRuleAllow(lib, os))
                continue;

            // - vanilla libraries.
            if (lib.contains("downloads") && lib["downloads"].contains("artifact"))
            {
                const auto& artifact = lib["downloads"]["artifact"];

                std::string url = artifact["url"];
                std::string path = artifact["path"];

                if (url.find("natives") == std::string::npos) 
                {
                    urls.emplace_back(url, path);
                }
            }

            // - fabric libraries.
            if (lib.contains("url") && lib.contains("name"))
            {
                std::string baseurl = lib["url"];
                std::string name = lib["name"];

                size_t firstcolon = name.find(':');
                size_t lastcolon = name.rfind(':');
                if (firstcolon == std::string::npos || firstcolon == lastcolon) 
                    continue;
    
                std::string group = name.substr(0, firstcolon);
                std::string artifact = name.substr(firstcolon + 1, lastcolon - firstcolon - 1);
                std::string version = name.substr(lastcolon + 1);
                
                std::replace(group.begin(), group.end(), '.', '/');
                std::string path = group + "/" + artifact + "/" + version + "/" + artifact + "-" + version + ".jar";
                urls.emplace_back(baseurl + path, "libraries/" + path);
            }
        }
        return urls;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

}

}
