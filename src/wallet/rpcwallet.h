// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_RPCWALLET_H
#define BITCOIN_WALLET_RPCWALLET_H

#include <span.h>
#include <wallet/walletutil.h>

#include <any>
#include <memory>
#include <string>
#include <vector>

class CCoinControl;
class CRPCCommand;
class CWallet;
class CWalletTx;
class JSONRPCRequest;
class LegacyScriptPubKeyMan;
class UniValue;
class CTransaction;
struct CRecipient;
struct PartiallySignedTransaction;
struct WalletContext;

extern const std::string HELP_REQUIRING_PASSPHRASE;

Span<const CRPCCommand> GetWalletRPCCommands();

/**
 * Figures out what wallet, if any, to use for a JSONRPCRequest.
 *
 * @param[in] request JSONRPCRequest that wishes to access a wallet
 * @return nullptr if no wallet should be used, or a pointer to the CWallet
 */
std::shared_ptr<CWallet> GetWalletForJSONRPCRequest(const JSONRPCRequest& request);

void EnsureWalletIsUnlocked(const CWallet&);
WalletContext& EnsureWalletContext(const std::any& context);
LegacyScriptPubKeyMan& EnsureLegacyScriptPubKeyMan(CWallet& wallet, bool also_create = false);

/* These are private to rpcwallet.cpp upstream, but are used also from
   rpcnames.cpp in Namecoin.  */
UniValue SendMoney(CWallet& wallet, const CCoinControl& coin_control,
                   const CTxIn* withInput,
                   std::vector<CRecipient>& recipients, mapValue_t map_value, bool verbose);

RPCHelpMan getaddressinfo();
RPCHelpMan signrawtransactionwithwallet();
#endif //BITCOIN_WALLET_RPCWALLET_H
