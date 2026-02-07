#include "settings_dialog.h"
#include "i18n.h"
#include "notification.h"

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QCheckBox>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QPixmap>
#include <QPainterPath>
#include <QEvent>
#include <QRegion>
#include <QSpinBox>
#include <QSlider>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>
#include <QSettings>
#include <QApplication>
#include <QFontDatabase>
#include <algorithm>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {

struct MemoryRecommendation {
    int xms;
    int xmx;
};

int clampValue(int value, int minValue, int maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

QIcon glyphIcon(QChar glyph, const QColor &color = QColor("#c9d3ee"))
{
    QPixmap pix(16, 16);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont font("Segoe MDL2 Assets");
    font.setPixelSize(14);
    p.setFont(font);
    p.setPen(color);
    p.drawText(pix.rect(), Qt::AlignCenter, QString(glyph));
    return QIcon(pix);
}

MemoryRecommendation recommendMemory(int totalMb)
{
    auto roundToStep = [](int value, int step) {
        if (step <= 0) return value;
        return ((value + step / 2) / step) * step;
    };
    int xms = 1024;
    int xmx = 2048;

    if (totalMb >= 16384) {
        xms = 4096;
        xmx = 8192;
    } else if (totalMb >= 12288) {
        xms = 3072;
        xmx = 6144;
    } else if (totalMb >= 8192) {
        xms = 2048;
        xmx = 4096;
    } else if (totalMb >= 6144) {
        xms = 1536;
        xmx = 3072;
    } else {
        xms = 1024;
        xmx = 2048;
    }

    xms = roundToStep(xms, 1024);
    xmx = roundToStep(xmx, 1024);

    int maxSafe = static_cast<int>(totalMb * 0.8);
    if (maxSafe < 1024) {
        maxSafe = totalMb;
    }

    if (xmx > maxSafe) xmx = maxSafe;
    if (xms > maxSafe) xms = maxSafe;
    if (xmx > totalMb) xmx = totalMb;
    if (xms > totalMb) xms = totalMb;
    if (xms > xmx) xmx = xms;
    if (xms < 512) xms = 512;
    if (xmx < 1024) xmx = 1024;
    return {xms, xmx};
}

MemoryRecommendation recommendMemoryDynamic(int totalMb, int availMb)
{
    if (availMb <= 0) {
        availMb = totalMb;
    }
    auto roundToStep = [](int value, int step) {
        if (step <= 0) return value;
        return ((value + step / 2) / step) * step;
    };
    MemoryRecommendation base = recommendMemory(totalMb);
    int availTarget = static_cast<int>(availMb * 0.6);
    availTarget = roundToStep(availTarget, 1024);
    if (availTarget < 1024) {
        availTarget = std::min(1024, totalMb);
    }
    int xmx = std::min(base.xmx, availTarget);
    int xms = std::min(base.xms, std::max(512, xmx / 2));
    if (xms > xmx) xms = xmx;
    if (xmx < 1024) xmx = 1024;
    if (xms < 512) xms = 512;
    return {xms, xmx};
}

int availableRamMb()
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<int>(status.ullAvailPhys / (1024 * 1024));
    }
#endif
    return -1;
}

void applyRoundedMask(QWidget *widget, int radius)
{
    if (!widget) {
        return;
    }
    const QRect rect = widget->rect();
    if (rect.isEmpty()) {
        return;
    }
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    QRegion region(path.toFillPolygon().toPolygon());
    widget->setMask(region);
}

class PopupRounder : public QObject
{
public:
    explicit PopupRounder(int radius, QObject *parent = nullptr)
        : QObject(parent), radius(radius)
    {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (event->type() == QEvent::Show || event->type() == QEvent::Resize) {
            if (auto *widget = qobject_cast<QWidget *>(watched)) {
                applyRoundedMask(widget, radius);
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    int radius;
};

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

SettingsDialog::SettingsDialog(int totalMb,
                               int xmsMb,
                               int xmxMb,
                               bool autoMemory,
                               bool perfJvmProfile,
                               const QString &extraJvmArgs,
                               int downloadLimitKbps,
                               bool musicEnabled,
                               int musicVolume,
                               bool animationsEnabled,
                               bool animateBackgroundEnabled,
                               int availableMb,
                               const QString &langCode,
                               const QString &fontFamily,
                               QWidget *parent)
    : QDialog(parent), totalMb(totalMb), availableMb(availableMb), langCode(langCode), initialFontFamily(fontFamily)
{
    setWindowTitle(I18n::tr("Settings"));
    setModal(true);
    setFixedSize(560, 320);
    perfJvmProfileValue = perfJvmProfile;
    extraJvmArgsValue = extraJvmArgs;

    int minMb = 256;
    int maxMb = totalMb > minMb ? totalMb : minMb;

    xmsSpin = new QSpinBox(this);
    xmxSpin = new QSpinBox(this);
    xmsSpin->setRange(minMb, maxMb);
    xmxSpin->setRange(minMb, maxMb);
    xmsSpin->setSingleStep(1024);
    xmxSpin->setSingleStep(1024);
    xmsSpin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    xmxSpin->setButtonSymbols(QAbstractSpinBox::UpDownArrows);

    xmsSpin->setValue(clampValue(xmsMb, minMb, maxMb));
    xmxSpin->setValue(clampValue(xmxMb, minMb, maxMb));

    connect(xmsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::syncLimits);
    connect(xmxSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsDialog::syncLimits);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(12);

    navList = new QListWidget(this);
    auto *generalItem = new QListWidgetItem(glyphIcon(QChar(0xE713)), I18n::tr("General"));
    auto *themeItem = new QListWidgetItem(glyphIcon(QChar(0xE790)), I18n::tr("Theme"));
    auto *memoryItem = new QListWidgetItem(glyphIcon(QChar(0xE7F8)), I18n::tr("Memory"));
    auto *perfItem = new QListWidgetItem(glyphIcon(QChar(0xE9D9)), I18n::tr("Performance"));
    navList->addItem(generalItem);
    navList->addItem(themeItem);
    navList->addItem(memoryItem);
    navList->addItem(perfItem);
    navList->setIconSize(QSize(16, 16));
    navList->setFixedWidth(230);
    navList->setSpacing(4);
    navList->setSelectionMode(QAbstractItemView::SingleSelection);
    navList->setFocusPolicy(Qt::NoFocus);
    navList->setCurrentRow(0);

    stack = new QStackedWidget(this);

    QWidget *generalPage = new QWidget(this);
    auto *generalLayout = new QVBoxLayout(generalPage);
    generalLayout->setContentsMargins(0, 0, 0, 0);
    generalLayout->setSpacing(10);

    QFormLayout *generalForm = new QFormLayout();
    QLabel *langLabel = new QLabel(I18n::tr("Language"), this);
    langCombo = new QComboBox(this);
    langCombo->addItem(QIcon(":/icons/flag_en.svg"), "English", "en");
    langCombo->addItem(QIcon(":/icons/flag_ru.svg"), "Русский", "ru");
    langCombo->addItem(QIcon(":/icons/flag_uk.svg"), "Українська", "uk");
    langCombo->setIconSize(QSize(16, 11));
    if (auto *view = langCombo->view()) {
        view->setAttribute(Qt::WA_StyledBackground, true);
        view->setFrameStyle(QFrame::NoFrame);
        if (auto *viewport = view->viewport()) {
            viewport->setAutoFillBackground(false);
        }
        view->setStyleSheet(R"QSS(
            QAbstractItemView {
                background-color: #1b2231;
                color: #e8ecf2;
                selection-background-color: #33405a;
                selection-color: #e8ecf2;
                outline: none;
                border: 1px solid #5aa9e6;
                border-radius: 8px;
                padding: 4px;
            }
            QAbstractItemView::item {
                padding: 4px 8px;
                border-radius: 6px;
            }
            QAbstractItemView::item:selected {
                background-color: #33405a;
                border-radius: 6px;
            }
        )QSS");

        if (auto *popup = view->parentWidget()) {
            popup->setAttribute(Qt::WA_StyledBackground, true);
            popup->setObjectName("LangPopup");
            popup->setContentsMargins(2, 2, 2, 2);
            popup->setStyleSheet(R"QSS(
                QWidget#LangPopup {
                    background-color: #1b2231;
                    border: 1px solid #5aa9e6;
                    border-radius: 10px;
                }
            )QSS");
            popup->installEventFilter(new PopupRounder(8, popup));
        }
    }
    int langIndex = langCombo->findData(langCode.isEmpty() ? "en" : langCode);
    if (langIndex >= 0) {
        langCombo->setCurrentIndex(langIndex);
    }
    generalForm->addRow(langLabel, langCombo);
    generalLayout->addLayout(generalForm);

    musicCheck = new QCheckBox(I18n::tr("Enable music"), this);
    musicCheck->setChecked(musicEnabled);
    musicSlider = new QSlider(Qt::Horizontal, this);
    musicSlider->setRange(0, 100);
    musicSlider->setValue(std::clamp(musicVolume, 0, 100));
    musicSlider->setEnabled(musicEnabled);
    connect(musicCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (musicSlider) {
            musicSlider->setEnabled(enabled);
        }
        if (musicPreviewCallback) {
            musicPreviewCallback(enabled, musicSlider ? musicSlider->value() : 0);
        }
    });
    connect(musicSlider, &QSlider::valueChanged, this, [this](int value) {
        if (musicPreviewCallback) {
            musicPreviewCallback(musicCheck ? musicCheck->isChecked() : true, value);
        }
    });

    auto *musicForm = new QFormLayout();
    musicForm->addRow(musicCheck);
    musicForm->addRow(I18n::tr("Music volume"), musicSlider);
    generalLayout->addLayout(musicForm);
    generalLayout->addStretch();

    QWidget *themePage = new QWidget(this);
    auto *themeLayout = new QVBoxLayout(themePage);
    themeLayout->setContentsMargins(0, 0, 0, 0);
    themeLayout->setSpacing(10);

    QLabel *themeTitle = new QLabel(I18n::tr("Theme"), this);
    QFont sectionFont("Montserrat", 10, QFont::Bold);
    themeTitle->setFont(sectionFont);
    themeLayout->addWidget(themeTitle);

    auto *themeForm = new QFormLayout();
    QLabel *fontLabel = new QLabel(I18n::tr("Font"), this);
    fontCombo = new QComboBox(this);
    QStringList families = QFontDatabase::families();
    std::sort(families.begin(), families.end(), [](const QString &a, const QString &b) {
        return a.toLower() < b.toLower();
    });
    for (const auto &family : families) {
        fontCombo->addItem(family);
        QFont itemFont(family);
        fontCombo->setItemData(fontCombo->count() - 1, itemFont, Qt::FontRole);
    }
    if (!fontFamily.isEmpty()) {
        int fontIndex = fontCombo->findText(fontFamily, Qt::MatchFixedString);
        if (fontIndex >= 0) {
            fontCombo->setCurrentIndex(fontIndex);
        }
    }
    connect(fontCombo, &QComboBox::currentTextChanged, this, [](const QString &family) {
        if (family.trimmed().isEmpty()) {
            return;
        }
        QFont font = QApplication::font();
        font.setFamily(family);
        QApplication::setFont(font);
    });
    themeForm->addRow(fontLabel, fontCombo);
    themeLayout->addLayout(themeForm);

    animationsCheck = new QCheckBox(I18n::tr("Enable animations"), this);
    animationsCheck->setChecked(animationsEnabled);
    themeLayout->addWidget(animationsCheck);

    animateBackgroundCheck = new QCheckBox(I18n::tr("Animate background"), this);
    animateBackgroundCheck->setChecked(animateBackgroundEnabled);
    themeLayout->addWidget(animateBackgroundCheck);
    themeLayout->addStretch();

    QWidget *memoryPage = new QWidget(this);
    auto *memoryLayout = new QVBoxLayout(memoryPage);
    memoryLayout->setContentsMargins(0, 0, 0, 0);
    memoryLayout->setSpacing(10);

    autoCheck = new QCheckBox(I18n::tr("Auto memory (recommended)"), this);
    autoCheck->setChecked(autoMemory);
    connect(autoCheck, &QCheckBox::toggled, this, &SettingsDialog::applyAutoState);

    infoLabel = new QLabel(this);
    updateRecommended();
    infoLabel->setStyleSheet("color: #9fb0d6;");
    infoLabel->setWordWrap(true);

    warnLabel = new QLabel(this);
    warnLabel->setStyleSheet("color: #f7c65b;");
    warnLabel->setWordWrap(true);
    warnLabel->setVisible(false);

    QFormLayout *memForm = new QFormLayout();
    memForm->addRow(I18n::tr("Min memory (Xms, MB)"), xmsSpin);
    memForm->addRow(I18n::tr("Max memory (Xmx, MB)"), xmxSpin);

    memoryLayout->addWidget(autoCheck);
    memoryLayout->addWidget(infoLabel);
    memoryLayout->addWidget(warnLabel);
    memoryLayout->addLayout(memForm);
    memoryLayout->addStretch();

    QWidget *perfPage = new QWidget(this);
    auto *perfLayout = new QVBoxLayout(perfPage);
    perfLayout->setContentsMargins(0, 0, 0, 0);
    perfLayout->setSpacing(10);

    auto *gpuTipsButton = new QPushButton(I18n::tr("GPU tips"), this);
    gpuTipsButton->setObjectName("GpuTipsButton");
    connect(gpuTipsButton, &QPushButton::clicked, this, [this]() {
        showSystemNotification(I18n::tr("GPU tips"),
                               I18n::tr("Windows: Settings → System → Display → Graphics → add javaw.exe or LDL.exe → set High performance → Save."));
    });

    QLabel *limitLabel = new QLabel(I18n::tr("Download speed limit (KB/s, 0 = unlimited)"), this);
    downloadLimitSpin = new QSpinBox(this);
    downloadLimitSpin->setRange(0, 500000);
    downloadLimitSpin->setSingleStep(256);
    downloadLimitSpin->setValue(std::max(0, downloadLimitKbps));

    perfLayout->addWidget(gpuTipsButton);
    perfLayout->addSpacing(6);
    perfLayout->addWidget(limitLabel);
    perfLayout->addWidget(downloadLimitSpin);
    perfLayout->addStretch();

    stack->addWidget(generalPage);
    stack->addWidget(themePage);
    stack->addWidget(memoryPage);
    stack->addWidget(perfPage);

      connect(navList, &QListWidget::currentRowChanged, this, [this](int row) {
          if (row < 0 || row >= stack->count()) {
              return;
          }
          stack->setCurrentIndex(row);
      });

    QWidget *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(10);
    rightLayout->addWidget(stack, 1);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch();
    auto *cancel = new QPushButton(I18n::tr("Cancel"), this);
    cancel->setObjectName("SecondaryButton");
    auto *save = new QPushButton(I18n::tr("Save"), this);
    save->setObjectName("PrimaryButton");
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(save, &QPushButton::clicked, this, &QDialog::accept);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    rightLayout->addLayout(buttons);

    root->addWidget(navList);
    root->addWidget(rightPanel, 1);

    connect(this, &QDialog::rejected, this, [this]() {
        if (!initialFontFamily.isEmpty()) {
            QFont font = QApplication::font();
            font.setFamily(initialFontFamily);
            QApplication::setFont(font);
        }
    });

    setStyleSheet(R"QSS(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #151c29, stop:1 #1c2636);
            color: #e8ecf2;
            border: 1px solid #2f3b52;
            border-radius: 12px;
        }
        QListWidget {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1f2a3b, stop:1 #1a2333);
            border: 1px solid #2f3b52;
            border-radius: 10px;
            padding: 2px;
        }
        QListWidget::item {
            padding: 6px 8px;
            color: #c9d3ee;
            border-radius: 8px;
        }
        QListWidget::item:focus {
            outline: none;
        }
        QListWidget::item:selected {
            background-color: #273954;
            border: 1px solid #5aa9e6;
            color: #e8ecf2;
        }
        QListWidget::item:selected:active {
            background-color: #273954;
        }
        QListWidget::item:selected:!active {
            background-color: #273954;
        }
        QSpinBox, QComboBox, QLineEdit {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #222c3e, stop:1 #1d2737);
            border: 1px solid #2f3b52;
            border-radius: 8px;
            padding: 4px 24px 4px 8px;
            color: #e8ecf2;
        }
        QSpinBox::up-button, QSpinBox::down-button {
            subcontrol-origin: border;
            width: 18px;
            height: 12px;
            border-left: 1px solid #2f3b52;
            background-color: #202a3b;
        }
        QSpinBox::up-button {
            subcontrol-position: top right;
            border-top-right-radius: 8px;
            border-bottom: 1px solid #2f3a4f;
        }
        QSpinBox::down-button {
            subcontrol-position: bottom right;
            border-bottom-right-radius: 8px;
            border-top: 1px solid #2f3a4f;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #2a3447;
        }
        QSpinBox::up-arrow {
            image: url(":/icons/arrow_up.svg");
            width: 8px;
            height: 6px;
        }
        QSpinBox::down-arrow {
            image: url(":/icons/arrow_down.svg");
            width: 8px;
            height: 6px;
        }
        QComboBox::drop-down {
            subcontrol-origin: border;
            subcontrol-position: top right;
            width: 20px;
            border-left: 1px solid #2f3b52;
            background-color: #202a3b;
            border-top-right-radius: 8px;
            border-bottom-right-radius: 8px;
        }
        QComboBox::down-arrow {
            image: url(":/icons/chevron_down.svg");
            width: 8px;
            height: 6px;
        }
        QComboBox QAbstractItemView {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1b2231, stop:1 #151b28);
            color: #e8ecf2;
            border: 1px solid #5aa9e6;
            border-radius: 8px;
            padding: 4px;
            selection-background-color: #33405a;
            selection-color: #e8ecf2;
            outline: none;
        }
        QCheckBox {
            color: #c9d3ee;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 14px;
            height: 14px;
            border-radius: 4px;
        }
        QCheckBox::indicator:unchecked {
            background-color: #212a3a;
            border: 1px solid #2f3a4f;
        }
        QCheckBox::indicator:checked {
            background-color: #5aa9e6;
            border: 1px solid #5aa9e6;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2f3b52, stop:1 #263246);
            border: 1px solid #3a4763;
            border-radius: 10px;
            padding: 6px 14px;
            color: #e8ecf2;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #36455f, stop:1 #2b3a51);
        }
        QPushButton#PrimaryButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #2b6be6, stop:1 #234bb8);
            border-color: #3e7bf0;
            font-weight: 600;
        }
        QPushButton#PrimaryButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #3373ef, stop:1 #2751c2);
            border-color: #4b86ff;
        }
        QPushButton#PrimaryButton:pressed {
            background-color: #2344a0;
        }
        QPushButton#SecondaryButton {
            background: transparent;
            border-color: #3a4763;
            color: #9fb0d6;
        }
        QPushButton#SecondaryButton:hover {
            background: rgba(47, 59, 82, 120);
            color: #e8ecf2;
        }
    )QSS");

    syncLimits();
    applyAutoState(autoMemory);
    memoryTimer = new QTimer(this);
    memoryTimer->setInterval(1000);
    connect(memoryTimer, &QTimer::timeout, this, [this]() {
        updateRecommended();
        if (autoCheck && autoCheck->isChecked()) {
            const int availNow = availableRamMb();
            const int avail = availNow > 0 ? availNow : this->availableMb;
            const auto rec = recommendMemoryDynamic(this->totalMb, avail);
            xmsSpin->setValue(rec.xms);
            xmxSpin->setValue(rec.xmx);
        }
        updateMemoryWarning();
    });
    memoryTimer->start();
    QTimer::singleShot(0, this, [this]() { applyFadeIn(this, 200); });
}

int SettingsDialog::xmsMb() const
{
    return xmsSpin->value();
}

int SettingsDialog::xmxMb() const
{
    return xmxSpin->value();
}

bool SettingsDialog::autoMemoryEnabled() const
{
    return autoCheck ? autoCheck->isChecked() : false;
}

bool SettingsDialog::perfJvmProfileEnabled() const
{
    return perfJvmProfileValue;
}

QString SettingsDialog::extraJvmArgs() const
{
    return extraJvmArgsValue;
}

int SettingsDialog::downloadLimitKbps() const
{
    return downloadLimitSpin ? downloadLimitSpin->value() : 0;
}

bool SettingsDialog::musicEnabled() const
{
    return musicCheck ? musicCheck->isChecked() : true;
}

int SettingsDialog::musicVolume() const
{
    return musicSlider ? musicSlider->value() : 60;
}

bool SettingsDialog::animationsEnabled() const
{
    return animationsCheck ? animationsCheck->isChecked() : true;
}

bool SettingsDialog::animateBackgroundEnabled() const
{
    return animateBackgroundCheck ? animateBackgroundCheck->isChecked() : true;
}

QString SettingsDialog::selectedLanguage() const
{
    return langCombo ? langCombo->currentData().toString() : langCode;
}

QString SettingsDialog::selectedFontFamily() const
{
    return fontCombo ? fontCombo->currentText().trimmed() : QString();
}

void SettingsDialog::setMusicPreviewCallback(const std::function<void(bool, int)> &callback)
{
    musicPreviewCallback = callback;
}

void SettingsDialog::updateRecommended()
{
    const bool useDynamic = autoCheck ? autoCheck->isChecked() : false;
    const int availNow = availableRamMb();
    const int avail = availNow > 0 ? availNow : availableMb;
    const auto rec = useDynamic ? recommendMemoryDynamic(totalMb, avail) : recommendMemory(totalMb);
    infoLabel->setText(I18n::tr("Total RAM: %1 MB\nAvailable: %2 MB\nRecommended: Xms %3 MB, Xmx %4 MB")
        .arg(totalMb).arg(avail).arg(rec.xms).arg(rec.xmx));
}

void SettingsDialog::applyAutoState(bool enabled)
{
    if (enabled) {
        const auto rec = recommendMemoryDynamic(totalMb, availableMb);
        xmsSpin->setValue(rec.xms);
        xmxSpin->setValue(rec.xmx);
    }
    xmsSpin->setEnabled(!enabled);
    xmxSpin->setEnabled(!enabled);
    updateRecommended();
    updateMemoryWarning();
}

void SettingsDialog::syncLimits()
{
    int xms = xmsSpin->value();
    int xmx = xmxSpin->value();
    if (xms > xmx) {
        xmxSpin->setValue(xms);
    } else if (xmx < xms) {
        xmsSpin->setValue(xmx);
    }
    updateMemoryWarning();
}

void SettingsDialog::updateMemoryWarning()
{
    if (!warnLabel) {
        return;
    }
    const int maxMb = totalMb > 0 ? totalMb : 1;
    const int threshold = static_cast<int>(maxMb * 0.8);
    const int current = xmxSpin ? xmxSpin->value() : 0;
    if (current > threshold) {
        warnLabel->setText(I18n::tr("Warning: Xmx exceeds 80% of RAM. The system may stutter."));
        warnLabel->setVisible(true);
    } else {
        warnLabel->setVisible(false);
    }
}
