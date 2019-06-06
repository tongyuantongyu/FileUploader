# File Uploader

Upload file from local to remote multithread.<br/><br/>

Due to limitation of git submodule, you need to manually add
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