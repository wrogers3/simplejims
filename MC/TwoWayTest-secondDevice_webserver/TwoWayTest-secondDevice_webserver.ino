/*WiFiAccessPoint.ino creates a WiFi access point and provides a web server on
  it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or
  http://192.168.4.1/L to turn it off OR Run raw TCP "GET /H" and "GET /L" on
  PuTTY terminal with 192.168.4.1 as IP address and 80 as port

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <WiFi.h>
#include <WiFiAP.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <esp_now.h>

#define LED_BUILTIN 2
#define closeGate1 10
#define openGate1 11
#define queryG1status 12
#define closeGate2 20
#define openGate2 21
#define queryG2status 22
#define ledPin \
	3  // Set the GPIO pin where you connected your test LED or comment this \
     // line out if your dev board has a built-in LED
String header;
// Set these to your desired credentials.
const char *ssid = "ESP32 c8:f0:9e:9b:88:7c";
const char *password = "Password";
WiFiServer server(80);

// REPLACE WITH THE MAC Address of your receiver
uint8_t broadcastAddress[] = {0x40, 0x22, 0xd8, 0x05, 0x2b, 0x98};

// Define variables to store incoming readings
int incomingMessage;

// Variable to store if sending data was successful
String success;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
	int message;
} struct_message;

struct_message incomingPacket;
struct_message outgoingPacket;
esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
	Serial.print("\r\nLast Packet Send Status:\t");
	Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success"
	                                              : "Delivery Fail");
	if (status == 0) {
		success = "Delivery Success :)";
	} else {
		success = "Delivery Fail :(";
	}
}
// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
	memcpy(&incomingPacket, incomingData, sizeof(incomingPacket));
	Serial.print("Bytes received: ");
	Serial.println(len);
	incomingMessage = incomingPacket.message;
}

bool g1;
bool g2;

void checkStatus(int gateNum) {
	esp_err_t result;
	if (gateNum == 1) {
		outgoingPacket.message = queryG1status;
	} else {
		outgoingPacket.message = queryG2status;
	}

	result = esp_now_send(broadcastAddress, (uint8_t *)&outgoingPacket,
	                      sizeof(outgoingPacket));
	checkResult(result);
}
bool checkResult(esp_err_t result) {
	if (result == ESP_OK) {
		Serial.println("Sent with success\n");
		return true;
	} else {
		Serial.println("Error sending the data\n");
		return false;
	}
}

void gateClose(int gateNum) {
	Serial.println("Gate " + String(gateNum) + " open command received\n");
	//
}

void gateOpen(int gateNum) {
	Serial.println("Gate " + String(gateNum) + " close command received\n");
}

void setup() {
	// close the gate associated with ESP when initializing
	g2 = false;
	// insert code here

	pinMode(LED_BUILTIN, OUTPUT);

	Serial.begin(115200);
	Serial.println();
	Serial.println("Configuring access point...");
	// Set device as a Wi-Fi Station and access point
	WiFi.mode(WIFI_AP_STA);
	// You can remove the password parameter if you want the AP to be open.
	WiFi.softAP(ssid, password);
	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.begin();

	Serial.println("Server started");

	// Init ESP-NOW
	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	} else {
    Serial.println("ESP_NOW initialized");
  }

	// Once ESPNow is successfully Init, we will register for Send CB to
	// get the status of Trasnmitted packet
	esp_now_register_send_cb(OnDataSent);
	// Register peer
	memcpy(peerInfo.peer_addr, broadcastAddress, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	// Add peer
	if (esp_now_add_peer(&peerInfo) != ESP_OK) {
		Serial.println("Failed to add peer");
		return;
	}
	// Register for a callback function that will be called when data is
	// received
	esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
	WiFiClient client = server.available();  // listen for incoming clients
	esp_err_t result;
	if (client) {                       // if you get a client,
		Serial.println("New Client.");  // print a message out the serial port
		String currentLine = "";

		// check status of gate 1
		checkStatus(1);

		// make a String to hold incoming data from the client
		while (client.connected()) {     // loop while the client's connected
			if (client.available()) {    // if there's bytes to read from the
				                         // client,
				char c = client.read();  // read a byte, then
				Serial.write(c);
				header += c;
				if (c == '\n') {  // if the byte is a newline character
					// if the current line is blank, you got two newline
					// characters in a row. that's the end of the client HTTP
					// request, so send a response:
					if (currentLine.length() == 0) {
						// HTTP headers always start with a response code (e.g.
						// HTTP/1.1 200 OK) and a content-type so the client
						// knows what's coming, then a blank line:
						client.println("HTTP/1.1 200 OK");
						client.println("Content-type:text/html");
						client.println("Connection: close");
						client.println();

						// response to query of gate 1
						if (incomingMessage == true) {
							Serial.println("query result: Gate is open\n");
							incomingMessage = -1;
							g1 = true;
						} else if (incomingMessage == false) {
							Serial.println("query result: Gate is closed\n");
							incomingMessage = -1;
							g1 = false;
						}

						if (incomingMessage == queryG1status) {
							outgoingPacket.message = g1;
						} else if (incomingMessage == queryG2status) {
							outgoingPacket.message = g2;
						}
						result = esp_now_send(broadcastAddress,
						                      (uint8_t *)&outgoingPacket,
						                      sizeof(outgoingPacket));
						if (checkResult(result)) {
							Serial.println(
							    "Successfully sent query response\n");
						}

						// if receive messsage from another device
						if (incomingMessage == openGate2) {
							Serial.println("Gate 2 opening (remote)");
							g2 = true;
							gateOpen(2);
							// do the code
						}
						if (incomingMessage = closeGate2) {
							Serial.println("Gate 2 closing (remote)");
							g2 = false;
							gateClose(2);
							// close the gate
						}

						// Opens gate 1
						if (header.indexOf("GET /gate1/open") >= 0) {
							Serial.println("Gate 1 opening (sent command)");
							outgoingPacket.message = openGate1;
							result = esp_now_send(broadcastAddress,
							                      (uint8_t *)&outgoingPacket,
							                      sizeof(outgoingPacket));
							if (checkResult(result)) {
								g1 = true;
								Serial.println("Gate 1 opened\n");
							}

						} else if (header.indexOf("GET /gate1/close") >= 0) {
							Serial.println("Gate 1 closing (sent command)");
							outgoingPacket.message = closeGate1;
							result = esp_now_send(broadcastAddress,
							                      (uint8_t *)&outgoingPacket,
							                      sizeof(outgoingPacket));
							if (checkResult(result)) {
								g1 = false;
								Serial.println("Gate 1 closed\n");
							}
						} else if (header.indexOf("GET /gate2/open") >= 0) {
							gateOpen(2);
						} else if (header.indexOf("GET /gate2/close") >= 0) {
							gateClose(2);
						}

						// Display the HTML web page
						client.println("<!DOCTYPE html><html>");
						client.println(
						    "<head><meta name=\"viewport\" "
						    "content=\"width=device-width, initial-scale=1\">");
						client.println("<link rel=\"icon\" href=\"data:,\">");
						// CSS to style the on/off buttons
						// Feel free to change the background-color and
						// font-size attributes to fit your preferences
						client.println(
						    "<style>html { font-family: Roboto; display: "
						    "inline-block; margin: 0px auto; text-align: "
						    "center;}");
						client.println(
						    ".button { background-color: #4CAF50; border: "
						    "none; color: white; padding: 16px 40px;");
						client.println(
						    "text-decoration: none; font-size: 30px; margin: "
						    "2px; cursor: pointer;}");
                client.println(".button2 {background-color: #555555;}</style></head>");

						// Web Page Heading
						client.println("<body><h1>Simple Jim's</h1>");

						// Display current state, and ON/OFF buttons for GPIO 26
						String state;
						if (g1) {
							state = "Open";
						} else {
							state = "closed";
						}
						client.println("<p>Gate 1 - State " + state + "</p>");
						// If the output26State is off, it displays the ON
						// button
						if (state == "closed") {
							client.println(
							    "<p><a href=\"/gate1/on\"><button "
							    "class=\"button\">CLOSE</button></a></p>");
						} else {
							client.println(
							    "<p><a href=\"/gate1/off\"><button "
							    "class=\"button "
							    "button2\">OPEN</button></a></p>");
						}

						if (g2) {
							state = "Open";
						} else {
							state = "closed";
						}
						// Display current state, and ON/OFF buttons for GPIO 27
						client.println("<p>Gatee 2 - State " + state + "</p>");
						// If the output27State is off, it displays the ON
						// button
						if (state == "closed") {
							client.println(
							    "<p><a href=\"/gate2/on\"><button "
							    "class=\"button\">OPEN</button></a></p>");
						} else {
							client.println(
							    "<p><a href=\"/gate2/off\"><button "
							    "class=\"button "
							    "button2\">CLOSE</button></a></p>");
						}
						client.println("</body></html>");

						// The HTTP response ends with another blank line
						client.println();
						// Break out of the while loop
						break;
					} else {  // if you got a newline, then clear currentLine
						currentLine = "";
					}
				} else if (c != '\r') {  // if you got anything else but a
					                     // carriage return character,
					currentLine += c;    // add it to the end of the currentLine
				}
			}
		}
		// Clear the header variable
		header = "";
		// Close the connection
		client.stop();
		Serial.println("Client disconnected.");
		Serial.println("");
	}
}

