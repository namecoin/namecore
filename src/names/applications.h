// Copyright (c) 2021 Jeremy Rand
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_NAMES_APPLICATIONS
#define H_BITCOIN_NAMES_APPLICATIONS

// TODO: remove some of these includes if they're not needed

#include <script/script.h>

#include <stdexcept>
#include <string>

class UniValue;

enum class NameNamespace
{
    Domain,
    DomainData,
    Identity,
    IdentityData,
    NonStandard,
}

/**
 * Enum for the possible encodings of names/values in the RPC interface.
 */
enum class NameEncoding
{
  /**
   * Only printable ASCII characters (code in [0x20, 0x80)) are allowed.
   */
  ASCII,

  /**
   * Valid UTF-8 with printable characters (code >=0x20).
   */
  UTF8,

  /**
   * Hex-encoded arbitrary binary data.
   */
  HEX,
};

/* Accessors to the "default" encodings as configured by startup options.  */
NameEncoding ConfiguredNameEncoding ();
NameEncoding ConfiguredValueEncoding ();

/* The options defaults for the encodings.  */
static constexpr NameEncoding DEFAULT_NAME_ENCODING = NameEncoding::ASCII;
static constexpr NameEncoding DEFAULT_VALUE_ENCODING = NameEncoding::ASCII;

/* Utility functions to convert encodings to/from the enum.  They throw
   std::invalid_argument if the conversion fails.  */
NameEncoding EncodingFromString (const std::string& str);
std::string EncodingToString (NameEncoding enc);

/**
 * Exception that is thrown if a name/value string is invalid according
 * to the chosen encoding.
 */
class InvalidNameString : public std::invalid_argument
{

public:

  InvalidNameString () = delete;

  InvalidNameString (NameEncoding enc, const std::string& invalidStr);

};

/**
 * Encodes a name or value to a string with the given encoding.  Throws
 * InvalidNameString if the data is not valid for the encoding.
 */
std::string EncodeName (const valtype& data, NameEncoding enc);

/**
 * Decodes a string to a raw name/value.  Throws InvalidNameString
 * if the string is invalid for the requested encoding.
 */
valtype DecodeName (const std::string& str, NameEncoding enc);

/**
 * Encodes a name or value to a string that is meant to be printed to logs
 * or returned as part of a larger message (e.g. in the "listtransactions"
 * description).  This converts to ASCII if possible and otherwise returns
 * the string in hex.
 */
std::string EncodeNameForMessage (const valtype& data);

/**
 * Adds an encoded name or value to the UniValue object with the given key.
 * Also adds "key_encoding" with the chosen encoding's name.  If the data
 * cannot be represented with the encoding, a "key_error" field is added
 * instead of "key" itself.
 */
void AddEncodedNameToUniv (UniValue& obj, const std::string& key,
                           const valtype& data, NameEncoding enc);

#endif // H_BITCOIN_NAMES_APPLICATIONS
