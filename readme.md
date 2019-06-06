# File Uploader

Upload file from local to remote multithread.<br/><br/>

## How to run:
1. Clone the repository with its submodules
```bash
git clone https://github.com/tongyuantongyu/FileUploader.git
git submodule update --init --recursive
```
2. Due to limitation of git submodule, you need to manually add
```c++
#ifdef RELEASE_LOG
#define ELPP_DISABLE_DEBUG_LOGS
#define ELPP_DISABLE_TRACE_LOGS
#endif
```
to third_party\easyloggingpp\src\easylogging++.h under
```c++
#ifndef EASYLOGGINGPP_H
#define EASYLOGGINGPP_H
```
3. Compile cryptopp
```bash
.\third_party\cryptopp\make
```
4. Run cmake
For debug build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -G "CodeBlocks - MinGW Makefiles" .\
```
For release build
```bash
cmake -DCMAKE_BUILD_TYPE=Release -G "CodeBlocks - MinGW Makefiles" .\
```
5. Compile
```bash
make
```