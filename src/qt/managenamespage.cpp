#include <qt/managenamespage.h>
#include <qt/forms/ui_managenamespage.h>

#include <base58.h>
#include <consensus/validation.h> // cs_main
#include <names/common.h>
#include <qt/configurenamedialog.h>
#include <qt/csvmodelwriter.h>
#include <qt/guiutil.h>
#include <qt/nametablemodel.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <ui_interface.h>
#include <univalue.h>
#include <util.h>
#include <wallet/wallet.h>

#include <QMessageBox>
#include <QMenu>
#include <QSortFilterProxyModel>

ManageNamesPage::ManageNamesPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    platformStyle(platformStyle),
    ui(new Ui::ManageNamesPage),
    model(nullptr),
    walletModel(nullptr),
    proxyModel(nullptr)
{
    ui->setupUi(this);

    // Context menu actions
    QAction *copyNameAction = new QAction(tr("Copy &Name"), this);
    QAction *copyValueAction = new QAction(tr("Copy &Value"), this);
    QAction *configureNameAction = new QAction(tr("&Configure Name..."), this);
    QAction *renewNameAction = new QAction(tr("&Renew Name"), this);

    // Build context menu
    contextMenu = new QMenu();
    contextMenu->addAction(copyNameAction);
    contextMenu->addAction(copyValueAction);
    contextMenu->addAction(configureNameAction);
    contextMenu->addAction(renewNameAction);

    // Connect signals for context menu actions
    connect(copyNameAction, SIGNAL(triggered()), this, SLOT(onCopyNameAction()));
    connect(copyValueAction, SIGNAL(triggered()), this, SLOT(onCopyValueAction()));
    connect(configureNameAction, SIGNAL(triggered()), this, SLOT(on_configureNameButton_clicked()));
    connect(renewNameAction, SIGNAL(triggered()), this, SLOT(on_renewNameButton_clicked()));

    connect(ui->tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextualMenu(QPoint)));
    connect(ui->tableView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(on_configureNameButton_clicked()));
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->registerName->installEventFilter(this);
    ui->tableView->installEventFilter(this);
}

ManageNamesPage::~ManageNamesPage()
{
    delete ui;
}

void ManageNamesPage::setModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    model = walletModel->getNameTableModel();

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->tableView->setModel(proxyModel);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    ui->tableView->horizontalHeader()->setHighlightSections(false);

    // Set column widths
    ui->tableView->horizontalHeader()->resizeSection(
            NameTableModel::Name, 320);
#if QT_VERSION >= 0x050000
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
#else
    // this function introduced in QT5
    ui->tableView->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
#endif


    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectionChanged()));

    selectionChanged();
}

void ManageNamesPage::on_submitNameButton_clicked()
{
    if (!walletModel)
        return;

    QString name = ui->registerName->text();

    QString reason;
    if (!walletModel->nameAvailable(name, &reason))
    {
        if(reason.isEmpty())
            QMessageBox::warning(this, tr("Name registration"), tr("Name not available"));
        else
            QMessageBox::warning(this, tr("Name registration"), tr("Name not available<br>Reason: %1").arg(reason));

        ui->registerName->setFocus();
        return;
    }

    QString msg;
    if (name.startsWith("d/"))
        msg = tr("Are you sure you want to register domain name %1, which corresponds to domain %2? <br><br> NOTE: If your wallet is locked, you will be prompted to unlock it in 12 blocks.").arg(name).arg(name.mid(2) + ".bit");
    else
        msg = tr("Are you sure you want to register non-domain name %1? <br><br>NOTE: If your wallet is locked, you will be prompted to unlock it in 12 blocks.").arg(name);

    if (QMessageBox::Yes != QMessageBox::question(this, tr("Confirm name registration"), msg, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel))
        return;

    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
        return;

    std::string strName = name.toStdString();

    WalletModel::NameNewReturn res = walletModel->nameNew(name);
    if (!res.ok)
    {
        QMessageBox::warning(this, tr("Name registration failed"), QString::fromStdString(res.err_msg));
        return;
    }

    // reset UI text
    ui->registerName->setText("d/");
    ui->submitNameButton->setDefault(true);

    ConfigureNameDialog dlg(platformStyle, name, "", true, this);
    dlg.setModel(walletModel);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString data = dlg.getReturnData();
    std::string strData = data.toStdString();

    UniValue jsonData(UniValue::VOBJ);
    jsonData.pushKV ("txid", res.hex);
    jsonData.pushKV ("rand", res.rand);
    jsonData.pushKV ("data", strData);
    if (!res.toaddress.empty ())
        jsonData.pushKV ("toaddress", res.toaddress);

    walletModel->writePendingNameFirstUpdate(strName, res.rand, res.hex, strData, res.toaddress);

    int newRowIndex;
    model->updateEntry(name, dlg.getReturnData(), NameTableEntry::NAME_NEW, CT_NEW, "pending registration", &newRowIndex);
    ui->tableView->selectRow(newRowIndex);
    ui->tableView->setFocus();

    return;
}

bool ManageNamesPage::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn)
    {
        if (object == ui->registerName)
        {
            ui->submitNameButton->setDefault(true);
            ui->configureNameButton->setDefault(false);
        }
        else if (object == ui->tableView)
        {
            ui->submitNameButton->setDefault(false);
            ui->configureNameButton->setDefault(true);
        }
    }
    return QWidget::eventFilter(object, event);
}

void ManageNamesPage::selectionChanged()
{
    // Set button states based on selected tab and selection
    QTableView *table = ui->tableView;
    if (!table->selectionModel())
        return;

    const bool state = table->selectionModel()->hasSelection();
    ui->configureNameButton->setEnabled(state);
    ui->renewNameButton->setEnabled(state);
}

void ManageNamesPage::contextualMenu(const QPoint &point)
{
    QModelIndex index = ui->tableView->indexAt(point);
    if (index.isValid())
        contextMenu->exec(QCursor::pos());
}

void ManageNamesPage::onCopyNameAction()
{
    GUIUtil::copyEntryData(ui->tableView, NameTableModel::Name);
}

void ManageNamesPage::onCopyValueAction()
{
    GUIUtil::copyEntryData(ui->tableView, NameTableModel::Value);
}

void ManageNamesPage::on_configureNameButton_clicked()
{
    if (!ui->tableView->selectionModel())
        return;
    const QModelIndexList &indexes = ui->tableView->selectionModel()->selectedRows(NameTableModel::Name);
    if (indexes.isEmpty())
        return;
    WalletModel::UnlockContext ctx(walletModel->requestUnlock ());
    if (!ctx.isValid ())
        return;

    const QModelIndex &index = indexes.at(0);
    const QString &name = index.data(Qt::EditRole).toString();
    const std::string &strName = name.toStdString();
    const QString &value = index.sibling(index.row(), NameTableModel::Value).data(Qt::EditRole).toString();
    const bool fFirstUpdate = walletModel->pendingNameFirstUpdateExists(strName);

    ConfigureNameDialog dlg(platformStyle, name, value, fFirstUpdate, this);
    dlg.setModel(walletModel);
    if (dlg.exec() != QDialog::Accepted)
        return;

    const QString &qData = dlg.getReturnData();
    const std::string &strData = qData.toStdString();

    if(fFirstUpdate)
    {
        // update pending first
        CNamePendingData npd;
        bool success = walletModel->getPendingNameFirstUpdate(strName, &npd);
        if (success)
        {
            walletModel->writePendingNameFirstUpdate(strName, npd.getRand(), npd.getHex(), strData, npd.getToAddress());
            LogPrintf("configure:changing updating pending name_firstupdate name=%s value=%s\n",
                strName.c_str(), strData.c_str());
            model->updateEntry(name, qData, NameTableEntry::NAME_UNCONFIRMED, CT_UPDATED, "firstupdate pending");
        }
    }
    else
    {
        const QString &transferToAddress = dlg.getTransferTo();
        QString result = walletModel->nameUpdate(name, qData, transferToAddress);
        if (!result.isEmpty())
        {
            QMessageBox::warning(this, tr("Name update"), tr("Unable to update name.<br>Reason: %1").arg(result));
            return;
        }
        model->updateEntry(name, qData, NameTableEntry::NAME_UNCONFIRMED, CT_UPDATED, "update pending");
    }

}

void ManageNamesPage::on_renewNameButton_clicked ()
{
    if (!ui->tableView->selectionModel())
        return;
    const QModelIndexList &indexes = ui->tableView->selectionModel()->selectedRows(NameTableModel::Name);
    if (indexes.isEmpty())
        return;

    const QModelIndex &index = indexes.at(0);
    const QString &name = index.data(Qt::EditRole).toString();
    const QString &value = index.sibling(index.row(), NameTableModel::Value).data(Qt::EditRole).toString();

    // TODO: Warn if the "expires in" value is still high
    const QString msg
        = tr ("Are you sure you want to renew the name <b>%1</b>?")
        .arg (GUIUtil::HtmlEscape (name));
    const QString title = tr ("Confirm name renewal");

    QMessageBox::StandardButton res;
    res = QMessageBox::question (this, title, msg,
                                 QMessageBox::Yes | QMessageBox::Cancel,
                                 QMessageBox::Cancel);
    if (res != QMessageBox::Yes)
        return;

    WalletModel::UnlockContext ctx(walletModel->requestUnlock ());
    if (!ctx.isValid ())
        return;

    const QString err_msg = walletModel->nameUpdate(name, value, "");
    if (!err_msg.isEmpty() && err_msg != "ABORTED")
    {
        QMessageBox::critical(this, tr("Name update error"), err_msg);
        return;
    }

    model->updateEntry(name, value, NameTableEntry::NAME_UNCONFIRMED, CT_UPDATED, "update pending");
}

void ManageNamesPage::exportClicked()
{
    // CSV is currently the only supported format
    QString suffixOut = "";
    QString filename = GUIUtil::getSaveFileName(
            this,
            tr("Export Registered Names Data"),
            QString(),
            tr("Comma separated file (*.csv)"),
            &suffixOut);

    if (filename.isNull())
        return;

    CSVModelWriter writer(filename);

    // name, column, role
    writer.setModel(proxyModel);
    writer.addColumn("Name", NameTableModel::Name, Qt::EditRole);
    writer.addColumn("Value", NameTableModel::Value, Qt::EditRole);
    writer.addColumn("Expires In", NameTableModel::ExpiresIn, Qt::EditRole);
    writer.addColumn("Name Status", NameTableModel::NameStatus, Qt::EditRole);

    if (!writer.write())
    {
        QMessageBox::critical(this, tr("Error exporting"), tr("Could not write to file %1.").arg(filename),
                              QMessageBox::Abort, QMessageBox::Abort);
    }
}
