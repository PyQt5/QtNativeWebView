// clang-format off
#include <gio/gio.h>
#include <webkit2/webkit2.h>
// clang-format on

#include "private/qlinuxwebview.h"

#include <QDebug>
#include <QWindow>
#include <QTimer>
#include <QScreen>
#include <QUrl>

// clang-format off
#include <cairo/cairo.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <glib.h>
#include <gtk/gtkx.h>
// clang-format on

QLinuxWebViewPrivate::QLinuxWebViewPrivate(QObject *parent)
    : QNativeWebViewPrivate(parent), m_webview(nullptr), m_widget(nullptr), m_window(nullptr)
{
    qputenv("GDK_BACKEND", "x11");
    // Initialize GTK
    gtk_init(nullptr, nullptr);

    // Create WebView
    m_webview = WEBKIT_WEB_VIEW(webkit_web_view_new());
    WebKitWebView *webview = (WebKitWebView *)m_webview;
    if (webview && WEBKIT_IS_WEB_VIEW(webview)) {
        m_widget = gtk_plug_new(0);
        GtkWidget *widget = (GtkWidget *)m_widget;
        if (widget) {
            gtk_container_add(GTK_CONTAINER(widget), GTK_WIDGET(webview));
            gtk_widget_show_all(widget);
            gtk_widget_realize(widget);
            void *hWnd = reinterpret_cast<void *>(gtk_plug_get_id(GTK_PLUG(widget)));
            if (hWnd) {
                // Create a QWindow without a parent
                // This window is used for initializing the WebView2
                m_window = QWindow::fromWinId(WId(hWnd));
                m_window->setFlag(Qt::FramelessWindowHint); // No border
                QTimer::singleShot(0, this, [this]() { emit initialize(); });
            } else {
                qWarning() << "Can not get plug widget handle";
            }
        } else {
            qWarning() << "Failed to create plug widget";
        }
    } else {
        qWarning() << "Failed to create WebKit view";
    }
}

QLinuxWebViewPrivate::~QLinuxWebViewPrivate()
{
    stop();

    if (m_widget) {
        GtkWidget *widget = (GtkWidget *)m_widget;
        gtk_widget_hide(widget);
        gtk_widget_destroy(widget);
        m_widget = nullptr;
    }

    if (m_window) {
        m_window->destroy();
    }
}

void QLinuxWebViewPrivate::load(const QUrl &url)
{
    if (m_webview && url.isValid()) {
        webkit_web_view_load_uri((WebKitWebView *)m_webview, url.toString().toUtf8().constData());
    }
}

void QLinuxWebViewPrivate::setHtml(const QString &html, const QUrl &baseUrl) { }

void QLinuxWebViewPrivate::stop()
{
    if (m_webview) {
        webkit_web_view_stop_loading(static_cast<WebKitWebView *>(m_webview));
    }
}

void QLinuxWebViewPrivate::back()
{
    if (m_webview) {
        webkit_web_view_go_back(static_cast<WebKitWebView *>(m_webview));
    }
}

void QLinuxWebViewPrivate::forward()
{
    if (m_webview) {
        webkit_web_view_go_forward(static_cast<WebKitWebView *>(m_webview));
    }
}

void QLinuxWebViewPrivate::reload()
{
    if (m_webview) {
        webkit_web_view_reload(static_cast<WebKitWebView *>(m_webview));
    }
}

QWindow *QLinuxWebViewPrivate::nativeWindow()
{
    return m_window;
}

QString QLinuxWebViewPrivate::errorString() const
{
    return "";
}

QString QLinuxWebViewPrivate::userAgent() const
{
    return "";
}

bool QLinuxWebViewPrivate::setUserAgent(const QString &userAgent)
{
    return false;
}

void QLinuxWebViewPrivate::allCookies(const std::function<void(const QJsonObject &)> &callback) { }

bool QLinuxWebViewPrivate::setCookie(const QString &domain, const QString &name,
                                     const QString &value)
{
    return false;
}

void QLinuxWebViewPrivate::deleteCookie(const QString &domain, const QString &name) { }

void QLinuxWebViewPrivate::deleteAllCookies() { }

void QLinuxWebViewPrivate::evaluateJavaScript(const QString &scriptSource,
                                              const std::function<void(const QVariant &)> &callback)
{
}

void QLinuxWebViewPrivate::updateWindowGeometry()
{
    if (m_widget) {
        gtk_widget_set_size_request((GtkWidget *)m_widget,
                                    m_window->width() * m_window->devicePixelRatio(),
                                    m_window->height() * m_window->devicePixelRatio());
    }
}

void QLinuxWebViewPrivate::initialize()
{

    connect(m_window, &QWindow::widthChanged, this, &QLinuxWebViewPrivate::updateWindowGeometry,
            Qt::QueuedConnection);
    connect(m_window, &QWindow::heightChanged, this, &QLinuxWebViewPrivate::updateWindowGeometry,
            Qt::QueuedConnection);
    connect(m_window, &QWindow::screenChanged, this, &QLinuxWebViewPrivate::updateWindowGeometry,
            Qt::QueuedConnection);

    g_signal_connect_swapped(m_widget, "destroy", G_CALLBACK(+[](QLinuxWebViewPrivate *instance) {
                                 qDebug() << "webview container destroy";
                             }),
                             this);
    g_signal_connect_swapped(m_webview, "destroy", G_CALLBACK(+[](QLinuxWebViewPrivate *instance) {
                                 qDebug() << "webview destroy";
                             }),
                             this);

    // url change
    g_signal_connect_swapped(m_webview, "notify::uri",
                             G_CALLBACK(+[](QLinuxWebViewPrivate *instance, GParamSpec *pspec) {
                                 qDebug() << "notify::uri";
                                 if (instance && instance->m_webview) {
                                     const QString url = webkit_web_view_get_uri(
                                             static_cast<WebKitWebView *>(instance->m_webview));
                                     emit instance->urlChanged(QUrl(url));
                                 }
                             }),
                             this);

    // title change
    g_signal_connect_swapped(m_webview, "notify::title",
                             G_CALLBACK(+[](QLinuxWebViewPrivate *instance, GParamSpec *pspec) {
                                 qDebug() << "notify::title";
                                 if (instance && instance->m_webview) {
                                     const QString title = webkit_web_view_get_title(
                                             static_cast<WebKitWebView *>(instance->m_webview));
                                     emit instance->titleChanged(title);
                                 }
                             }),
                             this);

    // load progress change
    g_signal_connect_swapped(
            m_webview, "notify::estimated-load-progress",
            G_CALLBACK(+[](QLinuxWebViewPrivate *instance, GParamSpec *pspec) {
                qDebug() << "notify::estimated-load-progress";
                if (instance && instance->m_webview) {
                    int value = webkit_web_view_get_estimated_load_progress(
                                        static_cast<WebKitWebView *>(instance->m_webview))
                            * 100;
                    emit instance->loadProgress(value);
                }
            }),
            this);

    // load status
    g_signal_connect_swapped(
            m_webview, "load-changed",
            G_CALLBACK(+[](QLinuxWebViewPrivate *instance, WebKitLoadEvent event) {
                qDebug() << "load-changed";
                if (instance && instance->m_webview) {
                    WebKitWebView *webview = static_cast<WebKitWebView *>(instance->m_webview);
                    WebKitLoadEvent ev = static_cast<WebKitLoadEvent>(event);
                    switch (event) {
                    case WEBKIT_LOAD_STARTED:
                        instance->m_error = "";
                        emit instance->loadStarted();
                        break;
                    case WEBKIT_LOAD_FINISHED:
                        instance->m_error = "";
                        emit instance->loadFinished(true);
                        break;
                    default:
                        break;
                    }
                }
            }),
            this);

    // load failed
    g_signal_connect_swapped(m_webview, "load-failed",
                             G_CALLBACK(+[](QLinuxWebViewPrivate *instance, WebKitLoadEvent event,
                                            char *url, GError *error) -> gboolean {
                                 qDebug() << "load-failed";
                                 if (instance) {
                                     instance->m_error = error->message;
                                     emit instance->loadFinished(false);
                                 }
                                 return false;
                             }),
                             this);
}
