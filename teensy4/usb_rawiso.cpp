#include <Arduino.h>
#include "usb_dev.h"
#include "usb_rawiso.h"
#include "debug/printf.h"


#ifdef RAWISO_INTERFACE // defined by usb_dev.h -> usb_desc.h

extern volatile uint8_t usb_high_speed;
/*static*/ transfer_t tx_transfer __attribute__ ((used, aligned(32)));


/*DMAMEM*/ //uint16_t usb_rawiso_transmit_buffer[RAWISO_TX_SIZE/2] __attribute__ ((used, aligned(32)));


IsochronousTx * activeObj = nullptr ;
uint32_t _static_c_buffer[RAWISO_TX_SIZE] __attribute__ ((used, aligned(32))) ;

uint32_t _test_pattern_0[RAWISO_TX_SIZE/sizeof(uint32_t)] __attribute__ ((used, aligned(32))) ;
uint32_t _test_pattern_1[RAWISO_TX_SIZE/sizeof(uint32_t)] __attribute__ ((used, aligned(32))) ;

volatile uint16_t _static_c_buffer_len = 0;
volatile uint8_t counter = 0 ;
volatile bool iso_underrun = true ;



#define TX_TARGET 768
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
            uint16_t targetBufferLen = activeObj->mBlockAvailable > (NB_BLOCKS_IN_FIFO/2) ? (TX_TARGET+RAW_ISO_BLOCK_SIZE) : TX_TARGET ;
            while(activeObj->mBlockAvailable > 0 && lenOfBuffer < (targetBufferLen/sizeof(uint32_t))){
                uint16_t k ;
                memcpy(&_static_c_buffer[lenOfBuffer], activeObj -> mBlockPtr[activeObj -> mBlockReadIndex], RAW_ISO_BLOCK_SIZE);
                activeObj -> mBlockReadIndex ++ ;
                activeObj -> mBlockReadIndex = activeObj -> mBlockReadIndex >= NB_BLOCKS_IN_FIFO ? 0 : activeObj -> mBlockReadIndex ;
                activeObj -> mBlockAvailable -- ; //Should mutex protect for safety
                lenOfBuffer += (RAW_ISO_BLOCK_SIZE/sizeof(uint32_t));
            }
            usb_prepare_transfer(&tx_transfer, _static_c_buffer, lenOfBuffer*sizeof(uint32_t), 0);
            arm_dcache_flush_delete(_static_c_buffer, lenOfBuffer*sizeof(uint32_t));
            usb_transmit(RAWISO_TX_ENDPOINT, &tx_transfer);
        }else{
            //UNDERRUN
            memset(&tx_transfer, 0, sizeof(tx_transfer));
            usb_transmit(RAWISO_TX_ENDPOINT, &tx_transfer);
        }
        /*if(tx_transfer.callback_param){
            usb_prepare_transfer(&tx_transfer, _test_pattern_0, 768, 0);
            arm_dcache_flush_delete(_test_pattern_0, 768);
        }else{
            usb_prepare_transfer(&tx_transfer, _test_pattern_1, 768, 1);
            arm_dcache_flush_delete(_test_pattern_1, 768);
        }
        usb_transmit(RAWISO_TX_ENDPOINT, &tx_transfer);*/

        
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
  uint32_t initial_value = 0xDEADBEEF;
  for(uint32_t i = 0 ; i < (RAWISO_TX_SIZE/sizeof(uint32_t)) ; i ++){
        _test_pattern_0[i] = initial_value ;
        _test_pattern_1[i] = ~initial_value ;
  }
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
    this -> mBlockPtr = (uint32_t **) malloc(NB_BLOCKS_IN_FIFO * sizeof(uint32_t *));
    for(int i = 0 ; i < NB_BLOCKS_IN_FIFO ; i ++){
        this -> mBlockPtr[i] = (uint32_t *) malloc(RAW_ISO_BLOCK_SIZE) ;
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
        sent = true ;
    }
    __enable_irq();
    return sent;
}


#endif //RAWHISO_INTERFACE