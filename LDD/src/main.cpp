#include <QApplication>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QElapsedTimer>
#include <QThread>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "ui/register_dialog.h"
#include "ui/language_dialog.h"
#include "ui/i18n.h"
#include "ui/main_window.h"

namespace {

QString defaultApiBase()
{
    QString env = qEnvironmentVariable("LAUNCHER_API_URL");
    if (env.isEmpty()) {
        return "https://patient-lurleen-ldproject-aabc811a.koyeb.app";
    }
    return env;
}

QString sanitizeApiBase(QString value)
{
    if (value.contains("patient-urleen-ldproject-aabc811a.koyeb.app")) {
        value.replace("patient-urleen-ldproject-aabc811a.koyeb.app",
                      "patient-lurleen-ldproject-aabc811a.koyeb.app");
    }
    return value;
}

QString resolveAppRoot()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    if (dir.exists("backend")) {
        return dir.absolutePath();
    }
    dir.cdUp();
    if (dir.exists("backend")) {
        return dir.absolutePath();
    }
    return appDir;
}

bool isBackendHealthy(const QString &apiBaseUrl, int timeoutMs)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(apiBaseUrl + "/health"));
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    bool ok = timer.isActive() && reply->error() == QNetworkReply::NoError;
    reply->deleteLater();
    return ok;
}

bool isLocalApi(const QString &apiBaseUrl)
{
    QUrl url(apiBaseUrl);
    QString host = url.host().toLower();
    return host == "127.0.0.1" || host == "localhost";
}

bool startBackendDetached(const QString &appRoot)
{
    QString backendDir = QDir(appRoot).filePath("backend");
    if (!QFileInfo::exists(backendDir)) {
        return false;
    }

    QString pythonPath = QDir(appRoot).filePath(".venv/Scripts/python.exe");
    if (!QFileInfo::exists(pythonPath)) {
        pythonPath = QDir(appRoot).filePath(".venv/Scripts/pythonw.exe");
    }
    if (!QFileInfo::exists(pythonPath)) {
        pythonPath = "python";
    }

    QStringList args;
    args << "-m" << "uvicorn" << "app.main:app" << "--host" << "127.0.0.1" << "--port" << "8000";
    QProcess process;
    process.setProgram(pythonPath);
    process.setArguments(args);
    process.setWorkingDirectory(backendDir);
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        args->flags |= CREATE_NO_WINDOW;
    });
#endif
#endif
    return process.startDetached();
}

bool loadSession(QString &email, QString &token, QString &apiBaseUrl)
{
    QSettings settings;
    email = settings.value("auth/email").toString();
    token = settings.value("auth/token").toString();
    apiBaseUrl = settings.value("auth/apiBaseUrl").toString();
    const QString envApi = qEnvironmentVariable("LAUNCHER_API_URL");
    if (!envApi.isEmpty()) {
        apiBaseUrl = envApi;
    }
    if (apiBaseUrl.isEmpty()) {
        apiBaseUrl = defaultApiBase();
    }
    apiBaseUrl = sanitizeApiBase(apiBaseUrl);
    settings.setValue("auth/apiBaseUrl", apiBaseUrl);
    return !email.isEmpty() && !token.isEmpty();
}

void saveSession(const QString &email, const QString &token, const QString &apiBaseUrl)
{
    QSettings settings;
    settings.setValue("auth/email", email);
    settings.setValue("auth/token", token);
    settings.setValue("auth/apiBaseUrl", apiBaseUrl);
}

bool ensureBackendRunning(const QString &apiBaseUrl, QString *errorMessage)
{
    const bool localApi = isLocalApi(apiBaseUrl);
    const int initialTimeout = localApi ? 1200 : 6000;
    if (isBackendHealthy(apiBaseUrl, initialTimeout)) {
        return true;
    }

    if (localApi) {
        QString appRoot = resolveAppRoot();
        if (!startBackendDetached(appRoot)) {
            if (errorMessage) {
                *errorMessage = I18n::tr("Backend not found. Keep the backend folder next to the launcher or set LAUNCHER_API_URL.");
            }
            return false;
        }

        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < 6000) {
            if (isBackendHealthy(apiBaseUrl, 700)) {
                return true;
            }
            QThread::msleep(300);
        }
    }

    if (!localApi) {
        for (int i = 0; i < 2; ++i) {
            QThread::msleep(1500);
            if (isBackendHealthy(apiBaseUrl, 6000)) {
                return true;
            }
        }
    }

    if (errorMessage && errorMessage->isEmpty()) {
        *errorMessage = I18n::tr("Server is not running. Please start the backend and try again.")
                            + "\n\n" + apiBaseUrl;
    }
    return false;
}

} // namespace

int main(int argc, char *argv[])
{
#ifdef Q_OS_WIN
    HWND hwnd = GetConsoleWindow();
    if (hwnd) {
        ShowWindow(hwnd, SW_HIDE);
    }
#endif
    QApplication app(argc, argv);
    app.setOrganizationName("LDP");
    app.setApplicationName("LD Launcher");
    app.setStyleSheet(R"QSS(
        QMessageBox {
            background-color: #1b1f2a;
            color: #e8ecf2;
            border-radius: 10px;
        }
        QMessageBox QLabel {
            color: #e8ecf2;
        }
        QMessageBox QPushButton {
            background-color: #2b3446;
            border: 1px solid #39425a;
            border-radius: 8px;
            padding: 6px 14px;
            color: #e8ecf2;
        }
        QMessageBox QPushButton:hover {
            background-color: #33405a;
        }
        QToolTip {
            background-color: #1b1f2a;
            color: #e8ecf2;
            border: 1px solid #2a3040;
            border-radius: 6px;
        }
        QMenu {
            background-color: #1b1f2a;
            border: 1px solid #2a3040;
            border-radius: 10px;
        }
        QMenu::item {
            padding: 6px 14px 6px 32px;
            color: #e8ecf2;
            border-radius: 6px;
        }
        QMenu::icon {
            left: 10px;
        }
        QMenu::item:selected {
            background-color: #33405a;
            color: #e8ecf2;
            border-radius: 6px;
        }
        QComboBox {
            background-color: #202635;
            border: 1px solid #2c3446;
            border-radius: 6px;
            padding: 4px 24px 4px 6px;
            color: #e8ecf2;
        }
        QComboBox::drop-down {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 20px;
            border-left: 1px solid #2c3446;
            background-color: #202635;
            border-top-right-radius: 6px;
            border-bottom-right-radius: 6px;
        }
        QComboBox::down-arrow {
            image: url(":/icons/chevron_down.svg");
            width: 8px;
            height: 6px;
        }
        QComboBox QAbstractItemView {
            background-color: #1b1f2a;
            color: #e8ecf2;
            border: 1px solid #2a3040;
            border-radius: 8px;
            selection-background-color: #33405a;
            selection-color: #e8ecf2;
            outline: none;
        }
        QComboBox QAbstractItemView::item {
            padding: 4px 8px;
            border-radius: 6px;
        }
        QComboBox QAbstractItemView::item:selected {
            background-color: #33405a;
            border-radius: 6px;
        }
    )QSS");

    QSettings uiSettings;
    QString lang = uiSettings.value("ui/lang").toString();
    if (lang.isEmpty()) {
        LanguageDialog langDialog;
        if (langDialog.exec() != QDialog::Accepted) {
            return 0;
        }
        lang = langDialog.selectedLanguage();
        if (lang.isEmpty()) {
            lang = "en";
        }
        uiSettings.setValue("ui/lang", lang);
    }
    I18n::setLanguage(lang);

    QString email;
    QString token;
    QString apiBaseUrl;

    const bool hasSession = loadSession(email, token, apiBaseUrl);
    if (isLocalApi(apiBaseUrl)) {
        const QString appRoot = resolveAppRoot();
        const QString backendDir = QDir(appRoot).filePath("backend");
        if (!QFileInfo::exists(backendDir)) {
            const QString fallback = defaultApiBase();
            if (!fallback.isEmpty() && !isLocalApi(fallback)) {
                apiBaseUrl = fallback;
                QSettings settings;
                settings.setValue("auth/apiBaseUrl", apiBaseUrl);
            }
        }
    }

    QString backendError;
    if (!ensureBackendRunning(apiBaseUrl, &backendError)) {
        QMessageBox::warning(nullptr, I18n::tr("Server"), backendError);
        return 0;
    }

    if (!hasSession) {
        RegisterDialog registerDialog;
        if (registerDialog.exec() != QDialog::Accepted) {
            return 0;
        }
        email = registerDialog.registeredEmail();
        token = registerDialog.accessToken();
        apiBaseUrl = registerDialog.apiBaseUrl();
        saveSession(email, token, apiBaseUrl);
    }

    MainWindow window(email, apiBaseUrl, token);
    window.show();

    return app.exec();
}
