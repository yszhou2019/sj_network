import socket
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(('127.0.0.1', 5000))


sock.listen(5)
while True:
    sock_c, addr = sock.accept()
    while True:
        d = {

        }
        str = sock_c.recv(4*1024*1024)
        print(str)
        sock_c.send(b'abc')

sock.close()