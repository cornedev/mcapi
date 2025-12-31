#include "api.hpp"

namespace ccapi
{

bool StartProcess(const std::string& javapath, const std::string& args, OS os)
{
    if (!std::filesystem::exists(javapath))
    {
        std::cout << "Java not found: " << javapath << "\n";
        return false;
    }

    switch (os)
    {
        case OS::Windows:
        {
            #ifdef _WIN32
            std::string cmd = "\"" + javapath + "\" " + args;

            SECURITY_ATTRIBUTES sa{};
            sa.nLength = sizeof(sa);
            sa.bInheritHandle = TRUE;

            HANDLE outRead = nullptr, outWrite = nullptr;
            HANDLE errRead = nullptr, errWrite = nullptr;

            if (!CreatePipe(&outRead, &outWrite, &sa, 0) ||
                !SetHandleInformation(outRead, HANDLE_FLAG_INHERIT, 0))
            {
                std::cout << "Failed to create stdout pipe.\n";
                return false;
            }

            if (!CreatePipe(&errRead, &errWrite, &sa, 0) ||
                !SetHandleInformation(errRead, HANDLE_FLAG_INHERIT, 0))
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
                nullptr,
                cmd.data(),
                nullptr,
                nullptr,
                TRUE,
                CREATE_NO_WINDOW,
                nullptr,
                nullptr,
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

            std::thread([outRead]() {
                char buf[1024];
                DWORD read;
                while (ReadFile(outRead, buf, sizeof(buf) - 1, &read, nullptr) && read > 0)
                {
                    buf[read] = 0;
                    std::cout << buf;
                }
                CloseHandle(outRead);
            }).detach();

            std::thread([errRead]() {
                char buf[1024];
                DWORD read;
                while (ReadFile(errRead, buf, sizeof(buf) - 1, &read, nullptr) && read > 0)
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
            #else
            std::cout << "Please choose the correct OS.\n";
            return false;
            #endif
        }

        case OS::Linux:
        case OS::Macos:
        {
            #ifndef _WIN32
            if (chmod(javapath.c_str(), 0755) != 0)
            {
                std::perror("chmod failed.");
                return false;
            }

            int outPipe[2];
            int errPipe[2];

            if (pipe(outPipe) != 0 || pipe(errPipe) != 0)
            {
                std::cout << "Failed to create pipes.\n";
                return false;
            }

            pid_t pid = fork();
            if (pid < 0)
            {
                std::cout << "fork() failed.\n";
                return false;
            }

            if (pid == 0)
            {
                dup2(outPipe[1], STDOUT_FILENO);
                dup2(errPipe[1], STDERR_FILENO);

                close(outPipe[0]);
                close(outPipe[1]);
                close(errPipe[0]);
                close(errPipe[1]);

                std::string cmd = "\"" + javapath + "\" " + args;
                execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
                _exit(1);
            }

            close(outPipe[1]);
            close(errPipe[1]);

            std::thread([fd = outPipe[0]]() {
                char buf[1024];
                ssize_t n;
                while ((n = read(fd, buf, sizeof(buf))) > 0)
                    std::cout.write(buf, n);
                close(fd);
            }).detach();

            std::thread([fd = errPipe[0]]() {
                char buf[1024];
                ssize_t n;
                while ((n = read(fd, buf, sizeof(buf))) > 0)
                    std::cout.write(buf, n);
                close(fd);
            }).detach();

            std::thread([pid]() {
                int status;
                waitpid(pid, &status, 0);
            }).detach();

            return true;
            #else
            std::cout << "Please choose the correct OS.\n";
            return false;
            #endif
        }
    }
    return false;
}

}
