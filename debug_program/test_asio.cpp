//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>

using boost::asio::ip::tcp;
using boost::asio::buffer;

class Session : public std::enable_shared_from_this<Session> {
 public:
  tcp::socket s;
  std::string _tmp;
  Session(tcp::socket _s) : s(std::move(_s)) {
  }
  std::string _read(int length) {
    std::string _data = _tmp.substr(0, length);
    _tmp.erase(0, length);
    return _data;
  }
  void start() {
    read_loop();
  }
  void read_loop() {
    auto self(shared_from_this());
    std::function<std::string(int)> _t =
        std::bind(&Session::_read, shared_from_this(), std::placeholders::_1);
    _tmp.clear();
    _tmp.resize(5);
    async_read(s, buffer(_tmp), boost::asio::transfer_exactly(5),
               [this, self, _t](boost::system::error_code ec, std::size_t) {
                 if (!ec) {
                   std::cout << _t(5) << std::endl;
                 }
                 read_loop();
               });
  }
};

class Acceptor : public std::enable_shared_from_this<Acceptor> {
 private:
  tcp::acceptor acceptor_;
  tcp::socket socket_;
  std::vector<std::shared_ptr<Session>> children;
  int c = 0;
 public:
  // the socket with the client can't be copied.
  // constructor move the socket given to its member.
  explicit Acceptor(
      boost::asio::io_context &io_context,
      int port) :
      acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      socket_(io_context) {}
  void do_accept() {
    acceptor_.async_accept(socket_,
                           [this](boost::system::error_code ec) {
                             std::cout << "received input" << std::endl;
                             if (!ec) {
//            std::function<std::string(int)> _t = std::bind(&Acceptor::_read, &socket, std::placeholders::_1);
//            std::cout << _t(10) << std::endl;
                               auto _t =
                                   std::make_shared<Session>(std::move(socket_));
                               _t->start();
                               children[c] = _t;
                               ++c;
                             }
                             do_accept();
                           });
  }
};

int main() {
  WSADATA wsaData;
  WORD sockVersion = MAKEWORD(2, 2);
  if (WSAStartup(sockVersion, &wsaData) != 0) { exit(1); }

  boost::asio::io_context io_context;

  Acceptor a(io_context, 16666);

  a.do_accept();

  io_context.run();

  WSACleanup();
}