#pragma once

#include "usb_desc.h"
#include "usb_dev.h"

#if defined(RAWISO_INTERFACE)

#ifdef __cplusplus
extern "C" {
#endif
extern void usb_rawiso_configure();
extern uint16_t usb_rawiso_transmit_buffer[];
extern unsigned int usb_rawiso_transmit_callback(void);
extern int usb_rawiso_set_feature(void *stp, uint8_t *buf);
extern int usb_rawiso_get_feature(void *stp, uint8_t *data, uint32_t *datalen);
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

class IsochronousTx
{
public:
  IsochronousTx(void){ begin(); }
  bool available(void);
  uint32_t send(uint16_t * data, uint16_t len);
  void begin(void);
private:
  static uint16_t _usb_rawiso_transmit_buffer[RAWISO_TX_SIZE/2];
  static uint16_t _usb_rawiso_transmit_buffer_len ;
  static void tx_event(transfer_t *t) ;

};
#endif // __cplusplus

#endif // RAWISO