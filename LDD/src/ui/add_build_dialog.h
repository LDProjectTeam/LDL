#ifndef ADD_BUILD_DIALOG_H
#define ADD_BUILD_DIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>

#include "../core/build_info.h"

class AddBuildDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddBuildDialog(QWidget *parent = nullptr);

    BuildInfo build() const;
    void accept() override;

private:
    QLineEdit *idEdit;
    QLineEdit *nameEdit;
    QTextEdit *descEdit;
    QLineEdit *mcVersionEdit;
    QSpinBox *javaVersionSpin;
    QLineEdit *urlEdit;
    QLineEdit *checksumEdit;
    QSpinBox *sizeSpin;
    QLineEdit *imageUrlEdit;
    QLineEdit *tagsEdit;
};

#endif // ADD_BUILD_DIALOG_H
