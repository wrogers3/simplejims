/*WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.
 
  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
     OR
     Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port
 
  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/
 
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

const char *ssid = "SimpleJims";
const char *password = "password";
 
WiFiServer server(80);
 
#define MOTORPIN1 27
#define MOTORPIN2 26
#define ENABLE1 14

// hard code moment
const int freq = 30000;
const int pwmChannel = 0;
const int resolution = 8;
int dutyCycle = 200;
 
void setup() {
  pinMode(MOTORPIN1, OUTPUT);
  pinMode(MOTORPIN2, OUTPUT);
  pinMode(ENABLE1, OUTPUT);

  ledcSetup(pwmChannel, freq, resolution);
  ledcAttachPin(ENABLE1, pwmChannel);
 
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");
 
  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
 
  Serial.println("Server started");
}
 
void loop() {
  WiFiClient client = server.available();   // listen for incoming clients
 
  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
 
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
 
            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> to OPEN the door.<br>");
            client.print("Click <a href=\"/L\">here</a> to CLOSE the door.<br>");
 
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
 
        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
	    // this is where you enable motor spin 
	    Serial.println("Moving one way");
	    digitalWrite(MOTORPIN1, LOW);
	    digitalWrite(MOTORPIN2, HIGH);
	    delay(2000);
	    //stopping
	    digitalWrite(MOTORPIN1, LOW);
	    digitalWrite(MOTORPIN2, LOW);

        }
        if (currentLine.endsWith("GET /L")) {
	    Serial.println("Moving the other way");
	    digitalWrite(MOTORPIN1, HIGH);
	    digitalWrite(MOTORPIN2, LOW);
	    delay(2000);
	    //stopping
	    digitalWrite(MOTORPIN1, LOW);
	    digitalWrite(MOTORPIN2, LOW);

        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}
