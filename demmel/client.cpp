#include <string.h>
#include <strings.h>
#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <syslog.h>

//#include <boost/numeric/ublas/matrix.hpp>
//#include <boost/regex.hpp>
//#include <boost/iostreams/stream.hpp>
//#include <boost/iostreams/device/file_descriptor.hpp>

#include "client.h"

using namespace std;
//using namespace boost;


bool Client::find_response(const char* cmdname,
                           server_response *response) throw(const char *)
{
    unsigned char buf[60000];
    int len;
    bool response_found=false;

    while(!response_found) {

        int new_cmd, can_read;
        new_cmd = can_read = 0;
        while(!new_cmd) {
            len = rx_peek(buf, sizeof(buf));
            if (len<1) {
                throw(" server is disconnected");
            } else {
                for(int i=0;i<len;++i) {
                    if(buf[i]=='\n') {
                        can_read=i+1;
                        new_cmd=1;
                        break;
                    }
                }
           }
        }
        len = rx(buf, can_read);
        if (len<1) {
            throw(" server is disconnected");
        } else {
            buf[len]=0; // terminate string

            if(parse_server_response((const char*)buf,response,len)) {
                if(strcmp(cmdname,response->cmd.c_str())==0) {
                    response_found=true;
                }
            } else throw("Client: error parse_server_response");
        }
    }
   return true;
}

int Client::command(const char *ctrl_cmd, const char *arg,
                    server_response *response)
                    throw(const char *)
{
    int errors=0;
    ostringstream strs;

    if(arg!=NULL) {
        strs << ctrl_cmd << "=" << arg << endl;
    } else {
        strs << ctrl_cmd << endl;
    }
    try {
        tx((const unsigned char*)strs.str().c_str(),
           strlen(strs.str().c_str()));
    }
    catch (const char *str) {
        throw("server TX error");
    }

    find_response(ctrl_cmd, response);
    if(strcmp(response->stat.c_str(),"OK") != 0) {
        errors++;
        strs.clear(); strs.str("");
        strs << "command failed: status=" << response->stat << " result=" << response->result;
        throw(strs.str().c_str());
    }
    return errors;
}

bool Client::parse_server_response(const char *m, server_response *response, int maxlen)
{
/*
        regex re;
        cmatch matches;

#define EXPR_CMDLIST "(sample|currentfile|getstatus|suninfo|solarinfo|yield|dir|plot|setmode)"
#define REGEXP_SERVER_RESPONSE "^" EXPR_CMDLIST "\\s(OK|Error)\\s(.*$)"

        re = string(REGEXP_SERVER_RESPONSE);

        if (boost::regex_match(m, matches, re)) {
            // matches[0] contains the original string.  matches[n]
            // contains a sub_match object for each matching
            // subexpression
*/
            istringstream l(m);
            l >> response->cmd;
            l >> response->stat;
            response->result = new char[maxlen];
            l.getline(response->result,maxlen);
            return true;
/*
        } else {
            return false;
        }
*/
}

