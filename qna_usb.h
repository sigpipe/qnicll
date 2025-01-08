#ifndef _QNA_USB_H_
#define _QNA_USB_H_

#include "qnicll_internal.h"

int qna_usb_connect(char *devname,  qnicll_set_err_fn *err_fn);
int qna_usb_do_cmd(char *cmd, char *rsp, int rsp_len);
int qna_usb_disconnect(void);
  
#endif
