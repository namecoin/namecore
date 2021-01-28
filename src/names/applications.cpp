// Copyright (c) 2021 Jeremy Rand
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <names/applications.h>

// TODO: remove some of these includes if they're not needed

#include <logging.h>
#include <util/system.h>
#include <util/strencodings.h>

#include <univalue.h>

#include <cassert>
#include <regex>
#include <sstream>

NameNamespace
NamespaceFromString (const std::string& str)
{
    if (str == "d/")
        return NameNamespace::Domain;
    if (str == "dd/")
        return NameNamespace::DomainData;
    if (str == "id/")
        return NameNamespace::Identity;
    if (str == "idd/")
        return NameNamespace::IdentityData;

    return NameNamespace::NonStandard;
}

std::string
NamespaceToString (NameNamespace ns)
{
    switch (ns)
    {
        case NameNamespace::Domain:
            return "d/";
        case NameNamespace::DomainData:
            return "dd/";
        case NameNamespace::Identity:
            return "id/";
        case NameNamespace::IdentityData:
            return "idd/";
        case NameNamespace::NonStandard:
            return "";
    }

    /* NameNamespace values are only ever created internally in the binary
    and not received externally (except as string).  Thus it should never
    happen (and if it does, is a severe bug) that we see an unexpected
    value.  */
    assert (false);
}

NameNamespace
PurportedNamespaceFromName (const std::string& name)
{
    const auto slashPos = name.find("/");
    if (slashPos == std::string::npos)
    {
        return NameNamespace::NonStandard;
    }

    const std::string namespaceStr = name.substr(0, slashPos+1);
    return NamespaceFromString(namespaceStr);
}

NameNamespace
NamespaceFromName (const std::string& name)
{
    NameNamespace purported = PurportedNamespaceFromName(name);
    const std::string purportedStr = NamespaceToString(purported);
    const auto purportedLen = purportedStr.length();

    switch (purported)
    {
        case NameNamespace::Domain:
            const std::string label = name.substr(purportedLen);

            if (!IsStringValid (label, NameEncoding::ASCII))
            {
                return NameNamespace::NonStandard;
            }

            if (label.length() < 1)
            {
                return NameNamespace::NonStandard;
            }

            // Source: https://github.com/namecoin/proposals/blob/master/ifa-0001.md#keys
            if (label.length() > 63)
            {
                return NameNamespace::NonStandard;
            }

            // Source: https://github.com/namecoin/proposals/blob/master/ifa-0001.md#keys
            std::regex domainPattern("^(xn--)?[a-z0-9]+(-[a-z0-9]+)*$");
            if (!std::regex_match (label, domainPattern))
            {
                return NameNamespace::NonStandard;
            }

            // Reject digits-only labels
            std::regex digitsOnly("^[0-9]+$");
            if (std::regex_match (label, domainPattern))
            {
                return NameNamespace::NonStandard;
            }

            return NameNamespace::Domain;
        case NameNamespace::DomainData:
            const std::string label = name.substr(purportedLen);

            if (!IsStringValid (label, NameEncoding::ASCII))
            {
                return NameNamespace::NonStandard;
            }

            if (label.length() < 1)
            {
                return NameNamespace::NonStandard;
            }

            return NameNamespace::DomainData;
        case NameNamespace::Identity:
            const std::string label = name.substr(purportedLen);

            if (!IsStringValid (label, NameEncoding::ASCII))
            {
                return NameNamespace::NonStandard;
            }

            if (label.length() < 1)
            {
                return NameNamespace::NonStandard;
            }

            // Max id/ identifier length is 255 chars according to wiki spec.
            // But we don't need to check for this, because that's also the max
            // length of an identifier under the Namecoin consensus rules.

            // Same as d/ regex but without IDN prefix.
            // TODO: this doesn't exactly match the https://wiki.namecoin.org spec.
            std::regex identityPattern("^[a-z0-9]+(-[a-z0-9]+)*$");
            if (!std::regex_match (label, identityPattern))
            {
                return NameNamespace::NonStandard;
            }

            return NameNamespace::Identity;
        case NameNamespace::IdentityData:
            const std::string label = name.substr(purportedLen);

            if (!IsStringValid (label, NameEncoding::ASCII))
            {
                return NameNamespace::NonStandard;
            }

            if (label.length() < 1)
            {
                return NameNamespace::NonStandard;
            }

            return NameNamespace::IdentityData;
        case NameNamespace::NonStandard:
            return NameNamespace::NonStandard;
    }

    /* NameNamespace values are only ever created internally in the binary
    and not received externally (except as string).  Thus it should never
    happen (and if it does, is a severe bug) that we see an unexpected
    value.  */
    assert (false);
}

namespace
{

NameEncoding
EncodingFromOptions (const std::string& option, const NameEncoding defaultVal)
{
  const std::string value
      = gArgs.GetArg (option, EncodingToString (defaultVal));
  try
    {
      return EncodingFromString (value);
    }
  catch (const std::invalid_argument& exc)
    {
      LogPrintf ("Invalid value for %s:\n  %s\n  falling back to default %s\n",
                 option, exc.what (), EncodingToString (defaultVal));
      return defaultVal;
    }
}

} // anonymous namespace

NameEncoding
ConfiguredNameEncoding ()
{
  return EncodingFromOptions ("-nameencoding", DEFAULT_NAME_ENCODING);
}

NameEncoding
ConfiguredValueEncoding ()
{
  return EncodingFromOptions ("-valueencoding", DEFAULT_VALUE_ENCODING);
}

NameEncoding
EncodingFromString (const std::string& str)
{
  if (str == "ascii")
    return NameEncoding::ASCII;
  if (str == "utf8")
    return NameEncoding::UTF8;
  if (str == "hex")
    return NameEncoding::HEX;

  throw std::invalid_argument ("invalid name/value encoding: " + str);
}

std::string
EncodingToString (NameEncoding enc)
{
  switch (enc)
    {
    case NameEncoding::ASCII:
      return "ascii";
    case NameEncoding::UTF8:
      return "utf8";
    case NameEncoding::HEX:
      return "hex";
    }

  /* NameEncoding values are only ever created internally in the binary
     and not received externally (except as string).  Thus it should never
     happen (and if it does, is a severe bug) that we see an unexpected
     value.  */
  assert (false);
}

namespace
{

bool
IsStringValid (const std::string& str, const NameEncoding enc)
{
  switch (enc)
    {
    case NameEncoding::ASCII:
      for (const unsigned char c : str)
        if (c < 0x20 || c >= 0x80)
          return false;
      return true;

    case NameEncoding::UTF8:
      if (!IsValidUtf8String (str))
        return false;
      return true;

    case NameEncoding::HEX:
      return str == "" || IsHex (str);
    }

  assert (false);
}

void
VerifyStringValid (const std::string& str, const NameEncoding enc)
{
  if (IsStringValid (str, enc))
    return;

  throw InvalidNameString (enc, str);
}

std::string
InvalidNameStringMessage (const NameEncoding enc, const std::string& invalidStr)
{
  std::ostringstream msg;
  msg << "invalid string for encoding " << EncodingToString (enc) << ":"
      << " " << invalidStr;
  return msg.str ();
}

} // anonymous namespace

InvalidNameString::InvalidNameString (const NameEncoding enc,
                                      const std::string& invalidStr)
  : std::invalid_argument (InvalidNameStringMessage (enc, invalidStr))
{}

std::string
EncodeName (const valtype& data, const NameEncoding enc)
{
  std::string res;
  switch (enc)
    {
    case NameEncoding::ASCII:
    case NameEncoding::UTF8:
      res = std::string (data.begin (), data.end ());
      break;

    case NameEncoding::HEX:
      res = HexStr (data);
      break;
    }

  VerifyStringValid (res, enc);
  return res;
}

valtype
DecodeName (const std::string& str, const NameEncoding enc)
{
  VerifyStringValid (str, enc);

  switch (enc)
    {
    case NameEncoding::ASCII:
    case NameEncoding::UTF8:
      return valtype (str.begin (), str.end ());

    case NameEncoding::HEX:
      return ParseHex (str);
    }

  assert (false);
}

std::string
EncodeNameForMessage (const valtype& data)
{
  try
    {
      return "'" + EncodeName (data, NameEncoding::ASCII) + "'";
    }
  catch (const InvalidNameString& exc)
    {
      return "0x" + EncodeName (data, NameEncoding::HEX);
    }
}

void
AddEncodedNameToUniv (UniValue& obj, const std::string& key,
                      const valtype& data, const NameEncoding enc)
{
  try
    {
      obj.pushKV (key, EncodeName (data, enc));
    }
  catch (const InvalidNameString& exc)
    {
      obj.pushKV (key + "_error", "invalid data for " + EncodingToString (enc));
    }
  obj.pushKV (key + "_encoding", EncodingToString (enc));
}
