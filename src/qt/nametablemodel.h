#ifndef NAMETABLEMODEL_H
#define NAMETABLEMODEL_H

#include <qt/bitcoinunits.h>

#include <QAbstractTableModel>
#include <QStringList>

#include <memory>

class PlatformStyle;
class NameTablePriv;
class CWallet;
class WalletModel;

/**
   Qt model for "Manage Names" page.
 */
class NameTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit NameTableModel(const PlatformStyle *platformStyle, CWallet* wallet, WalletModel *parent=nullptr);
    virtual ~NameTableModel();

    enum ColumnIndex {
        Name = 0,
        Value = 1,
        ExpiresIn = 2,
        NameStatus = 3
    };

    /** @name Methods overridden from QAbstractTableModel
        @{*/
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    /*@}*/

private:
    CWallet *wallet;
    WalletModel *walletModel;
    QStringList columns;
    std::unique_ptr<NameTablePriv> priv;
    const PlatformStyle *platformStyle;
    int cachedNumBlocks;

    /** Notify listeners that data changed. */
    void emitDataChanged(int index);

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();

public Q_SLOTS:
    void updateEntry(const QString &name, const QString &value, int nHeight, int status, const QString &nameStatus, int *outNewRowIndex=nullptr);
    void updateExpiration();
    void updateTransaction(const QString &hash, int status);

    friend class NameTablePriv;
};

struct NameTableEntry
{
    QString name;
    QString value;
    int nHeight;
    QString nameStatus;

    static const int NAME_NEW = -1;             // Dummy nHeight value for not-yet-created names
    static const int NAME_NON_EXISTING = -2;    // Dummy nHeight value for unitinialized entries
    static const int NAME_UNCONFIRMED = -3;     // Dummy nHeight value for unconfirmed name transactions

    // NOTE: making this const throws warning indicating it will not be const
    bool HeightValid() { return nHeight >= 0; }
    static bool CompareHeight(int nOldHeight, int nNewHeight);    // Returns true if new height is better

    NameTableEntry() : nHeight(NAME_NON_EXISTING) {}
    NameTableEntry(const QString &name, const QString &value, int nHeight, const QString &nameStatus):
        name(name), value(value), nHeight(nHeight), nameStatus(nameStatus) {}
    NameTableEntry(const std::string &name, const std::string &value, int nHeight, const std::string &nameStatus):
        name(QString::fromStdString(name)), value(QString::fromStdString(value)), nHeight(nHeight), nameStatus(QString::fromStdString(nameStatus)) {}
};

#endif // NAMETABLEMODEL_H
