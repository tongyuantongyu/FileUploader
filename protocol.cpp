//
// Created by TYTY on 2019-05-07 007.
//

#include "protocol.h"
#include "./third_party/cryptopp/crc.h"

namespace protocol {

void _debug_value(const string &str) {
  // save and restore the cout status after left the function
  boost::io::ios_all_saver guard(std::cout);

  for (int i = 0; i < str.size(); i++) {
    std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(str[i])) << " ";
  }

  std::cout << std::endl;
}

// we can't get size from char pointer, so additional length info is required
void _debug_value(const char *str, int length) {
  boost::io::ios_all_saver guard(std::cout);

  for (int i = 0; i < length; i++) {
    std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(str[i])) << " ";
  }

  std::cout << std::endl;
}

// debug only
string _crc32(const string &input) {
  using namespace CryptoPP;
  CRC32 crc32;
  string dst;
  StringSource(input, true,
      new HashFilter(crc32, new HexEncoder(new StringSink(dst))));
  return dst;
}

string Randomsession::session(int length) {
  string m;
  m.reserve(length);
  // generate one byte for once
  for (int i = 0; i < length; ++i) {
    m += (char) index_dist(rng);
  }
  return m;
}

/** Data packet struct
 * Client: Server Hello
 * | MAGIC_HEADER 2 | [ Encrypted [ MAGIC_HEADER 2 | VERSION 4] ]
 *
 * Server: Client Hello
 * - Check is our header -> Close Connection
 * - Try decrypt with our key ->
 * | MAGIC_HEADER 2 | 1 1
 * - Check version compability ->
 * | MAGIC_HEADER 2 | 2 1 | [ Encrypted VERSION 4 ]
 * ( If all passed )
 * | MAGIC_HEADER 2 | 0 1 | [ Encrypted SESSION 32 ]
 * Where SESSION is a random 32 bytes data for further file transfer
 *
 * Client: File Negotiation
 * | MAGIC_HEADER 2 | [ Encrypted [ SESSION 32 | PIECE_SIZE 4 | FILE_LENGTH 8 | FILE_PATH 256 ] ]
 * File with too long path can not be upload.
 *
 * Server: File Negotiation
 * Can't open file for write
 * | MAGIC_HEADER 2 | [ Encrypted [ SESSION 32 | 1 1 ]
 * OK. Wait for data
 * | MAGIC_HEADER 2 | [ Encrypted [ SESSION 32 | 0 1 ]
 *
 * Client: Start Transfering file
 *  TCP Mode:
 *      Start forking threads
 *
 *  | MAGIC_HEADER_TRANSFER 2 | [ Encrypted [ SESSION 32 | FILE_PIECE_ORDER 8 | FILE_PIECE PIECE_SIZE ] ]
 *
 */

bool is_ourmsg(const string &msg) {
  try {
    if (msg.substr(2) == MAGIC_HEADER) {
      return true;
    } else {
      return false;
    }
  }
  catch (const std::out_of_range &e) {
    return false;
  }
}

bool is_ourmsg_tansfer(const string &msg) {
  try {
    if (msg.substr(2) == MAGIC_HEADER_TRANSFER) {
      return true;
    } else {
      return false;
    }
  }
  catch (const std::out_of_range &e) {
    return false;
  }
}

string read_msg(std::function<string(int)> &read) {
  try {
    if (read(2) != MAGIC_HEADER) {
      throw NotOurMsg("read_msg - head");
    }
    uint32_t sz = stoul(read(8), 0, 16);
    return read(sz);
  }
  catch (const std::exception &e) {
    LOG(WARNING) << e.what();
    throw NotOurMsg("read_msg - too short msg");
  }
}

string read_msg_transfer(std::function<string(int)> &read) {
  try {
    if (read(2) != MAGIC_HEADER_TRANSFER) {
      throw NotOurMsg("read_msg_transfer - head");
    }
    uint32_t sz = stoul(read(8), 0, 16);
    return read(sz);
  }
  catch (const std::exception &e) {
    LOG(WARNING) << e.what();
    throw NotOurMsg("read_msg_transfer - too short msg");
  }
}

uint32_t read_msg_len(std::function<string(int)> &read) {
  try {
    if (read(2) != MAGIC_HEADER) {
      throw NotOurMsg("read_msg_len - head");
    }
    uint32_t sz = stoul(read(8), 0, 16);
    return sz;
  }
  catch (const std::exception &e) {
    LOG(WARNING) << e.what();
    throw NotOurMsg("read_msg_len - too short msg");
  }
}

uint32_t read_msg_transfer_len(std::function<string(int)> &read) {
  try {
    if (read(2) != MAGIC_HEADER_TRANSFER) {
      throw NotOurMsg("read_msg_transfer_len - head");
    }
    uint32_t sz = stoul(read(8), 0, 16);
    return sz;
  }
  catch (const std::exception &e) {
    LOG(WARNING) << e.what();
    throw NotOurMsg("read_msg_transfer_len - too short msg");
  }
}

string read_msg_guess(std::function<string(int)> &read, int &type) {
  try {
    string _read = read(2);
    if (_read == MAGIC_HEADER) {
      type = 0;
    } else if (_read == MAGIC_HEADER_TRANSFER) {
      type = 1;
    } else {
      type = -1;
      return "";
    }
    uint32_t sz = stoul(read(8), 0, 16);
    auto a = read(sz);
    return a;
  }
  catch (const std::exception &e) {
    LOG(WARNING) << e.what();
    type = -1;
    return "";
  }
}

string build_msg(const string &raw_msg) {
  string msg = MAGIC_HEADER;
  msg += fixedLength(raw_msg.size(), 8);
  msg += raw_msg;
  return msg;
}

string build_msg_transfer(const string &raw_msg) {
  string msg = MAGIC_HEADER_TRANSFER;
  msg += fixedLength(raw_msg.size(), 8);
  msg += raw_msg;
  return msg;
}

/* Data packet struct
 * Client: Server Hello
 * | MAGIC_HEADER 2 | [ Encrypted [ MAGIC_HEADER 2 | VERSION 4] ]
 */

string server_hello_build(AESEncrypter &enc) {
  LOG(DEBUG) << "server_hello_build";
  string enc_str = MAGIC_HEADER;
  enc_str += VERSION;
  return enc.encrypt(enc_str);
}

int server_hello_verify(AESDecrypter &dec, const string &msg) {
  LOG(DEBUG) << "server_hello_verify";
  try {
    string dec_str = dec.decrypt(msg);
    string b = dec_str.substr(0, 2);
    if (dec_str.substr(0, 2) != MAGIC_HEADER) {
      LOG(DEBUG) << "Received unknown hello msg.";
      return 1;
    } else if (dec_str.substr(2, 4) != VERSION) {
      LOG(DEBUG) << "Client version unequal.";
      return 2;
    } else {
      LOG(DEBUG) << "Received hello msg.";
      return 0;
    }
  }
  catch (const std::out_of_range &e) {
    LOG(DEBUG) << "Received unknown hello msg.";
    return 1;
  }
}

/* Server: Client Hello
 * - Check is our header -> Close Connection
 * - Try decrypt with our key ->
 * | MAGIC_HEADER 2 | 1 1
 * - Check version compability ->
 * | MAGIC_HEADER 2 | 2 1 | [ Encrypted VERSION 4 ]
 * ( If all passed )
 * | MAGIC_HEADER 2 | 0 1 | [ Encrypted SESSION 32 ]
 * Where SESSION is a random 32 bytes data for further file transfer
 */

string client_hello_build(AESEncrypter &enc,
                          const int &status,
                          const string &session) {
  LOG(DEBUG) << "client_hello_build";
  if (status == 0) {
    string msg;
    msg += (char) 0x00;
    msg += enc.encrypt(session);
    return msg;
  } else if (status == 1) {
    string msg;
    msg += (char) 0x01;
    return msg;
  } else if (status == 2) {
    string msg;
    msg += enc.encrypt(VERSION);
    return msg;
  } else {
    throw std::invalid_argument("Wrong status.");
  }
}

int client_hello_verify(AESDecrypter &dec, const string &msg, string &session) {
  LOG(DEBUG) << "client_hello_verify";
  try {
    if (msg[0] == 0x00) {
      LOG(DEBUG) << "Got server reply.";
      string dec_str = dec.decrypt(msg.substr(1, msg.size() - 1));
      session = dec_str.substr(0, dec_str.size() - 1);
      return 0;
    } else if (msg[0] == 0x01) {
      LOG(DEBUG) << "Server using another key.";
      return 1;
    } else if (msg[0] == 0x02) {
      LOG(DEBUG) << "Server is another version.";
      return 2;
    } else {
      throw std::invalid_argument("Wrong status.");
    }
  }
  catch (const std::out_of_range &e) {
    LOG(DEBUG) << "Received unknown hello msg.";
    throw std::runtime_error("Client sent bad file hello info.");
  }
}

/* Client: File Negotiation
 * | MAGIC_HEADER 2 | [ Encrypted [ SESSION 32 | PIECE_SIZE 8 | FILE_LENGTH 16 | FILE_PATH VARY ] ]
 * File with too long path can not be upload.
 */

string
file_negotiation_build(AESEncrypter &enc,
                       const string &session,
                       const uint32_t &piece_size,
                       const uint64_t &file_length,
                       const string &file_path) {
  LOG(DEBUG) << "file_negotiation_build";
  string enc_str = session;
  enc_str += fixedLength(piece_size, 8);
  enc_str += fixedLength(file_length, 16);
  enc_str += file_path;
  return enc.encrypt(enc_str);
}

int file_negotiation_verify(AESDecrypter &dec,
                            const string &msg,
                            const string &session,
                            uint32_t &piece_size,
                            uint64_t &file_length,
                            string &file_path) {
  LOG(DEBUG) << "file_negotiation_verify";
  string dec_str = dec.decrypt(msg);
  try {
    if (dec_str.substr(0, 32) != session) {
      return 1;
    }
    piece_size = stoul(dec_str.substr(32, 8), 0, 16);
    file_length = stoull(dec_str.substr(40, 16), 0, 16);
    file_path = dec_str.substr(56, dec_str.size() - 56);
    return 0;
  }
  catch (const std::out_of_range &e) {
    throw std::runtime_error("Client sent bad file negotiation info.");
  }
}

/* Server: File Negotiation
 * Can't open file for write
 * | MAGIC_HEADER 2 | [ Encrypted [ SESSION 32 | 1 1 ]
 * OK. Wait for data
 * | MAGIC_HEADER 2 | [ Encrypted [ SESSION 32 | 0 1 ]
 */

string file_negotiation_reply(AESEncrypter &enc,
                              const string &session,
                              const int &status) {
  LOG(DEBUG) << "file_negotiation_reply";
  string enc_str = session;
  enc_str += (char) status;
  return enc.encrypt(enc_str);
}

int file_negotiation_finish(AESDecrypter &dec,
                            const string &msg,
                            const string &session) {
  LOG(DEBUG) << "file_negotiation_finish";
  try {
    string dec_str = dec.decrypt(msg);
    if (dec_str.substr(0, 32) != session) {
      throw std::runtime_error(
          "file_negotiation_finish - Server session conflict.");
    } else {
      return (int) *dec_str.substr(32, 1).c_str();
    }
  }
  catch (const std::out_of_range &e) {
    throw std::runtime_error(
        "file_negotiation_finish - Client sent bad file negotiation info.");
  }
}

/* Client: Start Transfering file
 *  | MAGIC_HEADER_TRANSFER 2 | [ Encrypted [ SESSION 32 | FILE_PIECE_ORDER 32 | FILE_PIECE PIECE_SIZE ] ]
 */

string file_transfer_init(AESEncrypter &enc, const string &session) {
  LOG(DEBUG) << "file_transfer_init";
//  string enc_str(16, char(0xFE));
//  enc_str += session;
  return enc.encrypt(session);
}

string file_transfer_init_read(AESDecrypter &dec, const string &msg) {
  LOG(DEBUG) << "file_transfer_init_read";
  try {
    auto dec_str = dec.decrypt(msg);
    return dec_str.substr(0, 32);
  }
  catch (const std::exception &e) {
    return "";
  }
}

string file_transfer_init_reply(AESEncrypter &enc, const int &status) {
  LOG(DEBUG) << "file_transfer_init_reply";
  string enc_str(16, char(0xFF));
  enc_str += char(status);
  return enc.encrypt(enc_str);
}

int file_transfer_init_confirm(AESDecrypter &dec, const string &msg) {
  LOG(DEBUG) << "file_transfer_init_confirm";
  return int(dec.decrypt(msg)[16]);
}

string file_transfer_build(AESEncrypter &enc,
                           const string &session,
                           const uint32_t &order,
                           const uint32_t &size,
                           const
                           string &piece) {
//  LOG(DEBUG) << "file_transfer_build";
  string enc_str;
  string _t;
  std::ostringstream a(_t);
  a << "order-" << order << " size-" << size
     << " CRC32-" << _crc32(piece);
  LOG(TRACE) << a.str();
  enc_str += session;
  enc_str += fixedLength(order, 8);
  enc_str += fixedLength(size, 8);
  enc_str += piece;
  return enc.encrypt(enc_str);
}

int file_transfer_read(AESDecrypter &dec,
                       const string &msg,
                       const string &session,
                       uint32_t &order,
                       uint32_t &size,
                       string &piece) {
//  LOG(DEBUG) << "file_transfer_read";
  try {
    string dec_str = dec.decrypt(msg);
    if (dec_str.substr(0, 32) != session) {
      throw std::runtime_error("file_transfer_read - Server session conflict.");
    }
    order = stoul(dec_str.substr(32, 8), 0, 16);
    size = stoul(dec_str.substr(40, 8), 0, 16);
    piece = dec_str.substr(48, dec_str.size() - 48);
    string _t;
    std::ostringstream a(_t);
    a << "order-" << order << " size-" << size
      << " CRC32-" << _crc32(piece);
    LOG(TRACE) << a.str();
    return 0;
  }
  catch (const std::out_of_range &e) {
    throw std::runtime_error(
        "file_transfer_read - Client sent bad file negotiation info.");
  }
}

string file_transfer_receive(AESEncrypter &enc,
                             const string &session,
                             const int &status) {
  LOG(DEBUG) << "file_transfer_receive";
  string enc_str(16, char(0xFF));
  enc_str += session;
  enc_str += (char) status;
  return enc.encrypt(enc_str);
}

int file_transfer_confirm(AESDecrypter &dec,
                          const string &msg,
                          const string &session) {
  LOG(DEBUG) << "file_transfer_confirm";
  try {
    string dec_str = dec.decrypt(msg);
    if (dec_str.substr(16, 32) != session) {
      throw std::runtime_error("Server session conflict.");
    } else {
      return (int) dec_str[48];
    }
  }
  catch (const std::out_of_range &e) {
    throw std::runtime_error("Client sent bad file transfered info.");
  }
}

}