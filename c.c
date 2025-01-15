#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include "qregc.h"

#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "qnicll.h"
#include "util.h"

#define BLEN 1024


void crash(int e, char *str) {
// desc: crashes wih informative messages
  printf("ERR: errcode %d\n", e);
  printf("     %s\n", qnicll_error_desc());
  if (errno)
    printf("     errno %d = %s\n", errno, strerror(errno));  
  exit(1);
}

// check and crash if error
#define C(CALL)     {int e = CALL; if (e) crash(e, "");}


int main(int argc, char *argv[]) {
  int i;
  char cmd[BLEN], *ipaddr, rsp[BLEN];
  cmd[0]=0;
  
  if (argc>1) {

    
    for (i=1;i<argc;++i) {

      
      strcat(cmd, argv[i]);
      if (i>=argc-1) break;
      strcat(cmd, " ");
    }
    // printf("%s\n", cmd);
  }
  ipaddr = util_read_ipaddr_dot_txt();

  qnicll_init_info_libiio_t init_info;
  strcpy(init_info.ipaddr, ipaddr);
  C(qnicll_init(&init_info));

  if (!strcmp(argv[1],"shutdown")) {
    C(qnicll_dbg_shutdown());
    return 0;
  }
  
  C(qnicll_do_qna_cmd(cmd, rsp, BLEN));

  util_print_all(rsp);
  qnicll_done();
}
