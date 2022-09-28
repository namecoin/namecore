// Copyright (c) 2019-2022 Daniel Kraft
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/namehash.h>

#include <hash.h>
#include <primitives/block.h>
#include <script/names.h>
#include <util/system.h>

#include <utility>
#include <vector>

/** Database "key prefix" for the actual hash entries.  */
constexpr uint8_t DB_HASH = 'h';

class NameHashIndex::DB : public BaseIndex::DB
{

public:

  explicit DB (const size_t cache_size, const bool memory, const bool wipe)
    : BaseIndex::DB (gArgs.GetDataDirNet () / "indexes" / "namehash",
                     cache_size, memory, wipe)
  {}

  bool
  ReadPreimage (const uint256& hash, valtype& name) const
  {
    return Read (std::make_pair (DB_HASH, hash), name);
  }

  bool WritePreimages (const std::vector<std::pair<uint256, valtype>>& data);

};

bool
NameHashIndex::DB::WritePreimages (
    const std::vector<std::pair<uint256, valtype>>& data)
{
  CDBBatch batch(*this);
  for (const auto& entry : data)
    batch.Write (std::make_pair (DB_HASH, entry.first), entry.second);

  return WriteBatch (batch);
}

NameHashIndex::NameHashIndex (std::unique_ptr<interfaces::Chain> chain,
                              const size_t cache_size, const bool memory,
                              const bool wipe)
  : BaseIndex(std::move (chain), "namehash"),
    db(std::make_unique<NameHashIndex::DB> (cache_size, memory, wipe))
{}

NameHashIndex::~NameHashIndex () = default;

bool
NameHashIndex::CustomAppend (const interfaces::BlockInfo& block)
{
  std::vector<std::pair<uint256, valtype>> data;
  for (const auto& tx : block.data->vtx)
    for (const auto& out : tx->vout)
      {
        const CNameScript nameOp(out.scriptPubKey);
        if (!nameOp.isNameOp () || nameOp.getNameOp () != OP_NAME_FIRSTUPDATE)
          continue;

        const valtype& name = nameOp.getOpName ();
        const uint256 hash = Hash (name);
        data.emplace_back (hash, name);
      }

  return db->WritePreimages (data);
}

BaseIndex::DB&
NameHashIndex::GetDB () const
{
  return *db;
}

bool
NameHashIndex::FindNamePreimage (const uint256& hash, valtype& name) const
{
  return db->ReadPreimage (hash, name);
}

std::unique_ptr<NameHashIndex> g_name_hash_index;
