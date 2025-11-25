#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "QNativeWebSettings"

#include <QDebug>
#include <QAction>
#include <QIcon>
#include <QStyle>
#include <QPixmap>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // action
    ui->actionBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    ui->actionForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    ui->actionRefresh->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));

    connect(ui->widgetBrowser, &QNativeWebView::titleChanged, this, &MainWindow::setWindowTitle);
    connect(ui->widgetBrowser, &QNativeWebView::loadStarted, this,
            [&] { qInfo() << "loadStarted"; });
    connect(ui->widgetBrowser, &QNativeWebView::loadFinished, this,
            [&](bool ok) { qInfo() << "loadFinished" << ok; });
    connect(ui->widgetBrowser, &QNativeWebView::loadProgress, this,
            [&](int progress) { qInfo() << "loadProgress" << progress; });

    // init url
    ui->urlEdit->setText("https://pyqt.site");
    emit ui->urlEdit->returnPressed();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_urlEdit_returnPressed()
{
    QString url = ui->urlEdit->text().trimmed();
    if (url.isEmpty()) {
        return;
    }

    ui->widgetBrowser->load(QUrl(url));
}

void MainWindow::on_actionBack_triggered(bool checked)
{
    ui->widgetBrowser->back();
}

void MainWindow::on_actionForward_triggered(bool checked)
{
    ui->widgetBrowser->forward();
}

void MainWindow::on_actionRefresh_triggered(bool checked) { }
