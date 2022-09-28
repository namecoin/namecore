// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <deploymentinfo.h>
#include <hash.h> // for signet block challenge hash
#include <script/interpreter.h>
#include <util/string.h>
#include <util/system.h>

#include <assert.h>

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
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
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
 * Main network on which people trade goods and services.
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 475000;
        /* Note that these are not the actual activation heights, but blocks
           after them.  They are too deep in the chain to be ever reorged,
           and thus this is also fine.  */
        consensus.BIP34Height = 250000;
        consensus.BIP65Height = 335000;
        consensus.BIP66Height = 250000;
        /* Namecoin activates CSV/Segwit with BIP16.  */
        consensus.CSVHeight = 475000;
        consensus.SegwitHeight = 475000;
        consensus.MinBIP9WarningHeight = 477016; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // The best chain should have at least this much work.
        // The value is the chain work of the Namecoin mainnet chain at height
        // 625'000, with best block hash:
        // fe5a2bfbdbb33b59d6e96271791c9fd130dc3d1f6c08a4e3f0457a2287498b9e
        consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000002623ca68ee9a9b2b0bbb3aaa");
        consensus.defaultAssumeValid = uint256S("0xfe5a2bfbdbb33b59d6e96271791c9fd130dc3d1f6c08a4e3f0457a2287498b9e"); // 625'000

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
        nDefaultPort = 8334;
        nPruneAfterHeight = 100000;
        m_assumed_blockchain_size = 8;
        m_assumed_chain_state_size = 1;

        genesis = CreateGenesisBlock(1303000001, 0xa21ea192u, 0x1c007fff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x000000000062b72c5e2ceb45fbc8587e807c155b0da735e6483dfba2f0a9c770"));
        assert(genesis.hashMerkleRoot == uint256S("0x41c62dbd9068c89a449525e3cd5ac61b20ece28c3c38b3f35b2161f0e6d3cb0d"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as an addrfetch if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("seed.namecoin.libreisp.se."); // Jonas Ostman
        vSeeds.emplace_back("nmc.seed.quisquis.de."); // Peter Conrad
        vSeeds.emplace_back("seed.nmc.markasoftware.com."); // Mark Polyakov
        vSeeds.emplace_back("dnsseed1.nmc.dotbit.zone."); // Stefan Stere
        vSeeds.emplace_back("dnsseed2.nmc.dotbit.zone."); // Stefan Stere
        vSeeds.emplace_back("dnsseed.nmc.testls.space."); // mjgill89

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,52);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,13);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,180);
        /* FIXME: Update these below.  */
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "nc";

        vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_main), std::end(chainparams_seed_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {  2016, uint256S("0000000000660bad0d9fbde55ba7ee14ddf766ed5f527e3fbca523ac11460b92")},
                {  4032, uint256S("0000000000493b5696ad482deb79da835fe2385304b841beef1938655ddbc411")},
                {  6048, uint256S("000000000027939a2e1d8bb63f36c47da858e56d570f143e67e85068943470c9")},
                {  8064, uint256S("000000000003a01f708da7396e54d081701ea406ed163e519589717d8b7c95a5")},
                { 10080, uint256S("00000000000fed3899f818b2228b4f01b9a0a7eeee907abd172852df71c64b06")},
                { 12096, uint256S("0000000000006c06988ff361f124314f9f4bb45b6997d90a7ee4cedf434c670f")},
                { 14112, uint256S("00000000000045d95e0588c47c17d593c7b5cb4fb1e56213d1b3843c1773df2b")},
                { 16128, uint256S("000000000001d9964f9483f9096cf9d6c6c2886ed1e5dec95ad2aeec3ce72fa9")},
                { 18940, uint256S("00000000000087f7fc0c8085217503ba86f796fa4984f7e5a08b6c4c12906c05")},
                { 30240, uint256S("e1c8c862ff342358384d4c22fa6ea5f669f3e1cdcf34111f8017371c3c0be1da")},
                { 57000, uint256S("aa3ec60168a0200799e362e2b572ee01f3c3852030d07d036e0aa884ec61f203")},
                {112896, uint256S("73f880e78a04dd6a31efc8abf7ca5db4e262c4ae130d559730d6ccb8808095bf")},
                {182000, uint256S("d47b4a8fd282f635d66ce34ebbeb26ffd64c35b41f286646598abfd813cba6d9")},
                {193000, uint256S("3b85e70ba7f5433049cfbcf0ae35ed869496dbedcd1c0fafadb0284ec81d7b58")},
                {250000, uint256S("514ec75480df318ffa7eb4eff82e1c583c961aa64cce71b5922662f01ed1686a")},
                {400000, uint256S("9d90cb7a56827c70b13192f1b2c6d6b2e6188abc13c5112d47cfd2f8efba8cce")},
                {474000, uint256S("83a3251ce38bf08481c3b6ab9128e5d0cbeedd0907dae64029d8669f35a64ad2")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
         // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 fe5a2bfbdbb33b59d6e96271791c9fd130dc3d1f6c08a4e3f0457a2287498b9e
            .nTime    = 1660391035,
            .nTxCount = 6482131,
            .dTxRate  = 0.01497457125189064,
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

    int DefaultCheckNameDB () const override
    {
        return -1;
    }
};

/**
 * Testnet (v3): public test network which is reset from time to time.
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 232000;
        /* As before, these are not the actual activation heights but some
           blocks after them.  */
        consensus.BIP34Height = 130000;
        consensus.BIP65Height = 130000;
        consensus.BIP66Height = 130000;
        /* Namecoin activates CSV/Segwit with BIP16.  */
        consensus.CSVHeight = 232000;
        consensus.SegwitHeight = 232000;
        consensus.MinBIP9WarningHeight = 234016; // segwit activation height + miner confirmation window
        consensus.powLimit = uint256S("0000000fffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.nMinDifficultySince = 1394838000; // 15 Mar 2014
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Deployment of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        // The best chain should have at least this much work.
        // The value is the chain work of the Namecoin testnet chain at height
        // 233,000, with best block hash:
        // bc66fc22b8a2988bdc519c4c6aa431bb57201e5102ad8b8272fcde2937b4d2f7
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000ed17e3004a583c4f");
        consensus.defaultAssumeValid = uint256S("0xbc66fc22b8a2988bdc519c4c6aa431bb57201e5102ad8b8272fcde2937b4d2f7"); // 233,100

        consensus.nAuxpowStartHeight = 0;
        consensus.nAuxpowChainId = 0x0001;
        consensus.fStrictChainId = false;
        consensus.nLegacyBlocksBefore = -1;

        consensus.rules.reset(new Consensus::TestNetConsensus());

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xfe;
        nDefaultPort = 18334;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 1;
        m_assumed_chain_state_size = 1;

        genesis = CreateTestnetGenesisBlock(1296688602, 0x16ec0bff, 0x1d07fff8, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("00000007199508e34a9ff81e6ec0c477a4cccff2a4767a8eee39c11db367b008"));
        assert(genesis.hashMerkleRoot == uint256S("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("dnsseed.test.namecoin.webbtc.com."); // Marius Hanne
        vSeeds.emplace_back("ncts.roanapur.info."); // Yanmaani

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        /* FIXME: Update these below.  */
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tn";

        // FIXME: Namecoin has no fixed seeds for testnet, so that the line
        // below errors out.  Use it once we have testnet seeds.
        //vFixedSeeds = std::vector<uint8_t>(std::begin(chainparams_seed_test), std::end(chainparams_seed_test));
        vFixedSeeds.clear();

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {  2016, uint256S("00000000b9e4132e1a803114bc88df3e49184a3c794c01a6eac334f12f4ccadb")},
                {  4032, uint256S("00000003fbc13a48b8de5c8742044c84b800edeabff8b39f7f23ac572c6d80ce")},
                {  8064, uint256S("f594a75db40244bc7baa808a695f796ba81cae5bb48fa920e367cdd31dbfb0e3")},
                { 10080, uint256S("398d44a1a6e58dce3f7463217f677c2532e42a83696dcc5d4d97329c00a10891")},
                { 12096, uint256S("22c9278493cda563565fc2a4250eff48bd68ed40cb5fb30029ca08ea6120ddab")},
                { 14112, uint256S("83bade3e3d88845eb52de90311a8017b1cdf725b15d19bc89c47a568f7b4e08c")},
                { 16128, uint256S("f456354835623f733bb928ed77d97ae06b96ad6c40aba63f51f94f06e905effc")},
                { 18144, uint256S("c0ec570117822ca3c76abd1d10449b283d8ad39c64290d6abafe2bed23917886")},
                { 34715, uint256S("0000000580cf4342f869e278d94d7e67d2ac8cae4a411082e0fd518a8091ebf2")},
                { 48384, uint256S("00000001d528af69dce584f882e3bdb36127104988607b726591cc5e62287922")},
                { 60480, uint256S("d3af823c32e890ca589dd4277aa4d27b8cd290396b7e0eeeee5121481fd43ca5")},
                {130000, uint256S("e0a05455d89a54bb7c1b5bb785d6b1b7c5bda42ed4ce8dc19d68652ba8835954")},
                {231000, uint256S("4964042a9c9ca5f1e104246f4d70ecb4f7217e02a2656379560e4ee4590f9870")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            // TODO to be specified in a future patch.
        };

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 4096 bc66fc22b8a2988bdc519c4c6aa431bb57201e5102ad8b8272fcde2937b4d2f7
            .nTime    = 1573859059,
            .nTxCount = 276907,
            .dTxRate  = 0.0002269357829851853,
        };

        assert(mapHistoricBugs.empty());
    }

    int DefaultCheckNameDB () const override
    {
        return -1;
    }
};

/**
 * Signet: test network with an additional consensus parameter (see BIP325).
 */
class SigNetParams : public CChainParams {
public:
    explicit SigNetParams(const ArgsManager& args) {
        std::vector<uint8_t> bin;
        vSeeds.clear();

        if (!args.IsArgSet("-signetchallenge")) {
            /* FIXME: Adjust the default signet challenge to something else if
               we want to use signet for Namecoin.  */
            bin = ParseHex("512103ad5e0edad18cb1f0fc0d28a3d4f1f3e445640337489abb10404f2d1e086be430210359ef5021964fe22d6f8e05b2463c9540ce96883fe3b278760f048f5189f2e6c452ae");
            //vSeeds.emplace_back("178.128.221.177");

            consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000001291fc22898");
            consensus.defaultAssumeValid = uint256S("0x000000d1a0e224fa4679d2fb2187ba55431c284fa1b74cbc8cfda866fd4d2c09"); // 105495
            m_assumed_blockchain_size = 1;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                // Data from RPC: getchaintxstats 4096 000000d1a0e224fa4679d2fb2187ba55431c284fa1b74cbc8cfda866fd4d2c09
                .nTime    = 1661702566,
                .nTxCount = 1903567,
                .dTxRate  = 0.02336701143027275,
            };
        } else {
            const auto signet_challenge = args.GetArgs("-signetchallenge");
            if (signet_challenge.size() != 1) {
                throw std::runtime_error(strprintf("%s: -signetchallenge cannot be multiple values.", __func__));
            }
            bin = ParseHex(signet_challenge[0]);

            consensus.nMinimumChainWork = uint256{};
            consensus.defaultAssumeValid = uint256{};
            m_assumed_blockchain_size = 0;
            m_assumed_chain_state_size = 0;
            chainTxData = ChainTxData{
                0,
                0,
                0,
            };
            LogPrintf("Signet with challenge %s\n", signet_challenge[0]);
        }

        if (args.IsArgSet("-signetseednode")) {
            vSeeds = args.GetArgs("-signetseednode");
        }

        strNetworkID = CBaseChainParams::SIGNET;
        consensus.signet_blocks = true;
        consensus.signet_challenge.assign(bin.begin(), bin.end());
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.BIP16Height = 1;
        consensus.BIP34Height = 1;
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1815; // 90% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("00000377ae000000000000000000000000000000000000000000000000000000");
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // Activation of Taproot (BIPs 340-342)
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        consensus.nAuxpowStartHeight = 0;
        consensus.nAuxpowChainId = 0x0001;
        consensus.fStrictChainId = true;
        consensus.nLegacyBlocksBefore = 0;

        consensus.rules.reset(new Consensus::TestNetConsensus());

        // message start is defined as the first 4 bytes of the sha256d of the block script
        HashWriter h{};
        h << consensus.signet_challenge;
        uint256 hash = h.GetHash();
        memcpy(pchMessageStart, hash.begin(), 4);

        nDefaultPort = 38334;
        nPruneAfterHeight = 1000;

        genesis = CreateTestnetGenesisBlock(1598918400, 52613770, 0x1e0377ae, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x00000008819873e925422c1ff0f99f7cc9bbb232af63a077a480a3633bee1ef6"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "tb";

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = false;
    }

    int DefaultCheckNameDB () const override
    {
        return -1;
    }
};

/**
 * Regression test: intended for private networks only. Has minimal difficulty to ensure that
 * blocks can be found instantly.
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID =  CBaseChainParams::REGTEST;
        consensus.signet_blocks = false;
        consensus.signet_challenge.clear();
        consensus.nSubsidyHalvingInterval = 150;
        consensus.BIP16Height = 0;
        consensus.BIP34Height = 1; // Always active unless overridden
        consensus.BIP65Height = 1;  // Always active unless overridden
        consensus.BIP66Height = 1;  // Always active unless overridden
        consensus.CSVHeight = 1;    // Always active unless overridden
        consensus.SegwitHeight = 0; // Always active unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 14 * 24 * 60 * 60; // two weeks
        consensus.nPowTargetSpacing = 10 * 60;
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.nMinDifficultySince = 0;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].bit = 2;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nStartTime = Consensus::BIP9Deployment::ALWAYS_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TAPROOT].min_activation_height = 0; // No activation delay

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

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
        nPruneAfterHeight = args.GetBoolArg("-fastprune", false) ? 100 : 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        genesis = CreateTestnetGenesisBlock(1296688602, 2, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
        assert(genesis.hashMerkleRoot == uint256S("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();
        vSeeds.emplace_back("dummySeed.invalid.");

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("5287b3809b71433729402429b7d909a853cfac5ed40f09117b242c275e6b2d63")},
            }
        };

        m_assumeutxo_data = MapAssumeutxo{
            {
                110,
                {AssumeutxoHash{uint256S("0xa6692e681f4819b0e21ca3b28d01fdc16045769359d14a9c4f558ca1b30736b7")}, 110},
            },
            {
                200,
                {AssumeutxoHash{uint256S("0x51c8d11d8b5c1de51543c579736e786aa2736206d1e11e627568029ce092cf62")}, 200},
            },
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "ncrt";

        assert(mapHistoricBugs.empty());
    }

    int DefaultCheckNameDB () const override
    {
        return 0;
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout, int min_activation_height)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
        consensus.vDeployments[d].min_activation_height = min_activation_height;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

static void MaybeUpdateHeights(const ArgsManager& args, Consensus::Params& consensus)
{
    for (const std::string& arg : args.GetArgs("-testactivationheight")) {
        const auto found{arg.find('@')};
        if (found == std::string::npos) {
            throw std::runtime_error(strprintf("Invalid format (%s) for -testactivationheight=name@height.", arg));
        }
        const auto name{arg.substr(0, found)};
        const auto value{arg.substr(found + 1)};
        int32_t height;
        if (!ParseInt32(value, &height) || height < 0 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Invalid height value (%s) for -testactivationheight=name@height.", arg));
        }
        if (name == "bip16") {
            consensus.BIP16Height = int{height};
        } else if (name == "segwit") {
            consensus.SegwitHeight = int{height};
        } else if (name == "bip34") {
            consensus.BIP34Height = int{height};
        } else if (name == "dersig") {
            consensus.BIP66Height = int{height};
        } else if (name == "cltv") {
            consensus.BIP65Height = int{height};
        } else if (name == "csv") {
            consensus.CSVHeight = int{height};
        } else {
            throw std::runtime_error(strprintf("Invalid name (%s) for -testactivationheight=name@height.", arg));
        }
    }
}

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    MaybeUpdateHeights(args, consensus);

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams = SplitString(strDeployment, ':');
        if (vDeploymentParams.size() < 3 || 4 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
        }
        int64_t nStartTime, nTimeout;
        int min_activation_height = 0;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 4 && !ParseInt32(vDeploymentParams[3], &min_activation_height)) {
            throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
        }
        bool found = false;
        for (int j=0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout, min_activation_height);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%d\n", vDeploymentParams[0], nStartTime, nTimeout, min_activation_height);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const ArgsManager& args, const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN) {
        return std::unique_ptr<CChainParams>(new CMainParams());
    } else if (chain == CBaseChainParams::TESTNET) {
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    } else if (chain == CBaseChainParams::SIGNET) {
        return std::unique_ptr<CChainParams>(new SigNetParams(args));
    } else if (chain == CBaseChainParams::REGTEST) {
        return std::unique_ptr<CChainParams>(new CRegTestParams(args));
    }
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(gArgs, network);
}
