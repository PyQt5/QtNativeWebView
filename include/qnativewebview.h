#ifndef QNATIVEWEBVIEW_H
#define QNATIVEWEBVIEW_H

#include "QNativeWebView_global.h"

#include <QWidget>
#include <QUrl>

class QNativeWebViewPrivate;

class QNATIVEWEBVIEW_EXPORT QNativeWebView : public QWidget
{
    Q_OBJECT

public:
    explicit QNativeWebView(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~QNativeWebView();
    QString errorString() const;

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
