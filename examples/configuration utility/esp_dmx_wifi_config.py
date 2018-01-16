#!/usr/bin/python

# This file is part of LXDMXWiFi_library used with the WiFi2DMX example
# Copyright 2016 by Claude Heintz Design
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# 
# * Neither the name of LXDMXWiFi_library nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# -----------------------------------------------------------------------------------
#  
#  Use this utility to configure the WiFi connection and DMX protocol of
#  an ESP8266 running WiFi2DMX firmware.  Holding pin 16 low on boot
#  causes WiFi2DMX to startup in access point mode, listening on the Art-Net port.
#  Connecting a computer's WiFi to the "ESP-DMX-WiFi" access point and
#  running this script will allow reading and uploading ESP-DMX config packets.
#
#  A ESP8266 running WiFi2DMX firmware that is configured to login to a WiFi
#  network can also be found and configured with this utility even if it is
#  configured to obtain an IP address through DHCP.  For WiFi2DMX
#  configured for Art-Net, a broadcast address can be used as a target to
#  communicate with the ESP8266.  For sACN, the multicast address assigned
#  to WiFi2DMX can be used. 
#
#
#	Original Python version does not support later LXDMXWiFi_library config settings
#


from Tkinter import *
import tkMessageBox
import socket

root = Tk()

########################################################## 
#################### global variables ####################

udpsocket = None
udpport = 0x1936

c1v = IntVar()
c2v = IntVar()
c3v = IntVar()
c4v = IntVar()
c5v = IntVar()
c6v = IntVar()
c7v = IntVar()
c8v = IntVar()
c9v = IntVar()
c10v = IntVar()
c11v = IntVar()
c12v = IntVar()

sv_ssid = StringVar()
sv_pwd = StringVar()
sv_apip = StringVar()
sv_apgw = StringVar()
sv_apsn= StringVar()
sv_stip = StringVar()
sv_stgw = StringVar()
sv_stsn= StringVar()
sv_mcip = StringVar()
sv_acnu = StringVar()
sv_ansn = StringVar()
sv_nn = StringVar()
sv_anu = StringVar()
sv_tar = StringVar()
sv_nettar = StringVar()

########################################################## 
#################### socket functions ####################

def setupSocket():
   global udpsocket
   global udpport
   if udpsocket != None:
      udpsocket.close()
   udpsocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
   udpsocket.setsockopt(socket.SOL_SOCKET,socket.SO_BROADCAST,1)
   udpsocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
   try:
      udpsocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
   except:
      pass
   udpsocket.bind(('0.0.0.0', udpport))
   udpsocket.settimeout(3.0)
   
def closeSocket():
   global udpsocket
   if udpsocket != None:
      udpsocket.close()
      udpsocket = None
      
def bytes2ipstr(d, i):
   return str(ord(d[i]))+"."+str(ord(d[i+1]))+"."+str(ord(d[i+2]))+"."+str(ord(d[i+3]))

def bytearr2ipstr(d,i):
   return str(d[i])+"."+str(d[i+1])+"."+str(d[i+2])+"."+str(d[i+3])
   
def ipstr2bytes(s, d, i):
   octs = s.split(".")
   if len(octs) == 4:
     d[i] = int(octs[0])
     d[i+1] = int(octs[1])
     d[i+2] = int(octs[2])
     d[i+3] = int(octs[3])
   
def recvWiFiConfig(d,a):
   h = d[0:7]
   if h != "ESP-DMX":
      return -1
   if ord(d[8]) == 0:
      global sv_ssid
      sv_ssid.set(d[12:44])
      global sv_pwd
      sv_pwd.set(d[76:39])
      
      global c1v
      global c2v
      if ( ord(d[10]) == 0 ):
         c1v.set(0)
         c2v.set(1)
      else:
         c1v.set(1)
         c2v.set(0)
      global c3v
      global c4v
      if ( (ord(d[11]) & 1) == 0 ):
         c3v.set(0)
         c4v.set(1)
      else:
         c3v.set(1)
         c4v.set(0)
      global c5v
      global c6v
      if ( (ord(d[11]) & 2) == 0 ):
         c5v.set(0)
         c6v.set(1)
      else:
         c5v.set(1)
         c6v.set(0)
      global c7v
      global c8v
      if ( (ord(d[11]) & 4) == 0 ):
         c7v.set(0)
         c8v.set(1)
      else:
         c7v.set(1)
         c8v.set(0)
      global c11v
      if ( (ord(d[11]) & 8) == 0 ):
         c11v.set(0)
      else:
         c11v.set(1)
      if ( (ord(d[11]) & 16) == 0 ):
         print ord(d[11])
         c12v.set(0)
      else:
         c12v.set(1)
         
      global sv_apip
      sv_apip.set(bytes2ipstr(d,140))
      global sv_apgw
      sv_apgw.set(bytes2ipstr(d,144))
      global sv_apsn
      sv_apsn.set(bytes2ipstr(d,148))
      global sv_stip
      sv_stip.set(bytes2ipstr(d,152))
      global sv_stgw
      sv_stgw.set(bytes2ipstr(d,156))
      global sv_stsn
      sv_stsn.set(bytes2ipstr(d,160))
      global sv_mcip
      sv_mcip.set(bytes2ipstr(d,164))
      global sv_acnu
      sv_acnu.set( str( ord(d[168]) + (ord(d[171]) << 8) ))
      global sv_ansn
      sv_ansn.set(str(ord(d[169])))
      global sv_anu
      sv_anu.set(str(ord(d[170])))
      global sv_nn
      sv_nn.set(d[172:203])
      global sv_nettar
      sv_nettar.set(bytes2ipstr(d,204))
      global sb
      sb['state'] = 'normal'
      global sv_tar
      sv_tar.set(a[0])
      return 0
   return 1
   
def readPacket():
   global udpsocket
   rv = 0
   try:
      data,addr = udpsocket.recvfrom(512)
      rv = recvWiFiConfig(data, addr)
   except:
      global sb
      sb['state'] = 'disabled'
      print "exception reading packet"
      sv_ssid.set("??")
      rv = -1
   return rv
   
def sendQuery():
   setupSocket()
   packet = bytearray(9)
   packet[0:7] = "ESP-DMX"
   packet[7] = 0
   packet[8] = '?'
   global udpsocket
   global udpport
   global sv_tar
   post_addr = sv_tar.get()
   udpsocket.sendto(packet, (post_addr, udpport)) #10.110.115.255 0x1936
   merr = readPacket() #1st read might be our own broadcast packet we just sent
   if merr != 0:
      merr = readPacket(); #2nd try
   if merr != 0:
      merr = readPacket(); #3rd try
   if ( merr == 0 ):
      tkMessageBox.showinfo("Success", "Located configuration for ESP-DMX node.")
   else:
      tkMessageBox.showerror("Connection Error", "Could not retrieve configuration.")
   
def upload():
   setupSocket()
   spacket = bytearray(208)
   for k in range(0,171):
      spacket[k] = 0
   spacket[0:7] = "ESP-DMX"
   spacket[8] = '!'
   
   global sv_ssid
   s = sv_ssid.get()
   spacket[12:12+len(s)] = s
   
   global c1v
   global sv_pwd
   s = sv_pwd.get()
   # don't upload obscured password for station
   if c1v.get() == 0 and s.startswith("****"):
      tkMessageBox.showerror("invalid password", "enter password")
      sv_pwd.set("**** enter password ****")
      return
   spacket[76:76+len(s)] = s
   
   spacket[10] = c1v.get()
   global c3v
   global c5v
   global c7v
   global c11v
   spacket[11] = c3v.get() + 2*c5v.get() + 4*c7v.get()+ 8*c11v.get()+ 16*c12v.get()
   
   global sv_apip
   ipstr2bytes(sv_apip.get(), spacket, 140)
   global sv_apgw
   ipstr2bytes(sv_apgw.get(), spacket, 144)
   global sv_apsn
   ipstr2bytes(sv_apsn.get(), spacket, 148)
   global sv_stip
   ipstr2bytes(sv_stip.get(), spacket, 152)
   global sv_stgw
   ipstr2bytes(sv_stgw.get(), spacket, 156)
   global sv_stsn
   ipstr2bytes(sv_stsn.get(), spacket, 160)
   global sv_mcip
   ipstr2bytes(sv_mcip.get(), spacket, 164)
   global sv_acnu
   spacket[168] = int(sv_acnu.get())
   global sv_ansn
   spacket[169] = int(sv_ansn.get())
   global sv_anu
   spacket[170] = int(sv_anu.get())
   s = sv_nn.get()
   sl = len(s)
   if ( sl > 31 ):
      sl = 31
   spacket[172:172+len(s)] = s
   spacket[203] = 0
   global sv_nettar
   ipstr2bytes(sv_nettar.get(), spacket, 204)
   
   global udpsocket
   global sv_tar
   post_addr = sv_tar.get()
   try:
      udpsocket.sendto(spacket, (post_addr, udpport))
   except:
      tkMessageBox.showerror("Connection Error", "Upload failed.")
      closeSocket()
      return
   merr = readPacket() #1st read might be our own broadcast packet we just sent
   if merr != 0:
      merr = readPacket(); #2nd try
   if merr != 0:
      merr = readPacket(); #3rd try
   if ( merr == 0 ):
      tkMessageBox.showinfo("Upload Success!", "Configuration confirmation reply received.")
   else:
      tkMessageBox.showerror("Upload Confirmation Error", "Could not confirm upload.")
   closeSocket()
   
def cancel_merge():
   setupSocket()
   spacket = bytearray(107)
   for k in range(0,106):
      spacket[k] = 0
   spacket[0:7] = "Art-Net"
   spacket[9] = 0x60 #opcode
   spacket[11] = 14  #protocol version
   spacket[106] = 0x01  #cancel merge command
   
   global udpsocket
   global sv_tar
   post_addr = sv_tar.get()
   try:
      udpsocket.sendto(spacket, (post_addr, udpport))
   except:
      tkMessageBox.showerror("Connection Error", "ArtAddress cancel merge failed.")
      closeSocket()
      return
      
def clear_output():
   setupSocket()
   spacket = bytearray(107)
   for k in range(0,106):
      spacket[k] = 0
   spacket[0:7] = "Art-Net"
   spacket[9] = 0x60 #opcode
   spacket[11] = 14  #protocol version
   spacket[106] = 0x90  #clear output command
   
   global udpsocket
   global sv_tar
   post_addr = sv_tar.get()
   try:
      udpsocket.sendto(spacket, (post_addr, udpport))
   except:
      tkMessageBox.showerror("Connection Error", "ArtAddress clear failed.")
      closeSocket()
      return
   
   
###########################################################  
#################### command functions ####################

def windowwillclose():
   closeSocket()
   sys.exit()

def cb1_cmd():
   if c1v.get():
      c2v = 0
      c2.deselect()
   else:
      c2v = 1
      c2.select()

def cb2_cmd():
   if c2v.get():
      c1.deselect()
   else:
      c1.select()
      
def cb3_cmd():
   if c3v.get():
      c4.deselect()
   else:
      c4.select()
   
      
def cb4_cmd():
   if c4v.get():
      c3.deselect()
      c7.deselect()
      c8.select()
   else:
      c3.select()
   
      
def cb5_cmd():
   if c5v.get():
      c6.deselect()
   else:
      c6.select()
      
def cb6_cmd():
   if c6v.get():
      c5.deselect()
   else:
      c5.select()
      
def cb7_cmd():
   if c7v.get():
      c8.deselect()
   else:
      c8.select()
      
def cb8_cmd():
   if c8v.get():
      c7.deselect()
   else:
      c7.select()
      
def cb9_cmd():
   global udpport
   if c9v.get():
      c10.deselect()
      udpport = 0x15C0
   else:
      c10.select()
      udpport = 0x1936
   print udpport

def cb10_cmd():
   global udpport
   if c10v.get():
      c9.deselect()
      udpport = 0x1936
   else:
      c9.select()
      udpport = 0x15C0
   print udpport

def cb11_cmd():
   pass
   
def cb12_cmd():
   pass

def get_info_cmd():
   sendQuery()
   
def upload_cmd():
   upload()
   
def cancelmerge_cmd():
   cancel_merge()
   
def clearoutput_cmd():
   clear_output()
   
###########################################################  
#################### Tk initialization ####################

f = Frame(root, height=620, width=580)
f.pack()
f.pack_propagate(0)

sf = Frame(f, height=10)
sf.pack(fill=X, side="top")

gs = Frame(f)
hs = Label(gs, text="SSID:", width=20, anchor="e")
hs.pack(side="left")
es = Entry(gs, width=32, textvariable=sv_ssid)
es.pack(side="left")
gs.pack(fill=X, side="top")

gp = Frame(f)
hp = Label(gp, text="password:", width=20, anchor="e")
hp.pack(side="left")
ep = Entry(gp, width=32, textvariable=sv_pwd)
ep.pack(side="left")
gp.pack(fill=X, side="top")

ga = Frame(f, padx=100, pady=10)
c1 = Checkbutton(ga, text="Access Point", width=14, variable=c1v, command=cb1_cmd)
c1.pack(side="left")
c2 = Checkbutton(ga, text="Station", width=14, variable=c2v, command=cb2_cmd)
c2.pack(side="left")
ga.pack(fill=X, side="top")

gb = Frame(f, padx=100)
c3 = Checkbutton(gb, text="sACN", width=14, variable=c3v, command=cb3_cmd)
c3.pack(side="left")
c4 = Checkbutton(gb, text="Art-Net", width=14, variable=c4v, command=cb4_cmd)
c4.pack(side="left")
gb.pack(fill=X, side="top")

gc = Frame(f, padx=100)
c5 = Checkbutton(gc, text="Static", width=14, variable=c5v, command=cb5_cmd)
c5.pack(side="left")
c6 = Checkbutton(gc, text="DHCP", width=14, variable=c6v, command=cb6_cmd)
c6.pack(side="left")
gc.pack(fill=X, side="top")

gd = Frame(f, padx=100)
c7 = Checkbutton(gd, text="Multicast", width=14, variable=c7v, command=cb7_cmd)
c7.pack(side="left")
c8 = Checkbutton(gd, text="Broadcast", width=14, variable=c8v, command=cb8_cmd)
c8.pack(side="left")
gd.pack(fill=X, side="top")

g1 = Frame(f)
h1 = Label(g1, text="ap address:", width=20, anchor="e")
h1.pack(side="left")
e1 = Entry(g1, width=16, textvariable=sv_apip)
e1.pack(side="left")
g1.pack(fill=X, side="top")

g2 = Frame(f)
h2 = Label(g2, text="ap gateway:", width=20, anchor="e")
h2.pack(side="left")
e2 = Entry(g2, width=16, textvariable=sv_apgw)
e2.pack(side="left")
g2.pack(fill=X, side="top")

g3 = Frame(f)
h3 = Label(g3, text="ap subnet:", width=20, anchor="e")
h3.pack(side="left")
e3 = Entry(g3, width=16, textvariable=sv_apsn)
e3.pack(side="left")
g3.pack(fill=X, side="top")

g4 = Frame(f)
h4 = Label(g4, text="sta address:", width=20, anchor="e")
h4.pack(side="left")
e4 = Entry(g4, width=16, textvariable=sv_stip)
e4.pack(side="left")
g4.pack(fill=X, side="top")

g5 = Frame(f)
h5 = Label(g5, text="sta gateway:", width=20, anchor="e")
h5.pack(side="left")
e5 = Entry(g5, width=16, textvariable=sv_stgw)
e5.pack(side="left")
g5.pack(fill=X, side="top")

g6 = Frame(f)
h6 = Label(g6, text="sta subnet:", width=20, anchor="e")
h6.pack(side="left")
e6 = Entry(g6, width=16, textvariable=sv_stsn)
e6.pack(side="left")
g6.pack(fill=X, side="top")

g7 = Frame(f)
h7 = Label(g7, text="multicast address:", width=20, anchor="e")
h7.pack(side="left")
e7 = Entry(g7, width=16, textvariable=sv_mcip)
e7.pack(side="left")
g7.pack(fill=X, side="top")

g8 = Frame(f)
h8 = Label(g8, text="sACN unverse:", width=20, anchor="e")
h8.pack(side="left")
e8 = Entry(g8, width=16, textvariable=sv_acnu)
e8.pack(side="left")
g8.pack(fill=X, side="top")

g9 = Frame(f)
h9 = Label(g9, text="Art-Net subnet:", width=20, anchor="e")
h9.pack(side="left")
e9 = Entry(g9, width=16, textvariable=sv_ansn)
e9.pack(side="left")
g9.pack(fill=X, side="top")
c12 = Checkbutton(g9, text="RDM", width=14, variable=c12v, command=cb12_cmd)
c12.pack(side="left")
g9.pack(fill=X, side="top")

g10 = Frame(f)
h10 = Label(g10, text="Art-Net universe:", width=20, anchor="e")
h10.pack(side="left")
e10 = Entry(g10, width=16, textvariable=sv_anu)
e10.pack(side="left")
sb = Button(g10, text="Art-Net Cancel Merge", width=15, command=cancelmerge_cmd)
sb.pack(side="left")
g10.pack(fill=X, side="top")

g11 = Frame(f)
h11 = Label(g11, text="Art-Net node name:", width=20, anchor="e")
h11.pack(side="left")
e11 = Entry(g11, width=16, textvariable=sv_nn)
e11.pack(side="left")
sb = Button(g11, text="Art-Net Clear Output", width=15, command=clearoutput_cmd)
sb.pack(side="left")
g11.pack(fill=X, side="top")

cf1 = Frame(f)
h11 = Label(cf1, text="DMX to net:", width=20, anchor="e")
h11.pack(side="left")
e11 = Entry(cf1, width=16, textvariable=sv_nettar)
sv_tar.set("10.255.255.255")
e11.pack(side="left")
c11 = Checkbutton(cf1, text="Input DMX", width=14, variable=c11v, command=cb11_cmd)
c11.pack(side="left")
cf1.pack(fill=X, side="top")

sf2 = Frame(f, height=20)
sf2.pack(fill=X, side="top")

cf = Frame(f)
h11 = Label(cf, text="Target:", width=8, anchor="e")
h11.pack(side="left")
e11 = Entry(cf, width=16, textvariable=sv_tar)
sv_tar.set("10.110.115.10")
e11.pack(side="left")
fb = Button(cf, text="Get Info", width=15, command=get_info_cmd)
fb.pack(side="left")
sb = Button(cf, text="Upload", width=15, state=DISABLED, command=upload_cmd)
sb.pack(side="left")
cf.pack(fill=X, side="top")

gp = Frame(f, padx=75)
c10 = Checkbutton(gp, text="Art-Net port", width=14, variable=c10v, command=cb10_cmd)
c10.pack(side="left")
gp.pack(fill=X, side="top")
gp = Frame(f, padx=75)
c9 = Checkbutton(gp, text="sACN port", width=14, variable=c9v, command=cb9_cmd)
c9.pack(side="left")
gp.pack(fill=X, side="top")
c10v.set(1)



root.title("ESP-DMX Configuration Utility")
root.protocol("WM_DELETE_WINDOW", windowwillclose)
root.mainloop()