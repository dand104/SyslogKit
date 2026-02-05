# SyslogKit
Simple and portable Syslog server + client

## Feature Overview
- Reception over UDP and TCP
- Logging to a local SQLite database.
- Real-time log view
- Export filtered logs to standard `.log` text files or binary `.db` backups.

## Project Structure
- `common/`: Core logic, syslog protocol parsing, and SQLite storage implementation.
- `client/`: Qt-based graphical user interface source code.

## Building from Source

1. **Requirements**:
    - CMake 3.30+
    - Qt 6 (Core, Widgets, GUI, Network)
    - A C++20 compatible compiler (Clang 18+, MSVC 2022, or GCC 11+)

2. **Sample Build Command**:
   ```bash
   cmake -B build
   cmake --build build

## License
Copyright 2025-2026 dand104

Licensed under the Apache License, Version 2.0; See LICENSE

