// include Bluetooth Low Energy library for Arduino/Genuino 101 
#include <CurieBLE.h>
#include <AccessoryShield.h>

#define SKETCH_DEBUG

#define LED_NUMBER 3

// create a new array setting the initial state of leds :
// when the sketch starts the red, green and blue leds are OFF
unsigned char RGBledState[] = {0, 0, 0};

char *ledName[] = {"Red", "Green", "Blue"};

BLEPeripheral Genuino101Peripheral;

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

  Genuino101Peripheral.setLocalName("RGBled");
  Genuino101Peripheral.setDeviceName("Genuino101");
  Genuino101Peripheral.setAdvertisedServiceUuid(RGBservice.uuid());
  Genuino101Peripheral.setAppearance(BLE_GAP_APPEARANCE_TYPE_GENERIC_COMPUTER);
  Genuino101Peripheral.setEventHandler(BLEConnected, BleConnectedCallback);
  Genuino101Peripheral.setEventHandler(BLEDisconnected, BleDisconnectedCallback);

  // Add the RGBservice Serivce to Attribute Table
  Genuino101Peripheral.addAttribute(RGBservice);
  // Add the RGBchar Characteristic to Attribute Table
  Genuino101Peripheral.addAttribute(RGBchar);

  
  RGBchar.setValue(RGBledState, LED_NUMBER);
  RGBchar.setEventHandler(BLEWritten, BleCharWrittenCallback);
  RGBchar.setEventHandler(BLESubscribed, BleCharSubscribedCallback);
  RGBchar.setEventHandler(BLEUnsubscribed, BleCharUnsubscribedCallback);
  
  Serial.println("BLE RGBled Service");
  Serial.print("RGBled Service UUID = ");
  Serial.print(RGBservice.uuid());

  Serial.println("\nBLE RGBled Characteristic");
  Serial.print("RGBled Characteristic UUID = ");
  Serial.println(RGBchar.uuid());
  Serial.println("Initial Characteristic Values :");
  for(int val = 0; val < LED_NUMBER; val++) {
    Serial.print("Value ");
    Serial.print(val);
    Serial.print(" = ");
    Serial.println(RGBchar[val]);
  }

  Genuino101Peripheral.begin();
  Serial.println("\nActivating Genuino 101 BLE ...");
  Serial.println("Listening for central devices and waiting for connections ...");

#ifdef SKETCH_DEBUG
  Serial.print("\nAdvertising packet length = ");
  Serial.println(Genuino101Peripheral.getAdvertisingLength());
  Serial.print("Advertising data = ");
  Serial.println((char *)Genuino101Peripheral.getAdvertising());
#endif

   Serial.println("\nUpdate RGB leds'state using the following sintax :");
   Serial.println("red <redState> green <greenState> blue <blueState>");
   Serial.println("Where <ledState>, <greenState>, <blueState> are values from 0 to 255");
}

void loop() {
  // poll peripheral listening for events
  Genuino101Peripheral.poll();

  bool updateRGB = true;
  
  while(Serial.available() > 0) {
    Serial.println("\nReading data from the Serial monitor ...");
    for(int led = 0; led < LED_NUMBER; led++) {
      String stringRead = Serial.readStringUntil(' ');
      Serial.print(stringRead);
      Serial.print(" = ");
      String ledColor = String(ledName[led]);
      if(stringRead.equals(ledColor)) {
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

void BleConnectedCallback(BLECentral &central) {
  Serial.println("\nGenuino 101 peripheral connected to central device");
  Serial.print("Bluetooth device address for central device is ");
  Serial.println(central.address());
}

void BleDisconnectedCallback(BLECentral &central) {
  Serial.println("\nGenuino 101 peripheral disconnected from central device");
  Serial.print("Bluetooth device address for central device is ");
  Serial.println(central.address());
}

void BleCharWrittenCallback(BLECentral &central, BLECharacteristic &characteristic) {
  const unsigned char *valuesUpdated = characteristic.value();
  unsigned int dataLength = characteristic.valueLength();
  Serial.print("\nRGBled Characteristic has ");
  Serial.print(dataLength);
  Serial.println(" values");
  if(dataLength == LED_NUMBER) {
    Serial.println("RGB led values written by central device : ");
    for(int8_t led = 0; led < LED_NUMBER; led++) {
      RGBledState[led] = valuesUpdated[led];
      Serial.print(ledName[led]);
      Serial.print(" led state : ");
      Serial.println(RGBledState[led]);
    }
    Serial.println("Setting the led states ...");
    accessoryShield.setRGB(RGBledState[0], RGBledState[1], RGBledState[2]);
    
  }
  else {
    Serial.println("Invalid number of data received");
  }
}

void BleCharSubscribedCallback(BLECentral &central, BLECharacteristic &characteristic) {
  Serial.println("\nNotification service to get data from the central device updated");
}

