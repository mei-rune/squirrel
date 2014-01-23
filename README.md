squirrel
=======
squirrel is an asynchronous HTTP server framework written in C. The goal of squirrel is to learn how to create a server with a minimal feature set that can handle a high rate of requests and connections with as low of latency and resource usage as possible.

squirrel uses the event loop based libuv platform layer that node.js is built on top of (also written in C). libuv abstracts IOCP on Windows and epoll/kqueue/event ports/etc. on Unix systems to provide efficient asynchronous I/O on all supported platforms.

squirrel isn't very useful yet but I wanted to open source it from the very beginning.

## Features
- Cross platform (Windows, Linux)
- HTTP keep-alive
- Non-blocking I/O

## Plans or Ideas
- HTTP handler routing
- SPDY support

## Building squirrel
To compile squirrel you need `git` and `python` installed and in your path.

squirrel uses `gyp` which supports generating make type builds or Visual Studio projects. The Visual Studio projects aren't fully complete so they may not function just yet but they will real soon, hang in there.
    
    git clone https://github.com/runner-mei/squirrel.git

### Compiling on Linux
    ./build.sh

### Compiling on Windows
Open the Developer Command Prompt for Visual Studio

    build.bat
