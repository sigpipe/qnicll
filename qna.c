//
// interface to QNA firmware
// (which runs on s750 board)
//
// Currently we connect through qna_usb,
// but someday maybe though PCI regs.

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include "qna.h"
#include "qna_usb.h"
#include <unistd.h>
//#include <termios.h>
//#include <unistd.h>
#include <errno.h>

typedef struct qna_state_st {
  int fd;
} qna_state_t;
static qna_state_t st={0};


char cmd[1024];
char rsp[1024];
qnicll_set_err_fn *set_errmsg_fn;

// return errors in a unified manner
static char errmsg[256];
static char errmsg_ll[256];

#define DO(FUNC)     \
  { int e;           \
    e = FUNC;        \
    if (e) return e; \
  }

#define BUG(MSG) return (*set_errmsg_fn)(MSG, QNICLL_ERR_BUG);





int qna_connect(char *devname, qnicll_set_err_fn *err_fn) {
  set_errmsg_fn = err_fn;
  return qna_usb_connect(devname, err_fn);
}

int do_cmd(char *cmd) {
  return qna_usb_do_cmd(cmd, rsp, 1024);
}

int do_cmd_get_int(char *cmd, int *i) {
  int e;
  int ii;
  DO(do_cmd(cmd));
  e = sscanf(rsp, "%d", &ii);
  if (e!=1) BUG("missing int in rsp");
  *i = ii;
  return 0;
  
}

int do_cmd_get_doub(char *cmd, double *d) {
  int e;
  double dd;
  DO(do_cmd(cmd));
  e = sscanf(rsp, "%lg", &dd);
  if (e!=1)
    return ((*set_errmsg_fn)("missing dbl in rsp", QNICLL_ERR_BUG));
  *d = dd;
  return 0;
}

int qna_set_txc_voa_atten_dB(double *atten_dB) {
// desc: sets transmit classical optical attenuation in units of dB.
  sprintf(cmd, "voa 1 %.2f\n", *atten_dB);
  return do_cmd_get_doub(cmd, atten_dB);
}

int qna_set_txq_voa_atten_dB(double *atten_dB) {
// desc: sets transmit quantum optical attenuation in units of dB.
  sprintf(cmd, "voa 2 %.2f\n", *atten_dB);
  return do_cmd_get_doub(cmd, atten_dB);
}

int qna_set_rx_voa_atten_dB(double *atten_dB) {
// desc: sets receiver attenuation (after the receiver EDFA).
  sprintf(cmd, "voa 3 %.2f\n", *atten_dB);
  return do_cmd_get_doub(cmd, atten_dB);
}

int qna_set_txsw(int *cross) {
  sprintf(cmd, "opsw 1 %d\n", *cross);
  return do_cmd_get_int(cmd, cross);
}

int qna_set_rxsw(int *cross) {
  sprintf(cmd, "opsw 2 %d\n", *cross);
  return do_cmd_get_int(cmd, cross);
}

int qna_set_rxefpc_basis(int *basis) {
  sprintf(cmd, "rxefpc %d\n", *basis);
  return do_cmd_get_int(cmd, basis);
}

int qna_disconnect(void){
  return qna_usb_disconnect();
}
