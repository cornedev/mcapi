#include "api.hpp"

namespace mcapi
{

bool StartProcess(const std::string& javapath, const std::string& args, OS os, Processhandle* process, bool qt)
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

            HANDLE outread = nullptr, outwrite = nullptr;
            HANDLE errread = nullptr, errwrite = nullptr;

            if (!CreatePipe(&outread, &outwrite, &sa, 0) ||
                !SetHandleInformation(outread, HANDLE_FLAG_INHERIT, 0))
            {
                std::cout << "Failed to create stdout pipe.\n";
                return false;
            }

            if (!CreatePipe(&errread, &errwrite, &sa, 0) ||
                !SetHandleInformation(errread, HANDLE_FLAG_INHERIT, 0))
            {
                std::cout << "Failed to create stderr pipe.\n";
                return false;
            }

            STARTUPINFOA si{};
            PROCESS_INFORMATION pi{};
            si.cb = sizeof(si);
            si.hStdOutput = outwrite;
            si.hStdError  = errwrite;
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

            CloseHandle(outwrite);
            CloseHandle(errwrite);

            if (!success)
            {
                std::cout << "Failed to start Java.\n";
                return false;
            }
            *process = pi.hProcess;
            CloseHandle(pi.hThread);

            std::thread([outread, qt]() {
                char buf[1024];
                DWORD read;
                while (ReadFile(outread, buf, sizeof(buf) - 1, &read, nullptr) && read > 0)
                {
                    buf[read] = 0;
                    if (qt)
                        qDebug() << buf;
                    else
                        std::cout << buf;
                }
                CloseHandle(outread);
            }).detach();

            std::thread([errread, qt]() {
                char buf[1024];
                DWORD read;
                while (ReadFile(errread, buf, sizeof(buf) - 1, &read, nullptr) && read > 0)
                {
                    buf[read] = 0;
                    if (qt)
                        qDebug() << buf;
                    else
                        std::cout << buf;
                }
                CloseHandle(errread);
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

            int outpipe[2];
            int errpipe[2];

            if (pipe(outpipe) != 0 || pipe(errpipe) != 0)
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
                dup2(outpipe[1], STDOUT_FILENO);
                dup2(errpipe[1], STDERR_FILENO);

                close(outpipe[0]);
                close(outpipe[1]);
                close(errpipe[0]);
                close(errpipe[1]);

                std::string cmd = "\"" + javapath + "\" " + args;
                execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);
                _exit(1);
            }
            *process = pid;

            close(outpipe[1]);
            close(errpipe[1]);

            std::thread([fd = outpipe[0], qt]() {
                char buf[1024];
                ssize_t n;
                while ((n = read(fd, buf, sizeof(buf))) > 0)
                    if (qt)
                        qDebug() << QString::fromUtf8(buf, n);
                    else
                        std::cout.write(buf, n);
                close(fd);
            }).detach();

            std::thread([fd = errpipe[0], qt]() {
                char buf[1024];
                ssize_t n;
                while ((n = read(fd, buf, sizeof(buf))) > 0)
                    if (qt)
                        qDebug() << QString::fromUtf8(buf, n);
                    else
                        std::cout.write(buf, n);
                close(fd);
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

bool StopProcess(Processhandle* process)
{
    #ifdef _WIN32
    if (!process || !*process)
        return false;

    DWORD exitcode = 0;
    if (!GetExitCodeProcess(*process, &exitcode))
        return false;

    if (exitcode != STILL_ACTIVE)
        return false;

    TerminateProcess(*process, 0);
    CloseHandle(*process);
    return true;
    #else
    if (!process || *process <= 0)
        return false;

    if (kill(*process, SIGTERM) != 0)
        return false;

    return true;
    #endif
}

bool DetectProcess(Processhandle* process)
{
    #ifdef _WIN32
    if (!process || !*process)
        return false;

    DWORD exitcode = 0;
    if (!GetExitCodeProcess(*process, &exitcode))
        return false;

    return exitcode == STILL_ACTIVE;
    #else
    if (!process || *process <= 0)
        return false;

    if (kill(*process, 0) == 0)
        return true;

    return false;
    #endif
}

}
