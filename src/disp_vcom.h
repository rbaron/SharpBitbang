#ifndef __DISP_VCOM_H__
#define __DISP_VCOM_H__

// Initializes the display alternating VCOM/VB and VA signals.
// It uses TIMER + GPIOTE + PPI peripherals to generate the alternating signals
// without CPU intervention.
int disp_vcom_init(void);

#endif  // __DISP_VCOM_H__