/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.
 
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

#define LEFT_F  27 // Motor A (left)  forward pin (IN1)
#define LEFT_R  26  // Motor A (left)  reverse pin (IN2)
#define RIGHT_F 33  // Motor B (right) forward pin (IN3)
#define RIGHT_R 25  // Motor B (right) reverse pin (IN4)

// NOTE: depending on how you attach your motors,
// you may need to swap forward and reverse pins.

// Motor enables (to control power)
#define LEFT_SPEED 14   // left enable pin  (ENA)
#define RIGHT_SPEED 32   // right enable pin (ENB)

#define MAX_SPEED 255
 
// Set these to your desired credentials.
const char *ssid = "yourAP";
const char *password = "yourPassword";
 
WiFiServer server(80);
 
 
void setup() { 
  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");
 
  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Setting pins for motors");

    // Declare the mode of our pins
    pinMode(LEFT_F, OUTPUT);
    pinMode(LEFT_R, OUTPUT);
    pinMode(RIGHT_R, OUTPUT);
    pinMode(RIGHT_F, OUTPUT);

    pinMode(LEFT_SPEED, OUTPUT);
    pinMode(RIGHT_SPEED, OUTPUT);
 
  Serial.println("Server started");
}
void open_door();
void close_door();

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
            client.print("Click <a href=\"/H\">here</a> to open door.<br>");
            client.print("Click <a href=\"/L\">here</a> to close door.<br>");
 
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
          open_door();
        }
        if (currentLine.endsWith("GET /L")) {
          close_door();               // GET /L turns the LED off
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
  //digitalWrite(LEFT_SPEED, HIGH);
  // open_door();
  // delay(3000);
  // close_door();
  // delay(2000);
}

void drive(int left_speed, int right_speed) {
    // Set left direction
    if (left_speed < 0) {
        left_speed = -left_speed;
        digitalWrite(LEFT_F, LOW);
        digitalWrite(LEFT_R, HIGH);
    } else {
        digitalWrite(LEFT_F, HIGH);
        digitalWrite(LEFT_R, LOW);
    }
    // Set right direction
    if (right_speed < 0) {
        right_speed = -right_speed;
        digitalWrite(RIGHT_F, LOW);
        digitalWrite(RIGHT_R, HIGH);
    } else {
        digitalWrite(RIGHT_F, HIGH);
        digitalWrite(RIGHT_R, LOW);
    }
    // Set left and right motor speeds
    if (left_speed) {
      digitalWrite(LEFT_SPEED, HIGH);
    } else {
      digitalWrite(LEFT_SPEED, LOW);
    }
    if (right_speed) {
      digitalWrite(RIGHT_SPEED, HIGH);
    } else {
      digitalWrite(RIGHT_SPEED, LOW);
    }
    // analogWrite(LEFT_SPEED, left_speed);
    // analogWrite(RIGHT_SPEED, right_speed);
}

void open_door() {
    drive(MAX_SPEED, MAX_SPEED);
    delay(250); // Wait 1 second
    drive(0,0);
    delay(250); // Wait 1 second
}

void close_door() {
    drive(-MAX_SPEED, -MAX_SPEED);
    delay(250); // Wait 1 second
    drive(0,0);
    delay(250); // Wait 1 second
}
