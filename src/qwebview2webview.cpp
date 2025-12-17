#include "private/qwebview2webview.h"

#include <QDebug>
#include <QWindow>
#include <QTimer>
#include <QScreen>
#include <QUrl>
#include <QPointer>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QJsonObject>
#include <QDir>

#ifndef Q_ASSERT_SUCCEEDED
#  define Q_ASSERT_SUCCEEDED(hr) \
      Q_ASSERT_X(SUCCEEDED(hr), Q_FUNC_INFO, qPrintable(qt_error_string(hr)));
#endif

QString WebErrorStatusToString(COREWEBVIEW2_WEB_ERROR_STATUS status)
{
    switch (status) {
#define STATUS_ENTRY(statusValue) \
    case statusValue:             \
        return QString(#statusValue);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_UNKNOWN);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_COMMON_NAME_IS_INCORRECT);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_EXPIRED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CLIENT_CERTIFICATE_CONTAINS_ERRORS);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_REVOKED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CERTIFICATE_IS_INVALID);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_SERVER_UNREACHABLE);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_TIMEOUT);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_ERROR_HTTP_INVALID_SERVER_RESPONSE);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_ABORTED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CONNECTION_RESET);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_DISCONNECTED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_CANNOT_CONNECT);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_HOST_NAME_NOT_RESOLVED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_REDIRECT_FAILED);
        STATUS_ENTRY(COREWEBVIEW2_WEB_ERROR_STATUS_UNEXPECTED_ERROR);

#undef STATUS_ENTRY
    case COREWEBVIEW2_WEB_ERROR_STATUS_VALID_AUTHENTICATION_CREDENTIALS_REQUIRED:
    case COREWEBVIEW2_WEB_ERROR_STATUS_VALID_PROXY_AUTHENTICATION_REQUIRED:
        break;
    }

    return QString("ERROR");
}

QWebView2WebViewPrivate::QWebView2WebViewPrivate(QObject *parent)
    : QNativeWebViewPrivate(parent),
      m_webviewController(nullptr),
      m_webview(nullptr),
      m_cookieManager(nullptr),
      m_window(new QWindow)
{
    // Create a QWindow without a parent
    // This window is used for initializing the WebView2

    m_window->setFlag(Qt::Tool);
    m_window->setFlag(Qt::FramelessWindowHint); // No border
    m_window->setFlag(Qt::WindowDoesNotAcceptFocus); // No focus
    m_window->setVisible(true);

    QTimer::singleShot(0, this, [this]() { emit initialize(); });
}

QWebView2WebViewPrivate::~QWebView2WebViewPrivate()
{
    if (m_window) {
        m_window->destroy();
    }
    m_webviewController = nullptr;
    m_webview = nullptr;
}

void QWebView2WebViewPrivate::load(const QUrl &url)
{
    m_url = url;
    if (m_webview) {
        HRESULT hr = m_webview->Navigate((wchar_t *)url.toString().utf16());
        Q_ASSERT_SUCCEEDED(hr);
        if (FAILED(hr)) {
            emit loadFinished(false);
        }
    }
}

void QWebView2WebViewPrivate::setHtml(const QString &html, const QUrl &baseUrl)
{
    if (m_webview && !html.isEmpty()) {
        const HRESULT hr = m_webview->NavigateToString((wchar_t *)html.utf16());
        Q_ASSERT_SUCCEEDED(hr);
        if (FAILED(hr)) {
            emit loadFinished(false);
        }
    }
}

void QWebView2WebViewPrivate::evaluateJavaScript(
        const QString &scriptSource, const std::function<void(const QVariant &)> &callback)
{
    if (!m_webview) {
        if (callback) {
            callback("");
        }
        return;
    }

    const HRESULT hr = m_webview->ExecuteScript(
            (wchar_t *)scriptSource.utf16(),
            Microsoft::WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                    [this, callback](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
                        QString resultStr = QString::fromWCharArray(resultObjectAsJson);
                        qInfo() << "resultStr:" << resultStr;

                        QJsonParseError parseError;
                        QJsonDocument jsonDoc =
                                QJsonDocument::fromJson(resultStr.toUtf8(), &parseError);

                        QVariant resultVariant;
                        if (parseError.error == QJsonParseError::NoError) {
                            resultVariant = jsonDoc.toVariant();
                        } else {
                            QString wrapped = QString("{\"value\":%1}").arg(resultStr);
                            jsonDoc = QJsonDocument::fromJson(wrapped.toUtf8(), &parseError);
                            if (parseError.error == QJsonParseError::NoError) {
                                resultVariant = jsonDoc.object().value("value").toVariant();
                            } else {
                                QJsonValue val = QJsonValue::fromVariant(resultStr);
                                resultVariant = val.toVariant();
                            }
                        }
                        if (errorCode != S_OK) {
                            QMetaObject::invokeMethod(
                                    this, [&] { callback(qt_error_string(errorCode)); });
                        } else {
                            QMetaObject::invokeMethod(this, [&] { callback(resultVariant); });
                        }
                        return errorCode;
                    })
                    .Get());
    Q_ASSERT_SUCCEEDED(hr);
}

void QWebView2WebViewPrivate::stop()
{
    if (m_webview) {
        const HRESULT hr = m_webview->Stop();
        Q_ASSERT_SUCCEEDED(hr);
    }
}

void QWebView2WebViewPrivate::back()
{
    if (m_webview) {
        const HRESULT hr = m_webview->GoBack();
        Q_ASSERT_SUCCEEDED(hr);
    }
}

void QWebView2WebViewPrivate::forward()
{
    if (m_webview) {
        const HRESULT hr = m_webview->GoForward();
        Q_ASSERT_SUCCEEDED(hr);
    }
}

void QWebView2WebViewPrivate::reload()
{
    if (m_webview) {
        const HRESULT hr = m_webview->Reload();
        Q_ASSERT_SUCCEEDED(hr);
    }
}

QWindow *QWebView2WebViewPrivate::nativeWindow()
{
    return m_window;
}

QString QWebView2WebViewPrivate::errorString() const
{
    return m_error;
}

QString QWebView2WebViewPrivate::userAgent() const
{
    if (m_webviewController) {
        ComPtr<ICoreWebView2Settings> settings;
        HRESULT hr = m_webview->get_Settings(&settings);
        Q_ASSERT_SUCCEEDED(hr);
        ComPtr<ICoreWebView2Settings2> settings2;
        hr = settings->QueryInterface(IID_PPV_ARGS(&settings2));

        if (settings2) {
            wchar_t *userAgent;
            hr = settings2->get_UserAgent(&userAgent);
            Q_ASSERT_SUCCEEDED(hr);
            QString userAgentString = QString::fromStdWString(userAgent);
            CoTaskMemFree(userAgent);
            return userAgentString;
        }
        qWarning() << "No http user agent available.";
        return "";
    }
    return "";
}

bool QWebView2WebViewPrivate::setUserAgent(const QString &userAgent)
{
    if (m_webviewController) {
        ComPtr<ICoreWebView2Settings> settings;
        HRESULT hr = m_webview->get_Settings(&settings);
        if (!SUCCEEDED(hr)) {
            return false;
        }

        ComPtr<ICoreWebView2Settings2> settings2;
        hr = settings->QueryInterface(IID_PPV_ARGS(&settings2));
        if (settings2) {
            hr = settings2->put_UserAgent((wchar_t *)userAgent.utf16());
            return SUCCEEDED(hr);
        } else {
            qWarning() << "No http user agent setting available.";
        }
    }

    return false;
}

QJsonObject QWebView2WebViewPrivate::allCookies() const
{
    QJsonObject cookies;

    if (m_webview && m_cookieManager) {
        HRESULT hr = m_cookieManager->GetCookies(
                L"",
                Microsoft::WRL::Callback<ICoreWebView2GetCookiesCompletedHandler>(
                        [&cookies](HRESULT result, ICoreWebView2CookieList *cookieList) -> HRESULT {
                            UINT count = 0;
                            cookieList->get_Count(&count);
                            for (UINT i = 0; i < count; ++i) {
                                ComPtr<ICoreWebView2Cookie> cookie;
                                if (SUCCEEDED(cookieList->GetValueAtIndex(i, &cookie))) {
                                    wchar_t *domain = nullptr;
                                    if (SUCCEEDED(cookie->get_Domain(&domain))) {
                                        // domain
                                        QString sdomain = QString::fromStdWString(domain);
                                        CoTaskMemFree(domain);
                                        if (!cookies.contains(sdomain)) {
                                            cookies.insert(sdomain, QJsonObject());
                                        }
                                        QJsonObject jdomain = cookies.value(sdomain).toObject();

                                        // name
                                        wchar_t *name = nullptr;
                                        if (SUCCEEDED(cookie->get_Name(&name))) {
                                            QString sname = QString::fromStdWString(name);
                                            CoTaskMemFree(name);
                                            jdomain.insert("name", sname);
                                            if (!jdomain.contains(sname)) {
                                                jdomain.insert(sname, QJsonObject());
                                            }
                                            QJsonObject ndomain = jdomain.value(sname).toObject();

                                            // value
                                            wchar_t *value = nullptr;
                                            if (SUCCEEDED(cookie->get_Value(&value))) {
                                                QString svalue = QString::fromStdWString(value);
                                                CoTaskMemFree(value);
                                                ndomain.insert("value", svalue);
                                            }

                                            // path
                                            wchar_t *path = nullptr;
                                            if (SUCCEEDED(cookie->get_Path(&path))) {
                                                QString spath = QString::fromStdWString(path);
                                                CoTaskMemFree(path);
                                                ndomain.insert("path", spath);
                                            }

                                            // expires
                                            double expires = 0;
                                            if (SUCCEEDED(cookie->get_Expires(&expires))) {
                                                ndomain.insert("expires", expires);
                                            }

                                            // httpOnly
                                            BOOL httpOnly = FALSE;
                                            if (SUCCEEDED(cookie->get_IsHttpOnly(&httpOnly))) {
                                                ndomain.insert("httpOnly", httpOnly);
                                            }

                                            // Secure
                                            BOOL secure = FALSE;
                                            if (SUCCEEDED(cookie->get_IsSecure(&secure))) {
                                                ndomain.insert("secure", secure);
                                            }

                                            // Session
                                            BOOL session = FALSE;
                                            if (SUCCEEDED(cookie->get_IsSession(&session))) {
                                                ndomain.insert("session", session);
                                            }
                                        }
                                    }
                                }
                            }
                            return S_OK;
                        })
                        .Get());
        Q_ASSERT_SUCCEEDED(hr);
    }

    return cookies;
}

bool QWebView2WebViewPrivate::setCookie(const QString &domain, const QString &name,
                                        const QString &value)
{
    if (m_webview && m_cookieManager) {
        ComPtr<ICoreWebView2Cookie> cookie;
        HRESULT hr =
                m_cookieManager->CreateCookie((wchar_t *)name.utf16(), (wchar_t *)value.utf16(),
                                              (wchar_t *)domain.utf16(), L"", &cookie);
        if (SUCCEEDED(hr)) {
            hr = m_cookieManager->AddOrUpdateCookie(cookie.Get());
            return SUCCEEDED(hr);
        }
    }

    return false;
}

void QWebView2WebViewPrivate::deleteCookie(const QString &domain, const QString &name)
{
    if (m_webview && m_cookieManager) {
        QString uri = domain;
        if (!uri.startsWith("http"))
            uri = "https://" + uri;
        HRESULT hr = m_cookieManager->GetCookies(
                (wchar_t *)uri.utf16(),
                Microsoft::WRL::Callback<ICoreWebView2GetCookiesCompletedHandler>(
                        [name, domain](HRESULT result,
                                       ICoreWebView2CookieList *cookieList) -> HRESULT {
                            UINT count = 0;
                            cookieList->get_Count(&count);
                            for (UINT i = 0; i < count; ++i) {
                                ComPtr<ICoreWebView2Cookie> cookie;
                                if (SUCCEEDED(cookieList->GetValueAtIndex(i, &cookie))) {
                                    wchar_t *domainPtr;
                                    wchar_t *namePtr;
                                    if (SUCCEEDED(cookie->get_Domain(&domainPtr))
                                        && SUCCEEDED(cookie->get_Name(&namePtr))) {
                                        QString sdomain = QString::fromStdWString(domainPtr);
                                        QString sname = QString::fromStdWString(namePtr);
                                        CoTaskMemFree(namePtr);
                                        CoTaskMemFree(domainPtr);
                                        if (domain == sdomain && name == sname) {
                                            return S_OK;
                                        }
                                    }
                                }
                            }
                            return S_OK;
                        })
                        .Get());
        Q_ASSERT_SUCCEEDED(hr);
        hr = m_cookieManager->DeleteCookiesWithDomainAndPath((wchar_t *)name.utf16(),
                                                             (wchar_t *)domain.utf16(), L"");
        Q_ASSERT_SUCCEEDED(hr);
    }
}

void QWebView2WebViewPrivate::deleteAllCookies()
{
    if (m_webview && m_cookieManager) {
        HRESULT hr = m_cookieManager->DeleteAllCookies();
        Q_ASSERT_SUCCEEDED(hr);
    }
}

HRESULT
QWebView2WebViewPrivate::onNavigationStarting(ICoreWebView2 *webview,
                                              ICoreWebView2NavigationStartingEventArgs *args)
{
    emit loadStarted();
    emit loadProgress(25);
    wchar_t *uri;
    HRESULT hr = args->get_Uri(&uri);
    Q_ASSERT_SUCCEEDED(hr);
    m_url = QString::fromStdWString(uri);
    emit urlChanged(m_url);
    CoTaskMemFree(uri);
    return S_OK;
}

HRESULT
QWebView2WebViewPrivate::onNavigationCompleted(ICoreWebView2 *webview,
                                               ICoreWebView2NavigationCompletedEventArgs *args)
{
    emit loadProgress(100);

    BOOL isSuccess;
    HRESULT hr = args->get_IsSuccess(&isSuccess);
    Q_ASSERT_SUCCEEDED(hr);

    emit loadFinished(isSuccess);

    COREWEBVIEW2_WEB_ERROR_STATUS errorStatus;
    hr = args->get_WebErrorStatus(&errorStatus);
    Q_ASSERT_SUCCEEDED(hr);
    if (errorStatus != COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED) {
        m_error = isSuccess ? "" : WebErrorStatusToString(errorStatus);
        if (!m_error.isEmpty()) {
            emit errorOccurred(m_error);
        }
    }

    return S_OK;
}

HRESULT
QWebView2WebViewPrivate::onWebResourceRequested(ICoreWebView2 *webview,
                                                ICoreWebView2WebResourceRequestedEventArgs *args)
{
    /*
    ComPtr<ICoreWebView2WebResourceRequest> request;
    ComPtr<ICoreWebView2WebResourceResponse> response;
    HRESULT hr = args->get_Request(&request);
    Q_ASSERT_SUCCEEDED(hr);
    wchar_t *uri;
    hr = request->get_Uri(&uri);
    std::wstring_view source(uri);

    if (!m_settings->allowFileAccess()) {
        ComPtr<ICoreWebView2Environment> environment;
        ComPtr<ICoreWebView2_2> webview2;
        m_webview->QueryInterface(IID_PPV_ARGS(&webview2));
        webview2->get_Environment(&environment);

        hr = environment->CreateWebResourceResponse(nullptr, 403, L"Access Denied", L"", &response);
        Q_ASSERT_SUCCEEDED(hr)
        hr = args->put_Response(response.Get());
        Q_ASSERT_SUCCEEDED(hr)
    }

    CoTaskMemFree(uri);
    */

    return S_OK;
}

HRESULT QWebView2WebViewPrivate::onContentLoading(ICoreWebView2 *webview,
                                                  ICoreWebView2ContentLoadingEventArgs *args)
{
    emit loadProgress(50);
    return S_OK;
}

HRESULT QWebView2WebViewPrivate::onDOMContentLoaded(ICoreWebView2 *webview,
                                                    ICoreWebView2DOMContentLoadedEventArgs *args)
{
    emit loadProgress(75);
    return S_OK;
}

HRESULT QWebView2WebViewPrivate::onDocumentTitleChanged(ICoreWebView2 *webview, IUnknown *args)
{
    wchar_t *title;
    HRESULT hr = webview->get_DocumentTitle(&title);
    Q_ASSERT_SUCCEEDED(hr);
    emit titleChanged(QString::fromStdWString(title));
    CoTaskMemFree(title);
    return S_OK;
}

HRESULT
QWebView2WebViewPrivate::onNewWindowRequested(ICoreWebView2 *webview,
                                              ICoreWebView2NewWindowRequestedEventArgs *args)
{
    Q_UNUSED(webview);
    // This blocks the spawning of new windows we don't control
    // FIXME actually handle new windows when QWebView has the API for them
    args->put_Handled(TRUE);
    return S_OK;
}

void QWebView2WebViewPrivate::updateWindowGeometry()
{
    if (m_webviewController) {
        RECT bounds;
        GetClientRect((HWND)m_window->winId(), &bounds);
        const HRESULT hr = m_webviewController->put_Bounds(bounds);
        Q_ASSERT_SUCCEEDED(hr);
    }
}

void QWebView2WebViewPrivate::initialize()
{
    // create platform window
    HWND hWnd = (HWND)m_window->winId();

    // signals
    connect(m_window, &QWindow::widthChanged, this, &QWebView2WebViewPrivate::updateWindowGeometry,
            Qt::QueuedConnection);
    connect(m_window, &QWindow::heightChanged, this, &QWebView2WebViewPrivate::updateWindowGeometry,
            Qt::QueuedConnection);
    connect(m_window, &QWindow::screenChanged, this, &QWebView2WebViewPrivate::updateWindowGeometry,
            Qt::QueuedConnection);

    QPointer<QWebView2WebViewPrivate> thisPtr = this;
    const QString userDataFolder =
            QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
            + QDir::separator() + QLatin1String("WebView2");
    using W2ControllerCallback = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
    auto controllerCallback = Microsoft::WRL::Callback<
            W2ControllerCallback>([thisPtr, this](HRESULT result,
                                                  ICoreWebView2Controller *controller) -> HRESULT {
        if (thisPtr.isNull())
            return S_FALSE;

        if (!controller)
            return S_FALSE;
        HRESULT hr;
        m_webviewController = controller;
        hr = m_webviewController->get_CoreWebView2(&m_webview);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<ICoreWebView2_2> webview2;
        hr = m_webview->QueryInterface(IID_PPV_ARGS(&webview2));
        Q_ASSERT_SUCCEEDED(hr);
        hr = webview2->get_CookieManager(&m_cookieManager);
        Q_ASSERT_SUCCEEDED(hr);

        // Add a few settings for the webview
        ComPtr<ICoreWebView2Settings> settings;
        hr = m_webview->get_Settings(&settings);
        Q_ASSERT_SUCCEEDED(hr);
        hr = settings->put_IsScriptEnabled(TRUE /*m_settings->javaScriptEnabled()*/);
        Q_ASSERT_SUCCEEDED(hr);
        hr = settings->put_AreDefaultScriptDialogsEnabled(TRUE);
        Q_ASSERT_SUCCEEDED(hr);
        hr = settings->put_IsWebMessageEnabled(TRUE);
        Q_ASSERT_SUCCEEDED(hr);

        QMetaObject::invokeMethod(this, "updateWindowGeometry", Qt::QueuedConnection);

        // Schedule an async task to navigate to the url
        // Because this is a callback and it might be triggered with a delay
        if (!m_url.isEmpty() && m_url.isValid() && !m_url.scheme().isEmpty()) {
            hr = m_webview->Navigate((wchar_t *)m_url.toString().utf16());
            Q_ASSERT_SUCCEEDED(hr);
        } else if (!m_initData.m_html.isEmpty()) {
            hr = m_webview->NavigateToString((wchar_t *)m_initData.m_html.utf16());
            Q_ASSERT_SUCCEEDED(hr);
        }
        if (m_initData.m_cookies.size() > 0) {
            //                    for (auto it = m_initData.m_cookies.constBegin();
            //                         it != m_initData.m_cookies.constEnd(); ++it)
            //                        setCookie(it->domain, it->name, it.value().value);
        }
        if (!m_initData.m_httpUserAgent.isEmpty()) {
            ComPtr<ICoreWebView2Settings2> settings2;
            hr = settings->QueryInterface(IID_PPV_ARGS(&settings2));
            if (settings2) {
                hr = settings2->put_UserAgent((wchar_t *)m_initData.m_httpUserAgent.utf16());
                if (SUCCEEDED(hr))
                    QTimer::singleShot(0, thisPtr, [thisPtr] {
                        //                                if (!thisPtr.isNull())
                        //                                    emit thisPtr->httpUserAgentChanged(
                        //                                            thisPtr->m_initData.m_httpUserAgent);
                    });
            }
        }

        EventRegistrationToken token;

        // add_NavigationStarting
        hr = m_webview->add_NavigationStarting(
                Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
                        [this](ICoreWebView2 *webview,
                               ICoreWebView2NavigationStartingEventArgs *args) -> HRESULT {
                            return this->onNavigationStarting(webview, args);
                        })
                        .Get(),
                &token);
        Q_ASSERT_SUCCEEDED(hr);

        // add_NavigationCompleted
        hr = m_webview->add_NavigationCompleted(
                Microsoft::WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
                        [this](ICoreWebView2 *webview,
                               ICoreWebView2NavigationCompletedEventArgs *args) -> HRESULT {
                            return this->onNavigationCompleted(webview, args);
                        })
                        .Get(),
                &token);
        Q_ASSERT_SUCCEEDED(hr);

        // add_WebResourceRequested
        m_webview->add_WebResourceRequested(
                Microsoft::WRL::Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                        [this](ICoreWebView2 *webview,
                               ICoreWebView2WebResourceRequestedEventArgs *args) -> HRESULT {
                            return this->onWebResourceRequested(webview, args);
                        })
                        .Get(),
                &token);

        // add_ContentLoading
        hr = m_webview->add_ContentLoading(
                Microsoft::WRL::Callback<ICoreWebView2ContentLoadingEventHandler>(
                        [this](ICoreWebView2 *webview, ICoreWebView2ContentLoadingEventArgs *args)
                                -> HRESULT { return this->onContentLoading(webview, args); })
                        .Get(),
                &token);
        Q_ASSERT_SUCCEEDED(hr);

        // add_DocumentTitleChanged
        hr = m_webview->add_DocumentTitleChanged(
                Microsoft::WRL::Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                        [this](ICoreWebView2 *webview, IUnknown *args) -> HRESULT {
                            return this->onDocumentTitleChanged(webview, args);
                        })
                        .Get(),
                &token);
        Q_ASSERT_SUCCEEDED(hr);

        // add_NewWindowRequested
        hr = m_webview->add_NewWindowRequested(
                Microsoft::WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                        [this](ICoreWebView2 *webview,
                               ICoreWebView2NewWindowRequestedEventArgs *args) -> HRESULT {
                            return this->onNewWindowRequested(webview, args);
                        })
                        .Get(),
                &token);
        Q_ASSERT_SUCCEEDED(hr);

        ComPtr<ICoreWebView2_22> webview22;
        hr = m_webview->QueryInterface(IID_PPV_ARGS(&webview22));
        Q_ASSERT_SUCCEEDED(hr);

        // add_DOMContentLoaded
        hr = webview22->add_DOMContentLoaded(
                Microsoft::WRL::Callback<ICoreWebView2DOMContentLoadedEventHandler>(
                        [this](ICoreWebView2 *webview, ICoreWebView2DOMContentLoadedEventArgs *args)
                                -> HRESULT { return this->onDOMContentLoaded(webview, args); })
                        .Get(),
                &token);
        Q_ASSERT_SUCCEEDED(hr);

        // AddWebResourceRequestedFilterWithRequestSourceKinds
        hr = webview22->AddWebResourceRequestedFilterWithRequestSourceKinds(
                L"file://*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL,
                COREWEBVIEW2_WEB_RESOURCE_REQUEST_SOURCE_KINDS_ALL);
        Q_ASSERT_SUCCEEDED(hr);
        QTimer::singleShot(0, this, &QWebView2WebViewPrivate::updateWindowGeometry);
        return S_OK;
    });
    using W2EnvironmentCallback = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
    auto environmentCallback = Microsoft::WRL::Callback<W2EnvironmentCallback>(
            [hWnd, thisPtr, controllerCallback](HRESULT result,
                                                ICoreWebView2Environment *env) -> HRESULT {
                env->CreateCoreWebView2Controller(hWnd, controllerCallback.Get());
                return S_OK;
            });
    CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataFolder.toStdWString().c_str(),
                                             nullptr, environmentCallback.Get());
}
