#include "gui.h"
#include "ui_gui.h"

static mcapi::Processhandle process{};
static gui* instance = nullptr;

void ConsoleHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    if (instance) {
        QMetaObject::invokeMethod(instance, [msg]() {
            instance->findChild<QPlainTextEdit*>("consolelog")->appendPlainText(msg);
        }, Qt::QueuedConnection);
    }
}

gui::gui(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::gui)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

    instance = this;
    qInstallMessageHandler(ConsoleHandler);

    // consolelog box.
    ui->consolelog->setReadOnly(true);

    // - version box.
    ui->versionbox->setStyleSheet("QComboBox { combobox-popup: 0; }");
    ui->versionbox->setMaxVisibleItems(10);
    GetVersions();
    connect(ui->versionbox, &QComboBox::currentTextChanged, this, &gui::on_versioncombo_changed);

    // - os box.
    ui->osbox->setStyleSheet("QComboBox { combobox-popup: 0; }");
    ui->osbox->addItem("windows");
    ui->osbox->addItem("linux");
    ui->osbox->addItem("macos");
    osselected = ui->osbox->currentText();
    connect(ui->osbox, &QComboBox::currentTextChanged, this, &gui::on_oscombo_changed);

    // - arch box.
    ui->archbox->setStyleSheet("QComboBox { combobox-popup: 0; }");
    ui->archbox->addItem("x64");
    ui->archbox->addItem("arm64");
    ui->archbox->addItem("x32");
    archselected = ui->archbox->currentText();
    connect(ui->archbox, &QComboBox::currentTextChanged, this, &gui::on_archcombo_changed);

    // - username input.
    connect(ui->usernameinput, &QLineEdit::textChanged, this, &gui::on_usernameinput_changed);
}

gui::~gui()
{
    qInstallMessageHandler(0);
    instance = nullptr;
    delete ui;
}

void gui::on_versioncombo_changed(const QString &version)
{
    versionselected = version;
}

void gui::on_oscombo_changed(const QString &os)
{
    osselected = os;
}

void gui::on_archcombo_changed(const QString &arch)
{
    archselected = arch;
}

void gui::on_usernameinput_changed(const QString &input)
{
    username = input;
}

void gui::GetVersions()
{
    ui->versionbox->clear();

    try
    {
        manifest = mcapi::DownloadVersionManifest();
        auto versions = mcapi::GetVersionsFromManifest(manifest);

        if (!versions || versions->empty())
            throw std::runtime_error("Failed to get versions.");

        for (const auto &v : *versions)
        {
            ui->versionbox->addItem(QString::fromStdString(v));
        }

        versionselected = ui->versionbox->currentText();
    }
    catch (...)
    {
        ui->versionbox->clear();
        ui->versionbox->addItem("1.21.11");
        ui->versionbox->addItem("1.8.9");
        versionselected = ui->versionbox->currentText();
    }
}

static QString GetOfflineClassPath(const QString& basepath)
{
    QStringList jars;
    QDirIterator iterator(basepath, QStringList() << "*.jar", QDir::Files, QDirIterator::Subdirectories);

    while (iterator.hasNext())
    {
        jars << iterator.next();
    }

    #ifdef Q_OS_WIN
    return jars.join(";");
    #else
    return jars.join(":");
    #endif
}

bool gui::StartVersion(const QString &username, const QString &versionselected, const QString &archselected, const QString &osselected)
{
    bool offlinemode = ui->schoolcheckbox->isChecked();

    // - offline fallbacks.
    if (offlinemode)
    {
        if (versionselected == "1.21.11")
        {
            qDebug() << "Launching minecraft with offline fallback.";
            mcapi::OS osenum = (osselected == "windows") ? mcapi::OS::Windows : (osselected == "linux") ? mcapi::OS::Linux : mcapi::OS::Macos;

            QString exedir = QCoreApplication::applicationDirPath();
            QString javapath = exedir + "/runtime/1.21.11/java/bin/" + (osenum == mcapi::OS::Windows ? "java.exe" : "java");
            QString librariespath = exedir + "/.mcapi/1.21.11/libraries";
            QString clientjarpath = exedir + "/.mcapi/versions/1.21.11/client.jar";
            QString assetspath = ".mcapi/1.21.11/assets";
            QString gamepath = ".mcapi/versions/1.21.11";
            QString nativespath = ".mcapi/1.21.11/natives";

            #ifdef Q_OS_WIN
            QString sep = ";";
            #else
            QString sep = ":";
            #endif

            QString classpath = GetOfflineClassPath(librariespath) + sep + clientjarpath;

            std::string launchcmd =
                "-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump "
                "-Xss1M "
                "-Djava.library.path=" + nativespath.toStdString() + " "
                "-Djna.tmpdir=" + nativespath.toStdString() + " "
                "-Dorg.lwjgl.system.SharedLibraryExtractPath=" + nativespath.toStdString() + " "
                "-Dio.netty.native.workdir=" + nativespath.toStdString() + " "
                "-Dminecraft.launcher.brand=mcapi "
                "-Dminecraft.launcher.version=1.0 "
                "-cp " + classpath.toStdString() +
                " net.minecraft.client.main.Main"
                " --username " + username.toStdString() +
                " --version 1.21.11"
                " --gameDir " + gamepath.toStdString() +
                " --assetsDir " + assetspath.toStdString() +
                " --assetIndex 29"
                " --uuid 00000000-0000-0000-0000-000000000000"
                " --accessToken 0"
                " --versionType release";

            bool launched = mcapi::StartProcess(javapath.toStdString(), launchcmd, osenum, &process);
            if (!launched)
            {
                qDebug() << "Failed to launch Minecraft. (offline fallback)";
                return false;
            }
            else
            {
                qDebug() << "Minecraft launched. (offline fallback)";
                return true;
            }
        }

        if (versionselected == "1.8.9")
        {
            qDebug() << "Launching minecraft with offline fallback.";
            mcapi::OS osenum = (osselected == "windows") ? mcapi::OS::Windows : (osselected == "linux") ? mcapi::OS::Linux : mcapi::OS::Macos;

            QString exedir = QCoreApplication::applicationDirPath();
            QString javapath = exedir + "/runtime/1.8.9/java/bin/" + (osenum == mcapi::OS::Windows ? "java.exe" : "java");
            QString librariespath = exedir + "/.mcapi/1.8.9/libraries";
            QString clientjarpath = exedir + "/.mcapi/versions/1.8.9/client.jar";
            QString assetspath = ".mcapi/1.8.9/assets";
            QString gamepath = ".mcapi/versions/1.8.9";
            QString nativespath = ".mcapi/1.8.9/natives";

            #ifdef Q_OS_WIN
            QString sep = ";";
            #else
            QString sep = ":";
            #endif

            QString classpath = GetOfflineClassPath(librariespath) + sep + clientjarpath;

            std::string launchcmd =
                "-Xmx2G "
                "-Xms1G "
                "-Djava.library.path=" + nativespath.toStdString() + " "
                "-cp " + classpath.toStdString() +
                " net.minecraft.client.main.Main"
                " --username " + username.toStdString() +
                " --version 1.8.9"
                " --gameDir " + gamepath.toStdString() +
                " --assetsDir " + assetspath.toStdString() +
                " --assetIndex 1.8"
                " --uuid 00000000-0000-0000-0000-000000000000"
                " --accessToken 0"
                " --userProperties {}"
                " --userType mojang";

            bool launched = mcapi::StartProcess(javapath.toStdString(), launchcmd, osenum, &process);
            if (!launched)
            {
                qDebug() << "Failed to launch Minecraft. (offline fallback)";
                return false;
            }
            else
            {
                qDebug() << "Minecraft launched. (offline fallback)";
                return true;
            }
        }
        qDebug() << "Minecraft version not supported on offline fallback.";
    }
    // - end offline fallbacks.

    // - download version json.
    auto versionjsonurl = mcapi::GetVersionJsonDownloadUrl(manifest, versionselected.toStdString());
    if (!versionjsonurl)
    {
        qDebug() << "Version not found in manifest.";
        return false;
    }
    auto versionurl = *versionjsonurl;

    qDebug() << "Downloading version json...";
    auto versionjsonopt = mcapi::DownloadVersionJson(versionurl, versionselected.toStdString());
    if (!versionjsonopt)
    {
        qDebug() << "Failed to download version json.";
        return false;
    }
    qDebug() << "Version json downloaded.";
    auto versionjson = *versionjsonopt;

    // - download client jar.
    auto jarurlopt = mcapi::GetClientJarDownloadUrl(versionjson);
    if (!jarurlopt)
    {
        qDebug() << "Failed to get client jar url.";
        return false;
    }
    auto jarurl = *jarurlopt;

    qDebug() << "Downloading client jar...";
    auto clientjar = mcapi::DownloadClientJar(jarurl, versionselected.toStdString());
    if (!clientjar)
    {
        qDebug() << "Failed to download client jar.";
        return false;
    }
    qDebug() << "Client jar downloaded.";

    // - download asset index.
    auto indexurlopt = mcapi::GetAssetIndexJsonDownloadUrl(versionjson);
    if (!indexurlopt)
    {
        qDebug() << "Failed to get asset index URL.";
        return false;
    }
    auto indexurl = *indexurlopt;

    qDebug() << "Downloading Asset index...";
    auto assetjsonopt = mcapi::DownloadAssetIndexJson(indexurl, versionselected.toStdString());
    if (!assetjsonopt)
    {
        qDebug() << "Failed to download asset index.";
        return false;
    }
    auto assetjson = *assetjsonopt;
    qDebug() << "Asset index downloaded.";

    // - download assets.
    auto assetsurlopt = mcapi::GetAssetsDownloadUrl(assetjson);
    if (!assetsurlopt)
    {
        qDebug() << "Failed to get assets download urls.";
        return false;
    }

    qDebug() << "Downloading assets... (this may take a while)";
    auto assets = mcapi::DownloadAssets(*assetsurlopt, versionselected.toStdString());
    if (!assets)
    {
        qDebug() << "Failed to download assets.";
        return false;
    }
    qDebug() << "Assets downloaded.";

    // - conversion.
    mcapi::OS osenum;
    if (osselected == "windows")
        osenum = mcapi::OS::Windows;
    else if (osselected == "linux")
        osenum = mcapi::OS::Linux;
    else if (osselected == "macos")
        osenum = mcapi::OS::Macos;
    else
    {
        qDebug() << "Invalid OS.";
        return false;
    }
    mcapi::Arch archenum;
    if (archselected == "x64")
        archenum = mcapi::Arch::x64;
    else if (archselected == "x32")
        archenum = mcapi::Arch::x32;
    else if (archselected == "arm64")
        archenum = mcapi::Arch::arm64;
    else
    {
        qDebug() << "Invalid architecture.";
        return false;
    }

    // - download java.
    auto javaversionopt = mcapi::GetJavaVersion(versionjson);
    if (!javaversionopt)
    {
        qDebug() << "Failed to get java version.";
        return false;
    }
    int javaversion = *javaversionopt;

    auto javaurlopt = mcapi::GetJavaDownloadUrl(javaversion, osenum, archenum);
    if (!javaurlopt)
    {
        qDebug() << "Failed to get java download url.";
        return false;
    }
    auto javaurl = *javaurlopt;

     qDebug() << "Downloading java...";
    auto javaopt = mcapi::DownloadJava(javaurl, versionselected.toStdString());
    if (!javaopt)
    {
        qDebug() << "Failed to download java.";
        return false;
    }
    qDebug() << "Java downloaded.";
    auto java = *javaopt;

    // - download libraries.
    auto librariesurlopt = mcapi::GetLibrariesDownloadUrl(versionjson, osenum);
    if (!librariesurlopt)
    {
        qDebug() << "Failed to get libraries.";
        return false;
    }

    qDebug() << "Downloading libraries...";
    auto librariesdownloaded = mcapi::DownloadLibraries(*librariesurlopt, versionselected.toStdString());
    if (!librariesdownloaded)
    {
        qDebug() << "Failed to download libraries.";
        return false;
    }
    qDebug() << "Libraries downloaded.";

    // - extract natives.
    qDebug() << "Extracting natives...";
    auto nativesurlopt = mcapi::GetLibrariesNatives(versionselected.toStdString(), versionjson, osenum, archenum);
    if (!nativesurlopt)
    {
        qDebug() << "Failed to get natives urls.";
        return false;
    }

    qDebug() <<"Downloading natives...";
    auto nativesjarsopt = mcapi::DownloadLibrariesNatives(*nativesurlopt, versionselected.toStdString());
    if (!nativesjarsopt)
    {
        qDebug() << "Failed to download native jars.";
        return false;
    }

    auto nativesextracted = mcapi::ExtractLibrariesNatives(*nativesjarsopt, versionselected.toStdString(), osenum);
    if (!nativesextracted)
    {
        qDebug() << "Failed to extract natives.";
        return false;
    }
    qDebug() << "Natives extracted.";

    // - build classpath.
    qDebug() << "Building classpath...";
    fs::path datapath = ".mcapi";
    auto classpathopt = mcapi::GetClassPath(versionjson, *librariesdownloaded, (datapath / "versions" / versionselected.toStdString() / "client.jar").string(), osenum);
    if (!classpathopt)
    {
        qDebug() << "Failed to build classpath.";
        return false;
    }
    auto classpath = *classpathopt;
    qDebug() << "Classpath built.";

    // - build launch command.
    qDebug() << "Building launch command...";
    auto launchcmdopt = mcapi::GetLaunchCommand(username.toStdString(), classpath, versionjson, versionselected.toStdString(), osenum);
    if (!launchcmdopt)
    {
        qDebug() << "Failed to build launch command.";
        return false;
    }
    auto launchcmd = *launchcmdopt;
    qDebug() << "Launch command built.";

    // - launch minecraft.
    std::string javapath;
    switch (osenum)
    {
    case mcapi::OS::Windows:
        javapath = "runtime/" + versionselected.toStdString() + "/java/bin/java.exe";
        break;

    case mcapi::OS::Linux:
    case mcapi::OS::Macos:
        javapath = "runtime/" + versionselected.toStdString() + "/java/bin/java";
        break;
    }
    bool launched = mcapi::StartProcess(javapath, launchcmd, osenum, &process);
    if (!launched)
    {
        qDebug() << "Failed to launch Minecraft.";
        return false;
    }
    else
    {
        qDebug() << "Minecraft launched.";
        return true;
    }
}

void gui::on_startbutton_clicked()
{
    if (running.exchange(true))
        return;

    if (ui->usernameinput->text().trimmed().isEmpty())
    {
        qDebug() << "Please enter an username.";
        running = false;
        return;
    }

    ui->startbutton->setEnabled(false);

    QFuture<void> future = QtConcurrent::run([this]() {
        if (!StartVersion(username, versionselected, archselected, osselected))
        {
            running = false;
            QMetaObject::invokeMethod(ui->startbutton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
            return;
        }
        while (mcapi::DetectProcess(&process))
        {
            QThread::sleep(1);
        }
        running = false;
        QMetaObject::invokeMethod(ui->startbutton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    });
}

void gui::on_stopbutton_clicked()
{
    if (!mcapi::StopProcess(&process))
    {
        qDebug() << "Failed to stop Minecraft.";
    }
    else
    {
        qDebug() << "Minecraft stopped.";
        running = false;
        ui->startbutton->setEnabled(true);
    }
}
