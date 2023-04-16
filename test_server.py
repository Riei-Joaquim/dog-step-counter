import socket
import urllib.request
import time

# bind all IP
HOST = '0.0.0.0' 
# Listen on Port 
PORT = 2255
SEND_PING_PORT = 10211
#Size of receive buffer   
BUFFER_SIZE = 1024    
# Create a TCP/IP socket
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
# Bind the socket to the host and port
s.bind((HOST, PORT))

collar_ip = ""
collar_token =""
while True:
    # Receive BUFFER_SIZE bytes data
    # data is a list with 2 elements
    # first is data
    #second is client address
    data = s.recvfrom(BUFFER_SIZE)
    if data:
        collar_ip = data[1][0]
        collar_token =  str(data[0]).split('|')[-1]
        print('discovery collar IP:' , collar_ip, "token Id:", collar_token)
        time.sleep(0.2)
        s.sendto(bytes("hello collar from server 101", "utf-8"), (collar_ip, SEND_PING_PORT))
        try:
            contents = urllib.request.urlopen("http://"+collar_ip+"/pedometer").read()
            break
        except:
            pass
# Close connection
s.close() 

# getData
while True:
    contents = urllib.request.urlopen("http://"+collar_ip+"/pedometer").read()
    print(contents)
    time.sleep(10)

