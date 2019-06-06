//
// Created by TYTY on 2019-05-07 007.
//

#include <iostream>
#include <iomanip>

#include "..\third_party\easyloggingpp\src\easylogging++.h"

#include "..\encrypt.h"

INITIALIZE_EASYLOGGINGPP

typedef unsigned char byte;

int main(int argc, char *argv[]) {


/**   Example usage given by library documentation
 *
//    //Key and IV setup
//    //AES encryption uses a secret key of a variable length (128-bit, 196-bit or 256-
//    //bit). This key is secretly exchanged between two parties before communication
//    //begins. DEFAULT_KEYLENGTH= 16 bytes
//    byte key[32];
//    byte iv[CryptoPP::AES::BLOCKSIZE];
//    memset(key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH);
//    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);
//
//    //
//    // String and Sink setup
//    //
//    std::string plaintext = "Now is the time for all good men to come to the aide...";
//    std::string ciphertext;
//    std::string decryptedtext;
//
//    //
//    // Dump Plain Text
//    //
//    std::cout << "Plain Text (" << plaintext.size() << " bytes)" << std::endl;
//    std::cout << plaintext;
//    std::cout << std::endl << std::endl;
//
//    //
//    // Create Cipher Text
//    //
//    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
//    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, iv);
//
//    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(ciphertext));
//    stfEncryptor.Put(reinterpret_cast<const unsigned char *>( plaintext.c_str()), plaintext.length() + 1);
//    stfEncryptor.MessageEnd();
//
//    //
//    // Dump Cipher Text
//    //
//    std::cout << "Cipher Text (" << ciphertext.size() << " bytes)" << std::endl;
//
//    for (int i = 0; i < ciphertext.size(); i++) {
//
//        std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(ciphertext[i])) << " ";
//    }
//
//    std::cout << std::endl << std::endl;
//
//    //
//    // Decrypt
//    //
//    CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
//    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);
//
//    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
//    stfDecryptor.Put(reinterpret_cast<const unsigned char *>( ciphertext.c_str()), ciphertext.size());
//    stfDecryptor.MessageEnd();
//
//    //
//    // Dump Decrypted Text
//    //
//    std::cout << "Decrypted Text: " << std::endl;
//    std::cout << decryptedtext;
//    std::cout << std::endl << std::endl;
 *
 */

  // sample verify code using the encapsuled interface

  encrypt::AESEncrypter enc("testtest");
  encrypt::AESDecrypter dec("testtest");

  std::string a = "lalalablablabla so it is quite boring ";

  enc.showkey();
  enc.showiv();
  dec.showkey();
  dec.showiv();

  std::string b = enc.encrypt(a);
  std::string c = dec.decrypt(b);

  std::cout << c << std::endl;

  return 0;
}