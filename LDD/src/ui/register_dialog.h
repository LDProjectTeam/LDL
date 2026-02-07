#ifndef REGISTER_DIALOG_H
#define REGISTER_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>

class QLabel;
class QNetworkAccessManager;

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);

    QString registeredEmail() const;
    QString accessToken() const;
    QString apiBaseUrl() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onRegisterClicked();
    void onToggleMode();
    void onGoogleClicked();
    void pollGoogleStatus();

private:
    void updateModeUI();

    QLineEdit *emailEdit;
    QLineEdit *passwordEdit;
    QLineEdit *confirmEdit;
    QLabel *confirmLabel;
    QPushButton *registerButton;
    QPushButton *toggleButton;
    QPushButton *googleButton;
    QLabel *googleStatusLabel;
    QNetworkAccessManager *googleManager;
    QTimer *googlePollTimer;
    QElapsedTimer googleElapsed;
    QString googleDeviceId;
    bool googlePolling = false;

    QString email;
    QString token;
    QString baseUrl;
    bool registerMode = true;
};

#endif // REGISTER_DIALOG_H
