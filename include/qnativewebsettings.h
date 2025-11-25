#ifndef QNATIVEWEBSETTINGS_H
#define QNATIVEWEBSETTINGS_H

#include <QObject>

class QNativeWebSettings : public QObject
{
    Q_OBJECT

public:
    virtual bool localStorageEnabled() const = 0;
    virtual bool javaScriptEnabled() const = 0;
    virtual bool localContentCanAccessFileUrls() const = 0;
    virtual bool allowFileAccess() const = 0;

    virtual void setLocalContentCanAccessFileUrls(bool enabled) = 0;
    virtual void setJavaScriptEnabled(bool enabled) = 0;
    virtual void setLocalStorageEnabled(bool enabled) = 0;
    virtual void setAllowFileAccess(bool enabled) = 0;

protected:
    explicit QNativeWebSettings(QObject *parent = nullptr) : QObject(parent) { }
};

#endif // QNATIVEWEBSETTINGS_H
