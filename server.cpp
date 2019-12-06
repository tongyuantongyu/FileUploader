//
// Created by TYTY on 2019-05-19 019.
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <string>
#include <functional>

#include "./third_party/cxxopts/include/cxxopts.hpp"

#include "file.h"
#include "protocol.h"

INITIALIZE_EASYLOGGINGPP

/// \file server.cpp
/// \brief Server Implementation
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

protocol::Randomsession sess_gen;

class Session;


/// \class Thread
/// \brief Class to handle a transfer thread
/// \detail this class will interact with a incoming thread and receive data
///         from it. It should be instanced only by Session
/// \datamember int number
///             thread number
/// \datamember tcp::socket socket_
///             transfer thread socket
/// \datamember protocol::AESEncrypter enc
///             encrypter object
/// \datamember protocol::AESDecrypter dec
///             decrypter object
/// \datamember std::shared_ptr<file::file_writer> _f
///             a shared pointer of file writer object
/// \datamember std::weak_ptr<Session> _sess
///             a weak pointer to the Session launched this Thread
///             use to inform finish
/// \datamember std::string _tmp
///             temp for storing data received in asynchorous operation
/// \datamember std::string session;
///             session id of this connection
/// \datamember uint32_t piece_size;
///             transfer file piece size
class Thread : public std::enable_shared_from_this<Thread> {
 private:
  int number;
  tcp::socket socket_;
  protocol::AESEncrypter enc;
  protocol::AESDecrypter dec;
  std::shared_ptr<file::file_writer> _f;
  std::weak_ptr<Session> _sess;
  std::string _tmp;
  std::string session;
  uint32_t piece_size;
 public:
  /// \brief emulator to the boost::asio read function
  /// \detail due to the limitation of asynchorous function, we can't simply
  ///         call the function and get the received data. Rather, we should
  ///         tell it where the data should be write in when data received
  ///         (a boost::asio::buffer object) and what should it do when all
  ///         desired data has received (a callable functor). Therefore, we
  ///         ask it to receive data to the _tmp temp storage and read data
  ///         from the storage to provide same logic to protocol layer logic.
  /// \param length how many bytes will the function read
  /// \return received data
  std::string _read(int length) {
    // read that length
    std::string _data = _tmp.substr(0, length);
    // remove it from temp
    _tmp.erase(0, length);
    // return readed data
    return _data;
  }
  /// \brief read the head info of the message
  /// \detail all packet has a fixed length head with header to mark our data
  ///         packet and a number to mark how long the packet is. We first read
  ///         the header, get the body length, and read the body then.
  void _read_head() {
    // reset temp
    _tmp.clear();
    _tmp.resize(10);

    auto self(shared_from_this());
    async_read(socket_, buffer(_tmp), boost::asio::transfer_exactly(10),
               [this, self](boost::system::error_code ec, std::size_t) {
      // this function is called after the task finished (here after head read)
                 if (!ec) {
                   _read_body();
                 }
               });
  }
  /// \brief read the body of the message
  void _read_body() {
    // get length info from received head data
    std::function<std::string(int)> _t =
        std::bind(&Thread::_read, shared_from_this(), std::placeholders::_1);
    uint32_t length = protocol::read_msg_transfer_len(_t);
    _tmp.clear();
    _tmp.resize(length);

    auto self(shared_from_this());
    async_read(socket_, buffer(_tmp), boost::asio::transfer_exactly(length),
               [this, self](boost::system::error_code ec, std::size_t) {
                 if (!ec) {
                   _write_file();
                 }
               });
  }
  /// \brief write the received data into file
  /// \note to add finishing logic the code need to reference to Session
  ///         class, so its definition is placed out the class below.
  void _write_file();

  /// \brief constructor
  /// \note this constructor simply initialize its members. for detailed
  ///       infomation, see class's datamenber explanation.
  Thread(
      tcp::socket _socket,
      protocol::AESEncrypter _enc,
      protocol::AESDecrypter _dec,
      std::shared_ptr<file::file_writer> f,
      std::string &_session,
      uint32_t &_piece_size,
      const int &_number,
      std::weak_ptr<Session> _s
  ) :
      socket_(std::move(_socket)),
      enc(_enc),
      dec(_dec),
      session(_session),
      piece_size(_piece_size),
      _f(std::move(f)),
      number(_number),
      _sess(std::move(_s)) {

//    async_write(socket_, buffer(protocol::build_msg(
//        protocol::file_transfer_init_reply(enc, 0))),
//                [this](boost::system::error_code ec, std::size_t) {
//                  if (!ec) {
//                    _read_head();
//                  }
//                });
//
//    _read_head();

  }
};


/// \class Session
/// \brief Class to handle a client
/// \detail this class will interact with a incoming client and
///         negotiate with it. after that, transfer will be handled by
///         Thread instance.
/// \datamember enum s_code { NOTSET, NEGOTIATED, FINISHED }
///             indicate Session status
///             NOT_SET: this Session hasn't finish negotiation with client.
///             NEGOTIATED: this Session is ready to receive transfer thread.
///             FINISHED: this Session has finished its task and can be deleted.
/// \datamember s_code status
///             store status
/// \datamember std::string session;
///             session id of this connection
/// \datamember tcp::socket socket_
///             transfer thread socket
/// \datamember protocol::AESEncrypter enc
///             encrypter object
/// \datamember protocol::AESDecrypter dec
///             decrypter object
/// \datamember std::shared_ptr<file::file_writer> _f
///             a shared pointer of file writer object
/// \datamember std::unordered_map<int, std::shared_ptr<Thread>> children
///             store and control the Threads
/// \datamember std::string _tmp
///             temp for storing data received in asynchorous operation
/// \datamember uint32_t piece_size;
///             transfer file piece size
/// \datamember std::atomic_int number = 0;
///             counter to compute thread id
///             use atomic object to ensure no duplicate count
/// \datamember int result
///             store the result of verify function
class Session : public std::enable_shared_from_this<Session> {
 public:
  enum s_code { NOTSET, NEGOTIATED, FINISHED };
  s_code status;
  std::string session;
 private:
  tcp::socket socket_;
  protocol::AESEncrypter enc;
  protocol::AESDecrypter dec;
  std::shared_ptr<file::file_writer> _f;
  std::unordered_map<int, std::shared_ptr<Thread>> children;
  std::string _tmp;
  uint32_t piece_size;
  std::atomic_int number = 0;
  int result;
 public:
  /// \brief emulator to the boost::asio read function
  /// \note for detailed info, see Thread::_read
  std::string _read(int length) {
    std::string _data = _tmp.substr(0, length);
    _tmp.erase(0, length);
    return _data;
  }
  /// \brief constructor
  /// \note this constructor simply initialize its members. for detailed
  ///       infomation, see class's datamenber explanation.
  Session(
      tcp::socket _socket,
      protocol::AESEncrypter _enc,
      protocol::AESDecrypter _dec,
      std::string &msg
  ) :
      socket_(std::move(_socket)),
      enc(_enc),
      dec(_dec),
      _tmp(msg) {
    status = NOTSET;
    session = sess_gen.session(32);

    // force send small tcp packet to make protocol negotiation
    // works properly
    boost::asio::ip::tcp::no_delay option(true);
    socket_.set_option(option);
  }
  /// \brief these four step split the whole handshake and negotiation
  ///         process. for works of each function, see the comment above
  ///         declaration and in the function.

  /// read data: Server Hello
  /// send data: Cilent Hello
  void step1() {
    /// read Server Hello part
    result = protocol::server_hello_verify(dec, _tmp);
    if (result != 0) {
      status = FINISHED;
      // call Acceptor to delete self
      return;
    }

    /// send Cilent Hello part
    async_write(socket_, buffer(protocol::build_msg(
        protocol::client_hello_build(enc, result, session))),
                [this](boost::system::error_code ec, std::size_t) {
                  if (!ec) {
                    step2();
                  }
                });
  }

  /// receive data: File Negotiation head
  void step2() {
    _tmp.clear();
    _tmp.resize(10);

    auto self(shared_from_this());
    async_read(socket_, buffer(_tmp), boost::asio::transfer_exactly(10),
               [this, self](boost::system::error_code ec, std::size_t) {
                 if (!ec) {
                   step3();
                 }
               });
  }

  /// read data: File Negotiation head
  /// receive data: File Negotiation body
  void step3() {

    std::function<std::string(int)> _t =
        std::bind(&Session::_read, shared_from_this(), std::placeholders::_1);
    uint32_t length = protocol::read_msg_len(_t);

    _tmp.clear();
    _tmp.resize(length);

    auto self(shared_from_this());
    async_read(socket_, buffer(_tmp), boost::asio::transfer_exactly(length),
               [this, self](boost::system::error_code ec, std::size_t) {
                 if (!ec) {
                   step4();
                 }
               });
  }

  /// read data: File Negotiation body
  /// send data: File Negotiation result
  void step4() {
    /// read File Negotiation body part
    uint64_t file_s;
    std::string path;
    protocol::file_negotiation_verify(dec,
                                      _tmp,
                                      session,
                                      piece_size,
                                      file_s,
                                      path);
    // move shared_ptr to class member to control life cycle
    try {
//      std::shared_ptr<file::file_writer>
//          _tf(new file::file_writer(path.c_str(), file_s));
      _f = std::make_shared<file::file_writer>(path, file_s);
      result = (int) !(_f->ok);
    }
    catch (file::NoEnoughSpace &e) {
      result = 1;
    }

    /// send File Negotiation result part
    async_write(socket_, buffer(protocol::build_msg(
        protocol::file_negotiation_reply(enc, session, result))),
                [this](boost::system::error_code ec, std::size_t) {
                  if (!ec) {
                    status = NEGOTIATED;
                  }
                });
    if (result != 0) {
      status = FINISHED;
    }
  }

  /// \brief Thread creator
  /// \detail after Acceptor identified the incoming connection is a Thread
  ///         belongs to this Session, Acceptor will call this function to
  ///         let Session create a new Thread to handle the client
  void attach_thread(tcp::socket _socket) {
    std::shared_ptr<Thread> _t(
        new Thread(std::move(_socket),
                   enc,
                   dec,
                   _f,
                   session,
                   piece_size,
                   number,
                   shared_from_this()));
    _t->_read_head();
    // store the children
    children[number] = std::move(_t);
    // increase count
    ++number;
  }
  /// \brief thread finish notify
  /// \detail after each thread finished, this function will be called once to
  ///         infrom the Session. when count go back to zero, file will be
  ///         closed and Session will be marked FINISHED.
  void finish_thread(int _number) {
    --number;
    if (number == 0) {
      status = FINISHED;
      _f->close();
    }
  }
};

/// \brief write the received data into file
void Thread::_write_file() {
  uint32_t order;
  uint32_t size;
  std::string piece;
  protocol::file_transfer_read(dec, _tmp, session, order, size, piece);
  if (order == 0 && size == 0) {
    // transfer finished. inform Session.
    std::shared_ptr<Session> _s(_sess);
    _s->finish_thread(number);
    return;
  }
  _f->write(piece, size, ((std::uintmax_t) order) * piece_size);
  _read_head();
}


/// \class Acceptor
/// \brief Class to accept incoming connection
/// \detail this class will accept new incoming connection and send to
///         related object to handle
/// \datamember protocol::AESEncrypter enc
///             encrypter object
/// \datamember protocol::AESDecrypter dec
///             decrypter object
/// \datamember tcp::acceptor acceptor_
///             accept connection to the specific port.
/// \datamember std::unordered_map<std::string, std::shared_ptr<Session>> children
///             store session - Session pointer pair to get
/// \datamember std::vector<std::string> s_list
///             session list to walk through to clean FINISHED Sessions.
class Acceptor : public std::enable_shared_from_this<Acceptor> {
 private:
  protocol::AESEncrypter enc;
  protocol::AESDecrypter dec;
  tcp::acceptor acceptor_;
  std::unordered_map<std::string, std::shared_ptr<Session>> children;
  std::vector<std::string> s_list;
 public:
  /// \brief constructor
  /// \note this constructor simply initialize its members. for detailed
  ///       infomation, see class's datamenber explanation.
  explicit Acceptor(
      boost::asio::io_context &io_context,
      int port,
      protocol::AESEncrypter &_enc,
      protocol::AESDecrypter &_dec) :
      // listen to 0.0.0.0 (Any address) with specific port
      acceptor_(io_context, tcp::endpoint(tcp::v6(), port)),
      enc(_enc),
      dec(_dec) {}
  /// \brief @static encapsulate boost::asio read function
  /// \detail encapsulate the boost::asio's read function and reduce parameter
  ///         count to make the function signature the same as read_msg series
  ///         functions' parameter's function
  /// \param socket_ the socket to read from
  /// \param length how many bytes will the function read
  /// \return received data
  static std::string _read(tcp::socket *socket_, int length) {
    std::string _tmp;
    _tmp.resize(length);
    read(*socket_, buffer(_tmp), boost::asio::transfer_exactly(length));
    return _tmp;
  };
  /// \brief this function recursive infinitely to keep accepting connection.
  void do_accept() {
    // first clean up finished Sessions
    std::vector<std::string> _tmp;
    for (auto const &_s : s_list) {
      if (children[_s]->status == Session::FINISHED) {
        children.erase(_s);
      } else {
        _tmp.push_back(_s);
      }
    }
    s_list = _tmp;

    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket) {
          // this lambda function will be called
          // when a new connection established
          if (!ec) {
            int type_;
            // bind _read to satisfy function signature request
            std::function<std::string(int)> _t =
                std::bind(&Acceptor::_read, &socket, std::placeholders::_1);
            std::string _msg = protocol::read_msg_guess(_t, type_);
            if (type_ == -1) {
              LOG(INFO) << "Receive connection but not our client.";
              do_accept();
            } else if (type_ == 0) {
              LOG(INFO) << "Receive new client connection.";
              // session handler
              std::shared_ptr<Session>
                  _s(new Session(std::move(socket), enc, dec, _msg));
              s_list.push_back(_s->session);
              _s->step1();
              children[_s->session] = std::move(_s);
              do_accept();
            } else if (type_ == 1) {
              // determine if session is known
              std::string _sess = protocol::file_transfer_init_read(dec, _msg);
              if (_sess.empty()) {
                LOG(INFO) << "Receive connection but not our client.";
                socket.close();
              } else if (children.find(_sess) != children.end()) {
                // thread handler (attach to session)
                LOG(INFO) << "Receive new client thread.";
                children[_sess]->attach_thread(std::move(socket));
              } else {
                LOG(INFO) << "Receive thread but not connected client.";
                socket.close();
              }
              do_accept();
            } else {
              LOG(WARNING) << "Not defined connection type.";
              do_accept();
            }
//            std::make_shared<session>(std::move(socket))->start();
          } else {
            LOG(WARNING) << ec;
            LOG(WARNING) << "Error while receiving connection.";
            do_accept();
          }
        });
  }
};

int main(int argc, char *argv[]) {

  // arguments reader
  cxxopts::Options options("FileUploader-Client", "Uploader client.");

  options.add_options()
      ("p,port", "Port number to bind", cxxopts::value<int>())
      ("k,key", "Encrypt key to communicate", cxxopts::value<std::string>());

  int port;
  std::string key;

  auto result = options.parse(argc, argv);

  try {
    port = result["p"].as<int>();
    key = result["k"].as<std::string>();
  }
  catch (const cxxopts::OptionException &e) {
    std::cerr << "Incorrect parameters" << std::endl;
    exit(1);
  }
  catch (const std::domain_error &e) {
    std::cerr << "One or more required parameters not given" << std::endl;
    exit(1);
  }

  el::Configurations defaultConf;
  defaultConf.setToDefault();
  defaultConf.setGlobally(
      el::ConfigurationType::Format, "[%datetime - %level]: %msg");
  el::Loggers::reconfigureLogger("default", defaultConf);

  #ifdef WIN32
  protocol::init_environment();
  #endif

  protocol::AESEncrypter enc(key);
  protocol::AESDecrypter dec(key);

  boost::asio::io_context io_context;

  Acceptor a(io_context, port, enc, dec);

  a.do_accept();

  io_context.run();

  #ifdef WIN32
  protocol::clear_environment();
  #endif

  return 0;
}
