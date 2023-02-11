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

#define NB_BLOCKS_IN_FIFO 16
#define RAW_ISO_BLOCK_SIZE 64
class IsochronousTx
{
public:
  IsochronousTx(void){ begin(); }
  bool available(void);
  bool sendBlock(uint8_t * data);
  void begin(void);
  volatile uint16_t mBlockAvailable ;
  volatile uint8_t mBlockWriteIndex ;
  volatile uint8_t mBlockReadIndex ;
  uint8_t ** mBlockPtr ;
private:
  static void tx_event(transfer_t *t) ;
};
#endif // __cplusplus

#endif // RAWISO