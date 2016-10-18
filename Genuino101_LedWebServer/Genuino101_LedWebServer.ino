/*
 Genuino101_LedWebServer

 Use your Arduino/Genuino 101 with the Ethernet Shield to create a
 Web Server to control the led connected to digital pin 3 

 Hardware:
 * Arduino/Genuino 101
 * Ethernet Shield
 * led attached to digital pin 3 (the led use a pull-down resistor)

 created 10 Sept 2016
 by Biagio Montaruli

 this code is in the public domain

 */
#include <SPI.h>
#include <Ethernet.h>

// MAC Address of Ethernet Shield
static byte MacAddress[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Instatiate a new object that represent an Ethernet Server
// listening on port 80 for client requests
// In this way Arduino/genuino acts as an Ethernet Server !
unsigned int ethPort = 80;
EthernetServer server(ethPort);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// if useDHCP is set to true we use DHCP to get the IP address
// otherwise we set a static IP
bool useDHCP = true;

// chip select (Slave Select) for the SD card slot is pin 4
const uint8_t SdChipSelect = 4;

// String object to store the Web Browser (Client) HTTP Request
String HttpRequest = "";

// led pin is digital pin 3
const byte ledPin = 3;
// initial state of led
bool ledState = LOW;

void setup() {
  // initialize Serial communication
  Serial.begin(9600);
  // wait for the Serial port to open. Needed for native USB only
  while(!Serial) ;
  // set the Slave Select pin for the SD card of the Ethernet Shield
  // as OUTPUT and turn HIGH the Slave Select pin in order to not
  // use the SD card
  pinMode(SdChipSelect, OUTPUT);
  digitalWrite(SdChipSelect, HIGH);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  // Initializes the ethernet library and network settings
  // Try to set up a new Ethernet connection and obtain a new IP address
  // from the DHCP server. Passing only the MAC address of the
  // Ethernet Shield to begin() method use the DHCP server to obtain
  // the IP address of Genuino 101 with the Ethernet Shield.
  // The begin() method returns 1 on a successful DHCP connection, 
  // or 0 on failure.
  if((Ethernet.begin(MacAddress) == 1) && (useDHCP == true)) {
    Serial.println("New Ethernet connection successfully initialized !");
    Serial.println("Printing info about the new Ethernet connection ...");
    Serial.print("IP address of Genuino 101 Web Server = ");
    Serial.println(Ethernet.localIP());
    Serial.print("IP address of DNS Server = ");
    Serial.println(Ethernet.dnsServerIP());
    Serial.print("Gateway IP = ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("Subnet Mask = ");
    Serial.println(Ethernet.subnetMask());
  }
  else {
    Serial.println("Failed to configure a new Ethernet connection through DHCP !");
    Serial.println("Configuring the new connection using a static IP ...");
    // using a static IP : 192.168.1.25
    // NOTE : set the static IP according your local ethernat configuration
    IPAddress staticIP(192, 168, 1, 25);
    Ethernet.begin(MacAddress, staticIP);
    Serial.print("Configuring the new Ethernet connection using the static IP ...");
    Serial.print("Static IP address of your Genuino 101 Web Server = ");
    for(uint8_t octet = 0; octet < 4; octet++) {
      Serial.print(staticIP[octet]);
      if(octet != 3) {
        Serial.print("."); 
      }
      else {
        Serial.println();
      }
    }
    Serial.print("Gateway IP = ");
    Serial.println(Ethernet.gatewayIP());
    Serial.print("Subnet Mask = ");
    Serial.println(Ethernet.subnetMask());
  }
}

void loop() {
  // Genuino 101 (Server) listens for requests of new clients
  EthernetClient client = server.available();
  // if a new client want to connect ...
  if(client) {
    Serial.println("Getting the connection request from a new client ...");
    bool detectBlankLine = true; 
    // while the client is connected to the Genuino 101 server
    while (client.connected()) {

      // if the client has sent some data Genuino 101
      if (client.available()) {

        // read the incoming byte form the Client, print it in the Serial monitor
        // and store in the HttpRequest String
        char newData = client.read();
        Serial.write(newData);
        HttpRequest += newData;
        
        // the http request of the client ends when the server receives
        // a newline character followed by a blank line and then
        // you can send a response
        if((newData == '\n') && (detectBlankLine)) {
          // if the Client HTTP Request includes "LedState=LED+ON" and if we haven't already changed
          // the led state (turned it ON), turn the led ON
          if((HttpRequest.indexOf("LedState=LED+ON") > 0) && (ledState == LOW)) {
            Serial.println("Turning the led ON");
            ledState = HIGH;
            digitalWrite(ledPin, ledState);
          }
          // else the Client HTTP Request includes "LedState=LED+OFF" and if we haven't already changed
          // the led state (turned it OFF), turn the led OFF
          else if((HttpRequest.indexOf("LedState=LED+OFF") > 0) && (ledState == HIGH)) {
            Serial.println("Turning the built-in led OFF");
            ledState = LOW;
            digitalWrite(ledPin, ledState);
          }
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          // Type of the content of the 
          client.println("Content-Type: text/html");
          // the connection will be closed after completion of the response
          client.println("Connection: close");
          // refresh the page automatically every 5 sec
          client.println("Refresh: 5");
          client.println();
          // send to the Web Browser (Client) a Web Page with two buttons to change the led state
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.println("<head>");
          client.println("<title>Genuino 101 Web Server</title>");
          client.println("</head>");
          client.println("<body>");
          client.println("<h1>Web Server to control the built-in led of Genuino 101 board</h1>");
          client.println("<p>Use the checkbox to control the built-in led status</p>");
          client.println("<form method=\"get\">");
          client.println("<input type=\"submit\" name=\"LedState\" value=\"LED ON\">");
          client.println("<input type=\"submit\" name=\"LedState\" value=\"LED OFF\">");
          client.println("</form>");
          client.println("</body>");
          client.println("</html>");
          // break connection with the client
          HttpRequest = "";
          break;
        }

        if(newData == '\n') {
          detectBlankLine = true;
        }
        else if(newData != '\r') {
          detectBlankLine = false;
        }
      }
    }
    // client has been disconnected ...
    // wait for the browser to receive data send by Genuino 101 Server
    delay(1);
    // close the connection with the client
    client.stop();
    Serial.println("Client disconnected");
  }
}
