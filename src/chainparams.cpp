// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

bool CChainParams::IsHistoricBug(const uint256& txid, unsigned nHeight, BugType& type) const
{
    const std::pair<unsigned, uint256> key(nHeight, txid);
    std::map<std::pair<unsigned, uint256>, BugType>::const_iterator mi;

    mi = mapHistoricBugs.find (key);
    if (mi != mapHistoricBugs.end ())
    {
        type = mi->second;
        return true;
    }

    return false;
}

static CBlock CreateGenesisBlock(const CScript& genesisInputScript, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = genesisInputScript;
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "... choose what comes next.  Lives of your own, or a return to chains. -- V";
    const CScript genesisInputScript = CScript() << 0x1c007fff << CScriptNum(522) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    const CScript genesisOutputScript = CScript() << ParseHex("04b620369050cd899ffbbc4e8ee51e8c4534a855bb463439d63d235d4779685d8b6f4870a238cf365ac94fa13ef9a2a22cd99d0d5ee86dcabcafce36c7acf43ce5") << OP_CHECKSIG;
    return CreateGenesisBlock(genesisInputScript, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Build genesis block for testnet.  In Namecoin, it has a changed timestamp
 * and output script (it uses Bitcoin's).
 */
static CBlock CreateTestnetGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
    const CScript genesisInputScript = CScript() << 0x1d00ffff << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(genesisInputScript, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        /* FIXME: Set once we need the value in main.cpp.  */
        consensus.BIP34Height = -1;
        consensus.BIP34Hash = uint256();
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;

        consensus.nAuxpowChainId = 0x0001;
        consensus.nAuxpowStartHeight = 19200;
        consensus.fStrictChainId = true;
        consensus.nLegacyBlocksBefore = 19200;

        consensus.rules.reset(new Consensus::MainNetConsensus());

        /** 
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xf9;
        pchMessageStart[1] = 0xbe;
        pchMessageStart[2] = 0xb4;
        pchMessageStart[3] = 0xfe;
        vAlertPubKey = ParseHex("04ba207043c1575208f08ea6ac27ed2aedd4f84e70b874db129acb08e6109a3bbb7c479ae22565973ebf0ac0391514511a22cb9345bdb772be20cfbd38be578b0c");
        nDefaultPort = 8334;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1303000001, 0xa21ea192u, 0x1c007fff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000000062b72c5e2ceb45fbc8587e807c155b0da735e6483dfba2f0a9c770"));
        assert(genesis.hashMerkleRoot == uint256S("0x41c62dbd9068c89a449525e3cd5ac61b20ece28c3c38b3f35b2161f0e6d3cb0d"));

        vSeeds.push_back(CDNSSeedData("digi-masters.com", "namecoindnsseed.digi-masters.com"));
        vSeeds.push_back(CDNSSeedData("digi-masters.uk", "namecoindnsseed.digi-masters.uk"));
        vSeeds.push_back(CDNSSeedData("domob.eu", "seed.namecoin.domob.eu"));
        vSeeds.push_back(CDNSSeedData("quisquis.de", "nmc.seed.quisquis.de"));
        vSeeds.push_back(CDNSSeedData("webbtc.com", "dnsseed.namecoin.webbtc.com"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,52);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,13);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,180);
        /* FIXME: Update these below.  */
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (  2016, uint256S("0000000000660bad0d9fbde55ba7ee14ddf766ed5f527e3fbca523ac11460b92"))
            (  4032, uint256S("0000000000493b5696ad482deb79da835fe2385304b841beef1938655ddbc411"))
            (  6048, uint256S("000000000027939a2e1d8bb63f36c47da858e56d570f143e67e85068943470c9"))
            (  8064, uint256S("000000000003a01f708da7396e54d081701ea406ed163e519589717d8b7c95a5"))
            ( 10080, uint256S("00000000000fed3899f818b2228b4f01b9a0a7eeee907abd172852df71c64b06"))
            ( 12096, uint256S("0000000000006c06988ff361f124314f9f4bb45b6997d90a7ee4cedf434c670f"))
            ( 14112, uint256S("00000000000045d95e0588c47c17d593c7b5cb4fb1e56213d1b3843c1773df2b"))
            ( 16128, uint256S("000000000001d9964f9483f9096cf9d6c6c2886ed1e5dec95ad2aeec3ce72fa9"))
            ( 18940, uint256S("00000000000087f7fc0c8085217503ba86f796fa4984f7e5a08b6c4c12906c05"))
            ( 30240, uint256S("e1c8c862ff342358384d4c22fa6ea5f669f3e1cdcf34111f8017371c3c0be1da"))
            ( 57000, uint256S("aa3ec60168a0200799e362e2b572ee01f3c3852030d07d036e0aa884ec61f203"))
            (112896, uint256S("73f880e78a04dd6a31efc8abf7ca5db4e262c4ae130d559730d6ccb8808095bf"))
            (182000, uint256S("d47b4a8fd282f635d66ce34ebbeb26ffd64c35b41f286646598abfd813cba6d9"))
            (193000, uint256S("3b85e70ba7f5433049cfbcf0ae35ed869496dbedcd1c0fafadb0284ec81d7b58")),
            1409050213, // * UNIX timestamp of last checkpoint block
            1441814,    // * total number of transactions between genesis and last checkpoint
                        //   (the tx=... number in the SetBestChain debug.log lines)
            1635.0      // * estimated number of transactions per day after checkpoint
        };

        /* See also doc/NamecoinBugs.txt for more explanation on the
           historical bugs added below.  */

        /* These transactions have name outputs but a non-Namecoin tx version.
           They contain NAME_NEWs, which are fine, and also NAME_FIRSTUPDATE.
           The latter are not interpreted by namecoind, thus also ignore
           them for us here.  */
        addBug(98423, "bff3ed6873e5698b97bf0c28c29302b59588590b747787c7d1ef32decdabe0d1", BUG_FULLY_IGNORE);
        addBug(98424, "e9b211007e5cac471769212ca0f47bb066b81966a8e541d44acf0f8a1bd24976", BUG_FULLY_IGNORE);
        addBug(98425, "8aa2b0fc7d1033de28e0192526765a72e9df0c635f7305bdc57cb451ed01a4ca", BUG_FULLY_IGNORE);

        /* These are non-Namecoin tx that contain just NAME_NEWs.  Those were
           handled with a special rule previously, but now they are fully
           disallowed and we handle the few exceptions here.  It is fine to
           "ignore" them, as their outputs need no special Namecoin handling
           before they are reused in a NAME_FIRSTUPDATE.  */
        addBug(98318, "0ae5e958ff05ad8e273222656d98d076097def6d36f781a627c584b859f4727b", BUG_FULLY_IGNORE);
        addBug(98321, "aca8ce46da1bbb9bb8e563880efcd9d6dd18342c446d6f0e3d4b964a990d1c27", BUG_FULLY_IGNORE);
        addBug(98424, "c29b0d9d478411462a8ac29946bf6fdeca358a77b4be15cd921567eb66852180", BUG_FULLY_IGNORE);
        addBug(98425, "221719b360f0c83fa5b1c26fb6b67c5e74e4e7c6aa3dce55025da6759f5f7060", BUG_FULLY_IGNORE);
        addBug(193518, "597370b632efb35d5ed554c634c7af44affa6066f2a87a88046532d4057b46f8", BUG_FULLY_IGNORE);
        addBug(195605, "0bb8c7807a9756aefe62c271770b313b31dee73151f515b1ac2066c50eaeeb91", BUG_FULLY_IGNORE);
        addBug(195639, "3181930765b970fc43cd31d53fc6fc1da9439a28257d9067c3b5912d23eab01c", BUG_FULLY_IGNORE);
        addBug(195639, "e815e7d774937d96a4b265ed4866b7e3dc8d9f2acb8563402e216aba6edd1e9e", BUG_FULLY_IGNORE);
        addBug(195639, "cdfe6eda068e09fe760a70bec201feb041b8c660d0e98cbc05c8aa4106eae6ab", BUG_FULLY_IGNORE);
        addBug(195641, "1e29e937b2a9e1f18af500371b8714157cf5ac7c95461913e08ce402de64ae75", BUG_FULLY_IGNORE);
        addBug(195648, "d44ed6c0fac251931465f9123ada8459ec954cc6c7b648a56c9326ff7b13f552", BUG_FULLY_IGNORE);
        addBug(197711, "dd77aea50a189935d0ef36a04856805cd74600a53193c539eb90c1e1c0f9ecac", BUG_FULLY_IGNORE);
        addBug(204151, "f31875dfaf94bd3a93cfbed0e22d405d1f2e49b4d0750cb13812adc5e57f1e47", BUG_FULLY_IGNORE);

        /* This transaction has both a NAME_NEW and a NAME_FIRSTUPDATE as
           inputs.  This was accepted due to the "argument concatenation" bug.
           It is fine to accept it as valid and just process the NAME_UPDATE
           output that builds on the NAME_FIRSTUPDATE input.  (NAME_NEW has no
           special side-effect in applying anyway.)  */
        addBug(99381, "774d4c446cecfc40b1c02fdc5a13be6d2007233f9d91daefab6b3c2e70042f05", BUG_FULLY_APPLY);

        /* These were libcoin's name stealing bugs.  */
        addBug(139872, "2f034f2499c136a2c5a922ca4be65c1292815c753bbb100a2a26d5ad532c3919", BUG_IN_UTXO);
        addBug(139936, "c3e76d5384139228221cce60250397d1b87adf7366086bc8d6b5e6eee03c55c7", BUG_FULLY_IGNORE);
    }

    int DefaultCheckNameDB () const
    {
        return -1;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nMajorityEnforceBlockUpgrade = 51;
        consensus.nMajorityRejectBlockOutdated = 75;
        consensus.nMajorityWindow = 100;
        /* FIXME: Set once we need the value in main.cpp.  */
        consensus.BIP34Height = -1;
        consensus.BIP34Hash = uint256();
        consensus.powLimit = uint256S("0000000fffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.nMinDifficultySince = 1394838000; // 15 Mar 2014
        consensus.fPowNoRetargeting = false;

        consensus.nAuxpowStartHeight = 0;
        consensus.nAuxpowChainId = 0x0001;
        consensus.fStrictChainId = false;
        consensus.nLegacyBlocksBefore = -1;

        consensus.rules.reset(new Consensus::TestNetConsensus());

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xfe;
        vAlertPubKey = ParseHex("04fc9702847840aaf195de8442ebecedf5b095cdbb9bc716bda9110971b28a49e0ead8564ff0db22209e0374782c093bb899692d524e9d6a6956e7c5ecbcd68284");
        nDefaultPort = 18334;
        nPruneAfterHeight = 1000;

        genesis = CreateTestnetGenesisBlock(1296688602, 0x16ec0bff, 0x1d07fff8, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("00000007199508e34a9ff81e6ec0c477a4cccff2a4767a8eee39c11db367b008"));
        assert(genesis.hashMerkleRoot == uint256S("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        vSeeds.push_back(CDNSSeedData("webbtc.com", "dnsseed.test.namecoin.webbtc.com"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        /* FIXME: Update these below.  */
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (  2016, uint256S("00000000b9e4132e1a803114bc88df3e49184a3c794c01a6eac334f12f4ccadb"))
            (  4032, uint256S("00000003fbc13a48b8de5c8742044c84b800edeabff8b39f7f23ac572c6d80ce"))
            (  8064, uint256S("f594a75db40244bc7baa808a695f796ba81cae5bb48fa920e367cdd31dbfb0e3"))
            ( 10080, uint256S("398d44a1a6e58dce3f7463217f677c2532e42a83696dcc5d4d97329c00a10891"))
            ( 12096, uint256S("22c9278493cda563565fc2a4250eff48bd68ed40cb5fb30029ca08ea6120ddab"))
            ( 14112, uint256S("83bade3e3d88845eb52de90311a8017b1cdf725b15d19bc89c47a568f7b4e08c"))
            ( 16128, uint256S("f456354835623f733bb928ed77d97ae06b96ad6c40aba63f51f94f06e905effc"))
            ( 18144, uint256S("c0ec570117822ca3c76abd1d10449b283d8ad39c64290d6abafe2bed23917886"))
            ( 34715, uint256S("0000000580cf4342f869e278d94d7e67d2ac8cae4a411082e0fd518a8091ebf2"))
            ( 48384, uint256S("00000001d528af69dce584f882e3bdb36127104988607b726591cc5e62287922"))
            ( 60480, uint256S("d3af823c32e890ca589dd4277aa4d27b8cd290396b7e0eeeee5121481fd43ca5")),
            1407617760,
            93107,
            230
        };

        assert(mapHistoricBugs.empty());
    }

    int DefaultCheckNameDB () const
    {
        return -1;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMajorityEnforceBlockUpgrade = 750;
        consensus.nMajorityRejectBlockOutdated = 950;
        consensus.nMajorityWindow = 1000;
        consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
        consensus.BIP34Hash = uint256();
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.nMinDifficultySince = 0;
        consensus.fPowNoRetargeting = true;

        consensus.nAuxpowStartHeight = 0;
        consensus.nAuxpowChainId = 0x0001;
        consensus.fStrictChainId = true;
        consensus.nLegacyBlocksBefore = 0;

        consensus.rules.reset(new Consensus::RegTestConsensus());

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18445;
        nPruneAfterHeight = 1000;

        genesis = CreateTestnetGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("5287b3809b71433729402429b7d909a853cfac5ed40f09117b242c275e6b2d63")),
            0,
            0,
            0
        };
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        assert(mapHistoricBugs.empty());
    }

    int DefaultCheckNameDB () const
    {
        return 0;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}
