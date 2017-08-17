// include Bluetooth Low Energy library for Arduino/Genuino 101 
#include <CurieBLE.h>
#include <AccessoryShield.h>

#define SKETCH_DEBUG

#define LED_NUMBER 3

// create a new array setting the initial state of leds :
// when the sketch starts the red, green and blue leds are OFF
byte RGBledState[] = {0, 0, 0};

String ledColor[3] = {"red", "green", "blue"};

// create a new BLE service with a custom UUID with 128 byte size
BLEService RGBservice("21D7422C-C808-4A6C-94D8-22812427B7D6");

// create a new BLE Characteristic with the following settings :
// 1) a custom characteristic UUID (128 byte)
// 2) Characteristic Properties : Read, Write and Notify
// 3) the maximum lenght of data for the Characteristic Value
BLECharacteristic RGBchar("21D7422D-C808-4A6C-94D8-22812427B7D6",
                          BLERead | BLEWrite | BLENotify,
                          sizeof(RGBledState));

void setup() {
  Serial.begin(115200);
  while(!Serial) ;

  accessoryShield.begin();

  BLE.begin();

  BLE.setLocalName("RGBled");
  BLE.setDeviceName("Genuino101");
  BLE.setAdvertisedService(RGBservice);
  BLE.setEventHandler(BLEConnected, BleConnectedCallback);
  BLE.setEventHandler(BLEDisconnected, BleDisconnectedCallback);

  // Add the RGBchar Characteristic to RGBservice
  RGBservice.addCharacteristic(RGBchar);

  // Add the RGBservice Serivce to Attribute Table
  BLE.addService(RGBservice);
  
  RGBchar.setValue(RGBledState, LED_NUMBER);
  RGBchar.setEventHandler(BLEWritten, BleCharWrittenCallback);
  RGBchar.setEventHandler(BLESubscribed, BleCharSubscribedCallback);
    
  Serial.print("RGBled Service UUID: ");
  Serial.println(RGBservice.uuid());

  Serial.print("RGBled Characteristic UUID: ");
  Serial.println(RGBchar.uuid());
  Serial.println("Initial Characteristic Value :");
  for(int val = 0; val < LED_NUMBER; val++) {
    Serial.print("value ");
    Serial.print(val+1);
    Serial.print(" = ");
    Serial.println(RGBchar[val]);
  }

  BLE.advertise();
  Serial.println("Listening for central devices and waiting for connections ...");

   Serial.println("\nUpdate RGB leds'state using the following sintax :");
   Serial.println("red <redState> green <greenState> blue <blueState>");
   Serial.println("Where <ledState>, <greenState>, <blueState> are values from 0 to 255");
}

void loop() {
  // poll peripheral listening for events
  BLE.poll();

  bool updateRGB = true;
  
  while(Serial.available() > 0) {
    Serial.println("\nReading data from the Serial monitor ...");
    for(int led = 0; led < LED_NUMBER; led++) {
      String stringRead = Serial.readStringUntil(' ');
      Serial.print(stringRead);
      Serial.print(" = ");
      if(stringRead.equals(ledColor[led])) {
        stringRead = Serial.readStringUntil(' ');
        RGBledState[led] = stringRead.toInt();
        Serial.println(RGBledState[led]);
      }
      else {
        updateRGB = false;
        break;
      }
    }
    if(updateRGB) {
      accessoryShield.setRGB(RGBledState[0], RGBledState[1], RGBledState[2]);
      RGBchar.setValue(RGBledState, LED_NUMBER);
      break;
    }
  }
}

void BleConnectedCallback(BLEDevice central) {
  Serial.print("\nGenuino 101 (Peripheral) connected to Central device ");
  Serial.println(central.address());
}

void BleDisconnectedCallback(BLEDevice central) {
  Serial.print("\nGenuino 101 (Peripheral) disconnected from Central device");
  Serial.println(central.address());
}

void BleCharWrittenCallback(BLECentral &central, BLECharacteristic &characteristic) {
  const unsigned char *valuesUpdated = characteristic.value();
  unsigned int dataLength = characteristic.valueLength();
  Serial.print("\nCentral device sent ");
  Serial.print(dataLength);
  Serial.println(" values");
  if(dataLength == LED_NUMBER) {
    Serial.println("RGB led values received: ");
    for(int8_t led = 0; led < LED_NUMBER; led++) {
      RGBledState[led] = valuesUpdated[led];
      Serial.print(ledColor[led]);
      Serial.print(" led state : ");
      Serial.println(RGBledState[led]);
    }
    Serial.println("Updating led states ...");
    accessoryShield.setRGB(RGBledState[0], RGBledState[1], RGBledState[2]);
    
  }
  else {
    Serial.println("Invalid number of data received");
  }
}

void BleCharSubscribedCallback(BLEDevice central, BLECharacteristic characteristic) {
  Serial.println("\nNotification service to get data from the central device");
}

