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

int wifiChanged = 1;

/*#########################################################################
 * 
 * 	Run inital Setup Stuff
 * 
 * 
 * #######################################################################*/

void setup(void) 
{
	// start serial port 
	Serial.begin(9600); 
	delay(500);
	Serial.println("");	Serial.println("");	Serial.println("");
	Serial.println("==== Los Angeles Community Fridge - Monitor System ===="); 
	sensors.begin();
	
	
	EEPROM.begin(512);

//------------------- Get wifi credentals from memory
	
	wifiSSID = read_String(0,33);
	wifiPASS = read_String(64,64);
	
//------------------- Start wifi	
	
	WiFi.mode(WIFI_STA);
} 

/*#########################################################################
 * 
 * 
 * #######################################################################*/

String curtemp;
int tempcorrect = 0;
unsigned long timerRead;
unsigned long timerInput;

String incomingMode;
String incomingData;
char incoming;
int incomingSegment = 0;
int incomingCTR = 0;

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
	
//-------------------------------------------- 	

	if(abs(millis() - timerRead) > 5000)
	{
		timerRead = millis();
		
		Serial.println("");
		Serial.print("Reading temperatures..."); 
		sensors.requestTemperatures(); // Send the command to get temperature readings
		Serial.print("Temperature is: "); 
		
		curtemp = sensors.getTempFByIndex(0);
		
		if(curtemp.toInt() < -100)
		{
			Serial.println("Error");
			tempcorrect = 0;
		}
		else
		{
			tempcorrect = 1;
			Serial.println(curtemp);
		}

	//-------------------------------------------- Upload if the Wifi is Connected
		
		if(WiFi.status() == WL_CONNECTED)
		{
			Serial.println("Wifi Connected");

		}
		else
		{
			Serial.print("Cannot Connect to network \""); Serial.print(wifiSSID.c_str()); Serial.println("\"");
		}
	}
	

	
	/*

	
	if(WiFi.status() == WL_CONNECTED)
	{
		Serial.println("Connected");
		
		HTTPClient http;
		http.begin("http://extra.georgecoffey.com/LACF/monitor.php?temp=" + curtemp);  //Specify request destination
		int httpCode = http.GET();                                                                  //Send the request
		
		if (httpCode > 0)
		{ //Check the returning code
		
			String payload = http.getString();   //Get the request response payload
			Serial.println(payload);                     //Print the response payload
		
		}
		
		http.end();   //Close connection
	}
	
	
	*/
	
//-------------------------------------------- Check for incoming configuration stuff from the Serial Port

	if(Serial.available())
	{
		timerInput = millis();
		
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
