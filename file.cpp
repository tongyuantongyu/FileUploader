//
// Created by TYTY on 2019-04-21 021.
//

#include "file.h"

using namespace file;

using std::string;

file_reader::file_reader(const char *_file_name, const int &buff_size)
    : buff_s(buff_size) {

  // using std::filesystem library to check file availability
  try {
    file_size = fs::file_size(_file_name);
  }
  catch (fs::filesystem_error &e) {
    LOG(ERROR) << "Failed in reading file: " << e.what();
    // since some error occured during reading the file, we simply exit
    exit(1);
  }

  if (file_size == -1) {
    LOG(ERROR) << "Failed in reading file: File not exist.";
    // since the file not exist, we simply exit
    exit(1);
  }
  // open the file in binary read mode
  _file.open(_file_name, std::ios::in | std::ios::binary);
  // put the pointer to the begin of the file
  _file.seekg(0);

  // file opened successfully
  ok = true;

  LOG(INFO) << "File opened for reading: " << _file_name;
}

file_reader::~file_reader() {
  // close the file when file_reader object is recycled
  // - but only if it is open now
  if (_file.is_open()) {
    _file.close();
  }
  LOG(INFO) << "File reading closed.";
}

int file_reader::read(char *buffer, std::uintmax_t &offset_) {

  // confirm that there's only one thread doing file reading operation
  std::lock_guard<std::mutex> lock(_lock);

  // check file status before reading it
  if (!(ok && _file.is_open())) {
    LOG(WARNING) << "File not open";
    return 0;
  } else if (_file.eof()) {
    // transfer thread should send a packet inform file transfer finished
    // after got 0 read byte return
    LOG(DEBUG) << "File reached end before reading.";
    ok = false;
    return 0;
  } else if (file_size < buff_s) {
    // there we add an offset data member to store piece count
    // see below for reason
    offset_ = offset * buff_s;
    ++offset;

    _file.read(buffer, buff_s);

//    LOG(DEBUG) << "File read: offset " << offset_ << " block_size "
//               << file_size;

    // below decrease the file_size in each opetation so now it is
    // the size of the last piece
    return file_size;
  } else {
    _file.read(buffer, buff_s);

    // during debug the offset increcement has some strange disorder
    // seems compiler will do out of order execution here
    // which can mess up the reading offset config
    // offset_ = _file.tellg()
    offset_ = offset * buff_s;
    ++offset;

    // decrease the file_size to calculate size of the last piece
    file_size -= buff_s;
//    LOG(TRACE) << "File read: offset " << offset_ << " block_size " << buff_s;
    return buff_s;
  }
}

int file_reader::read_all(char *buffer) {
  _file.read(buffer, file_size);
  return 0;
}

file_writer::file_writer(const char *_file_name, const std::uintmax_t &file_size) {

  // get remaining space of current directory
  fs::space_info space = fs::space(fs::current_path());

  // throw exception when no enough space.
  if (space.available < file_size) {
    LOG(ERROR) << "Failed in writing file: No enough space.";
    throw NoEnoughSpace();
  }

  // open file in binary write mode
  _file.open(_file_name, std::ios::out | std::ios::binary);
  // put the pointer to the begin of the file
  _file.seekp(0);

  // check if file is open
  if (_file.is_open()) {
    ok = true;
  }

  LOG(INFO) << "File opened for writing: " << _file_name;
}

file_writer::~file_writer() {
  if (_file.is_open()) {
    // flush to make everything write into disk
    _file.flush();
    _file.close();
    LOG(INFO) << "File writing closed.";
  }
}

void file_writer::close() {
  if (_file.is_open()) {
    _file.flush();
    _file.close();
    LOG(INFO) << "File writing closed.";
  }
}

int file_writer::write(const char *buffer,
                       const int &len,
                       const std::uintmax_t &offset) {

  // only one thread can do write operation
  std::lock_guard<std::mutex> lock(_lock);

  if (!(_file.is_open())) {
    LOG(DEBUG) << "File not open";
    return 0;
  } else {
    // move pointer to piece offset
    _file.seekp(offset);
    _file.write(buffer, len);
//    LOG(TRACE) << "File write: offset " << offset << " block_size " << len;
    return len;
  }
}

// same function with different input type
int file_writer::write(const string &buffer,
                       const int &len,
                       const std::uintmax_t &offset) {

  std::lock_guard<std::mutex> lock(_lock);

  if (!(_file.is_open())) {
    LOG(DEBUG) << "File not open";
    return 0;
  } else {
    _file.seekp(offset);
    _file.write(buffer.c_str(), len);
//    LOG(TRACE) << "File write: offset " << offset << " block_size " << len;
    return len;
  }
}
