// Copyright (c) 2024 Rose Turing
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef H_BITCOIN_NAMES_RECORDS
#define H_BITCOIN_NAMES_RECORDS

#include <string>

class DomainRecord{

    private:
        std::string domain;

    public:
        std::string getDomain();
        void setDomain(const std::string& text);

        DomainRecord(std::string& domain);
        DomainRecord();

        virtual bool validate() = 0;

};

class AddressRecord : public DomainRecord{

    private:
        std::string address;

    public:
        std::string getAddress();
        void setAddress(const std::string& text);

        using DomainRecord::DomainRecord;
        AddressRecord(std::string& domain, std::string& address);

        virtual bool validate() override = 0;
};

class IPv4Record : public AddressRecord{

    public:
        using AddressRecord::AddressRecord;
        bool validate() override;
};

class IPv6Record : public AddressRecord{

    public:
        using AddressRecord::AddressRecord;
        bool validate() override;
};

#endif //H_BITCOIN_NAMES_RECORDS
