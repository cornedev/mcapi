#ifndef CONSOLE_H
#define CONSOLE_H

#include <QWidget>

namespace Ui {
class console;
}

class console : public QWidget
{
    Q_OBJECT

public:
    explicit console(QWidget *parent = nullptr);
    ~console();

    void appendmessage(const QString& msg);

protected:
    void closeEvent(QCloseEvent* event);

private:
    Ui::console *ui;
};

#endif // CONSOLE_H