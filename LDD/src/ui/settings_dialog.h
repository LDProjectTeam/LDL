#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <QDialog>
#include <QString>
#include <QPointer>
#include <functional>

class QSpinBox;
class QLabel;
class QListWidget;
class QStackedWidget;
class QComboBox;
class QCheckBox;
class QSlider;
class QTimer;
class QGraphicsOpacityEffect;
class QPropertyAnimation;
class QComboBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(int totalMb,
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
                   QWidget *parent = nullptr);

    int xmsMb() const;
    int xmxMb() const;
    bool autoMemoryEnabled() const;
    bool perfJvmProfileEnabled() const;
    QString extraJvmArgs() const;
    int downloadLimitKbps() const;
    bool musicEnabled() const;
    int musicVolume() const;
    bool animationsEnabled() const;
    bool animateBackgroundEnabled() const;
    QString selectedLanguage() const;
    QString selectedFontFamily() const;
    void setMusicPreviewCallback(const std::function<void(bool, int)> &callback);

private:
    void syncLimits();
    void updateRecommended();
    void applyAutoState(bool enabled);
    void updateMemoryWarning();

    int totalMb;
    int availableMb;
    QSpinBox *xmsSpin;
    QSpinBox *xmxSpin;
    QLabel *infoLabel;
    QLabel *warnLabel;
    QCheckBox *autoCheck;
    bool perfJvmProfileValue = false;
    QString extraJvmArgsValue;
    QSpinBox *downloadLimitSpin;
    QListWidget *navList;
    QStackedWidget *stack;
    QComboBox *langCombo;
    QCheckBox *musicCheck;
    QSlider *musicSlider;
    QCheckBox *animationsCheck = nullptr;
    QCheckBox *animateBackgroundCheck = nullptr;
    QComboBox *fontCombo = nullptr;
    QString langCode;
    QString initialFontFamily;
    std::function<void(bool, int)> musicPreviewCallback;
    QTimer *memoryTimer;
    QPointer<QWidget> tabFadeTarget;
    QPointer<QGraphicsOpacityEffect> tabFadeEffect;
    QPointer<QPropertyAnimation> tabFadeAnim;
};

#endif // SETTINGS_DIALOG_H
