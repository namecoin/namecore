#ifndef DNSDIALOG_H
#define DNSDIALOG_H

#include "platformstyle.h"

#include <QDialog>

namespace Ui {
    class DNSDialog;
    const QString AddSubdomain = "Add Subdomain...";
}

class WalletModel;

/** Dialog for editing an address and associated information.
 */
class DNSDialog: public QDialog
{
    Q_OBJECT

public:

    explicit DNSDialog(const PlatformStyle *platformStyle,
            const QString &_name, const QString &_data,
            QWidget *parent = nullptr);
    ~DNSDialog();

    void setModel(WalletModel *walletModel);
    const QString fmtDotBit(const QString name);

private:
    Ui::DNSDialog *ui;
    const PlatformStyle *platformStyle;
    WalletModel *walletModel;
    const QString name;
    const QString data;

private Q_SLOTS:
    void launchSubDomainDialog();
};

#endif // DNSDIALOG_H
