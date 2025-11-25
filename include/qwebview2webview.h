#ifndef QWEBVIEW2WEBVIEW_H
#define QWEBVIEW2WEBVIEW_H

#include "qnativewebview_p.h"
#include "qnativewebsettings.h"
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

class QWebView2WebViewSettingsPrivate : public QNativeWebSettings
{
    Q_OBJECT
public:
    explicit QWebView2WebViewSettingsPrivate(ICoreWebView2Controller *controller,
                                             QObject *parent = nullptr);

    bool localStorageEnabled() const override;
    bool javaScriptEnabled() const override;
    bool localContentCanAccessFileUrls() const override;
    bool allowFileAccess() const override;

    void setLocalContentCanAccessFileUrls(bool enabled) override;
    void setJavaScriptEnabled(bool enabled) override;
    void setLocalStorageEnabled(bool enabled) override;
    void setAllowFileAccess(bool enabled) override;

private:
    ComPtr<ICoreWebView2Controller> m_webviewController;
    ComPtr<ICoreWebView2> m_webview;
    bool m_allowFileAccess;
    bool m_localContentCanAccessFileUrls;
    bool m_javaScriptEnabled;
};

class QWebView2WebViewPrivate : public QNativeWebViewPrivate
{
    Q_OBJECT
public:
    explicit QWebView2WebViewPrivate(QObject *parent = nullptr);
    ~QWebView2WebViewPrivate();

    void load(const QUrl &url) override;
    void setHtml(const QString &html, const QUrl &baseUrl = QUrl()) override;
    void stop() override;
    void back() override;
    void forward() override;
    void reload() override;

    QNativeWebSettings *settings() override;
    QWindow *nativeWindow() override;

private Q_SLOTS:
    HRESULT onNavigationStarting(ICoreWebView2 *webview,
                                 ICoreWebView2NavigationStartingEventArgs *args);
    HRESULT onNavigationCompleted(ICoreWebView2 *webview,
                                  ICoreWebView2NavigationCompletedEventArgs *args);
    HRESULT onWebResourceRequested(ICoreWebView2 *sender,
                                   ICoreWebView2WebResourceRequestedEventArgs *args);
    HRESULT onContentLoading(ICoreWebView2 *webview, ICoreWebView2ContentLoadingEventArgs *args);
    HRESULT onNewWindowRequested(ICoreWebView2 *webview,
                                 ICoreWebView2NewWindowRequestedEventArgs *args);
    void updateWindowGeometry();
    void initialize();

private:
    QUrl m_url;
    ComPtr<ICoreWebView2Controller> m_webviewController;
    ComPtr<ICoreWebView2> m_webview;
    ComPtr<ICoreWebView2CookieManager> m_cookieManager;
    QWebView2WebViewSettingsPrivate *m_settings;
    QWindow *m_window;
    QWebViewInitData m_initData;
};

#endif // QWEBVIEW2WEBVIEW_H
