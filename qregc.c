//
//  qregc.c
//  client code to talk to qreg demon
//

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
//#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "qregc.h"

char rxbuf[1024];



// char errmsg[256];

//int min(int a, int b) {
//  return (a<b)?a:b;
//};


int soc;
qnicll_set_err_fn *my_err_fn;
#define BUG(MSG) return (*my_err_fn)(MSG, QNICLL_ERR_BUG);




int rd_pkt(char *buf, int buf_sz) {
  char len_buf[4];
  int l;
  ssize_t sz;
  sz = read(soc, (void *)len_buf, 4);
  if (sz<4) BUG("read hdr fail");
  l = (int)ntohl(*(uint32_t *)len_buf);
  l = MIN(buf_sz, l);
  // printf("will read %d\n", l);
  sz = read(soc, (void *)buf, l);
  if (sz<l) BUG("read body fail");
  return sz;
}

int wr_pkt(char *buf, int buf_sz) {
  char len_buf[4];
  uint32_t l;
  ssize_t sz;
  l = htonl(buf_sz);
  sz = write(soc, (void *)&l, 4);
  if (sz!=4)
    BUG("write len fail");
  // printf("will write %d bytes\n", buf_sz);
  sz = write(soc, (void *)buf, buf_sz);
  if (sz!=buf_sz) BUG("write buf fail");
  return 0;
}

char buf[64];

int wr_str(char *buf) {
  int sz = strlen(buf);
  return wr_pkt(buf, sz);
}


int rd_int(int *v) {
  int n, e, i, l = rd_pkt(rxbuf, 1023);
  rxbuf[l]=0;
  n = sscanf(rxbuf, "%d %d", &e, &i);
  if (n!=2) BUG("no int rsp");
  *v=i;
  return e;
}

int qregc_connect(char *hostname, qnicll_set_err_fn *err_fn) {
// err_fn: function to use to report errors.
  int e, l;
  char rx[16];
  struct sockaddr_in srvr_addr;

  my_err_fn = err_fn;
  soc = socket(AF_INET, SOCK_STREAM, 0);
  if (soc<0) BUG("cant make socket to listen on ");
  
  memset((void *)&srvr_addr, 0, sizeof(srvr_addr));

  srvr_addr.sin_family = AF_INET;
  srvr_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  srvr_addr.sin_port = htons(5000);

  e=inet_pton(AF_INET, hostname, &srvr_addr.sin_addr);
  //  e=inet_pton(AF_INET, "10.0.0.5", &srvr_addr.sin_addr);
  if (e<0) BUG("cant convert ip addr");

  e = connect(soc, (struct sockaddr *)&srvr_addr, sizeof(srvr_addr));
  if (e<0) BUG("connect failed");
  
  return 0;
}


  


int qregc_set_probe_pd_samps(int *pd_samps) {
  sprintf(buf, "probe_pd %d", *pd_samps);
  wr_str(buf);
  return rd_int(pd_samps);
}

int qregc_set_probe_len_bits(int *probe_len_bits) {
  int e;
  sprintf(buf, "probe_len %d", *probe_len_bits);
  e = wr_str(buf);
  if (e) return e;
  return rd_int(probe_len_bits);
}

int qregc_set_probe_qty(int *probe_qty) {
  int e;
  sprintf(buf, "probe_qty %d", *probe_qty);
  e = wr_str(buf);
  if (e) return e;
  return rd_int(probe_qty);
}

int qregc_tx(int *en) {
  sprintf(buf, "tx %d", *en);
  wr_str(buf);
  return rd_int(en);
}

int qregc_set_use_lfsr(int *use) {
  sprintf(buf, "lfsr_en %d", *use);
  wr_str(buf);
  return rd_int(use);
}

int qregc_set_tx_always(int *en) {
  sprintf(buf, "tx_always %d", *en);
  wr_str(buf);
  return rd_int(en);
}

int qregc_set_tx_0(int *en) {
  sprintf(buf, "tx_0 %d", *en);
  wr_str(buf);
  return rd_int(en);
}

int qregc_disconnect(void) {
  wr_str("q");
  close(soc);
}

