#include "gui.h"
#include "ui_gui.h"

static mcapi::Processhandle process{};
static gui* instance = nullptr;

void ConsoleHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (instance && instance->consolewindow)
    {
        QMetaObject::invokeMethod(instance->consolewindow, [msg]() {
            instance->consolewindow->appendmessage(msg);
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

    // - cancel login button.
    ui->logincancelbutton->hide();

    // - pages.
    ui->pages->setCurrentIndex(0);

    // - play offline button.
    connect(ui->offlinebutton, &QPushButton::clicked, [this]()
    {
        ui->pages->setCurrentIndex(1);
    });

    // - use latest login checkbox.
    connect(ui->latestlogin, &QCheckBox::toggled, this, [this](bool checked)
    {
        microsoftlatestlogin = checked;
    });

    // - back button.
    connect(ui->backbutton, &QPushButton::clicked, [this]()
    {
        ui->pages->setCurrentIndex(0);
        ui->usernameinput->setText("");
        ui->usernameinput->setEnabled(true);
    });

    // - console window and console button.
    consolewindow = new console(this);
    consolewindow->setWindowFlags(Qt::Window);

    connect(consolewindow, &QObject::destroyed, [this]()
    {
        consolewindow = nullptr;
    });
    consolewindow->hide();

    connect(ui->consolebutton, &QPushButton::clicked, [this]()
    {
        if (consolewindow)
        {
            consolewindow->show();
            consolewindow->raise();
            consolewindow->activateWindow();
        }
    });

    // - loader box.
    ui->loaderbox->setStyleSheet("QComboBox { combobox-popup: 0; }");
    ui->loaderbox->setMaxVisibleItems(10);
    ui->loaderbox->clear();
    ui->loaderbox->addItem("vanilla");
    ui->loaderbox->addItem("fabric");
    loaderselected = ui->loaderbox->currentText();
    connect(ui->loaderbox, &QComboBox::currentTextChanged, this, &gui::on_loadercombo_changed);

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
    mcapi::auth::StopMicrosoftLoginListener();
    qInstallMessageHandler(0);
    instance = nullptr;
    delete ui;
}

void gui::on_loadercombo_changed(const QString &loader)
{
    loaderselected = loader;
    GetVersions();
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
    ui->versionbox->blockSignals(true);
    ui->versionbox->clear();

    std::optional<std::vector<std::string>> versions;

    if (loaderselected == "vanilla")
    {
        if (versionsvanilla)
        {
            versions = versionsvanilla;
        }
        else
        {
            auto manifest = mcapi::vanilla::DownloadVersionManifest();
            versions = mcapi::vanilla::GetVersionsFromManifest(*manifest);
            versionsvanilla = versions;
        }
    }
    else if (loaderselected == "fabric")
    {
        if (versionsfabric)
        {
            versions = versionsfabric;
        }
        else
        {
            auto meta = mcapi::fabric::DownloadVersionMeta();
            versions = mcapi::fabric::GetVersionsFromMeta(*meta);
            versionsfabric = versions;
        }
    }

    if (!versions || versions->empty())
    {
        qDebug() << "Failed to get versions, using fallback.";

        ui->versionbox->clear();
        if (loaderselected == "vanilla")
        {
            ui->versionbox->addItem("1.21.11");
            ui->versionbox->addItem("1.8.9");
        }
        else if (loaderselected == "fabric")
        {
            ui->versionbox->addItem("1.21.11");
        }
        versionselected = ui->versionbox->currentText();
    }
    else
    {
        for (const auto &v : *versions)
        {
            ui->versionbox->addItem(QString::fromStdString(v));
        }
    }

    versionselected = ui->versionbox->currentText();
    ui->versionbox->blockSignals(false);
}

bool gui::StartVersion(const QString &username, const QString &loaderselected, const QString &versionselected, const QString &archselected, const QString &osselected)
{
    if (loaderselected == "vanilla")
    {
        // - download manifest.
        auto manifestopt = mcapi::vanilla::DownloadVersionManifest();
        if (!manifestopt)
        {
            qDebug() << "Failed to download manifest.";
            return false;
        }
        auto manifest = *manifestopt;
        
        // - download version json.
        auto versionjsonurl = mcapi::vanilla::GetVersionJsonDownloadUrl(manifest, versionselected.toStdString());
        if (!versionjsonurl)
        {
            qDebug() << "Version not found in manifest.";
            return false;
        }
        auto versionurl = *versionjsonurl;

        qDebug() << "Downloading version json...";
        auto versionjsonopt = mcapi::vanilla::DownloadVersionJson(versionurl, versionselected.toStdString());
        if (!versionjsonopt)
        {
            qDebug() << "Failed to download version json (are you offline?).";
            return false;
        }
        qDebug() << "Version json downloaded.";
        auto versionjson = *versionjsonopt;

        // - download client jar.
        auto jarurlopt = mcapi::vanilla::GetClientJarDownloadUrl(versionjson);
        if (!jarurlopt)
        {
            qDebug() << "Failed to get client jar url.";
            return false;
        }
        auto jarurl = *jarurlopt;

        qDebug() << "Downloading client jar...";
        auto clientjar = mcapi::vanilla::DownloadClientJar(jarurl, versionselected.toStdString());
        if (!clientjar)
        {
            qDebug() << "Failed to download client jar.";
            return false;
        }
        qDebug() << "Client jar downloaded.";

        // - download asset index.
        auto indexurlopt = mcapi::vanilla::GetAssetIndexJsonDownloadUrl(versionjson);
        if (!indexurlopt)
        {
            qDebug() << "Failed to get asset index URL.";
            return false;
        }
        auto indexurl = *indexurlopt;

        qDebug() << "Downloading Asset index...";
        auto assetjsonopt = mcapi::vanilla::DownloadAssetIndexJson(indexurl, versionselected.toStdString());
        if (!assetjsonopt)
        {
            qDebug() << "Failed to download asset index.";
            return false;
        }
        auto assetjson = *assetjsonopt;
        qDebug() << "Asset index downloaded.";

        // - download assets.
        auto assetsurlopt = mcapi::vanilla::GetAssetsDownloadUrl(assetjson);
        if (!assetsurlopt)
        {
            qDebug() << "Failed to get assets download urls.";
            return false;
        }

        qDebug() << "Downloading assets... (this may take a while)";
        auto assets = mcapi::vanilla::DownloadAssets(*assetsurlopt, versionselected.toStdString());
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
        auto librariesurlopt = mcapi::vanilla::GetLibrariesDownloadUrl(versionjson, osenum);
        if (!librariesurlopt)
        {
            qDebug() << "Failed to get libraries.";
            return false;
        }

        qDebug() << "Downloading libraries...";
        auto librariesdownloaded = mcapi::vanilla::DownloadLibraries(*librariesurlopt, versionselected.toStdString());
        if (!librariesdownloaded)
        {
            qDebug() << "Failed to download libraries.";
            return false;
        }
        qDebug() << "Libraries downloaded.";

        // - extract natives.
        qDebug() << "Extracting natives...";
        auto nativesurlopt = mcapi::vanilla::GetLibrariesNatives(versionselected.toStdString(), versionjson, osenum, archenum);
        if (!nativesurlopt)
        {
            qDebug() << "Failed to get natives urls.";
            return false;
        }

        qDebug() <<"Downloading natives...";
        auto nativesjarsopt = mcapi::vanilla::DownloadLibrariesNatives(*nativesurlopt, versionselected.toStdString());
        if (!nativesjarsopt)
        {
            qDebug() << "Failed to download native jars.";
            return false;
        }

        auto nativesextracted = mcapi::vanilla::ExtractLibrariesNatives(*nativesjarsopt, versionselected.toStdString(), osenum);
        if (!nativesextracted)
        {
            qDebug() << "Failed to extract natives.";
            return false;
        }
        qDebug() << "Natives extracted.";

        // - build classpath.
        qDebug() << "Building classpath...";
        fs::path datapath = ".mcapi";
        auto classpathopt = mcapi::vanilla::GetClassPath(versionjson, *librariesdownloaded, (datapath / "versions" / versionselected.toStdString() / "client.jar").string(), osenum);
        if (!classpathopt)
        {
            qDebug() << "Failed to build classpath.";
            return false;
        }
        auto classpath = *classpathopt;
        qDebug() << "Classpath built.";

        // - build launch command depending on whether the user is offline or logged in.
        std::string launchcmd;
        if (microsoft)
        {
            qDebug() << "Building launch command...";
            auto launchcmdopt = mcapi::vanilla::GetLaunchCommand(microsoftusername, classpath, versionjson, versionselected.toStdString(), osenum, microsoftuuid, microsoftaccesstoken, "msa");
            if (!launchcmdopt)
            {
                qDebug() << "Failed to build launch command.";
                return false;
            }
            launchcmd = *launchcmdopt;
            qDebug() << "Launch command built.";
        }
        else
        {
            qDebug() << "Building launch command...";
            auto launchcmdopt = mcapi::vanilla::GetLaunchCommand(username.toStdString(), classpath, versionjson, versionselected.toStdString(), osenum);
            if (!launchcmdopt)
            {
                qDebug() << "Failed to build launch command.";
                return false;
            }
            launchcmd = *launchcmdopt;
            qDebug() << "Launch command built.";
        }

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
        bool launched = mcapi::StartProcess(javapath, launchcmd, osenum, &process, true);
        if (!launched)
        {
            QMetaObject::invokeMethod(this, [this](){QMessageBox::critical(this, "error", "Failed to launch minecraft.");}, Qt::QueuedConnection);
            return false;
        }
        else
        {
            QMetaObject::invokeMethod(this, [this](){QMessageBox::information(this, "info", "Minecraft launched.");}, Qt::QueuedConnection);
            return true;
        }
    }
    else if (loaderselected == "fabric")
    {
        // - download fabric meta.
        auto meta = mcapi::fabric::DownloadVersionMeta();

        // - download meta json that contains the loader version.
        auto loadermetaurlopt = mcapi::fabric::GetLoaderMetaUrl(versionselected.toStdString());
        if (!loadermetaurlopt)
        {
            qDebug() << "Failed to get loader meta url (are you offline?).";
            return false;
        }
        auto loadermetaurl = *loadermetaurlopt;

        auto loadermetaopt = mcapi::fabric::DownloadLoaderMeta(loadermetaurl);
        if (!loadermetaopt)
        {
            qDebug() << "Failed to download loader meta.";
            return false;
        }
        auto loadermeta = *loadermetaopt;
        qDebug() << "Loader meta downloaded.";

        auto loaderopt = mcapi::fabric::GetLoaderVersion(loadermeta);
        if (!loaderopt)
        {
            qDebug() << "Failed to get loader version.";
            return false;
        }
        auto loader = *loaderopt;
        qDebug() << "Meta downloaded.";

        QString versionid = versionselected + "-fabric-loader-" + QString::fromStdString(loader);

        // - download fabric version json.
        auto versionjsonurl = mcapi::fabric::GetLoaderJsonDownloadUrl(loader, versionselected.toStdString());
        if (!versionjsonurl)
        {
            qDebug() << "Version not found in fabric manifest.";
            return false;
        }
        auto versionurl = *versionjsonurl;
        qDebug() << "succes";

        qDebug() << "Downloading version json...";
        auto loaderjsonopt = mcapi::fabric::DownloadLoaderJson(versionurl, loader, versionselected.toStdString());
        if (!loaderjsonopt)
        {
            qDebug() << "Failed to download version json.";
            return false;
        }
        auto loaderjson = *loaderjsonopt;
        qDebug() << "Version json downloaded.";

        // - merge vanilla json with fabric json.
        auto mergedjsonopt = mcapi::fabric::GetLoaderJson(loaderjson, loader, versionselected.toStdString());
        if (!mergedjsonopt)
        {
            qDebug() << "Failed to create merged version json.";
            return false;
        }
        auto mergedjson = *mergedjsonopt;

        // - download client jar.
        auto jarurlopt = mcapi::vanilla::GetClientJarDownloadUrl(mergedjson);
        if (!jarurlopt)
        {
            qDebug() << "Failed to get client jar url.";
            return false;
        }
        auto jarurl = *jarurlopt;

        qDebug() << "Downloading client jar...";
        auto clientjar = mcapi::vanilla::DownloadClientJar(jarurl, versionid.toStdString());
        if (!clientjar)
        {
            qDebug() << "Failed to download client jar.";
            return false;
        }
        qDebug() << "Client jar downloaded.";

        // - download asset index.
        auto indexurlopt = mcapi::vanilla::GetAssetIndexJsonDownloadUrl(mergedjson);
        if (!indexurlopt)
        {
            qDebug() << "Failed to get asset index URL.";
            return false;
        }
        auto indexurl = *indexurlopt;

        qDebug() << "Downloading Asset index...";
        auto assetjsonopt = mcapi::vanilla::DownloadAssetIndexJson(indexurl, versionid.toStdString());
        if (!assetjsonopt)
        {
            qDebug() << "Failed to download asset index.";
            return false;
        }
        auto assetjson = *assetjsonopt;
        qDebug() << "Asset index downloaded.";

        // - download assets.
        auto assetsurlopt = mcapi::vanilla::GetAssetsDownloadUrl(assetjson);
        if (!assetsurlopt)
        {
            qDebug() << "Failed to get assets download urls.";
            return false;
        }

        qDebug() << "Downloading assets... (this may take a while)";
        auto assets = mcapi::vanilla::DownloadAssets(*assetsurlopt, versionid.toStdString());
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
        auto javaversionopt = mcapi::GetJavaVersion(mergedjson);
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
        auto javaopt = mcapi::DownloadJava(javaurl, versionid.toStdString());
        if (!javaopt)
        {
            qDebug() << "Failed to download java.";
            return false;
        }
        qDebug() << "Java downloaded.";
        auto java = *javaopt;

        // - download libraries.
        auto librariesurlopt = mcapi::fabric::GetLoaderLibrariesDownloadUrl(mergedjson, osenum);
        if (!librariesurlopt)
        {
            qDebug() << "Failed to get libraries.";
            return false;
        }

        qDebug() << "Downloading libraries...";
        auto librariesdownloaded = mcapi::vanilla::DownloadLibraries(*librariesurlopt, versionid.toStdString());
        if (!librariesdownloaded)
        {
            qDebug() << "Failed to download libraries.";
            return false;
        }
        qDebug() << "Libraries downloaded.";

        // - extract natives.
        qDebug() << "Extracting natives...";
        auto nativesurlopt = mcapi::vanilla::GetLibrariesNatives(versionid.toStdString(), mergedjson, osenum, archenum);
        if (!nativesurlopt)
        {
            qDebug() << "Failed to get natives urls.";
            return false;
        }

        qDebug() <<"Downloading natives...";
        auto nativesjarsopt = mcapi::vanilla::DownloadLibrariesNatives(*nativesurlopt, versionid.toStdString());
        if (!nativesjarsopt)
        {
            qDebug() << "Failed to download native jars.";
            return false;
        }

        auto nativesextracted = mcapi::vanilla::ExtractLibrariesNatives(*nativesjarsopt, versionid.toStdString(), osenum);
        if (!nativesextracted)
        {
            qDebug() << "Failed to extract natives.";
            return false;
        }
        qDebug() << "Natives extracted.";

        // - build classpath.
        qDebug() << "Building classpath...";
        fs::path datapath = ".mcapi";
        auto classpathopt = mcapi::vanilla::GetClassPath(mergedjson, *librariesdownloaded, (datapath / "versions" / versionid.toStdString() / "client.jar").string(), osenum);
        if (!classpathopt)
        {
            qDebug() << "Failed to build classpath.";
            return false;
        }
        auto classpath = *classpathopt;
        qDebug() << "Classpath built.";

        // - build launch command depending on whether the user is offline or logged in.
        std::string launchcmd;
        if (microsoft)
        {
            qDebug() << "Building launch command...";
            auto launchcmdopt = mcapi::vanilla::GetLaunchCommand(microsoftusername, classpath, mergedjson, versionid.toStdString(), osenum, microsoftuuid, microsoftaccesstoken, "msa");
            if (!launchcmdopt)
            {
                qDebug() << "Failed to build launch command.";
                return false;
            }
            launchcmd = *launchcmdopt;
            qDebug() << "Launch command built.";
        }
        else
        {
            qDebug() << "Building launch command...";
            auto launchcmdopt = mcapi::vanilla::GetLaunchCommand(username.toStdString(), classpath, mergedjson, versionid.toStdString(), osenum);
            if (!launchcmdopt)
            {
                qDebug() << "Failed to build launch command.";
                return false;
            }
            launchcmd = *launchcmdopt;
            qDebug() << "Launch command built.";
        }

        // - launch minecraft.
        std::string javapath;
        switch (osenum)
        {
        case mcapi::OS::Windows:
            javapath = "runtime/" + versionid.toStdString() + "/java/bin/java.exe";
            break;

        case mcapi::OS::Linux:
        case mcapi::OS::Macos:
            javapath = "runtime/" + versionid.toStdString() + "/java/bin/java";
            break;
        }
        bool launched = mcapi::StartProcess(javapath, launchcmd, osenum, &process, true);
        if (!launched)
        {
            QMetaObject::invokeMethod(this, [this](){QMessageBox::critical(this, "error", "Failed to launch minecraft.");}, Qt::QueuedConnection);
            return false;
        }
        else
        {
            QMetaObject::invokeMethod(this, [this](){QMessageBox::information(this, "info", "Minecraft launched.");}, Qt::QueuedConnection);
            return true;
        }
    }
    return false;
}

void gui::on_startbutton_clicked()
{
    if (processrunning.exchange(true))
        return;

    if (ui->usernameinput->text().trimmed().isEmpty())
    {
        QMetaObject::invokeMethod(this, [this](){QMessageBox::critical(this, "error", "Please enter a username.");}, Qt::QueuedConnection);
        processrunning = false;
        return;
    }

    ui->startbutton->setEnabled(false);

    QFuture<void> future = QtConcurrent::run([this]()
    {
        if (!StartVersion(username, loaderselected, versionselected, archselected, osselected))
        {
            processrunning = false;
            QMetaObject::invokeMethod(ui->startbutton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
            return;
        }
        while (mcapi::DetectProcess(&process))
        {
            QThread::sleep(1);
        }
        processrunning = false;
        QMetaObject::invokeMethod(ui->startbutton, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
    });
}

void gui::on_stopbutton_clicked()
{
    if (!mcapi::StopProcess(&process))
    {
        QMessageBox::critical(this, "error", "Failed to stop minecraft.");
    }
    else
    {
        QMessageBox::information(this, "info", "Minecraft stopped.");
        processrunning = false;
        ui->startbutton->setEnabled(true);
    }
}

bool gui::StartMicrosoftLogin()
{
    std::string accesstoken;
    if (!microsoftlogin)
    {
        // - get microsoft login url.
        auto codeurlopt = mcapi::auth::GetMicrosoftLoginUrl();
        if (!codeurlopt)
        {
            qDebug() << "Failed to get auth url.";
            return false;
        }
        auto codeurl = *codeurlopt;

        // - start microsoft login listener for auth code.
        qDebug() << "180 seconds until login gets cancelled. Please press the cancel button if you want to retry the login process.";
        auto codeopt = mcapi::auth::StartMicrosoftLoginListener(codeurl);
        if (!codeopt)
        {
            qDebug() << "Failed to login with Microsoft.";
            return false;
        }
        auto code = *codeopt;
        if (code == "timeout")
        {
            qDebug() << "Login canceled (timeout reached).";
            return false;
        }

        // - get access token json.
        auto accessjsonopt = mcapi::auth::GetAccessTokenJson(code);
        if (!accessjsonopt)
        {
            qDebug() << "Failed to get access token json.";
            return false;
        }
        auto accessjson = *accessjsonopt;

        // - get access token from json.
        auto accesstokenopt = mcapi::auth::GetAccessTokenFromJson(accessjson);
        if (!accesstokenopt)
        {
            qDebug() << "Failed to get access token.";
            return false;
        }
        accesstoken = *accesstokenopt;

        // - get refresh token from json.
        auto refreshtokenopt = mcapi::auth::GetRefreshTokenFromJson(accessjson);
        if (!refreshtokenopt)
        {
            qDebug() << "Failed to get access token.";
            return false;
        }
        auto refreshtoken = *refreshtokenopt;
    }
    if (microsoftlogin)
    {
        // - get access token from refresh token.
        auto accesstokenopt = mcapi::auth::GetAccessTokenFromRefreshToken();
        if (!accesstokenopt)
        {
            qDebug() << "Failed to get access token.";
            return false;
        }
        accesstoken = *accesstokenopt;
    }

    // - get xbox token json.
    auto xboxjsonopt = mcapi::auth::GetXboxTokenJson(accesstoken);
    if (!xboxjsonopt)
    {
        qDebug() << "Failed to get xbox token json.";
        return false;
    }
    auto xboxjson = *xboxjsonopt;

    // - get xbox token from json.
    auto xboxtokenopt = mcapi::auth::GetXboxTokenFromJson(xboxjson);
    if (!xboxtokenopt)
    {
        qDebug() << "Failed to get xbox token from json.";
        return false;
    }
    auto xboxtoken = *xboxtokenopt;

    // - get xbox hash from json.
    auto xboxhashopt = mcapi::auth::GetXboxHashFromJson(xboxjson);
    if (!xboxhashopt)
    {
        qDebug() << "Failed to get xbox hash from json.";
        return false;
    }
    auto xboxhash = *xboxhashopt;

    // - get xsts token json.
    auto xstsjsonopt = mcapi::auth::GetXstsTokenJson(xboxtoken);
    if (!xstsjsonopt)
    {
        qDebug() << "Failed to get xsts token json.";
        return false;
    }
    auto xstsjson = *xstsjsonopt;

    // - get xsts token from json.
    auto xststokenopt = mcapi::auth::GetXstsTokenFromJson(xstsjson);
    if (!xststokenopt)
    {
        qDebug() << "Failed to get xsts token from json.";
        return false;
    }
    auto xststoken = *xststokenopt;

    // - get minecraft token json.
    auto mcjsonopt = mcapi::auth::GetMinecraftTokenJson(xboxhash, xststoken);
    if (!mcjsonopt)
    {
        qDebug() << "Failed to get minecraft token json.";
        return false;
    }
    auto mcjson = *mcjsonopt;

    // - get minecraft token from json.
    auto mctokenopt = mcapi::auth::GetMinecraftTokenFromJson(mcjson);
    if (!mctokenopt)
    {
        qDebug() << "Failed to get minecraft token from json.";
        return false;
    }
    auto mctoken = *mctokenopt;

    // - verify minecraft ownership.
    auto ownerjsonopt = mcapi::auth::GetMinecraftOwnershipJson(mctoken);
    if (!ownerjsonopt)
    {
        qDebug() << "Failed to get minecraft entitlements json.";
        return false;
    }
    auto ownerjson = *ownerjsonopt;

    auto owns = mcapi::auth::GetMinecraftOwnershipFromJson(ownerjson);
    if (!owns.value())
    {
        QMetaObject::invokeMethod(this, [this](){QMessageBox::critical(this, "error", "This account doesn't own minecraft.");}, Qt::QueuedConnection);
        loginfailedmessage = false;
        return false;
    }

    // - get minecraft profile json.
    auto profileopt = mcapi::auth::GetMinecraftProfileJson(mctoken);
    if (!profileopt)
    {
        qDebug() << "Failed to get minecraft profile json.";
        return false;
    }
    auto profilejson = *profileopt;

    // - get minecraft username from profile json.
    auto usernameopt = mcapi::auth::GetUsernameFromProfileJson(profilejson);
    if (!usernameopt)
    {
        qDebug() << "Failed to get minecraft username from profile json.";
        return false;
    }
    auto username = *usernameopt;

    // - get minecraft uuid from profile json.
    auto uuidopt = mcapi::auth::GetUuidFromProfileJson(profilejson);
    if (!uuidopt)
    {
        qDebug() << "Failed to get minecraft uuid from profile json.";
        return false;
    }
    auto uuid = *uuidopt;

    qDebug() << QString::fromStdString(username);
    qDebug() << QString::fromStdString(uuid);

    microsoftusername = username;
    microsoftuuid = uuid;
    microsoftaccesstoken = mctoken;

    ui->usernameinput->setText(QString::fromStdString(username));
    ui->usernameinput->setEnabled(false);
    return true;
}

void gui::on_loginmicrosoftbutton_clicked()
{
    if (loginrunning.exchange(true))
        return;

    if (microsoftlatestlogin)
    {
        microsoftlogin = fs::exists(".mcapi/refresh_token");
    }
    else
    {
        microsoftlogin = false;
    }

    if (!microsoftlogin)
    {
        ui->logincancelbutton->show();
        microsoft = true;
    }
    ui->loginmicrosoftbutton->setEnabled(false);
    ui->offlinebutton->setEnabled(false);

    loginfailedmessage = true;

    QFuture<void> future = QtConcurrent::run([this]()
    {
        bool success = StartMicrosoftLogin();

        QMetaObject::invokeMethod(this, [this, success]()
        {
            loginrunning = false;

            if (!microsoftlogin)
            {
                ui->logincancelbutton->hide();
            }
            ui->loginmicrosoftbutton->setEnabled(true);
            ui->offlinebutton->setEnabled(true);

            if (!success && loginfailedmessage)
            {
                QMessageBox::critical(this, "error", "Login failed.");
            }
            else if (success)
            {
                qDebug() << "Logged in.";
                ui->pages->setCurrentIndex(1);
                QMessageBox::information(this, "info", "Logged in as: " + QString::fromStdString(microsoftusername));
            }
        }, Qt::QueuedConnection);
    });
}

void gui::on_logincancelbutton_clicked()
{
    bool running = mcapi::auth::StopMicrosoftLoginListener();
    if (running)
        qDebug() << "Login canceled.";

    ui->logincancelbutton->hide();
    ui->loginmicrosoftbutton->setEnabled(true);
    ui->offlinebutton->setEnabled(true);
    loginrunning = false;
    microsoft = false;
}
