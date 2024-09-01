// Copyright (c) 2024 Rose Turing
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <names/domain.h>

#include <names/applications.h>
#include <names/records.h>

#include <string>
#include <list>

std::string Domain::getName(){
    return name;
}

void Domain::setName(const std::string& text){
    this->name = text;
}

std::list<IPv4Record*> Domain::getIPv4s(){
    return this->ipv4s;
}

void Domain::setIPv4s(const std::list<IPv4Record*> ipv4s){
    this->ipv4s = ipv4s;
}

std::list<IPv6Record*> Domain::getIPv6s(){
    return this->ipv6s;
}

void Domain::setIPv6s(const std::list<IPv6Record*> ipv6s){
    this->ipv6s = ipv6s;
}

Domain::Domain(const std::string& name){
    this->name = name;
}

Domain::Domain(){
    this->name = "";
}


