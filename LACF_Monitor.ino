/*#########################################################
 * 	Los Angeles Community Fridge - Monitoring system
 * 
 * 	Version 0.0.1
 * 
 * 
 * 
 * 
 * 
 * ##########################################################*/

// include libraries
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

#include <OneWire.h> 
#include <DallasTemperature.h>

#include <EEPROM.h>




// Data wire is plugged into pin 4 on the Arduino-ESP8266
#define ONE_WIRE_BUS 4
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


String wifiSSID = "";
String wifiPASS = "";
String macString = "";

int wifiChanged = 1;
byte mac[6];



//--------------------------------------------------------- Web Server Setup

const char *host = "extra.georgecoffey.com";
const int httpsPort = 443;  //HTTPS= 443 and HTTP = 80

//------------------------------------- SHA1 finger print of certificate use web browser to view and copy

const char fingerprint[] PROGMEM = "F1 B1 EA AA C9 6C CC 51 6D A9 82 A6 75 B7 92 DB D1 7F 05 95";




/*#########################################################################
 * 
 * 	Run inital Setup Stuff
 * 
 * 
 * #######################################################################*/

void setup(void) 
{
//--------------------------------------------------------- Set Wifi Mode	
	WiFi.mode(WIFI_OFF);
	delay(1000);
	WiFi.mode(WIFI_STA);
	
//------------------- Start Serial connection
	
	Serial.begin(9600); 
	delay(100);
	Serial.println("");	Serial.println("");	Serial.println("");
	Serial.println("==== Los Angeles Community Fridge - Monitor System ===="); 
	
//------------------- Initialize Sensor interface
	
	sensors.begin();
	
//------------------- Setup EEPROM (Flash) connection
	
	EEPROM.begin(512);

//------------------- Get wifi credentals from memory
	
	wifiSSID = read_String(0,33);
	wifiPASS = read_String(64,64);
	
//------------------- Get device MAC address as something unique to send
	
	WiFi.macAddress(mac);

	char add[2];
	for (int i=0; i < 6; i++)
	{
		sprintf(add, "%02X", mac[i]);
		macString += add;
		if(i < 5 )
		{
			macString += ':';
		}
	}
	Serial.print("Device Mac Address: ");
	Serial.println(macString);
	
} 

/*#########################################################################
 * 
 * 
 * #######################################################################*/

String curtemp;
int tempcorrect = 0;
unsigned long timerRead;
unsigned long timerSend;
unsigned long timerInput;
unsigned long timerSendTimeout;

int sending = 0;

String incomingMode;
String incomingData;
char incoming;
int incomingSegment = 0;
int incomingCTR = 0;


WiFiClientSecure httpsClient;
int sendRetry;
int incomingRead = 0;

void loop(void) 
{
	
	
//-------------------------------------------- Connect or change wifi information

	if(wifiChanged == 1)
	{
		Serial.println("");
		Serial.print("Wifi Network is \"");	Serial.print(wifiSSID);	Serial.println("\"");
		Serial.print("Wifi Password is \"");	Serial.print(wifiPASS);	Serial.println("\"");
		WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
		wifiChanged = 0;
	}
	
//-------------------------------------------- Check the temperature every 30 seconds

	if(abs(millis() - timerRead) > 10000)
	{
		timerRead = millis();
		
		sensors.requestTemperatures(); // Send the command to get temperature readings
		Serial.print("Temperature is: "); 
		
		curtemp = sensors.getTempFByIndex(0);
		
		if(curtemp.toInt() < -100)
		{
			Serial.print("Error");
			tempcorrect = 0;
		}
		else
		{
			tempcorrect = 1;
			Serial.print(curtemp);
		}
		Serial.print(" - ");

	//-------------------------------------------- Check if the Wifi is Connected and output that status
		
		if(WiFi.status() == WL_CONNECTED)
		{
			Serial.print("Wifi Connected to network \""); Serial.print(wifiSSID.c_str()); Serial.println("\"");

		}
		else
		{
			Serial.print("Trying to connect to network \""); Serial.print(wifiSSID.c_str()); Serial.println("\"");
		}
		
		Serial.println("");
	}
	

//-------------------------------------------- Send the temperature via wifi every 5 minutes

	if(WiFi.status() == WL_CONNECTED)
	{
		if(abs(millis() - timerSend) > 60000 && sending == 0)
		{
			timerSend = millis();
			timerSendTimeout = millis();
			
		//-------------------------------------------------- Do the inital interface setup
			
			Serial.print("Sending data to server - 1");
			//httpsClient.setFingerprint(fingerprint);
			httpsClient.setInsecure();
			httpsClient.setTimeout(15000); // 15 Seconds
			sending = 1;
			sendRetry = 0;
		}

	//----------------------------------------- Mode 1 - Setup inital HTTPS connection, or timeout
	
		if(sending == 1)
		{
			if(!httpsClient.connect(host, httpsPort) && (sendRetry < 30))
			{
				Serial.println("trying to connect ...");
				sendRetry++;
			}
			else if (sendRetry < 30)
			{
				sending = 2;
			}
			
			if(sendRetry==30)
			{
				Serial.println("Connection failed");
				sending = 0;
			}
		}
		
	//----------------------------------------- Mode 2 - Once connected setup the data that should go to the url and start to send it
	
		if(sending == 2)
		{
			String Link = "/LACF/monitor.php?temp=" + curtemp + "&network=" + wifiSSID + "&mac=" + macString;  //Specify request destination

			Serial.print("requesting URL: ");
			Serial.println(host+Link);

			httpsClient.print(String("GET ") + Link + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

			Serial.println("request sent");
			sending = 3;
		}
	
	//----------------------------------------- Mode 3 - Run the header connection stuff
	
		if(sending == 3)
		{
			if(httpsClient.connected())
			{
				String line = httpsClient.readStringUntil('\n');
				if (line == "\r")
				{
					Serial.println("headers received - Getting main data");
					sending = 4;
				}
			}
		}
		
		if(sending == 4)
		{
			String line;
			if(httpsClient.available())
			{        
				line = httpsClient.readStringUntil('\n');  //Read Line by Line
				Serial.println(line); //Print response
			}
			else
			{
				sending = 5;	
			}
		}
		
		if(sending == 5)
		{
			Serial.println("==========");
			Serial.println("closing connection");
			sending = 0;
		}
	}
	
//-------------------------------------------- Clear current connection if it's taking too long
	
	if(abs(millis() - timerSendTimeout) > 180000 && sending > 0)
	{
		Serial.println("###### UNABLE TO SEND DATA - TIMEOUT ###");
		sending = 0;
	}
	
//-------------------------------------------- Check for incoming configuration stuff from the Serial Port

	incomingRead = 0;
	while(Serial.available() && incomingRead < 30)
	{
		timerInput = millis();
		incomingRead++;
		
		incoming = Serial.read();
		if(incoming == '\n')
		{
			if(incomingMode == "SSID")
			{
				Serial.print("Setting SSID to ");
				Serial.println(incomingData);
				
				wifiSSID = incomingData;
				
				save_String(wifiSSID, 0);
				wifiChanged = 1;
			}
			else if(incomingMode == "PASS" || incomingMode == "PASSWORD")
			{
				Serial.print("Setting password to ");
				wifiPASS = incomingData;
				
				Serial.println(wifiPASS);
				save_String(wifiPASS, 64);
				wifiChanged = 1;
			}
			else
			{
				Serial.println("Unknown input");
			}
			incomingSegment = 0;
			incomingCTR = 0;
			incomingMode = "";
			incomingData = "";
		}
		else if(incoming == ':')
		{
			incomingSegment = 1;
			incomingCTR = 0;
		}
		else
		{
			if(incomingSegment == 0)
			{
				incomingMode += incoming;
			}
			else if(incomingSegment == 1)
			{
				incomingData = incomingData + incoming;
			}
		}
		incomingCTR++;
	}
	
	if(abs(millis() - timerInput) > 2000)
	{
		incomingSegment = 0;
		incomingCTR = 0;
		incomingMode = "";
		incomingData = "";
	}
} 


String read_String(char add, int maxlen)
{
	String out;
	
	int len = 0;
	char k;
	k=EEPROM.read(add);
	while(k > 32 && len < maxlen)   //Read until null character
	{    
		//Serial.print(add+len); Serial.print(" - "); Serial.print(k); Serial.print(" - ");	Serial.println(k, HEX);
		out += k;
		len++;
		k=EEPROM.read(add+len);
	}
	
	return out;
}

void save_String(String value, int address)
{
	int i;
	for(i=0; i<value.length(); i++)
	{
		EEPROM.write(address + i, value[i]);
	}
	EEPROM.write(address + i, 30);
	EEPROM.commit(); 
}
