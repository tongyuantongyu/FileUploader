//
// Created by TYTY on 2019-06-03 003.
//

#include "encrypt.h"

using namespace encrypt;

// Encrypter implementation

AESEncrypter::AESEncrypter(const string &_key) {
  if (_key.length() < 7) {
    LOG(WARNING) << "You are using a short key which is weak to attack.";
  }

  // scrypt algorithm
  CryptoPP::Scrypt keygen;
  keygen.DeriveKey(key, 32, (byte *) _key.c_str(), _key.size(), salt, 32);

  // constrcut aes related object
  CryptoPP::AES::Encryption _aes(key, 32);
  aesEncryption = _aes;
  CryptoPP::ECB_Mode_ExternalCipher::Encryption _ecb(aesEncryption, iv);
  ecbEncryption = _ecb;
}

AESEncrypter::AESEncrypter(AESEncrypter &a) {
  // copy the key
  memcpy(key, a.key, 33);

  // make another aes related object
  CryptoPP::AES::Encryption _aes(key, 32);
  aesEncryption = _aes;
  CryptoPP::ECB_Mode_ExternalCipher::Encryption _ecb(aesEncryption, iv);
  ecbEncryption = _ecb;
}

void AESEncrypter::showkey() {

  // dump as hex
  CryptoPP::HexEncoder encoder;
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(key, sizeof(key));
  encoder.MessageEnd();

  LOG(DEBUG) << "Now using key: " << output;
}

void AESEncrypter::showiv() {

  // dump as hex
  CryptoPP::HexEncoder encoder;
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(iv, sizeof(iv));
  encoder.MessageEnd();

  LOG(DEBUG) << "Now using iv: " << output;
}

string AESEncrypter::encrypt(const string &plain) {

  string encrypted;

  CryptoPP::StreamTransformationFilter
      stfEncryptor(ecbEncryption, new CryptoPP::StringSink(encrypted));
  // not that in encryption we need to give plain.length() + 1
  // while in decryption we not
  stfEncryptor.Put(reinterpret_cast<const unsigned char *>( plain.c_str()),
                   plain.length() + 1);
  stfEncryptor.MessageEnd();

  return encrypted;
}

// Decrypter implementation

// Implementation is very likely to Encrypter
AESDecrypter::AESDecrypter(const string &_key) {

  // scrypt
  CryptoPP::Scrypt keygen;
  keygen.DeriveKey(key, 32, (byte *) _key.c_str(), _key.size(), salt, 32);

  CryptoPP::AES::Decryption _aes(key, 32);
  aesDecryption = _aes;
  CryptoPP::ECB_Mode_ExternalCipher::Decryption _ecb(aesDecryption, iv);
  ecbDecryption = _ecb;
}

AESDecrypter::AESDecrypter(AESDecrypter &a) {
  memcpy(key, a.key, 33);

  CryptoPP::AES::Decryption _aes(key, 32);
  aesDecryption = _aes;
  CryptoPP::ECB_Mode_ExternalCipher::Decryption _ecb(aesDecryption, iv);
  ecbDecryption = _ecb;
}

void AESDecrypter::showkey() {

  // dump as hex
  CryptoPP::HexEncoder encoder;
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(key, sizeof(key));
  encoder.MessageEnd();

  LOG(DEBUG) << "Now using key: " << output;
}

void AESDecrypter::showiv() {

  // dump as hex
  CryptoPP::HexEncoder encoder;
  std::string output;
  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(iv, sizeof(iv));
  encoder.MessageEnd();

  LOG(DEBUG) << "Now using iv: " << output;
}

string AESDecrypter::decrypt(const string &cipher) {

  string decrypted;

  CryptoPP::StreamTransformationFilter
      stfDecryptor(ecbDecryption, new CryptoPP::StringSink(decrypted));
  stfDecryptor.Put(reinterpret_cast<const unsigned char *>( cipher.c_str()),
                   cipher.length());
  stfDecryptor.MessageEnd();

  return decrypted;

}