#include <qt/nametablemodel.h>

#include <names/common.h>
#include <names/main.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <ui_interface.h>
#include <univalue.h>
#include <util.h>
#include <validation.h> // cs_main
#include <wallet/wallet.h>

#include <QDebug>
#include <QObject>
#include <QTimer>

// ExpiresIn column is right-aligned as it contains numbers
namespace {
    int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,     // Name
        Qt::AlignLeft|Qt::AlignVCenter,     // Value
        Qt::AlignRight|Qt::AlignVCenter,    // Expires in
        Qt::AlignRight|Qt::AlignVCenter,    // Name Sataus
    };
}

struct NameTableEntryLessThan
{
    bool operator()(const NameTableEntry &a, const NameTableEntry &b) const
    {
        return a.name < b.name;
    }
    bool operator()(const NameTableEntry &a, const QString &b) const
    {
        return a.name < b;
    }
    bool operator()(const QString &a, const NameTableEntry &b) const
    {
        return a < b.name;
    }
};

// Returns true if new height is better
bool NameTableEntry::CompareHeight(int nOldHeight, int nNewHeight)
{
    if(nOldHeight == NAME_NON_EXISTING)
        return true;

    // We use optimistic way, assuming that unconfirmed transaction will eventually become confirmed,
    // so we update the name in the table immediately. Ideally we need a separate way of displaying
    // unconfirmed names (e.g. grayed out)
    if(nNewHeight == NAME_UNCONFIRMED)
        return true;

    // Here we rely on the fact that dummy height values are always negative
    return nNewHeight > nOldHeight;
}

// Private implementation
class NameTablePriv
{
public:
    CWallet *wallet;
    QList<NameTableEntry> cachedNameTable;
    NameTableModel *parent;

    NameTablePriv(CWallet *wallet, NameTableModel *parent):
        wallet(wallet), parent(parent) {}

    void refreshNameTable()
    {
        qDebug() << "NameTableModel::refreshNameTable";
        cachedNameTable.clear();
        std::map< std::string, NameTableEntry > vNamesO;

        // unconfirmed (name_pending) names
        JSONRPCRequest namePendingRequest;
        namePendingRequest.strMethod = "name_pending";
        namePendingRequest.params = NullUniValue;
        namePendingRequest.fHelp = false;
        UniValue pendingNames;

        try {
            pendingNames = tableRPC.execute(namePendingRequest);
        } catch (const UniValue& e) {
            // although we shouldn't typically encounter error here, we
            // should continue and try to add confirmed names and
            // pending names. show error to user in case something
            // actually went wrong so they can potentially recover
            UniValue message = find_value( e, "message");
            LogPrintf ("name_pending lookup error: %s\n", message.get_str().c_str());
        }

        if(pendingNames.isArray())
        {
            for (const auto& v : pendingNames.getValues())
            {
                std::string name = find_value ( v, "name").get_str();
                std::string data = find_value ( v, "value").get_str();
                vNamesO[name] = NameTableEntry(
                    name, data,
                    NameTableEntry::NAME_UNCONFIRMED,
                    "pending registration");
                LogPrintf("found pending name: name=%s\n", name.c_str());
            }
        }

        // confirmed names (name_list)
        JSONRPCRequest nameListRequest;
        nameListRequest.strMethod = "name_list";
        nameListRequest.params = NullUniValue;
        nameListRequest.fHelp = false;
        UniValue confirmedNames;

        try {
            confirmedNames = tableRPC.execute(nameListRequest);
        } catch (const UniValue& e) {
            // NOTE: see note for catch for above name_pending call, the
            // same reasoning applies for continuing here
            UniValue message = find_value( e, "message");
            LogPrintf ("name_list lookup error: %s\n", message.get_str().c_str());
        }

        // will be an object if name_list command isn't available/other error
        if(confirmedNames.isArray())
        {
            for (const auto& v : confirmedNames.getValues())
            {
                if(find_value ( v, "expired").get_bool())
                    continue;
                std::string name = find_value ( v, "name").get_str();
                std::string data = find_value ( v, "value").get_str();
                int height = find_value ( v, "height").get_int();
                vNamesO[name] = NameTableEntry(name, data, height, "confirmed");
                LogPrintf("found confirmed name: name=%s height=%i\n", name.c_str(), height);
            }
        }

        // Add existing names
        for (const auto& item : vNamesO)
            cachedNameTable.append(item.second);

        // Add pending names (name_new)
        LOCK(wallet->cs_wallet);
        for (const auto& item : wallet->namePendingMap)
        {
            CNamePendingData npd = CNamePendingData(item.second);
            cachedNameTable.append(
                NameTableEntry(item.first,
                               npd.getData(),
                               NameTableEntry::NAME_NEW,
                               "pending firstupdate"));
        }

        // qLowerBound() and qUpperBound() require our cachedNameTable list to be sorted in asc order
        qSort(cachedNameTable.begin(), cachedNameTable.end(), NameTableEntryLessThan());
    }

    bool findInModel(const QString &name, int *lowerIndex=nullptr, int *upperIndex=nullptr)
    {
        // Find name in model
        QList<NameTableEntry>::iterator lower = qLowerBound(
            cachedNameTable.begin(), cachedNameTable.end(), name, NameTableEntryLessThan());
        QList<NameTableEntry>::iterator upper = qUpperBound(
            cachedNameTable.begin(), cachedNameTable.end(), name, NameTableEntryLessThan());
        if (lowerIndex)
            *lowerIndex = (lower - cachedNameTable.begin());
        if (upperIndex)
            *upperIndex = (upper - cachedNameTable.begin());
        return lower != upper;
    }

    void refreshName(const valtype &inName, QString *qNameStatus = nullptr)
    {
        LOCK(cs_main);

        std::string strName = ValtypeToString(inName);
        int lowerIndex, upperIndex;
        bool inModel = findInModel(QString::fromStdString(strName), &lowerIndex, &upperIndex);
        QList<NameTableEntry>::iterator lower = (cachedNameTable.begin() + lowerIndex);

        // preserve previous name state if available
        std::string strNameStatus;
        if (qNameStatus)
            strNameStatus = qNameStatus->toStdString();
        else
            strNameStatus = "";

        std::string strData = "";
        int height = NameTableEntry::NAME_NON_EXISTING;
        if(inModel)
        {
            strNameStatus = lower->nameStatus.toStdString();
            height = lower->nHeight;
            strData = lower->value.toStdString();
        }

        // NOTE: name_show only reflects the status of the name as it is
        // currently existig in the chain. this means that if you issue
        // something like a name_update and issue name_show, the name_update
        // results will not be reflected until block confirmation
        UniValue params (UniValue::VOBJ);
        params.pushKV ("name", strName);

        JSONRPCRequest jsonRequest;
        jsonRequest.strMethod = "name_show";
        jsonRequest.params = params;
        jsonRequest.fHelp = false;

        UniValue res;
        try {
            res = tableRPC.execute(jsonRequest);
        } catch (const UniValue& e) {
            UniValue message = find_value(e, "message");
            std::string errorStr = message.get_str();
            LogPrintf ("unexpected name_show response on refreshName=%s: %s\n",
                    strName.c_str(), errorStr.c_str());
            return;
        }

        UniValue heightResult = find_value(res, "height");
        if (!heightResult.isNum())
        {
            LogPrintf ("No height for name %s\n", strName.c_str());
            return;
        }

        // we have a height, this means we either have a confirmed
        // name_firstupdate or we have either an old or new
        // name_update (no way to tell via name_show)
        height = heightResult.get_int();

        UniValue valTxid = find_value(res, "txid");
        if (!valTxid.isStr())
        {
            LogPrintf ("No txid for name %s\n", strName.c_str());
            return;
        }

        std::string strTxid = valTxid.get_str();

        // get transaction and look for confirms, update name status
        // if we have confirms
        UniValue txparams (UniValue::VOBJ);
        txparams.pushKV ("txid", strTxid);

        JSONRPCRequest txJsonRequest;
        txJsonRequest.strMethod = "gettransaction";
        txJsonRequest.params = txparams;
        txJsonRequest.fHelp = false;

        UniValue txRes;
        try {
            txRes = tableRPC.execute(txJsonRequest);
        } catch (const UniValue& e) {
            UniValue message = find_value(e, "message");
            std::string errorStr = message.get_str();
            LogPrintf ("unexpected gettransaction response on refreshName=%s: %s\n",
                    strName.c_str(), errorStr.c_str());
            return;
        }

        UniValue valConfirms = find_value(txRes, "confirmations");
        if (!valConfirms.isNum())
        {
            LogPrintf ("No confirmations for name %s\n", strName.c_str());
            return;
        }

        const unsigned int uConfirms = static_cast<unsigned int>(valConfirms.get_int());
        if (!qNameStatus && uConfirms >= MIN_FIRSTUPDATE_DEPTH)
            strNameStatus = "confirmed";

        NameTableEntry nameObj(strName, strData, height, strNameStatus);

        if(findInModel(nameObj.name))
        {
            // In model - update or delete
            if(nameObj.nHeight != NameTableEntry::NAME_NON_EXISTING)
            {
                LogPrintf ("refreshName result : %s - refreshed in the table\n", qPrintable(nameObj.name));
                updateEntry(nameObj.name, nameObj.value, nameObj.nHeight, CT_UPDATED, nameObj.nameStatus);
            }
            else
            {
                LogPrintf("refreshName result : %s - deleted from the table\n", qPrintable(nameObj.name));
                updateEntry(nameObj.name, nameObj.value, nameObj.nHeight, CT_DELETED, nameObj.nameStatus);
            }
        }
        else
        {
            // Not in model - add or do nothing
            if(nameObj.nHeight != NameTableEntry::NAME_NON_EXISTING)
            {
                LogPrintf("refreshName result : %s - added to the table\n", qPrintable(nameObj.name));
                updateEntry(nameObj.name, nameObj.value, nameObj.nHeight, CT_NEW, nameObj.nameStatus);
            }
            else
            {
                LogPrintf("refreshName result : %s - ignored (not in the table)\n", qPrintable(nameObj.name));
            }
        }
    }

    void updateEntry(const QString &name, const QString &value, int nHeight,
            int status, const QString &nameStatus, int *outNewRowIndex=nullptr)
    {
        int lowerIndex, upperIndex;
        bool inModel = findInModel(name, &lowerIndex, &upperIndex);
        QList<NameTableEntry>::iterator lower = (cachedNameTable.begin() + lowerIndex);
        QList<NameTableEntry>::iterator upper = (cachedNameTable.begin() + upperIndex);

        switch(status)
        {
        case CT_NEW:
            if(inModel)
            {
                if(outNewRowIndex)
                {
                    *outNewRowIndex = parent->index(lowerIndex, 0).row();
                    // HACK: ManageNamesPage uses this to ensure updating and get selected row,
                    // so we do not write warning into the log in this case
                }
                else {
                    LogPrintf ("Warning: NameTablePriv::updateEntry: Got CT_NEW, but entry is already in model\n");
                }
                break;
            }
            parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
            cachedNameTable.insert(lowerIndex, NameTableEntry(name, value, nHeight, nameStatus));
            parent->endInsertRows();
            if(outNewRowIndex)
                *outNewRowIndex = parent->index(lowerIndex, 0).row();
            break;
        case CT_UPDATED:
            if(!inModel)
            {
                LogPrintf ("Warning: NameTablePriv::updateEntry: Got CT_UPDATED, but entry is not in model\n");
                break;
            }
            lower->name = name;
            lower->value = value;
            lower->nHeight = nHeight;
            lower->nameStatus = nameStatus;
            parent->emitDataChanged(lowerIndex);
            break;
        case CT_DELETED:
            if(!inModel)
            {
                LogPrintf ("Warning: NameTablePriv::updateEntry: Got CT_DELETED, but entry is not in model\n");
                break;
            }
            parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex - 1);
            cachedNameTable.erase(lower, upper);
            parent->endRemoveRows();
            break;
        }
    }

    int size()
    {
        return cachedNameTable.size();
    }

    NameTableEntry *index(int idx)
    {
        if(idx >= 0 && idx < cachedNameTable.size())
        {
            return &cachedNameTable[idx];
        }
        else
        {
            return nullptr;
        }
    }
};

NameTableModel::NameTableModel(const PlatformStyle *platformStyle, CWallet* wallet, WalletModel *parent):
        QAbstractTableModel(parent),
        wallet(wallet),
        walletModel(parent),
        priv(new NameTablePriv(wallet, this)),
        platformStyle(platformStyle)
{
    columns << tr("Name") << tr("Value") << tr("Expires In") << tr("Status");
    priv->refreshNameTable();

    QTimer *timer = new QTimer(this);
    // TODO: move updateExpiration from a constant timer to a legit slot
    connect(timer, SIGNAL(timeout()), this, SLOT(updateExpiration()));
    timer->start(MODEL_UPDATE_DELAY);

    subscribeToCoreSignals();
}

NameTableModel::~NameTableModel()
{
    unsubscribeFromCoreSignals();
}

void NameTableModel::updateExpiration()
{
    int nBestHeight = chainActive.Height();
    if(nBestHeight != cachedNumBlocks)
    {
        LOCK(cs_main);

        cachedNumBlocks = nBestHeight;
        std::vector<NameTableEntry*> expired;
        // Blocks came in since last poll.
        // Delete expired names
        for (int i = 0, n = priv->size(); i < n; i++)
        {
            NameTableEntry *item = priv->index(i);

            const Consensus::Params& params = Params().GetConsensus();
            int nHeight = item->nHeight;
            int expirationDepth = params.rules->NameExpirationDepth(nHeight);

            priv->refreshName(ValtypeFromString(item->name.toStdString()));

            // remove expired confirmed names
            if((nHeight + expirationDepth <= nBestHeight) && (item->nameStatus == "confirmed"))
            {
                expired.push_back(item);
            }

        }

        // process all expirations in bulk (don't mutate table while iterating
        for (NameTableEntry *item : expired)
            priv->updateEntry(item->name, item->value, item->nHeight, CT_DELETED, "expired");

        // Invalidate expiration counter for all rows.
        // Qt is smart enough to only actually request the data for the
        // visible rows.
        //emit
        dataChanged(index(0, ExpiresIn), index(priv->size()-1, ExpiresIn));
    }
}

void NameTableModel::updateTransaction(const QString &hash, int status)
{
    uint256 hash256;
    std::string strHash = hash.toStdString();
    hash256.SetHex(strHash);

    LOCK(wallet->cs_wallet);
    // Find transaction in wallet
    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash256);
    if(mi == wallet->mapWallet.end())
    {
        LogPrintf ("tx %s has no name in wallet\n", strHash);
        return;
    }
    CWalletTx wtx = mi->second;

    // const auto vout = tx->vout;
    for (const CTxOut& txout : wtx.tx->vout)
    {
        if(!CNameScript::isNameScript(txout.scriptPubKey))
        {
            continue;
        }

        CNameScript nameScript(txout.scriptPubKey);
        switch (nameScript.getNameOp())
        {
            case OP_NAME_NEW:
                break;

            case OP_NAME_FIRSTUPDATE:
                {
                    QString qNameStatus = "pending firstupdate";
                    priv->refreshName(nameScript.getOpName(), &qNameStatus);
                    break;
                }
            case OP_NAME_UPDATE:
                {
                    QString qNameStatus = "pending update";
                    priv->refreshName(nameScript.getOpName(), &qNameStatus);
                    break;
                }

            default:
                assert (false);
        }
    }

}

int NameTableModel::rowCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int NameTableModel::columnCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QVariant NameTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();

    NameTableEntry *rec = static_cast<NameTableEntry*>(index.internalPointer());

    // TODO: implement Qt::ForegroudRole for font color styling for states?
    // TODO: implement Qt::ToolTipRole show name status on tooltip
    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
            case Name:
                return rec->name;
            case Value:
                return rec->value;
            case ExpiresIn:
                {
                    if(!rec->HeightValid()) {
                        return QVariant();
                    }
                    int nBestHeight = chainActive.Height();
                    const Consensus::Params& params = Params().GetConsensus();
                    return rec->nHeight + params.rules->NameExpirationDepth(rec->nHeight) - nBestHeight;
                }
            case NameStatus:
                return rec->nameStatus;
        }
    }
    return QVariant();
}

QVariant NameTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation != Qt::Horizontal)
        return QVariant();

    if(role == Qt::DisplayRole)
        return columns[section];

    if(role == Qt::TextAlignmentRole)
        return column_alignments[section];

    if(role == Qt::ToolTipRole)
    {
        switch(section)
        {
            case Name:
                return tr("Name registered using Namecoin.");

            case Value:
                return tr("Data associated with the name.");

            case ExpiresIn:
                return tr("Number of blocks, after which the name will expire. Update name to renew it.\nEmpty cell means pending(awaiting automatic name_firstupdate or awaiting network confirmation).");
        }
    }
    return QVariant();
}

Qt::ItemFlags NameTableModel::flags(const QModelIndex &index) const
{
    if(!index.isValid())
        return 0;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QModelIndex NameTableModel::index(int row, int column, const QModelIndex &parent /* = QModelIndex()*/) const
{
    Q_UNUSED(parent);
    NameTableEntry *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    return QModelIndex();
}

// queue notifications to show a non freezing progress dialog e.g. for rescan
struct TransactionNotification
{
public:
    TransactionNotification() {}
    TransactionNotification(const uint256 hash, const ChangeType status, const bool showTransaction):
        hash(hash), status(status), showTransaction(showTransaction) {}

    void invoke(NameTableModel *ntm)
    {
        QString strHash = QString::fromStdString(hash.GetHex());
        QMetaObject::invokeMethod(ntm, "updateTransaction", Qt::QueuedConnection,
                                  Q_ARG(QString, strHash),
                                  Q_ARG(int, status));
    }
private:
    const uint256 hash;
    ChangeType status;
    bool showTransaction;
};

static bool fQueueNotifications = false;
static std::vector< TransactionNotification > vQueueNotifications;

static void NotifyTransactionChanged(NameTableModel *ntm, CWallet *wallet, const uint256 &hash, ChangeType status)
{
    // Find transaction in wallet
    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
    bool inWallet = mi != wallet->mapWallet.end();

    TransactionNotification notification(hash, status, inWallet);

    if(fQueueNotifications)
    {
        vQueueNotifications.push_back(notification);
        return;
    }
    notification.invoke(ntm);
}


void
NameTableModel::updateEntry(const QString &name, const QString &value,
                            int nHeight, int status, const QString &nameStatus,
                            int *outNewRowIndex)
{
    priv->updateEntry(name, value, nHeight, status, nameStatus, outNewRowIndex);
}

void
NameTableModel::emitDataChanged(int idx)
{
    //emit
    dataChanged(index(idx, 0), index(idx, columns.length()-1));
}

void
NameTableModel::subscribeToCoreSignals()
{
    // Connect signals to wallet
    wallet->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    // wallet->ShowProgress.connect(boost::bind(ShowProgress, this, _1, _2));
}

void
NameTableModel::unsubscribeFromCoreSignals()
{
    // Disconnect signals from wallet
    wallet->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
    // wallet->ShowProgress.disconnect(boost::bind(ShowProgress, this, _1, _2));
}
