#include "main_window.h"

#include "catalog_dialog.h"
#include "register_dialog.h"
#include "i18n.h"
#include "settings_dialog.h"
#include "notification.h"
#include "../core/config_manager.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFrame>
#include <QFont>
#include <QHBoxLayout>
#include <QAbstractListModel>
#include <QMessageBox>
#include <QMenu>
#include <QProgressBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QToolButton>
#include <QDesktopServices>
#include <QStyle>
#include <QPainter>
#include <QPixmap>
#include <QColor>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSpacerItem>
#include <QSize>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QSettings>
#include <QCoreApplication>
#include <QHash>
#include <QDir>
#include <QMetaObject>
#include <QUrl>
#include <QProcess>
#include <QDialog>
#include <QLabel>
#include <QFileInfo>
#include <QFile>
#include <QDirIterator>
#include <QRegularExpression>
#include <QEvent>
#include <QPainterPath>
#include <QRegion>
#include <QTimer>
#include <QWidgetAction>
#include <QUrl>
#include <QMouseEvent>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTextEdit>
#include <QUuid>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QThread>
#include <QMediaPlayer>
#include <QItemSelectionModel>
#include <QStyledItemDelegate>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QAudioOutput>
#endif
#include <QFileDialog>
#include <QDateTime>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <functional>
#include <cmath>

#include <atomic>
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>

#include "../core/downloader.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <wininet.h>
#endif
class AnimatedBackgroundWidget : public QWidget
{
public:
    explicit AnimatedBackgroundWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAutoFillBackground(false);
    }

    void setAnimated(bool enabled)
    {
        animated = enabled;
        update();
    }

    void setPhase(qreal newPhase)
    {
        phase = newPhase;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const qreal cx = animated ? 0.22 + 0.2 * std::sin(phase * 0.8) : 0.22;
        const qreal cy = animated ? 0.08 + 0.18 * std::cos(phase * 0.7) : 0.08;
        const qreal radius = std::max(width(), height()) * 1.2;
        const qreal pulse = 0.5 + 0.5 * std::sin(phase * 0.4);
        auto tint = [pulse](const QColor &base) {
            const int delta = static_cast<int>(12 + pulse * 20);
            QColor out = base;
            out = out.lighter(100 + delta);
            return out;
        };
        QRadialGradient gradient(width() * cx, height() * cy, radius);
        gradient.setColorAt(0.0, tint(QColor("#1a2334")));
        gradient.setColorAt(0.55, tint(QColor("#141b28")));
        gradient.setColorAt(1.0, QColor("#111722"));
        painter.fillRect(rect(), gradient);

        if (animated) {
            const qreal blobRadius = radius * 0.45;
            QPointF c1(width() * (0.25 + 0.18 * std::sin(phase * 0.6)),
                       height() * (0.25 + 0.14 * std::cos(phase * 0.8)));
            QPointF c2(width() * (0.8 + 0.12 * std::cos(phase * 0.5)),
                       height() * (0.6 + 0.12 * std::sin(phase * 0.7)));

            QRadialGradient blob1(c1, blobRadius);
            blob1.setColorAt(0.0, QColor(90, 169, 230, 150));
            blob1.setColorAt(1.0, QColor(90, 169, 230, 0));

            QRadialGradient blob2(c2, blobRadius * 0.85);
            blob2.setColorAt(0.0, QColor(58, 100, 170, 120));
            blob2.setColorAt(1.0, QColor(58, 100, 170, 0));

            painter.setCompositionMode(QPainter::CompositionMode_Screen);
            painter.fillRect(rect(), blob1);
            painter.fillRect(rect(), blob2);
        }
    }

private:
    bool animated = true;
    qreal phase = 0.0;
};

namespace {

enum class TaskKind {
    Download,
    Extract,
    Write
};

struct Task {
    TaskKind kind;
    QString label;
    QString url;
    QString path;
    QString authHeader;
    QByteArray data;
    QString extractTo;
    qint64 expectedSize = -1;
    bool canSkip = true;
};

struct VerifyItem {
    QString path;
    QString sha1;
    qint64 size = -1;
    QString url;
};

struct ManifestEntry {
    QString relPath;
    QString sha1;
    qint64 size = -1;
};

struct MemoryRecommendation {
    int xms;
    int xmx;
};

void applyFadeIn(QWidget *widget, int durationMs = 200)
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

void applyPanelFade(QWidget *widget, int delayMs, int durationMs = 220)
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
    auto startAnim = [anim]() {
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    };
    if (delayMs > 0) {
        QTimer::singleShot(delayMs, widget, startAnim);
    } else {
        startAnim();
    }
    QObject::connect(anim, &QPropertyAnimation::finished, widget, [widget, effect]() {
        if (widget->graphicsEffect() == effect) {
            widget->setGraphicsEffect(nullptr);
        }
        effect->deleteLater();
    });
}

void animateActionVisibility(QWidget *widget, bool visible)
{
    if (!widget) {
        return;
    }
    widget->setVisible(visible);
    widget->setMaximumHeight(QWIDGETSIZE_MAX);
}

bool ensureDirForFile(const QString &filePath);
QString extractDetailMessage(const QByteArray &body);
bool readBuildMeta(const QString &root, QJsonObject &out);
bool computeSha1(const QString &path, QString &out);

constexpr long long kMb = 1024 * 1024;

int bytesToMb(long long bytes)
{
    if (bytes <= 0) {
        return 0;
    }
    return static_cast<int>((bytes + kMb - 1) / kMb);
}

long long taskWeightBytes(const Task &task)
{
    if (task.expectedSize > 0) {
        return task.expectedSize;
    }
    if (task.kind == TaskKind::Write && !task.data.isEmpty()) {
        return task.data.size();
    }
    if (task.kind == TaskKind::Extract && !task.path.isEmpty()) {
        QFileInfo info(task.path);
        if (info.exists()) {
            return info.size();
        }
    }
    if (task.kind == TaskKind::Download) {
        return 0;
    }
    return 5 * kMb;
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

QString normalizeSha256String(const QString &value)
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

QString buildSizeText(const BuildInfo &build)
{
    if (build.sizeBytes > 0) {
        return QString::number(build.sizeBytes / 1000000);
    }
    return I18n::tr("Unknown");
}

QString priceTextForBuild(const BuildInfo &build, bool lockedForUser)
{
    if (build.isFree) {
        return I18n::tr("Free");
    }
    double price = static_cast<double>(build.priceCents) / 100.0;
    QString text = I18n::tr("Paid $%1").arg(price, 0, 'f', 2);
    if (lockedForUser) {
        text += " (" + I18n::tr("Locked") + ")";
    }
    return text;
}

QStringList splitJvmArgs(const QString &input)
{
    QStringList out;
    QString current;
    bool inQuote = false;
    for (int i = 0; i < input.size(); ++i) {
        const QChar ch = input.at(i);
        if (ch == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (ch.isSpace() && !inQuote) {
            if (!current.isEmpty()) {
                out << current;
                current.clear();
            }
            continue;
        }
        current.append(ch);
    }
    if (!current.isEmpty()) {
        out << current;
    }
    return out;
}

} // namespace

struct BuildListItem {
    BuildInfo build;
    int buildIndex = -1;
    bool locked = false;
    QPixmap image;
};

class BuildListModel : public QAbstractListModel
{
public:
    explicit BuildListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    enum Roles {
        BuildIndexRole = Qt::UserRole + 1,
        LockedRole,
        ImageRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid()) {
            return 0;
        }
        return items.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= items.size()) {
            return {};
        }
        const auto &item = items.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            return QString::fromUtf8(item.build.name.c_str());
        case BuildIndexRole:
            return item.buildIndex;
        case LockedRole:
            return item.locked;
        case ImageRole:
            return item.image;
        default:
            return {};
        }
    }

    void setItems(QVector<BuildListItem> nextItems)
    {
        beginResetModel();
        items = nextItems;
        endResetModel();
    }

    const BuildListItem *itemAt(int row) const
    {
        if (row < 0 || row >= items.size()) {
            return nullptr;
        }
        return &items[row];
    }

    int buildIndexAtRow(int row) const
    {
        const auto *item = itemAt(row);
        return item ? item->buildIndex : -1;
    }

    int rowForBuildId(const QString &id) const
    {
        for (int i = 0; i < items.size(); ++i) {
            if (QString::fromStdString(items[i].build.id) == id) {
                return i;
            }
        }
        return -1;
    }

    void setImageForRow(int row, const QPixmap &pix)
    {
        if (row < 0 || row >= items.size()) {
            return;
        }
        items[row].image = pix;
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, {ImageRole});
    }

private:
    QVector<BuildListItem> items;
};

namespace {

class BuildListDelegate : public QStyledItemDelegate
{
public:
    explicit BuildListDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return {480, 132};
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        const auto *model = static_cast<const BuildListModel *>(index.model());
        const BuildListItem *item = model ? model->itemAt(index.row()) : nullptr;
        if (!item) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QRect cardRect = option.rect.adjusted(6, 6, -6, -6);
        QColor border("#32435f");
        QColor bg(26, 34, 49, 200);
        if (option.state & QStyle::State_Selected) {
            border = QColor("#5aa9e6");
            bg = QColor(31, 40, 58, 215);
        } else if (option.state & QStyle::State_MouseOver) {
            border = QColor("#6bb7f0");
            bg = QColor(33, 44, 64, 210);
        }

        painter->setBrush(bg);
        painter->setPen(QPen(border, 1));
        painter->drawRoundedRect(cardRect, 12, 12);

        const QRect thumbRect(cardRect.left() + 12, cardRect.top() + 12, 112, 86);
        const QRect imageRect = thumbRect.adjusted(4, 4, -4, -4);

        painter->setBrush(QColor("#1a2332"));
        painter->setPen(QPen(QColor("#314058"), 1));
        painter->drawRoundedRect(thumbRect, 12, 12);

        painter->setBrush(QColor("#0f141e"));
        painter->setPen(QPen(QColor("#2a364b"), 1));
        painter->drawRoundedRect(imageRect, 10, 10);

        if (!item->image.isNull()) {
            QPixmap scaled = item->image.scaled(imageRect.size(),
                                                Qt::KeepAspectRatioByExpanding,
                                                Qt::SmoothTransformation);
            QPainterPath clipPath;
            clipPath.addRoundedRect(imageRect, 10, 10);
            painter->setClipPath(clipPath);
            painter->drawPixmap(imageRect.topLeft(), scaled);
            painter->setClipping(false);
        } else {
            painter->setPen(QColor("#7d88a6"));
            painter->drawText(imageRect, Qt::AlignCenter, I18n::tr("No image"));
        }

        const int textLeft = thumbRect.right() + 12;
        const int textRight = cardRect.right() - 12;
        QRect textRect(textLeft, cardRect.top() + 12, textRight - textLeft, cardRect.height() - 24);

        QFont nameFont = option.font;
        nameFont.setPointSize(11);
        nameFont.setBold(true);
        QFont metaFont = option.font;
        metaFont.setPointSize(9);
        QFont tagFont = option.font;
        tagFont.setPointSize(8);

        const QString name = QString::fromUtf8(item->build.name.c_str());
        const QString loaderText = item->build.loader.empty()
            ? I18n::tr("Loader: ?")
            : I18n::tr("Loader: %1").arg(QString::fromStdString(item->build.loader));
        const QString meta = I18n::tr("%1 MB | Minecraft %2 | %3 | Java %4")
            .arg(buildSizeText(item->build))
            .arg(QString::fromStdString(item->build.minecraftVersion))
            .arg(loaderText)
            .arg(item->build.javaVersion);
        const QString price = priceTextForBuild(item->build, item->locked);

        int y = textRect.top();
        painter->setFont(nameFont);
        painter->setPen(QColor("#e8ecf2"));
        const QFontMetrics nameFm(nameFont);
        painter->drawText(QRect(textRect.left(), y, textRect.width(), nameFm.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, name);
        y += nameFm.height() + 4;

        painter->setFont(metaFont);
        painter->setPen(QColor("#9fb0d6"));
        const QFontMetrics metaFm(metaFont);
        painter->drawText(QRect(textRect.left(), y, textRect.width(), metaFm.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, meta);
        y += metaFm.height() + 4;

        painter->setPen(QColor("#dce6ff"));
        painter->drawText(QRect(textRect.left(), y, textRect.width(), metaFm.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, price);
        y += metaFm.height() + 6;

        painter->setFont(tagFont);
        QFontMetrics tagFm(tagFont);
        const QString tagsCsv = QString::fromStdString(item->build.tagsCsv);
        QStringList tags = tagsCsv.split(',', Qt::SkipEmptyParts);
        if (tags.isEmpty()) {
            painter->setPen(QColor("#7d88a6"));
            painter->drawText(QRect(textRect.left(), y, textRect.width(), tagFm.height()),
                              Qt::AlignLeft | Qt::AlignVCenter, I18n::tr("No tags"));
        } else {
            int x = textRect.left();
            const int maxX = textRect.right();
            for (const auto &tagRaw : tags) {
                const QString tag = tagRaw.trimmed();
                if (tag.isEmpty()) {
                    continue;
                }
                const int textW = tagFm.horizontalAdvance(tag);
                const int pillW = textW + 16;
                const int pillH = tagFm.height() + 6;
                if (x + pillW > maxX) {
                    const QString dots = "...";
                    const int dotsW = tagFm.horizontalAdvance(dots) + 16;
                    if (x + dotsW <= maxX) {
                        QRect pillRect(x, y, dotsW, pillH);
                        painter->setBrush(QColor("#283347"));
                        painter->setPen(QPen(QColor("#34445f"), 1));
                        painter->drawRoundedRect(pillRect, pillH / 2, pillH / 2);
                        painter->setPen(QColor("#d3def7"));
                        painter->drawText(pillRect, Qt::AlignCenter, dots);
                    }
                    break;
                }
                QRect pillRect(x, y, pillW, pillH);
                painter->setBrush(QColor("#283347"));
                painter->setPen(QPen(QColor("#34445f"), 1));
                painter->drawRoundedRect(pillRect, pillH / 2, pillH / 2);
                painter->setPen(QColor("#d3def7"));
                painter->drawText(pillRect, Qt::AlignCenter, tag);
                x += pillW + 6;
            }
        }

        painter->restore();
    }
};

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

QIcon solidPlusIcon(const QColor &color = QColor("#d9e3ff"))
{
    const int size = 14;
    const int stroke = 2;
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(Qt::NoPen);
    p.setBrush(color);
    const int mid = size / 2;
    const int half = stroke / 2;
    p.drawRect(mid - half, 1, stroke, size - 2);
    p.drawRect(1, mid - half, size - 2, stroke);
    return QIcon(pix);
}

QIcon glyphIcon(const QString &glyphs, const QColor &color = QColor("#c9d3ee"))
{
    if (glyphs.isEmpty()) {
        return {};
    }
    return glyphIcon(glyphs.at(0), color);
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

class RoundedComboBox : public QComboBox
{
public:
    explicit RoundedComboBox(QWidget *parent = nullptr)
        : QComboBox(parent)
    {
    }

    void setPopupStyle(const QString &name, const QString &styleSheet, int radius)
    {
        popupName = name;
        popupStyle = styleSheet;
        popupRadius = radius;
    }

protected:
    void showPopup() override
    {
        QComboBox::showPopup();
        QWidget *popup = view() ? view()->window() : nullptr;
        if (!popup) {
            return;
        }
        if (popup != popupWidget) {
            popupWidget = popup;
            popupWidget->setAttribute(Qt::WA_StyledBackground, true);
            if (!popupName.isEmpty()) {
                popupWidget->setObjectName(popupName);
            }
            if (!popupStyle.isEmpty()) {
                popupWidget->setStyleSheet(popupStyle);
            }
            if (popupRadius > 0) {
                popupWidget->installEventFilter(new PopupRounder(popupRadius, popupWidget));
            }
        }
    }

private:
    QString popupName;
    QString popupStyle;
    int popupRadius = 0;
    QPointer<QWidget> popupWidget;
};

bool downloadBytesWinInet(const QString &url, const QString &extraHeaders, QByteArray &out, QString *error)
{
#ifdef Q_OS_WIN
    auto fallback = [&](QString *errOut) -> bool {
        QNetworkAccessManager manager;
        QNetworkRequest request{QUrl(url)};
        const QStringList headerLines = extraHeaders.split("\r\n", Qt::SkipEmptyParts);
        for (const auto &line : headerLines) {
            const int idx = line.indexOf(':');
            if (idx > 0) {
                const QByteArray key = line.left(idx).trimmed().toUtf8();
                const QByteArray value = line.mid(idx + 1).trimmed().toUtf8();
                if (!key.isEmpty()) {
                    request.setRawHeader(key, value);
                }
            }
        }
        QNetworkReply *reply = manager.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        out = reply->readAll();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            if (errOut) {
                if (status >= 400) {
                    *errOut = I18n::tr("HTTP error %1: %2").arg(status).arg(url);
                } else if (isOfflineNetworkError(reply->error())) {
                    *errOut = I18n::tr("No internet connection. Please check your network.");
                } else {
                    *errOut = I18n::tr("Network error: %1").arg(reply->errorString());
                }
            }
            reply->deleteLater();
            return false;
        }
        reply->deleteLater();
        return true;
    };

    HINTERNET hInternetSession = InternetOpenA("MinecraftLauncher/1.0",
                                               INTERNET_OPEN_TYPE_PRECONFIG,
                                               NULL, NULL, 0);
    if (!hInternetSession) {
        if (error) {
            *error = I18n::tr("InternetOpen failed");
        }
        return fallback(error);
    }

    std::string headers = extraHeaders.toStdString();
    DWORD headerLen = headers.empty() ? 0 : static_cast<DWORD>(headers.size());
    DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
    if (url.startsWith("https://")) {
        flags |= INTERNET_FLAG_SECURE;
    }

    HINTERNET hHttpFile = InternetOpenUrlA(hInternetSession, url.toStdString().c_str(),
                                           headers.empty() ? nullptr : headers.c_str(),
                                           headerLen, flags, 0);
    if (!hHttpFile) {
        InternetCloseHandle(hInternetSession);
        if (error) {
            *error = I18n::tr("InternetOpenUrl failed: %1").arg(url);
        }
        return fallback(error);
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (HttpQueryInfoA(hHttpFile,
                       HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                       &statusCode, &statusCodeSize, NULL)) {
        if (statusCode != 200 && statusCode != 206) {
            InternetCloseHandle(hHttpFile);
            InternetCloseHandle(hInternetSession);
            if (error) {
                *error = I18n::tr("HTTP error %1: %2").arg(statusCode).arg(url);
            }
            return fallback(error);
        }
    }

    out.clear();
    char buffer[8192];
    DWORD bytesRead = 0;
    while (true) {
        if (!InternetReadFile(hHttpFile, buffer, sizeof(buffer), &bytesRead)) {
            InternetCloseHandle(hHttpFile);
            InternetCloseHandle(hInternetSession);
            if (error) {
                *error = I18n::tr("InternetReadFile failed");
            }
            return fallback(error);
        }
        if (bytesRead == 0) {
            break;
        }
        out.append(buffer, static_cast<int>(bytesRead));
    }

    InternetCloseHandle(hHttpFile);
    InternetCloseHandle(hInternetSession);
    return true;
#else
    Q_UNUSED(url);
    Q_UNUSED(extraHeaders);
    Q_UNUSED(out);
    if (error) {
        *error = I18n::tr("WinInet not available");
    }
    return false;
#endif
}

bool ensureDirForFile(const QString &filePath)
{
    QFileInfo info(filePath);
    QDir dir(info.absolutePath());
    if (dir.exists()) {
        return true;
    }
    return dir.mkpath(".");
}

bool copyDirRecursive(const QString &srcDir, const QString &dstDir, QString *error)
{
    QDir src(srcDir);
    if (!src.exists()) {
        if (error) {
            *error = I18n::tr("Folder not found: %1").arg(srcDir);
        }
        return false;
    }
    QDirIterator it(srcDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString srcPath = it.next();
        const QString relPath = src.relativeFilePath(srcPath);
        const QString dstPath = QDir(dstDir).filePath(relPath);
        if (!ensureDirForFile(dstPath)) {
            if (error) {
                *error = I18n::tr("Failed to create directory for %1").arg(dstPath);
            }
            return false;
        }
        QFile::remove(dstPath);
        if (!QFile::copy(srcPath, dstPath)) {
            if (error) {
                *error = I18n::tr("Failed to write file: %1").arg(dstPath);
            }
            return false;
        }
    }
    return true;
}

bool writeFile(const QString &path, const QByteArray &data, QString *error)
{
    if (!ensureDirForFile(path)) {
        if (error) {
            *error = I18n::tr("Failed to create directory for %1").arg(path);
        }
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = I18n::tr("Failed to write file: %1").arg(path);
        }
        return false;
    }
    if (file.write(data) != data.size()) {
        if (error) {
            *error = I18n::tr("Failed to write file: %1").arg(path);
        }
        file.close();
        return false;
    }
    file.close();
    return true;
}

QByteArray loadResourceBytes(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = I18n::tr("Core module not found.");
        }
        return {};
    }
    return file.readAll();
}

bool writeAuthFile(const QString &minecraftDir,
                   const QString &buildId,
                   const QString &apiBaseUrl,
                   const QString &deviceId,
                   const QString &launchToken,
                   QString *error)
{
    if (launchToken.trimmed().isEmpty()) {
        if (error) {
            *error = I18n::tr("Missing launch token.");
        }
        return false;
    }
    if (deviceId.trimmed().isEmpty()) {
        if (error) {
            *error = I18n::tr("Missing device id.");
        }
        return false;
    }
    const QString authPath = QDir(minecraftDir).filePath("ldp_auth.txt");
    QByteArray content;
    content += "launchToken=" + launchToken.toUtf8() + "\n";
    content += "buildId=" + buildId.toUtf8() + "\n";
    content += "api=" + apiBaseUrl.toUtf8() + "\n";
    content += "deviceId=" + deviceId.toUtf8() + "\n";
    return writeFile(authPath, content, error);
}

bool writeUserCache(const QString &minecraftDir,
                    const QString &uuid,
                    const QString &username,
                    QString *error)
{
    if (uuid.trimmed().isEmpty() || username.trimmed().isEmpty()) {
        return true;
    }
    QJsonArray entries;
    QJsonObject entry;
    entry.insert("name", username);
    entry.insert("uuid", uuid);
    entry.insert("expiresOn", QDateTime::currentDateTimeUtc().addYears(10).toString(Qt::ISODate));
    entries.append(entry);
    QJsonDocument doc(entries);
    const QString cachePath = QDir(minecraftDir).filePath("usercache.json");
    return writeFile(cachePath, doc.toJson(QJsonDocument::Compact), error);
}

bool ensureAuthMod(const QString &minecraftDir, QString *error)
{
    const QString modPath = QDir(minecraftDir).filePath("mods/core-utils.jar");
    QByteArray data = loadResourceBytes(":/mods/core-utils.jar", error);
    if (data.isEmpty()) {
        return false;
    }
    return writeFile(modPath, data, error);
}

int clampInt(int value, int minValue, int maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

void setNestedInt(QJsonObject &obj,
                  const QStringList &keys,
                  int index,
                  int value)
{
    if (index >= keys.size()) {
        return;
    }
    const QString key = keys.at(index);
    if (index == keys.size() - 1) {
        obj.insert(key, value);
        return;
    }
    QJsonObject child = obj.value(key).toObject();
    setNestedInt(child, keys, index + 1, value);
    obj.insert(key, child);
}

bool updateJsonIntValue(const QString &path,
                        const QStringList &keys,
                        int value,
                        QString *error)
{
    QFile file(path);
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = I18n::tr("Failed to open file: %1").arg(path);
        }
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();
    QJsonParseError parseError{};
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) {
            *error = I18n::tr("Invalid JSON: %1").arg(path);
        }
        return false;
    }

    QJsonObject root = doc.object();
    setNestedInt(root, keys, 0, value);

    QJsonDocument outDoc(root);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = I18n::tr("Failed to write file: %1").arg(path);
        }
        return false;
    }
    file.write(outDoc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void applyCpuTuning(const QString &minecraftDir)
{
    int cpu = QThread::idealThreadCount();
    if (cpu <= 0) {
        cpu = 4;
    }
    int sodiumThreads = clampInt(cpu / 2, 1, 6);
    int smoothbootMain = clampInt(cpu / 2, 2, 6);

    const QString configDir = QDir(minecraftDir).filePath("config");
    QString err;
    updateJsonIntValue(QDir(configDir).filePath("sodium-options.json"),
                       {"performance", "chunk_builder_threads"},
                       sodiumThreads, &err);
    updateJsonIntValue(QDir(configDir).filePath("smoothboot.json"),
                       {"threadCount", "main"},
                       smoothbootMain, &err);
}

QString fileUrlForTemplate(const QString &absTemplatePath)
{
    QString path = QDir::fromNativeSeparators(absTemplatePath);
    if (path.size() >= 2 && path.at(1) == ':') {
        return "file:///" + path;
    }
    return "file://" + path;
}

bool writeResourceFile(const QString &resourcePath, const QString &targetPath, QString *error)
{
    QFile res(resourcePath);
    if (!res.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = I18n::tr("Failed to read resource: %1").arg(resourcePath);
        }
        return false;
    }
    const QByteArray data = res.readAll();
    res.close();
    return writeFile(targetPath, data, error);
}

bool dirHasEntries(const QString &path)
{
    QDir dir(path);
    if (!dir.exists()) {
        return false;
    }
    return !dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty();
}

bool shouldSkipDownload(const Task &task)
{
    if (!task.canSkip || task.path.isEmpty()) {
        return false;
    }
    QFileInfo info(task.path);
    if (!info.exists()) {
        return false;
    }
    if (task.expectedSize > 0) {
        return info.size() == task.expectedSize;
    }
    return info.size() > 0;
}

bool shouldSkipWrite(const Task &task)
{
    if (!task.canSkip || task.path.isEmpty()) {
        return false;
    }
    QFileInfo info(task.path);
    return info.exists() && info.size() > 0;
}

bool shouldSkipExtract(const Task &task)
{
    if (!task.canSkip || task.extractTo.isEmpty()) {
        return false;
    }
    return dirHasEntries(task.extractTo);
}

QString resolveImageUrl(const QString &apiBaseUrl, const BuildInfo &build)
{
    QString img = QString::fromStdString(build.imageUrl);
    if (img.isEmpty()) {
        return {};
    }
    if (img.startsWith('/')) {
        return apiBaseUrl + img;
    }
    if (img.contains("github.com") || img.contains("raw.githubusercontent.com")) {
        return apiBaseUrl + "/builds/" + QString::fromStdString(build.id) + "/image";
    }
    return img;
}

QString safeFolderName(const QString &name)
{
    QString out = name.trimmed();
    if (out.isEmpty()) {
        return "build";
    }
    static const QString bad = "\\/:*?\"<>|";
    for (QChar &ch : out) {
        if (bad.contains(ch)) {
            ch = '_';
        }
    }
    return out;
}

QString formatEtaText(qint64 seconds)
{
    if (seconds < 0) {
        return I18n::tr("ETA: --");
    }
    int h = static_cast<int>(seconds / 3600);
    int m = static_cast<int>((seconds % 3600) / 60);
    int s = static_cast<int>(seconds % 60);
    QString time;
    if (h > 0) {
        time = QString("%1:%2:%3")
                   .arg(h, 2, 10, QChar('0'))
                   .arg(m, 2, 10, QChar('0'))
                   .arg(s, 2, 10, QChar('0'));
    } else {
        time = QString("%1:%2")
                   .arg(m, 2, 10, QChar('0'))
                   .arg(s, 2, 10, QChar('0'));
    }
    return I18n::tr("ETA: %1").arg(time);
}

class RoundedProgressBar : public QWidget
{
public:
    explicit RoundedProgressBar(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(10);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        animTimer = new QTimer(this);
        animTimer->setInterval(30);
        QObject::connect(animTimer, &QTimer::timeout, this, [this]() {
            animOffset = (animOffset + 4) % 200;
            update();
        });
    }

    void setShowText(bool enabled)
    {
        showText = enabled;
        update();
    }

    void setRange(int minValue, int maxValue)
    {
        min = minValue;
        max = maxValue;
        if (max <= min) {
            indeterminate = true;
            if (!animTimer->isActive()) {
                animTimer->start();
            }
        } else {
            indeterminate = false;
            if (animTimer->isActive()) {
                animTimer->stop();
            }
        }
        update();
    }

    void setValue(int value)
    {
        val = value;
        update();
    }

    int maximum() const { return max; }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF r = rect();
        const qreal radius = r.height() / 2.0;

        QColor bg("#212836");
        QColor border("#2b3446");
        QColor fill("#5aa9e6");

        painter.setPen(QPen(border, 1));
        painter.setBrush(bg);
        painter.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);

        const qreal innerRadius = std::max<qreal>(0.0, radius - 1.0);
        QPainterPath clipPath;
        clipPath.addRoundedRect(r.adjusted(1, 1, -1, -1), innerRadius, innerRadius);
        painter.save();
        painter.setClipPath(clipPath);

        if (indeterminate) {
            const qreal chunkW = r.width() * 0.3;
            const qreal x = (animOffset / 200.0) * (r.width() + chunkW) - chunkW;
            QRectF chunkRect(x, r.top() + 1, chunkW, r.height() - 2);
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            qreal chunkRadius = chunkRect.height() / 2.0;
            painter.drawRoundedRect(chunkRect, chunkRadius, chunkRadius);
            painter.restore();
            return;
        }

        if (max <= min) {
            painter.restore();
            return;
        }
        qreal ratio = (val - min) / static_cast<qreal>(max - min);
        ratio = std::clamp<qreal>(ratio, 0.0, 1.0);
        qreal fillWidth = (r.width() - 2) * ratio;
        if (fillWidth <= 0.5) {
            painter.restore();
            return;
        }
        QRectF fillRect(r.left() + 1, r.top() + 1, fillWidth, r.height() - 2);
        qreal fillRadius = std::min(fillRect.height() / 2.0, fillRect.width() / 2.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawRoundedRect(fillRect, fillRadius, fillRadius);
        painter.restore();

        if (showText) {
            int percent = static_cast<int>(ratio * 100.0 + 0.5);
            painter.setPen(QColor("#e8ecf2"));
            QFont font = painter.font();
            font.setPointSize(8);
            painter.setFont(font);
            painter.drawText(r, Qt::AlignCenter, QString("%1%").arg(percent));
        }
    }

private:
    int min = 0;
    int max = 0;
    int val = 0;
    bool indeterminate = true;
    int animOffset = 0;
    QTimer *animTimer = nullptr;
    bool showText = false;
};

class ConsoleDialog : public QDialog
{
public:
    explicit ConsoleDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(I18n::tr("Console"));
        setModal(false);
        resize(720, 420);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(12, 12, 12, 12);
        root->setSpacing(10);

        statusLabel = new QLabel(I18n::tr("Starting..."), this);
        statusLabel->setObjectName("ConsoleStatus");
        root->addWidget(statusLabel);

        logView = new QTextEdit(this);
        logView->setReadOnly(true);
        logView->setObjectName("ConsoleLog");
        root->addWidget(logView, 1);

        auto *buttons = new QHBoxLayout();
        buttons->addStretch();
        stopButton = new QPushButton(I18n::tr("Stop"), this);
        closeButton = new QPushButton(I18n::tr("Close"), this);
        buttons->addWidget(stopButton);
        buttons->addWidget(closeButton);
        root->addLayout(buttons);

        connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
        connect(stopButton, &QPushButton::clicked, this, [this]() {
            if (onStop) {
                onStop();
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
              QLabel#ConsoleStatus {
                  color: #9fb0d6;
              }
              QTextEdit#ConsoleLog {
                  background-color: #0f131c;
                  border: 1px solid #2f3b52;
                  border-radius: 10px;
                  color: #e8ecf2;
                  padding: 8px;
                  font-family: Consolas, "Cascadia Mono", "Courier New";
                  font-size: 10pt;
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
          )QSS");
          QTimer::singleShot(0, this, [this]() { applyFadeIn(this, 180); });
      }

    void appendLine(const QString &line)
    {
        logView->append(line);
    }

    void setStatus(const QString &text)
    {
        statusLabel->setText(text);
    }

    void setStopEnabled(bool enabled)
    {
        if (stopButton) {
            stopButton->setEnabled(enabled);
        }
    }

    void setStopHandler(const std::function<void()> &handler)
    {
        onStop = handler;
    }

private:
    QLabel *statusLabel;
    QTextEdit *logView;
    QPushButton *closeButton;
    QPushButton *stopButton;
    std::function<void()> onStop;
};

class AdminUpdateDialog : public QDialog
{
public:
    AdminUpdateDialog(const QString &apiBaseUrl,
                      const QString &accessToken,
                      const BuildInfo *currentBuild,
                      QWidget *parent = nullptr)
        : QDialog(parent), apiBaseUrl(apiBaseUrl), accessToken(accessToken)
    {
        setWindowTitle(I18n::tr("Admin"));
        setModal(true);
        setFixedSize(520, 440);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(14, 14, 14, 14);
        root->setSpacing(10);

        QFormLayout *form = new QFormLayout();
        buildIdEdit = new QLineEdit(this);
        repoEdit = new QLineEdit(this);
        assetEdit = new QLineEdit(this);

        if (currentBuild) {
            const QString id = QString::fromStdString(currentBuild->id);
            buildIdEdit->setText(id);
            if (!currentBuild->githubRepo.empty()) {
                repoEdit->setText(QString::fromStdString(currentBuild->githubRepo));
            }
            if (!currentBuild->githubAsset.empty()) {
                assetEdit->setText(QString::fromStdString(currentBuild->githubAsset));
            }
            QSettings settings;
            const QString savedRepo = settings.value("admin/repo/" + id).toString();
            const QString savedAsset = settings.value("admin/asset/" + id).toString();
            if (!savedRepo.isEmpty()) {
                repoEdit->setText(savedRepo);
            }
            if (!savedAsset.isEmpty()) {
                assetEdit->setText(savedAsset);
            }
        }

        repoEdit->setPlaceholderText("owner/repo");
        assetEdit->setPlaceholderText("build.zip");

        form->addRow(I18n::tr("Build ID"), buildIdEdit);
        form->addRow(I18n::tr("Repo (owner/repo)"), repoEdit);
          form->addRow(I18n::tr("Asset name (optional)"), assetEdit);
        root->addLayout(form);

        statusLabel = new QLabel(I18n::tr("Tip: keep asset name stable for auto-updates."), this);
        statusLabel->setStyleSheet("color: #a5b4d8;");
        statusLabel->setWordWrap(true);
        root->addWidget(statusLabel);

        updateNameCheck = new QCheckBox(I18n::tr("Use asset name as build name"), this);
        updateNameCheck->setChecked(true);
        root->addWidget(updateNameCheck);

        auto *buttons = new QHBoxLayout();
        buttons->addStretch();
        updateButton = new QPushButton(I18n::tr("Update from latest"), this);
        auto *closeButton = new QPushButton(I18n::tr("Close"), this);
        buttons->addWidget(updateButton);
        buttons->addWidget(closeButton);
        root->addLayout(buttons);

        auto *line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setStyleSheet("color: #2f3a4f;");
        root->addWidget(line);

        auto *deviceTitle = new QLabel(I18n::tr("Reset device binding"), this);
        deviceTitle->setStyleSheet("color: #c9d3ee; font-weight: bold;");
        root->addWidget(deviceTitle);

        QFormLayout *deviceForm = new QFormLayout();
        deviceEmailEdit = new QLineEdit(this);
        deviceEmailEdit->setPlaceholderText(I18n::tr("email@gmail.com"));
        deviceForm->addRow(I18n::tr("User email"), deviceEmailEdit);
        root->addLayout(deviceForm);

        deviceStatusLabel = new QLabel("", this);
        deviceStatusLabel->setStyleSheet("color: #a5b4d8;");
        deviceStatusLabel->setWordWrap(true);
        root->addWidget(deviceStatusLabel);

        auto *deviceButtons = new QHBoxLayout();
        deviceButtons->addStretch();
        resetDeviceButton = new QPushButton(I18n::tr("Reset device"), this);
        deviceButtons->addWidget(resetDeviceButton);
        root->addLayout(deviceButtons);

        connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(updateButton, &QPushButton::clicked, this, [this]() { runUpdate(); });
        connect(resetDeviceButton, &QPushButton::clicked, this, [this]() { runResetDevice(); });

          setStyleSheet(R"QSS(
              QDialog {
                  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                      stop:0 #151c29, stop:1 #1c2636);
                  color: #e8ecf2;
                  border: 1px solid #2f3b52;
                  border-radius: 12px;
              }
              QLabel {
                  color: #c9d3ee;
              }
              QLineEdit {
                  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                      stop:0 #222c3e, stop:1 #1d2737);
                  border: 1px solid #2f3a4f;
                  border-radius: 8px;
                  padding: 4px 10px;
                  color: #e8ecf2;
              }
              QLineEdit:focus {
                  border: 1px solid #5aa9e6;
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
          )QSS");
          QTimer::singleShot(0, this, [this]() { applyFadeIn(this, 180); });
      }

private:
      void runUpdate()
      {
          const QString buildId = buildIdEdit->text().trimmed();
          QString repo = repoEdit->text().trimmed();
          QString asset = assetEdit->text().trimmed();
          if (buildId.isEmpty()) {
              statusLabel->setText(I18n::tr("Fill in all fields."));
              return;
          }
          QSettings settings;
          if (repo.isEmpty()) {
              statusLabel->setText(I18n::tr("Fill in all fields."));
              return;
          }

        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl(apiBaseUrl + "/admin/api/builds/github-latest"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        if (!accessToken.isEmpty()) {
            request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
        }

        QJsonObject payload;
        payload["buildId"] = buildId;
        payload["repo"] = repo;
        payload["assetName"] = asset;
        payload["updateName"] = updateNameCheck ? updateNameCheck->isChecked() : false;
        QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson());

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QByteArray body = reply->readAll();
        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            QString detail = extractDetailMessage(body);
            if (detail.isEmpty() && isOfflineNetworkError(reply->error())) {
                detail = I18n::tr("No internet connection. Please check your network.");
            } else if (detail.isEmpty()) {
                detail = reply->errorString();
            }
            statusLabel->setText(I18n::tr("Update failed: %1").arg(detail));
            reply->deleteLater();
            return;
        }
        reply->deleteLater();

          settings.setValue("admin/repo/" + buildId, repo);
          if (asset.isEmpty()) {
              settings.remove("admin/asset/" + buildId);
          } else {
              settings.setValue("admin/asset/" + buildId, asset);
          }
          statusLabel->setText(I18n::tr("Update successful."));
      }

      void runResetDevice()
      {
          const QString email = deviceEmailEdit ? deviceEmailEdit->text().trimmed() : QString();
          if (email.isEmpty() || !email.contains('@')) {
              if (deviceStatusLabel) {
                  deviceStatusLabel->setText(I18n::tr("Invalid email."));
              }
              return;
          }

          QNetworkAccessManager manager;
          QNetworkRequest request(QUrl(apiBaseUrl + "/admin/api/device-reset"));
          request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
          if (!accessToken.isEmpty()) {
              request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
          }

          QJsonObject payload;
          payload["email"] = email;
          QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson());

          QEventLoop loop;
          QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
          loop.exec();

          const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
          QByteArray body = reply->readAll();
          if (reply->error() != QNetworkReply::NoError || status >= 400) {
              QString detail = extractDetailMessage(body);
              if (detail == "User not found") {
                  detail = I18n::tr("User not found.");
              } else if (detail.isEmpty() && isOfflineNetworkError(reply->error())) {
                  detail = I18n::tr("No internet connection. Please check your network.");
              } else if (detail.isEmpty()) {
                  detail = reply->errorString();
              }
              if (deviceStatusLabel) {
                  deviceStatusLabel->setText(detail);
              }
              reply->deleteLater();
              return;
          }
          reply->deleteLater();

          QJsonDocument doc = QJsonDocument::fromJson(body);
          QString statusText = I18n::tr("Device binding reset.");
          if (doc.isObject()) {
              const QString res = doc.object().value("status").toString();
              if (res == "no_binding") {
                  statusText = I18n::tr("Device not bound.");
              }
          }
          if (deviceStatusLabel) {
              deviceStatusLabel->setText(statusText);
          }
      }

    QString apiBaseUrl;
    QString accessToken;
    QLineEdit *buildIdEdit = nullptr;
    QLineEdit *repoEdit = nullptr;
    QLineEdit *assetEdit = nullptr;
    QLabel *statusLabel = nullptr;
    QPushButton *updateButton = nullptr;
    QCheckBox *updateNameCheck = nullptr;
    QLineEdit *deviceEmailEdit = nullptr;
    QLabel *deviceStatusLabel = nullptr;
    QPushButton *resetDeviceButton = nullptr;
};

struct ProgressUi {
    QDialog *dialog;
    QLabel *titleLabel;
    RoundedProgressBar *bar;
    QLabel *statusLabel;
    RoundedProgressBar *taskBar;
    QLabel *speedLabel;
    QLabel *etaLabel;
    QLabel *filesLabel;
};

ProgressUi createProgressUi(QWidget *parent, int steps)
{
    auto *dialog = new QDialog(parent);
    dialog->setWindowTitle(I18n::tr("Downloading"));
    dialog->setModal(true);
    dialog->setFixedSize(520, 200);

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(8);

    auto *titleLabel = new QLabel(I18n::tr("Preparing downloads..."), dialog);
    titleLabel->setStyleSheet("color: #e8ecf2; font-weight: bold;");
    auto *bar = new RoundedProgressBar(dialog);
    bar->setFixedHeight(18);
    bar->setShowText(true);
    if (steps <= 0) {
        bar->setRange(0, 0);
    } else {
        bar->setRange(0, steps);
        bar->setValue(0);
    }

    auto *infoRow = new QHBoxLayout();
    infoRow->setSpacing(8);
    auto *speedLabel = new QLabel(I18n::tr("Speed: %1 MB/s").arg("0.0"), dialog);
    speedLabel->setStyleSheet("color: #a5b4d8;");
    auto *etaLabel = new QLabel(I18n::tr("ETA: --"), dialog);
    etaLabel->setStyleSheet("color: #a5b4d8;");
    auto *filesLabel = new QLabel(I18n::tr("Files: %1 / %2").arg(0).arg(0), dialog);
    filesLabel->setStyleSheet("color: #a5b4d8;");
    infoRow->addWidget(speedLabel);
    infoRow->addSpacing(12);
    infoRow->addWidget(etaLabel);
    infoRow->addStretch();
    infoRow->addWidget(filesLabel);

    auto *statusLabel = new QLabel(I18n::tr("Starting..."), dialog);
    statusLabel->setStyleSheet("color: #a5b4d8;");
    statusLabel->setWordWrap(true);
    statusLabel->setMinimumHeight(40);

    auto *taskBar = new RoundedProgressBar(dialog);
    taskBar->setFixedHeight(10);
    taskBar->setShowText(false);
    taskBar->setRange(0, 0);

    layout->addWidget(titleLabel);
    layout->addWidget(bar);
    layout->addLayout(infoRow);
    layout->addWidget(statusLabel);
    layout->addWidget(taskBar);

    dialog->setStyleSheet(R"QSS(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #151c29, stop:1 #1c2636);
            color: #e8ecf2;
            border: 1px solid #2f3b52;
            border-radius: 12px;
        }
    )QSS");

    dialog->show();
    return {dialog, titleLabel, bar, statusLabel, taskBar, speedLabel, etaLabel, filesLabel};
}

bool extractZip(const QString &zipPath, const QString &destPath, QString *error)
{
    QFile file(zipPath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = I18n::tr("Archive not found: %1").arg(zipPath);
        }
        return false;
    }
    QByteArray magic = file.read(4);
    file.close();
    if (magic.size() < 4 || !(magic.startsWith("PK"))) {
        if (error) {
            *error = I18n::tr("Downloaded file is not a ZIP archive: %1").arg(zipPath);
        }
        return false;
    }

    QDir().mkpath(destPath);
    QString command = QString("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force")
                          .arg(zipPath, destPath);
    int code = QProcess::execute("powershell",
                                 {"-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command});
    if (code != 0) {
        if (error) {
            *error = I18n::tr("Failed to extract archive: %1").arg(zipPath);
        }
        return false;
    }
    return true;
}

QString extractDetailMessage(const QByteArray &body)
{
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        return {};
    }
    return doc.object().value("detail").toString();
}

QString taskStatusText(const QString &action, const QString &progressText, const QString &filePath, const QString &destPath)
{
    QString text = action;
    if (!progressText.isEmpty()) {
        text += " | " + progressText;
    }
    return text;
}

QString labelDownload()
{
    return I18n::tr("Downloading");
}

QString labelExtract()
{
    return I18n::tr("Extracting");
}

QString labelWrite()
{
    return I18n::tr("Writing");
}

QString taskActionText(const Task &task, const QString &fallback)
{
    return task.label.isEmpty() ? fallback : task.label;
}

QString translateDownloaderError(const QString &err)
{
    if (err.isEmpty()) {
        return err;
    }
    if (err == "InternetOpen failed") {
        return I18n::tr("InternetOpen failed");
    }
    if (err == "InternetReadFile failed") {
        return I18n::tr("InternetReadFile failed");
    }
    const QString urlPrefix = "InternetOpenUrl failed: ";
    if (err.startsWith(urlPrefix)) {
        return I18n::tr("InternetOpenUrl failed: %1").arg(err.mid(urlPrefix.size()));
    }
    const QString writePrefix = "Failed to write file: ";
    if (err.startsWith(writePrefix)) {
        return I18n::tr("Failed to write file: %1").arg(err.mid(writePrefix.size()));
    }
    const QString dirPrefix = "Failed to create directory for ";
    if (err.startsWith(dirPrefix)) {
        return I18n::tr("Failed to create directory for %1").arg(err.mid(dirPrefix.size()));
    }
    QRegularExpression re("^HTTP error (\\d+): (.+)$");
    QRegularExpressionMatch match = re.match(err);
    if (match.hasMatch()) {
        return I18n::tr("HTTP error %1: %2").arg(match.captured(1), match.captured(2));
    }
    return err;
}

bool rulesAllow(const QJsonArray &rules)
{
    if (rules.isEmpty()) {
        return true;
    }
    bool allowed = false;
    for (const auto &val : rules) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject rule = val.toObject();
        QString action = rule.value("action").toString();
        QJsonObject os = rule.value("os").toObject();
        QString osName = os.value("name").toString();
        const bool matches = osName.isEmpty() || osName == "windows";
        if (matches) {
            allowed = (action == "allow");
        }
    }
    return allowed;
}

QString mavenPathFromName(const QString &name)
{
    const QStringList parts = name.split(':');
    if (parts.size() < 3) {
        return {};
    }
    QString group = parts[0];
    group.replace('.', '/');
    const QString artifact = parts[1];
    const QString version = parts[2];
    QString classifier;
    if (parts.size() >= 4) {
        classifier = parts[3];
    }
    QString file = artifact + "-" + version;
    if (!classifier.isEmpty()) {
        file += "-" + classifier;
    }
    file += ".jar";
    return group + "/" + artifact + "/" + version + "/" + file;
}

bool buildVanillaTasks(const QString &mcVersion,
                       const QString &minecraftDir,
                       QVector<Task> &tasks,
                       QString *error)
{
    QByteArray manifestBytes;
    if (!downloadBytesWinInet("https://launchermeta.mojang.com/mc/game/version_manifest.json",
                              "", manifestBytes, error)) {
        return false;
    }
    QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestBytes);
    if (!manifestDoc.isObject()) {
        if (error) {
            *error = I18n::tr("Invalid version manifest.");
        }
        return false;
    }
    QJsonArray versions = manifestDoc.object().value("versions").toArray();
    QString versionUrl;
    for (const auto &val : versions) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject obj = val.toObject();
        if (obj.value("id").toString() == mcVersion) {
            versionUrl = obj.value("url").toString();
            break;
        }
    }
    if (versionUrl.isEmpty()) {
        if (error) {
            *error = I18n::tr("Minecraft version not found: %1").arg(mcVersion);
        }
        return false;
    }

    QByteArray versionBytes;
    if (!downloadBytesWinInet(versionUrl, "", versionBytes, error)) {
        return false;
    }
    QJsonDocument versionDoc = QJsonDocument::fromJson(versionBytes);
    if (!versionDoc.isObject()) {
        if (error) {
            *error = I18n::tr("Invalid version JSON.");
        }
        return false;
    }
    QJsonObject versionObj = versionDoc.object();
    const QString versionId = versionObj.value("id").toString(mcVersion);
    const QString versionFolder = minecraftDir + "/versions/" + versionId;

    tasks.push_back({
        TaskKind::Write,
        I18n::tr("Saving version JSON"),
        {},
        versionFolder + "/" + versionId + ".json",
        {},
        versionBytes,
        {},
        versionBytes.size(),
        true
    });

    const QJsonObject downloads = versionObj.value("downloads").toObject();
    const QJsonObject clientObj = downloads.value("client").toObject();
    const QString clientUrl = clientObj.value("url").toString();
    const qint64 clientSize = static_cast<qint64>(clientObj.value("size").toDouble(0));
    if (!clientUrl.isEmpty()) {
        tasks.push_back({
            TaskKind::Download,
            I18n::tr("Downloading Minecraft client"),
            clientUrl,
            versionFolder + "/" + versionId + ".jar",
            {},
            {},
            {},
            clientSize,
            true
        });
    }

    const QJsonArray libraries = versionObj.value("libraries").toArray();
    for (const auto &libVal : libraries) {
        if (!libVal.isObject()) {
            continue;
        }
        QJsonObject lib = libVal.toObject();
        if (!rulesAllow(lib.value("rules").toArray())) {
            continue;
        }
        QJsonObject downloadsObj = lib.value("downloads").toObject();
        QJsonObject artifact = downloadsObj.value("artifact").toObject();
        if (!artifact.isEmpty()) {
            QString path = artifact.value("path").toString();
            QString url = artifact.value("url").toString();
            qint64 size = static_cast<qint64>(artifact.value("size").toDouble(0));
            if (!path.isEmpty() && !url.isEmpty()) {
                tasks.push_back({
                    TaskKind::Download,
                    I18n::tr("Downloading library"),
                    url,
                    minecraftDir + "/libraries/" + path,
                    {},
                    {},
                    {},
                    size,
                    true
                });
            }
        }

        QJsonObject natives = lib.value("natives").toObject();
        if (!natives.isEmpty()) {
            QString nativeKey = natives.value("windows").toString();
            if (nativeKey.isEmpty()) {
                nativeKey = natives.value("windows-64").toString();
            }
            QJsonObject classifiers = downloadsObj.value("classifiers").toObject();
            QJsonObject nativeObj = classifiers.value(nativeKey).toObject();
            if (!nativeObj.isEmpty()) {
                QString path = nativeObj.value("path").toString();
                QString url = nativeObj.value("url").toString();
                qint64 size = static_cast<qint64>(nativeObj.value("size").toDouble(0));
                if (!path.isEmpty() && !url.isEmpty()) {
                    QString nativeZip = minecraftDir + "/libraries/" + path;
                    tasks.push_back({
                        TaskKind::Download,
                        I18n::tr("Downloading native"),
                        url,
                        nativeZip,
                        {},
                        {},
                        {},
                        size,
                        true
                    });
                    tasks.push_back({
                        TaskKind::Extract,
                        I18n::tr("Extracting native"),
                        {},
                        nativeZip,
                        {},
                        {},
                        minecraftDir + "/natives/" + versionId,
                        -1,
                        false
                    });
                }
            }
        }
    }

    QJsonObject assetIndex = versionObj.value("assetIndex").toObject();
    const QString assetIndexId = assetIndex.value("id").toString();
    const QString assetIndexUrl = assetIndex.value("url").toString();
    if (!assetIndexUrl.isEmpty() && !assetIndexId.isEmpty()) {
        QByteArray assetBytes;
        if (!downloadBytesWinInet(assetIndexUrl, "", assetBytes, error)) {
            return false;
        }
        tasks.push_back({
            TaskKind::Write,
            I18n::tr("Saving asset index"),
            {},
            minecraftDir + "/assets/indexes/" + assetIndexId + ".json",
            {},
            assetBytes,
            {},
            assetBytes.size(),
            true
        });

        QJsonDocument assetDoc = QJsonDocument::fromJson(assetBytes);
        QJsonObject objects = assetDoc.object().value("objects").toObject();
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            QJsonObject obj = it.value().toObject();
            QString hash = obj.value("hash").toString();
            qint64 size = static_cast<qint64>(obj.value("size").toDouble(0));
            if (hash.length() < 2) {
                continue;
            }
            const QString subdir = hash.left(2);
            const QString assetUrl = "https://resources.download.minecraft.net/" + subdir + "/" + hash;
            const QString assetPath = minecraftDir + "/assets/objects/" + subdir + "/" + hash;
            tasks.push_back({
                TaskKind::Download,
                I18n::tr("Downloading asset"),
                assetUrl,
                assetPath,
                {},
                {},
                {},
                size,
                true
            });
        }
    }

    return true;
}

bool buildFabricTasks(const QString &mcVersion,
                      const QString &minecraftDir,
                      QVector<Task> &tasks,
                      QString *error)
{
    QByteArray loadersBytes;
    if (!downloadBytesWinInet("https://meta.fabricmc.net/v2/versions/loader/" + mcVersion,
                              "", loadersBytes, error)) {
        return false;
    }
    QJsonDocument loadersDoc = QJsonDocument::fromJson(loadersBytes);
    if (!loadersDoc.isArray()) {
        if (error) {
            *error = I18n::tr("Invalid Fabric loader list.");
        }
        return false;
    }
    QString loaderVersion;
    QJsonArray loaders = loadersDoc.array();
    for (const auto &val : loaders) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject obj = val.toObject();
        QJsonObject loader = obj.value("loader").toObject();
        loaderVersion = loader.value("version").toString();
        if (!loaderVersion.isEmpty()) {
            break;
        }
    }
    if (loaderVersion.isEmpty()) {
        if (error) {
            *error = I18n::tr("Fabric loader version not found.");
        }
        return false;
    }

    const QString profileUrl = QString("https://meta.fabricmc.net/v2/versions/loader/%1/%2/profile/json")
                                   .arg(mcVersion, loaderVersion);
    QByteArray profileBytes;
    if (!downloadBytesWinInet(profileUrl, "", profileBytes, error)) {
        return false;
    }

    QJsonDocument profileDoc = QJsonDocument::fromJson(profileBytes);
    if (!profileDoc.isObject()) {
        if (error) {
            *error = I18n::tr("Invalid Fabric profile JSON.");
        }
        return false;
    }
    QJsonObject profileObj = profileDoc.object();
    const QString profileId = profileObj.value("id").toString();
    if (profileId.isEmpty()) {
        if (error) {
            *error = I18n::tr("Fabric profile id missing.");
        }
        return false;
    }

    const QString profileFolder = minecraftDir + "/versions/" + profileId;
    tasks.push_back({
        TaskKind::Write,
        I18n::tr("Saving Fabric profile"),
        {},
        profileFolder + "/" + profileId + ".json",
        {},
        profileBytes,
        {},
        profileBytes.size(),
        true
    });

    QJsonArray libraries = profileObj.value("libraries").toArray();
    for (const auto &val : libraries) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject lib = val.toObject();
        if (!rulesAllow(lib.value("rules").toArray())) {
            continue;
        }
        const QString name = lib.value("name").toString();
        const QString urlBase = lib.value("url").toString("https://libraries.minecraft.net/");
        const QString path = mavenPathFromName(name);
        if (path.isEmpty()) {
            continue;
        }
        QString url = urlBase;
        if (!url.endsWith('/')) {
            url += "/";
        }
        url += path;
        tasks.push_back({
            TaskKind::Download,
            I18n::tr("Downloading Fabric library"),
            url,
            minecraftDir + "/libraries/" + path,
            {},
            {},
            {},
            -1,
            true
        });
    }

    return true;
}

bool loadJsonObject(const QString &path, QJsonObject &out, QString *error)
{
    QFile file(path);
    if (!file.exists()) {
        if (error) {
            *error = I18n::tr("File not found: %1").arg(path);
        }
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = I18n::tr("Failed to read file: %1").arg(path);
        }
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        if (error) {
            *error = I18n::tr("Invalid JSON: %1").arg(path);
        }
        return false;
    }
    out = doc.object();
    return true;
}

QString findFabricVersionId(const QString &versionsDir, const QString &mcVersion)
{
    QDir dir(versionsDir);
    if (!dir.exists()) {
        return {};
    }
    const QString prefix = "fabric-loader-" + mcVersion;
    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const auto &entry : entries) {
        if (entry.startsWith(prefix)) {
            return entry;
        }
    }
    // Fallback: any Fabric loader present.
    for (const auto &entry : entries) {
        if (entry.startsWith("fabric-loader-")) {
            return entry;
        }
    }
    return {};
}

QString findJavaExecutable(const QString &rootDir)
{
    QDirIterator it(rootDir, {"javaw.exe", "java.exe"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo info(path);
        if (info.fileName().compare("javaw.exe", Qt::CaseInsensitive) == 0) {
            return path;
        }
        if (info.fileName().compare("java.exe", Qt::CaseInsensitive) == 0) {
            return path;
        }
    }
    return {};
}

QString findBuildRoot(const QString &appDir,
                      const QString &folderName,
                      const QString &mcVersion,
                      bool requireFabric)
{
    const QString parentDir = QDir(appDir).absolutePath() + "/..";
    const QStringList candidates = {
        appDir,
        QDir(parentDir).absolutePath(),
        QDir(appDir).filePath("build-mingw"),
        QDir(parentDir).filePath("build-mingw")
    };

    QString fallback;
    for (const auto &base : candidates) {
        const QString root = QDir(base).filePath(folderName);
        const QString minecraftDir = QDir(root).filePath("minecraft");
        if (QDir(root).exists() && QDir(minecraftDir).exists()) {
            if (fallback.isEmpty()) {
                fallback = root;
            }
            if (requireFabric) {
                const QString versionsDir = QDir(minecraftDir).filePath("versions");
                if (!findFabricVersionId(versionsDir, mcVersion).isEmpty()) {
                    return root;
                }
            } else {
                return root;
            }
        }
    }

    return fallback;
}

QString findBuildRootByMeta(const QString &appDir,
                            const QString &buildId,
                            const QString &mcVersion,
                            bool requireFabric)
{
    if (buildId.isEmpty()) {
        return {};
    }
    const QString parentDir = QDir(appDir).absolutePath() + "/..";
    const QStringList candidates = {
        appDir,
        QDir(parentDir).absolutePath(),
        QDir(appDir).filePath("build-mingw"),
        QDir(parentDir).filePath("build-mingw")
    };

    for (const auto &base : candidates) {
        QDir baseDir(base);
        if (!baseDir.exists()) {
            continue;
        }
        const QStringList entries = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const auto &entry : entries) {
            const QString root = baseDir.filePath(entry);
            QJsonObject meta;
            if (!readBuildMeta(root, meta)) {
                continue;
            }
            if (meta.value("id").toString() != buildId) {
                continue;
            }
            const QString minecraftDir = QDir(root).filePath("minecraft");
            if (!QDir(minecraftDir).exists()) {
                continue;
            }
            if (requireFabric) {
                const QString versionsDir = QDir(minecraftDir).filePath("versions");
                if (!findFabricVersionId(versionsDir, mcVersion).isEmpty()) {
                    return root;
                }
            } else {
                return root;
            }
        }
    }
    return {};
}

QString resolveJavaPathForBuild(const BuildInfo &build, const QString &rootDir)
{
    if (!build.javaPath.empty()) {
        QString custom = QString::fromStdString(build.javaPath);
        if (QFileInfo::exists(custom)) {
            return custom;
        }
    }
    if (!rootDir.isEmpty()) {
        return findJavaExecutable(rootDir);
    }
    return {};
}

bool isBuildInstalled(const BuildInfo &build, const QString &appDirPath)
{
    const QString mcVersion = QString::fromStdString(build.minecraftVersion);
    const bool requireFabric = (QString::fromStdString(build.loader).compare("Fabric", Qt::CaseInsensitive) == 0);
    QString root = findBuildRoot(appDirPath, safeFolderName(QString::fromUtf8(build.name.c_str())),
                                 mcVersion, requireFabric);
    if (!root.isEmpty()) {
        return true;
    }
    if (!build.id.empty()) {
        root = findBuildRoot(appDirPath, safeFolderName(QString::fromStdString(build.id)),
                             mcVersion, requireFabric);
        return !root.isEmpty();
    }
    return false;
}

QString resolveBuildRoot(const BuildInfo &build, const QString &appDirPath, bool *requireFabricOut = nullptr)
{
    const QString mcVersion = QString::fromStdString(build.minecraftVersion);
    const bool requireFabric = (QString::fromStdString(build.loader).compare("Fabric", Qt::CaseInsensitive) == 0);
    if (requireFabricOut) {
        *requireFabricOut = requireFabric;
    }
    QString root = findBuildRoot(appDirPath, safeFolderName(QString::fromUtf8(build.name.c_str())),
                                 mcVersion, requireFabric);
    if (!root.isEmpty()) {
        return root;
    }
    if (!build.id.empty()) {
        root = findBuildRoot(appDirPath, safeFolderName(QString::fromStdString(build.id)),
                             mcVersion, requireFabric);
        if (!root.isEmpty()) {
            return root;
        }
        root = findBuildRootByMeta(appDirPath, QString::fromStdString(build.id),
                                   mcVersion, requireFabric);
    }
    return root;
}

QString buildMetaPath(const QString &root)
{
    return QDir(root).filePath(".ldp_build.json");
}

QString buildManifestPath(const QString &root)
{
    return QDir(root).filePath(".ldp_manifest.json");
}

QStringList manifestTargets()
{
    return {
        "minecraft/mods",
        "minecraft/config",
        "minecraft/defaultconfigs",
        "minecraft/kubejs",
        "minecraft/resourcepacks",
        "minecraft/shaderpacks",
        "minecraft/datapacks",
        "minecraft/scripts"
    };
}

bool appendManifestDir(const QString &root, const QString &relDir, QVector<ManifestEntry> &entries, QString *error)
{
    const QString absDir = QDir(root).filePath(relDir);
    QDir dir(absDir);
    if (!dir.exists()) {
        return true;
    }
    QDirIterator it(absDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString absPath = it.next();
        QFileInfo info(absPath);
        QString sha;
        if (!computeSha1(absPath, sha)) {
            if (error) {
                *error = I18n::tr("Failed to read file: %1").arg(absPath);
            }
            return false;
        }
        const QString rel = QDir(root).relativeFilePath(absPath).replace("\\", "/");
        entries.push_back({rel, sha, info.size()});
    }
    return true;
}

bool writeBuildManifest(const QString &root, QString *error)
{
    QVector<ManifestEntry> entries;
    for (const auto &rel : manifestTargets()) {
        if (!appendManifestDir(root, rel, entries, error)) {
            return false;
        }
    }
    QJsonArray files;
    for (const auto &entry : entries) {
        QJsonObject obj;
        obj["path"] = entry.relPath;
        obj["sha1"] = entry.sha1;
        obj["size"] = static_cast<double>(entry.size);
        files.push_back(obj);
    }
    QJsonObject rootObj;
    rootObj["version"] = 1;
    rootObj["createdAt"] = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();
    rootObj["files"] = files;

    QFile file(buildManifestPath(root));
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = I18n::tr("Failed to write file: %1").arg(buildManifestPath(root));
        }
        return false;
    }
    file.write(QJsonDocument(rootObj).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool readBuildManifest(const QString &root, QVector<ManifestEntry> &entries, QString *error)
{
    QFile file(buildManifestPath(root));
    if (!file.exists()) {
        if (error) {
            *error = I18n::tr("Build manifest not found. Reinstall the build to enable verification.");
        }
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = I18n::tr("Failed to read file: %1").arg(buildManifestPath(root));
        }
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        if (error) {
            *error = I18n::tr("Invalid JSON: %1").arg(buildManifestPath(root));
        }
        return false;
    }
    const QJsonArray files = doc.object().value("files").toArray();
    for (const auto &val : files) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject obj = val.toObject();
        ManifestEntry entry;
        entry.relPath = obj.value("path").toString();
        entry.sha1 = obj.value("sha1").toString();
        entry.size = static_cast<qint64>(obj.value("size").toDouble(-1));
        if (!entry.relPath.isEmpty()) {
            entries.push_back(entry);
        }
    }
    return true;
}

bool writeBuildMeta(const QString &root, const BuildInfo &build)
{
    QJsonObject obj;
    obj["id"] = QString::fromStdString(build.id);
    obj["name"] = QString::fromStdString(build.name);
    obj["description"] = QString::fromStdString(build.description);
    obj["minecraftVersion"] = QString::fromStdString(build.minecraftVersion);
    obj["loader"] = QString::fromStdString(build.loader);
    obj["javaVersion"] = build.javaVersion;
    obj["checksum"] = QString::fromStdString(build.checksum);
    obj["sizeBytes"] = static_cast<double>(build.sizeBytes);
    obj["imageUrl"] = QString::fromStdString(build.imageUrl);
    obj["tags"] = QString::fromStdString(build.tagsCsv);
    obj["downloadedAt"] = QDateTime::currentDateTimeUtc().toSecsSinceEpoch();

    QFile file(buildMetaPath(root));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool readBuildMeta(const QString &root, QJsonObject &out)
{
    QFile file(buildMetaPath(root));
    if (!file.exists()) {
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        return false;
    }
    out = doc.object();
    return true;
}

bool isBuildOutdated(const BuildInfo &build, const QString &root)
{
    const QString expected = normalizeSha256String(QString::fromStdString(build.checksum));
    if (expected.isEmpty()) {
        return false;
    }
    QJsonObject meta;
    if (!readBuildMeta(root, meta)) {
        return false;
    }
    const QString localChecksum = normalizeSha256String(meta.value("checksum").toString());
    if (localChecksum.isEmpty()) {
        return false;
    }
    return localChecksum != expected;
}

QString substituteVars(QString text, const QHash<QString, QString> &vars)
{
    for (auto it = vars.constBegin(); it != vars.constEnd(); ++it) {
        text.replace("${" + it.key() + "}", it.value());
    }
    return text;
}

bool askYesNo(QWidget *parent, const QString &title, const QString &text)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(title);
    box.setText(text);
    auto *yes = box.addButton(I18n::tr("Yes"), QMessageBox::YesRole);
    auto *no = box.addButton(I18n::tr("No"), QMessageBox::NoRole);
    box.setDefaultButton(yes);
    box.exec();
    return box.clickedButton() == yes;
}

void appendArgsValue(const QJsonValue &value, QStringList &out, const QHash<QString, QString> &vars)
{
    if (value.isString()) {
        out << substituteVars(value.toString(), vars);
    } else if (value.isArray()) {
        const QJsonArray arr = value.toArray();
        for (const auto &entry : arr) {
            appendArgsValue(entry, out, vars);
        }
    }
}

void appendArgsArray(const QJsonArray &array, QStringList &out, const QHash<QString, QString> &vars)
{
    for (const auto &val : array) {
        if (val.isString()) {
            out << substituteVars(val.toString(), vars);
            continue;
        }
        if (!val.isObject()) {
            continue;
        }
        QJsonObject obj = val.toObject();
        if (!rulesAllow(obj.value("rules").toArray())) {
            continue;
        }
        appendArgsValue(obj.value("value"), out, vars);
    }
}

QStringList parseArgsString(const QString &args, const QHash<QString, QString> &vars)
{
    QStringList out;
    const QStringList parts = splitJvmArgs(args);
    for (const auto &part : parts) {
        if (!part.isEmpty()) {
            out << substituteVars(part, vars);
        }
    }
    return out;
}

void forceArgValue(QStringList &args, const QString &key, const QString &value)
{
    bool found = false;
    for (int i = 0; i < args.size(); ++i) {
        if (args[i] != key) {
            continue;
        }
        found = true;
        if (i + 1 < args.size()) {
            args[i + 1] = value;
        } else {
            args << value;
        }
        ++i;
    }
    if (!found) {
        args << key << value;
    }
}




void appendLibraries(const QJsonObject &versionObj, const QString &minecraftDir, QStringList &out)
{
    const QJsonArray libs = versionObj.value("libraries").toArray();
    for (const auto &val : libs) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject lib = val.toObject();
        if (!rulesAllow(lib.value("rules").toArray())) {
            continue;
        }

        QJsonObject downloadsObj = lib.value("downloads").toObject();
        QJsonObject artifact = downloadsObj.value("artifact").toObject();
        QString path;
        if (!artifact.isEmpty()) {
            path = artifact.value("path").toString();
        } else {
            const QString name = lib.value("name").toString();
            path = mavenPathFromName(name);
        }
        if (path.isEmpty()) {
            continue;
        }
        const QString fullPath = minecraftDir + "/libraries/" + path;
        if (QFileInfo::exists(fullPath)) {
            out << fullPath;
        }
    }
}

bool computeSha1(const QString &path, QString &out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!hash.addData(&file)) {
        file.close();
        return false;
    }
    file.close();
    out = QString::fromUtf8(hash.result().toHex());
    return true;
}

void appendVerifyLibraries(const QJsonObject &versionObj, const QString &minecraftDir, QVector<VerifyItem> &items)
{
    const QJsonArray libs = versionObj.value("libraries").toArray();
    for (const auto &val : libs) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject lib = val.toObject();
        if (!rulesAllow(lib.value("rules").toArray())) {
            continue;
        }
        QJsonObject downloadsObj = lib.value("downloads").toObject();
        QJsonObject artifact = downloadsObj.value("artifact").toObject();
        QString path;
        QString sha1;
        qint64 size = -1;
        QString url;
        if (!artifact.isEmpty()) {
            path = artifact.value("path").toString();
            sha1 = artifact.value("sha1").toString();
            size = static_cast<qint64>(artifact.value("size").toDouble(0));
            url = artifact.value("url").toString();
        } else {
            const QString name = lib.value("name").toString();
            path = mavenPathFromName(name);
            if (!path.isEmpty()) {
                url = "https://libraries.minecraft.net/" + path;
            }
        }
        if (path.isEmpty()) {
            continue;
        }
        items.push_back({minecraftDir + "/libraries/" + path, sha1, size, url});
    }
}

void appendVerifyAssets(const QString &assetIndexId, const QString &minecraftDir, QVector<VerifyItem> &items)
{
    if (assetIndexId.isEmpty()) {
        return;
    }
    const QString indexPath = minecraftDir + "/assets/indexes/" + assetIndexId + ".json";
    QJsonObject indexObj;
    QString err;
    if (!loadJsonObject(indexPath, indexObj, &err)) {
        return;
    }
    QJsonObject objects = indexObj.value("objects").toObject();
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        QJsonObject obj = it.value().toObject();
        QString hash = obj.value("hash").toString();
        qint64 size = static_cast<qint64>(obj.value("size").toDouble(0));
        if (hash.length() < 2) {
            continue;
        }
        const QString subdir = hash.left(2);
        const QString assetPath = minecraftDir + "/assets/objects/" + subdir + "/" + hash;
        const QString assetUrl = "https://resources.download.minecraft.net/" + subdir + "/" + hash;
        items.push_back({assetPath, hash, size, assetUrl});
    }
}

QString assetIndexIdFromVersion(const QJsonObject &versionObj)
{
    QJsonObject assetIndex = versionObj.value("assetIndex").toObject();
    return assetIndex.value("id").toString();
}

QString fetchDownloadUrl(const QString &apiBaseUrl, const QString &accessToken, const QString &buildId, QString *error)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(apiBaseUrl + "/builds/" + buildId + "/download"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!accessToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    }

    QNetworkReply *reply = manager.post(request, "{}");
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray body = reply->readAll();
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            if (error) {
                if (status == 403) {
                    *error = I18n::tr("This build is paid. Please contact support.");
                } else if (isOfflineNetworkError(reply->error())) {
                    *error = I18n::tr("No internet connection. Please check your network.");
                } else {
                    QString detail = extractDetailMessage(body);
                    if (!detail.isEmpty()) {
                        *error = detail;
            } else {
                *error = reply->errorString();
            }
            }
        }
        reply->deleteLater();
        return {};
    }
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        if (error) {
            *error = I18n::tr("Invalid download response.");
        }
        return {};
    }

    QString url = doc.object().value("downloadUrl").toString();
    if (url.startsWith('/')) {
        url = apiBaseUrl + url;
    }
    return url;
}

QString fetchLaunchToken(const QString &apiBaseUrl,
                         const QString &accessToken,
                         const QString &buildId,
                         const QString &deviceId,
                         int *expiresOut,
                         QString *error)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(apiBaseUrl + "/auth/launch-token"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!accessToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    }

    QJsonObject payload;
    payload["buildId"] = buildId;
    payload["deviceId"] = deviceId;

    QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray body = reply->readAll();
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            if (error) {
                QString detail = extractDetailMessage(body);
                if (!detail.isEmpty()) {
                    if (detail == "Device not authorized") {
                        *error = I18n::tr("Device not authorized.");
                    } else if (detail == "Launch token expired") {
                        *error = I18n::tr("Launch token expired.");
                    } else if (detail == "Launch token invalid") {
                        *error = I18n::tr("Launch token invalid.");
                    } else {
                        *error = detail;
                    }
                } else if (isOfflineNetworkError(reply->error())) {
                    *error = I18n::tr("No internet connection. Please check your network.");
                } else {
                    *error = reply->errorString();
                }
            }
        reply->deleteLater();
        return {};
    }
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        if (error) {
            *error = I18n::tr("Invalid launch token response.");
        }
        return {};
    }
    const QJsonObject obj = doc.object();
    const QString token = obj.value("token").toString();
    if (expiresOut) {
        *expiresOut = obj.value("expiresIn").toInt(0);
    }
    return token;
}

QString fetchCheckoutUrl(const QString &apiBaseUrl, const QString &accessToken, const QString &buildId, QString *error)
{
    auto doRequest = [&](const QString &base, int *statusOut, QString *errOut) -> QString {
        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl(base + "/payments/create"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        if (!accessToken.isEmpty()) {
            request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
        }

        QJsonObject payload;
        payload["buildId"] = buildId;
        QNetworkReply *reply = manager.post(request, QJsonDocument(payload).toJson());

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        QByteArray body = reply->readAll();
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusOut) {
            *statusOut = status;
        }
        if (reply->error() != QNetworkReply::NoError || status >= 400) {
            if (errOut) {
                QString detail = extractDetailMessage(body);
                if (status == 401 || status == 403) {
                    *errOut = I18n::tr("Access denied. Please log in and try again.");
                } else if (isOfflineNetworkError(reply->error())) {
                    *errOut = I18n::tr("No internet connection. Please check your network.");
                } else if (!detail.isEmpty()) {
                    *errOut = detail;
                } else {
                    *errOut = reply->errorString();
                }
            }
            reply->deleteLater();
            return {};
        }
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            if (errOut) {
                *errOut = I18n::tr("Invalid payment response.");
            }
            return {};
        }
        return doc.object().value("checkoutUrl").toString();
    };

    int status = 0;
    QString err;
    QString url = doRequest(apiBaseUrl, &status, &err);
    if (url.isEmpty() && status == 404) {
        QUrl base(apiBaseUrl);
        if (!base.scheme().isEmpty() && !base.host().isEmpty()) {
            QString root = base.scheme() + "://" + base.host();
            if (base.port() > 0) {
                root += ":" + QString::number(base.port());
            }
            if (root != apiBaseUrl) {
                QString retryErr;
                int retryStatus = 0;
                QString retryUrl = doRequest(root, &retryStatus, &retryErr);
                if (!retryUrl.isEmpty()) {
                    return retryUrl;
                }
                if (err.isEmpty()) {
                    err = retryErr;
                }
            }
        }
    }

    if (url.isEmpty() && error) {
        *error = err;
    }
    return url;
}

}

MainWindow::MainWindow(const QString &userEmail, const QString &apiBaseUrl, const QString &accessToken, QWidget *parent)
    : QMainWindow(parent), userEmail(userEmail), apiBaseUrl(apiBaseUrl), accessToken(accessToken)
{
    setWindowTitle(I18n::tr("LD Launcher"));
    QIcon appIcon(":/images/1.png");
    if (!appIcon.isNull()) {
        setWindowIcon(appIcon);
        QApplication::setWindowIcon(appIcon);
    }
    resize(1100, 700);

    QFont appFont("Segoe UI", 10);
    QApplication::setFont(appFont);

    setupUI();
    if (qApp) {
        qApp->installEventFilter(this);
    }
    applyTranslations();
    loadMemorySettings();
    loadPerformanceSettings();
    loadDownloadSettings();
    loadAudioSettings();
    loadUiSettings();
    loadBuilds();
    applyAudioSettings();
    updateBackgroundAnimation();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (didAnimate) {
        return;
    }
    didAnimate = true;
    applyFadeIn(this, 220);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress && buildList) {
        auto *widget = qobject_cast<QWidget *>(watched);
        if (widget && widget->window() == this) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            QWidget *viewport = buildList->viewport();
            if (viewport) {
                const QPoint localPos = viewport->mapFromGlobal(mouseEvent->globalPos());
                if (viewport->rect().contains(localPos)) {
                    if (!buildList->indexAt(localPos).isValid()) {
                        buildList->clearSelection();
                        buildList->setCurrentIndex(QModelIndex());
                        onBuildSelected(-1);
                    }
                    return QMainWindow::eventFilter(watched, event);
                }
            }
            if (sidePanelFrame && (widget == sidePanelFrame || sidePanelFrame->isAncestorOf(widget))) {
                return QMainWindow::eventFilter(watched, event);
            }
            if (!buildList->isAncestorOf(widget) && widget != buildList) {
                buildList->clearSelection();
                buildList->setCurrentIndex(QModelIndex());
                onBuildSelected(-1);
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::setupUI()
{
    auto *centralWidget = new AnimatedBackgroundWidget(this);
    backgroundWidget = centralWidget;
    setCentralWidget(centralWidget);

    QVBoxLayout *rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    // Top bar
    QFrame *topBar = new QFrame(this);
    topBarFrame = topBar;
    topBar->setObjectName("TopBar");
    topBar->setAttribute(Qt::WA_StyledBackground, true);
    topBar->setAutoFillBackground(true);
    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 12, 16, 12);

    logoLabel = new QLabel(this);
    QPixmap logoPix(":/images/2.png");
    if (!logoPix.isNull()) {
        logoLabel->setPixmap(logoPix.scaled(28, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    logoLabel->setFixedSize(28, 28);
    logoLabel->setAlignment(Qt::AlignCenter);

    titleLabel = new QLabel(I18n::tr("LD Launcher"), this);

    musicButton = new QToolButton(this);
    musicButton->setObjectName("MusicButton");
    musicButton->setText("");
    musicButton->setIconSize(QSize(16, 16));
    musicButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    musicButton->setToolTip(I18n::tr("Music"));

    accountButton = new QToolButton(this);
    accountButton->setObjectName("AccountButton");
    accountButton->setText("");
    accountButton->setIcon(glyphIcon(QChar(0xE700)));
    accountButton->setIconSize(QSize(16, 16));
    accountButton->setPopupMode(QToolButton::InstantPopup);
    accountButton->setToolButtonStyle(Qt::ToolButtonIconOnly);

    auto *accountMenu = new QMenu(this);
    accountMenu->setAttribute(Qt::WA_StyledBackground, true);
    accountMenu->installEventFilter(new PopupRounder(10, accountMenu));
    accountMenu->setStyleSheet(R"QSS(
        QMenu {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #1b2435, stop:1 #141b28);
            border: 1px solid #33405a;
            border-radius: 12px;
            min-width: 180px;
            padding: 6px;
        }
        QMenu::item {
            padding: 8px 16px 8px 44px;
            min-width: 220px;
            margin: 0px;
            color: #e8ecf2;
            border-radius: 8px;
        }
        QMenu::icon {
            left: 16px;
        }
        QMenu::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #2b3b55, stop:1 #24324a);
            color: #e8ecf2;
            border: 1px solid #5aa9e6;
            border-radius: 8px;
        }
        QMenu::separator {
            height: 1px;
            background-color: #2a3447;
            margin: 4px 8px;
        }
        QMenu::item:disabled {
            color: #6f7994;
        }
    )QSS");
    QWidget *headerWidget = new QWidget(accountMenu);
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(12, 10, 12, 6);
    headerLayout->setSpacing(4);
    accountHeaderLabel = new QLabel(I18n::tr("Account"), headerWidget);
    accountHeaderLabel->setStyleSheet("color: #7d88a6; font-size: 10px; text-transform: uppercase;");
    accountEmailLabel = new QLabel(userEmail, headerWidget);
    accountEmailLabel->setStyleSheet("color: #e8ecf2;");
    headerLayout->addWidget(accountHeaderLabel);
    headerLayout->addWidget(accountEmailLabel);

    auto *headerAction = new QWidgetAction(accountMenu);
    headerAction->setDefaultWidget(headerWidget);
    accountMenu->addAction(headerAction);
    accountMenu->addSeparator();

    actionSettings = accountMenu->addAction(glyphIcon(QChar(0xE713)), I18n::tr("Settings"));
    actionSupport = accountMenu->addAction(glyphIcon(QChar(0xE77B)), I18n::tr("Support"));
    actionAdmin = accountMenu->addAction(glyphIcon(QChar(0xE7EF)), I18n::tr("Admin"));
    actionAdmin->setVisible(isAdminUser());
    actionLogout = accountMenu->addAction(glyphIcon(QChar(0xE72B)), I18n::tr("Log out"));
    accountButton->setMenu(accountMenu);

    connect(actionSettings, &QAction::triggered, this, &MainWindow::onSettingsClicked);
    connect(actionSupport, &QAction::triggered, this, []() {
        const QUrl url(QStringLiteral("https://mail.google.com/mail/u/6/#inbox?compose=DmwnWrRqhKZWhkdSGTNlpcxRtrgNbPdmkSPpSjwZmTXFMVlWhFFkrtkctNLHTSdqvrwWTdlvGnLQ"));
        QDesktopServices::openUrl(url);
    });
    connect(actionAdmin, &QAction::triggered, this, &MainWindow::onAdminClicked);
    connect(actionLogout, &QAction::triggered, this, [this]() {
        QSettings settings;
        settings.remove("auth");

        hide();
        RegisterDialog registerDialog(this);
        if (registerDialog.exec() != QDialog::Accepted) {
            close();
            return;
        }

        userEmail = registerDialog.registeredEmail();
        accessToken = registerDialog.accessToken();
        apiBaseUrl = registerDialog.apiBaseUrl();
        settings.setValue("auth/email", userEmail);
        settings.setValue("auth/token", accessToken);
        settings.setValue("auth/apiBaseUrl", apiBaseUrl);

        if (accountEmailLabel) {
            accountEmailLabel->setText(userEmail);
        }
        if (actionAdmin) {
            actionAdmin->setVisible(isAdminUser());
        }
        loadBuilds();
        show();
    });

    musicPlayer = new QMediaPlayer(this);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    musicOutput = new QAudioOutput(this);
    musicPlayer->setAudioOutput(musicOutput);
#endif
    musicPlayer->setSource(QUrl("qrc:/audio/LDP.mp3"));
    connect(musicPlayer, &QMediaPlayer::mediaStatusChanged, this,
            [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia && musicEnabled && musicVolume > 0) {
            musicPlayer->setPosition(0);
            musicPlayer->play();
        }
    });
    connect(musicButton, &QToolButton::clicked, this, [this]() {
        saveAudioSettings(!musicEnabled, musicVolume);
    });

    topLayout->addWidget(logoLabel);
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();

    QFrame *topActions = new QFrame(this);
    topActionsFrame = topActions;
    topActions->setObjectName("TopActions");
    topActions->setAttribute(Qt::WA_StyledBackground, true);
    topActions->setAutoFillBackground(true);
    auto *topActionsLayout = new QHBoxLayout(topActions);
    topActionsLayout->setContentsMargins(8, 6, 8, 6);
    topActionsLayout->setSpacing(6);
    topActionsLayout->addWidget(musicButton);
    topActionsLayout->addWidget(accountButton);
    topLayout->addWidget(topActions);

    rootLayout->addWidget(topBar);

    // Body
    QHBoxLayout *bodyLayout = new QHBoxLayout();
    bodyLayout->setSpacing(12);

    // Center: builds
    QFrame *cardsArea = new QFrame(this);
    cardsAreaFrame = cardsArea;
    cardsArea->setObjectName("CardsArea");
    cardsArea->setAttribute(Qt::WA_StyledBackground, true);
    cardsArea->setAutoFillBackground(true);
    QVBoxLayout *cardsLayout = new QVBoxLayout(cardsArea);
    cardsLayout->setContentsMargins(16, 16, 16, 16);
    cardsLayout->setSpacing(10);

    QHBoxLayout *cardsHeader = new QHBoxLayout();
    cardsTitleLabel = new QLabel(I18n::tr("Builds"), this);

    addBuildButton = new QPushButton("+", this);
    addBuildButton->setObjectName("AddBuildButton");
    addBuildButton->setFixedSize(36, 36);
    addBuildButton->setText("");
    addBuildButton->setIcon(solidPlusIcon());
    addBuildButton->setIconSize(QSize(14, 14));
    connect(addBuildButton, &QPushButton::clicked, this, &MainWindow::onAddBuildClicked);

    cardsHeader->addWidget(cardsTitleLabel);
    cardsHeader->addStretch();
    cardsHeader->addWidget(addBuildButton);
    cardsLayout->addLayout(cardsHeader);

    auto *filterLayout = new QHBoxLayout();
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText(I18n::tr("Search builds"));
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setMinimumHeight(30);
    tagFilter = new RoundedComboBox(this);
    tagFilter->setObjectName("TagFilter");
    tagFilter->setMinimumHeight(30);
    tagFilter->setMinimumWidth(140);
    if (auto *roundedCombo = dynamic_cast<RoundedComboBox *>(tagFilter)) {
        roundedCombo->setPopupStyle(
            "TagFilterPopup",
            R"QSS(
                QWidget#TagFilterPopup {
                    background-color: #1b2231;
                    border: 1px solid #33405a;
                    border-radius: 10px;
                }
            )QSS",
            10);
    }
    filterLayout->addWidget(searchEdit, 1);
    filterLayout->addWidget(tagFilter);
    cardsLayout->addLayout(filterLayout);

    connect(searchEdit, &QLineEdit::textChanged, this, [this]() {
        refreshBuildList();
    });
    connect(tagFilter, &QComboBox::currentIndexChanged, this, [this](int) {
        refreshBuildList();
    });

    buildListModel = new BuildListModel(this);
    buildList = new QListView(this);
    buildList->setModel(buildListModel);
    buildList->setItemDelegate(new BuildListDelegate(buildList));
    buildList->setSelectionMode(QAbstractItemView::SingleSelection);
    buildList->setSpacing(8);
    buildList->setUniformItemSizes(true);
    buildList->setLayoutMode(QListView::Batched);
    buildList->setBatchSize(30);
    buildList->setMouseTracking(true);
    buildList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    buildList->setFocusPolicy(Qt::NoFocus);
    buildList->setContextMenuPolicy(Qt::CustomContextMenu);
    if (buildList->selectionModel()) {
        connect(buildList->selectionModel(), &QItemSelectionModel::currentChanged,
                this, [this](const QModelIndex &current, const QModelIndex &) {
                    onBuildSelected(current.row());
                });
    }
    connect(buildList, &QListView::customContextMenuRequested, this, &MainWindow::onBuildContextMenu);

    cardsLayout->addWidget(buildList);

    imageManager = new QNetworkAccessManager(this);

    bodyLayout->addWidget(cardsArea, 3);

    // Right panel: actions
    QFrame *sidePanel = new QFrame(this);
    sidePanelFrame = sidePanel;
    sidePanel->setObjectName("SidePanel");
    sidePanel->setAttribute(Qt::WA_StyledBackground, true);
    sidePanel->setAutoFillBackground(true);
    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(16, 16, 16, 16);
    sideLayout->setSpacing(10);

    actionsTitleLabel = new QLabel(I18n::tr("Actions"), this);

    detailsLabel = new QLabel(I18n::tr("Select a build to see details"), this);
    detailsLabel->setWordWrap(true);
    detailsLabel->setObjectName("DetailsLabel");

    memoryLabel = new QLabel(this);
    memoryLabel->setObjectName("MemoryLabel");
    memoryLabel->setWordWrap(true);

    downloadButton = new QPushButton(I18n::tr("Download"), this);
    downloadButton->setObjectName("DownloadButton");
    downloadButton->setIcon(glyphIcon(QChar(0xE118)));
    downloadButton->setIconSize(QSize(18, 18));

    playButton = new QPushButton(I18n::tr("Play"), this);
    playButton->setObjectName("PlayButton");
    playButton->setIcon(glyphIcon(QChar(0xE768)));
    playButton->setIconSize(QSize(18, 18));

    playConsoleButton = new QPushButton(I18n::tr("Play with console"), this);
    playConsoleButton->setObjectName("PlayConsoleButton");
    playConsoleButton->setIcon(glyphIcon(QChar(0xE756)));
    playConsoleButton->setIconSize(QSize(18, 18));

    stopButton = new QPushButton(I18n::tr("Stop"), this);
    stopButton->setObjectName("StopButton");
    stopButton->setIcon(glyphIcon(QChar(0xE15B)));
    stopButton->setIconSize(QSize(18, 18));

    verifyButton = new QPushButton(I18n::tr("Verify files"), this);
    verifyButton->setObjectName("VerifyButton");
    verifyButton->setIcon(glyphIcon(QChar(0xE73E)));
    verifyButton->setIconSize(QSize(18, 18));

    removeButton = new QPushButton(I18n::tr("Delete from device"), this);
    removeButton->setObjectName("RemoveButton");
    removeButton->setIcon(glyphIcon(QChar(0xE74D)));
    removeButton->setIconSize(QSize(18, 18));

    removeListButton = new QPushButton(I18n::tr("Remove from my builds"), this);
    removeListButton->setObjectName("RemoveListButton");
    removeListButton->setIcon(glyphIcon(QChar(0xE74D)));
    removeListButton->setIconSize(QSize(18, 18));

    connect(downloadButton, &QPushButton::clicked, this, &MainWindow::onDownloadClicked);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::onPlayClicked);
    connect(playConsoleButton, &QPushButton::clicked, this, &MainWindow::onPlayConsoleClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(verifyButton, &QPushButton::clicked, this, &MainWindow::onVerifyClicked);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::onRemoveClicked);
    connect(removeListButton, &QPushButton::clicked, this, &MainWindow::onRemoveFromListClicked);

    sideLayout->addWidget(actionsTitleLabel);
    sideLayout->addWidget(detailsLabel);
    sideLayout->addWidget(memoryLabel);
    sideLayout->addStretch();
    sideLayout->addWidget(downloadButton);
    sideLayout->addWidget(playButton);
    sideLayout->addWidget(playConsoleButton);
    sideLayout->addWidget(verifyButton);
    sideLayout->addWidget(stopButton);
    sideLayout->addWidget(removeButton);
    sideLayout->addWidget(removeListButton);

    bodyLayout->addWidget(sidePanel, 1);

    rootLayout->addLayout(bodyLayout);
    updateHeaderFonts();

      // Styles
      setStyleSheet(R"QSS(
          QMainWindow {
              background: transparent;
              color: #e8ecf2;
          }
            #TopBar {
                background: transparent;
                border: 1px solid #2d3c55;
                border-radius: 14px;
            }
          #MemoryLabel { color: #9fb0d6; }
          #AccountButton {
              color: #d3ddf5;
              background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                  stop:0 #2a3447, stop:1 #242d3e);
              border: 1px solid #3b4a66;
              border-radius: 10px;
              padding: 6px 10px;
          }
          #MusicButton {
              color: #d3ddf5;
              background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                  stop:0 #2a3447, stop:1 #242d3e);
              border: 1px solid #3b4a66;
              border-radius: 10px;
              padding: 6px 10px;
              margin-right: 6px;
          }
          #AccountButton::menu-indicator { image: none; }
          #AccountButton:hover {
              background-color: #2f3a4f;
          }
          #MusicButton:hover {
              background-color: #2f3a4f;
          }
            #CardsArea, #SidePanel {
                background: transparent;
                border: 1px solid #2d3c55;
                border-radius: 14px;
            }
            #TopActions {
                background: transparent;
                border: 1px solid #2d3c55;
                border-radius: 12px;
            }
          QListView {
              background: transparent;
              border: none;
          }
          QListView::item {
              background: transparent;
              border: none;
              padding: 0;
          }
          QListView::item:selected {
              background: transparent;
              border: none;
          }
          QListView::item:focus {
              outline: none;
          }
            QLabel#DetailsLabel {
                color: #c9d3ee;
            }
            QLineEdit {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #222c3e, stop:1 #1d2737);
                border: 1px solid #2f3a4f;
                border-radius: 8px;
                padding: 6px 10px;
                color: #e8ecf2;
            }
            QLineEdit:focus {
                border: 1px solid #5aa9e6;
            }
            QComboBox#TagFilter {
                background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                    stop:0 #222c3e, stop:1 #1d2737);
                border: 1px solid #2f3a4f;
                border-radius: 8px;
                padding: 4px 26px 4px 10px;
                color: #e8ecf2;
            }
            QComboBox#TagFilter::drop-down {
                subcontrol-origin: border;
                subcontrol-position: top right;
                width: 20px;
                border-left: 1px solid #2f3a4f;
                background-color: #202635;
                border-top-right-radius: 8px;
                border-bottom-right-radius: 8px;
            }
            QComboBox#TagFilter::down-arrow {
                image: url(":/icons/chevron_down.svg");
                width: 8px;
                height: 6px;
            }
            QComboBox#TagFilter QAbstractItemView {
                background: #1b2435;
                border: 1px solid #33405a;
                border-radius: 10px;
                padding: 4px;
                color: #e8ecf2;
                outline: 0;
            }
            QComboBox#TagFilter QAbstractItemView::item {
                padding: 6px 10px;
                border-radius: 8px;
            }
            QComboBox#TagFilter QAbstractItemView::item:selected {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #2b3b55, stop:1 #24324a);
                color: #e8ecf2;
            }
            QPushButton {
              background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                  stop:0 #2f3b52, stop:1 #263246);
              border: 1px solid #3a4763;
              border-radius: 10px;
              padding: 8px 12px;
              color: #e8ecf2;
          }
          QPushButton:hover {
              background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                  stop:0 #36455f, stop:1 #2b3a51);
          }
          QPushButton:pressed {
              background-color: #28344a;
          }
          QPushButton:disabled {
              color: #6f7994;
              background-color: #232a3a;
              border-color: #2a3144;
          }
          QPushButton#PlayButton {
              background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                  stop:0 #2b6be6, stop:1 #234bb8);
              border-color: #3e7bf0;
              font-weight: 600;
          }
          QPushButton#PlayButton:hover {
              background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                  stop:0 #3373ef, stop:1 #2751c2);
              border-color: #4b86ff;
          }
          QPushButton#PlayButton:pressed {
              background-color: #2344a0;
          }
          QPushButton#RemoveButton,
          QPushButton#RemoveListButton {
              background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                  stop:0 #5a1f2b, stop:1 #3b141e);
              border-color: #7a2736;
          }
          QPushButton#RemoveButton:hover,
          QPushButton#RemoveListButton:hover {
              background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                  stop:0 #6a2432, stop:1 #451725);
          }
          QPushButton#RemoveButton:pressed,
          QPushButton#RemoveListButton:pressed {
              background-color: #35101c;
          }
            QPushButton#AddBuildButton {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 #24324a, stop:1 #1f2a3c);
                color: #d9e3ff;
                font-weight: bold;
                border-radius: 18px;
                border: 1px solid #2d3c55;
                padding: 0px;
            }
            QPushButton#AddBuildButton:hover {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 #2a3a55, stop:1 #24344d);
                border: 1px solid #395070;
            }
          QMenu {
              background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                  stop:0 #1b2435, stop:1 #141b28);
              border: 1px solid #33405a;
              border-radius: 12px;
              padding: 6px;
          }
          QMenu::item {
              padding: 6px 12px;
              border-radius: 8px;
              color: #e8ecf2;
          }
          QMenu::item:selected {
              background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                  stop:0 #2b3b55, stop:1 #24324a);
          }
          QToolTip {
              background-color: #1b2231;
              color: #e8ecf2;
              border: 1px solid #2d3850;
              padding: 4px 6px;
              border-radius: 6px;
          }
          QScrollBar:vertical {
              background: transparent;
              width: 8px;
              margin: 6px 0 6px 0;
          }
          QScrollBar::handle:vertical {
              background: #2f3a4f;
              min-height: 30px;
              border-radius: 4px;
          }
          QScrollBar::handle:vertical:hover {
              background: #3a4760;
          }
          QScrollBar::add-line:vertical,
          QScrollBar::sub-line:vertical {
              height: 0px;
              subcontrol-origin: margin;
          }
          QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
              background: transparent;
          }
      )QSS");
}

void MainWindow::applyTranslations()
{
    setWindowTitle(I18n::tr("LD Launcher"));
    if (titleLabel) {
        titleLabel->setText(I18n::tr("LD Launcher"));
    }
    if (cardsTitleLabel) {
        cardsTitleLabel->setText(I18n::tr("Builds"));
    }
    if (musicButton) {
        musicButton->setToolTip(I18n::tr("Music"));
    }
    if (actionsTitleLabel) {
        actionsTitleLabel->setText(I18n::tr("Actions"));
    }
    if (actionSettings) {
        actionSettings->setText(I18n::tr("Settings"));
    }
    if (actionSupport) {
        actionSupport->setText(I18n::tr("Support"));
    }
    if (actionAdmin) {
        actionAdmin->setText(I18n::tr("Admin"));
    }
    if (actionLogout) {
        actionLogout->setText(I18n::tr("Log out"));
    }
    if (accountHeaderLabel) {
        accountHeaderLabel->setText(I18n::tr("Account"));
    }
    if (accountEmailLabel) {
        accountEmailLabel->setText(userEmail);
    }
    if (downloadButton) {
        downloadButton->setText(I18n::tr("Download"));
    }
    if (playButton) {
        playButton->setText(I18n::tr("Play"));
    }
    if (playConsoleButton) {
        playConsoleButton->setText(I18n::tr("Play with console"));
    }
    if (stopButton) {
        stopButton->setText(I18n::tr("Stop"));
    }
    if (verifyButton) {
        verifyButton->setText(I18n::tr("Verify files"));
    }
    if (removeButton) {
        removeButton->setText(I18n::tr("Delete from device"));
    }
    if (removeListButton) {
        removeListButton->setText(I18n::tr("Remove from my builds"));
    }
    updateMemoryLabel();
    updateDetails(currentBuildIndex());
}

bool MainWindow::isAdminUser() const
{
    QString email = userEmail.trimmed();
    if (email.isEmpty()) {
        QSettings settings;
        email = settings.value("auth/email").toString().trimmed();
    }
    return email.compare("admin@ldp.com", Qt::CaseInsensitive) == 0;
}

bool MainWindow::isBuildLocked(const BuildInfo &build) const
{
    return build.locked && !build.isFree && !isAdminUser();
}

int MainWindow::buildIndexFromList(int listIndex) const
{
    if (!buildListModel) {
        return -1;
    }
    return buildListModel->buildIndexAtRow(listIndex);
}

int MainWindow::currentBuildIndex() const
{
    const int row = buildList ? buildList->currentIndex().row() : -1;
    return buildIndexFromList(row);
}

void MainWindow::loadBuilds()
{
    ConfigManager config;
    builds = config.loadBuilds();
    if (isAdminUser()) {
        for (auto &build : builds) {
            build.locked = false;
        }
    }
    syncBuildsFromServer();
    refreshBuildList();
}

void MainWindow::syncBuildsFromServer()
{
    if (apiBaseUrl.isEmpty() || accessToken.isEmpty() || builds.empty()) {
        return;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(apiBaseUrl + "/builds"));
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray body = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
        reply->deleteLater();
        return;
    }
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) {
        return;
    }

    QHash<QString, QJsonObject> byId;
    for (const auto &val : doc.array()) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject obj = val.toObject();
        byId.insert(obj.value("id").toString(), obj);
    }

    for (auto &build : builds) {
        const QString id = QString::fromStdString(build.id);
        if (!byId.contains(id)) {
            continue;
        }
        QJsonObject obj = byId.value(id);
        build.name = obj.value("name").toString().toUtf8().toStdString();
        build.description = obj.value("description").toString().toUtf8().toStdString();
        build.minecraftVersion = obj.value("minecraftVersion").toString().toStdString();
        build.loader = obj.value("loader").toString().toStdString();
        build.javaVersion = obj.value("javaVersion").toInt(build.javaVersion);
        build.sizeBytes = static_cast<long long>(obj.value("sizeBytes").toDouble(build.sizeBytes));
        build.checksum = obj.value("checksum").toString().toStdString();
        build.imageUrl = obj.value("imageUrl").toString().toStdString();
        build.githubRepo = obj.value("githubRepo").toString().toStdString();
        build.githubAsset = obj.value("githubAsset").toString().toStdString();
        build.isFree = obj.value("isFree").toBool(build.isFree);
        build.priceCents = obj.value("priceCents").toInt(build.priceCents);
        build.locked = obj.value("locked").toBool(build.locked);
        if (isAdminUser()) {
            build.locked = false;
        }

        if (obj.contains("tags") && obj.value("tags").isArray()) {
            QStringList tags;
            for (const auto &tagVal : obj.value("tags").toArray()) {
                tags << tagVal.toString();
            }
            build.tagsCsv = tags.join(',').toStdString();
        } else if (build.tagsCsv.empty()) {
            build.tagsCsv = obj.value("tags").toString().toStdString();
        }
    }
}

void MainWindow::refreshBuildList()
{
    QString selectedId;
    int selectedList = buildList ? buildList->currentIndex().row() : -1;
    int selectedBuild = buildIndexFromList(selectedList);
    if (selectedBuild >= 0 && selectedBuild < static_cast<int>(builds.size())) {
        selectedId = QString::fromStdString(builds[static_cast<size_t>(selectedBuild)].id);
    }

    const QString searchText = searchEdit
        ? searchEdit->text().trimmed().toLower()
        : QString();
    QString tagValue = tagFilter ? tagFilter->currentData().toString().trimmed().toLower() : QString();

    if (!buildList || !buildListModel) {
        return;
    }

    if (tagFilter) {
        QSignalBlocker blocker(tagFilter);
        const QString currentTag = tagFilter->currentData().toString();
        tagFilter->clear();
        tagFilter->addItem(I18n::tr("All tags"), "");
        QSet<QString> tags;
        for (const auto &build : builds) {
            const QString tagsCsv = QString::fromStdString(build.tagsCsv);
            const QStringList pieces = tagsCsv.split(',', Qt::SkipEmptyParts);
            for (const auto &piece : pieces) {
                const QString tag = piece.trimmed();
                if (!tag.isEmpty()) {
                    tags.insert(tag);
                }
            }
        }
        QStringList sorted = tags.values();
        std::sort(sorted.begin(), sorted.end(), [](const QString &a, const QString &b) {
            return a.toLower() < b.toLower();
        });
        for (const auto &tag : sorted) {
            tagFilter->addItem(tag, tag);
        }
        int idx = tagFilter->findData(currentTag);
        if (idx >= 0) {
            tagFilter->setCurrentIndex(idx);
        }
        tagValue = tagFilter->currentData().toString().trimmed().toLower();
    }

    QVector<BuildListItem> items;
    items.reserve(static_cast<int>(builds.size()));

    for (int i = 0; i < static_cast<int>(builds.size()); ++i) {
        const auto &build = builds[static_cast<size_t>(i)];
        const QString name = QString::fromUtf8(build.name.c_str());
        const QString desc = QString::fromUtf8(build.description.c_str());
        const QString loader = QString::fromStdString(build.loader);
        const QString version = QString::fromStdString(build.minecraftVersion);
        const QString tagsCsv = QString::fromStdString(build.tagsCsv);
        const QStringList tags = tagsCsv.split(',', Qt::SkipEmptyParts);

        bool tagMatch = true;
        if (!tagValue.isEmpty()) {
            tagMatch = false;
            for (const auto &tag : tags) {
                if (tag.trimmed().toLower() == tagValue) {
                    tagMatch = true;
                    break;
                }
            }
        }

        bool searchMatch = true;
        if (!searchText.isEmpty()) {
            const QString tagsLower = tagsCsv.toLower();
            searchMatch =
                name.toLower().contains(searchText)
                || desc.toLower().contains(searchText)
                || loader.toLower().contains(searchText)
                || version.toLower().contains(searchText)
                || tagsLower.contains(searchText);
        }

        if (!tagMatch || !searchMatch) {
            continue;
        }

        BuildListItem entry;
        entry.build = build;
        entry.buildIndex = i;
        entry.locked = isBuildLocked(build);
        const QString id = QString::fromStdString(build.id);
        if (buildImageCache.contains(id)) {
            entry.image = buildImageCache.value(id);
        }
        items.push_back(entry);
    }

    buildListModel->setItems(items);

    if (imageManager) {
        for (int row = 0; row < buildListModel->rowCount(); ++row) {
            const BuildListItem *entry = buildListModel->itemAt(row);
            if (!entry || !entry->image.isNull()) {
                continue;
            }
            const QString id = QString::fromStdString(entry->build.id);
            if (pendingImageLoads.contains(id)) {
                continue;
            }
            QString img = resolveImageUrl(apiBaseUrl, entry->build);
            if (img.isEmpty()) {
                continue;
            }
            pendingImageLoads.insert(id);
            QNetworkRequest req{QUrl(img)};
            if (!accessToken.isEmpty()) {
                req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
            }
            QNetworkReply *reply = imageManager->get(req);
            connect(reply, &QNetworkReply::finished, this, [this, reply, id]() {
                QByteArray data = reply->readAll();
                QPixmap pix;
                pix.loadFromData(data);
                if (!pix.isNull()) {
                    buildImageCache.insert(id, pix);
                    if (buildListModel) {
                        int row = buildListModel->rowForBuildId(id);
                        if (row >= 0) {
                            buildListModel->setImageForRow(row, pix);
                        }
                    }
                }
                pendingImageLoads.remove(id);
                reply->deleteLater();
            });
        }
    }

    if (!selectedId.isEmpty()) {
        int row = buildListModel->rowForBuildId(selectedId);
        if (row >= 0) {
            buildList->setCurrentIndex(buildListModel->index(row, 0));
            return;
        }
    }

    updateDetails(-1);
}

void MainWindow::updateDetails(int index)
{
    bool hasSelection = index >= 0 && index < static_cast<int>(builds.size());
    downloadButton->setEnabled(hasSelection);
    const bool running = gameProcess && gameProcess->state() != QProcess::NotRunning;
    playButton->setEnabled(hasSelection && !running);
    if (playConsoleButton) {
        playConsoleButton->setEnabled(hasSelection && !running);
    }
    if (stopButton) {
        stopButton->setEnabled(running);
    }
    removeButton->setEnabled(hasSelection);

    bool showDownload = false;
    bool showPlay = false;
    bool showPlayConsole = false;
    bool showStop = running;
    bool showVerify = false;
    bool showRemove = false;
    bool showRemoveList = false;

    if (!hasSelection) {
        detailsLabel->setText(I18n::tr("Select a build to see details"));
        downloadButton->setText(I18n::tr("Download"));
        playButton->setEnabled(false);
        if (playConsoleButton) {
            playConsoleButton->setEnabled(false);
        }
    } else {
        const auto &build = builds[static_cast<size_t>(index)];
        const QString appDirPath = QCoreApplication::applicationDirPath();
        const bool installed = isBuildInstalled(build, appDirPath);
        const QString rootPath = resolveBuildRoot(build, appDirPath);
        const QString name = QString::fromUtf8(build.name.c_str());
        const QString desc = QString::fromUtf8(build.description.c_str());
        const QString priceInfo = priceTextForBuild(build, isBuildLocked(build));
        const QString details = I18n::tr(
            "Name: %1\nMinecraft: %2\nLoader: %3\nJava: %4\nSize: %5 MB\nPrice: %6\n\n%7")
            .arg(name)
            .arg(QString::fromStdString(build.minecraftVersion))
            .arg(build.loader.empty() ? I18n::tr("Unknown") : QString::fromStdString(build.loader))
            .arg(build.javaVersion)
            .arg(buildSizeText(build))
            .arg(priceInfo)
            .arg(desc);
        detailsLabel->setText(details);

        const bool outdated = installed && !rootPath.isEmpty() && isBuildOutdated(build, rootPath);
        const bool locked = isBuildLocked(build);
        showDownload = locked ? !installed : (!installed || outdated);
        showPlay = installed && !locked;
        showPlayConsole = installed && !locked;
        showStop = installed || running;
        showVerify = installed;
        showRemove = installed;
        showRemoveList = !installed;

        if (locked) {
            downloadButton->setText(I18n::tr("Buy"));
            downloadButton->setEnabled(!installed);
            playButton->setEnabled(false);
            if (playConsoleButton) {
                playConsoleButton->setEnabled(false);
            }
        } else {
            downloadButton->setText(outdated ? I18n::tr("Update") : I18n::tr("Download"));
            downloadButton->setEnabled(!installed || outdated);
            playButton->setEnabled(installed && !running);
            if (playConsoleButton) {
                playConsoleButton->setEnabled(installed && !running);
            }
        }
        if (stopButton) {
            stopButton->setEnabled(running);
        }
        if (verifyButton) {
            verifyButton->setEnabled(installed && !running);
        }
    }

    animateActionVisibility(removeListButton, showRemoveList);
    animateActionVisibility(removeButton, showRemove);
    animateActionVisibility(stopButton, showStop);
    animateActionVisibility(verifyButton, showVerify);
    animateActionVisibility(playConsoleButton, showPlayConsole);
    animateActionVisibility(playButton, showPlay);
    animateActionVisibility(downloadButton, showDownload);
}

int MainWindow::totalRamMb() const
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<int>(status.ullTotalPhys / (1024 * 1024));
    }
#endif
    return 4096;
}

int MainWindow::availableRamMb() const
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<int>(status.ullAvailPhys / (1024 * 1024));
    }
#endif
    return totalRamMb();
}

void MainWindow::loadMemorySettings()
{
    int totalMb = totalRamMb();
    int minMb = 256;

    QSettings settings;
    memoryAuto = settings.value("java/auto", true).toBool();
    int xms = settings.value("java/xms_mb", 0).toInt();
    int xmx = settings.value("java/xmx_mb", 0).toInt();

    if (xms <= 0 || xmx <= 0) {
        const auto rec = recommendMemory(totalMb);
        xms = rec.xms;
        xmx = rec.xmx;
    }

    if (memoryAuto) {
        const auto rec = recommendMemory(totalMb);
        xms = rec.xms;
        xmx = rec.xmx;
    }

    if (xms < minMb) xms = minMb;
    if (xmx < minMb) xmx = minMb;
    if (xms > totalMb) xms = totalMb;
    if (xmx > totalMb) xmx = totalMb;
    if (xms > xmx) xmx = xms;

    memoryXmsMb = xms;
    memoryXmxMb = xmx;

    settings.setValue("java/auto", memoryAuto);
    settings.setValue("java/xms_mb", memoryXmsMb);
    settings.setValue("java/xmx_mb", memoryXmxMb);
    updateMemoryLabel();
}

void MainWindow::loadPerformanceSettings()
{
    QSettings settings;
    perfJvmProfile = settings.value("perf/jvm_profile", true).toBool();
    extraJvmArgs = settings.value("perf/extra_jvm_args", "").toString();
}

void MainWindow::loadDownloadSettings()
{
    QSettings settings;
    downloadLimitKbps = settings.value("download/limit_kbps", 0).toInt();
}

void MainWindow::loadAudioSettings()
{
    QSettings settings;
    musicEnabled = settings.value("audio/enabled", true).toBool();
    musicVolume = settings.value("audio/volume", 60).toInt();
    if (musicVolume < 0) musicVolume = 0;
    if (musicVolume > 100) musicVolume = 100;
}

void MainWindow::loadUiSettings()
{
    QSettings settings;
    animationsEnabled = settings.value("ui/animations", true).toBool();
    animateBackground = settings.value("ui/animate_background", true).toBool();
    const QString fontFamily = settings.value("ui/font_family").toString();
    if (!fontFamily.isEmpty()) {
        applyUiFont(fontFamily);
    }
}

void MainWindow::saveMemorySettings(int xmsMb, int xmxMb, bool autoMemory)
{
    if (xmsMb > xmxMb) {
        xmxMb = xmsMb;
    }
    memoryAuto = autoMemory;
    if (memoryAuto) {
        const auto rec = recommendMemory(totalRamMb());
        memoryXmsMb = rec.xms;
        memoryXmxMb = rec.xmx;
    } else {
        memoryXmsMb = xmsMb;
        memoryXmxMb = xmxMb;
    }
    QSettings settings;
    settings.setValue("java/auto", memoryAuto);
    settings.setValue("java/xms_mb", memoryXmsMb);
    settings.setValue("java/xmx_mb", memoryXmxMb);
    updateMemoryLabel();
}

void MainWindow::savePerformanceSettings(bool perfProfile, const QString &extraArgs)
{
    perfJvmProfile = perfProfile;
    extraJvmArgs = extraArgs;
    QSettings settings;
    settings.setValue("perf/jvm_profile", perfJvmProfile);
    settings.setValue("perf/extra_jvm_args", extraJvmArgs);
}

void MainWindow::saveDownloadSettings(int limitKbps)
{
    downloadLimitKbps = std::max(0, limitKbps);
    QSettings settings;
    settings.setValue("download/limit_kbps", downloadLimitKbps);
}

void MainWindow::saveAudioSettings(bool enabled, int volume)
{
    musicEnabled = enabled;
    musicVolume = std::clamp(volume, 0, 100);
    QSettings settings;
    settings.setValue("audio/enabled", musicEnabled);
    settings.setValue("audio/volume", musicVolume);
    applyAudioSettings();
}

void MainWindow::saveUiSettings(bool enabled, bool animateBackgroundValue, const QString &fontFamily)
{
    animationsEnabled = enabled;
    animateBackground = animateBackgroundValue;
    QSettings settings;
    settings.setValue("ui/animations", animationsEnabled);
    settings.setValue("ui/animate_background", animateBackground);
    if (!fontFamily.isEmpty()) {
        settings.setValue("ui/font_family", fontFamily);
        applyUiFont(fontFamily);
    }
}

void MainWindow::applyUiFont(const QString &fontFamily)
{
    if (fontFamily.isEmpty()) {
        return;
    }
    QFont font = QApplication::font();
    if (font.family() == fontFamily) {
        return;
    }
    font.setFamily(fontFamily);
    QApplication::setFont(font);
    updateHeaderFonts();
    if (detailsLabel) {
        detailsLabel->setFont(font);
    }
    if (memoryLabel) {
        memoryLabel->setFont(font);
    }
    if (searchEdit) {
        searchEdit->setFont(font);
    }
    if (tagFilter) {
        tagFilter->setFont(font);
        if (auto *view = tagFilter->view()) {
            view->setFont(font);
        }
    }
    if (downloadButton) {
        downloadButton->setFont(font);
    }
    if (playButton) {
        playButton->setFont(font);
    }
    if (playConsoleButton) {
        playConsoleButton->setFont(font);
    }
    if (verifyButton) {
        verifyButton->setFont(font);
    }
    if (stopButton) {
        stopButton->setFont(font);
    }
    if (removeButton) {
        removeButton->setFont(font);
    }
    if (removeListButton) {
        removeListButton->setFont(font);
    }
    if (buildList) {
        buildList->viewport()->update();
    }
}

void MainWindow::updateHeaderFonts()
{
    QFont base = QApplication::font();
    if (titleLabel) {
        QFont f = base;
        f.setPointSize(16);
        f.setBold(true);
        titleLabel->setFont(f);
    }
    if (cardsTitleLabel) {
        QFont f = base;
        f.setPointSize(12);
        f.setBold(true);
        cardsTitleLabel->setFont(f);
    }
    if (actionsTitleLabel) {
        QFont f = base;
        f.setPointSize(12);
        f.setBold(true);
        actionsTitleLabel->setFont(f);
    }
}

void MainWindow::updateBackgroundAnimation()
{
    if (!backgroundWidget) {
        return;
    }
    backgroundWidget->setAnimated(animateBackground);
    if (!backgroundTimer) {
        backgroundTimer = new QTimer(this);
        backgroundTimer->setInterval(33);
        connect(backgroundTimer, &QTimer::timeout, this, [this]() {
            backgroundPhase += 0.05;
            const double cycle = 6.283185307 * 10.0;
            if (backgroundPhase > cycle) {
                backgroundPhase -= cycle;
            }
            if (backgroundWidget) {
                backgroundWidget->setPhase(backgroundPhase);
            }
        });
    }
    if (animateBackground) {
        if (!backgroundTimer->isActive()) {
            backgroundTimer->start();
        }
    } else {
        if (backgroundTimer->isActive()) {
            backgroundTimer->stop();
        }
        backgroundPhase = 0.0;
        backgroundWidget->setPhase(backgroundPhase);
    }
}

void MainWindow::updateMusicIcon()
{
    if (!musicButton) {
        return;
    }
    if (!musicEnabled || musicVolume <= 0) {
        musicButton->setIcon(glyphIcon(QChar(0xE74F)));
        return;
    }
    if (musicVolume < 30) {
        musicButton->setIcon(glyphIcon(QChar(0xE993)));
    } else if (musicVolume < 70) {
        musicButton->setIcon(glyphIcon(QChar(0xE994)));
    } else {
        musicButton->setIcon(glyphIcon(QChar(0xE995)));
    }
}

void MainWindow::applyAudioSettings()
{
    if (!musicPlayer) {
        return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (musicOutput) {
        musicOutput->setVolume(musicVolume / 100.0);
    }
#else
    musicPlayer->setVolume(musicVolume);
#endif
    updateMusicIcon();
    if (!musicEnabled || musicVolume <= 0) {
        musicPlayer->pause();
        return;
    }
    musicPlayer->play();
}

void MainWindow::updateMemoryLabel()
{
    if (!memoryLabel) {
        return;
    }
    int totalMb = totalRamMb();
    if (memoryAuto) {
        int availMb = availableRamMb();
        const auto rec = recommendMemoryDynamic(totalMb, availMb);
        memoryLabel->setText(I18n::tr("Memory: Auto (Xms %1 MB | Xmx %2 MB) (Total %3 MB)")
            .arg(rec.xms).arg(rec.xmx).arg(totalMb));
    } else {
        memoryLabel->setText(I18n::tr("Memory: Xms %1 MB | Xmx %2 MB (Total %3 MB)")
            .arg(memoryXmsMb).arg(memoryXmxMb).arg(totalMb));
    }
}

void MainWindow::onBuildSelected(int index)
{
    updateDetails(buildIndexFromList(index));
}

void MainWindow::onBuildContextMenu(const QPoint &pos)
{
    const QModelIndex idx = buildList->indexAt(pos);
    if (!idx.isValid()) {
        return;
    }

    buildList->setCurrentIndex(idx);
    int listIndex = buildList->currentIndex().row();
    int index = buildIndexFromList(listIndex);
    if (index < 0 || index >= static_cast<int>(builds.size())) {
        return;
    }

    QMenu menu(this);
    menu.setAttribute(Qt::WA_StyledBackground, true);
    menu.installEventFilter(new PopupRounder(10, &menu));
    menu.setStyleSheet(R"QSS(
        QMenu {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #1b2435, stop:1 #141b28);
            border: 1px solid #33405a;
            border-radius: 12px;
            min-width: 180px;
            padding: 6px;
        }
        QMenu::item {
            padding: 8px 16px 8px 44px;
            min-width: 220px;
            margin: 0px;
            color: #e8ecf2;
            border-radius: 8px;
        }
        QMenu::icon {
            left: 16px;
        }
        QMenu::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #2b3b55, stop:1 #24324a);
            color: #e8ecf2;
            border: 1px solid #5aa9e6;
            border-radius: 8px;
        }
        QMenu::separator {
            height: 1px;
            background-color: #2a3447;
            margin: 4px 8px;
        }
        QMenu::item:disabled {
            color: #6f7994;
        }
    )QSS");
    const auto &build = builds[static_cast<size_t>(index)];
    bool locked = isBuildLocked(build);
    QAction *download = menu.addAction(
        locked ? glyphIcon(QChar(0xE7BF)) : glyphIcon(QChar(0xE896)),
        locked ? I18n::tr("Buy") : I18n::tr("Download"));
    QAction *play = menu.addAction(glyphIcon(QChar(0xE768)), I18n::tr("Play"));
    QAction *playConsole = menu.addAction(glyphIcon(QChar(0xE756)), I18n::tr("Play with console"));
    QAction *stop = menu.addAction(glyphIcon(QChar(0xE71A)), I18n::tr("Stop"));
    QAction *verify = menu.addAction(glyphIcon(QChar(0xE946)), I18n::tr("Verify files"));
    QAction *removeDevice = menu.addAction(glyphIcon(QChar(0xE74D)), I18n::tr("Delete from device"));
    QAction *removeList = menu.addAction(glyphIcon(QChar(0xE8D5)), I18n::tr("Remove from my builds"));
    const bool installed = isBuildInstalled(build, QCoreApplication::applicationDirPath());
    download->setVisible(!installed);
    play->setVisible(installed && !locked);
    playConsole->setVisible(installed && !locked);
    const bool running = gameProcess && gameProcess->state() != QProcess::NotRunning;
    play->setEnabled(installed && !running);
    playConsole->setEnabled(installed && !running);
    stop->setVisible(running);
    stop->setEnabled(running);
    verify->setVisible(installed);
    verify->setEnabled(installed && !running);
    removeDevice->setVisible(installed);
    removeList->setVisible(!installed);

    QAction *chosen = menu.exec(buildList->viewport()->mapToGlobal(pos));
    if (chosen == download) {
        onDownloadClicked();
    } else if (chosen == play) {
        onPlayClicked();
    } else if (chosen == playConsole) {
        onPlayConsoleClicked();
    } else if (chosen == stop) {
        onStopClicked();
    } else if (chosen == verify) {
        onVerifyClicked();
    } else if (chosen == removeDevice) {
        onRemoveClicked();
    } else if (chosen == removeList) {
        onRemoveFromListClicked();
    }
}

void MainWindow::onAddBuildClicked()
{
    QMenu menu(this);
    QAction *fromCatalog = menu.addAction(glyphIcon(QChar(0xE8B7)), I18n::tr("Add Build"));
    QAction *importBuild = menu.addAction(glyphIcon(QChar(0xE8D6)), I18n::tr("Import build"));

    const QPoint pos = addBuildButton
        ? addBuildButton->mapToGlobal(QPoint(0, addBuildButton->height()))
        : QCursor::pos();
    QAction *chosen = menu.exec(pos);
    if (!chosen) {
        return;
    }
    if (chosen == importBuild) {
        onImportBuildClicked();
        return;
    }

    CatalogDialog dialog(apiBaseUrl, accessToken, isAdminUser(), this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    BuildInfo build = dialog.selectedBuild();
    if (build.id.empty()) {
        QMessageBox::warning(this, I18n::tr("Build Catalog"), I18n::tr("Build ID is missing."));
        return;
    }

    ConfigManager config;
    config.saveBuild(build);
    loadBuilds();
}

void MainWindow::onSettingsClicked()
{
    const bool prevMusicEnabled = musicEnabled;
    const int prevMusicVolume = musicVolume;
    QSettings uiSettings;
    const QString currentFont = uiSettings.value("ui/font_family", QApplication::font().family()).toString();

    SettingsDialog dialog(totalRamMb(),
                          memoryXmsMb,
                          memoryXmxMb,
                          memoryAuto,
                          perfJvmProfile,
                          extraJvmArgs,
                          downloadLimitKbps,
                          musicEnabled,
                          musicVolume,
                          animationsEnabled,
                          animateBackground,
                          availableRamMb(),
                          I18n::language(),
                          currentFont,
                          this);
    dialog.setMusicPreviewCallback([this](bool enabled, int volume) {
        musicEnabled = enabled;
        musicVolume = std::clamp(volume, 0, 100);
        applyAudioSettings();
    });
    if (dialog.exec() != QDialog::Accepted) {
        musicEnabled = prevMusicEnabled;
        musicVolume = prevMusicVolume;
        applyAudioSettings();
        return;
    }
    saveMemorySettings(dialog.xmsMb(), dialog.xmxMb(), dialog.autoMemoryEnabled());
    savePerformanceSettings(dialog.perfJvmProfileEnabled(),
                            dialog.extraJvmArgs());
    saveDownloadSettings(dialog.downloadLimitKbps());
    saveAudioSettings(dialog.musicEnabled(), dialog.musicVolume());
    saveUiSettings(dialog.animationsEnabled(),
                   dialog.animateBackgroundEnabled(),
                   dialog.selectedFontFamily());
    updateBackgroundAnimation();

    const QString newLang = dialog.selectedLanguage();
    if (!newLang.isEmpty() && newLang != I18n::language()) {
        QSettings uiSettings;
        uiSettings.setValue("ui/lang", newLang);
        I18n::setLanguage(newLang);
        int currentRow = buildList ? buildList->currentIndex().row() : -1;
        applyTranslations();
        refreshBuildList();
        if (buildListModel && currentRow >= 0 && currentRow < buildListModel->rowCount()) {
            buildList->setCurrentIndex(buildListModel->index(currentRow, 0));
        }
    }
}

void MainWindow::onAdminClicked()
{
    if (!isAdminUser()) {
        QMessageBox::warning(this, I18n::tr("Admin"), I18n::tr("Access denied. Please log in and try again."));
        return;
    }
    BuildInfo *current = nullptr;
    int index = currentBuildIndex();
    if (index >= 0 && index < static_cast<int>(builds.size())) {
        current = &builds[static_cast<size_t>(index)];
    }
    AdminUpdateDialog dialog(apiBaseUrl, accessToken, current, this);
    dialog.exec();
    syncBuildsFromServer();
    refreshBuildList();
}

void MainWindow::onDownloadClicked()
{
    int index = currentBuildIndex();
    if (index < 0 || index >= static_cast<int>(builds.size())) {
        QMessageBox::warning(this, I18n::tr("Download"), I18n::tr("Select a build first."));
        return;
    }

    const QString selectedId = QString::fromStdString(builds[static_cast<size_t>(index)].id);
    syncBuildsFromServer();
    refreshBuildList();

    int updatedIndex = -1;
    if (!selectedId.isEmpty()) {
        for (int i = 0; i < static_cast<int>(builds.size()); ++i) {
            if (QString::fromStdString(builds[static_cast<size_t>(i)].id) == selectedId) {
                updatedIndex = i;
                break;
            }
        }
    }
    if (updatedIndex < 0 || updatedIndex >= static_cast<int>(builds.size())) {
        QMessageBox::warning(this, I18n::tr("Download"), I18n::tr("Select a build first."));
        return;
    }
    const auto build = builds[static_cast<size_t>(updatedIndex)];
    if (isBuildLocked(build)) {
        QString paymentError;
        const QString checkoutUrl = fetchCheckoutUrl(apiBaseUrl, accessToken,
                                                     QString::fromStdString(build.id),
                                                     &paymentError);
        if (checkoutUrl.isEmpty()) {
            QMessageBox::warning(this, I18n::tr("Buy"),
                                 paymentError.isEmpty() ? I18n::tr("Payment failed.") : paymentError);
            return;
        }
        QDesktopServices::openUrl(QUrl(checkoutUrl));
        return;
    }
    QString downloadError;
    const QString downloadUrl = fetchDownloadUrl(apiBaseUrl, accessToken,
                                                 QString::fromStdString(build.id),
                                                 &downloadError);
    if (downloadUrl.isEmpty()) {
        QMessageBox::warning(this, I18n::tr("Download"),
                             downloadError.isEmpty() ? I18n::tr("Download URL is missing.") : downloadError);
        return;
    }

      const QString javaUrl = "https://cdn.azul.com/zulu/bin/zulu17.62.17-ca-jdk17.0.17-win_x64.zip";
    const qint64 buildExpectedSize = build.sizeBytes > 0 ? static_cast<qint64>(build.sizeBytes) : -1;

    QDir appDir(QCoreApplication::applicationDirPath());
    const QString folderName = safeFolderName(QString::fromUtf8(build.name.c_str()));
    const QString folderPath = appDir.filePath(folderName);
    QDir().mkpath(folderPath);
    QDir buildDir(folderPath);

    const QString javaPath = buildDir.filePath("java17.zip");
    const QString folderPathCopy = buildDir.path();
    const QString minecraftDir = buildDir.filePath("minecraft");
    QString buildFileName = QString::fromStdString(build.id);
    if (!buildFileName.endsWith(".zip")) {
        buildFileName += ".zip";
    }
    const QString buildPath = buildDir.filePath(buildFileName);

    QUrl apiUrl(apiBaseUrl);
    QUrl fileUrl(downloadUrl);
    QString authHeader;
    if (fileUrl.isValid()
        && apiUrl.isValid()
        && fileUrl.scheme() == apiUrl.scheme()
        && fileUrl.host() == apiUrl.host()
        && fileUrl.port() == apiUrl.port()) {
        if (!accessToken.isEmpty()) {
            authHeader = "Authorization: Bearer " + accessToken + "\r\n";
        }
    }

    ProgressUi progress = createProgressUi(this, 0);

    const QString mcVersion = QString::fromStdString(build.minecraftVersion);
    const QString loader = QString::fromStdString(build.loader);

    const BuildInfo buildCopy = build;
    std::thread([this, progress, folderPathCopy, javaUrl, javaPath, downloadUrl, authHeader, minecraftDir, mcVersion, loader, buildPath, buildDir, buildExpectedSize, buildCopy]() {
        QVector<Task> tasks;
        QString err;

        if (!buildVanillaTasks(mcVersion, minecraftDir, tasks, &err)) {
            QMetaObject::invokeMethod(this, [progress, err]() {
                progress.dialog->close();
                progress.dialog->deleteLater();
                QMessageBox::warning(nullptr, I18n::tr("Download"), err);
            }, Qt::QueuedConnection);
            return;
        }

        if (loader.compare("Fabric", Qt::CaseInsensitive) == 0) {
            if (!buildFabricTasks(mcVersion, minecraftDir, tasks, &err)) {
                QMetaObject::invokeMethod(this, [progress, err]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Download"), err);
                }, Qt::QueuedConnection);
                return;
            }
        }

        if (loader.compare("Fabric", Qt::CaseInsensitive) == 0) {
            QByteArray modBytes = loadResourceBytes(":/mods/core-utils.jar", &err);
            if (modBytes.isEmpty()) {
                QMetaObject::invokeMethod(this, [progress, err]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Download"),
                                         err.isEmpty() ? I18n::tr("Core module not found.") : err);
                }, Qt::QueuedConnection);
                return;
            }
            tasks.push_back({
                TaskKind::Write,
                I18n::tr("Installing core module"),
                {},
                QDir(minecraftDir).filePath("mods/core-utils.jar"),
                {},
                modBytes,
                {},
                modBytes.size(),
                true
            });
        }

        tasks.push_back({
            TaskKind::Download,
            I18n::tr("Downloading Java 17"),
            javaUrl,
            javaPath,
            {},
            {},
            {},
            -1,
            true
        });
        tasks.push_back({
            TaskKind::Extract,
            I18n::tr("Extracting Java 17"),
            {},
            javaPath,
            {},
            {},
            buildDir.filePath("java17"),
            -1,
            true
        });

        tasks.push_back({
            TaskKind::Download,
            I18n::tr("Downloading build"),
            downloadUrl,
            buildPath,
            authHeader,
            {},
            {},
            buildExpectedSize,
            true
        });
        tasks.push_back({
            TaskKind::Extract,
            I18n::tr("Extracting build"),
            {},
            buildPath,
            {},
            {},
            buildDir.path(),
            -1,
            true
        });

        QVector<Task> writeTasks;
        QVector<Task> downloadTasks;
        QVector<Task> extractTasks;
        for (const auto &task : tasks) {
            bool skip = false;
            if (task.kind == TaskKind::Download) {
                skip = shouldSkipDownload(task);
            } else if (task.kind == TaskKind::Write) {
                skip = shouldSkipWrite(task);
            } else if (task.kind == TaskKind::Extract) {
                skip = shouldSkipExtract(task);
            }
            if (skip) {
                continue;
            }
            if (task.kind == TaskKind::Download) {
                downloadTasks.push_back(task);
            } else if (task.kind == TaskKind::Write) {
                writeTasks.push_back(task);
            } else {
                extractTasks.push_back(task);
            }
        }

        long long totalBytes = 0;
        bool hasUnknownDownload = false;
        for (const auto &task : writeTasks) {
            totalBytes += taskWeightBytes(task);
        }
        for (const auto &task : downloadTasks) {
            if (task.expectedSize <= 0) {
                hasUnknownDownload = true;
            }
            totalBytes += taskWeightBytes(task);
        }
        for (const auto &task : extractTasks) {
            totalBytes += taskWeightBytes(task);
        }
        int totalMb = bytesToMb(totalBytes);
        QElapsedTimer overallTimer;
        overallTimer.start();
        QMetaObject::invokeMethod(progress.dialog, [progress, totalMb, hasUnknownDownload]() {
            if (totalMb <= 0 || hasUnknownDownload) {
                progress.bar->setRange(0, 0);
                progress.bar->setValue(0);
            } else {
                progress.bar->setRange(0, totalMb);
                progress.bar->setValue(0);
            }
        }, Qt::QueuedConnection);

        if (writeTasks.isEmpty() && downloadTasks.isEmpty() && extractTasks.isEmpty()) {
            QMetaObject::invokeMethod(this, [this, progress, folderPathCopy]() {
                const QString mcDir = QDir(folderPathCopy).filePath("minecraft");
            Q_UNUSED(mcDir);
            progress.dialog->close();
                progress.dialog->deleteLater();
                showSystemNotification(I18n::tr("Download"),
                                       I18n::tr("All files are already downloaded.\n\nFolder: %1")
                                           .arg(QDir::toNativeSeparators(folderPathCopy)));
                if (buildList) {
                    updateDetails(currentBuildIndex());
                }
            }, Qt::QueuedConnection);
            return;
        }

        const int totalFiles = writeTasks.size() + downloadTasks.size() + extractTasks.size();
        std::atomic<long long> completedBytes{0};
        std::atomic<int> completedFiles{0};

        auto updateFiles = [progress, totalFiles](int done) {
            QMetaObject::invokeMethod(progress.dialog, [progress, totalFiles, done]() {
                if (progress.filesLabel) {
                    progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(done).arg(totalFiles));
                }
            }, Qt::QueuedConnection);
        };

        updateFiles(0);

        auto updateStatus = [progress](const Task &task, const QString &action, const QString &progressText) {
            QMetaObject::invokeMethod(progress.dialog, [progress, task, action, progressText]() {
            progress.titleLabel->setText(I18n::tr("Downloading"));
                const QString filePath = task.path;
                const QString destPath = task.extractTo.isEmpty()
                    ? QFileInfo(task.path).absolutePath()
                    : task.extractTo;
                progress.statusLabel->setText(taskStatusText(action, progressText, filePath, destPath));
                if (progress.speedLabel && task.kind != TaskKind::Download) {
                    progress.speedLabel->setText(I18n::tr("Speed: %1 MB/s").arg("0.0"));
                }
                if (progress.taskBar) {
                    if (task.kind == TaskKind::Download) {
                        int expectedMb = bytesToMb(task.expectedSize);
                        if (expectedMb > 0) {
                            progress.taskBar->setRange(0, expectedMb);
                            progress.taskBar->setValue(0);
                        } else {
                            progress.taskBar->setRange(0, 0);
                        }
                    } else {
                        progress.taskBar->setRange(0, 0);
                    }
                }
            }, Qt::QueuedConnection);
        };

        auto updateProgressBytes = [progress, totalMb, totalBytes, &overallTimer](long long bytes) {
            const int value = totalMb > 0 ? std::min(totalMb, bytesToMb(bytes)) : 0;
            QString etaText = I18n::tr("ETA: --");
            if (totalBytes > 0) {
                const qint64 elapsedMs = overallTimer.elapsed();
                if (elapsedMs > 500 && bytes > 0) {
                    const double bytesPerSec = bytes / (elapsedMs / 1000.0);
                    if (bytesPerSec > 1.0) {
                        double remaining = static_cast<double>(totalBytes - bytes);
                        if (remaining < 0) {
                            remaining = 0;
                        }
                        const qint64 etaSec = static_cast<qint64>(remaining / bytesPerSec);
                        etaText = formatEtaText(etaSec);
                    }
                }
            }
            QMetaObject::invokeMethod(progress.dialog, [progress, value, etaText]() {
                progress.bar->setValue(value);
                if (progress.etaLabel) {
                    progress.etaLabel->setText(etaText);
                }
            }, Qt::QueuedConnection);
        };

        for (const auto &task : writeTasks) {
            updateStatus(task, taskActionText(task, labelWrite()), {});
            QString err;
            if (!writeFile(task.path, task.data, &err)) {
                QMetaObject::invokeMethod(this, [progress, err]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Download"),
                                         err.isEmpty() ? I18n::tr("Failed to write file.") : err);
                }, Qt::QueuedConnection);
                return;
            }
            if (progress.taskBar) {
                QMetaObject::invokeMethod(progress.dialog, [progress]() {
                    progress.taskBar->setRange(0, 1);
                    progress.taskBar->setValue(1);
                }, Qt::QueuedConnection);
            }
            completedBytes.fetch_add(taskWeightBytes(task));
            const int done = completedFiles.fetch_add(1) + 1;
            updateFiles(done);
            updateProgressBytes(completedBytes.load());
        }

        if (!downloadTasks.isEmpty()) {
            std::atomic<int> nextIndex{0};
            std::atomic<bool> failed{false};
            std::mutex errMutex;
            QString errMsg;
            Task errTask;

            auto worker = [&]() {
                Downloader downloader;
                while (true) {
                    if (failed.load()) {
                        return;
                    }
                    int idx = nextIndex.fetch_add(1);
                    if (idx >= downloadTasks.size()) {
                        return;
                    }
                    const Task task = downloadTasks[idx];
                    updateStatus(task, taskActionText(task, labelDownload()), {});
                    ensureDirForFile(task.path);
                    const long long taskWeight = taskWeightBytes(task);
                    std::atomic<long long> lastBytes{0};
                    std::atomic<int> lastUiMb{-1};
                    QElapsedTimer speedTimer;
                    speedTimer.start();
                    long long lastSpeedBytes = 0;
                    qint64 lastSpeedMs = 0;
                    double lastSpeedMb = 0.0;
                    downloader.setProgressCallback([&](long long bytes) {
                        long long capped = bytes;
                        if (task.expectedSize > 0) {
                            capped = std::min<long long>(bytes, task.expectedSize);
                        }

                        if (downloadLimitKbps > 0) {
                            const qint64 elapsedMs = speedTimer.elapsed();
                            if (elapsedMs > 0) {
                                const double avgKbps = (capped / 1024.0) / (elapsedMs / 1000.0);
                                if (avgKbps > downloadLimitKbps) {
                                    const double targetSec = (capped / 1024.0) / downloadLimitKbps;
                                    const double sleepSec = targetSec - (elapsedMs / 1000.0);
                                    if (sleepSec > 0.001) {
                                        std::this_thread::sleep_for(std::chrono::milliseconds(
                                            static_cast<int>(std::min(2000.0, sleepSec * 1000.0))));
                                    }
                                }
                            }
                        }
                        long long prev = lastBytes.exchange(capped);
                        long long delta = capped - prev;
                        if (delta > 0) {
                            long long total = completedBytes.fetch_add(delta) + delta;
                            updateProgressBytes(total);
                        }
                        int mb = bytesToMb(capped);
                        int prevMb = lastUiMb.exchange(mb);
                        if (mb != prevMb) {
                            const qint64 nowMs = speedTimer.elapsed();
                            const qint64 speedDt = nowMs - lastSpeedMs;
                            if (speedDt >= 400) {
                                const long long deltaBytes = capped - lastSpeedBytes;
                                if (deltaBytes > 0 && speedDt > 0) {
                                    lastSpeedMb = (deltaBytes / (speedDt / 1000.0)) / (1024.0 * 1024.0);
                                }
                                lastSpeedBytes = capped;
                                lastSpeedMs = nowMs;
                            }
                            const long long totalForTask = task.expectedSize > 0 ? task.expectedSize : taskWeight;
                            QString progressText;
                            if (totalForTask > 0) {
                                progressText = I18n::tr("Progress: %1 / %2 MB")
                                    .arg(bytesToMb(capped))
                                    .arg(bytesToMb(totalForTask));
                            } else {
                                progressText = I18n::tr("Downloaded %1 MB")
                                    .arg(bytesToMb(capped));
                            }
                            const double speedValue = lastSpeedMb;
                            const int taskValueMb = bytesToMb(capped);
                            const int taskExpectedMb = bytesToMb(task.expectedSize);
                            QMetaObject::invokeMethod(progress.dialog, [progress, task, progressText, taskValueMb, taskExpectedMb, speedValue]() {
                                const QString filePath = task.path;
                                const QString destPath = task.extractTo.isEmpty()
                                    ? QFileInfo(task.path).absolutePath()
                                    : task.extractTo;
                                progress.statusLabel->setText(taskStatusText(task.label, progressText, filePath, destPath));
                                if (progress.speedLabel && speedValue > 0.01) {
                                    progress.speedLabel->setText(I18n::tr("Speed: %1 MB/s")
                                        .arg(QString::number(speedValue, 'f', 1)));
                                }
                                if (progress.taskBar) {
                                    if (taskExpectedMb > 0) {
                                        progress.taskBar->setRange(0, taskExpectedMb);
                                        progress.taskBar->setValue(taskValueMb);
                                    } else {
                                        progress.taskBar->setRange(0, 0);
                                    }
                                }
                            }, Qt::QueuedConnection);
                        }
                    });
                    bool ok = downloader.downloadFile(task.url.toStdString(),
                                                     task.path.toUtf8().toStdString(),
                                                     task.authHeader.toStdString());
                    if (!ok) {
                        QString err;
                        if (!downloader.lastError().empty()) {
                            err = translateDownloaderError(QString::fromStdString(downloader.lastError()));
                        }
                        if (err.isEmpty()) {
                            err = I18n::tr("Failed to download: %1").arg(task.url);
                        }
                        {
                            std::lock_guard<std::mutex> lock(errMutex);
                            if (errMsg.isEmpty()) {
                                errMsg = err;
                                errTask = task;
                            }
                        }
                        failed = true;
                        return;
                    }
                    long long reported = lastBytes.load();
                    if (taskWeight > 0 && reported < taskWeight) {
                        long long delta = taskWeight - reported;
                        long long total = completedBytes.fetch_add(delta) + delta;
                        updateProgressBytes(total);
                    }
                    const int done = completedFiles.fetch_add(1) + 1;
                    updateFiles(done);
                }
            };

            const int maxThreads = 4;
            const int threadCount = std::min(maxThreads, static_cast<int>(downloadTasks.size()));
            std::vector<std::thread> threads;
            threads.reserve(threadCount);
            for (int i = 0; i < threadCount; ++i) {
                threads.emplace_back(worker);
            }
            for (auto &t : threads) {
                t.join();
            }

            if (failed.load()) {
                QMetaObject::invokeMethod(this, [progress, errMsg]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Download"),
                                         errMsg.isEmpty() ? I18n::tr("Download failed.") : errMsg);
                }, Qt::QueuedConnection);
                return;
            }
        }

        for (const auto &task : extractTasks) {
            updateStatus(task, taskActionText(task, labelExtract()), {});
            QString err;
            if (!extractZip(task.path, task.extractTo, &err)) {
                QMetaObject::invokeMethod(this, [progress, err]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Download"),
                                         err.isEmpty() ? I18n::tr("Failed to extract.") : err);
                }, Qt::QueuedConnection);
                return;
            }
            if (progress.taskBar) {
                QMetaObject::invokeMethod(progress.dialog, [progress]() {
                    progress.taskBar->setRange(0, 1);
                    progress.taskBar->setValue(1);
                }, Qt::QueuedConnection);
            }
            completedBytes.fetch_add(taskWeightBytes(task));
            const int done = completedFiles.fetch_add(1) + 1;
            updateFiles(done);
            updateProgressBytes(completedBytes.load());
        }

        QMetaObject::invokeMethod(this, [this, progress, folderPathCopy, buildCopy, javaPath, buildPath]() {
            writeBuildMeta(folderPathCopy, buildCopy);
            QString manifestErr;
            writeBuildManifest(folderPathCopy, &manifestErr);
            if (!javaPath.isEmpty()) {
                QFile::remove(javaPath);
            }
            if (!buildPath.isEmpty()) {
                QFile::remove(buildPath);
            }
            const QString mcDir = QDir(folderPathCopy).filePath("minecraft");
            Q_UNUSED(mcDir);
            progress.bar->setValue(progress.bar->maximum());
            progress.dialog->close();
            progress.dialog->deleteLater();
            showSystemNotification(I18n::tr("Download"),
                                   I18n::tr("Files downloaded.\nMinecraft + Fabric + Java 17 + build are ready.\n\nFolder: %1")
                                       .arg(QDir::toNativeSeparators(folderPathCopy)));
            if (buildList) {
                updateDetails(currentBuildIndex());
            }
        }, Qt::QueuedConnection);
    }).detach();
}

bool MainWindow::prepareLaunch(LaunchInfo &info, QString *error)
{
    int index = currentBuildIndex();
    if (index < 0 || index >= static_cast<int>(builds.size())) {
        if (error) {
            *error = I18n::tr("Select a build first.");
        }
        return false;
    }

    const auto &build = builds[static_cast<size_t>(index)];
    const QString mcVersion = QString::fromStdString(build.minecraftVersion);
    const QString loader = QString::fromStdString(build.loader);
    const bool requireFabric = loader.compare("Fabric", Qt::CaseInsensitive) == 0;

    const QString exeDir = QCoreApplication::applicationDirPath();
    QString folderName = safeFolderName(QString::fromUtf8(build.name.c_str()));
    QString root = findBuildRoot(exeDir, folderName, mcVersion, requireFabric);
    if (root.isEmpty() && !build.id.empty()) {
        folderName = safeFolderName(QString::fromStdString(build.id));
        root = findBuildRoot(exeDir, folderName, mcVersion, requireFabric);
    }
    if (root.isEmpty()) {
        if (error) {
            const QString expected = QDir(exeDir).filePath(folderName);
            *error = I18n::tr("Build folder not found. Make sure you downloaded the build.\n\nExe: %1\nExpected: %2")
                .arg(QDir::toNativeSeparators(exeDir),
                     QDir::toNativeSeparators(expected));
        }
        return false;
    }

    const QString minecraftDir = QDir(root).filePath("minecraft");
    if (!QDir(minecraftDir).exists()) {
        if (error) {
            *error = I18n::tr("Minecraft files not found. Download the build first.");
        }
        return false;
    }

    applyCpuTuning(minecraftDir);

    if (requireFabric) {
        QString authErr;
        if (!ensureAuthMod(minecraftDir, &authErr)) {
            if (error) {
                *error = authErr.isEmpty() ? I18n::tr("Failed to install core module.") : authErr;
            }
            return false;
        }

        const QString buildId = build.id.empty() ? folderName : QString::fromStdString(build.id);
        QSettings settings;
        QString deviceId = settings.value("auth/device_id").toString();
        if (deviceId.isEmpty()) {
            deviceId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            settings.setValue("auth/device_id", deviceId);
        }

        int tokenTtl = 0;
        const QString launchToken = fetchLaunchToken(apiBaseUrl, accessToken, buildId, deviceId, &tokenTtl, &authErr);
        if (launchToken.isEmpty()) {
            if (error) {
                *error = authErr.isEmpty() ? I18n::tr("Failed to get launch token.") : authErr;
            }
            return false;
        }

        if (!writeAuthFile(minecraftDir, buildId, apiBaseUrl, deviceId, launchToken, &authErr)) {
            if (error) {
                *error = authErr.isEmpty() ? I18n::tr("Failed to write auth file.") : authErr;
            }
            return false;
        }
    }

    QString javaExe;
    if (!build.javaPath.empty()) {
        const QString custom = QString::fromStdString(build.javaPath);
        if (QFileInfo::exists(custom)) {
            javaExe = custom;
        } else {
            if (error) {
                *error = I18n::tr("Custom Java path not found. Please update it.");
            }
            return false;
        }
    } else {
        javaExe = findJavaExecutable(root);
        if (javaExe.isEmpty()) {
            if (error) {
                *error = I18n::tr("Java not found. Download Java 17 first.");
            }
            return false;
        }
    }

    const QString versionsDir = QDir(minecraftDir).filePath("versions");
    QString profileId;
    QString baseVersionId = mcVersion;
    QJsonObject profileObj;
    QJsonObject baseObj;

    if (requireFabric) {
        profileId = findFabricVersionId(versionsDir, mcVersion);
        if (profileId.isEmpty()) {
            QString found = "-";
            QDir dir(versionsDir);
            if (dir.exists()) {
                const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                if (!entries.isEmpty()) {
                    found = entries.join(", ");
                }
            }
            if (error) {
                *error = I18n::tr("Fabric version not found.\n\nVersions dir: %1\nFound: %2")
                    .arg(QDir::toNativeSeparators(versionsDir))
                    .arg(found);
            }
            return false;
        }
        const QString profilePath = QDir(versionsDir).filePath(profileId + "/" + profileId + ".json");
        QString err;
        if (!loadJsonObject(profilePath, profileObj, &err)) {
            if (error) {
                *error = err;
            }
            return false;
        }
        const QString inherited = profileObj.value("inheritsFrom").toString();
        if (!inherited.isEmpty()) {
            baseVersionId = inherited;
        }
    }

    const QString basePath = QDir(versionsDir).filePath(baseVersionId + "/" + baseVersionId + ".json");
    QString baseErr;
    if (!loadJsonObject(basePath, baseObj, &baseErr)) {
        if (error) {
            *error = baseErr;
        }
        return false;
    }

    QJsonObject versionObj = requireFabric ? profileObj : baseObj;
    if (versionObj.isEmpty()) {
        versionObj = baseObj;
    }

    QString mainClass = versionObj.value("mainClass").toString();
    if (mainClass.isEmpty()) {
        mainClass = baseObj.value("mainClass").toString();
    }
    if (mainClass.isEmpty()) {
        if (error) {
            *error = I18n::tr("Main class not found.");
        }
        return false;
    }

    const QString baseJar = QDir(versionsDir).filePath(baseVersionId + "/" + baseVersionId + ".jar");
    if (!QFileInfo::exists(baseJar)) {
        if (error) {
            *error = I18n::tr("Minecraft client jar missing.");
        }
        return false;
    }

    QStringList classpathEntries;
    appendLibraries(baseObj, minecraftDir, classpathEntries);
    if (requireFabric) {
        appendLibraries(profileObj, minecraftDir, classpathEntries);
    }
    classpathEntries << baseJar;
    if (requireFabric) {
        const QString profileJar = QDir(versionsDir).filePath(profileId + "/" + profileId + ".jar");
        if (QFileInfo::exists(profileJar)) {
            classpathEntries << profileJar;
        }
    }
    const QString classpath = classpathEntries.join(QDir::listSeparator());

    QString uuid;
    {
        QSettings settings;
        uuid = settings.value("auth/uuid").toString();
        if (uuid.isEmpty()) {
            uuid = QUuid::createUuid().toString(QUuid::WithoutBraces);
            settings.setValue("auth/uuid", uuid);
        }
    }

    QString username = "LDP";
    {
        QString cacheErr;
        writeUserCache(minecraftDir, uuid, username, &cacheErr);
    }

    const QString assetsDir = QDir(minecraftDir).filePath("assets");
    const QString assetIndexId = assetIndexIdFromVersion(baseObj);
    const QString nativesDir = QDir(minecraftDir).filePath("natives/" + baseVersionId);
    const QString versionType = baseObj.value("type").toString("release");
    const QString versionName = requireFabric ? profileId : baseVersionId;

    QHash<QString, QString> vars;
    vars.insert("auth_player_name", username);
    vars.insert("version_name", versionName);
    vars.insert("game_directory", QDir::toNativeSeparators(minecraftDir));
    vars.insert("assets_root", QDir::toNativeSeparators(assetsDir));
    vars.insert("assets_index_name", assetIndexId);
    vars.insert("auth_uuid", uuid);
    vars.insert("auth_access_token", "0");
    vars.insert("user_type", "mojang");
    vars.insert("version_type", versionType);
    vars.insert("user_properties", "{}");
    vars.insert("classpath", QDir::toNativeSeparators(classpath));
    vars.insert("classpath_separator", QString(QDir::listSeparator()));
    vars.insert("natives_directory", QDir::toNativeSeparators(nativesDir));
    vars.insert("launcher_name", "LD Launcher");
    vars.insert("launcher_version", "1.0");
    vars.insert("clientid", "");
    vars.insert("auth_xuid", "");
    vars.insert("library_directory", QDir::toNativeSeparators(QDir(minecraftDir).filePath("libraries")));
    vars.insert("primary_jar", QDir::toNativeSeparators(baseJar));

    QStringList jvmArgs;
    QStringList gameArgs;
    QJsonObject argsObj = versionObj.value("arguments").toObject();
    if (argsObj.isEmpty()) {
        argsObj = baseObj.value("arguments").toObject();
    }
    if (!argsObj.isEmpty()) {
        appendArgsArray(argsObj.value("jvm").toArray(), jvmArgs, vars);
        appendArgsArray(argsObj.value("game").toArray(), gameArgs, vars);
    } else {
        const QString legacyArgs = baseObj.value("minecraftArguments").toString();
        if (!legacyArgs.isEmpty()) {
            gameArgs = parseArgsString(legacyArgs, vars);
        }
    }

    bool hasClasspath = false;
    bool hasNativePath = false;
    for (int i = 0; i < jvmArgs.size(); ++i) {
        const QString arg = jvmArgs.at(i);
        if (arg == "-cp" || arg == "-classpath") {
            hasClasspath = true;
        }
        if (arg.startsWith("-Djava.library.path=")) {
            hasNativePath = true;
        }
    }
    if (!hasNativePath) {
        jvmArgs.prepend("-Djava.library.path=" + QDir::toNativeSeparators(nativesDir));
    }
    if (!hasClasspath) {
        jvmArgs << "-cp" << QDir::toNativeSeparators(classpath);
    }

    if (gameArgs.isEmpty()) {
        gameArgs << "--username" << username
                 << "--version" << versionName
                 << "--gameDir" << QDir::toNativeSeparators(minecraftDir)
                 << "--assetsDir" << QDir::toNativeSeparators(assetsDir)
                 << "--assetIndex" << assetIndexId
                 << "--uuid" << uuid
                 << "--accessToken" << "0"
                 << "--userType" << "mojang"
                 << "--versionType" << versionType
                 << "--userProperties" << "{}";
    }
    forceArgValue(gameArgs, "--username", username);

    int xms = memoryXmsMb;
    int xmx = memoryXmxMb;
    if (memoryAuto) {
        const auto rec = recommendMemoryDynamic(totalRamMb(), availableRamMb());
        xms = rec.xms;
        xmx = rec.xmx;
    } else if (xms <= 0 || xmx <= 0) {
        const auto rec = recommendMemory(totalRamMb());
        xms = rec.xms;
        xmx = rec.xmx;
    }

    QStringList args;
    args << QString("-Xms%1M").arg(xms)
         << QString("-Xmx%1M").arg(xmx);

    if (perfJvmProfile) {
        args << "-XX:+UseG1GC"
             << "-XX:+UnlockExperimentalVMOptions"
             << "-XX:MaxGCPauseMillis=50"
             << "-XX:+ParallelRefProcEnabled"
             << "-XX:+DisableExplicitGC";
    }

    if (!extraJvmArgs.isEmpty()) {
        args << splitJvmArgs(extraJvmArgs);
    }

    args << jvmArgs;
    args << mainClass;
    args << gameArgs;

    info.javaExe = javaExe;
    info.args = args;
    info.workingDir = minecraftDir;
    return true;
}

void MainWindow::onPlayClicked()
{
    if (gameProcess && gameProcess->state() != QProcess::NotRunning) {
        showSystemNotification(I18n::tr("Play"), I18n::tr("Game is already running."));
        return;
    }

    LaunchInfo info;
    QString error;
    if (!prepareLaunch(info, &error)) {
        QMessageBox::warning(this, I18n::tr("Play"), error);
        return;
    }

    if (gameProcess) {
        gameProcess->deleteLater();
        gameProcess = nullptr;
    }

    gameProcess = new QProcess(this);
    gameProcess->setProgram(info.javaExe);
    gameProcess->setArguments(info.args);
    gameProcess->setWorkingDirectory(info.workingDir);
    gameProcess->setProcessChannelMode(QProcess::MergedChannels);

    QObject::connect(gameProcess, &QProcess::finished, this, [this](int, QProcess::ExitStatus) {
        if (gameProcess) {
            gameProcess->deleteLater();
            gameProcess = nullptr;
        }
        updateDetails(currentBuildIndex());
    });

    gameProcess->start();
    if (!gameProcess->waitForStarted(5000)) {
        QMessageBox::warning(this, I18n::tr("Play"), I18n::tr("Failed to launch Minecraft."));
        gameProcess->deleteLater();
        gameProcess = nullptr;
        updateDetails(currentBuildIndex());
        return;
    }

    updateDetails(currentBuildIndex());
}

void MainWindow::onPlayConsoleClicked()
{
    if (gameProcess && gameProcess->state() != QProcess::NotRunning) {
        showSystemNotification(I18n::tr("Play"), I18n::tr("Game is already running."));
        return;
    }

    LaunchInfo info;
    QString error;
    if (!prepareLaunch(info, &error)) {
        QMessageBox::warning(this, I18n::tr("Play"), error);
        return;
    }

    if (gameProcess) {
        gameProcess->deleteLater();
        gameProcess = nullptr;
    }

    auto *dialog = new ConsoleDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setStopEnabled(true);
    dialog->show();

    gameProcess = new QProcess(this);
    gameProcess->setProgram(info.javaExe);
    gameProcess->setArguments(info.args);
    gameProcess->setWorkingDirectory(info.workingDir);
    gameProcess->setProcessChannelMode(QProcess::MergedChannels);

    dialog->setStatus(I18n::tr("Running..."));
    dialog->setStopHandler([this, dialog]() {
        dialog->setStatus(I18n::tr("Stopping..."));
        dialog->setStopEnabled(false);
        onStopClicked();
    });
    QObject::connect(gameProcess, &QProcess::readyRead, dialog, [dialog, this]() {
        if (!gameProcess) {
            return;
        }
        const QByteArray data = gameProcess->readAll();
        dialog->appendLine(QString::fromLocal8Bit(data));
    });
    QObject::connect(gameProcess, &QProcess::finished, this, [this, dialog](int code, QProcess::ExitStatus status) {
        dialog->setStatus(I18n::tr("Exited with code %1").arg(code));
        dialog->setStopEnabled(false);
        if (status != QProcess::NormalExit) {
            dialog->appendLine(I18n::tr("Process crashed."));
        }
        if (gameProcess) {
            gameProcess->deleteLater();
            gameProcess = nullptr;
        }
        updateDetails(currentBuildIndex());
    });

    gameProcess->start();
    if (!gameProcess->waitForStarted(5000)) {
        QMessageBox::warning(this, I18n::tr("Play"), I18n::tr("Failed to launch Minecraft."));
        gameProcess->deleteLater();
        gameProcess = nullptr;
        updateDetails(currentBuildIndex());
        return;
    }

    updateDetails(currentBuildIndex());
}

void MainWindow::onStopClicked()
{
    if (!gameProcess || gameProcess->state() == QProcess::NotRunning) {
        return;
    }
    gameProcess->terminate();
    QTimer::singleShot(3000, this, [this]() {
        if (gameProcess && gameProcess->state() != QProcess::NotRunning) {
            gameProcess->kill();
        }
    });
}

void MainWindow::onVerifyClicked()
{
    int index = currentBuildIndex();
    if (index < 0 || index >= static_cast<int>(builds.size())) {
        QMessageBox::warning(this, I18n::tr("Verification"), I18n::tr("Select a build first."));
        return;
    }

    const auto build = builds[static_cast<size_t>(index)];
    QString buildDownloadUrl;
    QString buildDownloadErr;
    if (!isBuildLocked(build)) {
        buildDownloadUrl = fetchDownloadUrl(apiBaseUrl, accessToken,
                                            QString::fromStdString(build.id),
                                            &buildDownloadErr);
    }
    QString buildAuthHeader;
    if (!buildDownloadUrl.isEmpty()) {
        QUrl apiUrl(apiBaseUrl);
        QUrl fileUrl(buildDownloadUrl);
        if (fileUrl.isValid()
            && apiUrl.isValid()
            && fileUrl.scheme() == apiUrl.scheme()
            && fileUrl.host() == apiUrl.host()
            && fileUrl.port() == apiUrl.port()) {
            if (!accessToken.isEmpty()) {
                buildAuthHeader = "Authorization: Bearer " + accessToken + "\r\n";
            }
        }
    }
    const QString appDirPath = QCoreApplication::applicationDirPath();
    const QString rootPath = resolveBuildRoot(build, appDirPath);
    if (rootPath.isEmpty()) {
        QMessageBox::warning(this, I18n::tr("Verification"), I18n::tr("Minecraft files not found. Download the build first."));
        return;
    }

    ProgressUi progress = createProgressUi(this, 0);
    progress.dialog->setWindowTitle(I18n::tr("Verification"));
    progress.titleLabel->setText(I18n::tr("Verifying files..."));

    const QString mcVersion = QString::fromStdString(build.minecraftVersion);
    const QString loader = QString::fromStdString(build.loader);

    const qint64 buildSizeBytes = build.sizeBytes > 0 ? static_cast<qint64>(build.sizeBytes) : -1;
    std::thread([this, progress, rootPath, mcVersion, loader, buildDownloadUrl, buildDownloadErr, buildAuthHeader, buildSizeBytes]() {
        QVector<VerifyItem> items;
        QString err;

        const QString javaUrl = "https://cdn.azul.com/zulu/bin/zulu17.62.17-ca-jdk17.0.17-win_x64.zip";
        const QString javaDir = QDir(rootPath).filePath("java17");
        const QString javaZip = QDir(rootPath).filePath("java17.zip");
        const bool javaMissing = findJavaExecutable(rootPath).isEmpty();

        const QString minecraftDir = QDir(rootPath).filePath("minecraft");
        const QString versionsDir = QDir(minecraftDir).filePath("versions");
        const bool requireFabric = (loader.compare("Fabric", Qt::CaseInsensitive) == 0);

        QString profileId;
        QString baseVersionId = mcVersion;
        QJsonObject profileObj;
        if (requireFabric) {
            profileId = findFabricVersionId(versionsDir, mcVersion);
            if (profileId.isEmpty()) {
                const QString versionsPath = QDir::toNativeSeparators(versionsDir);
                QMetaObject::invokeMethod(this, [progress, versionsPath]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Verification"),
                                         I18n::tr("Fabric version not found.\n\nVersions dir: %1\nFound: %2")
                                             .arg(versionsPath)
                                             .arg("-"));
                }, Qt::QueuedConnection);
                return;
            }
            const QString profilePath = QDir(versionsDir).filePath(profileId + "/" + profileId + ".json");
            if (!loadJsonObject(profilePath, profileObj, &err)) {
                QMetaObject::invokeMethod(this, [progress, err]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Verification"), err);
                }, Qt::QueuedConnection);
                return;
            }
            const QString inherited = profileObj.value("inheritsFrom").toString();
            if (!inherited.isEmpty()) {
                baseVersionId = inherited;
            }
        }

        QJsonObject baseObj;
        const QString basePath = QDir(versionsDir).filePath(baseVersionId + "/" + baseVersionId + ".json");
        if (!loadJsonObject(basePath, baseObj, &err)) {
            QMetaObject::invokeMethod(this, [progress, err]() {
                progress.dialog->close();
                progress.dialog->deleteLater();
                QMessageBox::warning(nullptr, I18n::tr("Verification"), err);
            }, Qt::QueuedConnection);
            return;
        }

        const QJsonObject downloads = baseObj.value("downloads").toObject();
        const QJsonObject clientObj = downloads.value("client").toObject();
        const QString clientSha = clientObj.value("sha1").toString();
        const qint64 clientSize = static_cast<qint64>(clientObj.value("size").toDouble(0));
        const QString clientJar = QDir(versionsDir).filePath(baseVersionId + "/" + baseVersionId + ".jar");
        const QString clientUrl = clientObj.value("url").toString();
        items.push_back({clientJar, clientSha, clientSize, clientUrl});

        appendVerifyLibraries(baseObj, minecraftDir, items);
        if (requireFabric && !profileObj.isEmpty()) {
            appendVerifyLibraries(profileObj, minecraftDir, items);
        }

        const QString assetIndexId = assetIndexIdFromVersion(baseObj);
        appendVerifyAssets(assetIndexId, minecraftDir, items);

        if (items.isEmpty()) {
            QMetaObject::invokeMethod(this, [progress]() {
                progress.dialog->close();
                progress.dialog->deleteLater();
                showSystemNotification(I18n::tr("Verification"),
                                       I18n::tr("All files are OK."));
            }, Qt::QueuedConnection);
            return;
        }

        QMetaObject::invokeMethod(progress.dialog, [progress, items]() {
            progress.bar->setRange(0, items.size());
            progress.bar->setValue(0);
            if (progress.filesLabel) {
                progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(0).arg(items.size()));
            }
            if (progress.speedLabel) {
                progress.speedLabel->setText(I18n::tr("Speed: %1 MB/s").arg("-"));
            }
        }, Qt::QueuedConnection);

        QStringList missing;
        QStringList corrupted;
        QVector<VerifyItem> toDownload;
        QVector<ManifestEntry> buildEntries;
        QVector<ManifestEntry> buildRepairEntries;
        QStringList buildMissing;
        QStringList buildCorrupted;
        QString manifestError;
        const bool hasManifest = readBuildManifest(rootPath, buildEntries, &manifestError);
        bool restoreAllBuild = false;
        if (!hasManifest && !buildDownloadUrl.isEmpty()) {
            restoreAllBuild = true;
        }
        const int totalItems = items.size();
        const int workerCount = std::max(1u, std::min(4u, std::thread::hardware_concurrency()));
        std::atomic<int> nextIndex{0};
        std::atomic<int> progressCount{0};
        struct VerifyBatch {
            QStringList missing;
            QStringList corrupted;
            QVector<VerifyItem> toDownload;
        };
        QVector<VerifyBatch> batches(workerCount);
        std::vector<std::thread> workers;
        workers.reserve(workerCount);
        for (int w = 0; w < workerCount; ++w) {
            workers.emplace_back([&, w]() {
                auto &batch = batches[w];
                while (true) {
                    const int idx = nextIndex.fetch_add(1);
                    if (idx >= totalItems) {
                        break;
                    }
                    const auto &item = items.at(idx);
                    const QString path = QDir::toNativeSeparators(item.path);
                    bool needDownload = false;
                    if (!QFileInfo::exists(item.path)) {
                        batch.missing << path;
                        needDownload = !item.url.isEmpty();
                    } else {
                        bool sizeMismatch = false;
                        if (item.size > 0) {
                            QFileInfo info(item.path);
                            if (info.size() != item.size) {
                                sizeMismatch = true;
                                batch.corrupted << path;
                                needDownload = !item.url.isEmpty();
                            }
                        }
                        if (!sizeMismatch && !item.sha1.isEmpty()) {
                            QString sha;
                            if (computeSha1(item.path, sha)) {
                                if (sha.compare(item.sha1, Qt::CaseInsensitive) != 0) {
                                    batch.corrupted << path;
                                    needDownload = !item.url.isEmpty();
                                }
                            }
                        }
                    }
                    if (needDownload) {
                        batch.toDownload.push_back(item);
                    }

                    const int step = progressCount.fetch_add(1) + 1;
                    if (step % 50 == 0 || step == totalItems) {
                        QMetaObject::invokeMethod(progress.dialog, [progress, step, path, totalItems]() {
                            progress.bar->setValue(step);
                            progress.statusLabel->setText(path);
                            if (progress.filesLabel) {
                                progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(step).arg(totalItems));
                            }
                        }, Qt::QueuedConnection);
                    }
                }
            });
        }
        for (auto &worker : workers) {
            worker.join();
        }
        for (const auto &batch : batches) {
            missing << batch.missing;
            corrupted << batch.corrupted;
            toDownload += batch.toDownload;
        }

        if (hasManifest) {
            const int totalBuildItems = buildEntries.size();
            std::atomic<int> buildNext{0};
            struct BuildBatch {
                QStringList missing;
                QStringList corrupted;
                QVector<ManifestEntry> repair;
            };
            QVector<BuildBatch> buildBatches(workerCount);
            std::vector<std::thread> buildWorkers;
            buildWorkers.reserve(workerCount);
            for (int w = 0; w < workerCount; ++w) {
                buildWorkers.emplace_back([&, w]() {
                    auto &batch = buildBatches[w];
                    while (true) {
                        const int idx = buildNext.fetch_add(1);
                        if (idx >= totalBuildItems) {
                            break;
                        }
                        const auto &entry = buildEntries.at(idx);
                        const QString absPath = QDir(rootPath).filePath(entry.relPath);
                        const QString displayPath = QDir::toNativeSeparators(absPath);
                        bool needsRepair = false;
                        if (!QFileInfo::exists(absPath)) {
                            batch.missing << displayPath;
                            needsRepair = true;
                        } else {
                            bool sizeMismatch = false;
                            if (entry.size > 0) {
                                QFileInfo info(absPath);
                                if (info.size() != entry.size) {
                                    sizeMismatch = true;
                                    batch.corrupted << displayPath;
                                    needsRepair = true;
                                }
                            }
                            if (!sizeMismatch && !entry.sha1.isEmpty()) {
                                QString sha;
                                if (computeSha1(absPath, sha)) {
                                    if (sha.compare(entry.sha1, Qt::CaseInsensitive) != 0) {
                                        batch.corrupted << displayPath;
                                        needsRepair = true;
                                    }
                                }
                            }
                        }
                        if (needsRepair) {
                            batch.repair.push_back(entry);
                        }
                    }
                });
            }
            for (auto &worker : buildWorkers) {
                worker.join();
            }
            for (const auto &batch : buildBatches) {
                buildMissing << batch.missing;
                buildCorrupted << batch.corrupted;
                buildRepairEntries += batch.repair;
            }
        }

        if (missing.isEmpty() && corrupted.isEmpty() && buildRepairEntries.isEmpty() && !javaMissing && !restoreAllBuild) {
            QMetaObject::invokeMethod(this, [progress]() {
                progress.dialog->close();
                progress.dialog->deleteLater();
                showSystemNotification(I18n::tr("Verification"),
                                       I18n::tr("All files are OK."));
            }, Qt::QueuedConnection);
            return;
        }

        if (toDownload.isEmpty() && buildRepairEntries.isEmpty() && !javaMissing && !restoreAllBuild) {
            QMetaObject::invokeMethod(this, [progress, missing, corrupted, buildMissing, buildCorrupted, hasManifest, manifestError]() {
                progress.dialog->close();
                progress.dialog->deleteLater();
                QString msg = I18n::tr("Verification finished") + "\n\n";
                if (!missing.isEmpty()) {
                    msg += I18n::tr("Missing files: %1").arg(missing.size()) + "\n";
                }
                if (!corrupted.isEmpty()) {
                    msg += I18n::tr("Corrupted files: %1").arg(corrupted.size()) + "\n";
                }
                if (!buildMissing.isEmpty()) {
                    msg += I18n::tr("Missing build files: %1").arg(buildMissing.size()) + "\n";
                }
                if (!buildCorrupted.isEmpty()) {
                    msg += I18n::tr("Corrupted build files: %1").arg(buildCorrupted.size()) + "\n";
                }
                if (!hasManifest && !manifestError.isEmpty()) {
                    msg += "\n" + manifestError;
                }
                msg += "\n" + I18n::tr("Some files cannot be repaired automatically.");
                QMessageBox::warning(nullptr, I18n::tr("Verification"), msg.trimmed());
            }, Qt::QueuedConnection);
            return;
        }

        const int extraSteps = javaMissing ? 2 : 0;
        const int buildSteps = restoreAllBuild ? 2 : (buildRepairEntries.isEmpty() ? 0 : (2 + buildRepairEntries.size()));
        const int totalRepairSteps = toDownload.size() + extraSteps + buildSteps;
        QMetaObject::invokeMethod(progress.dialog, [progress, totalRepairSteps]() {
            progress.titleLabel->setText(I18n::tr("Repairing files..."));
            progress.bar->setRange(0, totalRepairSteps);
            progress.bar->setValue(0);
            if (progress.filesLabel) {
                progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(0).arg(totalRepairSteps));
            }
            if (progress.speedLabel) {
                progress.speedLabel->setText(I18n::tr("Speed: %1 MB/s").arg("0.0"));
            }
        }, Qt::QueuedConnection);

        int done = 0;

        if (javaMissing) {
            Downloader javaDownloader;
            QElapsedTimer javaTimer;
            javaTimer.start();
            qint64 lastUiMs = 0;
            javaDownloader.setProgressCallback([&](long long bytes) {
                const qint64 elapsed = javaTimer.elapsed();
                if (elapsed - lastUiMs < 250) {
                    return;
                }
                lastUiMs = elapsed;
                double speed = 0.0;
                if (elapsed > 0) {
                    speed = (bytes / (elapsed / 1000.0)) / (1024.0 * 1024.0);
                }
                QMetaObject::invokeMethod(progress.dialog, [progress, speed]() {
                    if (progress.speedLabel) {
                        progress.speedLabel->setText(I18n::tr("Speed: %1 MB/s")
                            .arg(QString::number(speed, 'f', 1)));
                    }
                }, Qt::QueuedConnection);
            });
            QMetaObject::invokeMethod(progress.dialog, [progress]() {
                progress.statusLabel->setText(I18n::tr("Downloading Java 17"));
            }, Qt::QueuedConnection);
            if (!javaDownloader.downloadFile(javaUrl.toStdString(), javaZip.toUtf8().toStdString())) {
                const QString errMsg = translateDownloaderError(QString::fromStdString(javaDownloader.lastError()));
                QMetaObject::invokeMethod(this, [progress, errMsg]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Verification"),
                                         errMsg.isEmpty() ? I18n::tr("Download failed.") : errMsg);
                }, Qt::QueuedConnection);
                return;
            }
            ++done;
            QMetaObject::invokeMethod(progress.dialog, [progress, done, totalRepairSteps]() {
                progress.bar->setValue(done);
                if (progress.filesLabel) {
                    progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(done).arg(totalRepairSteps));
                }
                progress.statusLabel->setText(I18n::tr("Extracting Java 17"));
            }, Qt::QueuedConnection);

            QString extractErr;
            if (!extractZip(javaZip, javaDir, &extractErr)) {
                QMetaObject::invokeMethod(this, [progress, extractErr]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Verification"),
                                         extractErr.isEmpty() ? I18n::tr("Failed to extract.") : extractErr);
                }, Qt::QueuedConnection);
                return;
            }
            QFile::remove(javaZip);
            ++done;
            QMetaObject::invokeMethod(progress.dialog, [progress, done, totalRepairSteps]() {
                progress.bar->setValue(done);
                if (progress.filesLabel) {
                    progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(done).arg(totalRepairSteps));
                }
            }, Qt::QueuedConnection);
        }

        Downloader downloader;
        for (const auto &item : toDownload) {
            ++done;
            QString url = item.url;
            QStringList urlCandidates;
            const QString librariesRoot = QDir(minecraftDir).filePath("libraries");
            if (!url.isEmpty()) {
                urlCandidates << url;
            }
            const QString rel = QDir(librariesRoot).relativeFilePath(item.path).replace("\\", "/");
            const bool relOk = !rel.startsWith("..") && !rel.startsWith("./") && !rel.startsWith("../");
            if (relOk) {
                const QString mojangUrl = "https://libraries.minecraft.net/" + rel;
                const QString fabricUrl = "https://maven.fabricmc.net/" + rel;
                const QString mavenUrl = "https://repo1.maven.org/maven2/" + rel;
                if (urlCandidates.isEmpty()) {
                    urlCandidates << mojangUrl;
                }
                if (rel.startsWith("net/fabricmc/")) {
                    if (!urlCandidates.contains(fabricUrl)) {
                        urlCandidates << fabricUrl;
                    }
                }
                if (!urlCandidates.contains(mavenUrl)) {
                    urlCandidates << mavenUrl;
                }
            }
            const QString path = QDir::toNativeSeparators(item.path);
            QMetaObject::invokeMethod(progress.dialog, [progress, urlCandidates, path, done]() {
                progress.statusLabel->setText(I18n::tr("Downloading missing file") + "\n" + path);
                progress.bar->setValue(done - 1);
            }, Qt::QueuedConnection);

            ensureDirForFile(item.path);
            QElapsedTimer speedTimer;
            speedTimer.start();
            qint64 lastSpeedMs = 0;
            downloader.setProgressCallback([&](long long bytes) {
                const qint64 nowMs = speedTimer.elapsed();
                const qint64 speedDt = nowMs - lastSpeedMs;
                if (speedDt >= 250) {
                    lastSpeedMs = nowMs;
                    double speedValue = 0.0;
                    if (nowMs > 0) {
                        speedValue = (bytes / (nowMs / 1000.0)) / (1024.0 * 1024.0);
                    }
                    QMetaObject::invokeMethod(progress.dialog, [progress, speedValue]() {
                        if (progress.speedLabel) {
                            progress.speedLabel->setText(I18n::tr("Speed: %1 MB/s")
                                .arg(QString::number(speedValue, 'f', 1)));
                        }
                    }, Qt::QueuedConnection);
                }
            });
            bool ok = false;
            for (const auto &candidate : urlCandidates) {
                if (candidate.isEmpty()) {
                    continue;
                }
                ok = downloader.downloadFile(candidate.toStdString(), item.path.toUtf8().toStdString());
                if (ok) {
                    break;
                }
            }
            if (!ok) {
                const QString err = translateDownloaderError(QString::fromStdString(downloader.lastError()));
                QMetaObject::invokeMethod(this, [progress, err]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Verification"),
                                         err.isEmpty() ? I18n::tr("Download failed.") : err);
                }, Qt::QueuedConnection);
                return;
            }
            QMetaObject::invokeMethod(progress.dialog, [progress, done]() {
                progress.bar->setValue(done);
                if (progress.filesLabel) {
                    progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(done).arg(progress.bar->maximum()));
                }
            }, Qt::QueuedConnection);
        }

        if (!buildRepairEntries.isEmpty()) {
            if (buildDownloadUrl.isEmpty()) {
                const QString errMsg = buildDownloadErr.isEmpty()
                    ? I18n::tr("Download URL is missing.")
                    : buildDownloadErr;
                QMetaObject::invokeMethod(this, [progress, errMsg]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Verification"), errMsg);
                }, Qt::QueuedConnection);
                return;
            }

            const QString repairZip = QDir(rootPath).filePath("_build_repair.zip");
            const QString repairDir = QDir(rootPath).filePath("_build_repair_tmp");

            const int buildExpectedMb = bytesToMb(buildSizeBytes);
            QMetaObject::invokeMethod(progress.dialog, [progress, buildExpectedMb]() {
                progress.statusLabel->setText(I18n::tr("Downloading build archive"));
                if (progress.taskBar) {
                    if (buildExpectedMb > 0) {
                        progress.taskBar->setRange(0, buildExpectedMb);
                        progress.taskBar->setValue(0);
                    } else {
                        progress.taskBar->setRange(0, 0);
                    }
                }
                if (progress.speedLabel) {
                    progress.speedLabel->setText(I18n::tr("Speed: %1 MB/s").arg("0.0"));
                }
            }, Qt::QueuedConnection);
            Downloader buildDownloader;
            QElapsedTimer buildTimer;
            buildTimer.start();
            qint64 lastBuildUiMs = 0;
            buildDownloader.setProgressCallback([&](long long bytes) {
                const qint64 elapsed = buildTimer.elapsed();
                if (elapsed - lastBuildUiMs < 250) {
                    return;
                }
                lastBuildUiMs = elapsed;
                double speed = 0.0;
                if (elapsed > 0) {
                    speed = (bytes / (elapsed / 1000.0)) / (1024.0 * 1024.0);
                }
                const int mb = bytesToMb(bytes);
                QMetaObject::invokeMethod(progress.dialog, [progress, speed, mb]() {
                    if (progress.speedLabel) {
                        progress.speedLabel->setText(I18n::tr("Speed: %1 MB/s")
                            .arg(QString::number(speed, 'f', 1)));
                    }
                    if (progress.taskBar && progress.taskBar->maximum() > 0) {
                        progress.taskBar->setValue(mb);
                    }
                    progress.statusLabel->setText(I18n::tr("Downloading build archive") + "\n"
                                                  + I18n::tr("Downloaded %1 MB").arg(mb));
                }, Qt::QueuedConnection);
            });
            if (!buildDownloader.downloadFile(buildDownloadUrl.toStdString(),
                                              repairZip.toUtf8().toStdString(),
                                              buildAuthHeader.toStdString())) {
                const QString errMsg = translateDownloaderError(QString::fromStdString(buildDownloader.lastError()));
                QMetaObject::invokeMethod(this, [progress, errMsg]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Verification"),
                                         errMsg.isEmpty() ? I18n::tr("Download failed.") : errMsg);
                }, Qt::QueuedConnection);
                return;
            }
            ++done;
            QMetaObject::invokeMethod(progress.dialog, [progress, done]() {
                progress.bar->setValue(done);
                if (progress.filesLabel) {
                    progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(done).arg(progress.bar->maximum()));
                }
                progress.statusLabel->setText(I18n::tr("Extracting build archive"));
            }, Qt::QueuedConnection);

            QString extractErr;
            if (!extractZip(repairZip, repairDir, &extractErr)) {
                QMetaObject::invokeMethod(this, [progress, extractErr]() {
                    progress.dialog->close();
                    progress.dialog->deleteLater();
                    QMessageBox::warning(nullptr, I18n::tr("Verification"),
                                         extractErr.isEmpty() ? I18n::tr("Failed to extract.") : extractErr);
                }, Qt::QueuedConnection);
                return;
            }
            ++done;
            QMetaObject::invokeMethod(progress.dialog, [progress, done]() {
                progress.bar->setValue(done);
                if (progress.filesLabel) {
                    progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(done).arg(progress.bar->maximum()));
                }
            }, Qt::QueuedConnection);

            if (restoreAllBuild) {
                ++done;
                QString copyErr;
                if (!copyDirRecursive(repairDir, rootPath, &copyErr)) {
                    QMetaObject::invokeMethod(this, [progress, copyErr]() {
                        progress.dialog->close();
                        progress.dialog->deleteLater();
                        QMessageBox::warning(nullptr, I18n::tr("Verification"),
                                             copyErr.isEmpty() ? I18n::tr("Failed to write file.") : copyErr);
                    }, Qt::QueuedConnection);
                    return;
                }
                QMetaObject::invokeMethod(progress.dialog, [progress, done]() {
                    progress.statusLabel->setText(I18n::tr("Restoring build files"));
                    progress.bar->setValue(done);
                    if (progress.filesLabel) {
                        progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(done).arg(progress.bar->maximum()));
                    }
                }, Qt::QueuedConnection);
            } else {
                for (const auto &entry : buildRepairEntries) {
                    ++done;
                    const QString src = QDir(repairDir).filePath(entry.relPath);
                    const QString dst = QDir(rootPath).filePath(entry.relPath);
                    ensureDirForFile(dst);
                    if (QFileInfo::exists(src)) {
                        QFile::remove(dst);
                        QFile::copy(src, dst);
                    }
                    QMetaObject::invokeMethod(progress.dialog, [progress, done, dst]() {
                        progress.statusLabel->setText(I18n::tr("Restoring build files") + "\n" + QDir::toNativeSeparators(dst));
                        progress.bar->setValue(done);
                        if (progress.filesLabel) {
                            progress.filesLabel->setText(I18n::tr("Files: %1 / %2").arg(done).arg(progress.bar->maximum()));
                        }
                    }, Qt::QueuedConnection);
                }
            }

            QDir(repairDir).removeRecursively();
            QFile::remove(repairZip);
        }

        QMetaObject::invokeMethod(this, [progress, missing, corrupted, buildMissing, buildCorrupted, restoreAllBuild]() {
            progress.dialog->close();
            progress.dialog->deleteLater();
            QString msg = I18n::tr("Verification finished") + "\n\n";
            if (!missing.isEmpty()) {
                msg += I18n::tr("Missing files: %1").arg(missing.size()) + "\n";
            }
            if (!corrupted.isEmpty()) {
                msg += I18n::tr("Corrupted files: %1").arg(corrupted.size()) + "\n";
            }
            if (!buildMissing.isEmpty()) {
                msg += I18n::tr("Missing build files: %1").arg(buildMissing.size()) + "\n";
            }
            if (!buildCorrupted.isEmpty()) {
                msg += I18n::tr("Corrupted build files: %1").arg(buildCorrupted.size()) + "\n";
            }
            msg += "\n" + I18n::tr("Missing files were downloaded.");
            if (!buildMissing.isEmpty() || !buildCorrupted.isEmpty() || restoreAllBuild) {
                msg += "\n" + I18n::tr("Build files were restored.");
            }
            showSystemNotification(I18n::tr("Verification"), msg.trimmed());
        }, Qt::QueuedConnection);
    }).detach();
}

void MainWindow::onImportBuildClicked()
{
    const QString zipPath = QFileDialog::getOpenFileName(this,
                                                         I18n::tr("Import build"),
                                                         QDir::homePath(),
                                                         "ZIP (*.zip)");
    if (zipPath.isEmpty()) {
        return;
    }
    QFileInfo info(zipPath);
    const QString folderName = safeFolderName(info.completeBaseName());
    QDir appDir(QCoreApplication::applicationDirPath());
    const QString destPath = appDir.filePath(folderName);
    if (QDir(destPath).exists()) {
        if (!askYesNo(this, I18n::tr("Import build"),
                      I18n::tr("Delete local files for this build?\n\n%1").arg(folderName))) {
            return;
        }
        QDir(destPath).removeRecursively();
    }

    QString err;
    if (!extractZip(zipPath, destPath, &err)) {
        QMessageBox::warning(this, I18n::tr("Import build"),
                             err.isEmpty() ? I18n::tr("Build import failed.") : err);
        return;
    }

    QJsonObject meta;
    if (!readBuildMeta(destPath, meta)) {
        QMessageBox::warning(this, I18n::tr("Import build"), I18n::tr("Build import failed."));
        return;
    }

    BuildInfo build;
    build.id = meta.value("id").toString().toStdString();
    build.name = meta.value("name").toString().toStdString();
    build.description = meta.value("description").toString().toStdString();
    build.minecraftVersion = meta.value("minecraftVersion").toString().toStdString();
    build.loader = meta.value("loader").toString().toStdString();
    build.javaVersion = meta.value("javaVersion").toInt();
    build.checksum = meta.value("checksum").toString().toStdString();
    build.sizeBytes = static_cast<long long>(meta.value("sizeBytes").toDouble(0));
    build.imageUrl = meta.value("imageUrl").toString().toStdString();
    build.tagsCsv = meta.value("tags").toString().toStdString();
    build.isFree = true;
    build.locked = false;

    if (build.id.empty() || build.name.empty()) {
        QMessageBox::warning(this, I18n::tr("Import build"), I18n::tr("Build import failed."));
        return;
    }

    ConfigManager config;
    config.saveBuild(build);
    loadBuilds();
    showSystemNotification(I18n::tr("Import build"), I18n::tr("Build imported."));
}

void MainWindow::onRemoveClicked()
{
    int index = currentBuildIndex();
    if (index < 0 || index >= static_cast<int>(builds.size())) {
        QMessageBox::warning(this, I18n::tr("Delete from device"), I18n::tr("Select a build first."));
        return;
    }

    const auto build = builds[static_cast<size_t>(index)];
    const QString appDirPath = QCoreApplication::applicationDirPath();
    const QString rootPath = resolveBuildRoot(build, appDirPath);
    const bool installed = !rootPath.isEmpty();
    const bool outdated = installed && isBuildOutdated(build, rootPath);

    if (outdated && !isBuildLocked(build)) {
        if (!askYesNo(this, I18n::tr("Update"),
                      I18n::tr("Update this build? It will re-download all files.\n\n%1")
                          .arg(QString::fromUtf8(build.name.c_str())))) {
            return;
        }
        if (QDir(rootPath).exists()) {
            QDir(rootPath).removeRecursively();
        }
    }
    const QString name = QString::fromUtf8(build.name.c_str());
    if (!askYesNo(this, I18n::tr("Delete from device"),
                  I18n::tr("Delete local files for this build?\n\n%1").arg(name))) {
        return;
    }

    QDir appDir(QCoreApplication::applicationDirPath());
    bool deleted = false;
    QString folderName = safeFolderName(name);
    QString folderPath = appDir.filePath(folderName);
    if (QDir(folderPath).exists()) {
        QDir folder(folderPath);
        deleted = folder.removeRecursively();
    } else if (!build.id.empty()) {
        folderName = safeFolderName(QString::fromStdString(build.id));
        folderPath = appDir.filePath(folderName);
        if (QDir(folderPath).exists()) {
            QDir folder(folderPath);
            deleted = folder.removeRecursively();
        }
    }
    Q_UNUSED(deleted);
    updateDetails(index);
}

void MainWindow::onRemoveFromListClicked()
{
    int index = currentBuildIndex();
    if (index < 0 || index >= static_cast<int>(builds.size())) {
        QMessageBox::warning(this, I18n::tr("Remove from my builds"), I18n::tr("Select a build first."));
        return;
    }

    const auto build = builds[static_cast<size_t>(index)];
    const QString name = QString::fromUtf8(build.name.c_str());
    if (!askYesNo(this, I18n::tr("Remove from my builds"),
                  I18n::tr("Remove this build from your list?\n\n%1").arg(name))) {
        return;
    }

    ConfigManager config;
    config.removeBuild(build.id);
    loadBuilds();
}

 
