#include "dnsdialog.h"
#include "ui_dnsdialog.h"

#include "dnssubdomaindialog.h"
#include "dnsspectypes.h"
#include "names/main.h"
#include "platformstyle.h"
#include "wallet/wallet.h"
#include "walletmodel.h"

#include <QMessageBox>
#include <QClipboard>

DNSDialog::DNSDialog(const PlatformStyle *platformStyle,
        const QString &_name, const QString &_data,
        QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint),
    ui(new Ui::DNSDialog),
    platformStyle(platformStyle),
    name(_name),
    data(_data)
{
    ui->setupUi(this);

    // TODO: add all subdomains here, select tld by default
    ui->comboDomain->addItem(fmtDotBit(name));
    ui->comboDomain->addItem(Ui::AddSubdomain);

    // TODO: initialize from json
    Domain domain(_name, Domain::TopLevel);
    domain.load(data);
    if (domain.getA() && !domain.getA()->getHost().isEmpty())
    {
        std::cout << "gotA and getHost\n";
        const QString host = domain.getA()->getHost();
        std::cout << "1 A.host Empty? " << host.isEmpty() << '\n';
    }
    else
    {
        std::cout << "not gotA or not getHost\n";
        domain.getA()->setHost("lol");
        const QString host = domain.getA()->getHost();
        std::cout << "2 A.host Empty? " << host.isEmpty() << '\n';
    }
    connect(ui->comboDomain, SIGNAL(currentIndexChanged(int)), this, SLOT(launchSubDomainDialog()));
}

DNSDialog::~DNSDialog()
{
    delete ui;
}

const QString DNSDialog::fmtDotBit(const QString name)
{
    return QString(name.mid(2) + ".bit");
}

void DNSDialog::setModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
}

void DNSDialog::launchSubDomainDialog()
{
    if(ui->comboDomain->currentText() != Ui::AddSubdomain)
        return;

    DNSSubDomainDialog dlg(platformStyle, name, this);
    if (dlg.exec() != QDialog::Accepted)
    {
        ui->comboDomain->setCurrentIndex(0);
        return;
    }

    const QString subDomain = dlg.getReturnData();
    const QString fullName = subDomain + "." + name.mid(2) + ".bit";

    int size = ui->comboDomain->count();
    // set to ix zero so we don't re-pop subdomain modal upon insert/select
    ui->comboDomain->setCurrentIndex(0);
    ui->comboDomain->insertItem(size - 1, fullName);
    ui->comboDomain->setCurrentIndex(size - 1);
}
