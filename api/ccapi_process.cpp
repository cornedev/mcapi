#include "api.hpp"

namespace ccapi
{

#ifdef _WIN32
bool StartProcess(const std::string& javapath, const std::string& args)
{
    if (!std::filesystem::exists(javapath))
    {
        std::cout << "Java not found: " << javapath << "\n";
        return false;
    }

    std::string cmd = "\"" + javapath + "\" " + args;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE outRead = NULL, outWrite = NULL;
    HANDLE errRead = NULL, errWrite = NULL;

    if (!CreatePipe(&outRead, &outWrite, &sa, 0)
        || !SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0))
    {
        std::cout << "Failed to create stdout pipe.\n";
        return false;
    }
    if (!CreatePipe(&errRead, &errWrite, &sa, 0)
        || !SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0))
    {
        std::cout << "Failed to create stderr pipe.\n";
        return false;
    }

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};
    si.cb = sizeof(si);
    si.hStdOutput = outWrite;
    si.hStdError  = errWrite;
    si.dwFlags = STARTF_USESTDHANDLES;

    BOOL success = CreateProcessA(
        NULL,
        cmd.data(),
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi
    );

    CloseHandle(outWrite);
    CloseHandle(errWrite);

    if (!success)
    {
        std::cout << "Failed to start Java.\n";
        return false;
    }

    // read
    std::thread([outRead]() {
        char buf[1024];
        DWORD read;
        while (ReadFile(outRead, buf, sizeof(buf)-1, &read, NULL) && read > 0)
        {
            buf[read] = 0;
            std::cout << buf;
        }
        CloseHandle(outRead);
    }).detach();
    std::thread([errRead]() {
        char buf[1024];
        DWORD read;
        while (ReadFile(errRead, buf, sizeof(buf)-1, &read, NULL) && read > 0)
        {
            buf[read] = 0;
            std::cout << buf;
        }
        CloseHandle(errRead);
    }).detach();

    std::thread([pi]() {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }).detach();
    return true;
}
#endif

}
