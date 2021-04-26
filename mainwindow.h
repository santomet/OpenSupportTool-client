#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "settingsdialog.h"
#include "machinecreatedialog.h"
#include <QNetworkAccessManager>
#include <QMessageBox>
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

    void on_actionMachineCreate_triggered();

    void on_actionMove_triggered();

    void on_actionCreate_Directory_triggered();

    void on_actionDestroy_triggered();

    void on_actionRequest_a_Tunnel_triggered();

    void on_actionDestroy_the_Tunnel_triggered();

private:

    QTreeWidgetItem *processDirs(QJsonObject dir);

    Ui::MainWindow *ui;
    QNetworkAccessManager *n {new QNetworkAccessManager(this)};
    SettingsDialog *ad {new SettingsDialog(this)};
    MachineCreateDialog *mcd {new MachineCreateDialog(n, this)};

    QString authBearer {""};
    QByteArray authHeaderValue;

    QTimer *refreshTick{new QTimer(this)};

    QMap<QListWidgetItem*, int> userGroupItemToIDMap;
    QMap<QTreeWidgetItem*, int> machineItemToIDMap;
    QMap<QTreeWidgetItem*, int> directoryItemToIDMap;
    QMap<QListWidgetItem*, int> tunnelItemToIDMap;
};
#endif // MAINWINDOW_H
