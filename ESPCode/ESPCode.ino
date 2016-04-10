#define ESP8266_LED 5

#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SoftwareSerial.h>
#include <ParallaxLCD.h>

unsigned long tempUpdateTime = 0;
float currentTemp = 85.0;
byte ledStatus = LOW;

//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "Bill Wi the Science Fi";
const char WiFiPSK[] = "#######";

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing
const int DIGITAL_PIN = 12; // Digital pin to be read

WiFiServer server(80);

#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

ParallaxLCD lcd(14,2,16); // desired pin, rows, cols

void setup() 
{
  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);
  
  initHardware();
  lcd.setup();
  server.begin();
  
  lcd.backLightOn();
  lcd.empty();
}

void loop() 
{
  unsigned long currentMillis = millis();
  if (currentMillis - tempUpdateTime >= 1500) {
    tempUpdateTime = currentMillis;
    do {
      DS18B20.requestTemperatures(); 
      currentTemp = DS18B20.getTempCByIndex(0);
    } while (currentTemp == 85.0 || currentTemp == (-127.0));
    currentTemp = currentTemp * 9.0/5 + 32;
    //lcd.empty();
    lcd.at(0,2,"Temperature: ");
    lcd.at(1,4,String(currentTemp) + " F");
  }
  
  if(WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, LOW); // Write LED high/low
  } else {
    digitalWrite(LED_PIN, HIGH);
  }
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  } else {
    respondToWebRequest(&client);
  }
  
}

void initHardware()
{
  Serial.begin(115200);
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // Don't need to set ANALOG_PIN as input, 
  // that's all it can be.
}

void respondToWebRequest(WiFiClient * clientP){
  WiFiClient client = *clientP;
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  int val = -1; // We'll use 'val' to keep track of both the
                // request type (read/set) and value if set.
  if (req.indexOf("/led/0") != -1)
    val = 0; // Will write LED low
  else if (req.indexOf("/led/1") != -1)
    val = 1; // Will write LED high
  else if (req.indexOf("/read") != -1)
    val = -2; // Will print pin reads
  // Otherwise request will be invalid. We'll say as much in HTML

  // Set GPIO5 according to the request
  if (val >= 0)
    digitalWrite(LED_PIN, val);

  client.flush();

  // Prepare the response. Start with the common header:
  String s = "HTTP/1.1 200 OK\r\n";
  s += "Content-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE HTML>\r\n<html>\r\n";

  s += "Temperature = ";
  s += String(currentTemp);
  s += " F<br>";
  
  // If we're setting the LED, print out a message saying we did
  if (val >= 0)
  {
    s += "LED is now ";
    s += (val)?"on":"off";
  }
  else if (val == -2)
  { // If we're reading pins, print out those values:
    s += "Analog Pin = ";
    s += String(analogRead(ANALOG_PIN));
    s += "<br>"; // Go to the next line.
    s += "Digital Pin 12 = ";
    s += String(digitalRead(DIGITAL_PIN));
  }
  else
  {
    s += "Invalid Request.<br> Try /led/1, /led/0, or /read.";
  }
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}


