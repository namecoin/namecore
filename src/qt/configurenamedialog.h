#ifndef CONFIGURENAMEDIALOG_H
#define CONFIGURENAMEDIALOG_H

#include <qt/platformstyle.h>

#include <QDialog>

namespace Ui {
    class ConfigureNameDialog;
}

class WalletModel;

/** Dialog for editing an address and associated information.
 */
class ConfigureNameDialog : public QDialog
{
    Q_OBJECT

public:

    explicit ConfigureNameDialog(const PlatformStyle *platformStyle,
                                 const QString &_name, const QString &data,
                                 bool _firstUpdate, QWidget *parent = nullptr);
    ~ConfigureNameDialog();

    void setModel(WalletModel *walletModel);
    const QString &getReturnData() const { return returnData; }
    const QString &getTransferTo() const { return returnTransferTo; }

public Q_SLOTS:
    void accept() override;
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();

private:
    Ui::ConfigureNameDialog *ui;
    const PlatformStyle *platformStyle;
    QString returnData;
    QString returnTransferTo;
    WalletModel *walletModel;
    const QString name;
    const bool firstUpdate;
};

#endif // CONFIGURENAMEDIALOG_H
