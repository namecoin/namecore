#include <qt/test/managenamestests.h>

#include <qt/bitcoinamountfield.h>
#include <qt/callback.h>
#include <qt/configurenamedialog.h>
#include <qt/managenamespage.h>
#include <qt/nametablemodel.h>
#include <qt/optionsmodel.h>
#include <qt/overviewpage.h>
#include <qt/platformstyle.h>
#include <qt/qvalidatedlineedit.h>
#include <qt/receivecoinsdialog.h>
#include <qt/receiverequestdialog.h>
#include <qt/recentrequeststablemodel.h>
#include <qt/sendcoinsdialog.h>
#include <qt/sendcoinsentry.h>
#include <qt/transactiontablemodel.h>
#include <qt/transactionview.h>
#include <qt/walletmodel.h>
#include <rpc/server.h>
#include <test/test_bitcoin.h>
#include <validation.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QListView>
#include <QDialogButtonBox>

namespace
{

//! Press "Yes" or "Cancel" buttons in modal send confirmation dialog.
void ConfirmMsgBox(QString* text = nullptr, bool cancel = false)
{
    QTimer::singleShot(0, makeCallback([text, cancel](Callback* callback) {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (!widget->inherits("QMessageBox")) continue;
            QMessageBox * box = qobject_cast<QMessageBox*>(widget);
            if (text) *text = box->text();
            qDebug("clicking ConfirmMsgBox");
            box->button(cancel ? QMessageBox::Cancel : QMessageBox::Yes)->click();
        }
        delete callback;
    }), SLOT(call()));
}

//! Press "Yes" or "Cancel" buttons in modal send confirmation dialog.
void ConfNamesDialog(const QString &data, bool cancel = false)
{
    QTimer::singleShot(1000, makeCallback([data, cancel](Callback* callback) {
        for (QWidget* widget : QApplication::topLevelWidgets()) {
            if (!widget->inherits("ConfigureNameDialog")) continue;
            ConfigureNameDialog * dlg = qobject_cast<ConfigureNameDialog*>(widget);
            QLineEdit* dataEdit = dlg->findChild<QLineEdit*>("dataEdit");
            dataEdit->setText(data);
            qDebug("accepting names dialog");
            dlg->accept();
        }
        delete callback;
    }), SLOT(call()));
}

void GenerateCoins(int nblocks)
{
    UniValue params (UniValue::VOBJ);
    params.pushKV ("nblocks", nblocks);

    JSONRPCRequest jsonRequest;
    jsonRequest.strMethod = "generate";
    jsonRequest.params = params;
    jsonRequest.fHelp = false;

    UniValue res = tableRPC.execute(jsonRequest);
}

//! Find index of name in names list.
QModelIndex FindTx(const QAbstractItemModel& model, const QString& name)
{
    int rows = model.rowCount({});
    for (int row = 0; row < rows; ++row) {
        QModelIndex index = model.index(row, 0, {});
        if (model.data(index, NameTableModel::Name) == name) {
            return index;
        }
    }
    return {};
}


void TestManageNamesGUI()
{
    // Utilize the normal testsuite setup (we have no fixtures in Qt tests
    // so we have to do it like this).
    WalletTestingSetup testSetup(CBaseChainParams::REGTEST);

    // The Qt/wallet testing manifolds don't appear to instantiate the wallets
    // correctly for multi-wallet bitcoin so this is a hack in place until that
    // happens
    std::unique_ptr<CWalletDBWrapper> dbw(new CWalletDBWrapper(&bitdb, "wallet_test.dat"));
    CWallet wallet(std::move(dbw));

    vpwallets.insert(vpwallets.begin(), &wallet);

    bool firstRun;
    wallet.LoadWallet(firstRun);

    // Set up wallet and chain with 105 blocks (5 mature blocks for spending).
    GenerateCoins(105);
    CWalletDB(wallet.GetDBHandle()).LoadWallet(&wallet);
    RegisterWalletRPCCommands(tableRPC);

    // Create widgets for interacting with the names UI
    std::unique_ptr<const PlatformStyle> platformStyle(PlatformStyle::instantiate("other"));
    ManageNamesPage manageNamesPage(platformStyle.get());
    OptionsModel optionsModel;
    WalletModel walletModel(platformStyle.get(), &wallet, &optionsModel);
    manageNamesPage.setModel(&walletModel);

    const QString& name = "test/name1";
    const QString& data = "{\"key\": \"value\"}";

    // make sure we have no names
    NameTableModel* nameTableModel = walletModel.getNameTableModel();
    QCOMPARE(nameTableModel->rowCount(), 0);

    // register a name via the UI (register name_new)
    QValidatedLineEdit* registerName = manageNamesPage.findChild<QValidatedLineEdit*>("registerName");
    registerName->setText(name);
    QCOMPARE(registerName->text(), name);

    // queue click on the warning dialog
    ConfirmMsgBox();
    // queue filling out the configure names dialog with data
    ConfNamesDialog(data);

    /**
     * TODO: BELOW, find a better way to deal with minor UI changes upstream
     * otherwise these tests will break every time a popup changes
     * or is added to the UI, of which there can be many.
     */

    // // click the OK button to finalize name_new & wallet namePendingData write
    // QPushButton* submitNameButton = manageNamesPage.findChild<QPushButton*>("submitNameButton");
    // submitNameButton->click();

    // ConfirmMsgBox();
    // QEventLoop().processEvents();

    // // check nametablemodel for name
    // {
    //     QCOMPARE(nameTableModel->rowCount(), 1);
    //     QModelIndex nameIx = FindTx(*nameTableModel, name);
    //     QVERIFY(nameIx.isValid());
    // }

    // // make sure the expires is blank (pending)
    // {
    //     QModelIndex nameIx = FindTx(*nameTableModel, name);
    //     QVERIFY(nameIx.isValid());
    //     QModelIndex expIx = nameTableModel->index(nameIx.row(), NameTableModel::ExpiresIn);
    //     QVERIFY(expIx.isValid());
    //     QCOMPARE(nameTableModel->data(expIx, 0).toString(),QString(""));
    // }

    // // make sure data is there
    // {
    //     QModelIndex nameIx = FindTx(*nameTableModel, name);
    //     QVERIFY(nameIx.isValid());
    //     QModelIndex valIx = nameTableModel->index(nameIx.row(), NameTableModel::Value);
    //     QVERIFY(valIx.isValid());
    //     QCOMPARE(nameTableModel->data(valIx, 0).toString(),data);
    // }

    // // make sure the pending data is in the wallet
    // {
    //   QVERIFY(walletModel.pendingNameFirstUpdateExists(name.toStdString()));
    // }

    // TODO: need to refactor the NameTableModel so the slots and emitters
    // for updating expirations etc work properly in the testsuite
    // // confirm both operations
    // {
    //     GenerateCoins(12 * 5);
    //     ConfirmMsgBox();
    //     nameTableModel->updateExpiration();
    //     Q_EMIT(nameTableModel->updateExpiration());
    //     QEventLoop().processEvents();
    //     qDebug("processed events?");

    //     // now make sure expires has a value post-confirm of both ops
    //     QModelIndex nameIx = FindTx(*nameTableModel, name);
    //     QVERIFY(nameIx.isValid());
    //     QModelIndex expIx = nameTableModel->index(0, NameTableModel::ExpiresIn);
    //     QVERIFY(expIx.isValid());
    //     QCOMPARE(nameTableModel->data(expIx, NameTableModel::ExpiresIn).toInt(),31);
    // }
}

}

void ManageNamesTests::manageNamesTests()
{
    TestManageNamesGUI();
}
