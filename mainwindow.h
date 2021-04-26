#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "settingsdialog.h"
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QMap>
#include <QDateTime>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

enum TreeItemType {
    Machine = QTreeWidgetItem::UserType,
    Directory = QTreeWidgetItem::UserType + 1
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionMy_Account_triggered();


    void login();
    void requestUserGroups();
    void requestDirsAndMachines();
    void requestMachineDetails(int id);
    void requestTunnelListForMachine(int id);

    void updateCurrentlySelectedObject();

private:

    QTreeWidgetItem *processDirs(QJsonObject dir);

    Ui::MainWindow *ui;
    SettingsDialog *ad {new SettingsDialog(this)};
    QNetworkAccessManager *n {new QNetworkAccessManager(this)};

    QString authBearer {""};
    QByteArray authHeaderValue;

    QTimer *refreshTick{new QTimer(this)};

    QMap<QListWidgetItem*, int> userGroupItemToIDMap;
    QMap<QTreeWidgetItem*, int> machineItemToIDMap;
    QMap<QTreeWidgetItem*, int> directoryItemToIDMap;
    QMap<QListWidgetItem*, int> tunnelItemToIDMap;
};
static QByteArray authHeaderName {QByteArray::fromStdString("Authorization")};
#endif // MAINWINDOW_H
