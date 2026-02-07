#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QListView>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QHash>
#include <QSet>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <vector>

#include "../core/build_info.h"

class QNetworkAccessManager;
class QAction;
class QProcess;
class QMediaPlayer;
class QLineEdit;
class QComboBox;
class BuildListModel;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QAudioOutput;
#endif
class AnimatedBackgroundWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const QString &userEmail, const QString &apiBaseUrl, const QString &accessToken, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onBuildSelected(int index);
    void onBuildContextMenu(const QPoint &pos);
    void onAddBuildClicked();
    void onDownloadClicked();
    void onPlayClicked();
    void onRemoveClicked();
    void onRemoveFromListClicked();
    void onSettingsClicked();
    void onPlayConsoleClicked();
    void onStopClicked();
    void onVerifyClicked();
    void onImportBuildClicked();
    void onAdminClicked();

private:
    struct LaunchInfo {
        QString javaExe;
        QStringList args;
        QString workingDir;
    };
    bool prepareLaunch(LaunchInfo &info, QString *error);
    void setupUI();
    void loadBuilds();
    void refreshBuildList();
    void updateDetails(int index);
    void applyTranslations();
    void syncBuildsFromServer();
    void loadMemorySettings();
    void saveMemorySettings(int xmsMb, int xmxMb, bool autoMemory);
    void loadPerformanceSettings();
    void savePerformanceSettings(bool perfProfile, const QString &extraArgs);
    void loadDownloadSettings();
    void saveDownloadSettings(int limitKbps);
    void loadAudioSettings();
    void saveAudioSettings(bool enabled, int volume);
    void loadUiSettings();
    void saveUiSettings(bool animationsEnabled, bool animateBackground, const QString &fontFamily);
    void applyUiFont(const QString &fontFamily);
    void updateBackgroundAnimation();
    void applyAudioSettings();
    void updateMusicIcon();
    void updateMemoryLabel();
    int totalRamMb() const;
    int availableRamMb() const;
    bool isAdminUser() const;
    bool isBuildLocked(const BuildInfo &build) const;
    int buildIndexFromList(int listIndex) const;
    int currentBuildIndex() const;
    void updateHeaderFonts();

    QListView *buildList;
    BuildListModel *buildListModel = nullptr;
    QPushButton *addBuildButton;
    QLineEdit *searchEdit = nullptr;
    QComboBox *tagFilter = nullptr;
    QToolButton *musicButton;
    QToolButton *accountButton;
    QFrame *topBarFrame = nullptr;
    QFrame *cardsAreaFrame = nullptr;
    QFrame *sidePanelFrame = nullptr;
    QFrame *topActionsFrame = nullptr;
    QLabel *accountHeaderLabel;
    QLabel *accountEmailLabel;
    QLabel *titleLabel;
    QLabel *logoLabel;
    QLabel *cardsTitleLabel;
    QLabel *actionsTitleLabel;
    QAction *actionSettings;
    QAction *actionSupport;
    QAction *actionLogout;
    QAction *actionAdmin = nullptr;
    QLabel *detailsLabel;
    QLabel *memoryLabel;
    QNetworkAccessManager *imageManager;
    QPushButton *downloadButton;
    QPushButton *playButton;
    QPushButton *playConsoleButton;
    QPushButton *stopButton;
    QPushButton *verifyButton;
    QPushButton *removeButton;
    QPushButton *removeListButton;
    QProcess *gameProcess = nullptr;
    QMediaPlayer *musicPlayer = nullptr;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QAudioOutput *musicOutput = nullptr;
#endif
    AnimatedBackgroundWidget *backgroundWidget = nullptr;
    QTimer *backgroundTimer = nullptr;
    qreal backgroundPhase = 0.0;

    QString userEmail;
    QString apiBaseUrl;
    QString accessToken;
    std::vector<BuildInfo> builds;
    int memoryXmsMb = 0;
    int memoryXmxMb = 0;
    bool memoryAuto = true;
    bool perfJvmProfile = false;
    QString extraJvmArgs;
    int downloadLimitKbps = 0;
    bool didAnimate = false;
    bool musicEnabled = true;
    int musicVolume = 60;
    bool animationsEnabled = true;
    bool animateBackground = true;
    QHash<QString, QPixmap> buildImageCache;
    QSet<QString> pendingImageLoads;
};

#endif // MAIN_WINDOW_H
