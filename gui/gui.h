#ifndef GUI_H
#define GUI_H

#include <QWidget>
#include <QDebug>
#include <QComboBox>
#include <QString>
#include <QtConcurrent>
#include <QThread>

#include "../api/api.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class gui;
}
QT_END_NAMESPACE

class gui : public QWidget
{
    Q_OBJECT

public:
    explicit gui(QWidget *parent = nullptr);
    ~gui() override;

private slots:
    void on_startbutton_clicked();
    void on_versioncombo_changed(const QString &version);
    void on_oscombo_changed(const QString &os);
    void on_archcombo_changed(const QString &os);
    void on_usernameinput_changed(const QString &input);

    void on_stopbutton_clicked();

private:
    Ui::gui *ui;

    std::string manifest;
    std::atomic<bool> running{false};

    QString versionselected;
    QString osselected;
    QString archselected;

    QString username;

    void GetVersions();
    bool StartVersion(const QString &username, const QString &versionselected, const QString &archselected, const QString &osselected);
};
#endif // GUI_H