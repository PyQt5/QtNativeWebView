#ifndef QLINUXWEBVIEW_H
#define QLINUXWEBVIEW_H

#include "qnativewebview_p.h"

class QLinuxWebViewPrivate : public QNativeWebViewPrivate
{
    Q_OBJECT
public:
    explicit QLinuxWebViewPrivate(QObject *parent = nullptr);
    ~QLinuxWebViewPrivate();

    void load(const QUrl &url) override;
    void setHtml(const QString &html, const QUrl &baseUrl = QUrl()) override;
    void stop() override;
    void back() override;
    void forward() override;
    void reload() override;

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
    QString m_error;

private:
    void *m_webview; // WebKitWebView
    void *m_widget; // GtkWidget
    QWindow *m_window;
};

#endif // QLINUXWEBVIEW_H
