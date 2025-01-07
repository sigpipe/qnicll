

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include "qnac.h"
#include <unistd.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

typedef struct qnac_state_st {
  int fd;
} qnac_state_t;
static qnac_state_t st={0};


char cmd[256];
qnicll_set_err_fn *my_err_fn;

#define BUG(MSG) return(*my_err_fn)(MSG, QNICLL_ERR_BUG);


int set_attrib(int fd, int speed) {

    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
       // printf("Error from tcgetattr: %s\n", strerror(errno));
       return -1;
    }

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
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      //        printf("Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


int qnac_connect(char *devname, qnicll_set_err_fn *err_fn) {
  int e;
  // printf("open %s\n", devname);
  my_err_fn = err_fn;
  st.fd=open(devname, O_RDWR | O_NOCTTY | O_SYNC);
  if (st.fd<0) BUG("cant open");
  e = set_attrib(st.fd, 115200);
  if (e) BUG("cant set baud");
  return 0;
}

char *do_cmd(int fd, char *cmd) {
  return cmd;
}

int do_cmd_get_doub(char *cmd, double *d) {
  int n;
  double t;
  char *rsp;
  rsp = do_cmd(st.fd, cmd);
  n = sscanf(rsp,"%lg", &t);
  if (n!=1) BUG("bad double rsp from qnac");
  *d = t;
  return 0;
}

int qnac_set_txc_voa_atten_dB(double *atten_dB) {
  int e;
  double d;
  sprintf(cmd, "txc %.2f\n", *atten_dB);
  e = do_cmd_get_doub(cmd, &d);
  if (e) return e;
  *atten_dB = d;
  return 0;
}

int qnac_set_txq_voa_atten_dB(double *atten_dB) {
// desc: sets transmit quantum optical attenuation in units of dB.
// inputs: atten_dB: requested attenuation in units of dB.
// sets: atten_dB: set to the new actual effective attenuation.
// returns: one of QNICLL_ERR_* (after attenuation is achieved)
  return 0;  
}


int qnac_set_rx_voa_atten_dB(double *atten_dB) {
// desc: sets receiver attenuation (after the receiver EDFA).
// inputs: atten_dB: requested attenuation in units of dB.
// sets: atten_dB: set to the new actual effective attenuation.
  return 0;  
}

int qnac_disconnect(void){
  close(st.fd);
  return 0;  
}

int qnac_set_txsw(int setting) {
  sprintf(cmd, "txsw %d\n", !!setting);
  do_cmd(st.fd, cmd);  
}

int qnac_set_rxsw(int setting) {
  sprintf(cmd, "rxsw %d\n", !!setting);
  do_cmd(st.fd, cmd);  
}

