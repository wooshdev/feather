#!/usr/bin/env python

import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('127.0.0.1', 80))
sock.send("GET /\n HTTP/1.1\r\n\r\n")
print sock.makefile().readline()
