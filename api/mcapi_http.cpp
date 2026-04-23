#include "api.hpp"

namespace mcapi
{

// - helpers.
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
// - end helpers.

std::optional<std::string> GET(const std::wstring& url, GETmode mode, const std::string& filename, const std::string& folder, const std::vector<std::string>& headers)
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

    std::pair<std::string*, std::ofstream*> userdata
    {
        (mode == GETmode::MemoryOnly || mode == GETmode::MemoryAndDisk) ? &response : nullptr,
        (mode == GETmode::DiskOnly  || mode == GETmode::MemoryAndDisk) ? &out : nullptr
    };

    curl_easy_setopt(curl, CURLOPT_URL, curlurl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userdata);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // - security.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");

    struct curl_slist* headerlist = nullptr;
    for (const auto& h : headers)
        headerlist = curl_slist_append(headerlist, h.c_str());
    if (headerlist)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

    CURLcode res = curl_easy_perform(curl);
    if (headerlist)
        curl_slist_free_all(headerlist);
    curl_easy_cleanup(curl);

    if (out.is_open())
        out.close();

    if (res != CURLE_OK)
        return std::nullopt;

    if (mode == GETmode::DiskOnly)
        return std::string{};

    return response;
}

std::optional<std::string> POST(const std::wstring& url, const std::string& body, const std::vector<std::string>& headers)
{
    std::string curlurl(url.begin(), url.end());

    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl)
        return std::nullopt;

    std::pair<std::string*, std::ofstream*> userdata
    {
        &response, nullptr
    };

    curl_easy_setopt(curl, CURLOPT_URL, curlurl.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &userdata);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    struct curl_slist* headerlist = nullptr;
    for (const auto& h : headers)
        headerlist = curl_slist_append(headerlist, h.c_str());
    if (headerlist)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

    // - security.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");

    CURLcode res = curl_easy_perform(curl);
    if (headerlist)
        curl_slist_free_all(headerlist);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        return std::nullopt;

    return response;
}

}
