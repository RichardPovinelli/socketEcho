//http://www.cygwin.com/ml/cygwin/2001-04/msg00549.html
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include "posixComDaemon.h"

int posixComOpen(char *, int, char, int, int);
char posixComRead(int, char *);

int main(int argc, char **argv) {
  char *device;
  int rate = -1;
  char parity = -1;
  int databits = -1;
  int stopbits = -1;
  int portID = -1;
  int result = 0;

  if (argc == 6) {
    device = argv[1];
    rate = convertRate(argv[2]);
    parity = convertParity(argv[3]);
    databits = convertDatabits(argv[4]);
    stopbits = convertStopbits(argv[5]);
    portID = posixComOpen(device, rate, parity, databits, stopbits);
    printf("%c", (char) portID); //send the portID back to PosixCom.java
    fflush(stdout);
    result = poll(portID);
  }//if
  else {
    printf("\nUsage: %s <port> <rate> <parity> <databits> <stopbits>\n", argv[0]);
    printf("<port> has the format '/dev/comX',where X = 0..8\n");
    result = -1;
  }
  return result;
} //main

/** Converts rate into proper values */
int convertRate(char* rateString) {
  int rate = atoi(rateString);
  switch (rate) {
    case 50: return B50;
    case 75: return B75;
    case 110: return B110;
    case 134: return B134;
    case 150: return B150;
    case 200: return B200;
    case 300: return B300;
    case 600: return B600;
    case 1200: return B1200;
    case 1800: return B1800;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    default:
      fprintf(stderr, "%d is an invalid baud rate.", rate);
      exit(-1); //exit with an error
  } //switch
  return -1;
} //convertRate

/** Converts databits into proper values */
int convertParity(char* parityString) {
  char parity = parityString[0];
  switch (parity) {
    case 'E': return PARENB;
    case 'O': return PARODD;
    case 'N': return 0;
    default:
      fprintf(stderr, "%c is an invalid parity.", parity);
      exit(-1); //exit with an error
  } //switch
  return -1;
} //convertParity

/** Converts databits into proper values */
int convertDatabits(char* databitsString) {
  int databits = atoi(databitsString);
  switch (databits) {
    case 5: return CS5;
    case 6: return CS6;
    case 7: return CS7;
    case 8: return CS8;
    default:
      fprintf(stderr, "%d is an invalid number of databits.", databits);
      exit(-1); //exit with an error
  } //switch
  return -1;
} //convertDatabits

/** Converts stopbits into proper values */
int convertStopbits(char* stopbitsString) {
  int stopbits = atoi(stopbitsString);
  switch (stopbits) {
    case 1: return 0;
    case 2: return CSTOPB;
    default:
      fprintf(stderr, "%d is an invalid number of stopbits.", stopbits);
      exit(-1); //exit with an error
  } //switch
  return -1;
} //convertStopbits

int posixComClose(int port) {
  int rtnval = 0;
  if (port != -1) {
    rtnval = (int) close(port);
  }

  return (rtnval);
}

/** Open up the com port. */
int posixComOpen(char *device, int rate, char parity, int databits,
        int stopbits) {

  int portID = -1;
  struct termios t;

  /* Open the com port for read/write access */
  portID = open(device, O_RDWR);

  /* Set the parity, data bits, and stop bits */
  t.c_iflag = IGNPAR;
  t.c_oflag = 0;
  t.c_cflag &= ~CSIZE; /* Zero out the CSIZE bits */
  t.c_cflag = CLOCAL | databits | stopbits | parity;
  t.c_lflag = IEXTEN;

  /* Set the data rate */
  if ((cfsetispeed(&t, rate) != -1) && (cfsetospeed(&t, rate) != -1)) {
    if (tcsetattr(portID, TCSANOW, &t) == -1)
      portID = -1;
  }//if
  else {
    portID = -1;
  } //else

  if (portID == -1) {
    fprintf(stderr, "Couldn't open port.", stopbits);
    exit(-1); //exit with an error
  } //if

  return (portID);
} //posixComOpen

/** Poll the com port and send to stdout. Poll the stdin and send to the com port. */
int poll(int portID) {
  int done = 0;
  char input = 0;
  int flags = 0;
  int fd = 0;
  int result = 0;

  fd = fileno(stdin);
  flags = fcntl(fd, F_GETFL, 0); //get control flags for stdin
  fcntl(fd, F_SETFL, flags | O_NONBLOCK); //make stdin nonblocking

  while (!done) {
    if (posixComDataReady(portID) && posixComRead(portID, &input)) {
      printf("%c", input);
      fflush(stdout);
    } //if

    result = getchar();
    if (result != -1) {
      posixComWrite(portID, (char) result);
    }
  } //while
  return 0;
}//poll

int posixComDataReady(int portID) {
  struct timeval tvptr;
  fd_set rset;
  int status = -1;
  int rtnval = 0;

  /* Block for 1 ms, so we don't just spin, could make proportional to 1/rate */
  tvptr.tv_sec = 0;
  tvptr.tv_usec = 1000;

  FD_ZERO(&rset); // Initialize the read set to zero
  FD_SET(portID, &rset); // And now turn on the read set
  status = select(portID + 1, &rset, NULL, NULL, &tvptr);

  if (status == -1) /* Error */ {
    fprintf(stderr, "Failed in checking if the port had data ready.");
    exit(-1); //exit with an error
    rtnval = 0;
  } else if (status > 0) {
    if (FD_ISSET(portID, &rset)) {
      rtnval = 1;
    }
  }
  return (rtnval);
} //posixComDataReady

char posixComRead(int port, char *input) {
  return (read(port, input, 1) == 1);
} //posixComRead

char posixComWrite(int port, char src) {
  return (write(port, &src, 1) == 1);
} //posixComWrite
