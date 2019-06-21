//
// Created by TYTY on 2019-05-19 019.
//

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <chrono>

#include "./third_party/cxxopts/include/cxxopts.hpp"

#include "file.h"
#include "protocol.h"

INITIALIZE_EASYLOGGINGPP

/// \file client.cpp
/// \brief Client Implementation
/// \note
/** Workflow
 * Client: Server Hello
 * Server: Client Hello
 * Client: File Negotiation
 * Server: File Negotiation
 * Client: Start Transfering file
 * Cilent: Finish Transfering file
 * Server: Close file and clean
 */

using boost::asio::ip::tcp;
using boost::asio::buffer;

/// \class Uploader
/// \brief Class to perform upload work
/// \detail after created Uploader will handle all negotiation work
///         and start required threads to upload data to the server
/// \datamember std::string ip
///             ip of the server
/// \datamember int port
///             port of the server
/// \datamember tcp::socket socket_
///             tcp socket of the server
/// \datamember protocol::AESEncrypter enc
///             encrypter object
/// \datamember protocol::AESDecrypter dec
///             decrypter object
/// \datamember std::string session
///             session id of this connection
/// \datamember int piece_size
///             transfer file piece size
/// \datamember std::string file_name
///             file name (and path)
/// \datamember file::file_reader f
///             file reader object
/// \datamember int ths
///             threads count
class Uploader : public std::enable_shared_from_this<Uploader> {
 private:
  std::string ip;
  int port;
  tcp::socket socket_;
  protocol::AESEncrypter enc;
  protocol::AESDecrypter dec;
  std::string session;
  int piece_size;
  std::string file_name;
  file::file_reader f;
  int ths;
 public:
  // transfer thread function.
  // for access convenience, make it friend function
  friend void transfer(Uploader *ul);
  /// \brief constructor
  /// \note this constructor simply initialize its members. for detailed
  ///       infomation, see class's datamenber explanation.
  explicit Uploader(
      std::string &_ip,
      int &_port,
      tcp::socket &sock,
      protocol::AESEncrypter &_enc,
      protocol::AESDecrypter &_dec,
      std::string &_file_name,
      int &_piece_size,
      int &thread_number) :
      ip(_ip),
      port(_port),
      // socket is the established connection between the client and server
      // therefore can not be copied. use std::move to move the socket
      socket_(std::move(sock)),
      // copy the encrypter and decrypter
      enc(_enc),
      dec(_dec),
      file_name(_file_name),
      piece_size(_piece_size),
      f(file_name.c_str(), piece_size),
      ths(thread_number) {
    // force send small tcp packet to make protocol negotiation
    // works properly
    boost::asio::ip::tcp::no_delay option(true);
    socket_.set_option(option);
  }
  /// \brief encapsulate boost::asio read function
  /// \detail encapsulate the boost::asio's read function and reduce parameter
  ///         count to make the function signature the same as read_msg series
  ///         functions' parameter's function
  /// \param length how many bytes will the function read
  /// \return received data
  std::string _read(int length) {
    std::string _tmp;
    // resize string to actually make let boost::asio::buffer get the length
    // info.
    // reserve didn't change string length so buffer will see it as 0 length
    // buffer and can  not receive anything
    _tmp.resize(length);
    read(socket_, buffer(_tmp), boost::asio::transfer_exactly(length));
    return _tmp;
  };
  /// \brief handshake period logic.
  void handshake() {
    //Client: Server hello
    socket_.write_some(buffer(protocol::build_msg(protocol::server_hello_build(
        enc))));

    //Client: Verify client hello

    //wrap the scope for function to call for data
    try {
      // use std::bind to convert member functino to functor object to pass it
      // to external function to call
      std::function<std::string(int)>
          _t = std::bind(&Uploader::_read, this, std::placeholders::_1);
      int status =
          protocol::client_hello_verify(dec, protocol::read_msg(_t), session);
      if (status != 0) {
        exit(1);
      }
    }
    catch (const std::exception &e) {
      e.what();
      exit(1);
    }
  }
  /// \brief negotiation period logic.
  void file_negotiation() {
    //Client: Send negotiate message
    socket_.write_some(buffer(protocol::build_msg(
        protocol::file_negotiation_build(enc,
                                         session,
                                         piece_size,
                                         f.get_size(),
                                         file_name)
    )));

    //Client: Check negotiate response
    try {
      std::function<std::string(int)>
          _t = std::bind(&Uploader::_read, this, std::placeholders::_1);
      int status = protocol::file_negotiation_finish(dec,
                                                     protocol::read_msg(_t),
                                                     session);
      if (status != 0) {
        exit(1);
      }
    }
    catch (const std::exception &e) {
      e.what();
      exit(1);
    }
  }
  /// \brief transfer period logic.
  /// \note Versus server, even using asio to do asynchorous programming,
  ///         we decided to make client send data to server synchorous,
  ///         because it's difficult to control concurrent number in
  ///         asynchorous code.
#ifndef _MSC_VER
  // This code can not compile on MSVC but compile and works fine on G++
  void file_transfer() {
    // thread pointers array
    std::thread *threads[ths];
    // set all to nullptr
    for (int i = 0; i < ths; ++i) {
      threads[i] = nullptr;
    }
    for (int i = 0; i < ths; ++i) {
      // start new threads
      // use lambda function to encapsulate transfer task
      threads[i] = new std::thread([this]() { transfer(this); });
    }
    // join the threads (after task finished, thread can be joined)
    for (auto t : threads) {
      if (t && t->joinable()) {
        t->join();
        // then the pointer is useless
        delete t;
      }
    }
    // here threads is a stack object so can be deleted automaticly
  }
#else
  void file_transfer() {
    // MSVC see ths as variable so we must make it heap object
//    auto threads = new std::thread * [ths];
    // update: use smart pointer to hold heap object to make sure it
    // can be deleted
    std::unique_ptr<std::thread *[]> threads(new std::thread * [ths]);
    for (int i = 0; i < ths; ++i) {
      threads[i] = new std::thread([this]() {transfer(this); });
    }
    for (int i = 0; i < ths; ++i) {
      if (threads[i]->joinable()) {
        threads[i]->join();
        delete threads[i];
      }
    }
//    // and delete it
//    delete[] threads;
  // since we used smart pointer, the object will automaticly be deleted
  // after left the scope due to RAII even when exception throwed
  }
#endif
};

/// \brief file transfer worker.
/// \param ul pointer to negotiated Uploader object
void transfer(Uploader *ul) {

  // transfer threads use different io_context
  boost::asio::io_context io_context;
  tcp::socket sock(io_context);

  // Encrypter / Decrypter object is not thread safe so we need to
  // copy it for each thread
  protocol::AESEncrypter enc(ul->enc);
  protocol::AESDecrypter dec(ul->dec);

  // connect to the server and configure socket
  boost::asio::ip::tcp::endpoint
      ep(boost::asio::ip::address::from_string(ul->ip), ul->port);

  sock.connect(ep);

  boost::system::error_code error;

  std::string _sess = ul->session;

  // file transfer init

  boost::asio::write(sock,
                     buffer(protocol::build_msg_transfer(
                         protocol::file_transfer_init(
                         enc,
                         ul->session))),
                     error);

//  std::function<std::string(int)>
//      _t = std::bind(&Uploader::_read, ul, std::placeholders::_1);
//
//  if (protocol::file_transfer_init_confirm(
//      dec, protocol::read_msg_transfer(_t)) != 0) {
//    LOG(WARNING) << "Server encountered error.";
//  }

  int size;

  // we have to use heap memory and char pointer now.
  // TODO: Update file module to support smart pointer
  char *_read_buf = new char[ul->piece_size];

  // make memory release even if error occured.
  try {
    do {
      std::uintmax_t _offset;

      size = ul->f.read(_read_buf, _offset);

      // give length info to string constructor
      // to prevent construct stop at first 0x00 byte
      std::string _read_str(_read_buf, ul->piece_size);

      boost::asio::write(sock,
                         buffer(protocol::build_msg_transfer(
                             protocol::file_transfer_build(
                                 enc,
                                 _sess,
                                 _offset / ul->piece_size,
                                 size,
                                 _read_str))),
                         error);

//    try {
//      std::function<std::string(int)>
//          _t = std::bind(&Uploader::_read, ul, std::placeholders::_1);
//      int status = protocol::file_transfer_confirm(
//          dec, protocol::read_msg_transfer(_t), ul->session);
//      if (status != 0) {
//        exit(1);
//      }
//    }
//    catch (const std::exception &e) {
//      e.what();
//      exit(1);
//    }
      // reset memory in case error happened
      memset(_read_buf, 0, ul->piece_size);
    } while (size != 0);

    // send finish packet
    boost::asio::write(sock, buffer(protocol::build_msg_transfer(
        protocol::file_transfer_build(enc, _sess, 0, 0, " "))), error);
  }
  catch (std::exception &e) {
    LOG(ERROR) << e.what();
    // remember to delete _read_buf
    delete[] _read_buf;
    return;
  }
  delete[] _read_buf;
}

int main(int argc, char *argv[]) {

  // time start
  std::chrono::steady_clock::time_point  now = std::chrono::steady_clock::now();

  // arguments reader
  cxxopts::Options options("FileUploader-Client", "Uploader client.");

  options.add_options()
      ("h,host", "IP Address to connect", cxxopts::value<std::string>())
      ("p,port", "Port number to connect", cxxopts::value<int>())
      ("k,key", "Encrypt key to communicate", cxxopts::value<std::string>())
      ("f,file", "File name", cxxopts::value<std::string>())
      ("t,thread", "Threads to connect", cxxopts::value<int>())
      ("s,size", "Piece size(byte) to split file in", cxxopts::value<int>());

  std::string host;
  int port;
  std::string key;
  std::string file_name;
  int thread_num;
  int piece_size;

  auto result = options.parse(argc, argv);

  // different logic to required and optional parameters
  try {
    host = result["h"].as<std::string>();
    port = result["p"].as<int>();
    key = result["k"].as<std::string>();
    file_name = result["f"].as<std::string>();
  }
  catch (const cxxopts::OptionException &e) {
    std::cerr << "Incorrect parameters" << std::endl;
    exit(1);
  }
  catch (const std::domain_error &e) {
    std::cerr << "One or more required parameters not given" << std::endl;
    exit(1);
  }

  try {
    thread_num = result["t"].as<int>();
  }
  catch (const std::domain_error &e) {
    thread_num = 1;
  }

  try {
    piece_size = result["s"].as<int>();
  }
  catch (const std::domain_error &e) {
    piece_size = 65536;
  }

  el::Configurations defaultConf;
  defaultConf.setToDefault();
  defaultConf.setGlobally(
      el::ConfigurationType::Format, "[%datetime - %level]: %msg");
  el::Loggers::reconfigureLogger("default", defaultConf);

  // On windows, to use BSD sockets these steps are required.
  #ifdef WIN32
  protocol::init_environment();
  #endif

  protocol::AESEncrypter enc(key);
  protocol::AESDecrypter dec(key);

  // main thread io_context
  boost::asio::io_context io_context;
  tcp::socket sock(io_context);

  boost::asio::ip::tcp::endpoint
      ep(boost::asio::ip::address::from_string(host), port);

  sock.connect(ep);

  Uploader ul(host, port, sock, enc, dec, file_name, piece_size, thread_num);

  ul.handshake();

  ul.file_negotiation();

  ul.file_transfer();

  io_context.run();

  #ifdef WIN32
  protocol::clear_environment();
  #endif

  // print time info
  auto t2 = std::chrono::steady_clock::now();
  std::chrono::duration<double> time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(t2 - now);

  LOG(INFO) << "Upload finished using "<< time_span.count() << "seconds.";

  return 0;
}
