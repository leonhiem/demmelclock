//
// $Id$
//
// Copyright (c) 2009 G.W. Kant, Astron, The Netherlands
//                    E. van der Wal, Astron, The Netherlands
//                    L. Hiemstra

#include <stdlib.h>
#include <sys/ioctl.h> 
#include <arpa/inet.h> // htons
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <syslog.h>

#include "socket.h"



/*
   TCP Server Socket listens to port p and binds to interface iface.
   Set iface="" to not bind to an interface.
*/
TCPSSocket::TCPSSocket(uint16_t p, const char* iface, int maxservers) throw(const char *)
{
        port = p;
        MaxServers = maxservers;
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock < 0) 
                throw "TCPSSocket: socket creation error"; 
        int val = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
                                &val, sizeof(val)) < 0) {
                close(sock);
                cerr << "Error: " << strerror(errno) << endl;
                throw "TCPSSocket: cannot set socket option SO_REUSEADDR";
        }
        if (setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE,
                                (void*) iface, strlen(iface)) < 0) {
                close(sock);
                cerr << "Error: " << strerror(errno) << endl;

                throw "TCPSSocket: cannot set socket option SO_BINDTODEVICE";
        }
        /* Give the socket a name. */
        name.sin_family = AF_INET;
        name.sin_port = htons (port);
        name.sin_addr.s_addr = htonl (INADDR_ANY);
        if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
                close(sock);
                cerr << "Error: " << strerror(errno) << endl;
                throw "TCPSSocket: cannot bind socket";
        }

}

TCPSSocket::~TCPSSocket()
{
  close(sock);
}

int TCPSSocket::listen(void) throw(const char *)
{
        ostringstream strs;
        if (::listen (sock, 1) < 0) 
                throw "TCPSSocket::listen: cannot put socket into listening state";

        /* Block until input arrives on the server socket. */
        int nsock;
        struct sockaddr_in clientname;
        socklen_t size = sizeof (clientname);

        nsock = accept (sock,(struct sockaddr *) &clientname,&size);
        if (nsock < 0) 
                exit(-1);

        strs << "Server: Client connect from host " 
                << inet_ntoa (clientname.sin_addr) 
                << ":" <<  ntohs(clientname.sin_port)
                << endl;

        syslog(LOG_INFO,strs.str().c_str());

        return nsock;
}


TCPCSSocket::TCPCSSocket(int s)
{
    sock = s;
}

TCPCSSocket::~TCPCSSocket()
{
  close(sock);
}

size_t TCPCSSocket::rx(unsigned char *buf, size_t maxlen)
{
  size_t nrxbytes = 0;
  //struct timeval timeout;
  //timeout.tv_sec = timeoutms / 1000;
  //timeout.tv_usec = ( timeoutms % 1000 ) * 1000;
 
  // Receive a datagram from a tx source
  do
  {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    // No time out
    select(sock + 1, &readfds, NULL, NULL, NULL);
    if FD_ISSET(sock, &readfds)
      nrxbytes = recvfrom(sock, buf, maxlen, 0, NULL, NULL);
      break;
  } while (1);
  return nrxbytes;
}

size_t TCPCSSocket::rx_peek(unsigned char *buf, size_t maxlen)
{
  size_t nrxbytes = 0;
  //struct timeval timeout;
  //timeout.tv_sec = timeoutms / 1000;
  //timeout.tv_usec = ( timeoutms % 1000 ) * 1000;
 
  // Receive a datagram from a tx source
  do
  {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    // No time out
    select(sock + 1, &readfds, NULL, NULL, NULL);
    if FD_ISSET(sock, &readfds)
      nrxbytes = recvfrom(sock, buf, maxlen, MSG_PEEK, NULL, NULL);
      break;
  } while (1);
  return nrxbytes;
}

size_t TCPCSSocket::tx(const unsigned char* mes, size_t len) throw(const char*)
{
  int ntxbytes = write(sock, mes, len);
  if (ntxbytes < 0) 
    throw "TCPCSSocket::tx: could not send message";
  return ntxbytes;
}

/*
 * TCP Client Socket connects to server/tcpport
 */
TCPCSocket::TCPCSocket(const char *srv, int port) throw(const char*)
{
    struct sockaddr_in address;
    struct hostent *hp;
    int optval = 1;
    static char strerror_buf[200];

    tcpport=port;
    server = new char(strlen(srv)+1);
    strcpy((char *)server,srv);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        sprintf(strerror_buf,"socket: %s",strerror(errno));
        throw(strerror_buf);
    }

    if(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,(void *)&optval,sizeof(optval)) == -1) {
        sprintf(strerror_buf,"setsockopt: %s",strerror(errno));
        throw(strerror_buf);
    }

    hp = gethostbyname(server);
    if(hp == NULL) {
        sprintf(strerror_buf,"gethostbyname: %s",strerror(errno));
        throw(strerror_buf);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(tcpport);

    /* Copy IP address in Inet address struct */
    memcpy((char *) &address.sin_addr, hp->h_addr, sizeof(address.sin_addr));
  
    /* Try to connect to the server */
    if (connect(sock,(struct sockaddr *)&address, sizeof(struct sockaddr_in))) {
        sprintf(strerror_buf,"connect: %s",strerror(errno));
        throw(strerror_buf);
    }
}

TCPCSocket::~TCPCSocket()
{
    close(sock);
    delete server;
}

size_t TCPCSocket::rx(unsigned char *buf, size_t maxlen)
{
  size_t nrxbytes = 0;
  //struct timeval timeout;
  //timeout.tv_sec = timeoutms / 1000;
  //timeout.tv_usec = ( timeoutms % 1000 ) * 1000;
 
  // Receive a datagram from a tx source
  do
  {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    // No time out
    select(sock + 1, &readfds, NULL, NULL, NULL);
    if FD_ISSET(sock, &readfds)
      nrxbytes = recvfrom(sock, buf, maxlen, 0, NULL, NULL);
      break;
  } while (1);
  return nrxbytes;
}

size_t TCPCSocket::rx_peek(unsigned char *buf, size_t maxlen)
{
  size_t nrxbytes = 0;
  //struct timeval timeout;
  //timeout.tv_sec = timeoutms / 1000;
  //timeout.tv_usec = ( timeoutms % 1000 ) * 1000;
 
  // Receive a datagram from a tx source
  do
  {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    // No time out
    select(sock + 1, &readfds, NULL, NULL, NULL);
    if FD_ISSET(sock, &readfds)
      nrxbytes = recvfrom(sock, buf, maxlen, MSG_PEEK, NULL, NULL);
      break;
  } while (1);
  return nrxbytes;
}

size_t TCPCSocket::tx(const unsigned char* mes, size_t len) throw(const char*)
{
  int ntxbytes = write(sock, mes, len);
  if (ntxbytes < 0) 
    throw "TCPCSocket::tx: could not send message";
  return ntxbytes;
}

