

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "qna_usb.h"
#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include "qnicll.h"
#include "util.h"

#define IBUF_LEN (1024)
char ibuf[IBUF_LEN];

int fd;
qnicll_set_err_fn *my_set_errmsg_fn;

char umsg[1024];
#define RBUG(MSG) return (*my_set_errmsg_fn)(MSG, QNICLL_ERR_BUG);

#define DO(CALL) {int e=CALL; if (e) return e;}

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


    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN | ECHOE | ECHOK | ECHONL | ECHOCTL );
    tty.c_lflag |= ICANON;
    tty.c_oflag &= ~OPOST;

    /* fetch bytes as they become available */
    tty.c_cc[VEOL] = '>'; // end of line char
    tty.c_cc[VMIN]  =  1;
    tty.c_cc[VTIME] = .01;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
      RBUG("Error from tcsetattr");

    return 0;
}

int dbg=1;

int wr(char *str) {
  size_t l;
  ssize_t n;
  l=strlen(str);
  if (dbg) {
    printf("qna_usb w: ");
    util_print_all(str);
    printf("\n");
  }
  n = write(fd, str, l);
  if (n<0) RBUG("write failed");
  if (n!=l) {
    sprintf(umsg, "did not write %zd chars", l);
    RBUG(umsg);
  }
  return 0;  
}

int rd_ibuf(void) {
  ssize_t sz;
  if (dbg) {
    printf("read\n");
    printf("qna_usb r: ");
    fsync(0);
  }
  sz =read(fd, ibuf, IBUF_LEN-1);
  if (sz<0) RBUG("read from qna usb failed");
  ibuf[(int)sz]=0;
  if (dbg) {
    util_print_all(ibuf);
    printf("\n");
  }
  return 0;
}

int qna_usb_connect(char *devname,  qnicll_set_err_fn *set_errmsg_fn) {
  my_set_errmsg_fn = set_errmsg_fn;
  fd=open(devname, O_RDWR | O_NOCTTY | O_SYNC);
  if (fd<0) {
    sprintf(umsg, "cant open usb device %s", devname);
    RBUG(umsg);
  }
  if (set_attrib(fd, 115200))
    RBUG("cant set serial usb port attributes");
  
  DO(wr("i\r"));
  DO(rd_ibuf());
  printf("got rsp %s\n", ibuf);
  return 0;
}



int qna_usb_do_cmd(char *cmd, char *rsp, int rsp_len) {
  ssize_t n;
  int p_len;
  char *p;
  DO(wr(cmd));
  DO(wr("\r"));

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
