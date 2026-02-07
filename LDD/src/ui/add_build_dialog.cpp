#include "add_build_dialog.h"
#include "i18n.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>
#include <QFont>
#include <QSettings>

namespace {
bool animationsEnabled()
{
    QSettings settings;
    return settings.value("ui/animations", true).toBool();
}

void applyFadeIn(QWidget *widget, int durationMs = 180)
{
    if (!widget || !animationsEnabled()) {
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

QString normalizeSha256Input(const QString &value)
{
    QString out = value.trimmed();
    if (out.startsWith("sha256:", Qt::CaseInsensitive)) {
        out = out.mid(7).trimmed();
    } else if (out.startsWith("sha256=", Qt::CaseInsensitive)) {
        out = out.mid(7).trimmed();
    }
    out = out.toLower();
    static const QRegularExpression re("^[0-9a-f]{64}$");
    if (!re.match(out).hasMatch()) {
        return {};
    }
    return out;
}
}

AddBuildDialog::AddBuildDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(I18n::tr("Add Build"));
    setMinimumSize(460, 420);
    setModal(true);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(10);

    QLabel *title = new QLabel(I18n::tr("New Build"), this);
    QFont titleFont("Montserrat", 14, QFont::Bold);
    title->setFont(titleFont);
    root->addWidget(title);

    QFormLayout *form = new QFormLayout();

    idEdit = new QLineEdit(this);
    nameEdit = new QLineEdit(this);
    descEdit = new QTextEdit(this);
    mcVersionEdit = new QLineEdit(this);
    javaVersionSpin = new QSpinBox(this);
    urlEdit = new QLineEdit(this);
    checksumEdit = new QLineEdit(this);
    sizeSpin = new QSpinBox(this);
    imageUrlEdit = new QLineEdit(this);
    tagsEdit = new QLineEdit(this);

    javaVersionSpin->setRange(8, 21);
    javaVersionSpin->setValue(17);
    sizeSpin->setRange(0, 500000);
    sizeSpin->setSuffix(" MB");

    descEdit->setFixedHeight(80);

    form->addRow(I18n::tr("Build ID"), idEdit);
    form->addRow(I18n::tr("Name"), nameEdit);
    form->addRow(I18n::tr("Description"), descEdit);
    form->addRow(I18n::tr("Minecraft Version"), mcVersionEdit);
    form->addRow(I18n::tr("Java Version"), javaVersionSpin);
    form->addRow(I18n::tr("Download URL"), urlEdit);
    form->addRow(I18n::tr("Checksum (sha256)"), checksumEdit);
    form->addRow(I18n::tr("Size"), sizeSpin);
    form->addRow(I18n::tr("Image URL"), imageUrlEdit);
    form->addRow(I18n::tr("Tags (comma)"), tagsEdit);

    root->addLayout(form);

    QHBoxLayout *actions = new QHBoxLayout();
    actions->addStretch();
    QPushButton *cancel = new QPushButton(I18n::tr("Cancel"), this);
    QPushButton *ok = new QPushButton(I18n::tr("Add"), this);
    ok->setDefault(true);
    actions->addWidget(cancel);
    actions->addWidget(ok);
    root->addLayout(actions);

    connect(cancel, &QPushButton::clicked, this, &AddBuildDialog::reject);
    connect(ok, &QPushButton::clicked, this, &AddBuildDialog::accept);

    setStyleSheet(R"QSS(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #151c29, stop:1 #1c2636);
            color: #e8ecf2;
            border: 1px solid #2f3b52;
            border-radius: 12px;
        }
        QLineEdit, QTextEdit, QSpinBox {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #222c3e, stop:1 #1d2737);
            border: 1px solid #2f3b52;
            border-radius: 8px;
            padding: 6px 10px;
            color: #e8ecf2;
        }
        QLineEdit:focus, QTextEdit:focus, QSpinBox:focus {
            border: 1px solid #5aa9e6;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2f3b52, stop:1 #263246);
            border: 1px solid #3a4763;
            border-radius: 10px;
            padding: 8px 14px;
            color: #e8ecf2;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #36455f, stop:1 #2b3a51);
        }
    )QSS");
    QTimer::singleShot(0, this, [this]() { applyFadeIn(this, 180); });
}

void AddBuildDialog::accept()
{
    const QString rawChecksum = checksumEdit->text().trimmed();
    if (!rawChecksum.isEmpty()) {
        const QString normalized = normalizeSha256Input(rawChecksum);
        if (normalized.isEmpty()) {
            QMessageBox::warning(this, I18n::tr("Add Build"),
                                 I18n::tr("Checksum must be a SHA-256 hex string (64 chars)."));
            return;
        }
        checksumEdit->setText(normalized);
    }
    QDialog::accept();
}

BuildInfo AddBuildDialog::build() const
{
    BuildInfo build;
    build.id = idEdit->text().trimmed().toStdString();
    build.name = nameEdit->text().trimmed().toUtf8().toStdString();
    build.description = descEdit->toPlainText().trimmed().toUtf8().toStdString();
    build.minecraftVersion = mcVersionEdit->text().trimmed().toStdString();
    build.javaVersion = javaVersionSpin->value();
    build.downloadUrl = urlEdit->text().trimmed().toStdString();
    build.checksum = normalizeSha256Input(checksumEdit->text()).toStdString();
    build.sizeBytes = static_cast<long long>(sizeSpin->value()) * 1000000LL;
    build.imageUrl = imageUrlEdit->text().trimmed().toStdString();
    build.tagsCsv = tagsEdit->text().trimmed().toStdString();
    return build;
}
