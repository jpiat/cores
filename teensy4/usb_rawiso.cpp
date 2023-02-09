#include <Arduino.h>
#include "usb_dev.h"
#include "usb_rawiso.h"
#include "debug/printf.h"

#ifdef RAWISO_INTERFACE // defined by usb_dev.h -> usb_desc.h

extern volatile uint8_t usb_high_speed;
static void tx_event(transfer_t *t);
/*static*/ transfer_t tx_transfer __attribute__ ((used, aligned(32)));


/*DMAMEM*/ //uint16_t usb_rawiso_transmit_buffer[RAWISO_TX_SIZE/2] __attribute__ ((used, aligned(32)));


uint16_t _static_c_buffer[RAWISO_TX_SIZE/2] __attribute__ ((used, aligned(32))) ;
volatile uint16_t _static_c_buffer_len = 0;

static void tx_event(transfer_t *t)
{

    //TODO: need to understand how much data is ready to transfer
    //TODO: copy the data to the iso_transmit_buffer or straight to the tx struct
    //usb_rawiso_sync_feedback = feedback_accumulator >> usb_rawiso_sync_rshift;
    //memset(usb_rawiso_transmit_buffer, 0xFF, RAWISO_TX_SIZE);
    //if( (_static_c_buffer_len) > 0){
        usb_prepare_transfer(&tx_transfer, _static_c_buffer, _static_c_buffer_len*sizeof(uint16_t), 0);
        //arm_dcache_flush_delete(_static_c_buffer, RAWISO_TX_SIZE);
        usb_transmit(RAWISO_TX_ENDPOINT, &tx_transfer);
        _static_c_buffer_len = 0 ;
    //}
}

void usb_rawiso_configure(void)
{
  printf("iso\n");
  memset(&tx_transfer, 0, sizeof(tx_transfer));
  usb_config_tx_iso(RAWISO_TX_ENDPOINT, RAWISO_TX_SIZE, 1, tx_event);
  tx_event(NULL);
}

void IsochronousTx::begin(void)
{
    _static_c_buffer_len = 0 ;
}

bool IsochronousTx::available(void)
{
    return (_static_c_buffer_len == 0) ;
}

uint32_t IsochronousTx::send(uint8_t * data, uint16_t len)
{
    if(this->available()){
        if(len > RAWISO_TX_SIZE) len = RAWISO_TX_SIZE  ;
        memcpy(_static_c_buffer, data, len );
        _static_c_buffer_len = len/sizeof(uint16_t) ;
        return len ;
    }
    return 0;
}


struct setup_struct {
  union {
    struct {
  uint8_t bmRequestType;
  uint8_t bRequest;
  union {
    struct {
      uint8_t bChannel;  // 0=main, 1=left, 2=right
      uint8_t bCS;       // Control Selector
    };
    uint16_t wValue;
  };
  union {
    struct {
      uint8_t bIfEp;     // type of entity
      uint8_t bEntityId; // UnitID, TerminalID, etc.
    };
    uint16_t wIndex;
  };
  uint16_t wLength;
    };
  };
};

int usb_rawiso_tx_get_feature(void *stp, uint8_t *data, uint32_t *datalen)
{
  return 0;
}

int usb_rawiso_tx_set_feature(void *stp, uint8_t *buf) 
{
  return 0;
}

#endif //RAWHISO_INTERFACE