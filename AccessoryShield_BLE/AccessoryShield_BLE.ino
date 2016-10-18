#include <AccessoryShield.h>
#include <CurieBLE.h>

// #define SKETCH_DEBUG

#define TEMP_UNIT CELSIUS
#define LED_NUMBER 3
#define BUZZER_VALUES 3
#define DHT11_VALUES 4

// create a new array setting the initial state of leds :
// when the sketch starts the red, green and blue leds are OFF
unsigned char RGBledState[] = {0, 0, 0};
String ledColors[] = {"Red", "Green", "Blue"};

typedef struct {
  unsigned int freq;
  unsigned int delayTime;
  bool actived;
} BuzzerData;
BuzzerData buzzer = { 125, 2, false};
unsigned char buzzerValues[BUZZER_VALUES];

typedef struct {
  float temp;
  TemperatureUnit tempUnit;
  float humidity;
  float heatIndex;
} DHT11sensor;
DHT11sensor dht11;
unsigned char DHT11data[4];

unsigned int potValue;

JoystickMode joystickOldState, joystickNewState;
String joystickModes[] = {"NONE", "UP", "DOWN", "LEFT", "RIGHT", "PUSHED", "NONE_OR_DOWN"};

typedef struct {
  JoystickMode joystickValue;
  String joystickStringState;
} JoystickData;

bool relayState;

BLEPeripheral Genuino101Peripheral;

BLEService AccessoryShieldControl("6BC62B12-E4CE-41C3-9906-475A843245CA");

BLECharacteristic RGBled("6BC62B12-E4CE-41C3-9906-475A843245CA",
                         BLEWrite | BLERead,
                         sizeof(RGBledState));

BLECharacteristic DHT11("6BC62B15-E4CE-41C3-9906-475A843245CA",
                        BLERead | BLENotify,
                        sizeof(DHT11data));

BLECharacteristic Buzzer("6BC62B18-E4CE-41C3-9906-475A843245CA",
                         BLEWrite | BLERead,
                         sizeof(Buzzer));

BLEUnsignedCharCharacteristic Pot("6BC62B1b-E4CE-41C3-9906-475A843245CA",
                                 BLERead | BLENotify);

BLEUnsignedCharCharacteristic Joystick("6BC62B1e-E4CE-41C3-9906-475A843245CA",
                                       BLERead | BLENotify);

BLEUnsignedCharCharacteristic Relay("6BC62B21-E4CE-41C3-9906-475A843245CA",
                                    BLERead | BLEWrite);

//float temp, humidity;
//unsigned int buzzerFreq;
//int dht11State;
//JoystickMode joystickOldValue, joystickNewValue;
//long timeCounter;

void setup() {
  // Initialize Serial comunication
  Serial.begin(115200);
  // wait for the serial port to connect, Needed for USB native only
  while (!Serial) ;
  // initialize the Accessory Shield library
  accessoryShield.begin();

  accessoryShield.clearOledDisplay();
  accessoryShield.oledPaint();

  Genuino101Peripheral.setLocalName("AccessoryShield");
  // Genuino101Peripheral.setDeviceName("Genuino101");
  Genuino101Peripheral.setAdvertisedServiceUuid(AccessoryShieldControl.uuid());
  // Genuino101Peripheral.setAppearance(BLE_GAP_APPEARANCE_TYPE_GENERIC_COMPUTER);
  Genuino101Peripheral.setEventHandler(BLEConnected, BleConnectedCallback);
  Genuino101Peripheral.setEventHandler(BLEDisconnected, BleDisconnectedCallback);

  // Add the AccessoryShieldControl Serivce to Attribute Table
  Genuino101Peripheral.addAttribute(AccessoryShieldControl);

  // Add the RGBchar Characteristic to Attribute Table
  Genuino101Peripheral.addAttribute(RGBled);
  RGBled.setValue(RGBledState, LED_NUMBER);
  RGBled.setEventHandler(BLEWritten, RGBWrittenCallback);

  dht11.tempUnit = TEMP_UNIT;
  dht11.temp = accessoryShield.getTemperature(dht11.tempUnit);
  dht11.humidity = accessoryShield.getHumidity();
  dht11.heatIndex = accessoryShield.computeHeatIndex(dht11.tempUnit);
  Serial.print("\nTemperature = ");
  Serial.print(dht11.temp);
  Serial.println(getTempUnitSymbol(dht11.tempUnit));
  Serial.print("Humidity = ");
  Serial.println(dht11.humidity);
  Serial.print("Heat index = ");
  Serial.println(dht11.heatIndex);

  DHT11data[0] = dht11.temp;
  DHT11data[1] = dht11.tempUnit;
  DHT11data[2] = dht11.humidity;
  DHT11data[3] = dht11.heatIndex;

  Genuino101Peripheral.addAttribute(DHT11);
  DHT11.setValue(DHT11data, DHT11_VALUES);

  Serial.print("\nBuzzer initial frequency = ");
  Serial.println(buzzer.freq);
  Serial.print("Buzzer initial delay = ");
  Serial.println(buzzer.delayTime);
  Serial.print("Buzzer initial state = ");
  if (buzzer.actived) {
    Serial.println("Activated (HIGH)");
  }
  else {
    Serial.println("Disabled (LOW)");
  }
  buzzerValues[0] = buzzer.freq;
  buzzerValues[1] = buzzer.delayTime;
  buzzerValues[2] = buzzer.actived;

  Genuino101Peripheral.addAttribute(Buzzer);
  Buzzer.setValue(buzzerValues, BUZZER_VALUES);
  Buzzer.setEventHandler(BLEWritten, BuzzerWrittenCallback);

  potValue = accessoryShield.readPot();
  Serial.print("\nPotentiometer value = ");
  Serial.println(potValue);
  Genuino101Peripheral.addAttribute(Pot);
  Pot.setValue(potValue);

  joystickOldState = accessoryShield.getJoystickValue();
  Serial.print("\nJoystick initial state : ");
  Serial.println(getJoystickStringState(joystickOldState));
  Genuino101Peripheral.addAttribute(Joystick);
  Joystick.setValue(joystickOldState);

  relayState = accessoryShield.getRelayState();
  Serial.print("\nRelay initial state = ");
  if (relayState) {
    Serial.println("Activated (HIGH)");
  }
  else {
    Serial.println("Disabled (LOW)");
  }
  Genuino101Peripheral.addAttribute(Relay);
  Relay.setValue(relayState);
  Relay.setEventHandler(BLEWritten, RelayWrittenCallback);

  Genuino101Peripheral.begin();
  Serial.println("\nActivating Genuino 101 BLE ...");
  Serial.println("Listening for central devices and waiting for connections ...");
}

void loop() {
  Genuino101Peripheral.poll();

  updateAccessoryShieldData();
}

String getTempUnitSymbol(TemperatureUnit tempUnit) {
  String symbol = "°C";
  if (tempUnit == FARENEITH) {
    symbol.replace("C", "F");
  }
  else if (tempUnit == KELVIN) {
    symbol.replace("°C", "K");
  }
  return symbol;
}

void updateAccessoryShieldData(void) {
  int DHT11state = accessoryShield.getEnvironmentalData(dht11.temp, dht11.humidity, dht11.tempUnit);
  dht11.heatIndex = accessoryShield.computeHeatIndex(dht11.tempUnit);
#ifdef SKETCH_DEBUG
  Serial.print("Temperature = ");
  Serial.print(dht11.temp);
  Serial.println(getTempUnitSymbol(dht11.tempUnit));
  Serial.print("Humidity = ");
  Serial.println(dht11.humidity);
  Serial.print("Heat Index = ");
  Serial.println(dht11.heatIndex);
#endif
  if ((DHT11state == DHT11_DATA_READ) && (dht11.heatIndex != NAN)) {
    DHT11data[0] = dht11.temp;
    DHT11data[2] = dht11.humidity;
    DHT11data[3] = dht11.heatIndex;
    DHT11.setValue(DHT11data, DHT11_VALUES);
  }

  potValue = accessoryShield.readPot();
#ifdef SKETCH_DEBUG
  Serial.print("\nPotentiometer value = ");
  Serial.println(potValue);
#endif
  Pot.setValue(potValue);

  joystickNewState = accessoryShield.getJoystickValue();
#ifdef SKETCH_DEBUG
  Serial.print("\nJoystick state : ");
  Serial.println(getJoystickStringState(joystickNewState));
#endif
  if (joystickNewState != joystickOldState) {
    Joystick.setValue(joystickNewState);
    joystickOldState = joystickNewState;
  }

  relayState = accessoryShield.getRelayState();
#ifdef SKETCH_DEBUG
  Serial.print("\nRelay initial state = ");
  if (relayState) {
    Serial.println("Activated (HIGH)");
  }
  else {
    Serial.println("Disabled (LOW)");
  }
#endif
  Relay.setValue(relayState);
}

String getJoystickStringState(JoystickMode jMode) {
  return joystickModes[jMode];
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

void RGBWrittenCallback(BLECentral &centralDevice, BLECharacteristic &RGBchar) {
  const unsigned char *valuesUpdated = RGBchar.value();
  unsigned int dataLength = RGBchar.valueLength();
  Serial.print("\nRGBled Characteristic has ");
  Serial.print(dataLength);
  Serial.println(" values");
  if (dataLength == LED_NUMBER) {
    Serial.println("RGB led states changed by central device.");
    for (byte led = 0; led < LED_NUMBER; led++) {
      RGBledState[led] = valuesUpdated[led];
      Serial.print(ledColors[led]);
      Serial.print(" led state : ");
      Serial.println(RGBledState[led]);
    }
    Serial.println("Setting the RGB led states ...");
    accessoryShield.setRGB(RGBledState[0], RGBledState[1], RGBledState[2]);

  }
  else {
    Serial.println("Invalid number of data received");
  }
}

void BuzzerWrittenCallback(BLECentral &centralDevice, BLECharacteristic &buzzerChar) {
  const unsigned char *valuesUpdated = buzzerChar.value();
  unsigned int dataLength = buzzerChar.valueLength();
  Serial.print("\nBuzzer Characteristic has ");
  Serial.print(dataLength);
  Serial.println(" values");
  if (dataLength == BUZZER_VALUES) {
    Serial.println("Buzzer values updated by central device :");
    buzzer.freq = valuesUpdated[0];
    Serial.print("Buzzer frequency = ");
    Serial.println(buzzer.freq);
    buzzer.delayTime = valuesUpdated[1];
    Serial.print("Buzzer delay = ");
    Serial.println(buzzer.delayTime);
    Serial.print("Buzzer state = ");
    if (valuesUpdated[2]) {
      Serial.println("Activated (HIGH)");
      buzzer.actived = true;
      Serial.println("Playing the buzzer");
      accessoryShield.playBuzzer(buzzer.freq, buzzer.delayTime);
    }
    else {
      Serial.println("Disabled (LOW)");
      buzzer.actived = false;
    }
  }
  else {
    Serial.println("Invalid number of data received");
  }
}

void RelayWrittenCallback(BLECentral &centralDevice, BLECharacteristic &relayChar) {
  const unsigned char *updatedValue = relayChar.value();
  Serial.println("\nRelay state updated by central device");
  if (*updatedValue) {
    Serial.println("Activating the relay");
    accessoryShield.activateRelay();
  }
  else {
    Serial.println("Disabling the relay");
    accessoryShield.disableRelay();
  }
}

