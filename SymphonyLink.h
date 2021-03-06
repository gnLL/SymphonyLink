
#ifndef SYMPHONYLINK_H
#define SYMPHONYLINK_H


#include "arduino.h"
#include "ll_ifc_consts.h"
#include "ll_ifc_symphony.h"

enum DownlinkMode
{	
	OFF = 0,
	ON ,
	MAILBOX	
};

enum AntennaMode
{
	UFL = 1,
	TRACE = 2
};

typedef enum modemState
{
    INIT=0,
    CONNECTING,
	LINK_INIT,
	READ_TO_SEND,
    SENDING_FRAME
};

class SymphonyLink {
	
	public:
	
		SymphonyLink();
		boolean begin(uint32_t net_token, uint8_t* app_token, DownlinkMode dl_mode, uint8_t qos);
		boolean write(uint8_t* buf, uint16_t len);
		boolean read (uint8_t* buf, uint8_t* len);
		
		modemState updateModemState(void);
		boolean setAntenna(AntennaMode ant);
		
	
		
	private:
		
		modemState _state;
		uint32_t _net_token;
		uint8_t _app_token[APP_TOKEN_LEN];
		ll_downlink_mode _downlink_mode;
		uint8_t _qos;
		ll_rx_state _rxState;
		ll_tx_state _txState;
		ll_state _modState;
		uint32_t _IRQ;
		
		enum ll_rx_state getRxState();
		enum ll_tx_state getTxState();
		enum ll_state getModState();
		uint32_t getISR();
	
	
		boolean getIRQ(uint32_t flagsToClear);
		boolean getState(void);

};



#endif // SYMPHONYLINK_H