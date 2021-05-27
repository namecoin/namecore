// Copyright (c) 2014-2021 Daniel Kraft
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_NAMES_MEMPOOL
#define H_BITCOIN_NAMES_MEMPOOL

#include <names/common.h>
#include <primitives/transaction.h>
#include <uint256.h>
#include <sync.h>

#include <map>
#include <memory>
#include <set>

class CChainState;
class ChainstateManager;
class CTxMemPool;
class CTxMemPoolEntry;

/**
 * Default limit for the length of pending name chains that can be
 * created with name_update.
 *
 * For now, we do not allow any chains (besides a single operation)
 * at all.  This is because we introduced support for them only recently,
 * so that most miners/relaying nodes may not accept them at all.  Once
 * the network has upgraded sufficiently, we should increase this to a value
 * higher but still lower than the general mempool ancestor limit.
 */
static constexpr unsigned DEFAULT_NAME_CHAIN_LIMIT = 1;

/**
 * Handle the name component of the transaction mempool.  This keeps track
 * of name operations that are in the mempool and ensures that all transactions
 * kept are consistent.  For instance, no two transactions are allowed to
 * register the same name, and name registration transactions are removed if a
 * conflicting registration makes it into a block.
 */
class CNameMemPool
{

private:

  /** The parent mempool object.  Used to e.g. remove conflicting tx.  */
  CTxMemPool& pool;

  /**
   * Keep track of names that are registered by transactions in the pool.
   * For any given name, at most one registering transaction is allowed in the
   * mempool (as all others would conflict with it).
   */
  std::map<valtype, uint256> mapNameRegs;

  /**
   * Keep track of all transactions that update a given name.  For each name,
   * this may be a whole chain of updates.  This field is used to remove the
   * transactions from the mempool should the name expire (and the updates
   * thus become invalid).
   *
   * We also use this to determine the length of chains of pending name_update
   * operations.
   */
  std::map<valtype, std::set<uint256>> updates;

  /**
   * Map NAME_NEW hashes to the corresponding transaction IDs.  This is
   * data that is kept only in memory but never cleared (until a restart).
   * It is used to prevent "name_new stealing", at least in a "soft" way.
   */
  std::map<valtype, uint256> mapNameNews;

public:

  /**
   * Constructs with reference to parent mempool.
   */
  explicit CNameMemPool (CTxMemPool& p)
    : pool(p)
  {}

  /**
   * Checks whether a particular name is being registered by
   * some transaction in the mempool.  Does not lock, this is
   * done by the parent mempool (which calls through afterwards).
   */
  bool
  registersName (const valtype& name) const
  {
    return mapNameRegs.count (name) > 0;
  }

  /**
   * Checks whether a particular name has at least one pending update.
   * Does not lock.
   */
  bool
  updatesName (const valtype& name) const
  {
    const auto mit = updates.find (name);
    if (mit == updates.end ())
      return false;
    return !mit->second.empty ();
  }

  /**
   * Returns the number of pending operations on this name in the mempool.
   * In other words, this is the "length" of the chain of operations that
   * are already pending.
   */
  unsigned pendingChainLength (const valtype& name) const;

  /**
   * Returns the last outpoint of a (potential) chain of pending name operations
   * for the given name.  This is the output that should be spent with the
   * next constructed name update.  Returns a null outpoint if the name
   * is not being updated at the moment.
   */
  COutPoint lastNameOutput (const valtype& name) const;

  /**
   * Clears all data.
   */
  void
  clear ()
  {
    mapNameRegs.clear ();
    updates.clear ();
    mapNameNews.clear ();
  }

  /**
   * Adds an entry without checking it.  It should have been checked
   * already.  If this conflicts with the mempool, it may throw.
   */
  void addUnchecked (const CTxMemPoolEntry& entry);

  /**
   * Removes the given mempool entry.  It is assumed that it is present.
   */
  void remove (const CTxMemPoolEntry& entry);

  /**
   * Removes conflicts for the given tx, based on name operations.  I.e.,
   * if the tx registers a name that conflicts with another registration
   * in the mempool, detect this and remove the mempool tx accordingly.
   */
  void removeConflicts (const CTransaction& tx);

  /**
   * Removes conflicts in the mempool due to unexpired names.  This removes
   * conflicting name registrations that are no longer possible.
   */
  void removeUnexpireConflicts (const std::set<valtype>& unexpired);
  /**
   * Removes conflicts in the mempool due to expired names.  This removes
   * conflicting name updates that are no longer possible.
   */
  void removeExpireConflicts (const std::set<valtype>& expired);

  /**
   * Performs sanity checks.  Throws if it fails.
   */
  void check (ChainstateManager& chainman, CChainState& active_chainstate) const;

  /**
   * Checks if a tx can be added (based on name criteria) without
   * causing a conflict.
   */
  bool checkTx (const CTransaction& tx) const;

};

#endif // H_BITCOIN_NAMES_MEMPOOL
