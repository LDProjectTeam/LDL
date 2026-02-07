#include "language_dialog.h"
#include "i18n.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>
#include <QSettings>

namespace {
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
}

LanguageDialog::LanguageDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(I18n::tr("Select language"));
    setFixedSize(360, 180);
    setModal(true);

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    QLabel *title = new QLabel(I18n::tr("Choose your language"), this);
    QFont titleFont("Montserrat", 12, QFont::Bold);
    title->setFont(titleFont);
    root->addWidget(title);

    QHBoxLayout *row = new QHBoxLayout();
    QPushButton *en = new QPushButton(QIcon(":/icons/flag_en.svg"), "English", this);
    en->setObjectName("LangButton");
    QPushButton *ru = new QPushButton(QIcon(":/icons/flag_ru.svg"), "Русский", this);
    ru->setObjectName("LangButton");
    QPushButton *uk = new QPushButton(QIcon(":/icons/flag_uk.svg"), "Українська", this);
    uk->setObjectName("LangButton");
    const QSize iconSize(16, 11);
    en->setIconSize(iconSize);
    ru->setIconSize(iconSize);
    uk->setIconSize(iconSize);
    row->addWidget(en);
    row->addWidget(ru);
    row->addWidget(uk);
    root->addLayout(row);

    connect(en, &QPushButton::clicked, this, [this]() {
        lang = "en";
        accept();
    });
    connect(ru, &QPushButton::clicked, this, [this]() {
        lang = "ru";
        accept();
    });
    connect(uk, &QPushButton::clicked, this, [this]() {
        lang = "uk";
        accept();
    });

    setStyleSheet(R"QSS(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #151c29, stop:1 #1c2636);
            color: #e8ecf2;
            border: 1px solid #2f3b52;
            border-radius: 12px;
        }
        QPushButton#LangButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2f3b52, stop:1 #263246);
            border: 1px solid #3a4763;
            border-radius: 10px;
            padding: 8px 14px;
            color: #e8ecf2;
            min-width: 100px;
        }
        QPushButton#LangButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #36455f, stop:1 #2b3a51);
        }
    )QSS");

    QTimer::singleShot(0, this, [this]() { applyFadeIn(this, 180); });
}

QString LanguageDialog::selectedLanguage() const
{
    return lang;
}
