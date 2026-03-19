# XMDB Project Overview

XMDB is a media-first relational database that supports video, audio, and image codecs as built-in SQL column types.

## Architecture

The project is divided into several main components:

- **Core (`src/Core/`)**: The database engine, handling storage, B-Tree indexing (`BTreeIndex.cpp`), table management (`DBTable.cpp`), and execution context (`QueryExecutionContext.cpp`).
- **SQL (`src/SQL/`)**: The SQL processing layer, including the lexer (`Lexer.cpp`), parser (`Parser.cpp`), type checker (`type_check.cpp`), and intermediate representation (`ir.cpp`).
- **Server (`src/Server/`)**: A C++ HTTP server that exposes the database functionality over the network.
- **CLI (`src/CLI/`)**: A command-line interface for interacting with a running XMDB server.
- **GUI (`gui/`)**: A modern web-based interface built with React, Vite, and TypeScript.
- **Third Party (`third_party/`)**: External libraries used by the project.

## Technologies

- **Backend**: C++20, CMake, GoogleTest.
- **Frontend**: React 19, TypeScript, Vite, Zustand (state management), Monaco Editor.
- **Communication**: HTTP protocol between GUI/CLI and the Server.

## Building and Running

### Backend (C++)

Prerequisites: CMake >= 3.20, C++20 compatible compiler.

```powershell
# Fetch submodules
git submodule init
git submodule update --recursive

# Configure and build
cmake -B build
cmake --build build
```

The executables will be located in `build/src/`.
- `xmdb_server`: The database server.
- `xmdb`: The command-line client.

### Frontend (GUI)

Prerequisites: Node.js, npm/pnpm/yarn.

```powershell
cd gui
npm install
npm run dev
```

The GUI will be available at `http://localhost:5173` (by default).

## Testing

The project uses GoogleTest for C++ unit tests and a custom snapshot testing system.

- **Run unit tests**: Use `ctest` from the `build` directory or run the test executables directly (e.g., `build/tests/SQLParser-test`).
- **Test files**: Located in the `tests/` directory.
- **Snapshots**: Located in `tests/snaps/`, managed by `tests/snap.py`.

## Development Conventions

- **C++ Standard**: Strictly C++20.
- **Code Style**: Defined in `.clang-format`.
- **Naming**: Generally uses `snake_case` for functions and variables, `PascalCase` for classes and structs (though some core structs use `PascalCase` and others `snake_case`).
- **Allocations**: Often uses an `ArenaAllocator` (from `ok::ArenaAllocator`) for temporary compilation and execution data to ensure memory safety and performance.
- **Error Handling**: Uses a `Result` pattern and `Optional` types for explicit error management.
