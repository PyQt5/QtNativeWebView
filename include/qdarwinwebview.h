#ifndef QDARWINWEBVIEW_H
#define QDARWINWEBVIEW_H

#include "qnativewebview_p.h"
#include "qnativewebsettings.h"

Q_FORWARD_DECLARE_OBJC_CLASS(WKWebView);
Q_FORWARD_DECLARE_OBJC_CLASS(WKNavigation);
Q_FORWARD_DECLARE_OBJC_CLASS(WKWebViewConfiguration);

class QDarwinWebViewSettingsPrivate : public QNativeWebSettings
{
    Q_OBJECT
public:
    explicit QDarwinWebViewSettingsPrivate(WKWebViewConfiguration *conf, QObject *parent = nullptr);

    bool localStorageEnabled() const override;
    bool javaScriptEnabled() const override;
    bool localContentCanAccessFileUrls() const override;
    bool allowFileAccess() const override;

    void setLocalContentCanAccessFileUrls(bool enabled) override;
    void setJavaScriptEnabled(bool enabled) override;
    void setLocalStorageEnabled(bool enabled) override;
    void setAllowFileAccess(bool enabled) override;

private:
    WKWebViewConfiguration *m_conf;
    bool m_allowFileAccess;
    bool m_localContentCanAccessFileUrls;
};

class QDarwinWebViewPrivate : public QNativeWebViewPrivate
{
    Q_OBJECT
public:
    explicit QDarwinWebViewPrivate(QObject *parent = nullptr);
    ~QDarwinWebViewPrivate();

    void load(const QUrl &url) override;
    void setHtml(const QString &html, const QUrl &baseUrl = QUrl()) override;
    void stop() override;
    void back() override;
    void forward() override;
    void reload() override;

    QString title() const;
    int progressValue() const;

    QNativeWebSettings *settings() override;
    QWindow *nativeWindow() override;

private Q_SLOTS:
    void updateWindowGeometry();
    void initialize();

public:
    WKNavigation *m_navigation;

private:
    WKWebView *m_webview;
    QDarwinWebViewSettingsPrivate *m_settings;
    QWindow *m_window;
};

#endif // QDARWINWEBVIEW_H
