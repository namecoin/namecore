// Copyright (c) 2018 Daniel Kraft
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <names/encoding.h>

#include <logging.h>
#include <util/system.h>
#include <util/strencodings.h>

#include <univalue.h>

#include <cassert>
#include <sstream>

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

namespace
{

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
