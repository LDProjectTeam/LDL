#include "catalog_dialog.h"
#include "flow_layout.h"
#include "i18n.h"
#include "notification.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QEventLoop>
#include <QFont>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QFrame>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPointer>
#include <QPainterPath>
#include <QRegion>
#include <QScrollBar>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QTimer>
#include <QSettings>

namespace {

QString toUtf8String(const std::string &value)
{
    return QString::fromUtf8(value.c_str());
}

QString buildSizeText(const BuildInfo &build)
{
    if (build.sizeBytes > 0) {
        return QString::number(build.sizeBytes / 1000000);
    }
    return I18n::tr("Unknown");
}

std::string toUtf8StdString(const QString &value)
{
    QByteArray bytes = value.toUtf8();
    return std::string(bytes.constData(), static_cast<size_t>(bytes.size()));
}

QString apiBaseFromEnv(const QString &fallback)
{
    QString env = qEnvironmentVariable("LAUNCHER_API_URL");
    if (env.isEmpty()) {
        return fallback;
    }
    return env;
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

class BuildCardWidget : public QWidget
{
public:
    BuildCardWidget(const BuildInfo &build, bool lockedForUser, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setObjectName("BuildCard");
        setProperty("selected", false);
        setAttribute(Qt::WA_Hover, true);

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 12, 12, 12);
        layout->setSpacing(12);

        imageLabel = new QLabel(this);
        imageLabel->setFixedSize(110, 82);
        imageLabel->setStyleSheet("background-color: #263043; border-radius: 10px;");
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setText(I18n::tr("No image"));

        QVBoxLayout *textLayout = new QVBoxLayout();
        textLayout->setSpacing(4);

        QLabel *nameLabel = new QLabel(toUtf8String(build.name), this);
        QFont nameFont("Montserrat", 11, QFont::Bold);
        nameLabel->setFont(nameFont);

        const QString loaderText =
            build.loader.empty() ? I18n::tr("Loader: ?") : I18n::tr("Loader: %1").arg(QString::fromStdString(build.loader));
        QString priceText = build.isFree
            ? I18n::tr("Free")
            : I18n::tr("Paid $%1").arg(build.priceCents / 100.0, 0, 'f', 2);
        if (lockedForUser) {
            priceText += " (" + I18n::tr("Locked") + ")";
        }
        QLabel *metaLabel = new QLabel(
            I18n::tr("%1 MB | Minecraft %2 | %3 | Java %4")
                .arg(buildSizeText(build))
                .arg(QString::fromStdString(build.minecraftVersion))
                .arg(loaderText)
                .arg(build.javaVersion),
            this);
        QLabel *priceLabel = new QLabel(priceText, this);
        priceLabel->setObjectName("BuildPrice");
        metaLabel->setObjectName("BuildMeta");

        QWidget *tagsWidget = new QWidget(this);
        auto *tagsLayout = new FlowLayout(tagsWidget, 0, 6, 6);
        tagsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        QStringList tags = toUtf8String(build.tagsCsv).split(',', Qt::SkipEmptyParts);
        if (tags.isEmpty()) {
            QLabel *tag = new QLabel(I18n::tr("No tags"), this);
            tag->setStyleSheet("color: #7d88a6;");
            tagsLayout->addWidget(tag);
        } else {
            for (const auto &tagText : tags) {
                QFrame *pill = new QFrame(this);
                pill->setObjectName("TagPill");
                pill->setAttribute(Qt::WA_StyledBackground, true);
                pill->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
                pill->setMinimumHeight(20);
                pill->setStyleSheet(
                    "QFrame#TagPill { background-color: #283347; border: 1px solid #34445f; border-radius: 10px; }"
                    "QLabel#TagText { color: #d3def7; }");
                pill->installEventFilter(new PopupRounder(10, pill));
                auto *pillLayout = new QHBoxLayout(pill);
                pillLayout->setContentsMargins(8, 2, 8, 2);
                QLabel *tag = new QLabel(tagText.trimmed(), pill);
                tag->setObjectName("TagText");
                pillLayout->addWidget(tag);
                tagsLayout->addWidget(pill);
            }
        }

        textLayout->addWidget(nameLabel);
        textLayout->addWidget(metaLabel);
        textLayout->addWidget(priceLabel);
        textLayout->addWidget(tagsWidget);

        layout->addWidget(imageLabel);
        layout->addLayout(textLayout, 1);

        setStyleSheet(R"QSS(
            QWidget#BuildCard {
                background-color: rgba(26, 34, 49, 200);
                border: 1px solid #32435f;
                border-radius: 12px;
            }
            QWidget#BuildCard[selected="true"] {
                border: 1px solid #5aa9e6;
                background-color: rgba(31, 40, 58, 215);
            }
            QWidget#BuildCard:hover {
                border: 1px solid #6bb7f0;
                background-color: rgba(33, 44, 64, 210);
            }
            QLabel#BuildMeta {
                color: #9fb0d6;
            }
            QLabel#BuildPrice {
                color: #dce6ff;
            }
            QFrame#TagPill {
                background-color: #283347;
                border: 1px solid #34445f;
                border-radius: 12px;
            }
            QLabel#TagText {
                color: #d3def7;
            }
        )QSS");
        setMinimumHeight(120);
    }

    void setImage(const QPixmap &pixmap)
    {
        if (!pixmap.isNull()) {
            imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
            imageLabel->setText("");
        }
    }

private:
    QLabel *imageLabel;
};

QString extractDetail(const QByteArray &body)
{
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        return {};
    }
    return doc.object().value("detail").toString();
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

QString friendlyCatalogError(QNetworkReply *reply, const QByteArray &body)
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
    if (status == 401 || status == 403) {
        return I18n::tr("Access denied. Please log in and try again.");
    }
    if (status == 404) {
        return I18n::tr("Build not found.");
    }
    if (status == 422) {
        return I18n::tr("Invalid request. Please try again.");
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
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusOut) {
            *statusOut = statusCode;
        }
        if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
            if (errOut) {
                if (statusCode == 401 || statusCode == 403) {
                    *errOut = I18n::tr("Access denied. Please log in and try again.");
                } else {
                    QString detail = extractDetail(body);
                    *errOut = detail.isEmpty() ? reply->errorString() : detail;
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

} // namespace

CatalogDialog::CatalogDialog(const QString &apiBaseUrl, const QString &accessToken, bool adminUser, QWidget *parent)
    : QDialog(parent), apiBaseUrl(apiBaseUrl), accessToken(accessToken), adminUser(adminUser)
{
    if (this->apiBaseUrl.isEmpty()) {
        this->apiBaseUrl = apiBaseFromEnv("http://127.0.0.1:8000");
    }

    setWindowTitle(I18n::tr("Build Catalog"));
    resize(900, 620);
    setModal(true);

    setupUI();
    QTimer::singleShot(0, this, [this]() { applyFadeIn(this, 200); });
    loadCatalog();
}

BuildInfo CatalogDialog::selectedBuild() const
{
    return chosenBuild;
}

void CatalogDialog::setupUI()
{
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    QLabel *title = new QLabel(I18n::tr("Select a build"), this);
    QFont titleFont("Montserrat", 16, QFont::Bold);
    title->setFont(titleFont);
    root->addWidget(title);

    catalogList = new QListWidget(this);
    catalogList->setSelectionMode(QAbstractItemView::SingleSelection);
    catalogList->setSpacing(8);
    catalogList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    catalogList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    catalogList->setFrameShape(QFrame::NoFrame);
    catalogList->setFocusPolicy(Qt::NoFocus);
    connect(catalogList, &QListWidget::currentRowChanged, this, &CatalogDialog::onSelectionChanged);

    root->addWidget(catalogList, 1);

    QHBoxLayout *actions = new QHBoxLayout();
    actions->addStretch();
    addButton = new QPushButton(I18n::tr("Add"), this);
    addButton->setEnabled(false);
    connect(addButton, &QPushButton::clicked, this, &CatalogDialog::onAddClicked);
    actions->addWidget(addButton);
    root->addLayout(actions);

    setStyleSheet(R"QSS(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #151c29, stop:1 #1c2636);
            color: #e8ecf2;
            border: 1px solid #2f3b52;
            border-radius: 12px;
        }
        QListWidget {
            background: transparent;
            border: none;
        }
        QListWidget::item {
            background: transparent;
            border: none;
            padding: 0;
        }
        QListWidget::item:selected {
            background: transparent;
            border: none;
        }
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #5aa9e6, stop:1 #7bc0f0);
            border: none;
            border-radius: 10px;
            padding: 8px 16px;
            color: #0b1020;
            font-weight: bold;
        }
        QPushButton:disabled {
            background-color: #2a3144;
            color: #7d88a6;
        }
    )QSS");
}

void CatalogDialog::loadCatalog()
{
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(apiBaseUrl + "/builds"));
    if (!accessToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    }

    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray body = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
        QMessageBox::warning(this, I18n::tr("Build Catalog"), friendlyCatalogError(reply, body));
        reply->deleteLater();
        return;
    }
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isArray()) {
        QMessageBox::warning(this, I18n::tr("Build Catalog"), I18n::tr("Invalid catalog response."));
        return;
    }

    catalog.clear();
    QJsonArray arr = doc.array();
    for (const auto &val : arr) {
        if (!val.isObject()) {
            continue;
        }
        QJsonObject obj = val.toObject();
        BuildInfo build;
        build.id = obj.value("id").toString().toStdString();
        build.name = toUtf8StdString(obj.value("name").toString());
        build.description = toUtf8StdString(obj.value("description").toString());
        build.minecraftVersion = obj.value("minecraftVersion").toString().toStdString();
        build.loader = obj.value("loader").toString().toStdString();
        build.javaVersion = obj.value("javaVersion").toInt();
        build.sizeBytes = static_cast<long long>(obj.value("sizeBytes").toDouble());
        build.checksum = obj.value("checksum").toString().toStdString();
        build.imageUrl = obj.value("imageUrl").toString().toStdString();
        build.githubRepo = obj.value("githubRepo").toString().toStdString();
        build.githubAsset = obj.value("githubAsset").toString().toStdString();
        build.isFree = obj.value("isFree").toBool(true);
        build.priceCents = obj.value("priceCents").toInt(0);
        build.locked = obj.value("locked").toBool(false);
        if (adminUser) {
            build.locked = false;
        }

        if (obj.contains("tags") && obj.value("tags").isArray()) {
            QStringList tags;
            for (const auto &tagVal : obj.value("tags").toArray()) {
                tags << tagVal.toString();
            }
            build.tagsCsv = tags.join(',').toStdString();
        } else {
            build.tagsCsv = obj.value("tags").toString().toStdString();
        }

        catalog.push_back(build);
    }

    populateList();
}

bool CatalogDialog::isBuildLocked(const BuildInfo &build) const
{
    return build.locked && !build.isFree && !adminUser;
}

void CatalogDialog::populateList()
{
    catalogList->clear();

    QNetworkAccessManager *imageManager = new QNetworkAccessManager(this);

    for (int i = 0; i < catalog.size(); ++i) {
        const auto &build = catalog[i];
        auto *item = new QListWidgetItem();
        catalogList->addItem(item);

        auto *widget = new BuildCardWidget(build, isBuildLocked(build), this);
        catalogList->setItemWidget(item, widget);
        item->setSizeHint(widget->sizeHint());

        QString img = resolveImageUrl(apiBaseUrl, build);
        if (!img.isEmpty()) {
            QNetworkRequest req{QUrl(img)};
            if (!accessToken.isEmpty()) {
                req.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
            }
            QNetworkReply *reply = imageManager->get(req);
            QPointer<BuildCardWidget> safeWidget(widget);
            connect(reply, &QNetworkReply::finished, this, [reply, safeWidget]() {
                QByteArray data = reply->readAll();
                QPixmap pix;
                pix.loadFromData(data);
                if (safeWidget) {
                    safeWidget->setImage(pix);
                }
                reply->deleteLater();
            });
        }
    }

    onSelectionChanged(-1);
}

void CatalogDialog::onSelectionChanged(int index)
{
    for (int i = 0; i < catalogList->count(); ++i) {
        auto *item = catalogList->item(i);
        auto *widget = catalogList->itemWidget(item);
        if (widget) {
            widget->setProperty("selected", i == index);
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
            widget->update();
        }
    }

    if (index < 0 || index >= catalog.size()) {
        addButton->setEnabled(false);
        return;
    }

    const auto &build = catalog[index];
    if (isBuildLocked(build)) {
        addButton->setEnabled(true);
        addButton->setText(I18n::tr("Buy"));
    } else {
        addButton->setEnabled(true);
        addButton->setText(I18n::tr("Add"));
    }
}

void CatalogDialog::onAddClicked()
{
    int index = catalogList->currentRow();
    if (index < 0 || index >= catalog.size()) {
        return;
    }

    const auto &build = catalog[index];
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
        showSystemNotification(I18n::tr("Buy"),
                               I18n::tr("Checkout opened in your browser.\nAfter payment, reopen the catalog to refresh access."));
        return;
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl(apiBaseUrl + "/builds/" + QString::fromStdString(build.id) + "/download"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!accessToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    }

    QNetworkReply *reply = manager.post(request, "{}");
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray body = reply->readAll();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || statusCode >= 400) {
        QMessageBox::warning(this, I18n::tr("Build Catalog"), friendlyCatalogError(reply, body));
        reply->deleteLater();
        return;
    }
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        QMessageBox::warning(this, I18n::tr("Build Catalog"), I18n::tr("Invalid download response."));
        return;
    }

    QJsonObject obj = doc.object();
    BuildInfo result = build;
    QString downloadUrl = obj.value("downloadUrl").toString();
    if (downloadUrl.startsWith('/')) {
        downloadUrl = apiBaseUrl + downloadUrl;
    }
    result.downloadUrl = downloadUrl.toStdString();
    result.checksum = obj.value("checksum").toString().toStdString();
    result.sizeBytes = static_cast<long long>(obj.value("sizeBytes").toDouble());

    chosenBuild = result;
    accept();
}
