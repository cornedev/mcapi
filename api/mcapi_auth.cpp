#include "api.hpp"

namespace mcapi
{

// - helper defines.
static std::atomic<bool> logincancel{false};
static socket_t server;
// - end helper defines.

// - helpers.
static std::string GetCodeFromUrl(const std::string& request)
{
    size_t codepos = request.find("code=");
    if (codepos == std::string::npos) 
        return "";

    size_t end = request.find('&', codepos);
    if (end == std::string::npos)
        end = request.find(' ', codepos);

    return request.substr(codepos + 5, end - (codepos + 5));
}
// - end helpers.

namespace auth
{

std::optional<std::string> GetMicrosoftLoginUrl()
{
    const std::string clientid = "3801bb4f-aa98-4355-96a8-8e52ebe042bf";

    return "https://login.live.com/oauth20_authorize.srf?client_id=" + clientid + "&response_type=code&redirect_uri=http://127.0.0.1:8080/&scope=XboxLive.signin%20offline_access&prompt=select_account";
}

std::optional<std::string> StartMicrosoftLoginListener(const std::string& url)
{
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
        return std::nullopt;
    #endif

    socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        return std::nullopt;

    server = server_fd;
    logincancel = false;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        #ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
        #else
        close(server_fd);
        #endif
        server = -1;
        return std::nullopt;
    }
    if (listen(server_fd, 1) < 0)
    {
        #ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
        #else
        close(server_fd);
        #endif
        server = -1;
        return std::nullopt;
    }

    #ifdef _WIN32
    std::string cmd = "cmd /c start \"\" \"" + url + "\"";
    #elif __APPLE__
    std::string cmd = "open \"" + url + "\"";
    #else
    std::string cmd = "xdg-open \"" + url + "\"";
    #endif
    system(cmd.c_str());

    std::string code = "";
    while (!logincancel)
    {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(server_fd, &set);
        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int rv = select(server_fd + 1, &set, nullptr, nullptr, &timeout);
        if (rv < 0) 
            break;
        if (rv == 0) 
            continue;

        socket_t client = accept(server_fd, nullptr, nullptr);
        if (client < 0) 
            continue;

        timeval recv_timeout{};
        recv_timeout.tv_sec = 5;
        recv_timeout.tv_usec = 0;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&recv_timeout, sizeof(recv_timeout));

        char buffer[4096];
        int len = recv(client, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) 
        {
            #ifdef _WIN32
            closesocket(client);
            #else
            close(client);
            #endif
            continue;
        }
        buffer[len] = '\0';
        std::string request(buffer);
        code = GetCodeFromUrl(request);

        // - browser response.
        std::string redirect;
        std::ifstream file(".mcapi/mcapi_redirect.html", std::ios::binary);
        if (file)
        {
            std::ostringstream redirectbuffer;
            redirectbuffer << file.rdbuf();
            redirect = redirectbuffer.str();
        }
        else
        {
            redirect = "<html><body>Redirect file missing.</body></html>";
        }
        std::string response = "HTTP/1.1 200 OK\r\n" "Content-Type: text/html\r\n" "Content-Length: " + std::to_string(redirect.size()) + "\r\n" "\r\n" + redirect;
        #ifdef _WIN32
        send(client, response.c_str(), response.size(), 0);
        closesocket(client);
        #else
        write(client, response.c_str(), response.size());
        close(client);
        #endif

        if (!code.empty()) 
        {
            CURL* curl = curl_easy_init();
            if (curl)
            {
                int outlen = 0;
                char* codedecoded = curl_easy_unescape(curl, code.c_str(), code.length(), &outlen);
                if (codedecoded)
                {
                    code = std::string(codedecoded, outlen);
                    curl_free(codedecoded);
                }
                curl_easy_cleanup(curl);
            }
            break;
        }
    }

    #ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
    #else
    close(server_fd);
    #endif
    server = -1;

    if (code.empty())
        return std::nullopt;

    return code;
}

bool StopMicrosoftLoginListener()
{
    bool running = (server != -1);
    logincancel = true;

    if (server != -1)
    {
        #ifdef _WIN32
        closesocket(server);
        #else
        close(server);
        #endif
        server = -1;
    }
    return running;
}

std::optional<std::string> GetAccessTokenJson(const std::string& code)
{
    const std::string clientid = "3801bb4f-aa98-4355-96a8-8e52ebe042bf";

    CURL* curl = curl_easy_init();
    if (!curl)
        return std::nullopt;

    char* encoded = curl_easy_escape(curl, code.c_str(), code.length());
    if (!encoded)
    {
        curl_easy_cleanup(curl);
        return std::nullopt;
    }
    std::string body ="client_id=" + clientid + "&code=" + std::string(encoded) + "&grant_type=authorization_code" + "&redirect_uri=http://127.0.0.1:8080/";

    curl_free(encoded);
    curl_easy_cleanup(curl);

    return POST(L"https://login.live.com/oauth20_token.srf", body, {"Content-Type: application/x-www-form-urlencoded"});
}

std::optional<std::string> GetAccessTokenFromJson(const std::string& tokenjson)
{
    try
    {
        auto j = json::parse(tokenjson);
        if (!j.contains("access_token") || 
            !j["access_token"].is_string())
        {
            return std::nullopt;
        }
        return j["access_token"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetRefreshTokenFromJson(const std::string& tokenjson)
{
    try
    {
        auto j = json::parse(tokenjson);
        if (!j.contains("refresh_token") || 
            !j["refresh_token"].is_string())
        {
            return std::nullopt;
        }
        std::string token = j["refresh_token"].get<std::string>();

        const fs::path refreshtoken = datapath / "refresh_token";
        std::ofstream file(refreshtoken, std::ios::trunc);
        if (file)
            file << token;
        
        return token;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetAccessTokenFromRefreshToken()
{
    const fs::path refreshtoken = datapath / "refresh_token";
    std::ifstream file(refreshtoken);
    if (!file)
        return std::nullopt;

    std::string token;
    std::getline(file, token);

    if (token.empty())
        return std::nullopt;

    const std::string clientid = "3801bb4f-aa98-4355-96a8-8e52ebe042bf";

    CURL* curl = curl_easy_init();
    if (!curl)
        return std::nullopt;

    char* encoded = curl_easy_escape(curl, token.c_str(),static_cast<int>(token.size()));
    if (!encoded)
    {
        curl_easy_cleanup(curl);
        return std::nullopt;
    }
    std::string body = "client_id=" + clientid + "&refresh_token=" + std::string(encoded) + "&grant_type=refresh_token" + "&redirect_uri=http://127.0.0.1:8080/";

    curl_free(encoded);
    curl_easy_cleanup(curl);

    auto json = POST(L"https://login.live.com/oauth20_token.srf", body, {"Content-Type: application/x-www-form-urlencoded"});
    if (!json)
        return std::nullopt;

    GetRefreshTokenFromJson(*json);
    return GetAccessTokenFromJson(*json);
}

std::optional<std::string> GetXboxTokenJson(const std::string& accesstoken)
{
    std::string body = R"({"Properties": {"AuthMethod": "RPS", "SiteName": "user.auth.xboxlive.com", "RpsTicket": "d=)" + accesstoken + R"("}, "RelyingParty": "http://auth.xboxlive.com", "TokenType": "JWT"})";

    return POST(L"https://user.auth.xboxlive.com/user/authenticate", body, {"Content-Type: application/json"});
}

std::optional<std::string> GetXboxTokenFromJson(const std::string& xboxjson)
{
    try
    {
        auto j = json::parse(xboxjson);
        if (!j.contains("Token") || 
            !j["Token"].is_string())
        {
            return std::nullopt;
        }
        return j["Token"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetXboxHashFromJson(const std::string& xboxjson)
{
    try
    {
        auto j = json::parse(xboxjson);
        if (!j.contains("DisplayClaims") || 
            !j["DisplayClaims"].contains("xui") ||
            !j["DisplayClaims"]["xui"].is_array() ||
             j["DisplayClaims"]["xui"].empty() ||
            !j["DisplayClaims"]["xui"][0].contains("uhs") ||
            !j["DisplayClaims"]["xui"][0]["uhs"].is_string())
        {
            return std::nullopt;
        }
        return j["DisplayClaims"]["xui"][0]["uhs"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetXstsTokenJson(const std::string& xboxtoken)
{
    std::string body = R"({"Properties": {"SandboxId": "RETAIL", "UserTokens": [")" + xboxtoken + R"("]}, "RelyingParty": "rp://api.minecraftservices.com/", "TokenType": "JWT"})";

    return POST(L"https://xsts.auth.xboxlive.com/xsts/authorize", body, {"Content-Type: application/json"});
}

std::optional<std::string> GetXstsTokenFromJson(const std::string& xstsjson)
{
    try
    {
        auto j = json::parse(xstsjson);
        if (!j.contains("Token") || 
            !j["Token"].is_string())
        {
            return std::nullopt;
        }
        return j["Token"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetMinecraftTokenJson(const std::string& xboxhash, const std::string& xststoken)
{
    std::string body = R"({"identityToken":"XBL3.0 x=)" + xboxhash + ";" + xststoken + R"("})";

    return POST(L"https://api.minecraftservices.com/authentication/login_with_xbox", body, {"Content-Type: application/json"});
}

std::optional<std::string> GetMinecraftTokenFromJson(const std::string& minecraftjson)
{
    try
    {
        auto j = json::parse(minecraftjson);
        if (!j.contains("access_token") || 
            !j["access_token"].is_string())
        {
            return std::nullopt;
        }
        return j["access_token"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetMinecraftProfileJson(const std::string& minecrafttoken)
{
    return GET(L"https://api.minecraftservices.com/minecraft/profile", GETmode::MemoryOnly, "", "", {"Authorization: Bearer " + minecrafttoken, "Accept: application/json"});
}

std::optional<std::string> GetUsernameFromProfileJson(const std::string& profilejson)
{
    try
    {
        auto j = json::parse(profilejson);
        if (!j.contains("name") || 
            !j["name"].is_string())
        {
            return std::nullopt;
        }
        return j["name"].get<std::string>();
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<std::string> GetUuidFromProfileJson(const std::string& profilejson)
{
    try
    {
        auto j = json::parse(profilejson);
        if (!j.contains("id") || 
            !j["id"].is_string())
        {
            return std::nullopt;
        }

        std::string uuid = j["id"].get<std::string>();
        if (uuid.length() == 32)
        {
            return uuid.substr(0, 8) + "-" + uuid.substr(8, 4) + "-" + uuid.substr(12, 4) + "-" + uuid.substr(16, 4) + "-" + uuid.substr(20, 12);
        }
        return uuid;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

}

}
