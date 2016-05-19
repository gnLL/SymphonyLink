#include <SymphonyLink.h>

#define SL_RESET_PIN 7
#define SL_BOOT_PIN 8
#define SL_IRQ_PIN 13


SymphonyLink symlink;

void setup() 
{


	//configure these to match your network and application tokens
	//Step 1 - set desired network token
	uint32_t network_token = 0x4f50454e;		//open token

	//Step 2 - set desired application token
	uint8_t app_token[APP_TOKEN_LEN] = {0x61,0x04,0xce,0xd4,0x8d,0x49,0xa8,0xfb,0xcd,0x1e};
	

	//setup pin IO for standard linklabs eval board on an arduino.
	pinMode(SL_RESET_PIN, OUTPUT);
	
	//Not being used in this script
	pinMode(SL_BOOT_PIN, OUTPUT);
	pinMode(SL_IRQ_PIN, INPUT);

	digitalWrite(SL_BOOT_PIN, LOW);
	digitalWrite(SL_RESET_PIN, LOW);

	//Symphony module uses the Serial1 interface for communicaiton
	//115200.  8-N-1
	Serial1.begin(115200);
	
	//Due can allow debug and status signals out of the Serial port.
	Serial.begin(115200);


	//reset the module and wait 2 seconds for boot.
	digitalWrite(SL_RESET_PIN, HIGH);
	delay(10);
	digitalWrite(SL_RESET_PIN, LOW);
	delay(2000);

	Serial.write("Starting system\n");
	
	symlink.setAntenna(UFL);

	//send configuration data to initial the device
	symlink.begin(network_token, app_token, ON, 15);

	//Wait until the module is connected to the network. This could be part of the 
	//main loop if the module is going to be shutdown as part of the application
	while (READ_TO_SEND != symlink.updateModemState())
	{
		Serial.write("Waiting to connect\n");
		delay(1000);		//This could be shorter
	}
}


uint8_t data[2] = {0,0} ;		//TX data buffer.  Can be made bigger
uint8_t rxdata[256];			//RX data buffer.  The max message size is 256

boolean success;
uint8_t len;

void loop()
{
	
	if (READ_TO_SEND == symlink.updateModemState())
	{
		//Send simple counter
		data[0]++;
	
		//Write bytes to Conductor
		symlink.write(data, 2);
		delay(500);
	}

	delay(100);

}
