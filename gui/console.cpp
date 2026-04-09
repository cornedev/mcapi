#include "console.h"
#include "ui_console.h"

console::console(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::console)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

    // - consolelog box.
    ui->consolelog->setReadOnly(true);

    // - close button.
    connect(ui->closebutton, &QPushButton::clicked, this, &QWidget::close);
}

console::~console()
{
    delete ui;
}

void console::appendmessage(const QString& msg)
{
    ui->consolelog->appendPlainText(msg);
}

void console::closeEvent(QCloseEvent* event)
{
    event->ignore();
    this->hide();
}
