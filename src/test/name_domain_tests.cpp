// Copyright (c) 2024 Rose Turing
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../names/domain.h"

#include "../names/records.h"
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <list>

BOOST_AUTO_TEST_SUITE(name_domain_tests)

Domain test_domain;
BOOST_AUTO_TEST_CASE( domain_tests ){

    std::list<IPv4Record*> test_ipv4_list;

    std::list<IPv6Record*> test_ipv6_list;

    test_domain.setName("namecoin.bit");
    BOOST_CHECK(test_domain.getName() == "namecoin.bit");

    test_domain.setIPv4s(test_ipv4_list);
    BOOST_CHECK(test_domain.getIPv4s() == test_ipv4_list);

    test_domain.setIPv6s(test_ipv6_list);
    BOOST_CHECK(test_domain.getIPv6s() == test_ipv6_list);

};

BOOST_AUTO_TEST_SUITE_END()
