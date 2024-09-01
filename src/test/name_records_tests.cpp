// Copyright (c) 2024 Rose Turing
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../names/records.h"

#include <test/util/setup_common.h>

#include <string.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(name_records_tests)

IPv4Record test_ipv4_record;

BOOST_AUTO_TEST_CASE( ipv4_record_tests ){

    test_ipv4_record.setDomain("namecoin.bit");
    BOOST_CHECK(test_ipv4_record.getDomain() == "namecoin.bit");

    test_ipv4_record.setAddress("192.168.1.1");
    BOOST_CHECK(test_ipv4_record.getAddress() == "192.168.1.1");

    BOOST_CHECK_EQUAL(test_ipv4_record.validate(), true);

    test_ipv4_record.setAddress("1");
    BOOST_CHECK_EQUAL(test_ipv4_record.validate(), false);

}

IPv6Record test_ipv6_record;

BOOST_AUTO_TEST_CASE( ipv6_record_tests ){

    test_ipv6_record.setAddress("0:0:0:0:0:0:0:1");
    BOOST_CHECK(test_ipv6_record.getAddress() == "0:0:0:0:0:0:0:1");

    BOOST_CHECK_EQUAL(test_ipv6_record.validate(), true);

    test_ipv6_record.setAddress("1");
    BOOST_CHECK_EQUAL(test_ipv6_record.validate(), false);

}

BOOST_AUTO_TEST_SUITE_END()

