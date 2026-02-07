#ifndef CATALOG_DIALOG_H
#define CATALOG_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QVector>

#include "../core/build_info.h"

class CatalogDialog : public QDialog
{
    Q_OBJECT

public:
    CatalogDialog(const QString &apiBaseUrl, const QString &accessToken, bool adminUser, QWidget *parent = nullptr);

    BuildInfo selectedBuild() const;

private slots:
    void onSelectionChanged(int index);
    void onAddClicked();

private:
    void setupUI();
    void loadCatalog();
    void populateList();
    bool isBuildLocked(const BuildInfo &build) const;

    QString apiBaseUrl;
    QString accessToken;
    bool adminUser = false;

    QListWidget *catalogList;
    QPushButton *addButton;
    BuildInfo chosenBuild;

    QVector<BuildInfo> catalog;
};

#endif // CATALOG_DIALOG_H
