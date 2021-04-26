#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::accept()
{
    QSettings s;
    s.setValue("username", ui->lineEditUsername->text());
    s.setValue("password", ui->lineEditPassword->text());
    s.setValue("SSHKey", ui->sshKeyEdit->toPlainText());
    s.setValue("apiurl", ui->lineEditAPI->text());

    QDialog::accept();
}

void SettingsDialog::showEvent(QShowEvent *e)
{
    QSettings s;
    ui->lineEditUsername->setText(s.value("username", "").toString());
    ui->lineEditPassword->setText(s.value("password", "").toString());
    ui->sshKeyEdit->setPlainText(s.value("SSHKey", "").toString());
    ui->lineEditAPI->setText(s.value("apiurl", "").toString());


    QDialog::showEvent(e);
}

void SettingsDialog::setErrorMessage(QString str)
{
    ui->labelError->setText(str);
}
