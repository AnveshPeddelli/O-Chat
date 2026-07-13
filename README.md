# O_Chat

O_Chat is a small C++ console chat app for local-network or direct-IP chat.

One user hosts a room, other users join using the host IP address, and everyone in the room must use the same room key.

## Features

- Host or join from the same executable
- Multiple connected users
- Real-time chat over TCP
- Colored usernames in chat
- Shared room key for message encryption
- No saved chat history or database

## Requirements

- Windows
- CMake 3.20+
- Visual Studio Build Tools or Visual Studio
- vcpkg available in this repo

This project uses:

- C++17
- standalone ASIO
- Windows `bcrypt` for encryption helpers

## Build

From the project root:

```powershell
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=.\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Debug
```

The executable will be created at:

[O_Chat.exe](C:/Users/p.anvesh/source/repos/O_Chat/build/Debug/O_Chat.exe)

## Run

From the project root:

```powershell
.\build\Debug\O_Chat.exe
```

You will see:

```text
Choose a mode
  1) Host a room
  2) Join a room
```

## How to Host

1. Run the executable
2. Choose `1`
3. Enter a username
4. Enter a room key

After hosting, the app shows:

- the port number
- your LAN IP for same-network users
- a note about internet access

Example:

```text
[status] Hosting room on port 54000
[status] Same Wi-Fi/LAN users can join with: 192.168.3.151
```

## How to Join

1. Run the executable
2. Choose `2`
3. Enter the host IP
4. Enter your username
5. Enter the same room key as the host

Example:

```text
server ip > 192.168.3.151
username  > alice
room key  >
```

## Room Key

The room key is a shared password for the room.

Important:

- everyone in the same chat room must use the exact same room key
- if the room key is different, messages will not decrypt correctly

Good example room keys:

```text
office-chat-2026
team-sync-42
private-room-a1
```

## Chat Controls

- Type a message and press Enter to send it
- Type `/quit` to leave the room

Your own messages are shown as:

```text
[You] hello
```

Other users are shown like:

```text
[Alice] hello
[Bob] hi
```

System messages are shown separately:

```text
[system] Alice joined the room
```

## Same Network vs Internet

### Same Wi-Fi / Same LAN

This is the easiest setup.

Use the host machine's LAN IP, usually something like:

```text
192.168.x.x
10.x.x.x
```

### Over the Internet

This does **not** work automatically just because the host is running.

To connect over the internet, the host usually needs:

1. A public IP address
2. Router port forwarding for TCP port `54000`
3. Windows Firewall rule allowing port `54000`

Internet users must join using the host's **public IP**, not:

- `127.0.0.1`
- `192.168.x.x`
- `10.x.x.x`

unless they are on the same private network.

## Windows Firewall

If users on your LAN or the internet cannot connect, allow inbound TCP traffic on port `54000`.

In an Administrator PowerShell:

```powershell
New-NetFirewallRule -DisplayName "O_Chat TCP 54000" -Direction Inbound -Protocol TCP -LocalPort 54000 -Action Allow
```

## Router Port Forwarding

If friends outside your home/office network cannot connect, configure your router:

- Protocol: `TCP`
- External port: `54000`
- Internal port: `54000`
- Forward to: your host PC's LAN IP

Example:

```text
Forward TCP 54000 -> 192.168.3.151:54000
```

## Common Problems

### 1. It says `127.0.0.1`

`127.0.0.1` means "this same computer only".

Use:

- your LAN IP for same-network users
- your public IP for internet users

### 2. Friend connects but messages do not make sense

Most likely the room key is different.

Make sure everyone uses the exact same room key.

### 3. Friend cannot connect at all

Check:

- the host app is running
- the host is really in Host mode
- the joining user typed the correct IP
- Windows Firewall is not blocking port `54000`
- router port forwarding is configured for internet use

### 4. Internet friend still cannot connect after port forwarding

You may be behind CGNAT.

Common CGNAT/WAN private ranges:

- `10.x.x.x`
- `100.64.x.x` to `100.127.x.x`
- `172.16.x.x` to `172.31.x.x`
- `192.168.x.x`

If your router WAN IP is in one of those ranges, direct public incoming connections may not work.

## Recommended Alternative for Internet Use

If direct public IP connection is difficult, use a mesh VPN such as:

- Tailscale
- ZeroTier

Then each user can join using the VPN IP instead of public-IP + port-forwarding setup.

## Privacy Notes

The app is designed to avoid persistent chat history:

- no database
- no saved chat logs
- transient in-memory message handling

That said, this should still be treated as a prototype, not a production-grade secure messaging platform.

## Project Structure

- [src/main.cpp](C:/Users/p.anvesh/source/repos/O_Chat/src/main.cpp) - startup flow, host/join logic, chat loop
- [src/Client.cpp](C:/Users/p.anvesh/source/repos/O_Chat/src/Client.cpp) - client connection, send/receive
- [src/Server.cpp](C:/Users/p.anvesh/source/repos/O_Chat/src/Server.cpp) - server accept loop and broadcast
- [src/Session.cpp](C:/Users/p.anvesh/source/repos/O_Chat/src/Session.cpp) - per-user session handling
- [src/ConsoleUi.cpp](C:/Users/p.anvesh/source/repos/O_Chat/src/ConsoleUi.cpp) - console formatting and input behavior
- [src/Crypto.cpp](C:/Users/p.anvesh/source/repos/O_Chat/src/Crypto.cpp) - room-key encryption helpers

## Notes

- This app is currently Windows-focused
- The build currently targets the local vcpkg toolchain in this repo
- If a previous `O_Chat.exe` window is still open, close it before rebuilding

