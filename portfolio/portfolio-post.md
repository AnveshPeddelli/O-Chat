# Portfolio Post Copy

## Natural Version

I built a small C++ project called **O_Chat**.

The idea was simple: an office chat app that runs on the local network, without accounts, cloud storage, or a saved chat history. One person hosts the room, others join using the host IP, and messages are relayed in real time.

The project started as a skeleton. The headers were there, but the real `Client`, `Server`, and `Session` logic still needed to be written. I implemented the TCP flow with standalone ASIO, added multi-client session handling, and cleaned up the console UI so the app feels usable instead of just technically working.

One bug that taught me a lot: I stored port `54000` in a signed `short`, which overflowed and broke the connection. It looked like a socket issue at first, but the fix was changing the type to `std::uint16_t`.

The privacy goal is that chat data should be temporary. There is no database and no chat log file. I also started adding shared room-key encryption, but I am not calling it production-grade secure yet. For now, I see this as a privacy-oriented LAN chat prototype and a solid systems-programming project.

Tech used: C++, CMake, standalone ASIO, TCP sockets, Windows console.

## Short Version

Built **O_Chat**, a local-network office chat prototype in C++.

It supports host/join mode, multiple connected users, async TCP message relay, username-based chat formatting, and no saved chat history. I also worked on the privacy side by keeping messages ephemeral and starting a shared room-key encryption path.

Most useful bug: port `54000` broke because I stored it in a signed `short`. Changing it to `std::uint16_t` fixed the connection issue.

I would describe it as a prototype, not a production secure messenger, but it was a strong project for learning networking, async IO, cleanup, and practical privacy tradeoffs.

## Hashtags

`#cplusplus` `#networking` `#systemsprogramming` `#cmake` `#privacy` `#softwareengineering`
