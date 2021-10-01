//
//
// Copyright (c) 2012 L. Hiemstra
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <syslog.h>
#include <signal.h>

#include "serial.h"  
#include "client.h"


#define VERSION "v1.0"

using namespace std;
bool ThreadQuit=false;
Serial *serial;
Client *client=NULL;

string server_query(const char *cmd, const char *arg) throw(const char*)
{
    server_response response;
    int errors=0;
    if((errors=client->command(cmd,arg,&response)) ==0) {
        cout << response.result;
        return response.result;
    } else {
        string errstr=string("error: ");
        errstr.append(cmd);
        throw(errstr);
    }
}

string get_server_data(void)
{
    const char* host = "localhost";
    int port = 3340;
    int len;
    string s="empty";

    try {
        /*
        if(system("ssh -f -L 3339:localhost:3339 leon@de-schans.xs4all.nl sleep 10") < 0) {
            throw("ssh connect error");
        }
        */

        client = new Client(host,port);
        // read banner from server:
        unsigned char sbuf[1000];
        len = client->rx(sbuf, 1000);
        if (len<1) {
            delete client;
            throw("server is disconnected");
        } else {
            sbuf[len-1]=0; // remove endl
//            cout << "<banner>Connected to server: " << host
//                 << " at port: " << port
//                 << " banner: " << sbuf << "</banner>" << endl;

            server_query("open", "");
            //usleep(1000000);
            //try { server_query("read", "");
            //} catch(...) { /*cout << "ignoring error" << endl;*/ }
            //usleep(100000);


            server_query("write", "M");
            usleep(1000000);
            s=server_query("read", "") + "\r";

            usleep(1000000);

            server_query("write", "s");
            usleep(1000000);
            s = s + server_query("read", "") + "\r";

cout << "s=" << s << endl;
            /*
            usleep(1000000);
            server_query("write", "l");
            usleep(1000000);
            s = s + server_query("read", "") + "\r";
            */
            //server_query("close", "");

        }
        if(client) delete client;
    } catch(string str) {
         return str;
    }
    return s;
}

bool demmel_comm(const char *buf, const int len, bool in=false) throw(const char*)
{
    //printf("writing: ");
    //for(int i=0;i<len;i++) {
    //    printf("0x%x (%c)",buf[i],buf[i]);
    //}
    //printf("\n");

    if(serial->write(buf,len) <0) throw("error serial write");

    struct timeval to;
    to.tv_sec=1;
    to.tv_usec=0;

    if(serial->poll(&to)) {
        char bufin[100];
        int n;
        if((n=serial->read(bufin,sizeof(bufin))) <0) {
            if(n==-TO_ERR) { throw("timeout"); }
            else { throw("read"); }
        } else {
            //printf("reading: ");
            //for(int i=0;i<n;i++) {
            //   printf("0x%x (%c)",bufin[i],bufin[i]);
            //}
            //printf("\n");
        }
    }
    if(!in) return true;

    if(serial->poll(&to)) {
        char bufin[100];
        int n;
        if((n=serial->read(bufin,sizeof(bufin))) <0) {
            if(n==-TO_ERR) { throw("timeout"); }
            else { throw("read"); }
        } else {
            printf("reading again: ");
            for(int i=0;i<n;i++) {
               printf("0x%x (%c)",bufin[i],bufin[i]);
            }
            printf("\n");
        }
    }
    return true;
}

void process_SIGHUP (int sig)
{
//    syslog(LOG_WARNING,"SIGHUP Received, No actions at the moment\n");
    signal(SIGHUP,  process_SIGHUP);
}
void process_SIGPIPE(int sig)
{
//    syslog(LOG_WARNING,"SIGPIPE Received, No actions at the moment\n");
}
int init_daemon ()
{
    int status = 0;
    pid_t pid;

    signal(SIGHUP,  process_SIGHUP);
    signal(SIGPIPE, process_SIGPIPE); /* catches peer hangups */

    if ((pid = fork()) < 0) {
//        syslog(LOG_ERR,"fork error\n"); //,strerror(errno));
        status = 1; // Error
    } else {
        if (pid != 0) {
            exit(0); /* Parent goes bye-bye */

            /* Child continues */
            setsid();    /* Become session leader */
            chdir("/");  /* Change working directory */
            umask(0);    /* Clear out file mode creation mask */
        }
    }
    return status;
}


int main (int argc, char* argv[])
{
    char buf[1000];
    struct timeval t;
    struct tm *td = new(struct tm);
    int n;
    int tm_min_old=-1;
    int tm_minute_old=-1;
    char fn[100];
    string solarresult="empty";

    Serial_cfg ser_cfg;
    ser_cfg.icrnl = 0;
    ser_cfg.crtscts = 0;
    ser_cfg.xonxoff = 0;
    ser_cfg.block = 1;
    ser_cfg.sendbreak = 0;
    ser_cfg.baudrate = 115200;



    // init:
    try {
        serial = new Serial(0,&ser_cfg);

        buf[0]=0xAA; buf[1]='!';
        demmel_comm(buf, 2);
        //// set led
        //buf[1]='I'; buf[2]='L'; buf[3]='S'; buf[4]=0; buf[5]=2;
        //demmel_comm(buf, 6);
        //// blinking freq=1Hz
        //buf[1]='I'; buf[2]='L'; buf[3]='F'; buf[4]=50;
        //demmel_comm(buf, 5);
        // backlight intensity
        buf[1]='C'; buf[2]='I'; buf[3]=0; // 0..15
        demmel_comm(buf, 4);
    } catch(const char *str) {
         cerr << "thread_function throwed error: "
              << str << endl;
         exit(-1);
    }

#define RUN_AS_DAEMON
#ifdef RUN_AS_DAEMON
   // continue as a daemon:
   if(init_daemon () != 0) {
       exit(EXIT_FAILURE);
   }
#endif

    while(!ThreadQuit) {

        try {
            gettimeofday(&t,NULL);
            td = localtime(&t.tv_sec);

            if(td->tm_min != tm_minute_old) {
                tm_minute_old=td->tm_min;
                // clear display
                buf[1]='D'; buf[2]='E';
                demmel_comm(buf, 3);
                // goto position
                //buf[1]='C'; buf[2]='K'; buf[3]=0; buf[4]=0x4; buf[5]=0; buf[6]=0x32;
                buf[1]='C'; buf[2]='K'; buf[3]=0; buf[4]=0x4; buf[5]=0; buf[6]=0x2;
                demmel_comm(buf, 7);
                // set font
                buf[1]='A'; buf[2]='F'; buf[3]=0; buf[4]=7;
                demmel_comm(buf, 5);
                // set bold
                //buf[1]='A'; buf[2]='B'; buf[3]=1;
                //demmel_comm(buf, 4);
                // write text
                buf[1]='D'; buf[2]='T';
                sprintf(&buf[3],"%02d-%02d-%04d    %02d:%02d", //:%02d",
                        td->tm_mday,td->tm_mon+1,td->tm_year+1900,td->tm_hour,
                        td->tm_min); //,td->tm_sec);
                demmel_comm(buf, 3+strlen(&buf[3])+1);

                // set font
                buf[1]='A'; buf[2]='F'; buf[3]=0; buf[4]=0;
                demmel_comm(buf, 5);



                if(td->tm_min == tm_min_old || tm_min_old==-1) { 
                    // goto position
                    buf[1]='C'; buf[2]='K'; buf[3]=0; buf[4]=0x0; buf[5]=0; buf[6]=0x18;
                    demmel_comm(buf, 7);
                    // write text
                    buf[1]='D'; buf[2]='T';
                    sprintf(&buf[3],"connecting server...");
                    demmel_comm(buf, 3+strlen(&buf[3])+1);

                    tm_min_old=(td->tm_min+5)%60;
                    //cout << "get something new " << tm_min_old << " " << td->tm_min <<  endl;
                    solarresult=get_server_data();
                }

                // goto position
                buf[1]='C'; buf[2]='K'; buf[3]=0; buf[4]=0x0; buf[5]=0; buf[6]=0x18;
                demmel_comm(buf, 7);

                // write text
                buf[1]='D'; buf[2]='T';
                sprintf(&buf[3],"%s",solarresult.c_str());
                demmel_comm(buf, 3+strlen(&buf[3])+1);


                // read io
                //buf[1]='?'; buf[2]='I';
                //demmel_comm(buf, 3,true);
                //buf[1]='I'; buf[2]='?'; buf[3]='I';
                //demmel_comm(buf, 4,true);
            }
        } catch(const char *str) {
             cerr << "thread_function throwed error: "
                  << str << " trying restart" << endl;
        }

        usleep(25000000);
    }
}

