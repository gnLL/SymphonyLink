
#include "SymphonyLink.h"
#include "ll_ifc_symphony.h"


SymphonyLink::SymphonyLink()
{
	//Set values to zero or default states
	
	_net_token = 0;
	memset(_app_token, 0, APP_TOKEN_LEN);
	_downlink_mode = LL_DL_OFF;
	_qos = 0;
	connected = false;
	
	
}

boolean SymphonyLink::get_irq(uint32_t flagsToClear)
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

boolean SymphonyLink::get_state(void)
{
	if(0 > ll_get_state(&_modState,&_txState,&_rxState))
	{
		Serial.write("Error get_state\n");
		return false;
	}
	else
	{
		return true;
	}
}
	

boolean SymphonyLink::begin(uint32_t net_token, uint8_t* app_token, DownlinkMode dl_mode, uint8_t qos)
{
	uint8_t i;
	
	
	get_irq(0);
	
	get_state();

	
	_net_token = net_token;

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
	
	connected = false;

	Serial.write("sending config\n");
	//configure module and start connection
	if(0 > ll_config_set(_net_token, _app_token, _downlink_mode, _qos))
	{
		Serial.write("Error ll_config_set\n");
		return false;
	}
	
	delay(100);
	
	updateStatus();
	
	return true;


}



boolean SymphonyLink::write(uint8_t* buf, uint16_t len)
{
	
	//First make sure we are connected
	if (_modState == LL_STATE_IDLE_CONNECTED )
	{
		//Check to see if we are currently transmitting
		//Send packet if not
		if(_txState == LL_TX_STATE_TRANSMITTING)
		{
			  delay(100);
			  return false;
		}
		else
		{
			 if(0 > ll_message_send_ack(buf,len))
			 {
				 Serial.write("Error ll_message_send_ack\n");
			 }
		}
	}
	else
	{
		return false;
	}
	
	//get the IRQ flags
	if( false==get_irq(0))
	{
		return false;
	}
	
	//Keep looping until transmitt is done.
	//ToDo: refactor this into a new function so the main loop doesn not need to spin //on it
	while ((_IRQ & IRQ_FLAGS_TX_DONE) == 0)
	{
		delay(100);
		
		//get the IRQ flags
		if( false==get_irq(0))
		{
			return false;
		}
		
	}
	
	if( false==get_irq(IRQ_FLAGS_TX_DONE))
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

boolean SymphonyLink::updateStatus(void)
{
	
	if(false==get_irq(0))
	{
		return false;
	}
	else
	{
		
		
		if(false==get_state())
		{
			return false;	
		}
		else
		{
			if(_modState == LL_STATE_IDLE_CONNECTED )
			{
				connected = true;
			}
			else
			{
				connected = false;
			}
			return true;	
		}
		
		if(_rxState == LL_RX_STATE_RECEIVED_MSG)
		{
			
		}
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