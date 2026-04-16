#include "api.hpp"

namespace mcapi
{

namespace auth
{

std::optional<std::string> GetMicrosoftLoginUrl()
{
    const std::string clientid = "3801bb4f-aa98-4355-96a8-8e52ebe042bf";

    return "https://login.live.com/oauth20_authorize.srf?client_id=" + clientid + "&response_type=code&redirect_uri=http://127.0.0.1:8080/&scope=XboxLive.signin%20offline_access";
}

}

}
