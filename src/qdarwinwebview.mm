#include "private/qdarwinwebview.h"

#include <QDebug>
#include <QWindow>
#include <QTimer>
#include <QScreen>
#include <QUrl>
#include <QMessageBox>
#include <QInputDialog>
#include <QJsonObject>
#include <QFile>

// clang-format off
#include <CoreFoundation/CoreFoundation.h>
#include <WebKit/WebKit.h>
// clang-format on

#ifdef Q_OS_MACOS
#  include <AppKit/AppKit.h>

typedef NSView UIView;
#endif

@interface QtWKWebViewDelegate : NSObject <WKNavigationDelegate, WKUIDelegate> {
    QDarwinWebViewPrivate *qDarwinWebViewPrivate;
}
- (QtWKWebViewDelegate *)initWithQDarwinWebView:(QDarwinWebViewPrivate *)webViewPrivate;
- (void)pageDone;
- (void)handleError:(NSError *)error;

// protocol:
- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation;
- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation;
- (void)webView:(WKWebView *)webView
        didFailProvisionalNavigation:(WKNavigation *)navigation
                           withError:(NSError *)error;
- (void)webView:(WKWebView *)webView
        didFailNavigation:(WKNavigation *)navigation
                withError:(NSError *)error;

@end

@implementation QtWKWebViewDelegate
- (QtWKWebViewDelegate *)initWithQDarwinWebView:(QDarwinWebViewPrivate *)webViewPrivate
{
    if ((self = [super init])) {
        Q_ASSERT(webViewPrivate);
        qDarwinWebViewPrivate = webViewPrivate;
    }
    return self;
}

- (void)pageDone
{
    Q_EMIT qDarwinWebViewPrivate->loadProgress(qDarwinWebViewPrivate->progressValue());
}

- (void)handleError:(NSError *)error
{
    [self pageDone];
    NSString *errorString = [error localizedDescription];
    NSURL *failingURL = error.userInfo[@"NSErrorFailingURLKey"];
    Q_EMIT qDarwinWebViewPrivate->loadFinished(false);
    const QUrl url =
            [failingURL isKindOfClass:[NSURL class]] ? QUrl::fromNSURL(failingURL) : QUrl();
    qDebug() << "Error loading" << url << ":" << QString::fromNSString(errorString);
    // Q_EMIT qDarwinWebViewPrivate->loadingChanged(QWebViewLoadRequestPrivate(
    //         url, QWebView::LoadFailedStatus, QString::fromNSString(errorString)));
}

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
    Q_UNUSED(webView);
    qDebug() << "Did start provisional navigation";
    // WKNavigationDelegate gives us per-frame notifications while the QWebView API
    // should provide per-page notifications. Therefore we keep track of the last frame
    // to be started, if that finishes or fails then we indicate that it has loaded.
    if (qDarwinWebViewPrivate->m_navigation != navigation)
        qDarwinWebViewPrivate->m_navigation = navigation;
    else
        return;

    // Q_EMIT qDarwinWebViewPrivate->loadingChanged(QWebViewLoadRequestPrivate(
    //         qDarwinWebViewPrivate->url(), QWebView::LoadStartedStatus, QString()));
    Q_EMIT qDarwinWebViewPrivate->loadStarted();
    Q_EMIT qDarwinWebViewPrivate->loadProgress(qDarwinWebViewPrivate->progressValue());
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    Q_UNUSED(webView);
    qDebug() << "Did finish navigation";
    if (qDarwinWebViewPrivate->m_navigation != navigation)
        return;

    [self pageDone];
    Q_EMIT qDarwinWebViewPrivate->loadFinished(true);
    // Q_EMIT qDarwinWebViewPrivate->loadingChanged(QWebViewLoadRequestPrivate(
    //         qDarwinWebViewPrivate->url(), QWebView::LoadSucceededStatus, QString()));
}

- (void)webView:(WKWebView *)webView
        didFailProvisionalNavigation:(WKNavigation *)navigation
                           withError:(NSError *)error
{
    Q_UNUSED(webView);
    qDebug() << "Did fail provisional navigation:"
             << QString::fromNSString([error localizedDescription]);
    if (qDarwinWebViewPrivate->m_navigation != navigation)
        return;
    [self handleError:error];
}

- (void)webView:(WKWebView *)webView
        didFailNavigation:(WKNavigation *)navigation
                withError:(NSError *)error
{
    Q_UNUSED(webView);
    qDebug() << "Did fail navigation:" << QString::fromNSString([error localizedDescription]);
    if (qDarwinWebViewPrivate->m_navigation != navigation)
        return;
    [self handleError:error];
}

- (void)webView:(WKWebView *)webView
        decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction
                        decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
        __attribute__((availability(ios_app_extension, unavailable)))
{
    Q_UNUSED(webView);
    NSURL *url = navigationAction.request.URL;
    const BOOL handled = (^{
        // For links with target="_blank", open externally
        if (!navigationAction.targetFrame)
            return NO;

#if QT_MACOS_IOS_PLATFORM_SDK_EQUAL_OR_ABOVE(101300, 110000)
        if (__builtin_available(macOS 10.13, iOS 11.0, *)) {
            return [WKWebView handlesURLScheme:url.scheme];
        } else
#endif
        {
            // +[WKWebView handlesURLScheme:] is a stub that calls
            // WebCore::SchemeRegistry::isBuiltinScheme();
            // replicate that as closely as possible
            return [@[
                @"about", @"applewebdata", @"blob", @"data", @"file", @"http", @"https",
                @"javascript",
#ifdef Q_OS_MACOS
                @"safari-extension",
#endif
                @"webkit-fake-url", @"wss", @"x-apple-content-filter",
#ifdef Q_OS_MACOS
                @"x-apple-ql-id"
#endif
            ] containsObject:url.scheme];
        }
    })();
    if (!handled) {
#ifdef Q_OS_MACOS
        [[NSWorkspace sharedWorkspace] openURL:url];
#endif
    }
    decisionHandler(handled ? WKNavigationActionPolicyAllow : WKNavigationActionPolicyCancel);
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
    Q_UNUSED(object);
    Q_UNUSED(change);
    Q_UNUSED(context);
    if ([keyPath isEqualToString:@"estimatedProgress"]) {
        Q_EMIT qDarwinWebViewPrivate->loadProgress(qDarwinWebViewPrivate->progressValue());
    } else if ([keyPath isEqualToString:@"title"]) {
        Q_EMIT qDarwinWebViewPrivate->titleChanged(qDarwinWebViewPrivate->title());
    }
}

- (void)webView:(WKWebView *)webView
        runJavaScriptAlertPanelWithMessage:(NSString *)message
                          initiatedByFrame:(WKFrameInfo *)frame
                         completionHandler:(void (^)(void))completionHandler
{
    Q_UNUSED(webView);
    Q_UNUSED(frame);

    QMessageBox::warning(nullptr, "Alert", QString::fromNSString(message));
    completionHandler();
}

- (void)webView:(WKWebView *)webView
        runJavaScriptConfirmPanelWithMessage:(NSString *)message
                            initiatedByFrame:(WKFrameInfo *)frame
                           completionHandler:(void (^)(BOOL result))completionHandler
{
    Q_UNUSED(webView);
    Q_UNUSED(frame);

    QMessageBox::StandardButton result =
            QMessageBox::question(nullptr, "Confirm", QString::fromNSString(message));
    completionHandler(result == QMessageBox::Yes);
}

- (void)webView:(WKWebView *)webView
        runJavaScriptTextInputPanelWithPrompt:(NSString *)prompt
                                  defaultText:(NSString *)defaultText
                             initiatedByFrame:(WKFrameInfo *)frame
                            completionHandler:(void (^)(NSString *result))completionHandler
{
    Q_UNUSED(webView);
    Q_UNUSED(frame);

    QString result = QInputDialog::getText(nullptr, "Prompt", QString::fromNSString(prompt),
                                           QLineEdit::Normal, QString::fromNSString(defaultText));
    completionHandler(result.toNSString());
}

@end

QDarwinWebViewPrivate::QDarwinWebViewPrivate(QObject *parent)
    : QNativeWebViewPrivate(parent), m_webview(nil), m_navigation(nil), m_window(nullptr)
{
    initialize();
}

QDarwinWebViewPrivate::~QDarwinWebViewPrivate()
{
    [m_webview stopLoading];
    [m_webview removeObserver:m_webview.navigationDelegate
                   forKeyPath:@"estimatedProgress"
                      context:nil];
    [m_webview removeObserver:m_webview.navigationDelegate forKeyPath:@"title" context:nil];
    [m_webview.navigationDelegate release];
    m_webview.navigationDelegate = nil;
    [m_webview release];

    if (m_window) {
        m_window->destroy();
    }
}

void QDarwinWebViewPrivate::load(const QUrl &url)
{
    if (m_webview && url.isValid()) {
        if (url.isLocalFile()) {
            // NOTE: Check if the file exists before attempting to load it, we follow the same
            // asynchronous pattern as expected to not break the tests (Started + Failed).
            if (!QFile::exists(url.toLocalFile())) {
                emit loadStarted();
                emit loadFinished(false);
                return;
            }
            // We need to pass local files via loadFileURL and the read access should cover
            // the directory that the file is in, to facilitate loading referenced images etc
            [m_webview loadFileURL:url.toNSURL()
                    allowingReadAccessToURL:QUrl(url.toString(QUrl::RemoveFilename)).toNSURL()];
        } else {
            [m_webview loadRequest:[NSURLRequest requestWithURL:url.toNSURL()]];
        }
    } else {
        emit loadFinished(false);
    }
}

void QDarwinWebViewPrivate::setHtml(const QString &html, const QUrl &baseUrl)
{
    if (m_webview) {
        [m_webview loadHTMLString:html.toNSString() baseURL:baseUrl.toNSURL()];
    }
}

void QDarwinWebViewPrivate::stop()
{
    if (m_webview) {
        [m_webview stopLoading];
    }
}

void QDarwinWebViewPrivate::back()
{
    if (m_webview) {
        [m_webview goBack];
    }
}

void QDarwinWebViewPrivate::forward()
{
    if (m_webview) {
        [m_webview goForward];
    }
}

void QDarwinWebViewPrivate::reload()
{
    if (m_webview) {
        [m_webview reload];
    }
}

QString QDarwinWebViewPrivate::title() const
{
    if (m_webview) {
        return QString::fromNSString(m_webview.title);
    }

    return "";
}

int QDarwinWebViewPrivate::progressValue() const
{
    if (m_webview) {
        return int(m_webview.estimatedProgress * 100);
    }
    return 0;
}

QWindow *QDarwinWebViewPrivate::nativeWindow()
{
    return m_window;
}

QString QDarwinWebViewPrivate::errorString() const
{
    return "";
}

QString QDarwinWebViewPrivate::userAgent() const
{
    if (m_webview) {
        return QString::fromNSString(m_webview.customUserAgent);
    }

    return "";
}

bool QDarwinWebViewPrivate::setUserAgent(const QString &userAgent)
{
    if (m_webview && !userAgent.isEmpty()) {
        m_webview.customUserAgent = userAgent.toNSString();
        return true;
    }

    return false;
}

void QDarwinWebViewPrivate::allCookies(const std::function<void(const QJsonObject &)> &callback)
{
    if (m_webview) {
        WKHTTPCookieStore *cookieStore = m_webview.configuration.websiteDataStore.httpCookieStore;
        if (cookieStore == nullptr) {
            if (callback) {
                QMetaObject::invokeMethod(this, [&callback] { callback(QJsonObject()); });
            }
            return;
        }

        [cookieStore getAllCookies:^(NSArray *cookies) {
            QJsonObject result;
            for (NSHTTPCookie *cookie in cookies) {
                // domain
                QString strDomain = QString::fromNSString(cookie.domain);
                if (!result.contains(strDomain)) {
                    result.insert(strDomain, QJsonObject());
                }
                // name
                QString strName = QString::fromNSString(cookie.name);
                if (strName.isEmpty()) {
                    continue;
                }

                // expires
                double expires = 0;
                NSDate *expiresDate = cookie.expiresDate;
                if (expiresDate != nil) {
                    expires = [expiresDate timeIntervalSince1970];
                }

                QJsonValueRef refDomain = result[strDomain];
                QJsonObject jsonDomain = refDomain.toObject();
                jsonDomain[strName] = QJsonObject{
                    { "value", QString::fromNSString(cookie.value) },
                    { "path", QString::fromNSString(cookie.path) },
                    { "expires", expires },
                    { "httpOnly", (bool)cookie.HTTPOnly },
                    { "secure", (bool)cookie.secure },
                    { "session", (bool)cookie.sessionOnly },
                };
                refDomain = jsonDomain;
            }
            if (callback) {
                QMetaObject::invokeMethod(this, [&callback, &result] { callback(result); });
            }
        }];
        return;
    } else {
        if (callback) {
            callback(QJsonObject());
        }
    }
}

bool QDarwinWebViewPrivate::setCookie(const QString &domain, const QString &name,
                                      const QString &value)
{
    if (m_webview) {
        NSString *cookieDomain = domain.toNSString();
        NSString *cookieName = name.toNSString();
        NSString *cookieValue = value.toNSString();

        WKHTTPCookieStore *cookieStore = m_webview.configuration.websiteDataStore.httpCookieStore;

        if (cookieStore == nullptr) {
            return false;
        }

        NSMutableDictionary *cookieProperties = [NSMutableDictionary dictionary];
        [cookieProperties setObject:cookieName forKey:NSHTTPCookieName];
        [cookieProperties setObject:cookieValue forKey:NSHTTPCookieValue];
        [cookieProperties setObject:cookieDomain forKey:NSHTTPCookieDomain];
        [cookieProperties setObject:cookieDomain forKey:NSHTTPCookieOriginURL];
        [cookieProperties setObject:@"/" forKey:NSHTTPCookiePath];
        [cookieProperties setObject:@"0" forKey:NSHTTPCookieVersion];

        NSHTTPCookie *cookie = [NSHTTPCookie cookieWithProperties:cookieProperties];

        if (cookie == nullptr) {
            return false;
        }

        [cookieStore setCookie:cookie
                completionHandler:^{
                        /*
                    qDebug() << "Cookie set:" << QString::fromNSString(cookie.domain)
                             << QString::fromNSString(cookie.name)
                             << QString::fromNSString(cookie.value);
                    */
                }];

        return true;
    }

    return false;
}

void QDarwinWebViewPrivate::deleteCookie(const QString &domain, const QString &name)
{
    if (m_webview) {
        NSString *cookieDomain = domain.toNSString();
        NSString *cookieName = name.toNSString();

        WKHTTPCookieStore *cookieStore = m_webview.configuration.websiteDataStore.httpCookieStore;

        if (cookieStore == nullptr) {
            return;
        }

        [cookieStore getAllCookies:^(NSArray *cookies) {
            NSHTTPCookie *cookie;
            for (cookie in cookies) {
                if ([cookie.domain isEqualToString:cookieDomain] &&
                    [cookie.name isEqualToString:cookieName]) {
                    [cookieStore deleteCookie:cookie
                            completionHandler:^{
                                    /*
                                qDebug()
                                        << "Cookie deleted:" << QString::fromNSString(cookie.domain)
                                        << QString::fromNSString(cookie.name);
                                */
                            }];
                }
            }
        }];
    }
}

void QDarwinWebViewPrivate::deleteAllCookies()
{
    if (m_webview) {
        WKHTTPCookieStore *cookieStore = m_webview.configuration.websiteDataStore.httpCookieStore;

        [cookieStore getAllCookies:^(NSArray *cookies) {
            NSHTTPCookie *cookie;
            for (cookie in cookies) {
                [cookieStore deleteCookie:cookie
                        completionHandler:^{
                                /*
                            qDebug() << "Cookie removed:" << QString::fromNSString(cookie.domain)
                                     << QString::fromNSString(cookie.name);
                            */
                        }];
            }
        }];
    }
}

void QDarwinWebViewPrivate::evaluateJavaScript(
        const QString &scriptSource, const std::function<void(const QVariant &)> &callback)
{
}

void QDarwinWebViewPrivate::updateWindowGeometry() { }

void QDarwinWebViewPrivate::initialize()
{
    CGRect frame = CGRectMake(0.0, 0.0, 400, 400);
    m_webview = [[WKWebView alloc] initWithFrame:frame];
    m_webview.navigationDelegate = [[QtWKWebViewDelegate alloc] initWithQDarwinWebView:this];
    m_webview.UIDelegate = (id<WKUIDelegate>)m_webview.navigationDelegate;

    [m_webview addObserver:m_webview.navigationDelegate
                forKeyPath:@"estimatedProgress"
                   options:NSKeyValueObservingOptions(NSKeyValueObservingOptionNew)
                   context:nil];
    [m_webview addObserver:m_webview.navigationDelegate
                forKeyPath:@"title"
                   options:NSKeyValueObservingOptions(NSKeyValueObservingOptionNew)
                   context:nil];

    if (__builtin_available(macOS 13.3, iOS 16.4, tvOS 16.4, *)) {
        m_webview.inspectable = YES;
    }

    m_window = QWindow::fromWinId(reinterpret_cast<WId>(m_webview));
}
