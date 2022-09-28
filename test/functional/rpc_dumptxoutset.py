#!/usr/bin/env python3
# Copyright (c) 2019-2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the generation of UTXO snapshots using `dumptxoutset`.
"""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

import hashlib
from pathlib import Path


class DumptxoutsetTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        """Test a trivial usage of the dumptxoutset RPC command."""
        node = self.nodes[0]
        mocktime = node.getblockheader(node.getblockhash(0))['time'] + 1
        node.setmocktime(mocktime)
        self.generate(node, COINBASE_MATURITY)

        FILENAME = 'txoutset.dat'
        out = node.dumptxoutset(FILENAME)
        expected_path = Path(node.datadir) / self.chain / FILENAME

        assert expected_path.is_file()

        assert_equal(out['coins_written'], 100)
        assert_equal(out['base_height'], 100)
        assert_equal(out['path'], str(expected_path))
        # Blockhash should be deterministic based on mocked time.
        assert_equal(
            out['base_hash'],
            'ef4526133e4eaa9ee88b2e5c3fd49a6dd1cb6a282b49dc6d8ea6334786eeb829')

        with open(str(expected_path), 'rb') as f:
            digest = hashlib.sha256(f.read()).hexdigest()
            # UTXO snapshot hash should be deterministic based on mocked time.
            assert_equal(
                digest, '69edbefdd3e951378cc480652d1a3092789aa6cc92435f415fd0c48ccda6646f')

        assert_equal(
            out['txoutset_hash'], '8f24c0bc714b85bf5c104f478f1948e691a5da934515a966e8eb2940adf4d6c7')
        assert_equal(out['nchaintx'], 101)

        # Specifying a path to an existing or invalid file will fail.
        assert_raises_rpc_error(
            -8, '{} already exists'.format(FILENAME),  node.dumptxoutset, FILENAME)
        invalid_path = str(Path(node.datadir) / "invalid" / "path")
        assert_raises_rpc_error(
            -8, "Couldn't open file {}.incomplete for writing".format(invalid_path), node.dumptxoutset, invalid_path)


if __name__ == '__main__':
    DumptxoutsetTest().main()
