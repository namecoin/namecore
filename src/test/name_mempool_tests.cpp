// Copyright (c) 2014-2022 Daniel Kraft
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <coins.h>
#include <key_io.h>
#include <names/encoding.h>
#include <names/mempool.h>
#include <primitives/transaction.h>
#include <script/names.h>
#include <sync.h>
#include <txmempool.h>
#include <validation.h>
#include <validationinterface.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

/* No space between BOOST_FIXTURE_TEST_SUITE and '(', so that extraction of
   the test-suite name works with grep as done in the Makefile.  */
BOOST_AUTO_TEST_SUITE(name_mempool_tests)

namespace
{

class NameMempoolTestSetup : public TestingSetup
{

public:

  CTxMemPool& mempool;

  CScript ADDR;
  CScript OTHER_ADDR;

  const LockPoints lp;

  NameMempoolTestSetup ()
    : mempool(*Assert(m_node.mempool))
  {
    ADDR = CScript () << OP_TRUE;
    OTHER_ADDR = CScript () << OP_TRUE << OP_RETURN;

    ENTER_CRITICAL_SECTION (cs_main);
    ENTER_CRITICAL_SECTION (mempool.cs);
  }

  ~NameMempoolTestSetup ()
  {
    LEAVE_CRITICAL_SECTION (mempool.cs);
    LEAVE_CRITICAL_SECTION (cs_main);
  }

  /**
   * Returns a valtype name based on the given string.
   */
  static valtype
  Name (const std::string& str)
  {
    return DecodeName (str, NameEncoding::ASCII);
  }

  /**
   * Returns the hash bytes for a name_new.
   */
  static valtype
  NewHash (const std::string& nm, const char rand)
  {
    const valtype nameVal = Name (nm);
    const valtype randVal(20, rand);

    valtype toHash(randVal);
    toHash.insert (toHash.end (), nameVal.begin (), nameVal.end ());
    const uint160 hash = Hash160 (toHash);

    return valtype (hash.begin (), hash.end ());
  }

  /**
   * Builds a name new script for the given test name and rand based
   * on a character (just to allow different rand's).
   */
  static CScript
  NewScript (const CScript& addr, const std::string& nm, const char rand)
  {
    const valtype randVal(20, rand);

    return CNameScript::buildNameNew (addr, Name (nm), randVal);
  }

  /**
   * Builds a name_firstupdate script for the given name and rand value.
   * The value we update to is just a fixed one.
   */
  static CScript
  FirstScript (const CScript& addr, const std::string& nm, const char rand)
  {
    const valtype randVal(20, rand);
    const valtype value = DecodeName ("firstupdate value", NameEncoding::ASCII);

    return CNameScript::buildNameFirstupdate (addr, Name (nm), value, randVal);
  }

  /**
   * Builds a name_update script based on the given name and value.
   */
  static CScript
  UpdateScript (const CScript& addr, const std::string& nm,
                const std::string& val)
  {
    const valtype value = DecodeName (val, NameEncoding::ASCII);

    return CNameScript::buildNameUpdate (addr, Name (nm), value);
  }

  /**
   * Builds a transaction spending to a name-output script.  The transaction
   * is not valid, but it is "valid enough" for testing the name mempool
   * rules with it.
   */
  static CTransaction
  Tx (const CScript& out)
  {
    CMutableTransaction mtx;
    mtx.SetNamecoin ();
    mtx.vout.push_back (CTxOut (COIN, out));

    return CTransaction (mtx);
  }

  /**
   * Builds a mempool entry for the given transaction.
   */
  CTxMemPoolEntry
  Entry (const CTransaction& tx)
  {
    return CTxMemPoolEntry (MakeTransactionRef (tx), 0, 0, 100, false, 1, lp);
  }

};

} // anonymous namespace

/* ************************************************************************** */

BOOST_FIXTURE_TEST_CASE (invalid_tx, NameMempoolTestSetup)
{
  /* Invalid transactions should not crash / assert fail the mempool check.  */

  CMutableTransaction mtx;
  mtx.SetNamecoin ();
  mempool.checkNameOps (CTransaction (mtx));

  mtx.vout.push_back (CTxOut (COIN, NewScript (ADDR, "foo", 'a')));
  mtx.vout.push_back (CTxOut (COIN, NewScript (ADDR, "bar", 'b')));
  mtx.vout.push_back (CTxOut (COIN, FirstScript (ADDR, "foo", 'a')));
  mtx.vout.push_back (CTxOut (COIN, FirstScript (ADDR, "bar", 'b')));
  mtx.vout.push_back (CTxOut (COIN, UpdateScript (ADDR, "foo", "x")));
  mtx.vout.push_back (CTxOut (COIN, UpdateScript (ADDR, "bar", "y")));
  mempool.checkNameOps (CTransaction (mtx));
}

BOOST_FIXTURE_TEST_CASE (empty_mempool, NameMempoolTestSetup)
{
  /* While the mempool is empty (we do not add any transactions in this test),
     all should be fine without respect to conflicts among the transactions.  */

  BOOST_CHECK (!mempool.registersName (Name ("foo")));
  BOOST_CHECK (!mempool.updatesName (Name ("foo")));

  BOOST_CHECK (mempool.checkNameOps (Tx (NewScript (ADDR, "foo", 'a'))));
  BOOST_CHECK (mempool.checkNameOps (Tx (NewScript (ADDR, "foo", 'b'))));
  BOOST_CHECK (mempool.checkNameOps (Tx (NewScript (OTHER_ADDR, "foo", 'a'))));

  BOOST_CHECK (mempool.checkNameOps (Tx (FirstScript (ADDR, "foo", 'a'))));
  BOOST_CHECK (mempool.checkNameOps (Tx (FirstScript (ADDR, "foo", 'b'))));

  BOOST_CHECK (mempool.checkNameOps (Tx (UpdateScript (ADDR, "foo", "x"))));
  BOOST_CHECK (mempool.checkNameOps (Tx (UpdateScript (ADDR, "foo", "y"))));
}

BOOST_FIXTURE_TEST_CASE (pendingChainLength_lastNameOutput,
                         NameMempoolTestSetup)
{
  const auto txNew = Tx (NewScript (ADDR, "new", 'a'));
  const auto txReg = Tx (FirstScript (ADDR, "reg", 'b'));
  const auto txUpd = Tx (UpdateScript (ADDR, "upd", "x"));

  mempool.addUnchecked (Entry (txNew));
  mempool.addUnchecked (Entry (txReg));
  mempool.addUnchecked (Entry (txUpd));

  /* For testing chained name updates, we have to build a "real" chain of
     transactions with matching inputs and outputs.  */

  CMutableTransaction mtx;
  mtx.SetNamecoin ();
  mtx.vout.push_back (CTxOut (COIN, FirstScript (ADDR, "chain", 'a')));
  mtx.vout.push_back (CTxOut (COIN, ADDR));
  mtx.vout.push_back (CTxOut (COIN, OTHER_ADDR));
  const CTransaction chain1(mtx);
  mempool.addUnchecked (Entry (chain1));

  mtx.vout.clear ();
  mtx.vout.push_back (CTxOut (COIN, ADDR));
  mtx.vout.push_back (CTxOut (COIN, UpdateScript (ADDR, "chain", "x")));
  mtx.vin.push_back (CTxIn (COutPoint (chain1.GetHash (), 0)));
  const CTransaction chain2(mtx);
  mempool.addUnchecked (Entry (chain2));

  mtx.vout.clear ();
  mtx.vout.push_back (CTxOut (COIN, OTHER_ADDR));
  mtx.vout.push_back (CTxOut (COIN, UpdateScript (ADDR, "chain", "y")));
  mtx.vin.push_back (CTxIn (COutPoint (chain2.GetHash (), 0)));
  mtx.vin.push_back (CTxIn (COutPoint (chain1.GetHash (), 1)));
  const CTransaction chain3(mtx);
  mempool.addUnchecked (Entry (chain3));

  CMutableTransaction mtxCurrency;
  mtxCurrency.vin.push_back (CTxIn (COutPoint (chain1.GetHash (), 2)));
  mtxCurrency.vin.push_back (CTxIn (COutPoint (chain3.GetHash (), 0)));
  mempool.addUnchecked (Entry (CTransaction (mtxCurrency)));

  BOOST_CHECK (mempool.lastNameOutput (Name ("new")).IsNull ());
  BOOST_CHECK (mempool.lastNameOutput (Name ("reg"))
                  == COutPoint (txReg.GetHash (), 0));
  BOOST_CHECK (mempool.lastNameOutput (Name ("upd"))
                  == COutPoint (txUpd.GetHash (), 0));
  BOOST_CHECK (mempool.lastNameOutput (Name ("chain"))
                  == COutPoint (chain3.GetHash (), 1));

  BOOST_CHECK_EQUAL (mempool.pendingNameChainLength (Name ("new")), 0);
  BOOST_CHECK_EQUAL (mempool.pendingNameChainLength (Name ("reg")), 1);
  BOOST_CHECK_EQUAL (mempool.pendingNameChainLength (Name ("upd")), 1);
  BOOST_CHECK_EQUAL (mempool.pendingNameChainLength (Name ("chain")), 3);
}

BOOST_FIXTURE_TEST_CASE (name_new, NameMempoolTestSetup)
{
  const auto tx1 = Tx (NewScript (ADDR, "foo", 'a'));
  const auto tx1p = Tx (NewScript (OTHER_ADDR, "foo", 'a'));
  const auto tx2 = Tx (NewScript (ADDR, "foo", 'b'));

  const auto e1 = Entry (tx1);
  const auto e2 = Entry (tx2);
  BOOST_CHECK (e1.isNameNew () && e2.isNameNew ());
  BOOST_CHECK (e1.getNameNewHash () == NewHash ("foo", 'a'));
  BOOST_CHECK (e2.getNameNewHash () == NewHash ("foo", 'b'));

  mempool.addUnchecked (e1);
  mempool.addUnchecked (e2);
  BOOST_CHECK (mempool.checkNameOps (tx1));
  BOOST_CHECK (mempool.checkNameOps (tx2));
  BOOST_CHECK (!mempool.checkNameOps (tx1p));

  mempool.removeRecursive (tx1, MemPoolRemovalReason::EXPIRY);
  mempool.removeRecursive (tx2, MemPoolRemovalReason::EXPIRY);
  BOOST_CHECK (mempool.checkNameOps (tx1));
  BOOST_CHECK (mempool.checkNameOps (tx2));
  BOOST_CHECK (!mempool.checkNameOps (tx1p));
}

BOOST_FIXTURE_TEST_CASE (name_firstupdate, NameMempoolTestSetup)
{
  const auto tx1 = Tx (FirstScript (ADDR, "foo", 'a'));
  const auto tx2 = Tx (FirstScript (ADDR, "foo", 'b'));

  const auto e = Entry (tx1);
  BOOST_CHECK (e.isNameRegistration () && !e.isNameUpdate ());
  BOOST_CHECK (e.getName () == Name ("foo"));

  mempool.addUnchecked (e);
  BOOST_CHECK (mempool.registersName (Name ("foo")));
  BOOST_CHECK (!mempool.updatesName (Name ("foo")));
  BOOST_CHECK (!mempool.checkNameOps (tx2));

  mempool.removeRecursive (tx1, MemPoolRemovalReason::EXPIRY);
  BOOST_CHECK (!mempool.registersName (Name ("foo")));
  BOOST_CHECK (mempool.checkNameOps (tx1));
  BOOST_CHECK (mempool.checkNameOps (tx2));
}

BOOST_FIXTURE_TEST_CASE (name_update, NameMempoolTestSetup)
{
  const auto tx1 = Tx (UpdateScript (ADDR, "foo", "x"));
  const auto tx2 = Tx (UpdateScript (ADDR, "foo", "y"));
  const auto tx3 = Tx (UpdateScript (ADDR, "bar", "z"));

  const auto e1 = Entry (tx1);
  const auto e2 = Entry (tx2);
  const auto e3 = Entry (tx3);
  BOOST_CHECK (!e1.isNameRegistration () && e1.isNameUpdate ());
  BOOST_CHECK (e1.getName () == Name ("foo"));

  mempool.addUnchecked (e1);
  mempool.addUnchecked (e2);
  mempool.addUnchecked (e3);
  BOOST_CHECK (!mempool.registersName (Name ("foo")));
  BOOST_CHECK (mempool.updatesName (Name ("foo")));
  BOOST_CHECK (mempool.updatesName (Name ("bar")));

  mempool.removeRecursive (tx2, MemPoolRemovalReason::EXPIRY);
  BOOST_CHECK (mempool.updatesName (Name ("foo")));
  BOOST_CHECK (mempool.updatesName (Name ("bar")));

  mempool.removeRecursive (tx1, MemPoolRemovalReason::EXPIRY);
  BOOST_CHECK (!mempool.updatesName (Name ("foo")));
  BOOST_CHECK (mempool.updatesName (Name ("bar")));

  mempool.removeRecursive (tx3, MemPoolRemovalReason::EXPIRY);
  BOOST_CHECK (!mempool.updatesName (Name ("foo")));
  BOOST_CHECK (!mempool.updatesName (Name ("bar")));
}

BOOST_FIXTURE_TEST_CASE (mempool_sanity_check, NameMempoolTestSetup)
{
  mempool.addUnchecked (Entry (Tx (NewScript (ADDR, "new", 'a'))));
  mempool.addUnchecked (Entry (Tx (NewScript (ADDR, "new", 'b'))));

  mempool.addUnchecked (Entry (Tx (FirstScript (ADDR, "reg", 'a'))));
  mempool.addUnchecked (Entry (Tx (UpdateScript (ADDR, "reg", "n"))));

  mempool.addUnchecked (Entry (Tx (UpdateScript (ADDR, "upd", "x"))));
  mempool.addUnchecked (Entry (Tx (UpdateScript (ADDR, "upd", "y"))));

  LOCK (cs_main);
  auto& chainState = m_node.chainman->ActiveChainstate ();
  auto& view = chainState.CoinsTip ();

  const CNameScript nameOp(UpdateScript (ADDR, "upd", "o"));
  CNameData data;
  data.fromScript (100, COutPoint (uint256 (), 0), nameOp);
  view.SetName (Name ("upd"), data, false);

  mempool.checkNames (view, 10);
}

namespace
{

/**
 * Helper class that listens to TransactionRemovedFromMempool and records
 * the txid's of all transactions removed.
 */
class NameConflictTracker : public CValidationInterface
{

private:

  /** The txids that have been removed according to our callback.  */
  std::vector<uint256> txids;

public:

  NameConflictTracker ()
  {
    RegisterValidationInterface (this);
  }

  ~NameConflictTracker ()
  {
    UnregisterValidationInterface (this);
  }

  void
  TransactionRemovedFromMempool (const CTransactionRef& ptxn,
                                 const MemPoolRemovalReason reason,
                                 const uint64_t sequence) override
  {
    txids.push_back (ptxn->GetHash ());
  }

  /**
   * Expects the given list of txids to be removed.  Waits for all outstanding
   * callbacks to be processed as needed.
   */
  void
  ExpectTxids (const std::vector<uint256>& expected) const
  {
    while (GetMainSignals ().CallbacksPending () > 0)
      UninterruptibleSleep (std::chrono::milliseconds (10));
    BOOST_CHECK (txids == expected);
  }

};

} // anonymous namespace

BOOST_FIXTURE_TEST_CASE (registration_conflicts, NameMempoolTestSetup)
{
  const auto tx1 = Tx (FirstScript (ADDR, "foo", 'a'));
  const auto tx2 = Tx (FirstScript (ADDR, "foo", 'b'));
  const auto e = Entry (tx1);

  mempool.addUnchecked (e);
  BOOST_CHECK (mempool.registersName (Name ("foo")));
  BOOST_CHECK (!mempool.checkNameOps (tx2));

  NameConflictTracker tracker;
  mempool.removeConflicts (tx2);
  tracker.ExpectTxids ({tx1.GetHash ()});

  BOOST_CHECK (!mempool.registersName (Name ("foo")));
  BOOST_CHECK (mempool.checkNameOps (tx1));
  BOOST_CHECK (mempool.checkNameOps (tx2));
  BOOST_CHECK (mempool.mapTx.empty ());
}

BOOST_FIXTURE_TEST_CASE (expire_conflicts, NameMempoolTestSetup)
{
  const auto tx1 = Tx (UpdateScript (ADDR, "foo", "x"));
  const auto tx2 = Tx (UpdateScript (ADDR, "foo", "y"));
  const auto e1 = Entry (tx1);
  const auto e2 = Entry (tx2);

  mempool.addUnchecked (e1);
  mempool.addUnchecked (e2);
  BOOST_CHECK (mempool.updatesName (Name ("foo")));

  NameConflictTracker tracker;
  mempool.removeExpireConflicts ({Name ("foo")});
  tracker.ExpectTxids ({tx1.GetHash (), tx2.GetHash ()});

  BOOST_CHECK (!mempool.updatesName (Name ("foo")));
  BOOST_CHECK (mempool.mapTx.empty ());
}

BOOST_FIXTURE_TEST_CASE (unexpire_conflicts, NameMempoolTestSetup)
{
  const auto tx = Tx (FirstScript (ADDR, "foo", 'a'));
  const auto e = Entry (tx);

  mempool.addUnchecked (e);
  BOOST_CHECK (mempool.registersName (Name ("foo")));

  NameConflictTracker tracker;
  mempool.removeUnexpireConflicts ({Name ("foo")});
  tracker.ExpectTxids ({tx.GetHash ()});

  BOOST_CHECK (!mempool.registersName (Name ("foo")));
  BOOST_CHECK (mempool.mapTx.empty ());
}

/* ************************************************************************** */

BOOST_AUTO_TEST_SUITE_END ()
