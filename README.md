# TCP Ping

Author: Po-Hsuan Su (b08902107)

This is a ping tool using TCP. This tool consists of two executable, the client and the server.

This project is a homework of NTU CSIE Computer Networks, Fall 2019.

## Feature highlights

![Screen shot of the client](https://i.imgur.com/Y7p6sDQ.png)

- Can ping mutiple target simultaneously. (Implemented using `std::thread`)
- Colored output.
- Simple ping statistics.

## How to use

These instructions will get you a copy of the tool up and running.

### Prerequisites

- g++
- Boost C++ Libraries (Or more specific, the `libboost-program-options-dev` package)
- GNU Make

This tool can only run on linux.

### Installing

1. Get the source code of this tool. Maybe clone from GitHub. Skip this step if you already got the source code.

2. run `make` at the project root (at the same folder where this README.md file is).

If nothing goes wrong, you now get both the `client` and the `server` executable.

### Running

Usage of the server:

```text
Usage: ./server [--help] port

Positional Argument
  port - Port to listen on

Optional Arguments
  -h [ --help ]         Print help messages
```

The server should be started before the client.

Usage of the client:

```text
Usage: ./client [--help] [-n num] [-t timeout] [-w wait] target1 [target2 ...]

Positional Argument
  target - Target(s) in `host:port` form, host can be in hostname or ip

Optional Arguments
  -h [ --help ]                Print help messages
  -n [ --num ] arg (=0)        Amount of packets to send to each target, 0
                               means keep sending messages until program close
  -t [ --timeout ] arg (=1000) Maximum millisecond(s) the client needs to wait
                               for each response
  -w [ --wait ] arg (=1000)    Millisecond(s) the client need to wait between
                               each ping
```
