

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "qna_usb.h"
#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "qnicll.h"

int fd;
qnicll_set_err_fn *my_set_errmsg_fn;


#define RBUG(MSG) return (*my_set_errmsg_fn)(MSG, QNICLL_ERR_BUG);


static int set_attrib(int fd, int speed) {

    struct termios tty;

    if (tcgetattr(fd, &tty) < 0)
      RBUG("tcgetattr");

    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VMIN]  =  1;
    tty.c_cc[VTIME] = .01;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
      RBUG("Error from tcsetattr");

    return 0;
}

int qna_usb_connect(char *devname,  qnicll_set_err_fn *set_errmsg_fn) {
  my_set_errmsg_fn = set_errmsg_fn;
  fd=open(devname, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd<0) RBUG("cant open");
  if (set_attrib(fd, 115200)) RBUG("set serport attrib");
  return 0;
}

char ibuf[1024];

int qna_usb_do_cmd(char *cmd, char *rsp, int rsp_len) {
  ssize_t n;
  int p_len;
  char *p;
  n = write(fd, cmd, strlen(cmd));
  if (n<strlen(cmd)) RBUG("write cmd failed");
  n = write(fd,"\n", 1);
  if (n<0) RBUG("write CR failed");

  p     = rsp;
  p_len = rsp_len-1;
  while(1) {
    n = read(fd, p, 1023);
    if (n<0) RBUG("read error");
    p[n]=0;
    if (strstr(p,">")) return 0;
    p     += n;
    p_len -= n;
  }
  return 0;
}

int qna_usb_disconnect(void) {
  close(fd);
}
