#ifndef LANGUAGE_DIALOG_H
#define LANGUAGE_DIALOG_H

#include <QDialog>
#include <QString>

class LanguageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LanguageDialog(QWidget *parent = nullptr);

    QString selectedLanguage() const;

private:
    QString lang;
};

#endif // LANGUAGE_DIALOG_H
