#!/usr/bin/env python3

import socket
import struct

HOST = "127.0.0.1"
PORT = 44567

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((HOST, PORT))
        msg = b"Hello from client"
        msg_len = len(msg)
        buff_fmt = f"!I{msg_len}s"
        buff = struct.pack(buff_fmt, msg_len, msg)
        sock.sendall(buff)
        


if __name__ == "__main__":
    main()
