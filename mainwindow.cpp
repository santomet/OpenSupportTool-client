#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ad, &QDialog::accepted, this, &MainWindow::login);
    connect(ui->listWidgetUserGroups, &QListWidget::itemSelectionChanged, this, &MainWindow::requestDirsAndMachines);
    connect(ui->treeWidgetMachines, &QTreeWidget::itemSelectionChanged, this, &MainWindow::updateCurrentlySelectedObject);
    connect(ui->listWidgetTunnels, &QListWidget::itemSelectionChanged, this, &MainWindow::updateSSHCommand);


    connect(ui->lineEditDirectoryName, &QLineEdit::textChanged, [=](QString str) {ui->toolButtonDirectoryCreate->setEnabled(
                    !str.isEmpty() && ui->treeWidgetMachines->currentItem() && ui->treeWidgetMachines->currentItem()->type() == Directory
                    );});


    //refresh tick
    connect(refreshTick, &QTimer::timeout, this, &MainWindow::updateCurrentlySelectedObject);
    refreshTick->start(1000); //every second or so

    ui->toolButtonCreateTunnel->setDefaultAction(ui->actionRequest_a_Tunnel);
    ui->toolButtonDestroyTunnel->setDefaultAction(ui->actionDestroy_the_Tunnel);
    ui->toolButtonDirectoryCreate->setDefaultAction(ui->actionCreate_Directory);
    ui->toolButtonMachineCreate->setDefaultAction(ui->actionMachineCreate);
    ui->toolButtonMove->setDefaultAction(ui->actionMove);
    ui->toolButtonMove->hide();
    ui->toolButtonRemove->setDefaultAction(ui->actionDestroy);

    ui->toolButtonDirectoryCreate->setEnabled(false);

    //add something to the lifespan selector
    ui->comboBoxLifespan->addItem("15 Minutes", 900);
    ui->comboBoxLifespan->addItem("1 Hour", 3600);
    ui->comboBoxLifespan->addItem("5 Hours", 18000);
    ui->comboBoxLifespan->addItem("24 Hours", 86400);


    login();


}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_actionMy_Account_triggered()
{
    ad->exec();
}

void MainWindow::login()
{
    QSettings s;
    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/users/login");
//    QUrlQuery query;
//    query.addQueryItem("username", s.value("username", "").toString());
//    query.addQueryItem("password", s.value("password", "").toString());
//    query.addQueryItem("grant_type", "");
//    query.addQueryItem("scope", "");
//    query.addQueryItem("client_id", "");
//    query.addQueryItem("client_secret", "");
//    url.setQuery(query);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QString data = "grant_type=&username=" +s.value("username", "").toString() + "&password=" + s.value("password", "").toString() + "&scope=&client_id=&client_secret=";

//    QJsonObject obj;
//    obj.insert("grant_type", "");
//    obj.insert("username", s.value("username", "").toString());
//    obj.insert("password", s.value("password", "").toString());
//    obj.insert("scope", "");
//    obj.insert("client_id", "");
//    obj.insert("client_secret", "");

    QNetworkReply *reply = n->post(request, data.toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonObject obj = doc.object();
            authBearer = obj.value("access_token").toString();
            qDebug() << authBearer;
            if(!authBearer.isEmpty()) {
                authBearer = "Bearer " + authBearer;
                authHeaderValue = authBearer.toUtf8();
                requestUserGroups();
            }
            ad->setErrorMessage("");
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            ad->setErrorMessage("Wrong settings!");
            ad->exec();
        }
        reply->deleteLater();
    });
}

void MainWindow::requestUserGroups()
{
    //LOAD user groups
    QSettings s;
    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/users/user_group_list");
    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    QNetworkReply *reply = n->get(request);

    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonArray arr = doc.array();
            ui->listWidgetUserGroups->clear();
            userGroupItemToIDMap.clear();
            for(const QJsonValue &o : arr){
                QJsonObject obj = o.toObject();
                ui->listWidgetUserGroups->addItem(obj.value("name").toString());
                QListWidgetItem *it = ui->listWidgetUserGroups->item(ui->listWidgetUserGroups->count() - 1);
                userGroupItemToIDMap.insert(it, obj.value("id").toInt());
            }
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                login();
            }
        }
        reply->deleteLater();
    });
}

void MainWindow::requestDirsAndMachines()
{
    //Loads machines as a view from the selected user groups
    //which ID?
    int ugid = userGroupItemToIDMap.value(ui->listWidgetUserGroups->currentItem());
    QSettings s;
    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/machines/structured");
    QUrlQuery query;
    query.addQueryItem("user_group_id", QString::number(ugid));
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    QNetworkReply *reply = n->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonArray arr = doc.array();
            ui->treeWidgetMachines->clear();
            machineItemToIDMap.clear();
            directoryItemToIDMap.clear();
            for(const QJsonValue &o : arr){
                ui->treeWidgetMachines->addTopLevelItem( processDirs(o.toObject()));
            }
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                login();
            }
        }
        reply->deleteLater();
    });

}

void MainWindow::requestMachineDetails(int id)
{
    QSettings s;
    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/machines/get_machine");
    QUrlQuery query;
    query.addQueryItem("machine_id", QString::number(id));
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    QNetworkReply *reply = n->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonObject mach = doc.object();
            ui->machineNameLabel->setText(mach.value("title").toString());
            ui->machineDescriptionLabel->setText(mach.value("description").toString());
            ui->labelCpuUsage->setText(QString::number(mach.value("last_cpu_percent").toDouble(), 'f', 1) + "%");
            ui->labelRamUsage->setText(QString::number(mach.value("last_memory_percent").toDouble(), 'f', 1) + "%");
            ui->labelUsername->setText(mach.value("agent_user_name").toString());
            QDateTime lastAlive = QDateTime::fromString(mach.value("last_query_datetime").toString(), Qt::ISODateWithMs);
            lastAlive.setTimeSpec(Qt::UTC);
            ui->labelLastAlive->setText(QString::number(lastAlive.secsTo(QDateTime::currentDateTimeUtc())) + " secs ago");
            ui->toolButtonCreateTunnel->setEnabled(true);
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                login();
            }
        }
        reply->deleteLater();
    });
}

void MainWindow::requestTunnelListForMachine(int id)
{
    QSettings s;
    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/tunnels/list_tunnels_for_machine");

        QJsonObject obj;
        obj.insert("machine_id", id);
        obj.insert("connection_states", QJsonArray({2}));

    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = n->post(request, QJsonDocument(obj).toJson());
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonArray tarr = doc.array();
            //ui->listWidgetTunnels->clear();
            //tunnelItemToIDMap.clear(); //no need to clear this actually
            QList<QListWidgetItem*> toKeep;
            for(const QJsonValue &v : tarr) {
                QJsonObject t = v.toObject();
                int tid = t.value("id").toInt();
                QString itemstring = "Port ";
                itemstring += QString::number(t.value("port_to_tunnel").toInt());
                itemstring += " on ";
                itemstring += t.value("remote_ssh_server").toString();
                itemstring += ":";
                itemstring += QString::number(t.value("reverse_port").toInt());

                QDateTime exptime = QDateTime::fromString(t.value("timeout_time").toString(), Qt::ISODateWithMs);
                exptime.setTimeSpec(Qt::UTC);
                QString expiresin = QString::number(QDateTime::currentDateTimeUtc().secsTo(exptime));

                itemstring += " expires in " + expiresin + " s";

                if(tunnelItemToIDMap.values().contains(tid)) {
                    QListWidgetItem *it = tunnelItemToIDMap.key(tid);
                    if(it) {
                        it->setText(itemstring);
                    }
                    toKeep.append(it);
                }
                else {
                    ui->listWidgetTunnels->addItem(itemstring);
                    QListWidgetItem *it = ui->listWidgetTunnels->item(ui->listWidgetTunnels->count() - 1);
                    tunnelItemToIDMap.insert(it, tid);
                    toKeep.append(it);
                    QSettings s;
                    int vncport = s.value("VNCPort", 5950).toInt();
                    int socksport = s.value("SOCKSPort", 9050).toInt();


                    if(t.value("port_to_tunnel").toInt() == 22) {
                        QString sshCommand = "ssh ";
                        sshCommand += ui->labelUsername->text();
                        sshCommand += "@";
                        sshCommand += t.value("remote_ssh_server").toString();
                        sshCommand += " -p ";
                        sshCommand += QString::number(t.value("reverse_port").toInt());
                        sshCommand += QString(" -L %1:localhost:%2").arg(vncport).arg(5900);
                        sshCommand += QString(" -D %1").arg(socksport);

                        tunnelItemToCommand.insert(it, sshCommand);
                    }
                    else {
                        QString urladdress = "http://" + t.value("remote_ssh_server").toString() + ":" + QString::number(t.value("reverse_port").toInt());
                        tunnelItemToCommand.insert(it, urladdress);
                    }
                }


            }
            for(QListWidgetItem *it : tunnelItemToIDMap.keys()) {
                if(!toKeep.contains(it)) {
                    ui->listWidgetTunnels->takeItem(ui->listWidgetTunnels->row(it));
                    tunnelItemToIDMap.remove(it);
                    tunnelItemToCommand.remove(it);
                }
            }
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                login();
            }
        }
        reply->deleteLater();
    });
}

void MainWindow::updateSSHCommand()
{
    QListWidgetItem *item = ui->listWidgetTunnels->currentItem();
    if(!item || !tunnelItemToCommand.contains(item)) {
        ui->labelSSHcommand->setText("");
        return;
    }
    ui->labelSSHcommand->setText(tunnelItemToCommand.value(item));
}


void MainWindow::updateCurrentlySelectedObject()
{
    QTreeWidgetItem *item = ui->treeWidgetMachines->currentItem();
    if(!item) {
        return;
    }
    switch(item->type()) {
    case Machine:
    {
        //disable/enable buttons
        ui->actionCreate_Directory->setEnabled(false);
        ui->actionMachineCreate->setEnabled(false);
        ui->actionDestroy->setEnabled(true);
        ui->actionMove->setEnabled(true);
        ui->stackedWidget->setCurrentIndex(1);
        int id = machineItemToIDMap.value(item, -1);
        if(id < 0) {
            qDebug() << "Error! machine does not exist";
            return;
        }

        requestMachineDetails(id);
        requestTunnelListForMachine(id);

    }
        break;
    case Directory:
        ui->actionCreate_Directory->setEnabled(true);
        ui->actionMachineCreate->setEnabled(true);
        ui->actionDestroy->setEnabled(true);
        ui->actionMove->setEnabled(true);
        ui->stackedWidget->setCurrentIndex(0);
    default:
        break;

    }

}

QTreeWidgetItem *MainWindow::processDirs(QJsonObject dir)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(Directory);

    if(!dir.contains("machines")) {
        qDebug() << "Not a valid direcotry!";
        return nullptr;
    }

    item->setText(0, dir.value("name").toString());
    directoryItemToIDMap.insert(item, dir.value("id").toInt());
    item->setIcon(0, QIcon(":/icons/res/folder.svg"));

    //first add children
    for(const QJsonValue &ch : dir.value("children").toArray()) {
        item->addChild(processDirs(ch.toObject()));
    }

    //add machines
    for (const QJsonValue &m : dir.value("machines").toArray()) {
        QJsonObject mo = m.toObject();
        QTreeWidgetItem *mi = new QTreeWidgetItem(item, Machine);
        mi->setText(0, mo.value("title").toString());
        machineItemToIDMap.insert(mi, mo.value("id").toInt());
        mi->setIcon(0, QIcon(":/icons/res/machine.svg"));
    }

    return item;

}



void MainWindow::on_actionMachineCreate_triggered()
{
    mcd->reset();
    QTreeWidgetItem *item = ui->treeWidgetMachines->currentItem();
    if(!item) {
        return;
    }
    mcd->preset(authHeaderValue, item->text(0), directoryItemToIDMap.value(item));

    mcd->exec();
    requestDirsAndMachines();
}

void MainWindow::on_actionMove_triggered()
{

}

void MainWindow::on_actionCreate_Directory_triggered()
{
    QTreeWidgetItem *item = ui->treeWidgetMachines->currentItem();
    if(!item) {
        return;
    }
    if(item->type() != Directory) {
        return;
    }
    if(ui->lineEditDirectoryName->text().isEmpty()) {
        return;
    }
    QSettings s;
    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/machines/add_directory");
    QUrlQuery query;
    query.addQueryItem("name", ui->lineEditDirectoryName->text());
    query.addQueryItem("parent_id", QString::number(directoryItemToIDMap.value(item)));
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    QNetworkReply *reply = n->get(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            requestDirsAndMachines();
//            QTreeWidgetItem *newit = new QTreeWidgetItem(item, Directory);
//            newit->setText(0, ui->lineEditDirectoryName->text());
//            newit->setIcon(0, QIcon(":/icons/res/folder.svg"));

        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                login();
                return;
            }
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonObject obj = doc.object();
            if(obj.contains("detail")) {
                QMessageBox::critical(this, "API Error", obj.value("detail").toString());
            }
        }
        reply->deleteLater();
    });


}

void MainWindow::on_actionDestroy_triggered()
{
    QTreeWidgetItem *item = ui->treeWidgetMachines->currentItem();
    if(!item) {
        return;
    }
    QString text;
    QString apiadd;
    QUrlQuery query;
    switch(item->type()) {
    case Machine:
        text = ("Are you sure you want to destroy machine " + item->text(0) + "?");
        apiadd = "/machines/remove_machine";
        query.addQueryItem("machine_id", QString::number(machineItemToIDMap.value(item)));
        break;
    case Directory:
        text = ("Are you sure you want to remove directory " + item->text(0) + "?");
        apiadd = "/machines/remove_directory";
        query.addQueryItem("directory_id", QString::number(directoryItemToIDMap.value(item)));
        break;
    default:
        break;
    }

    int ret = QMessageBox::warning(this, "Deleting", text, QMessageBox::Ok | QMessageBox::Abort);
    if(ret == QMessageBox::Abort) {
        return;
    }

    QSettings s;
    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + apiadd);
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    QNetworkReply *reply = n->deleteResource(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            requestDirsAndMachines();
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                login();
                return;
            }
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonObject obj = doc.object();
            if(obj.contains("detail")) {
                QMessageBox::critical(this, "API Error", obj.value("detail").toString());
            }
        }
        reply->deleteLater();
    });


}

void MainWindow::on_actionRequest_a_Tunnel_triggered()
{
    if(ui->spinBoxPort->value() == 5900) {
        QMessageBox::warning(this, "VNC Warning", "Opening a port directly to VNC would enable anyone to connect without authentication. Please, use the recommended procedure.");
        return;
    }

    QTreeWidgetItem *item = ui->treeWidgetMachines->currentItem();
    if(!item || item->type() != Machine) {
        return;
    }
    int mid = machineItemToIDMap.value(item);
    QSettings s;
    QJsonObject obj;
    obj.insert("machine_id", mid);
    obj.insert("port_to_tunnel", ui->spinBoxPort->value());
    obj.insert("connection_type", 0); //We only know SSH connection right now :(
    obj.insert("temporary_ssh_pubkey", s.value("SSHKey").toString());
    obj.insert("timeout_seconds", ui->comboBoxLifespan->currentData().toInt());
    QJsonDocument doc(obj);

    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/tunnels/request_tunnel");
    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    QNetworkReply *reply = n->post(request, doc.toJson());
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            ui->toolButtonCreateTunnel->setEnabled(false);
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                login();
                return;
            }
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonObject obj = doc.object();
            if(obj.contains("detail")) {
                QMessageBox::critical(this, "API Error", obj.value("detail").toString());
            }
        }
        reply->deleteLater();
    });
}

void MainWindow::on_actionDestroy_the_Tunnel_triggered()
{
    QListWidgetItem *item = ui->listWidgetTunnels->currentItem();
    if(!item) {
        return;
    }
    int tid = tunnelItemToIDMap.value(item);

    int ret = QMessageBox::warning(this, "Destroying tunnel", "Are You sure you want to destroy the tunnel?", QMessageBox::Abort|QMessageBox::Ok);
    if(ret == QMessageBox::Abort) {
        return;
    }

    QSettings s;

    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/tunnels/destroy_tunnel");
    QUrlQuery query;
    query.addQueryItem("id", QString::number(tid));
    url.setQuery(query);
    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    QNetworkReply *reply = n->deleteResource(request);
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            //do nothing here
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                login();
                return;
            }
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonObject obj = doc.object();
            if(obj.contains("detail")) {
                QMessageBox::critical(this, "API Error", obj.value("detail").toString());
            }
        }
        reply->deleteLater();
    });

}

void MainWindow::on_pushButton_clicked()
{
    clipboard->setText(ui->labelSSHcommand->text());
}
