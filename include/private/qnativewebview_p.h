#ifndef QNATIVEWEBVIEW_P_H
#define QNATIVEWEBVIEW_P_H

#include <QObject>
#include <QUrl>
#include <QVariant>
#include <functional>

QT_BEGIN_NAMESPACE
class QWindow;
QT_END_NAMESPACE

class QNativeWebView;

class QNativeWebViewPrivate : public QObject
{
    Q_OBJECT

public:
    virtual void load(const QUrl &url) = 0;
    virtual void setHtml(const QString &html, const QUrl &baseUrl = QUrl()) = 0;
    virtual void stop() = 0;
    virtual void back() = 0;
    virtual void forward() = 0;
    virtual void reload() = 0;

    virtual QWindow *nativeWindow() = 0;
    virtual QString errorString() const = 0;
    virtual QString userAgent() const = 0;
    virtual bool setUserAgent(const QString &userAgent) = 0;
    virtual void allCookies(const std::function<void(const QJsonObject &)> &callback) = 0;
    virtual bool setCookie(const QString &domain, const QString &name, const QString &value) = 0;
    virtual void deleteCookie(const QString &domain, const QString &name) = 0;
    virtual void deleteAllCookies() = 0;
    virtual void evaluateJavaScript(const QString &scriptSource,
                                    const std::function<void(const QVariant &)> &callback = {}) = 0;

Q_SIGNALS:
    void loadStarted();
    void loadProgress(int progress);
    void loadFinished(bool ok);
    void titleChanged(const QString &title);
    void statusBarMessage(const QString &text);
    void linkClicked(const QUrl &url);
    void iconChanged(const QIcon &icon);
    void urlChanged(const QUrl &url);
    void errorOccurred(const QString &error);

protected:
    explicit QNativeWebViewPrivate(QObject *parent = nullptr) : QObject(parent) { }
};

#endif // QNATIVEWEBVIEW_P_H
