#ifndef QNATIVEWEBVIEW_H
#define QNATIVEWEBVIEW_H

#include "QNativeWebView_global.h"

#include <QWidget>
#include <QUrl>
#include <QJsonObject>
#include <functional>

class QNativeWebViewPrivate;

class QNATIVEWEBVIEW_EXPORT QNativeWebView : public QWidget
{
    Q_OBJECT

public:
    explicit QNativeWebView(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~QNativeWebView();
    QString errorString() const;
    QString userAgent() const;
    bool setUserAgent(const QString &userAgent);
    void allCookies(const std::function<void(const QJsonObject &)> &callback);
    bool setCookie(const QString &domain, const QString &name, const QString &value);
    void deleteCookie(const QString &domain, const QString &name);
    void deleteAllCookies();
    void evaluateJavaScript(const QString &scriptSource,
                            const std::function<void(const QVariant &)> &callback = {});

public Q_SLOTS:
    void load(const QUrl &url);
    void setHtml(const QString &html, const QUrl &baseUrl = QUrl());
    void stop();
    void back();
    void forward();
    void reload();

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

private:
    QNativeWebViewPrivate *d_ptr;
    Q_DECLARE_PRIVATE(QNativeWebView)
};

#endif // QNATIVEWEBVIEW_H
