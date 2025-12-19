#ifndef QDARWINWEBVIEW_H
#define QDARWINWEBVIEW_H

#include "qnativewebview_p.h"

Q_FORWARD_DECLARE_OBJC_CLASS(WKWebView);
Q_FORWARD_DECLARE_OBJC_CLASS(WKNavigation);
Q_FORWARD_DECLARE_OBJC_CLASS(WKWebViewConfiguration);

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

    QWindow *nativeWindow() override;
    QString errorString() const override;
    QString userAgent() const override;
    bool setUserAgent(const QString &userAgent) override;
    void allCookies(const std::function<void(const QJsonObject &)> &callback) override;
    bool setCookie(const QString &domain, const QString &name, const QString &value) override;
    void deleteCookie(const QString &domain, const QString &name) override;
    void deleteAllCookies() override;
    void evaluateJavaScript(const QString &scriptSource,
                            const std::function<void(const QVariant &)> &callback = {}) override;

private Q_SLOTS:
    void updateWindowGeometry();
    void initialize();

public:
    WKNavigation *m_navigation;

private:
    WKWebView *m_webview;
    QWindow *m_window;
};

#endif // QDARWINWEBVIEW_H
