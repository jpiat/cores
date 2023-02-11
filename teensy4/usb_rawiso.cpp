#include <Arduino.h>
#include "usb_dev.h"
#include "usb_rawiso.h"
#include "debug/printf.h"

#ifdef RAWISO_INTERFACE // defined by usb_dev.h -> usb_desc.h

extern volatile uint8_t usb_high_speed;
/*static*/ transfer_t tx_transfer __attribute__ ((used, aligned(32)));


/*DMAMEM*/ //uint16_t usb_rawiso_transmit_buffer[RAWISO_TX_SIZE/2] __attribute__ ((used, aligned(32)));


IsochronousTx * activeObj = nullptr ;
uint16_t _static_c_buffer[RAWISO_TX_SIZE/2] __attribute__ ((used, aligned(32))) ;
volatile uint16_t _static_c_buffer_len = 0;
volatile uint8_t counter = 0 ;
volatile bool iso_underrun = true ;

void tx_event(transfer_t *t)
{

    //TODO: need to understand how much data is ready to transfer
    //TODO: copy the data to the iso_transmit_buffer or straight to the tx struct
    //usb_rawiso_sync_feedback = feedback_accumulator >> usb_rawiso_sync_rshift;
    //memset(usb_rawiso_transmit_buffer, 0xFF, RAWISO_TX_SIZE);
    //if( (_static_c_buffer_len) > 0){
        //t->callback_param
        //TODO: cannot send nothing, the transfer should occur event with an incomplete data

        //micro frame 
        if(activeObj != nullptr){
            uint16_t lenOfBuffer = 0 ;
            while(activeObj->mBlockAvailable > 1 && lenOfBuffer < (RAWISO_TX_SIZE/sizeof(uint16_t))){
                memcpy(&_static_c_buffer[lenOfBuffer], activeObj -> mBlockPtr[activeObj -> mBlockReadIndex], RAW_ISO_BLOCK_SIZE);
                activeObj -> mBlockReadIndex ++ ;
                activeObj -> mBlockReadIndex = activeObj -> mBlockReadIndex >= NB_BLOCKS_IN_FIFO ? 0 : activeObj -> mBlockReadIndex ;
                activeObj -> mBlockAvailable -- ; //Should mutex protect for safety
                lenOfBuffer += RAW_ISO_BLOCK_SIZE/sizeof(uint16_t);
            }
            if(lenOfBuffer == 0){
                memset(&tx_transfer, 0, sizeof(tx_transfer));
            }
            usb_prepare_transfer(&tx_transfer, _static_c_buffer, lenOfBuffer*sizeof(uint16_t), 0);
            //arm_dcache_flush_delete(_static_c_buffer, lenOfBuffer);
            usb_transmit(RAWISO_TX_ENDPOINT, &tx_transfer);
        }else{
            //UNDERRUN
            usb_transmit(RAWISO_TX_ENDPOINT, &tx_transfer);
        }
        
        /*
        uint16_t offset = _static_c_buffer[(RAWISO_TX_SIZE/2/2) - 1];
        for(int i = 0 ; i < (RAWISO_TX_SIZE/2) ; i ++){
            _static_c_buffer[i] = offset + i ;
        }
        usb_prepare_transfer(&tx_transfer, _static_c_buffer, RAWISO_TX_SIZE-4, 0);
        arm_dcache_flush_delete(_static_c_buffer, RAWISO_TX_SIZE);
        usb_transmit(RAWISO_TX_ENDPOINT, &tx_transfer);*/
        //counter ++ ;
        //_static_c_buffer_len = 0;
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
    this -> mBlockAvailable = 0 ;
    this -> mBlockWriteIndex = 0;
    this -> mBlockReadIndex = 0;
    this -> mBlockPtr = (uint8_t **) malloc(NB_BLOCKS_IN_FIFO * sizeof(uint8_t *));
    for(int i = 0 ; i < NB_BLOCKS_IN_FIFO ; i ++){
        this -> mBlockPtr[i] = (uint8_t *) malloc(RAW_ISO_BLOCK_SIZE) ;
    }
    activeObj = this ;
}

bool IsochronousTx::available(void)
{   
    bool res ;
    __disable_irq();
    res =  this->mBlockAvailable < NB_BLOCKS_IN_FIFO ;
    __enable_irq();
    return res ;
}

bool IsochronousTx::sendBlock(uint8_t * data)
{

    __disable_irq();
    bool sent = false ;
    if(this->mBlockAvailable < NB_BLOCKS_IN_FIFO){
        memcpy(this -> mBlockPtr[this -> mBlockWriteIndex], data, RAW_ISO_BLOCK_SIZE);
        this -> mBlockWriteIndex ++ ;
        this -> mBlockWriteIndex = this -> mBlockWriteIndex >= NB_BLOCKS_IN_FIFO ? 0 : this -> mBlockWriteIndex ;
        this -> mBlockAvailable ++ ; 
    }
    __enable_irq();
    return sent;
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