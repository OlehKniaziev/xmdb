# XMDB
XMDB is a media-first relational database which supports most popular video, audio and image codecs as builtin SQL
column types.

## Quick start

### Building
Prerequisites:

* A linux system (might work on other POSIX-compatible systems like Mac or the BSDs, not tested though)
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

### Running the server
```shell
cd ./build/
# To override default configuration see the command line flags: ./src/Server/xmdb_server -help
./src/Server/xmdb_server
```

### Connecting to a running server from the terminal
```shell
cd ./build/
./src/CLI/xmdb -hostname <hostname> -port <port, default 6969> -db <database name> -user <username> -password <plaintext password of the user>
> create table tab (id int, name text);
> insert into tab(id, name) values (1, "hello"), (2, "world");
> select * from tab;
|1|hello|
|2|world|
```

### How to run the GUI
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

For the GUI usage guide, consult appropriate [documentation](./gui/README.md).

## Demo
![](./demos/general.gif)
