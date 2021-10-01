#ifndef CLIENT_H
#define CLIENT_H 1

#include <string>
#include "socket.h"
//#include "parse.h"

typedef struct {
    string cmd;
    string stat;
    char * result;
} server_response;


class Client : public TCPCSocket
{
private:
public:
Client(const char *host, int port) : TCPCSocket(host,port) {};

int command(const char *ctrl_cmd, const char *arg, server_response *response)
            throw(const char *);
bool find_response(const char* cmdname,
                   server_response *response) throw(const char *);
bool parse_server_response(const char *m, server_response *response,int len);
bool parse_ctrl_cmd(const char *ctrl_fullcmd, char *ctrl_cmd);

};
#endif
