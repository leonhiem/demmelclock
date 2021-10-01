#ifndef SOCKET_H
#define SOCKET_H
//
// $Id$
//
// Copyright (c) 2009 G.W. Kant, Astron, The Netherlands
//                    E. van der Wal, Astron, The Netherlands
//

#include <sys/socket.h>
#include <netpacket/packet.h>  // struct sockaddr_ll
#include <net/if.h>            // struct ifreq
#include <stddef.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <stdio.h>

using namespace std;

/*
   TCP Server Socket listens to port p and binds to interface iface.
   Set iface="" to not to bind to an interface.
*/
class TCPSSocket {
  struct sockaddr_in name;
  struct hostent* hostinfo;
  int sock;
  uint16_t port;
  int MaxServers;

 public:
  TCPSSocket(uint16_t p, const char* ifname, int maxservers=1) throw(const char*);
  ~TCPSSocket();
  int listen(void) throw(const char*);
};

// Connected TCP server socket
class TCPCSSocket {
 protected:
    int sock;

 public:
    TCPCSSocket(int descr = -1);
    ~TCPCSSocket();
    void Shutdown() { shutdown(sock,SHUT_RDWR); }
    size_t rx(unsigned char* buf, size_t size) ;
    size_t rx_peek(unsigned char* buf, size_t size) ;
    size_t tx(const unsigned char* data, size_t size) throw(const char*);
};

/*
   TCP Client Socket connects to server/tcpport
*/
class TCPCSocket {
 protected:
    int sock;
    const char *server;
    int tcpport;

 public:
    TCPCSocket(const char *srv, int port) throw(const char*);
    ~TCPCSocket();

    size_t rx(unsigned char *buf, size_t maxlen);
    size_t rx_peek(unsigned char *buf, size_t maxlen);
    size_t tx(const unsigned char* mes, size_t len) throw(const char*);
};

#endif // SOCKET_H
