# Sample client program to communicate with gpsim

import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("", 0x1234))

s.send("load s gensquares.cod\n")
print s.recv(20)

#s.send("break e start\n")
#print s.recv(20)

s.send("break e done\n")
print s.recv(20)

s.close()
