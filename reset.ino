#include "constant.h"

#include "wiring_private.h"
#include <RTCZero.h>
#include <Adafruit_SleepyDog.h>
#include <arduinoUART.h>

// LoRaWAN library
#include <arduinoLoRaWAN.h>
#include <configLoRaWAN.h>

RTCZero rtc;

/* Change these values to set the current initial time */
const uint8_t seconds = 0;
const uint8_t minutes = 00;
const uint8_t hours = 10;

/* Change these values to set the current initial date */
const uint8_t day = 20;
const uint8_t month = 2;
const uint8_t year = 17;

//////////////////////////////////////////////
uint8_t uart = 1;
//////////////////////////////////////////////

// Device parameters for Back-End registration
////////////////////////////////////////////////////////////
char DEVICE_EUI[]  = "0004A30B001BBB91";
char APP_EUI[] = "0102030405060708";
char APP_KEY[] = "27C6363C4B58C2FAA796DA60F762841B";
////////////////////////////////////////////////////////////

// Define port to use in Back-End: from 1 to 223
uint8_t port = 1;

// Define data payload to send (maximum is up to data rate)
char data[] = "010203";

// variable
uint8_t error = 0;
uint8_t _deviceState = DEVICE_STATE_INIT;

uint32_t cont = 0;

int err_tx = 0;

void alarmMatch()
{
	//Serial2.println("Wakeup");
}

// the setup function runs once when you press reset or power the board
void setup() {

	 // Switch unused pins as input and enabled built-in pullup
	 for (unsigned char pinNumber = 0; pinNumber < 23; pinNumber++)
	 {
		 pinMode(pinNumber, INPUT_PULLUP);
	 }

	 for (unsigned char pinNumber = 32; pinNumber < 34; pinNumber++)
	 {
		 pinMode(pinNumber, INPUT_PULLUP);
	 }

	 // NO 34 = PA19 = D12 reset pin

	 for (unsigned char pinNumber = 35; pinNumber < 42; pinNumber++)
	 {
		 pinMode(pinNumber, INPUT_PULLUP);
	 }

	 pinMode(25, INPUT_PULLUP);
	 pinMode(26, INPUT_PULLUP);

	pinMode(LED, OUTPUT);
	digitalWrite(LED, LOW);
	pinMode(RN_RESET, INPUT);
	pinMode(RN_RESET, OUTPUT);
	digitalWrite(RN_RESET, HIGH);
	pinMode(BAT_ADC_EN, OUTPUT);
	digitalWrite(BAT_ADC_EN, LOW);
	pinMode(BAT_ADC, INPUT);
	analogReadResolution(12);

//	pinMode(18, OUTPUT);
//	digitalWrite(18, HIGH);
	// pin reset host
	digitalWrite(PIN_RESET, LOW);
	pinMode(PIN_RESET, OUTPUT);

	//Serial.begin(9600);
	Serial.begin(9600);
//	pinPeripheral(6, PIO_SERCOM);
//	pinPeripheral(7, PIO_SERCOM);
//	Serial1.begin(57600);

	digitalWrite(LED, HIGH);
	delay(20000);
	digitalWrite(LED, LOW);
	Serial.println("START");

	Serial.end();
	USBDevice.detach();

	// RTC initialization
	rtc.begin();
	rtc.setTime(hours, minutes, seconds);
	rtc.setDate(day, month, year);

//	 error = LoRaWAN.ON(uart);
//	 arduinoLoRaWAN::printAnswer(error);

}

// the loop function runs over and over again forever
void loop() {

	switch( _deviceState )
	{

		case DEVICE_STATE_INIT:
		{
			Watchdog.disable();
			Watchdog.enable(16000);
		    //////////////////////////////////////////////
		    // 1. Switch on
		    //////////////////////////////////////////////
		    error = LoRaWAN.ON(uart);

		    //Serial2.print("1. Switch on: ");
		    arduinoLoRaWAN::printAnswer(error);



		    error = LoRaWAN.setDataRate(0);
		    arduinoLoRaWAN::printAnswer(error);
		    //Serial2.print("LoRaWAN._dataRate: ");
		    //Serial2.println(LoRaWAN._dataRate);

		    error = LoRaWAN.setRetries(0);
		    arduinoLoRaWAN::printAnswer(error);
		    //Serial2.print("LoRaWAN._dataRate: ");
		    //Serial2.println(LoRaWAN._dataRate);

		    error = LoRaWAN.setADR("off");
		    arduinoLoRaWAN::printAnswer(error);

			_deviceState = DEVICE_STATE_JOIN;
			Watchdog.disable();
			break;
		}

		case DEVICE_STATE_JOIN:
		{
			Watchdog.disable();
			Watchdog.enable(16000);
			//////////////////////////////////////////////
			// 6. Join network
			//////////////////////////////////////////////
			error = LoRaWAN.joinOTAA();
			arduinoLoRaWAN::printAnswer(error);

			// Check status
			 if( error == 0 )
			 {
			   //2. Join network OK
//			   Serial.println("Join network OK");

				 Watchdog.reset();
			   _deviceState = DEVICE_STATE_SEND;
			 }
			 else
			 {
			   //2. Join network error
//				 Serial.println("Join network error");
				 Watchdog.reset();
				 delay(10000);
				 _deviceState = DEVICE_STATE_INIT;
			 }
			break;
		}

		case DEVICE_STATE_SEND:
		{
			uint8_t datab[4];
			char datas[9];
			data[0]='\0';

			int sensorValue = 0;
			digitalWrite(BAT_ADC_EN, HIGH);
			delay(500);
			sensorValue = analogRead(BAT_ADC);
			digitalWrite(BAT_ADC_EN, LOW);
//			Serial.print("ADC: ");
//			Serial.print(sensorValue);
//			Serial.print(" V: ");
			float v = ((float)sensorValue)*(0.0013427734375);
//			Serial.print(v);
//			Serial.print(" A: ");
			float a = ( ( (v-3) / 1.2) * 254 ) + 1;
			uint8_t level = 0;
			if(a<=0)
				level = 1;
			else if(a>=254)
				level = 254;
			else
				level = (uint8_t) a;
//			Serial.println(level);

			error = LoRaWAN.setBatteryLevel(level);
			arduinoLoRaWAN::printAnswer(error);

			float a_10 = ( ( (v) / 10) * 255 );
			uint8_t level_10 = 0;
			if(a_10<=0)
				level_10 = 0;
			else if(a_10>=255)
				level_10 = 255;
			else
				level_10 = (uint8_t) a_10;

			memcpy(&datab[0], &level_10, 1);
			sprintf(datas,"%02X", datab[0] & 0xff);
			strcat(data, datas);

			//////////////////////////////////////////////
			// 3. Send unconfirmed packet
			//////////////////////////////////////////////


		    error = LoRaWAN.setDataRate(0);
		    arduinoLoRaWAN::printAnswer(error);

			error = LoRaWAN.sendConfirmed(port, data);
			arduinoLoRaWAN::printAnswer(error);

			// Error messages:
			/*
			* '6' : Module hasn't joined a network
			* '5' : Sending error
			* '4' : Error with data length
			* '2' : Module didn't response
			* '1' : Module communication error
			*/
			// Check status
			if( error == 0 )
			{
			 //3. Send Confirmed packet OK
//			 Serial.println("Send Confirmed packet OK");
				err_tx=0;
			 if (LoRaWAN._dataReceived == true)
			 {
			   //There's data on
			   //port number: LoRaWAN._port
			   //and Data in: LoRaWAN._data
//			   Serial.println("Downlink data");
//			   Serial.print("LoRaWAN._port: ");
//			   Serial.println(LoRaWAN._port);
//			   Serial.print("LoRaWAN._data: ");
//			   Serial.println(LoRaWAN._data);

			   int number = (int)strtol(LoRaWAN._data, NULL, 16);

//			   Serial.println(number);

			   // r host reset
			   if(number == 114)
			   {
					digitalWrite(PIN_RESET, HIGH);
					delay(2000);
					digitalWrite(PIN_RESET, LOW);
			   }
			   // auto-reset
			   if(number == 97)
			   {
				   Watchdog.reset();
				   Watchdog.enable(1000);
				   delay(2000);
			   }

			 }
			}
			else if( error == 6 )
			{
			   _deviceState = DEVICE_STATE_JOIN;
			}
			else if( error == 2 )
			{
			   _deviceState = DEVICE_STATE_INIT;
			}
			else
			{
			 //3. Send Confirmed packet error
//			   Serial.println("Send Confirmed packet ERROR");
				if(err_tx++ >= 5)
				{
					   Watchdog.enable(1000);
					   delay(2000);
				}
			}
			digitalWrite(LED, HIGH);
			delay(300);
			digitalWrite(LED, LOW);

			error = LoRaWAN.sleep(2000000);
			arduinoLoRaWAN::printAnswer(error);

//			Serial.print("Start sleep: ");
//			Serial.println(++cont);
//			//Serial2.flush();
//			//Serial2.end();

			//rtc.setAlarmSeconds((rtc.getAlarmSeconds() + 30) % 60);
			rtc.setAlarmMinutes((rtc.getAlarmMinutes() + 5) % 60);
			rtc.enableAlarm(rtc.MATCH_MMSS);
			rtc.attachInterrupt(alarmMatch);

			digitalWrite(LED, LOW);
			// Disable the watchdog entirely by calling Watchdog.disable();
			Watchdog.disable();
			rtc.standbyMode();
//			delay(60000);
			Watchdog.enable(16000);


//			Serial.println("Exit sleep");

			error = LoRaWAN.wakeUP();
			arduinoLoRaWAN::printAnswer(error);

			error = LoRaWAN.check();
			arduinoLoRaWAN::printAnswer(error);
		}
	}

}

