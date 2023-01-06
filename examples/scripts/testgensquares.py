# Sample client program to communicate with gpsim

import socket
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("", 0x1234))


s.send("help\n")
print s.recv(20)

# helper functions
def ParseInt(buf):
    if buf[0:3] == "$03" :
        return eval("0x"+buf[3:11])

    return 0


# Symbol Command 
s.send("$"+"97"+"0205start")
r = s.recv(20)
start_adr = ParseInt(r)
print "Start address:"  + str(start_adr)

s.send("reset\n")
print s.recv(20)

s.send("step\n")
print s.recv(20)
s.send("step\n")
print s.recv(20)

# define an example input and send it to the simulator:
x=4
count="0300000020"
s.send("$"+"99"+ count + "03%08x"%x)

# another example of assigning a memory location 
r = s.recv(20)
s.send("$"+"99"+ "0300000040" + "03000000ff")
r = s.recv(20)

# Move the program counter (doesn't work right now)

PCL="0300000002"
# Assign RAM command
#s.send("$"+"99"+ PCL +"03" + "%08x"%start_adr)
#st = "%08x"%start_adr
#s.send("$"+"99"+ PCL + "030000002")
#r = s.recv(20)
#print r


# start the simulation:

s.send("run\n")
print s.recv(20)


# Examine the results
xsq_lo_adr = "0300000022"
s.send("$"+"91"+xsq_lo_adr)
r = s.recv(20)
xsq_lo = ParseInt(r)
print "received:" + r + " decoded as " + str(xsq_lo)

xsq_hi_adr = "0300000023"
s.send("$"+"91"+xsq_hi_adr)
r = s.recv(20)
xsq_hi = ParseInt(r)
print "received:" + r + " decoded as " + str(xsq_hi)

xsq = xsq_hi*256 + xsq_lo
print " Sent " + str(x)
print " Received " + str(xsq)

if x*x == xsq :
  print "PASSED"

s.close()
