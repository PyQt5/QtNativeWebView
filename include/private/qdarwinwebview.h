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
