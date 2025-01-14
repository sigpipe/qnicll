
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

char errmsg[256];
void err(char *str) {
  printf("ERR: %s\n", str);
  printf("     %s\n", qnicll_error_desc());
  if (errno)
    printf("     errno %d = %s\n", errno, strerror(errno));
  exit(1);
}

void crash(int e, char *str) {
// desc: crashes wih informative messages
  printf("ERR: %d\n", e);
  printf("     %s\n", qnicll_error_desc());
  if (errno)
    printf("     errno %d = %s\n", errno, strerror(errno));  
  exit(1);
}

// check and crash if error
#define C(CALL)     {int e = CALL; if (e) crash(e, "");}
int main(void) {
  int num_dev, i, q, j, k, n, e, itr, d, t;
  char name[32], attr[32];
  ssize_t sz, tx_sz;
  const char *r;
  int samps, occ_samps;
  size_t left_sz;
  int tx_samps;
  void * adc_buf_p;
  FILE *fp;
  int fd;
  double x, y;
  double dd,fsamp_Hz;
  int tx_fmt, rx_fmt=QNICLL_DATA_FORMAT_LIBIIO;
  int fdmem;
  int *m;
  int cap_len_bufs; 
  long long int ll;
  int cap_len_samps, rx_samps, rx_status;
  int num_itr, b_i;
  time_t *times_s;
  int probe_qty=25;
  int probe_len_bits, probe_pd_samps;
  short int *tx_buf, *rx_buf;
  double sum;
  
  qnicll_settings_t set;

  int dbg_save=0;
  
  qnicll_init_info_libiio_t init_info;
  strcpy(init_info.ipaddr, "10.0.0.5");
  C(qnicll_init(&init_info));
  
  fsamp_Hz=0;
  C(qnicll_set_sample_rate_Hz(&fsamp_Hz)); // querry
  C(qnicll_get_settings(&set));

  C(qnicll_set_probe_datasrc(QNICLL_DATASRC_LFSR));
  i=0; C(qnicll_dbg_set_tx_0(&i));


  d=24;
  probe_pd_samps = (int)floor((d*1e-6)*fsamp_Hz/4)*4;
  C(qnicll_set_probe_pd_samps(&probe_pd_samps));
  i=probe_qty=10;
  qnicll_set_probe_qty(&probe_qty);
  if (probe_qty != i) {
    printf("ERR: probe qty actually %d\n", probe_qty);
    err("fail");
  }

  cap_len_samps = probe_pd_samps * probe_qty;
  rx_samps = cap_len_samps; // try to capture them all
  C(qnicll_set_rx_buf_sz_samps(&rx_samps));
  if (rx_samps != cap_len_samps) {
    printf("ERR: cant capture atomically\n");
    err("fail");
  }
  cap_len_bufs = (int)ceil((double)cap_len_samps / rx_samps); // is 1

  printf(" rx_samps %d = %.2f us\n", rx_samps, rx_samps/fsamp_Hz*1e6);
  
  if (dbg_save) {
    fd = open("out/d.raw", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXO);
    if (fd<0) err("cant open out/d.raw");
  }
  
  C(qnicll_txrx_en(1));
  
  for(b_i=0; b_i<cap_len_bufs; ++b_i) {
      C(qnicll_get_filled_rx_buf(&rx_buf, &occ_samps, &rx_status));
      // printf("DBG: refilled x%lx  %d\n", (size_t)rx_buf, occ_samps);
      if (cap_len_samps != occ_samps)
	printf("tried to refill %d but got %d samps\n",
	       cap_len_samps, occ_samps);

      sum=0;
      for(t=0;t<occ_samps;++t) {
	i = *(rx_buf+qnicll_idx_t2p(rx_fmt, 0, t)),
	q = *(rx_buf+qnicll_idx_t2p(rx_fmt, 1, t)),
	  // printf("%d\t%d\n", i, q);
	sum += i*i+q*q;
      }
      sum = sqrt(sum/occ_samps);
      printf("noise %.1lf ADCrms\n", sum);

      if (dbg_save) {
      left_sz = occ_samps * QNICLL_SAMP_SZ; // now in bytes
      while(left_sz > 0) {
        sz = write(fd, (void *)rx_buf, left_sz);
        if (sz<=0) err("write to file failed");
        if (sz >= left_sz) break;
	printf("WARN: tried to write %zd to file but wrote %zd bytes\n",
	       left_sz, sz);
	left_sz -= sz;
	rx_buf = (void *)((char *)rx_buf + sz);
      }
      }

      if (b_i==cap_len_bufs-1) break;
      
      C(qnicll_return_rx_buf());
  }

  C(qnicll_txrx_en(0));

  qnicll_done();

  if (dbg_save) {
  close(fd);


  
  fp = fopen("out/r.txt","w");
  //  fprintf(fp,"sfp_attn_dB = %d;\n",   sfp_attn_dB);
  fprintf(fp,"use_lfsr = %d;\n",  1);
  fprintf(fp,"tx_always = %d;\n", 0);
  //  fprintf(fp,"tx_0 = %d;\n",  st.tx_0);
  fprintf(fp,"probe_qty = %d;\n",    probe_qty);
  fprintf(fp,"probe_pd_samps = %d;\n", probe_pd_samps);
  
  fprintf(fp,"probe_len_bits = %d;\n", probe_len_bits);
  fprintf(fp,"data_hdr = 'i_adc q_adc';\n");
  fprintf(fp,"data_len_samps = %d;\n", cap_len_samps);
  fprintf(fp,"data_in_other_file = 2;\n");
  fprintf(fp,"num_itr = %d;\n", num_itr);
  fprintf(fp,"time = %d;\n", (int)time(0));
  //  fprintf(fp,"itr_times = [");
  //  for(i=0;i<num_itr;++i)
  //    fprintf(fp," %d", (int)(*(times_s+i)-*(times_s)));
  //  fprintf(fp, "];\n");
  
  fclose(fp);
  }
  return 0;
}
