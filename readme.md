# File Uploader

Upload file from local to remote in multithread.<br/><br/>
[https://github.com/tongyuantongyu/FileUploader](https://github.com/tongyuantongyu/FileUploader)

## How to run:
###1. Clone the repository with its submodules
```bash
git clone https://github.com/tongyuantongyu/FileUploader.git
cd FileUploader
git submodule update --init --recursive
```
###2. Compile cryptopp
```bash
.\third_party\cryptopp\make static
```
Or you can also download a prebuilt version of cryptopp library from the 
[release](https://github.com/tongyuantongyu/FileUploader/releases) page.<br/>
###3. Run cmake and compile<br/>
On Windows(Using MinGW)<br/>
For debug build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -G "MinGW Makefiles" -S ./ -B ./cmake-build-debug/
cmake --build ./cmake-build-debug --target all --
```
For release build
```bash
cmake -DCMAKE_BUILD_TYPE=Release -G "MinGW Makefiles" -S ./ -B ./cmake-build-release/
cmake --build ./cmake-build-release --target all --
```
On Linux(Only tested with GNU C++ Compiler 8.3.0)<br/>
For debug build
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -G "Unix Makefiles" -S ./ -B ./cmake-build-debug/
cmake --build ./cmake-build-debug --target all --
```
For release build
```bash
cmake -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" -S ./ -B ./cmake-build-release/
cmake --build ./cmake-build-release --target all --
```
Check cmake-build-debug or cmake-build-release folder for compiled binary.
