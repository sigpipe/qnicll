#ifndef _QREGC_H_
#define _QREGC_H_

#include "qnicll.h"
#include "qnicll_internal.h"

// these use QNICLL_ERR return codes
int qregc_connect(char *hostname, qnicll_set_err_fn err_fn);
int qregc_set_probe_qty(int *probe_qty);
int qregc_set_probe_pd_samps(int *pd_samps);
int qregc_set_probe_len_bits(int *probe_len_bits);
int qregc_set_tx_always(int *en);
int qregc_set_tx_0(int *en);
int qregc_set_use_lfsr(int *use);
int qregc_tx(int *en);
int qregc_disconnect(void);

int qregc_set_txc_voa_atten_dB(double *atten_dB);
int qregc_set_txq_voa_atten_dB(double *atten_dB);
int qregc_set_rx_voa_atten_dB(double *atten_dB);

int qregc_set_tx_opsw_cross(int *cross);
int qregc_set_rx_opsw_cross(int *cross);

int qregc_set_rxq_basis(int *basis);
int qregc_set_fpc_wp_dac(int wp, int *dac);  
int qregc_do_cmd(char *cmd, char *rsp, int rsp_len);
int qregc_shutdown(void);

#endif
