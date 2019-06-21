//
// Created by TYTY on 2019-04-21 021.
//

#ifndef TUNNEL_FILE_H
#define TUNNEL_FILE_H

#include <fstream>
#include <iostream>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <exception>

#include "./third_party/easyloggingpp/src/easylogging++.h"

/// \file file.h
/// \brief Header for file related works
/// \note All things are in `file` namespace

namespace file {

namespace fs = std::filesystem;

class NoEnoughSpace : public std::runtime_error {
 public:
  NoEnoughSpace()
      : std::runtime_error("No enough space for file to write.") {}
};

/// \class file_reader
/// \brief Class to hold a file stream
/// \detail This class is an encapsulation of the file std::ifstream which
///         provides simple methods to open and read file safely.
/// \datamember std::ifstream _file
///             file stream to perform read action on.
/// \datamember std::uintmax_t file_size
///             file size of the opened file.
///             during reading it will represent remaining file size
/// \datamember unsigned int buff_s
///             size of the read buffer. determine how many bytes will be read
///             for each call of read.
/// \datamember std::mutex _lock
///             simple mutex lock to prevent multi read process happen in the
///             same time which will mess the piece order.
/// \datamember bool ok @default_value: false
///             to determine if the file is opened and ready to read.
/// \datamember std::uintmax_t offset @default_value: 0
///             file read offset to provide piece order information.
class file_reader {
 private:
  std::ifstream _file;
  std::uintmax_t file_size;
  unsigned int buff_s;
  std::mutex _lock;
  bool ok = false;
  std::uintmax_t offset = 0;
 public:
  /// \brief constructor
  /// \param _file_name name (and place) of the file to be read
  /// \param buff_size the size of the read buffer used in read function
  file_reader(const char *_file_name, const int &buff_size);

  /// \brief constructor
  /// \note copy construct is not allowed as multi instance should never
  ///     control the same file. use an reference or pointer to the
  ///     instance. the instance is thread safe.
  file_reader(file_reader &_) = delete;

  /// \brief destructor
  /// \note close the file (if open) and destruct self.
  ~file_reader();

  /// \brief read data from file
  /// \param buffer space to save the read data in.
  ///         should have a size of at least @buff_s.
  /// \param offset_ store the start byte number of the read piece.
  /// \return length of the read data. Should be the same as @buff_s
  ///         unless reached the last piece or the end, at which will
  ///         return 0 for end, and real read bytes for last piece.
  int read(char *buffer, std::uintmax_t &offset_);

  /// \brief @debug read data from file
  /// \param buffer space to save the read data in.
  ///         should have a size of at least @file_size.
  /// \return always 0
  int read_all(char *buffer);

  /// \brief return size of the opened file.
  /// \return @file_size data member.
  std::uintmax_t get_size() { return file_size; }
};

/// \class file_writer
/// \brief Class to hold a file stream
/// \detail This class is an encapsulation of the file std::0fstream which
///         provides simple methods to open and write file safely.
/// \datamember std::ofstream _file
///             file stream to perform write action on.
/// \datamember std::mutex _lock
///             simple mutex lock to prevent multi read process happen in the
///             same time which will mess the piece order.
/// \datamember bool ok @default_value: false
///             to determine if the file is opened and ready to read.
class file_writer {
 private:
  std::ofstream _file;
  std::mutex _lock;
 public:
  bool ok = false;

  /// \brief constructor
  /// \param _file_name name (and place) of the file to be write
  /// \param file_size size of the file to be open to write
  /// \detail this function will check if thre are enough space left on the
  ///         file system. However it can't know if the space left become
  ///         insufficient after it check due to other write perform to the
  ///         file system.
  file_writer(const char *_file_name, const std::uintmax_t &file_size);

  /// \brief constructor
  /// \note copy construct is not allowed as multi instance should never
  ///     control the same file. use an reference or pointer to the
  ///     instance. the instance is thread safe.
  file_writer(file_writer &_) = delete;

  /// \brief destructor
  /// \note close the file (if open) and destruct self.
  ~file_writer();

  /// \brief close the file opened before the object itself destruct
  void close();

  /// \brief write data to the opened file
  /// \param buffer overloaded to accept both char * and string as data source.
  /// \param len length of the data to be write in. Should be equal or less than
  ///         the buffer size.
  /// \param offset offset of the piece to be write.
  int write(const char *buffer,
            const int &len,
            const std::uintmax_t &offset);

  int write(const std::string &buffer,
            const int &len,
            const std::uintmax_t &offset);
};

}

#endif //TUNNEL_FILE_H
