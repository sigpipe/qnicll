
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include "qregc.h"
#include <iio.h>
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


/*
void set_blocking_mode(struct iio_buffer *buf, int en) {
  int i;
  i=iio_buffer_set_blocking_mode(buf, en); // default is blocking.
  if (i) {
    printf("set block ret %d\n", i);
    err("cant set blocking");
  }
}
*/

#define NSAMP (1024)

/*
void chan_en(struct iio_channel *ch) {
  iio_channel_enable(ch);
  if (!iio_channel_is_enabled(ch))
    err("chan not enabled");
}
*/
#define DAC_N (256)
#define SYMLEN (4)
#define PAT_LEN (DAC_N/SYMLEN)
#define ADC_N (1024*8*4)



void delme_pause(char *prompt) {
  char buf[256];
  if (prompt && prompt[0])
    printf("%s > ", prompt);
  else
    printf("hit enter > ");
  scanf("%[^\n]", buf);
  getchar();
}

double ask_num(char *prompt, double dflt) {
  char buf[32];
  int n;
  double v;
  printf("%s (%g)> ", prompt, dflt);
  n=scanf("%[^\n]", buf);
  getchar();
  if (n==1)
    n=sscanf(buf, "%lg", &v);
  if (n!=1) v=dflt;
  return v;
}



int main(void) {
  int num_dev, i, j, k, n, e, itr;
  char name[32], attr[32];
  int pat[PAT_LEN] = {1,1,1,1,0,0,0,0,1,0,1,0,0,1,0,1,
       1,0,1,0,1,1,0,0,1,0,1,0,0,1,0,1,
       1,0,1,0,1,1,0,0,1,0,1,0,0,1,0,1,
       0,1,0,1,0,0,1,1,0,1,0,1,1,0,1,0};
  ssize_t sz, tx_sz;
  const char *r;
  struct iio_context *ctx=0;
  struct iio_device *dac, *adc;
  struct iio_channel *dac_ch0, *dac_ch1, *adc_ch0, *adc_ch1;
  struct iio_buffer *dac_buf, *adc_buf;
  int samps, occ_samps;

  size_t left_sz;
  
  int tx_samps;
  void * adc_buf_p;
  FILE *fp;
  int fd;
  double x, y;
  double dd,fsamp_Hz;

  int tx_fmt;
  
  short int rx_mem[ADC_N];
  short int corr[ADC_N];
  int fdmem;
  int *m;
  int cap_len_bufs; 
  long long int ll;
  int use_lfsr=1;
  int tx_always, tx_0;


  int cap_len_samps, rx_samps, rx_status;
  int num_itr, b_i;
  time_t *times_s;
  
  int probe_qty=25;
  int probe_len_bits, probe_pd_samps;

  short int *tx_buf, *rx_buf;
  


  qnicll_init_info_libiio_t init_info;
  strcpy(init_info.ipaddr, "10.0.0.5");
  //  strcpy(init_info.usbdev, "/dev/ttyUSB0");
  C(qnicll_init(&init_info));

  fsamp_Hz=0;
  C(qnicll_set_sample_rate_Hz(&fsamp_Hz)); // querry

  dd=999;
  C(qnicll_set_txc_voa_atten_dB(&dd));
  printf("classical voa %g\n", dd);
  dd=0;
  C(qnicll_set_txq_voa_atten_dB(&dd));
  printf("quantum voa %g\n", dd);
  i=100;
  //  C(qnicll_set_fpc_wp_dac(1, &i));
  //  printf("wp is %d\n", i);
#if 0  
  int probe_mod = QNICLL_MODULATION_IM;
  int osamp = 4;
  C(qnicll_set_probe_tx_modulation(&probe_mod, &osamp));
  
  use_lfsr = (int)ask_num("use_lfsr", 1);
  C(qnicll_set_probe_datasrc(use_lfsr?QNICLL_DATASRC_LFSR:QNICLL_DATASRC_CUSTOM));
  if (!use_lfsr) {
    int b; // bit
    int t; // temporal idx
    tx_samps = PAT_LEN * osamp;
    C(qnicll_set_tx_buf_sz_samps(&tx_samps));
    if (tx_samps != PAT_LEN * osamp)
      printf("WARN: wanted %d samps but using %d\n",
	     PAT_LEN * osamp, tx_samps);
    C(qnicll_get_free_tx_buf(&tx_buf));
    
    tx_fmt = 0;
    C(qnicll_set_probe_tx_format(&tx_fmt)); // querry
    // should work regardless of fmt
    for(t=b=0; (b<PAT_LEN)&&(t<tx_samps); ++b) {
      n=(pat[b]*2-1) * (32768/2);
      for(k=0; (k<osamp)&&(t<tx_samps); ++k)
        tx_buf[qnicll_idx_t2p(tx_fmt, 0, t++)] = n;
      // hardware does not replicate data to implement oversampling
    }
    C(qnicll_push_tx_buf()); // push data to fifo in FPGA
    // however DAC does not get this data yet.
  }

  // debug stuff
  tx_always = (int)ask_num("tx_always", 0);
  C(qnicll_dbg_set_tx_always(&tx_always));
  tx_0 = (int)ask_num("tx_0", 0);
  C(qnicll_dbg_set_tx_0(&tx_0));


  int d;
  // d = 2;
  d=24;
  d = ask_num("probe_pd (us)", d);
  i = (int)floor((d*1e-6)*fsamp_Hz/4)*4;
  C(qnicll_set_probe_pd_samps(&i));
  printf("probe_pd_samps %d\n", i);
  probe_pd_samps=i;


  probe_qty = (int)floor(ADC_N/probe_pd_samps);
  printf("max probes %d\n", probe_qty);
  probe_qty=100;
  i=probe_qty = ask_num("probe_qty", probe_qty);
  qnicll_set_probe_qty(&probe_qty);
  if (probe_qty != i) {
    printf("ERR: probe qty actually %d\n", probe_qty);
    err("fail");
  }

  cap_len_samps = probe_pd_samps * probe_qty;
  rx_samps = cap_len_samps; // try to capture them all
  C(qnicll_set_rx_buf_sz_samps(&rx_samps));
  cap_len_bufs = (int)ceil((double)cap_len_samps / rx_samps);

  printf(" rx_samps %d\n", rx_samps);
  printf(" and num bufs %d\n", cap_len_bufs);


   

  //  probe_qty = (int)ceil(cap_len_samps / probe_pd_samps);
  //  cap_len_samps = probe_qty * probe_pd_samps;
  //  printf(" cap_len_samps %d\n", cap_len_samps);
    
      //    printf("cap_len_samps %d must be < %d \n", cap_len_samps, ADC_N);
      //    printf("WARN: must increase buf size\n");

  

   
  //  printf("using probe_qty %d\n", probe_qty);
    
  
  i=128;  
  i = ask_num("probe_len_bits", i);
  qnicll_set_probe_len_bits(&i);
  printf("probe_len_bits %d\n", i);
  probe_len_bits = i;
  
  num_itr=1;
  // num_itr = ask_num("num_itr", 1);
  times_s = (time_t *)malloc(num_itr*sizeof(time_t));
  





  // sys/module/industrialio_buffer_dma/paraneters/max_bloxk_size is 16777216
  // iio_device_set_kernel_buffers_count(?
  //  i = iio_device_get_kernel_buffers_count(dev);
  //  printf("num k bufs %d\n", i);
  
  

  

  //  i=iio_buffer_set_blocking_mode(dac_buf, true); // default is blocking.
  //  if (i) err("cant set blocking");



  fd = open("out/d.raw", O_CREAT | O_WRONLY | O_TRUNC, S_IRWXU | S_IRWXO);
  if (fd<0) err("cant open out/d.raw");
  
  for (itr=0;itr<1;++itr) {

    printf("itr %d\n", itr);

    *(times_s + itr) = time(0);
    
    // cause transmission to DAC simultaneously
    // with samples rxed from ADC
    C(qnicll_txrx_en(1));
  
    for(b_i=0; b_i<cap_len_bufs; ++b_i) {
      printf("ref s\n");
      C(qnicll_get_filled_rx_buf(&rx_buf, &occ_samps, &rx_status));
      // printf("DBG: refilled x%lx  %d\n", (size_t)rx_buf, occ_samps);
      if (cap_len_samps != occ_samps)
	printf("tried to refill %d but got %d samps\n",
	       cap_len_samps, occ_samps);

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
      C(qnicll_return_rx_buf());
      
      // printf("wrote %zd\n", sz);
      // dma req will go low.
      
    }

    C(qnicll_txrx_en(0));
    
    //    prompt("end loop prompt ");
    //    myiio_print_adc_status();
    
    // printf("adc buf p x%x\n", adc_buf_p);

    /*
    // calls convert and copies data from buffer
    sz = cap_len_samps*2;
    sz_rx = iio_channel_read(adc_ch0, adc_buf, rx_mem_i, sz);
    if (sz_rx != sz)
      printf("ERR: read %zd bytes from buf but expected %zd\n", sz_rx, sz);

    sz_rx = iio_channel_read(adc_ch1, adc_buf, rx_mem_q, sz);
    if (sz_rx != sz)
      printf("ERR: read %zd bytes from buf but expected %zd\n", sz_rx, sz);
    for(i=0; i<4; ++i) {
      printf("%d %d\n",  rx_mem_i[i], rx_mem_q[i]);
    }
    */


    

    //    for(i=1; i<ADC_N; ++i)
    //      rx_mem[i] = (int)sqrt((double)rx_mem_i[i]*rx_mem_i[i] + (double)rx_mem_q[i]*rx_mem_q[i]);

    
    // display rx 
    /*    for(i=1; i<ADC_N; ++i)
      if (rx_mem[i]-rx_mem[i-1] > 100) break;
    if (i>=ADC_N)
      printf("WARN: no signal\n");
    else
      printf("signal at idx %d\n", i);
    
    y = rx_mem[i];
    x = (double)i/1233333333*1e9;
    */

  }
  close(fd);

  qnicll_done();
  
  fp = fopen("out/r.txt","w");
  //  fprintf(fp,"sfp_attn_dB = %d;\n",   sfp_attn_dB);
  fprintf(fp,"use_lfsr = %d;\n",   use_lfsr);
  fprintf(fp,"tx_always = %d;\n",  tx_always);
  //  fprintf(fp,"tx_0 = %d;\n",  st.tx_0);
  fprintf(fp,"probe_qty = %d;\n",    probe_qty);
  fprintf(fp,"probe_pd_samps = %d;\n", probe_pd_samps);
  
  fprintf(fp,"probe_len_bits = %d;\n", probe_len_bits);
  fprintf(fp,"data_hdr = 'i_adc q_adc';\n");
  fprintf(fp,"data_len_samps = %d;\n", cap_len_samps);
  fprintf(fp,"data_in_other_file = 2;\n");
  fprintf(fp,"num_itr = %d;\n", num_itr);
  fprintf(fp,"time = %d;\n", (int)time(0));
  fprintf(fp,"itr_times = [");
  for(i=0;i<num_itr;++i)
    fprintf(fp," %d", (int)(*(times_s+i)-*(times_s)));
  fprintf(fp, "];\n");

  
  fclose(fp);

#endif  
  return 0;
}

  
