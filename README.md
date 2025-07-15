# XMDB
XMDB is a media-first relational database which (*in the future*) supports most popular video, audio and image codecs as builtin SQL
column types.

## How to build
Prerequisites:

* A linux system (might work on other POSIX-compatible systems like Mac, not tested though)
* CMake >= 3.20
* A C compiler
* A C++ compiler with support for C++20

```shell
# Fetch the submodules
git submodule init
git submodule update --recursive
# Setup build
cmake -Bbuild
cd build
# Build everything
cmake --build .
```

After that, you will have multiple executables built inside the `build/src/*` subfoulders.

## How to run the GUI
Prerequisites:

* A working browser
* A npm-compatible JS package manager

```shell
# Install dependencies
cd gui
npm install
# Run the dev server
npm run dev
```

After that, you can open `http://localhost:5137` in your browser.

Alternatively, you can build the GUI project using `npm run build`, and then use it however you like.
Note that to function correctly you need to have a working HTTP server (not HTTPS currently) serving
built files which can be then accessed from the browser.

## Demo
![](./demos/general.gif)
