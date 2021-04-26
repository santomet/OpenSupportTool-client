#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void accept() override;
    void showEvent(QShowEvent *) override;

    void setErrorMessage(QString str);

private:
    Ui::SettingsDialog *ui;
};

#endif // SETTINGSDIALOG_H
