#include "qnativewebview.h"

#include <QVBoxLayout>
#include <QWindow>

#ifdef Q_OS_WIN
#  include "qwebview2webview.h"
#endif

#ifdef Q_OS_LINUX
#  include "qlinuxwebview.h"
#endif

#ifdef Q_OS_MACOS
#  include "qdarwinwebview.h"
#endif

QNativeWebView::QNativeWebView(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
#ifdef Q_OS_WIN
      ,
      d_ptr(new QWebView2WebViewPrivate(this))
#endif
#ifdef Q_OS_LINUX
      ,
      d_ptr(new QLinuxWebViewPrivate(this))
#endif
#ifdef Q_OS_MACOS
      ,
      d_ptr(new QDarwinWebViewPrivate(this))
#endif
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    QWindow *window = d_ptr->nativeWindow();
    if (window) {
        layout->addWidget(QWidget::createWindowContainer(window, this, Qt::FramelessWindowHint));
    }

    connect(d_ptr, &QNativeWebViewPrivate::loadStarted, this, &QNativeWebView::loadStarted);
    connect(d_ptr, &QNativeWebViewPrivate::loadProgress, this, &QNativeWebView::loadProgress);
    connect(d_ptr, &QNativeWebViewPrivate::loadFinished, this, &QNativeWebView::loadFinished);
    connect(d_ptr, &QNativeWebViewPrivate::titleChanged, this, &QNativeWebView::titleChanged);
    connect(d_ptr, &QNativeWebViewPrivate::statusBarMessage, this,
            &QNativeWebView::statusBarMessage);
    connect(d_ptr, &QNativeWebViewPrivate::linkClicked, this, &QNativeWebView::linkClicked);
    connect(d_ptr, &QNativeWebViewPrivate::iconChanged, this, &QNativeWebView::iconChanged);
    connect(d_ptr, &QNativeWebViewPrivate::urlChanged, this, &QNativeWebView::urlChanged);
}

QNativeWebView::~QNativeWebView() { }

QNativeWebSettings *QNativeWebView::settings()
{
    return d_ptr->settings();
}

void QNativeWebView::load(const QUrl &url)
{
    d_ptr->load(url);
}

void QNativeWebView::setHtml(const QString &html, const QUrl &baseUrl)
{
    d_ptr->setHtml(html, baseUrl);
}

void QNativeWebView::stop()
{
    d_ptr->stop();
}

void QNativeWebView::back()
{
    d_ptr->back();
}

void QNativeWebView::forward()
{
    d_ptr->forward();
}

void QNativeWebView::reload()
{
    d_ptr->reload();
}
