
#include "SymphonyLink.h"
#include "ll_ifc_symphony.h"



modemState SymphonyLink::updateModemState(void)
{
	//clear all flags
	getIRQ(0xFFFFFFFF);
	
	getState();
	
	switch (_state)
	{
		case INIT:
			Serial.write("State: INIT\r");
			
			//configure module and start connection
			if(0 > ll_config_set(_net_token, _app_token, _downlink_mode, _qos))
			{
				Serial.write("Error ll_config_set\n");
			}
			else
			{
				Serial.write("Symphony Initialized\n");
			
				_state = CONNECTING;
			}
			
			break;
			
		case CONNECTING:
			Serial.write("State: CONNECTING\r");
			
			switch(_modState)
			{
				case LL_STATE_INITIALIZING:
					_state = LINK_INIT;
				break;
				case LL_STATE_IDLE_CONNECTED:
					Serial.write("\nConnected\n");
					_state = READ_TO_SEND;
				break;
				case LL_STATE_IDLE_DISCONNECTED:
					//keep looping
				break;
				case  LL_STATE_ERROR:
					Serial.write("\nSTATE_ERROR\n");
					_state = INIT;
				break;
				default:
					Serial.write("\nBAD STATE\n");
			}

			break;
		case LINK_INIT:
			Serial.write("State: Initializing\r");
			switch(_modState)
				{
					case LL_STATE_INITIALIZING:
						_state = LINK_INIT;
					break;
					case LL_STATE_IDLE_CONNECTED:
						Serial.write("\nConnected\n");
						_state = READ_TO_SEND;
					break;
					case LL_STATE_IDLE_DISCONNECTED:
						//keep looping
					break;
					case  LL_STATE_ERROR:
						Serial.write("\nSTATE_ERROR\n");
						_state = INIT;
					break;
					default:
						Serial.write("\nBAD STATE\n");
				}
				break;
				
		case READ_TO_SEND:
			
			//device is ready to send data.  Waiting for message to send.
			if (_modState!=LL_STATE_IDLE_CONNECTED)
			{
				Serial.write("Device lost connection\n");
				_state = INIT;
			}
			else if(_txState == LL_TX_STATE_TRANSMITTING)
			{
				_state = SENDING_FRAME;
			}
			break;
				
		case SENDING_FRAME:
		
			Serial.write("State: SENDING_FRAME\r");
			if ((_IRQ & IRQ_FLAGS_TX_DONE) != 0)
			{
				Serial.write("\nSent message!!!!!!\n");
				//clear the flasg
				getIRQ(IRQ_FLAGS_TX_ERROR);
				_state = READ_TO_SEND;
				
			}
			else if  ((_IRQ & IRQ_FLAGS_TX_ERROR) != 0)
			{
				Serial.write("\nError sending frame\n");
				getIRQ(IRQ_FLAGS_TX_ERROR);

				_state = SENDING_FRAME;
			}
			break;

		default:
			while(1);
	}
	return	_state;
}

SymphonyLink::SymphonyLink()
{
	//Set values to zero or default states
	
	_net_token = 0;
	memset(_app_token, 0, APP_TOKEN_LEN);
	_downlink_mode = LL_DL_OFF;
	_qos = 0;
	_state = INIT;
	
	
}

boolean SymphonyLink::setAntenna(AntennaMode ant)
{
	if(0 > ll_antenna_set(ant))
	{
		Serial.write("Error ll_antenna_set\n");
		return false;
	}
	return true;
		
}

boolean SymphonyLink::getIRQ(uint32_t flagsToClear)
{
	//clear flags from reseting and connecting
	if(0 > ll_irq_flags(flagsToClear,&_IRQ))
	{
		Serial.write("Error ll_irq_flags\n");
		return false;
	}
	else
	{
		return true;
	}
	
}

boolean SymphonyLink::getState(void)
{
	if(0 > ll_get_state(&_modState,&_txState,&_rxState))
	{
		
		Serial.write("Error getModState\n");
		return false;
	}
	else
	{
		/*
			Serial.print("modstate=");
			Serial.print(_modState);
			Serial.print("\n");
		*/
		return true;
	}
}
	

boolean SymphonyLink::begin(uint32_t net_token, uint8_t* app_token, DownlinkMode dl_mode, uint8_t qos)
{
	uint8_t i;
	ll_mac_type_t mac_mode;
	
	
	getIRQ(0);
	
	
	
	//Read the MAC mode of the module.  Set to Symphony Link mode if not already set
	if(0 > ll_mac_mode_get(&mac_mode))
	{
		Serial.write("Error ll_mac_mode_get\n");
		return false;
	}
	
	
	//Setting the mode requires the module to reset and a 2 second delay should be inserted
	//until the module's host interface is back on line.
	if(mac_mode != SYMPHONY_LINK)
	{
		if(0 > ll_mac_mode_set(SYMPHONY_LINK))
		{
			Serial.write("Error ll_mac_mode_set\n");
			return false;
		}
		Serial.write("Setting to Symphony Link Mode\n");
		delay(2000);
	}
	
	_net_token = net_token;

	//Make local copy of apptoken
	for(i=0;i<APP_TOKEN_LEN; i++)
	{
		_app_token[i] = app_token[i];
	}

	switch (dl_mode)
	{
		case OFF:
			_downlink_mode = LL_DL_OFF;
			break;
			
		case ON:
			_downlink_mode = LL_DL_ALWAYS_ON;
			break;
			
		case MAILBOX:
			_downlink_mode = LL_DL_MAILBOX;
			break;
	}
	
	_qos = qos;
	
	delay(100);
	
	updateModemState();
	return true;

}



boolean SymphonyLink::write(uint8_t* buf, uint16_t len)
{
	int32_t ret;
	
	updateModemState();
	
	if (_state==READ_TO_SEND)
	{							
		ret = ll_message_send_ack(buf,len);
		if(ret<0)
		{
			Serial.write("Error sending frame\n");
			updateModemState();
			return false;
		}
		else
		{
			while(_state != SENDING_FRAME)
			{
				updateModemState();
				delay(100);
			}
		}
	}
	else
	{
		return false;
	}
	
	return true;

}


boolean SymphonyLink::read(uint8_t* buf, uint8_t* len)
{
	int16_t rssi;
	uint8_t snr;
	int32_t ret;
	
	if (_rxState == LL_RX_STATE_RECEIVED_MSG)
	{
			ret = ll_retrieve_message(buf,len, &rssi, &snr);
			if (ret<0)
			{
				Serial.print("Error ll_retrieve_message\n");
				return false;
			}
			else
			{
				return true;
			}
	}
	else
	{
		return false;
	}
	
}



int32_t transport_write(uint8_t* buf, uint16_t len)
{
	
    int32_t ret;	
	
    ret = Serial1.write(buf, len);
	
	
    if (ret < 0) {
        return -1;
    }

    return 0;
}


int32_t transport_read(uint8_t *buf, uint16_t len)
{
   
	
    uint8_t ret;
   
    uint32_t timeout_val = (500 * 1);

	Serial1.setTimeout(timeout_val);
	ret = Serial1.readBytes(buf, len); 

	if (ret > 0) 
	{
			return(ret);
	}
	else
	{
		  return(-1);
	}
}
