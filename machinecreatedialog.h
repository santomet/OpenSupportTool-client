#ifndef MACHINECREATEDIALOG_H
#define MACHINECREATEDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSettings>

namespace Ui {
class MachineCreateDialog;
}

class MachineCreateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MachineCreateDialog(QNetworkAccessManager *n, QWidget *parent = nullptr);
    ~MachineCreateDialog();

    void preset(QByteArray authh, QString name, int id);
    void reset();

private slots:
    void on_pushButtonCreate_clicked();

signals:
    void login();

private:
    Ui::MachineCreateDialog *ui;
    QNetworkAccessManager *n;
    QByteArray authHeaderValue;

    int id = -1;
    QString directoryName;

};
static QByteArray authHeaderName {QByteArray::fromStdString("Authorization")};

#endif // MACHINECREATEDIALOG_H
