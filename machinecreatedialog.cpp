#include "machinecreatedialog.h"
#include "ui_machinecreatedialog.h"

MachineCreateDialog::MachineCreateDialog(QNetworkAccessManager *n, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MachineCreateDialog),
    n(n)
{
    ui->setupUi(this);
}

MachineCreateDialog::~MachineCreateDialog()
{
    delete ui;
}

void MachineCreateDialog::preset(QByteArray authh, QString name, int id)
{
    directoryName = name;
    ui->labelDirectory->setText(name);
    this->id = id;
    authHeaderValue = authh;
}

void MachineCreateDialog::reset()
{
    ui->labelDirectory->setText("nothing");
    ui->labelInstallationUrl->setText("");
    ui->lineEditName->setText("");
    ui->plainTextEditDescription->setPlainText("");
    ui->labelError->setText("");
    ui->pushButtonCreate->setEnabled(true);
}

void MachineCreateDialog::on_pushButtonCreate_clicked()
{
    if(ui->lineEditName->text().isEmpty()) {
        ui->labelError->setText("Please, do not leave the name empty");
        return;
    }
    if(id < 0) {
        ui->labelError->setText("The directory has not been set, this should not happend!");
        return;
    }
    QJsonObject mo;
    mo.insert("title", ui->lineEditName->text());
    mo.insert("description", ui->plainTextEditDescription->toPlainText());
    mo.insert("directory_id", this->id);

    qDebug() << mo;

    QSettings s;
    QString apibase = s.value("apiurl", "").toString();
    QUrl url(apibase + "/machines/add_machine");

    QNetworkRequest request(url);
    request.setRawHeader(authHeaderName, authHeaderValue);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QNetworkReply *reply = n->post(request, QJsonDocument(mo).toJson());
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        if(reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc(QJsonDocument::fromJson(reply->readAll()));
            QJsonObject md = doc.object();
            ui->labelError->setText("");
            ui->labelInstallationUrl->setText(md.value("one_time_installer_url").toString());
            ui->pushButtonCreate->setEnabled(false);
        }
        else {
            qDebug() << "Wrong answer" << reply->error();
            if(reply->error() == QNetworkReply::AuthenticationRequiredError) {
                emit login();
                ui->labelError->setText("Authorization error!" + reply->readAll());
            }
        }
        reply->deleteLater();
    });


}
