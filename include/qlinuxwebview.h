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
    void stop() override;
    void back() override;
    void forward() override;
    void reload() override;

    QWindow *nativeWindow() override;

private Q_SLOTS:
    void updateWindowGeometry();
    void initialize();

private:
    void *m_webview; // WebKitWebView
    void *m_widget; // GtkWidget
    QWindow *m_window;
};

#endif // QLINUXWEBVIEW_H
