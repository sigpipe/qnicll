// qnicll.c
// libiio-based implementation for happy camper event

#include <stdio.h>
#include <iio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "qregc.h"
#include "qnicll.h"
#include "qna.h"

#define QNICLL_STR_LEN (256)


// One "samp" occupies this many bytes:
#define QNICLL_SAMP_SZ (2*sizeof(short int))

typedef struct qnicll_state_st {
  int mode;
  char errmsg[QNICLL_STR_LEN];
  char errmsg_ll[QNICLL_STR_LEN];
  char funcname[QNICLL_STR_LEN];
  struct iio_context *ctx;
  struct iio_device *dac, *adc;
  struct iio_channel *dac_ch[2], *adc_ch[2];
  struct iio_buffer *dac_buf, *adc_buf;
  int dac_samps, adc_samps;
  int probe_datasrc;

  int adc_filled, adc_occ_samps, adc_status;
  
  size_t samp_sz;
} qnicll_state_t;
static qnicll_state_t st={0};

// standard way of calling lower-level functions
// that might set an error message.
#define DO(CMD) { \
    int e=CMD;    \
    if (e) {      \
      sprintf(st.errmsg, "%s: %s", __func__, st.errmsg_ll);	\
      return e;   \
    }}

// lower level code fills in errmsg_ll using this:
static int ll_err(char *str, int err) {
  strcpy(st.errmsg_ll, str);
  return err;
}


#define RBUG(STR) {                            \
  sprintf(st.errmsg, "%s: %s", __func__, STR); \
  return QNICLL_ERR_BUG;                       \
}
#define RUSE(STR) {                            \
  sprintf(st.errmsg, "%s: %s", __func__, STR); \
  return QNICLL_ERR_MISUSE;                    \
}



char *qnicll_error_desc(void) {
// Desc: returns description of the last error, if any, if there was one.
//       If no error happened, do not call this function.
//       If no error happened, and you call this function, return value is undefined.  
//       Not all errors have descriptions, so this may return "".  
//       Contents may change after every qnicll call.
//       Don't try to free the string.
  return st.errmsg;
}

int qnicll_set_sample_rate_Hz(double *fsamp_Hz) {
  *fsamp_Hz = 1.233333333e9;
  return 0;
}



int qnicll_set_mode(int mode) {
// desc: invokes settings appropriate for specified mode. (optical switches,
//   voa settings, bias feedback goals, etc.  If called before prior set_mode finishes
// inputs: mode: requested mode. one of QNICLL_MODE_*
// returns: one of QNICLL_ERR_*
  int i;
  switch(mode) {
    case QNICLL_MODE_CDM:
      i=0;
      qna_set_txsw(&i);
      if (i!=0) RBUG("cant set txsw");
      i=0;
      qna_set_rxsw(&i);
      if (i!=0) RBUG("cant set rxsw");
      st.mode = QNICLL_MODE_CDM;
      break;
    case QNICLL_MODE_MEAS_NOISE:
      i=0;
      qna_set_txsw(&i);
      i=0;
      qna_set_rxsw(&i);
      st.mode = mode;
      break;
    case QNICLL_MODE_LINK_LOOPBACK:
      i=1;
      qna_set_txsw(&i);
      i=1;
      qna_set_rxsw(&i);
      st.mode = mode;
      break;
    default:
      RUSE("unsupported mode");
  }
  return 0;
}

int qnicll_set_probe_datasrc(int datasrc) {
// desc: determines how the probe is generated
// inputs: datasrc: requested datasrc. one of QNICLL_DATASRC_*
// returns: one of QNICLL_ERR_*
  int q, r, e;
  switch(datasrc) {
    case QNICLL_DATASRC_LFSR:
      r=q=1;
      goto set_lfsr;
    case QNICLL_DATASRC_CUSTOM:
      r=q=0;
    set_lfsr:
      e=qregc_set_use_lfsr(&r);
      if (e) return e;
      if (r!=q) return QNICLL_ERR_BUG;
      break;
    default:
      return QNICLL_ERR_MISUSE;
  }
  st.probe_datasrc = datasrc;
  return 0;
}


int qnicll_set_probe_tx_modulation(int *modulation, int *osamp_factor) {
// desc: Sets the optical modulation used by the nic to transmit its probes.
//       The modulation determines bits per symbol.
//       Currently only IM (intensity modulation) is implemented,
//       but QPSK, QAM, etc may eventually be possible.
//       Depending on implementation, may make underlying call to
//       set_symbol_map() (as per Young's HDL)
// 
//       Also sets the oversampling factor used by the qnic.
//       Currently, only 4 is implemented.
//       
// inputs: modulation: one of QNICLL_MODULATION_*
//         osamp_factor: requested oversampling factor.  samples per symbol.
// sets: modulation: the actual new modulation
//       osamp_factor: the actual new oversampling factor
// returns: one of QNICLL_ERR_*
  *modulation = QNICLL_MODULATION_IM;
  *osamp_factor = 4;
  return 0;
}

int qnicll_set_probe_tx_format(int *format) {
// desc: sets or querries the format used for
//       qnicll_set_probe_tx_data()
//       when you are using QNICLL_PROBE_DATASRC_CUSTOM.
//       Note that the rx format may be different.
// input: format: requested format. one of QNICLL_FORMAT_*  Set to zero to just do a querry.
// sets:  format: actual effective transmit format.
// returns: one of QNICLL_ERR_*
  *format = QNICLL_DATA_FORMAT_LIBIIO;
  return 0;
}

static int refill_rx_buf(void) {
  ssize_t sz;
  if (!st.adc_filled) {
    sz = iio_buffer_refill(st.adc_buf); // returns -1 or size in bytes
    if (sz<0) RBUG("cant refill buffer");
    st.adc_occ_samps = sz/(QNICLL_SAMP_SZ);
    st.adc_status=0;
    st.adc_filled=1;
  }
  return 0;
}

int qnicll_get_filled_rx_buf(short int **buf, int *occ_samps, int *status) {
  int e;
  e = refill_rx_buf();
  if (e) return e;

  // iio_buffer_start only returns a non-zero ptr after a refill.
  *buf = iio_buffer_start(st.adc_buf);
  *occ_samps = st.adc_occ_samps;
  *status    = st.adc_status;
  st.adc_filled=0;
  return 0;
}

int qnicll_return_rx_buf(void) {
  return refill_rx_buf();  
}

int qnicll_set_tx_buf_sz_samps(int *sz_samps) {
  int sz = *sz_samps;
  // sz is sizeof(short) * numer of enabled channels = 4  
  sz = ((int)(sz/4))*4;

  st.dac_buf = iio_device_create_buffer(st.dac, sz, false);
  // Is this a misnomer?  Does this actually create multiple buffers?
  if (!st.dac_buf) RBUG("cant make libiio dac buf");
  
  sz =    (iio_buffer_end(st.dac_buf)-iio_buffer_first(st.dac_buf,0))
       /  QNICLL_SAMP_SZ;
  st.dac_samps = sz;
  
  *sz_samps = sz;  
  return 0;
}


int qnicll_push_tx_buf(void) {
  int e;
  size_t sz;
  if (!st.dac_buf) RBUG("no current buffer");
  sz = iio_buffer_push(st.dac_buf);
  // with modern libiio driver, enques buffer and deques an empty one.
  // But do we have to call iio_device_create_buffer() again?
  if (!sz) RBUG("iio_buffer_push() failed");
  if (sz != st.dac_samps*QNICLL_SAMP_SZ) RBUG("iio_buffer_push() did not push all data");
  return 0;
}

int qnicll_get_free_tx_buf(short int **buf) {
  if (!st.dac_buf) RBUG("must call qnicll_set_tx_buf_sz_samps() first");
  *buf = iio_buffer_first(st.dac_buf,0);
  return 0;
}

int qnicll_set_probe_qty(int *probe_qty) {
// desc: sets the number of probes (or probe periods) to be
//       generated back-to-back in QNICLL_MODE_CDM mode.
//       You may be set it to 1 if you don't mind being inefficient.
// inputs: probe_qty: requested probe quantity
// sets:   probe_qty: set to actual new probe quantity
// returns: one of QNICLL_ERR_*
  return qregc_set_probe_qty(probe_qty);
}

int qnicll_set_probe_len_bits(int *probe_len_bits) {
// desc: sets the length of the probe, in units of bits.
//       Typically we use 32..128 bits.
// inputs: probe_len_bits: requested length of the probe in units of bits.
// sets:   probe_len_bits: actual new length of the probe
// returns: one of QNICLL_ERR_*
  return qregc_set_probe_len_bits(probe_len_bits);
}

int qnicll_set_probe_pd_samps(int *pd_samps) {
  return qregc_set_probe_pd_samps(pd_samps);
}



int qnicll_init(void *init_st) {
  // desc: initializes the library.
  // returns: one of QNICLL_ERR_*
  int i, e=0, v;
  int myiio_fd;
  char str[32];
  
  qnicll_init_info_libiio_t *libiio_init = (qnicll_init_info_libiio_t *)init_st;
  strcpy(str, "ip:");
  strcat(str, libiio_init->ipaddr);
  printf("DBG: try context %s\n", str);
  st.ctx = iio_create_context_from_uri(str);
  if (!st.ctx)
    RBUG("cannot get context. perhaps init_st.ipaddr is wrong.");

  // if we had a PCI connection, we would go through it.
  // however for now we go over inet
  if (qregc_connect(libiio_init->ipaddr, &ll_err))
    RBUG("inet connection to zcu board fail");


  // This is a serial USB connection
  // to the QNA firmware on the S750 board
  printf("DBG: try dev %s\n", libiio_init->usbdev);
  if (qna_connect(libiio_init->usbdev, &ll_err))
    RBUG("cant connect to QNA firmare on S750 board");    
  

  st.dac = iio_context_find_device(st.ctx, "axi-ad9152-hpc");
  if (!st.dac)
    RBUG("cannot find libiio dac");
  st.adc = iio_context_find_device(st.ctx, "axi-ad9680-hpc");
  if (!st.adc)
    RBUG("cannot find libiio adc");

  for(i=0;i<2;++i) {
    //sprintf(str,"voltage%d",i+1);
    // st.dac_ch[i] = iio_device_find_channel(st.dac, str, true);
    st.dac_ch[i] = iio_device_get_channel(st.dac, i);
    if (!st.dac_ch[i])
      RBUG("dac lacks channel");
    iio_channel_enable(st.dac_ch[i]);
    if (!iio_channel_is_enabled(st.dac_ch[i]))
      RBUG("chan not enabled");
  }

  for(i=0;i<2;++i) {  
    st.adc_ch[i] = iio_device_get_channel(st.adc, i);
    if (!st.adc_ch[i])
      RBUG("adc lacks chan");
    iio_channel_enable(st.adc_ch[i]);
    if (!iio_channel_is_enabled(st.adc_ch[i]))
      RBUG("chan not enabled");
  }
  
  st.samp_sz = iio_device_get_sample_size(st.adc);
  //  printf("  adc samp size %zd\n", sz);

  st.adc_buf=st.dac_buf=0;
  
  return 0;  
}



int qnicll_dbg_set_tx_always(int *en) {
  return qregc_set_tx_always(en);
}

int qnicll_dbg_set_tx_0(int *en) {
  return qregc_set_tx_0(en);
}

int qnicll_txrx_en(int en) {
  int e,r=en;
  e = qregc_tx(&r);
  if (e) return e;
  return (r==en)?0:QNICLL_ERR_BUG;
}



int qnicll_set_rx_buf_sz_samps(int *sz_samps) {
  int sz = *sz_samps;
  if (st.adc_buf) RUSE("cant change adc buf size");
  sz = (int)(sz/4)*4;
  st.adc_buf = iio_device_create_buffer(st.adc, sz, 0);
  if (!st.adc_buf) RBUG("cant make adc buf");
  *sz_samps = sz;
  return 0;
}




int qnicll_done(void) {
// desc: call when done using the library, if you want to be tidy.
//       Or don't ever call it!
  if (st.adc_buf) iio_buffer_destroy(st.adc_buf);
  if (st.dac_buf) iio_buffer_destroy(st.dac_buf);
  qregc_disconnect();  
  qna_disconnect();
}




int qnicll_idx_t2p(int format, int ch, int t_idx) {
// desc: converts temporal index to positional index and vice versus, according to format.
// inputs: ch: 0=intensity or I,  1 =phase or Q.
//         t_idx: temporal index.
// returns: positional index within buffer
  switch(format) {
    case QNICLL_DATA_FORMAT_LIBIIO:
      return t_idx*2+ch;
    case QNICLL_DATA_FORMAT_PCI_HIRES:
      return (t_idx/4)*8+ch*4+(t_idx%4);
    case QNICLL_DATA_FORMAT_PCI_LORES:
      return (t_idx/8)*16+ch*8+(t_idx%8);
    default:
      return 0;
  }
}


