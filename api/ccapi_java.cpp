#include "api.hpp"

namespace ccapi
{

std::optional<int> GetJavaVersion(const std::string& versionjson)
{
    try
    {
        auto j = json::parse(versionjson);
        if (!j.contains("javaVersion") || !j["javaVersion"].contains("majorVersion"))
            return std::nullopt;
            
        int major = j["javaVersion"]["majorVersion"].get<int>();
        switch (major)
        {
            case 8:
            case 16:
            case 17:
            case 21:
            case 25:
                return major;
            default:
                return std::nullopt;
        }
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetJavaDownloadUrl(int javaversion, OS os, Arch arch)
{
    if (arch != Arch::x64 && arch != Arch::arm64)
    {
        std::cout << "Unsupported architecture.\n";
        return std::nullopt;
    }

    const char* osStr = nullptr;
    switch (os)
    {
        case OS::Windows: osStr = "windows";
        break;
        case OS::Linux: osStr = "linux";
        break;
        case OS::Macos: osStr = "mac";
        break;
        default: return std::nullopt;
    }

    switch (javaversion)
    {
        case 8:
        case 16:
        case 17:
        case 21:
        case 25:
            break;
        default:
            return std::nullopt;
    }
    std::string archStr = (arch == Arch::x64) ? "x64" : "aarch64";
    return "https://api.adoptium.net/v3/binary/latest/" + std::to_string(javaversion) + "/ga/" + osStr + "/" + archStr + "/jdk/hotspot/normal/eclipse";
}

std::optional<std::string> DownloadJava(const std::string& javaurl, const std::string& versionid)
{
    if (javaurl.empty())
        return std::nullopt;

    const fs::path basedir = fs::path("runtime") / versionid;
    const fs::path javadir = basedir / "java";
    const fs::path archivepath = basedir / "runtime.archive";
    fs::create_directories(basedir);

    if (fs::exists(javadir) && fs::is_directory(javadir))
        return javadir.string();

    if (!fs::exists(archivepath) || fs::file_size(archivepath) == 0)
    {
        std::wstring wurl(javaurl.begin(), javaurl.end());
        auto result = GET(wurl, GETmode::DiskOnly, "runtime.archive", basedir.string());
        if (!result)
            return std::nullopt;
    }

    struct archive* a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, archivepath.string().c_str(), 10240) != ARCHIVE_OK)
    {
        archive_read_free(a);
        return std::nullopt;
    }

    fs::path extractedroot;
    struct archive_entry* entry;

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK)
    {
        const char* pathname = archive_entry_pathname(entry);
        if (!pathname)
        {
            archive_read_data_skip(a);
            continue;
        }
        fs::path entrypath(pathname);

        if (entrypath.is_absolute() || entrypath.string().find("..") != std::string::npos)
        {
            archive_read_data_skip(a);
            continue;
        }

        if (extractedroot.empty())
        {
            auto it = entrypath.begin();
            if (it != entrypath.end())
                extractedroot = basedir / (*it);
        }
        fs::path outpath = basedir / entrypath;

        if (archive_entry_filetype(entry) == AE_IFDIR)
        {
            fs::create_directories(outpath);
        }
        else
        {
            fs::create_directories(outpath.parent_path());

            std::ofstream out(outpath, std::ios::binary);
            if (!out)
            {
                archive_read_free(a);
                return std::nullopt;
            }

            const void* buff;
            size_t size;
            la_int64_t offset;

            while (true)
            {
                int r = archive_read_data_block(a, &buff, &size, &offset);
                if (r == ARCHIVE_EOF)
                    break;
                if (r != ARCHIVE_OK)
                {
                    archive_read_free(a);
                    return std::nullopt;
                }
                out.write(static_cast<const char*>(buff), size);
            }
        }
    }
    archive_read_free(a);

    if (!extractedroot.empty() && fs::exists(extractedroot))
    {
        fs::rename(extractedroot, javadir);
    }
    else
    {
        return std::nullopt;
    }
    fs::remove(archivepath);
    return javadir.string();
}

}
