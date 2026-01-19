#include <iostream>
#include <string>
#include <fstream>
#include "../api/api.hpp"

static mcapi::Processhandle process{};
int main()
{
    curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
    std::cout << "curl SSL backend: " << info->ssl_version << "\n";
    while (true)
    {
        std::cout << "mcapi clilauncher 1.0\n";
        std::cout << "1. Download all required assets for minecraft.\n";
        std::cout << "2. Download java for minecraft.\n";
        std::cout << "3. Launch minecraft.\n";
        std::cout << "4. Download server jar.\n";
        std::cout << "5. Launch server.\n";
        std::cout << "6. Exit.\n";
        std::cout << "Choose an option: ";

        int choice;
        std::cin >> choice;

        if (std::cin.fail())
        {
            std::cin.clear();
            std::cin.ignore(9999, '\n');
            std::cout << "Invalid input!\n\n";
            continue;
        }

        switch (choice)
        {
        case 1:
        {
            std::cout << "Enter Minecraft version (e.g. 1.21): ";
            std::string versionid;
            std::cin >> versionid;

            // download manifest.
            std::cout << "Downloading version manifest...\n";
            auto manifest = mcapi::DownloadVersionManifest();
            if (manifest.empty())
            {
                std::cout << "Failed to download manifest.\n";
                break;
            }
            std::cout << "Version manifest downloaded.\n";

            // download version json.
            auto versionjsonurl = mcapi::GetVersionJsonDownloadUrl(manifest, versionid);
            if (!versionjsonurl)
            {
                std::cout << "Version not found in manifest.\n";
                break;
            }
            auto versionurl = *versionjsonurl;

            std::cout << "Downloading version json...\n";
            auto versionjsonopt = mcapi::DownloadVersionJson(versionurl, versionid);
            if (!versionjsonopt)
            {
                std::cout << "Failed to download version json.\n";
                break;
            }
            std::cout << "Version json downloaded.\n";
            auto versionjson = *versionjsonopt;

            // download client jar.
            auto jarurlopt = mcapi::GetClientJarDownloadUrl(versionjson);
            if (!jarurlopt)
            {
                std::cout << "Failed to get client jar url.\n";
                break;
            }
            auto jarurl = *jarurlopt;

            std::cout << "Downloading client jar...\n";
            auto clientjar = mcapi::DownloadClientJar(jarurl, versionid);
            if (!clientjar)
            {
                std::cout << "Failed to download client jar.\n";
                break;
            }
            std::cout << "Client jar downloaded.\n";

            // download asset index.
            auto indexurlopt = mcapi::GetAssetIndexJsonDownloadUrl(versionjson);
            if (!indexurlopt)
            {
                std::cout << "Failed to get asset index URL.\n";
                break;
            }
            auto indexurl = *indexurlopt;

            std::cout << "Downloading Asset index\n";
            auto assetjsonopt = mcapi::DownloadAssetIndexJson(indexurl, versionid);
            if (!assetjsonopt)
            {
                std::cout << "Failed to download asset index.\n";
                break;
            }
            auto assetjson = *assetjsonopt;
            std::cout << "Asset index downloaded.\n";

            // download assets.
            auto assetsurlopt = mcapi::GetAssetsDownloadUrl(assetjson);
            if (!assetsurlopt)
            {
                std::cout << "Failed to get assets download urls.\n";
                break;
            }

            std::cout << "Downloading assets... (this may take a while)\n";
            auto assets = mcapi::DownloadAssets(*assetsurlopt, versionid);
            if (!assets)
            {
                std::cout << "Failed to download assets.\n";
                break;
            }
            std::cout << "Assets downloaded.\n\n";
            std::cout << "Done!\n";
            break;
        }

        case 2:
        {
            std::cout << "Enter Minecraft version (use the same one that you used when downloading assets!): ";
            std::string versionid;
            std::cin >> versionid;

            // download manifest.
            std::cout << "Downloading version manifest...\n";
            auto manifest = mcapi::DownloadVersionManifest();
            if (manifest.empty())
            {
                std::cout << "Failed to download manifest.\n";
                break;
            }
            std::cout << "Version manifest downloaded.\n";

            // download version json.
            auto versionjsonurl = mcapi::GetVersionJsonDownloadUrl(manifest, versionid);
            if (!versionjsonurl)
            {
                std::cout << "Version not found in manifest.\n";
                break;
            }
            auto versionurl = *versionjsonurl;

            std::cout << "Downloading version json...\n";
            auto versionjsonopt = mcapi::DownloadVersionJson(versionurl, versionid);
            if (!versionjsonopt)
            {
                std::cout << "Failed to download version json.\n";
                break;
            }
            std::cout << "Version json downloaded.\n";
            auto versionjson = *versionjsonopt;

            std::cout << "Enter OS (choose from: windows, linux, macos): ";
            std::string os;
            std::cin >> os;
            std::cout << "Enter architecture (choose from: x64, x32, arm64): ";
            std::string arch;
            std::cin >> arch;

            // conversion.
            mcapi::OS osenum;
            if (os == "windows")
                osenum = mcapi::OS::Windows;
            else if (os == "linux")
                osenum = mcapi::OS::Linux;
            else if (os == "macos")
                osenum = mcapi::OS::Macos;
            else
            {
                std::cerr << "Invalid OS.\n";
                return 1;
            }
            mcapi::Arch archenum;
            if (arch == "x64")
                archenum = mcapi::Arch::x64;
            else if (arch == "x32")
                archenum = mcapi::Arch::x32;
            else if (arch == "arm64")
                archenum = mcapi::Arch::arm64;
            else
            {
                std::cerr << "Invalid architecture.\n";
                return 1;
            }

            // download java.
            auto javaversionopt = mcapi::GetJavaVersion(versionjson);
            if (!javaversionopt)
            {
                std:: cout << "Failed to get java version.\n";
                break;
            }
            int javaversion = *javaversionopt;

            auto javaurlopt = mcapi::GetJavaDownloadUrl(javaversion, osenum, archenum);
            if (!javaurlopt)
            {
                std:: cout << "Failed to get java download url.\n";
                break;
            }
            auto javaurl = *javaurlopt;

            std::cout << "Downloading java...\n";
            auto javaopt = mcapi::DownloadJava(javaurl, versionid);
            if (!javaopt)
            {
                std:: cout << "Failed to download java.\n";
                break;
            }
            std::cout << "Java downloaded.\n";
            auto java = *javaopt;
            std::cout << "Done!\n\n";
            break;
        }

        case 3:
        {
            std::cout << "Enter Minecraft version (use the same one that you used when downloading assets and when downloading java!): ";
            std::string versionid;
            std::cin >> versionid;
            std::cout << "Enter username: ";
            std::string username;
            std::cin >> username;

            // download manifest.
            std::cout << "Downloading version manifest...\n";
            auto manifest = mcapi::DownloadVersionManifest();
            if (manifest.empty())
            {
                std::cout << "Failed to download manifest.\n";
                break;
            }
            std::cout << "Version manifest downloaded\n";

            // download version json.
            auto versionjsonurl = mcapi::GetVersionJsonDownloadUrl(manifest, versionid);
            if (!versionjsonurl)
            {
                std::cout << "Version not found in manifest.\n";
                break;
            }
            auto versionurl = *versionjsonurl;

            std::cout << "Downloading version json...\n";
            auto versionjsonopt = mcapi::DownloadVersionJson(versionurl, versionid);
            if (!versionjsonopt)
            {
                std::cout << "Failed to download version json.\n";
                break;
            }
            std::cout << "Version json downloaded.\n";
            auto versionjson = *versionjsonopt;

            std::cout << "Enter OS (choose from: windows, linux, macos): ";
            std::string os;
            std::cin >> os;
            std::cout << "Enter architecture (choose from: x64, x32, arm64): ";
            std::string arch;
            std::cin >> arch;

            // conversion.
            mcapi::OS osenum;
            if (os == "windows")
                osenum = mcapi::OS::Windows;
            else if (os == "linux")
                osenum = mcapi::OS::Linux;
            else if (os == "macos")
                osenum = mcapi::OS::Macos;
            else
            {
                std::cerr << "Invalid OS.\n";
                return 1;
            }
            mcapi::Arch archenum;
            if (arch == "x64")
                archenum = mcapi::Arch::x64;
            else if (arch == "x32")
                archenum = mcapi::Arch::x32;
            else if (arch == "arm64")
                archenum = mcapi::Arch::arm64;
            else
            {
                std::cerr << "Invalid architecture.\n";
                return 1;
            }

            // download libraries.
            auto librariesurlopt = mcapi::GetLibrariesDownloadUrl(versionjson, osenum);
            if (!librariesurlopt)
            {
                std::cout << "Failed to get libraries.\n";
                break;
            }

            std::cout << "Downloading libraries...\n";
            auto librariesdownloaded = mcapi::DownloadLibraries(*librariesurlopt, versionid);
            if (!librariesdownloaded)
            {
                std::cout << "Failed to download libraries.\n";
                break;
            }
            std::cout << "Libraries downloaded.\n";

            // extract natives.
            std::cout << "Extracting natives...\n";
            auto nativesurlopt = mcapi::GetLibrariesNatives(versionid, versionjson, osenum, archenum);
            if (!nativesurlopt)
            {
                std::cout << "Failed to get natives urls.\n";
                break;
            }

            std::cout << "Downloading natives...\n";
            auto nativesjarsopt = mcapi::DownloadLibrariesNatives(*nativesurlopt, versionid);
            if (!nativesjarsopt)
            {
                std::cout << "Failed to download native jars.\n";
                break;
            }

            auto nativesextracted = mcapi::ExtractLibrariesNatives(*nativesjarsopt, versionid, osenum);
            if (!nativesextracted)
            {
                std::cout << "Failed to extract natives.\n";
                break;
            }
            std::cout << "Natives extracted.\n";

            // build classpath.
            std::cout << "Building classpath...\n";
            fs::path datapath = ".mcapi";
            auto classpathopt = mcapi::GetClassPath(versionjson, *librariesdownloaded, (datapath / "versions" / versionid / "client.jar").string(), osenum);
            if (!classpathopt)
            {
                std::cout << "Failed to build classpath.\n";
                break;
            }
            auto classpath = *classpathopt;
            std::cout << "Classpath built.\n";

            // build launch command.
            std::cout << "Building launch command...\n";
            auto launchcmdopt = mcapi::GetLaunchCommand(username, classpath, versionjson, versionid, osenum);
            if (!launchcmdopt)
            {
                std::cout << "Failed to build launch command.\n";
                break;
            }
            auto launchcmd = *launchcmdopt;
            std::cout << "Launch command built.\n";

            // launch minecraft.
            std::string javapath;
            switch (osenum)
            {
                case mcapi::OS::Windows:
                    javapath = "runtime/" + versionid + "/java/bin/java.exe";
                    break;

                case mcapi::OS::Linux:
                case mcapi::OS::Macos:
                    javapath = "runtime/" + versionid + "/java/bin/java";
                    break;
            }
            bool launched = mcapi::StartProcess(javapath, launchcmd, osenum, &process);
            
            if (!launched)
                std::cout << "Failed to launch Minecraft.\n";
            else
                std::cout << "Minecraft launched.\n";
            break;
        }

        case 4:
        {
            std::cout << "Enter Minecraft version: ";
            std::string versionid;
            std::cin >> versionid;

            // download manifest.
            std::cout << "Downloading version manifest...\n";
            auto manifest = mcapi::DownloadVersionManifest();
            if (manifest.empty())
            {
                std::cout << "Failed to download manifest.\n";
                break;
            }
            std::cout << "Version manifest downloaded.\n";

            // download version json.
            auto versionjsonurl = mcapi::GetVersionJsonDownloadUrl(manifest, versionid);
            if (!versionjsonurl)
            {
                std::cout << "Version not found in manifest.\n";
                break;
            }
            auto versionurl = *versionjsonurl;

            std::cout << "Downloading version json...\n";
            auto versionjsonopt = mcapi::DownloadVersionJson(versionurl, versionid);
            if (!versionjsonopt)
            {
                std::cout << "Failed to download version json.\n";
                break;
            }
            std::cout << "Version json downloaded.\n";
            auto versionjson = *versionjsonopt;

            // download server jar.
            auto serverjarurlopt = mcapi::GetServerJarDownloadUrl(versionjson);
            if (!serverjarurlopt)
            {
                std::cout << "Could not find server jar in version json.\n";
                break;
            }
            auto serverjarurl = *serverjarurlopt;

            std::cout << "Downloading server jar...\n";
            auto versionjaropt = mcapi::DownloadServerJar(serverjarurl, versionid);
            if (!versionjaropt)
            {
                std::cout << "Could not download server jar.\n";
                break;
            }
            std::cout << "Server jar downloaded.\n\n";
            std::cout << "Done!\n\n";
            break;
        }

        case 5:
        {
            std::cout << "WIP.";
            break;
        }

        case 6:
        {
            std::cout << "Exiting...";
            return 0;
        }

        default:
            std::cout << "Invalid choice!\n";
        }
        std::cin.ignore();
        std::cin.get();
    }
    return 0;
}