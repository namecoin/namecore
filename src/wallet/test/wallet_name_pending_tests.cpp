// Copyright (c) 2016 B.X. Roberts
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc/server.h"
#include "names/common.h"
#include "wallet/wallet.h"
#include "dbwrapper.h"

#include "test/test_bitcoin.h"
#include "wallet/test/wallet_test_fixture.h"

#include <boost/test/unit_test.hpp>
#include <univalue.h>

extern CWallet* pwalletMain;

BOOST_FIXTURE_TEST_SUITE(wallet_name_pending_tests, WalletTestingSetup)

BOOST_AUTO_TEST_CASE(wallet_name_pending_tests)
{
    const std::string name1 = "test/name1";
    const std::string name2 = "test/name2";
    const std::string txid = "9f73e1dfa3cbae23d008307e42e72beb8c010546ea2a7b9ff32619676a9c64a6";
    const std::string rand = "092abbca8a938103abcc";
    const std::string data = "{\"foo\": \"bar\"}";
    const std::string toAddress = "N5e1vXUUL3KfhPyVjQZSes1qQ7eyarDbUU";

    CNamePendingData nameData;
    nameData.setHex(txid);
    nameData.setRand(rand);
    nameData.setData(data);

    CNamePendingData nameDataWithAddr;
    nameDataWithAddr.setHex(txid);
    nameDataWithAddr.setRand(rand);
    nameDataWithAddr.setData(data);
    nameDataWithAddr.setToAddress(toAddress);

    // test that a blank address returns a blank address
    {
        BOOST_CHECK(nameData.getToAddress().empty());
    }

    CWalletDBWrapper& dbw = pwalletMain->GetDBHandle();

    {
        // ensure pending names is blank to start
        LOCK(pwalletMain->cs_wallet);
        BOOST_CHECK(pwalletMain->namePendingMap.size() == 0);
    }

    {
        // write a valid pending name_update to wallet
        LOCK(pwalletMain->cs_wallet);
        BOOST_CHECK(CWalletDB(dbw).WriteNameFirstUpdate(name1, nameData));
        BOOST_CHECK(CWalletDB(dbw).WriteNameFirstUpdate(name2, nameDataWithAddr));
    }

    {
        // load the wallet and see if we get our pending name loaded
        LOCK(pwalletMain->cs_wallet);
        bool fFirstRun;
        BOOST_CHECK_NO_THROW(pwalletMain->LoadWallet(fFirstRun));
    }

    {
        // make sure we've added our pending name
        LOCK(pwalletMain->cs_wallet);
        BOOST_CHECK(pwalletMain->namePendingMap.size() > 0);
        BOOST_CHECK(pwalletMain->namePendingMap.find(name1) != pwalletMain->namePendingMap.end());
        BOOST_CHECK(pwalletMain->namePendingMap.find(name2) != pwalletMain->namePendingMap.end());
    }

    {
        // test removing the names
        LOCK(pwalletMain->cs_wallet);
        BOOST_CHECK(CWalletDB(dbw).EraseNameFirstUpdate(name1));
        BOOST_CHECK(CWalletDB(dbw).EraseNameFirstUpdate(name2));
    }
}

BOOST_AUTO_TEST_SUITE_END()
