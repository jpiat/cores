#include <Arduino.h>
#include "usb_dev.h"
#include "usb_rawiso.h"
#include "debug/printf.h"


#ifdef RAWISO_INTERFACE // defined by usb_dev.h -> usb_desc.h

extern volatile uint8_t usb_high_speed;
transfer_t tx_transfer __attribute__ ((used, aligned(32)));
transfer_t rx_transfer __attribute__ ((used, aligned(32)));
transfer_t sync_transfer __attribute__ ((used, aligned(32)));


/*DMAMEM*/ //uint16_t usb_rawiso_transmit_buffer[RAWISO_TX_SIZE/2] __attribute__ ((used, aligned(32)));


IsochronousRxTx * activeObj = nullptr ;
uint32_t _static_c_tx_buffer[RAWISO_TX_SIZE] __attribute__ ((used, aligned(32))) ;
uint32_t _static_c_rx_buffer[RAWISO_RX_SIZE] __attribute__ ((used, aligned(32))) ;
uint32_t _static_c_sync_buffer[1] __attribute__ ((used, aligned(32))) ;

uint32_t _test_pattern_0[RAWISO_TX_SIZE/sizeof(uint32_t)] __attribute__ ((used, aligned(32))) ;
uint32_t _test_pattern_1[RAWISO_TX_SIZE/sizeof(uint32_t)] __attribute__ ((used, aligned(32))) ;

volatile uint16_t _static_c_tx_buffer_len = 0;
volatile uint8_t counter = 0 ;
volatile bool iso_underrun = true ;



static void rx_event(transfer_t *t)
{
	if (t && activeObj) {
		int len = RAWISO_RX_SIZE - ((rx_transfer.status >> 16) & 0x7FFF);

        uint32_t * data = _static_c_rx_buffer ;
        while(len > 0){
            if(activeObj->mInBlockAvailable < NB_BLOCKS_IN_FIFO){
                memcpy(activeObj -> mInBlockPtr[activeObj -> mInBlockWriteIndex], data, RAW_ISO_BLOCK_IN_SIZE);
                activeObj -> mInBlockWriteIndex ++ ;
                activeObj -> mInBlockWriteIndex = activeObj -> mInBlockWriteIndex >= NB_BLOCKS_IN_FIFO ? 0 : activeObj -> mInBlockWriteIndex ;
                activeObj -> mInBlockAvailable ++ ; 
            }else{
                break ;
            }
            data += RAW_ISO_BLOCK_IN_SIZE/sizeof(uint32_t) ;
            len -= RAW_ISO_BLOCK_IN_SIZE;
        }
	}
	usb_prepare_transfer(&rx_transfer, _static_c_rx_buffer, RAWISO_RX_SIZE, 0);
	arm_dcache_delete(&_static_c_rx_buffer, RAWISO_RX_SIZE);
	usb_receive(RAWISO_RX_ENDPOINT, &rx_transfer);
}


#define TX_TARGET 768
void tx_event(transfer_t *t)
{

    //TODO: need to understand how much data is ready to transfer
    //TODO: copy the data to the iso_transmit_buffer or straight to the tx struct
    //usb_rawiso_sync_feedback = feedback_accumulator >> usb_rawiso_sync_rshift;
    //memset(usb_rawiso_transmit_buffer, 0xFF, RAWISO_TX_SIZE);
    //if( (_static_c_tx_buffer_len) > 0){
        //t->callback_param
        //TODO: cannot send nothing, the transfer should occur event with an incomplete data

        //micro frame 
        if(activeObj != nullptr){
            uint16_t lenOfBuffer = 0 ;
            uint16_t targetBufferLen = activeObj->mOutBlockAvailable > (NB_BLOCKS_IN_FIFO/2) ? (TX_TARGET+RAW_ISO_BLOCK_OUT_SIZE) : TX_TARGET ;
            while(activeObj->mOutBlockAvailable > 0 && lenOfBuffer < (targetBufferLen/sizeof(uint32_t))){
                memcpy(&_static_c_tx_buffer[lenOfBuffer], activeObj -> mOutBlockPtr[activeObj -> mOutBlockReadIndex], RAW_ISO_BLOCK_OUT_SIZE);
                activeObj -> mOutBlockReadIndex ++ ;
                activeObj -> mOutBlockReadIndex = activeObj -> mOutBlockReadIndex >= NB_BLOCKS_IN_FIFO ? 0 : activeObj -> mOutBlockReadIndex ;
                activeObj -> mOutBlockAvailable -- ; //Should mutex protect for safety
                lenOfBuffer += (RAW_ISO_BLOCK_OUT_SIZE/sizeof(uint32_t));
            }
            usb_prepare_transfer(&tx_transfer, _static_c_tx_buffer, lenOfBuffer*sizeof(uint32_t), 0);
            arm_dcache_flush_delete(_static_c_tx_buffer, lenOfBuffer*sizeof(uint32_t));
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
        uint16_t offset = _static_c_tx_buffer[(RAWISO_TX_SIZE/2/2) - 1];
        for(int i = 0 ; i < (RAWISO_TX_SIZE/2) ; i ++){
            _static_c_tx_buffer[i] = offset + i ;
        }
        usb_prepare_transfer(&tx_transfer, _static_c_tx_buffer, RAWISO_TX_SIZE-4, 0);
        arm_dcache_flush_delete(_static_c_tx_buffer, RAWISO_TX_SIZE);
        usb_transmit(RAWISO_TX_ENDPOINT, &tx_transfer);*/
        //counter ++ ;
        //_static_c_tx_buffer_len = 0;
    //}
}

/*void sync_event(transfer_t *t)
{

        if(activeObj != nullptr){
            uint8_t * sync_word = (uint8_t * ) &_static_c_sync_buffer[0];
            sync_word[0] = NB_BLOCKS_IN_FIFO ;
            sync_word[1] = activeObj->mOutBlockAvailable ;
            sync_word[2] = NB_BLOCKS_IN_FIFO ;
            sync_word[3] = activeObj-> mInBlockAvailable;
            usb_prepare_transfer(&sync_transfer, _static_c_sync_buffer, sizeof(uint32_t), 0);
            arm_dcache_flush_delete(_static_c_sync_buffer, sizeof(uint32_t));
            usb_transmit(RAWISO_SYNC_ENDPOINT, &sync_transfer);
        }else{
            //UNDERRUN
            memset(&sync_transfer, 0, sizeof(sync_transfer));
            usb_transmit(RAWISO_SYNC_ENDPOINT, &sync_transfer);
        }

}*/




void usb_rawiso_configure(void)
{
  uint32_t initial_value = 0xDEADBEEF;
  for(uint32_t i = 0 ; i < (RAWISO_TX_SIZE/sizeof(uint32_t)) ; i ++){
        _test_pattern_0[i] = initial_value ;
        _test_pattern_1[i] = ~initial_value ;
  }

  usb_config_rx_iso(RAWISO_RX_ENDPOINT, RAWISO_RX_SIZE, 1, rx_event);
  rx_event(NULL);
  memset(&tx_transfer, 0, sizeof(tx_transfer));
  usb_config_tx_iso(RAWISO_TX_ENDPOINT, RAWISO_TX_SIZE, 1, tx_event);
  tx_event(NULL);
  /*usb_config_tx_iso(RAWISO_SYNC_ENDPOINT, RAWISO_SYNC_TX_SIZE, 1, sync_event);
  sync_event(NULL);*/
}

void IsochronousRxTx::begin(void)
{
    _static_c_tx_buffer_len = 0 ;
    this -> mOutBlockAvailable = 0 ;
    this -> mOutBlockWriteIndex = 0;
    this -> mOutBlockReadIndex = 0;
    this -> mOutBlockPtr = (uint32_t **) malloc(NB_BLOCKS_IN_FIFO * sizeof(uint32_t *));
    for(int i = 0 ; i < NB_BLOCKS_IN_FIFO ; i ++){
        this -> mOutBlockPtr[i] = (uint32_t *) malloc(RAW_ISO_BLOCK_OUT_SIZE) ;
    }

    this -> mInBlockAvailable = 0 ;
    this -> mInBlockWriteIndex = 0;
    this -> mInBlockReadIndex = 0;
    this -> mInBlockPtr = (uint32_t **) malloc(NB_BLOCKS_IN_FIFO * sizeof(uint32_t *));
    for(int i = 0 ; i < NB_BLOCKS_IN_FIFO ; i ++){
        this -> mInBlockPtr[i] = (uint32_t *) malloc(RAW_ISO_BLOCK_IN_SIZE) ;
    }

    activeObj = this ;
}

bool IsochronousRxTx::available(void)
{   
    bool res ;
    __disable_irq();
    res =  this->mOutBlockAvailable < NB_BLOCKS_IN_FIFO ;
    __enable_irq();
    return res ;
}

bool IsochronousRxTx::sendBlock(uint8_t * data)
{

    __disable_irq();
    bool sent = false ;
    if(this->mOutBlockAvailable < NB_BLOCKS_IN_FIFO){
        memcpy(this -> mOutBlockPtr[this -> mOutBlockWriteIndex], data, RAW_ISO_BLOCK_OUT_SIZE);
        this -> mOutBlockWriteIndex ++ ;
        this -> mOutBlockWriteIndex = this -> mOutBlockWriteIndex >= NB_BLOCKS_IN_FIFO ? 0 : this -> mOutBlockWriteIndex ;
        this -> mOutBlockAvailable ++ ; 
        sent = true ;
    }
    __enable_irq();
    return sent;
}

bool IsochronousRxTx::rcvBlock(uint8_t * data)
{

    __disable_irq();
    bool rcv = false ;
    if(this->mInBlockAvailable > 0){
        memcpy(data, this -> mInBlockPtr[this -> mInBlockReadIndex], RAW_ISO_BLOCK_IN_SIZE);
        this -> mInBlockReadIndex ++ ;
        this -> mInBlockReadIndex = this -> mInBlockReadIndex >=  NB_BLOCKS_IN_FIFO ? 0 : this -> mInBlockReadIndex ;
        this -> mInBlockAvailable -- ; 
        rcv = true ;
    }
    __enable_irq();
    return rcv;
}

uint32_t IsochronousRxTx::rcvAvailable()
{
    uint32_t available = 0 ;
    __disable_irq();
    available = this->mInBlockAvailable  ;
    __enable_irq();
    return available;
}


#endif //RAWISO_INTERFACE