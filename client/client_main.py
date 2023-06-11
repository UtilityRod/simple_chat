#!/usr/bin/env python3

import socket
import struct

HOST = "127.0.0.1"
PORT = 44567

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((HOST, PORT))
        username = b"UtilityRod"
        username_len = len(username)
        buff_fmt = f"!II{username_len}s"
        buff = struct.pack(buff_fmt, 1, username_len, username)
        sock.sendall(buff)
        


if __name__ == "__main__":
    main()
