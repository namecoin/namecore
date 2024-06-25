// Copyright (c) 2024 Rose Turing
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "records.h"
#include "applications.h"

#include <string>

std::string DomainRecord::getDomain(){
    return domain;
}

void DomainRecord::setDomain(const std::string& text){
    domain = text;
}

DomainRecord::DomainRecord(std::string& domain){
    this->domain = domain;
}

DomainRecord::DomainRecord(){
    this->domain = "";
}

AddressRecord::AddressRecord(std::string& domain, std::string& address)
    :DomainRecord(domain)
{
    this->address = address;
}

std::string AddressRecord::getAddress(){
    return address;
}

void AddressRecord::setAddress(const std::string& text){
    this->address = text;
}

bool IPv4Record::validate() {
    return IsValidIPV4(this->getAddress());
}

bool IPv6Record::validate() {
    return IsValidIPV6(this->getAddress());
}
