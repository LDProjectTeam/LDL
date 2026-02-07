#include "register_dialog.h"
#include "i18n.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QFont>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QDesktopServices>
#include <QUuid>
#include <QSettings>
#include <QCloseEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>

namespace {

bool isOfflineNetworkError(QNetworkReply::NetworkError error)
{
    switch (error) {
    case QNetworkReply::HostNotFoundError:
    case QNetworkReply::NetworkSessionFailedError:
    case QNetworkReply::TemporaryNetworkFailureError:
    case QNetworkReply::UnknownNetworkError:
        return true;
    default:
        return false;
    }
}

QString extractDetail(const QByteArray &body)
{
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        return {};
    }
    return doc.object().value("detail").toString();
}

QString friendlyAuthError(bool registerMode, QNetworkReply *reply, const QByteArray &body)
{
    if (!reply) {
        return I18n::tr("Unknown error.");
    }

    switch (reply->error()) {
    case QNetworkReply::ConnectionRefusedError:
        return I18n::tr("Server is not running. Please start the backend and try again.");
    case QNetworkReply::TimeoutError:
        return I18n::tr("Server is not responding. Please try again later.");
    default:
        break;
    }

    if (isOfflineNetworkError(reply->error())) {
        return I18n::tr("No internet connection. Please check your network.");
    }

    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (registerMode && status == 409) {
        return I18n::tr("An account with this email already exists.");
    }
    if (!registerMode && (status == 401 || status == 403)) {
        return I18n::tr("Invalid email or password.");
    }
    if (status == 422) {
        return I18n::tr("Invalid input. Check email and password.");
    }
    if (status >= 500) {
        return I18n::tr("Server error. Please try again later.");
    }

    const QString detail = extractDetail(body);
    if (!detail.isEmpty()) {
        return detail;
    }

    if (reply->error() != QNetworkReply::NoError) {
        return I18n::tr("Network error: %1").arg(reply->errorString());
    }

    return I18n::tr("Unknown error.");
}

void applyFadeIn(QWidget *widget, int durationMs = 180)
{
    QSettings settings;
    if (!widget || !settings.value("ui/animations", true).toBool()) {
        return;
    }
    auto *effect = new QGraphicsOpacityEffect(widget);
    widget->setGraphicsEffect(effect);
    effect->setOpacity(0.0);
    auto *anim = new QPropertyAnimation(effect, "opacity", widget);
    anim->setDuration(durationMs);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    if (widget->isWindow()) {
        const QPoint endPos = widget->pos();
        const QPoint startPos = endPos + QPoint(0, 8);
        widget->move(startPos);
        auto *posAnim = new QPropertyAnimation(widget, "pos", widget);
        posAnim->setDuration(durationMs);
        posAnim->setStartValue(startPos);
        posAnim->setEndValue(endPos);
        posAnim->setEasingCurve(QEasingCurve::OutCubic);
        posAnim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

} // namespace

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(I18n::tr("Account"));
    setFixedSize(420, 270);
    setModal(true);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    QFormLayout *form = new QFormLayout();
    emailEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    confirmEdit = new QLineEdit(this);

    emailEdit->setPlaceholderText(I18n::tr("email@gmail.com"));
    passwordEdit->setEchoMode(QLineEdit::Password);
    confirmEdit->setEchoMode(QLineEdit::Password);

    form->addRow(I18n::tr("Email"), emailEdit);
    form->addRow(I18n::tr("Password"), passwordEdit);
    confirmLabel = new QLabel(I18n::tr("Confirm"), this);
    form->addRow(confirmLabel, confirmEdit);
    root->addLayout(form);

    QHBoxLayout *actions = new QHBoxLayout();
    toggleButton = new QPushButton(I18n::tr("Already have an account? Log in"), this);
    toggleButton->setFlat(true);
    toggleButton->setObjectName("ToggleButton");
    actions->addWidget(toggleButton);
    actions->addStretch();
    registerButton = new QPushButton(I18n::tr("Register"), this);
    registerButton->setDefault(true);
    actions->addWidget(registerButton);
    root->addLayout(actions);

    googleButton = new QPushButton(I18n::tr("Sign in with Google"), this);
    googleButton->setObjectName("GoogleButton");
    root->addWidget(googleButton);

    googleStatusLabel = new QLabel(this);
    googleStatusLabel->setObjectName("GoogleStatus");
    googleStatusLabel->setStyleSheet("color: #a9b9df;");
    googleStatusLabel->setWordWrap(true);
    googleStatusLabel->setVisible(false);
    root->addWidget(googleStatusLabel);

    connect(registerButton, &QPushButton::clicked, this, &RegisterDialog::onRegisterClicked);
    connect(toggleButton, &QPushButton::clicked, this, &RegisterDialog::onToggleMode);
    connect(googleButton, &QPushButton::clicked, this, &RegisterDialog::onGoogleClicked);

    QSettings settings;
    baseUrl = settings.value("auth/apiBaseUrl").toString();
    const QString envApi = qEnvironmentVariable("LAUNCHER_API_URL");
    if (!envApi.isEmpty()) {
        baseUrl = envApi;
    }
    if (baseUrl.isEmpty()) {
        baseUrl = "https://patient-lurleen-ldproject-aabc811a.koyeb.app";
    }
    if (baseUrl.contains("patient-urleen-ldproject-aabc811a.koyeb.app")) {
        baseUrl.replace("patient-urleen-ldproject-aabc811a.koyeb.app",
                        "patient-lurleen-ldproject-aabc811a.koyeb.app");
    }
    settings.setValue("auth/apiBaseUrl", baseUrl);
    googleManager = new QNetworkAccessManager(this);
    googlePollTimer = new QTimer(this);
    googlePollTimer->setInterval(1200);
    connect(googlePollTimer, &QTimer::timeout, this, &RegisterDialog::pollGoogleStatus);

    setStyleSheet(R"QSS(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #151c29, stop:1 #1c2636);
            color: #e8ecf2;
            border: 1px solid #2f3b52;
            border-radius: 12px;
        }
        QLineEdit {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #222c3e, stop:1 #1d2737);
            border: 1px solid #2f3b52;
            border-radius: 8px;
            padding: 6px 10px;
            color: #e8ecf2;
        }
        QLineEdit:focus {
            border: 1px solid #5aa9e6;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #5aa9e6, stop:1 #7bc0f0);
            border: none;
            border-radius: 10px;
            padding: 8px 14px;
            color: #0b1020;
            font-weight: bold;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #6bb7f0, stop:1 #8ad0ff);
        }
        QPushButton#ToggleButton {
            background-color: transparent;
            color: #a9b9df;
            padding: 0;
            border: none;
            text-align: left;
        }
        QPushButton#GoogleButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #263246, stop:1 #1f293a);
            border: 1px solid #3a4763;
            border-radius: 10px;
            padding: 8px 14px;
            color: #e8ecf2;
            font-weight: 600;
        }
        QPushButton#GoogleButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2f3b52, stop:1 #243247);
        }
        QLabel#GoogleStatus {
            color: #9fb0d6;
        }
    )QSS");

    updateModeUI();
    QTimer::singleShot(0, this, [this]() { applyFadeIn(this, 200); });
}

QString RegisterDialog::registeredEmail() const
{
    return email;
}

QString RegisterDialog::accessToken() const
{
    return token;
}

QString RegisterDialog::apiBaseUrl() const
{
    return baseUrl;
}

void RegisterDialog::onRegisterClicked()
{
    const QString mail = emailEdit->text().trimmed();
    const QString pass = passwordEdit->text();
    const QString confirm = confirmEdit->text();

    if (mail.isEmpty() || !mail.contains('@')) {
        QMessageBox::warning(this, I18n::tr("Register"), I18n::tr("Enter a valid email."));
        return;
    }
    if (pass.length() < 6) {
        QMessageBox::warning(this, I18n::tr("Account"), I18n::tr("Password must be at least 6 characters."));
        return;
    }
    if (registerMode && pass != confirm) {
        QMessageBox::warning(this, I18n::tr("Account"), I18n::tr("Passwords do not match."));
        return;
    }

    QNetworkAccessManager manager;
    QString endpoint = registerMode ? "/auth/register" : "/auth/login";
    QNetworkRequest request(QUrl(baseUrl + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject payload;
    payload["email"] = mail;
    payload["password"] = pass;

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray body = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
        QMessageBox::warning(this, I18n::tr("Account"), friendlyAuthError(registerMode, reply, body));
        reply->deleteLater();
        return;
    }
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        QMessageBox::warning(this, I18n::tr("Register"), I18n::tr("Invalid response from server."));
        return;
    }

    QJsonObject obj = doc.object();
    token = obj.value("access_token").toString();
    if (token.isEmpty()) {
        QMessageBox::warning(this, I18n::tr("Account"), I18n::tr("Missing access token."));
        return;
    }

    email = mail;
    accept();
}

void RegisterDialog::onToggleMode()
{
    registerMode = !registerMode;
    updateModeUI();
}

void RegisterDialog::closeEvent(QCloseEvent *event)
{
    if (googlePolling) {
        event->ignore();
        if (googleStatusLabel) {
            googleStatusLabel->setText(I18n::tr("Waiting for Google login..."));
            googleStatusLabel->setVisible(true);
        }
        return;
    }
    QDialog::closeEvent(event);
}

void RegisterDialog::updateModeUI()
{
    if (registerMode) {
        setWindowTitle(I18n::tr("Register"));
        registerButton->setText(I18n::tr("Register"));
        toggleButton->setText(I18n::tr("Already have an account? Log in"));
        confirmEdit->setVisible(true);
        confirmLabel->setVisible(true);
    } else {
        setWindowTitle(I18n::tr("Log in"));
        registerButton->setText(I18n::tr("Log in"));
        toggleButton->setText(I18n::tr("No account? Register"));
        confirmEdit->setVisible(false);
        confirmLabel->setVisible(false);
    }
}

void RegisterDialog::onGoogleClicked()
{
    if (googlePolling) {
        return;
    }
    googleButton->setEnabled(false);
    if (googleStatusLabel) {
        googleStatusLabel->setText(I18n::tr("Waiting for Google login..."));
        googleStatusLabel->setVisible(true);
    }
    googleDeviceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QUrl url(baseUrl + "/auth/google/start");
    QUrlQuery query;
    query.addQueryItem("device_id", googleDeviceId);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = googleManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray body = reply->readAll();
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            QString detail;
            QJsonDocument doc = QJsonDocument::fromJson(body);
            if (doc.isObject()) {
                detail = doc.object().value("detail").toString();
            }
            if (detail.isEmpty()) {
                detail = I18n::tr("Google login failed.");
            }
            if (googleStatusLabel) {
                googleStatusLabel->setText(detail);
                googleStatusLabel->setVisible(true);
            }
            googleButton->setEnabled(true);
            QMessageBox::warning(this, I18n::tr("Account"), detail);
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            googleButton->setEnabled(true);
            QMessageBox::warning(this, I18n::tr("Account"), I18n::tr("Google login failed."));
            return;
        }
        const QString authUrl = doc.object().value("authUrl").toString();
        if (authUrl.isEmpty()) {
            googleButton->setEnabled(true);
            QMessageBox::warning(this, I18n::tr("Account"), I18n::tr("Google login failed."));
            return;
        }

        QDesktopServices::openUrl(QUrl(authUrl));
        googlePolling = true;
        googleElapsed.restart();
        googlePollTimer->start();
    });
}

void RegisterDialog::pollGoogleStatus()
{
    if (!googlePolling) {
        return;
    }
    if (googleElapsed.hasExpired(120000)) {
        googlePolling = false;
        googlePollTimer->stop();
        googleButton->setEnabled(true);
        if (googleStatusLabel) {
            googleStatusLabel->setText(I18n::tr("Google login timed out. Please try again."));
            googleStatusLabel->setVisible(true);
        }
        QMessageBox::warning(this, I18n::tr("Account"), I18n::tr("Google login timed out. Please try again."));
        return;
    }

    QUrl url(baseUrl + "/auth/google/poll");
    QUrlQuery query;
    query.addQueryItem("device_id", googleDeviceId);
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = googleManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray body = reply->readAll();
        reply->deleteLater();

        if (statusCode == 204) {
            return;
        }
        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            googlePolling = false;
            googlePollTimer->stop();
            googleButton->setEnabled(true);
            if (googleStatusLabel) {
                googleStatusLabel->setText(I18n::tr("Google login failed."));
                googleStatusLabel->setVisible(true);
            }
            QMessageBox::warning(this, I18n::tr("Account"), I18n::tr("Google login failed."));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            return;
        }
        const QString access = doc.object().value("access_token").toString();
        const QString emailValue = doc.object().value("email").toString();
        if (access.isEmpty() || emailValue.isEmpty()) {
            return;
        }

        token = access;
        email = emailValue;
        googlePolling = false;
        googlePollTimer->stop();
        googleButton->setEnabled(true);
        accept();
    });
}
