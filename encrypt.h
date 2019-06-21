//
// Created by TYTY on 2019-06-03 003.
//

#ifndef FILE_TRANSFER_ENCRYPT_H_
#define FILE_TRANSFER_ENCRYPT_H_

#include <cstring>
#include <string>
#include <iomanip>
#include <iostream>

#include <boost/io/ios_state.hpp>

#include "./third_party/cryptopp/scrypt.h"
#include "./third_party/cryptopp/modes.h"
#include "./third_party/cryptopp/aes.h"
#include "./third_party/cryptopp/filters.h"
#include "./third_party/cryptopp/sha.h"
#include "./third_party/cryptopp/hex.h"

#include "./third_party/easyloggingpp/src/easylogging++.h"

/// \file encrypt.h
/// \brief Header for encrypt related functions
/// \note All things are in `encrypt` namespace

namespace encrypt {

const unsigned char salt[33] = "qwertyuiopasdfghjklzxcvbnm123456";

typedef unsigned char byte;

using std::string;

/// \brief @debug show raw value of the given data.
/// \param str data to show
/// \param length length of the data. Not required for string as it
///         holds size info.
inline void _debug_value(const string &str) {
  boost::io::ios_all_saver guard(std::cout);

  for (int i = 0; i < str.size(); i++) {
    std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(str[i])) << " ";
  }

  std::cout << std::endl;
}

inline void _debug_value(const char *str, int length) {
  boost::io::ios_all_saver guard(std::cout);

  for (int i = 0; i < length; i++) {
    std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(str[i])) << " ";
  }

  std::cout << std::endl;
}

/// \brief Scrypt algorithm
/// \detail this function will convert the simple plain password into fixed
///         length complex key to ensure safety.
///         See <https://en.wikipedia.org/wiki/Scrypt>
/// \param pass plain password
/// \return computed key
inline string _cryptopp_Scrypt(const string &pass) {
  unsigned char digest[33];
  CryptoPP::Scrypt keygen;
  keygen.DeriveKey(digest, 32, (byte *) pass.c_str(), pass.size(), salt, 32);
  return string((char *) digest, 32);
}

/// \class AESEncrypter
/// \brief Class to perform data encrypt
/// \detail This class mow uses AES-256-ECB as encrypt algorithm.
/// \datamember byte key[33]
///             computed key to actually do encrypt
/// \datamember byte iv[CryptoPP::AES::BLOCKSIZE]
///             Initialization Vector.
///             See <https://en.wikipedia.org/wiki/Initialization_vector>
/// \datamember CryptoPP::AES::Encryption aesEncryption
///             abstract aes encrypt instance
/// \datamember CryptoPP::ECB_Mode_ExternalCipher::Encryption ecbEncryption
///             abstract block cipher mode instance_
///             See <https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation#Electronic_Codebook_(ECB)>
/// \todo upgrade to more modern encrypt method like
///       AES-128-GCM or chacha20-poly1305
class AESEncrypter {
 private:
  // default key length 16bytes (128bits)
  byte key[33];
  byte iv[CryptoPP::AES::BLOCKSIZE] =
      {0x45, 0x4e, 0x43, 0x52, 0x59, 0x50, 0x54, 0x44, 0x45, 0x43, 0x52, 0x59,
       0x50, 0x54};
  CryptoPP::AES::Encryption aesEncryption;
  CryptoPP::ECB_Mode_ExternalCipher::Encryption ecbEncryption;

 public:
  /// \brief constructor
  /// \param __key key plain key
  AESEncrypter(const string &_key);

  /// \brief copy constructor
  /// \todo first 16 bytes encrypt/decrypt not working right.
  ///       now add 16bytes dummy data as head to bypass this problem.
  ///       further investigation required
  AESEncrypter(AESEncrypter &a);

  /// \brief @debug show the using key
  void showkey();

  /// \brief @debug show the using iv
  void showiv();

  /// \brief perform encrypt
  /// \param plain unencrypted data
  /// \return encrypted data
  string encrypt(const string &plain);
};

/// \class AESDecrypter
/// \brief Class to perform data decrypt
/// \detail This class mow uses AES-256-ECB as decrypt algorithm.
/// \datamember byte key[33]
///             computed key to actually do encrypt
/// \datamember byte iv[CryptoPP::AES::BLOCKSIZE]
///             Initialization Vector.
/// \datamember CryptoPP::AES::Encryption aesEncryption
///             abstract aes encrypt instance
/// \datamember CryptoPP::ECB_Mode_ExternalCipher::Encryption ecbEncryption
///             abstract block cipher mode instance
/// \todo upgrade to more modern encrypt method like
///       AES-128-GCM or chacha20-poly1305
class AESDecrypter {
 private:
  // default key length 16bytes (128bits)
  byte key[33];
  byte iv[CryptoPP::AES::BLOCKSIZE] =
      {0x45, 0x4e, 0x43, 0x52, 0x59, 0x50, 0x54, 0x44, 0x45, 0x43, 0x52, 0x59,
       0x50, 0x54};
  CryptoPP::AES::Decryption aesDecryption;
  CryptoPP::ECB_Mode_ExternalCipher::Decryption ecbDecryption;

 public:
  /// \brief constructor
  /// \param __key key plain key
  AESDecrypter(const string &_key);

  /// \brief copy constructor
  /// \todo first 16 bytes encrypt/decrypt not working right.
  ///       now add 16bytes dummy data as head to bypass this problem.
  ///       further investigation required
  AESDecrypter(AESDecrypter &a);

  /// \brief @debug show the using key
  void showkey();

  /// \brief @debug show the using iv
  void showiv();

  /// \brief perform decrypt
  /// \param cipher encrypted data
  /// \return decrypted data
  string decrypt(const string &cipher);
};

}
#endif //FILE_TRANSFER_ENCRYPT_H_
