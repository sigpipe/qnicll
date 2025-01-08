#ifndef _QNA_H_
#define _QNA_H_

// QNA Client
// The QNA firmware runs on the S750 board.

#include "qnicll.h"
#include "qnicll_internal.h"

int qna_connect(char *devname, qnicll_set_err_fn *err_fn);

int qna_set_txc_voa_atten_dB(double *atten_dB);
// desc: sets transmit classical optical attenuation in units of dB
//       Within the valid range, granularity of valid settings is better than 0.1 dB.
// inputs: atten_dB: requested attenuation in units of dB, typically 0
//                   to 60 but max attenuation is implementation and calibration
//                   dependent.  Boot-up default setting is something reasonable.
//                   (probably max atten)
// sets: atten_dB: set to the new actual effective attenuation.
// returns: one of QNICLL_ERR_* (after attenuation is achieved)


int qna_set_txq_voa_atten_dB(double *atten_dB);
// desc: sets transmit quantum optical attenuation in units of dB.
// inputs: atten_dB: requested attenuation in units of dB.
// sets: atten_dB: set to the new actual effective attenuation.
// returns: one of QNICLL_ERR_* (after attenuation is achieved)



int qna_set_rx_voa_atten_dB(double *atten_dB);
// desc: sets receiver attenuation (after the receiver EDFA).
// inputs: atten_dB: requested attenuation in units of dB.
// sets: atten_dB: set to the new actual effective attenuation.

int qna_set_txsw(int *cross);
// inputs: setting: 0 (thru) or 1 (cross)

int qna_set_rxsw(int *cross);
// inputs: setting: 0 (thru) or 1 (cross)


int qna_set_rxefpc_basis(int *basis);
// desc: sets rx EFPC basis
// in: requested basis.  typically 0 to 2
// sets: basis actually set to.


int qna_disconnect(void);

#endif
