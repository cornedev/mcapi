#include "api.hpp"

namespace ccapi
{

std::optional<std::string> GetJavaDownloadUrl(const std::string& versionjson)
{
    try
    {
        auto j = json::parse(versionjson);
        if (!j.contains("javaVersion") || !j["javaVersion"].contains("majorVersion"))
            return std::nullopt;
            
        int majorVersion = j["javaVersion"]["majorVersion"].get<int>();
        switch (majorVersion)
        {
            case 21: return "https://download.java.net/openjdk/jdk21/ri/openjdk-21+35_windows-x64_bin.zip";
            case 17: return "https://download.java.net/openjdk/jdk17.0.0.1/ri/openjdk-17.0.0.1+2_windows-x64_bin.zip";
            case 16: return "https://download.java.net/openjdk/jdk16/ri/openjdk-16+36_windows-x64_bin.zip";
            case 8:  return "https://download.java.net/openjdk/jdk8u44/ri/openjdk-8u44-windows-i586.zip";
            default: return std::nullopt;
        }
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> DownloadJava(const std::string& javaurl, const std::string& versionid)
{
    if (javaurl.empty())
        return std::nullopt;

    const fs::path basedir = fs::path("runtime") / versionid;
    const fs::path javadir = basedir / "java";
    const fs::path zippath = basedir / "runtime.zip";
    fs::create_directories(basedir);

    if (fs::exists(javadir) && fs::is_directory(javadir))
        return javadir.string();

    if (!fs::exists(zippath) || fs::file_size(zippath) == 0)
    {
        std::wstring wurl(javaurl.begin(), javaurl.end());
        auto result = GET(wurl, GETmode::DiskOnly, "runtime.zip", basedir.string());
        if (!result)
            return std::nullopt;
    }

    int err = 0;
    zip* archive = zip_open(zippath.string().c_str(), 0, &err);
    if (!archive)
        return std::nullopt;

    fs::path extractedroot;

    zip_int64_t entries = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < entries; ++i)
    {
        const char* name = zip_get_name(archive, i, 0);
        if (!name) continue;

        std::string entryname(name);

        if (extractedroot.empty())
        {
            auto pos = entryname.find('/');
            if (pos != std::string::npos)
                extractedroot = basedir / entryname.substr(0, pos);
        }

        zip_file* file = zip_fopen_index(archive, i, 0);
        if (!file) continue;

        zip_stat_t st{};
        zip_stat_index(archive, i, 0, &st);

        std::vector<char> buffer(st.size);
        zip_fread(file, buffer.data(), buffer.size());
        zip_fclose(file);

        fs::path outpath = basedir / entryname;
        fs::create_directories(outpath.parent_path());

        std::ofstream out(outpath, std::ios::binary);
        out.write(buffer.data(), buffer.size());
        out.close();
    }
    zip_close(archive);

    if (!extractedroot.empty() && fs::exists(extractedroot))
    {
        fs::rename(extractedroot, javadir);
    }
    else
    {
        return std::nullopt;
    }
    fs::remove(zippath);
    return javadir.string();
}

}
