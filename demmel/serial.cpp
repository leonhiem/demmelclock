/*
 * File:        serial.c
 * Author:      Leon Hiemstra
 * Date:
 * Description: Serial interface fucntions
 *
 */

#include <stdio.h> /* Standard input/output definitions */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h> /* String function definitions */
#include <unistd.h> /* UNIX standard function definitions */
#include <fcntl.h> /* File control definitions */
#include <errno.h> /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include "serial.h"

using namespace std;


Serial::Serial(int nr, Serial_cfg *c)  throw(const char*)
{
    int status = 0;
    int openflags;
    char dev_name[32];

    DTRstatus=0;

    devnr=nr;
    cfg=*c;
    sprintf(dev_name,"/dev/ttyAMA%d",devnr);
    //sprintf(dev_name,"/dev/ttyUSB%d",devnr);
    //sprintf(dev_name,"/dev/USBsolar");
    //printf("constructor Serial\n");


    fd_initialized = 0;

    /* Connect to serial device driver: */

    // O_NOCTTY flag: not "controlling terminal" mode
    // O_NDELAY flag: don't care about the state of the DCD signal line
    openflags = O_RDWR | O_NOCTTY;
    if(!cfg.block) openflags |= O_NONBLOCK;

    if((fd = open(dev_name, openflags)) < 0) {
        printf("Error opening %s reason: %s\n",dev_name,strerror(errno));
        throw("Error opening");
        status = -1;
    } else {
        unsigned int baudrate;

        struct termios options;

        if(cfg.block == 1) {
            fcntl(fd, F_SETFL, 0); // blocking IO
        } else {                                 // nonblocking
            //fcntl(fd[devnr], F_SETFL, FASYNC); 
            fcntl(fd, F_SETFL, FNDELAY); 
                                                 /* We don't use NDELAY option
                                                    but we use c_cc[VMIN] and
                                                    c_cc[VTIME] */
        }

        /*
         * Get the current options for the port...
         */
        if(tcgetattr(fd, &options) != 0)
            printf("Get serial attributes: %s\n",strerror(errno));

        /*
         * This is the configuration for the 480 GHZ scanner
         * (output of the `stty -a < /dev/ttyS0` command:
         *
         * speed 9600 baud; rows 56; columns 80; line = 0;
         * intr = ^C; quit = ^\; erase = ^?; kill = ^U; eof = ^D; eol = <undef>;
         * eol2 = <undef>; start = ^Q; stop = ^S; susp = ^Z; rprnt = ^R; 
         * werase = ^W;
         * lnext = ^V; flush = ^O; min = 0; time = 0;
         * -parenb -parodd cs8 hupcl -cstopb cread clocal crtscts
         * -ignbrk -brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr icrnl 
         * -ixon -ixoff
         * -iuclc -ixany -imaxbel
         * -opost -olcuc -ocrnl -onlcr -onocr -onlret -ofill -ofdel nl0 cr0 
         * tab0 bs0 vt0
         * ff0
         * -isig -icanon iexten -echo -echoe -echok -echonl -noflsh -xcase 
         * -tostop
         * -echoprt echoctl echoke
         *
         */

        switch(cfg.baudrate) {
            case 230400: baudrate = B230400; break;
            case 115200: baudrate = B115200; break;
            case 57600:  baudrate = B57600;  break;
            case 38400:  baudrate = B38400;  break;
            case 19200:  baudrate = B19200;  break;
            case 9600:   baudrate = B9600;  break;
            case 4800:   baudrate = B4800;  break;
            case 2400:   baudrate = B2400;  break;
            case 1200:   baudrate = B1200;  break;
            case 300:    baudrate = B300;   break;
            default:     baudrate = B9600;  break;
        }
        /* Set the baud rates */
        cfsetispeed(&options, baudrate);
        cfsetospeed(&options, baudrate);

        /**** C O N T R O L   O P T I O N S ****/
        /* Set the dataformat (8 databits, no parity, 1 stopbit (8N1)) */
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSIZE;  /* First zero out the bits */
        options.c_cflag |= CS8;     /* Set 8 bits/element */
        options.c_cflag &= ~CSTOPB; /* Not 2 stopbits but 1 */
        options.c_cflag &= ~PARODD;
        options.c_cflag |= HUPCL;   /* Hangup on last close */

        /* Enable the receiver and set local mode */
        options.c_cflag |= (CLOCAL | CREAD);

        /* hardware flow control */
        if(cfg.crtscts)
            options.c_cflag |= CRTSCTS; 
        else
            options.c_cflag &= ~(CRTSCTS); 

         /**** I N P U T   O P T I O N S ****/
         /* Disable break conditions, disable parity */
         options.c_iflag &= ~(IGNBRK | BRKINT | IGNPAR | PARMRK | INPCK);

         /* Map CR to NL only */
         options.c_iflag &= ~(ISTRIP | INLCR | IGNCR);
         if(cfg.icrnl)
             options.c_iflag |= (ICRNL);
         else
             options.c_iflag &= ~(ICRNL);

         /* software flow control */
         options.c_iflag &= ~IXANY;
         if(cfg.xonxoff)
             options.c_iflag |= (IXON | IXOFF);
         else
             options.c_iflag &= ~(IXON | IXOFF);


         /**** O U T P U T   O P T I O N S ****/
         /* Raw output */
         options.c_oflag &= ~(OPOST | OCRNL | ONLCR | ONOCR | ONLRET);
         options.c_oflag &= ~(OFILL | OFDEL);


         /**** L O C A L   O P T I O N S ****/
         /* Choosing Raw Input */
         options.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHONL);
         options.c_lflag &= ~(NOFLSH | XCASE | TOSTOP | ECHOPRT);

         //options.c_lflag |= (IEXTEN | ECHOCTL | ECHOKE);
         options.c_lflag &= ~(IEXTEN | ECHOCTL | ECHOKE);


         /* Setting Read Timeouts: */

         /* VMIN specifies the minimum number of characters to read */
         /* If VMIN=0, VTIME serves as a timeout value */
//         if(cfg.block == 1) {
//             options.c_cc[VMIN]=1; /* blocking IO */
//         } else {
//             options.c_cc[VMIN]=0; /* nonblocking IO */
//         }

         /* Time to wait for data (tenths of seconds) */
         /* VTIME is set to 0: wait 0 sec. */
         options.c_cc[VTIME]=1;

         options.c_line = N_TTY;  /* set line discipline  */

         /*
          * Flush input/output queue:
          */
         tcflush(fd, TCIOFLUSH);

         /*
          * Set the new options for the port...
          */
         if(tcsetattr(fd, TCSANOW, &options) != 0)
              printf("Set serial attributes: %s\n",strerror(errno));

         if(cfg.sendbreak) {
             /* Will send a break for 0.25-0.5 sec */
             tcsendbreak(fd,1); 
         }
         fd_initialized = 1;
    }
}

Serial::~Serial()
{
    printf("destructor Serial\n");
    if(fd_initialized) {
        close(fd);
    }
}

int Serial::write(const char *buf, int len)
{
    int n=0;

//#define SERIAL_FRAGMENTED 1
//#define SERIAL_FRAGMENTED_WITHDELAY 100000

#ifdef SERIAL_FRAGMENTED
    {
        int i;
        for(i=0;i<len;i++) {
            if((n = ::write(fd, &buf[i], 1)) < 0) {
                print_err(ID,"Write error: %s\n",strerror(errno));
                return -1;
            }
#ifdef SERIAL_FRAGMENTED_WITHDELAY
            usleep(SERIAL_FRAGMENTED_WITHDELAY);
#endif
        }
    }
#else
    if((n = ::write(fd, buf, len)) < 0) {
        printf("Write error: %s\n",strerror(errno));
        return -1;
    }
#endif

    return n;
}

int Serial::writef(const char *str, ...)
{
    int status=0;
    va_list ap;
    char *cmd = (char *)malloc((strlen(str)+1));

    if(cmd == NULL) 
    { 
        status = -MA_ERR; 
        printf("Malloc error\n");
    }
    else
    {
        va_start(ap, str);
        vsprintf(cmd,str,ap);
        va_end(ap);
        if(write((const char *)cmd,strlen(cmd)) < 0) status=-WR_ERR;
        free(cmd);
    }
    return status;
}

int Serial::read(char *buf, int len)
{
    int n;

    if((n = ::read(fd, (void *)buf, len)) < 0) {
        printf("Read error: %s\n",strerror(errno));
        return -1;
    }
    return n;
}

int Serial::read_response(char *buf, int len, int to)
{
    int status = 0;
    int read_bytes;

    struct timeval tv;
    unsigned long timeout,newtime;
    gettimeofday(&tv,NULL);
    timeout = tv.tv_sec + to;
    newtime = tv.tv_sec;

    read_bytes=0;
    while((newtime < timeout) && (read_bytes < len))
    {
        int ret;
        gettimeofday(&tv,NULL);
        newtime = tv.tv_sec;

        ret=read(&buf[read_bytes],(len-read_bytes));
        if(ret < 0) status = -RD_ERR;
        else read_bytes += ret;

        usleep(10);
    }
    if(newtime >= timeout) 
    {
        printf("Timeout error\n");
        status = -TO_ERR;
    }
    else
        status = read_bytes;

    return status;
}

int Serial::read_response_to_eos(char *buf, int maxlen, int to, const char eos)
{
    int status = 0;
    int read_bytes;
    char endc = 0;

    struct timeval tv;
    unsigned long timeout,newtime;
    gettimeofday(&tv,NULL);
    timeout = tv.tv_sec + to;
    newtime = tv.tv_sec;


    read_bytes=0;
    while((newtime < timeout) && (read_bytes < maxlen) && 
          (endc != eos) && (status != -RD_ERR))
    {
        int ret;
        int i;
        gettimeofday(&tv,NULL);
        newtime = tv.tv_sec;
  
        ret=read(&buf[read_bytes],(maxlen-read_bytes));
  
        if(ret < 0) 
            status = -RD_ERR;
        else 
        {
            for(i=0;(i<ret) && (endc!=eos);i++)
            {
                if(buf[i+read_bytes] == eos)
                {
                    endc = eos;
                    read_bytes += i+1;
                }
            }
            if(endc != eos) read_bytes += ret;
        }
        usleep(10);
    }
    if(newtime >= timeout) 
    {
        printf("Timeout error\n");
        status = -TO_ERR;
    }
    else
        status = read_bytes;

    return status;
}

int Serial::read_n(char *buf, int len)
{
    int read_bytes;

    read_bytes=0;
    while(read_bytes < len) {
        int ret;

        ret=read(&buf[read_bytes],(len-read_bytes));
        if(ret < 0) return -RD_ERR;
        else read_bytes += ret;
    }
    return read_bytes;
}

int Serial::read_n_synced(char *buf, int len, unsigned char sync)
{
    int read_bytes;
    char firstb=0;


    while(firstb != sync) {
        int ret;

        ret=read(&firstb,1);
        if(ret < 0) return -RD_ERR;
    }
    buf[0] = firstb;

    read_bytes=1;
    while(read_bytes < len) {
        int ret;

        ret=read(&buf[read_bytes],(len-read_bytes));
        if(ret < 0) return -RD_ERR;
        else read_bytes += ret;
    }
    return read_bytes;
}

int Serial::set_DTR(int val)
{
    int status;
    if(ioctl(fd, TIOCMGET, &status) < 0) {
        return -1;
    }
    DTRstatus=val;
    if(val == 0) {
        status &= ~TIOCM_DTR;
    } else {
        status |= TIOCM_DTR;
    }
    if(ioctl(fd, TIOCMSET, &status) < 0) {
        printf("Ioctl, reason: %s\n",strerror(errno));
        return -1;
    }
    return 0;
}

int Serial::set_RTS(int val)
{
    int status;
    if(ioctl(fd, TIOCMGET, &status) < 0) {
        printf("Ioctl, reason: %s\n",strerror(errno));
        return -1;
    }
    if(val == 0) {
        status &= ~TIOCM_RTS;
    } else {
        status |= TIOCM_RTS;
    }
    if(ioctl(fd, TIOCMSET, &status) < 0) {
        printf("Ioctl, reason: %s\n",strerror(errno));
        return -1;
    }
    return 0;
}

int Serial::poll(struct timeval *timeout)
{
    fd_set rset, workrset;   /* file descriptor read-sets for select() */
                             /*   rset     = current set */
                             /*   workrset = workset, modified by select() */
    int selrc;
    int status=0;

    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    workrset=rset;

    selrc=select(fd + 1, &workrset, NULL, NULL, timeout);
 
    if(selrc > 0 && FD_ISSET(fd, &workrset)) {
        status = 1; /* data available */
    } else if(selrc == 0) { /* timeout */
        status = 0; /* no data */
    }
    return status;
}

void Serial::flush(void)
{
    /*
     * Flush input/output queue:
     */
    tcflush(fd, TCIOFLUSH);
}

