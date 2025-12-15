#ifndef QWEBVIEW2WEBVIEW_H
#define QWEBVIEW2WEBVIEW_H

#include "qnativewebview_p.h"
#include <QMap>
#include <QUrl>

// clang-format off
#include <webview2.h>
#include <wrl.h>
#include <wrl/client.h>
// clang-format on

using namespace Microsoft::WRL;

// This is used to store informations before webview2 is initialized
// Because WebView2 initialization is async
struct QWebViewInitData
{
    QString m_html;
    struct CookieData
    {
        QString domain;
        QString name;
        QString value;
    };
    QMap<QString, CookieData> m_cookies;
    QString m_httpUserAgent;
};

class QWebView2WebViewPrivate : public QNativeWebViewPrivate
{
    Q_OBJECT
public:
    explicit QWebView2WebViewPrivate(QObject *parent = nullptr);
    ~QWebView2WebViewPrivate();

    void load(const QUrl &url) override;
    void setHtml(const QString &html, const QUrl &baseUrl = QUrl()) override;
    void evaluateJavaScript(const QString &scriptSource,
                            const std::function<void(const QVariant &)> &callback = {}) override;
    void stop() override;
    void back() override;
    void forward() override;
    void reload() override;

    QWindow *nativeWindow() override;
    QString errorString() const override;

private Q_SLOTS:
    HRESULT onNavigationStarting(ICoreWebView2 *webview,
                                 ICoreWebView2NavigationStartingEventArgs *args);
    HRESULT onNavigationCompleted(ICoreWebView2 *webview,
                                  ICoreWebView2NavigationCompletedEventArgs *args);
    HRESULT onWebResourceRequested(ICoreWebView2 *webview,
                                   ICoreWebView2WebResourceRequestedEventArgs *args);
    HRESULT onContentLoading(ICoreWebView2 *webview, ICoreWebView2ContentLoadingEventArgs *args);
    HRESULT onDOMContentLoaded(ICoreWebView2 *webview,
                               ICoreWebView2DOMContentLoadedEventArgs *args);
    HRESULT onDocumentTitleChanged(ICoreWebView2 *webview, IUnknown *args);
    HRESULT onNewWindowRequested(ICoreWebView2 *webview,
                                 ICoreWebView2NewWindowRequestedEventArgs *args);
    void updateWindowGeometry();
    void initialize();

private:
    QUrl m_url;
    QString m_error;
    ComPtr<ICoreWebView2Controller> m_webviewController;
    ComPtr<ICoreWebView2> m_webview;
    ComPtr<ICoreWebView2CookieManager> m_cookieManager;
    QWindow *m_window;
    QWebViewInitData m_initData;
};

#endif // QWEBVIEW2WEBVIEW_H
