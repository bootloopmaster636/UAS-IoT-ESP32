#include <Arduino.h>
#include <WiFi.h>
#include <aREST.h>
#include <FastLED.h>
#define LIGHTWEIGHT 1 // lightweight arest

// onboard rgb led
#define NUM_LEDS 1
#define DATA_PIN 48
CRGB leds[NUM_LEDS];

// aREST instance
aREST rest = aREST();
WiFiServer server(80);

// Thermistor sensor configs
int ThermistorPin = A0;
int Vo;

// variables
enum BeepMode { Single, Double, Long };
enum OpMode { Temp, Sound, Light, Manual };
enum Color { Red, Yellow, Green, Cyan, Blue, Magenta, White, Black };
bool masterSwitch;
OpMode mode;
int fromThermistor, fromPhotoresistor;

// pins
const int Lred = 4;
const int Lgreen = 5;
const int Lblue = 6;

const int relay = 12;
const int buzzer = 11;
const int button = 10;

const int thermistor = 17;
const int micDigital = 36;
const int photoresistor = 20;

void beep(BeepMode mode) {
  digitalWrite(buzzer, LOW);

  switch (mode) {
  case Single:
    digitalWrite(buzzer, HIGH);
    delay(80);

  case Double:
    digitalWrite(buzzer, HIGH);
    delay(20);
    digitalWrite(buzzer, LOW);
    delay(250);
    digitalWrite(buzzer, HIGH);
    delay(20);

  case Long:
    digitalWrite(buzzer, HIGH);
    delay(500);
  }

  digitalWrite(buzzer, LOW);
}

void changeLed(Color color) {
  digitalWrite(Lred, LOW);
  digitalWrite(Lgreen, LOW);
  digitalWrite(Lblue, LOW);

  switch (color) {
  case Red:
    digitalWrite(Lred, HIGH);
    digitalWrite(Lgreen, LOW);
    digitalWrite(Lblue, LOW);
    leds[0] = CRGB::Red;
    FastLED.show();
    break;

  case Yellow:
    digitalWrite(Lred, HIGH);
    digitalWrite(Lgreen, HIGH);
    digitalWrite(Lblue, LOW);
    leds[0] = CRGB::Yellow;
    FastLED.show();
    break;

  case Green:
    digitalWrite(Lred, LOW);
    digitalWrite(Lgreen, HIGH);
    digitalWrite(Lblue, LOW);
    leds[0] = CRGB::Green;
    FastLED.show();
    break;

  case Cyan:
    digitalWrite(Lred, LOW);
    digitalWrite(Lgreen, HIGH);
    digitalWrite(Lblue, HIGH);
    leds[0] = CRGB::Cyan;
    FastLED.show();
    break;

  case Blue:
    digitalWrite(Lred, LOW);
    digitalWrite(Lgreen, LOW);
    digitalWrite(Lblue, HIGH);
    leds[0] = CRGB::Blue;
    FastLED.show();
    break;

  case Magenta:
    digitalWrite(Lred, HIGH);
    digitalWrite(Lgreen, LOW);
    digitalWrite(Lblue, HIGH);
    leds[0] = CRGB::Magenta;
    FastLED.show();
    break;

  case White:
    digitalWrite(Lred, HIGH);
    digitalWrite(Lgreen, HIGH);
    digitalWrite(Lblue, HIGH);
    leds[0] = CRGB::White;
    FastLED.show();
    break;

  case Black:
    digitalWrite(Lred, LOW);
    digitalWrite(Lgreen, LOW);
    digitalWrite(Lblue, LOW);
    leds[0] = CRGB::Black;
    FastLED.show();
  }
}

void blink(Color color) {
  for (int i = 0; i < 3; i++) {
    changeLed(color);
    delay(500);
    changeLed(Black);
    delay(500);
  }
}

void setMasterSwitch(bool state) {
  if (state == true) {
    masterSwitch = true;
    changeLed(Green);
    digitalWrite(relay, HIGH);
  } else {
    masterSwitch = false;
    changeLed(Red);
    digitalWrite(relay, LOW);
  }
}

float getTempCelcius() {
  float R1 = 10000; // value of R1 on board
  float logR2, R2, T;
  float c1 = 0.001129148, c2 = 0.000234125, c3 = 0.0000000876741; //steinhart-hart coeficients for thermistor

  Vo = analogRead(thermistor);
  R2 = R1 * (4096.0 / (float)Vo - 1.0); //calculate resistance on thermistor
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2)); // temperature in Kelvin
  T = T - 273.15; //convert Kelvin to Celcius
  return T;
}

int apiToggleSwitch(String command) {
  int input = command.toInt();

  switch (input) {
  case 1:
    setMasterSwitch(true);
    return 1;

  case 0:
    setMasterSwitch(false);
    return 0;

  default:
    return -1;
  }
}

int apiChangeMode(String command) {
  if (command == "temp") {
    mode = Temp;
    blink(Blue);
    return 1;
  } else if (command == "sound") {
    mode = Sound;
    blink(Yellow);
    return 1;
  } else if (command == "light") {
    mode = Light;
    blink(Magenta);
    return 1;
  } else if (command == "manual") {
    mode = Manual;
    blink(Green);
    return 1;
  } else {
    return -1;
  }
}

void wifiSmartConf() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.beginSmartConfig();

  Serial.println("Waiting for SmartConfig.");
  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }

  //Wait for SmartConfig packet from mobile
  Serial.println("Waiting for SmartConfig.");
  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("SmartConfig received.");

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected.");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  changeLed(Red);

  mode = Manual;

  wifiSmartConf();

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Init variable and expose
  rest.set_name("lazy-switch");
  rest.set_id("123456");
  rest.function("toggleSwitch", apiToggleSwitch);
  rest.function("changeMode", apiChangeMode);
  rest.variable("masterSwitch", &masterSwitch);
  rest.variable("thermistor", &fromThermistor);
  rest.variable("photoresistor", &fromPhotoresistor);
  rest.variable("mode", &mode);

  // Init pins
  pinMode(micDigital, INPUT);
  pinMode(button, INPUT);
  pinMode(thermistor, INPUT);
  pinMode(photoresistor, INPUT);

  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(Lred, OUTPUT);
  pinMode(Lgreen, OUTPUT);
  pinMode(Lblue, OUTPUT);

  // signal finish
  beep(Single);
  blink(White);
  delay(3000);
  setMasterSwitch(true);
  Serial.println("Ready for action");
}

void loop() {
  switch (mode) {
  case Temp:
  {
    fromThermistor = getTempCelcius();
    Serial.print(">thermistor:");
    Serial.println(fromThermistor);

    if (fromThermistor >= 29) {
      setMasterSwitch(true);
    } else {
      setMasterSwitch(false);
    }
    break;
  }

  case Sound:
  {
    int fromMic = digitalRead(micDigital);
    Serial.print(">mic:");
    Serial.println(fromMic);

    if (fromMic == HIGH) {
      beep(Long);
      setMasterSwitch(!masterSwitch);
    }
    break;
  }

  case Light:
  {
    fromPhotoresistor = analogRead(photoresistor);
    Serial.print(">photoresistor:");
    Serial.println(fromPhotoresistor);

    if (fromPhotoresistor <= 1000) {
      setMasterSwitch(true);
    } else {
      setMasterSwitch(false);
    }
    break;
  }

  case Manual:
  {
    int fromButton = digitalRead(button);
    Serial.print(">button:");
    Serial.println(fromButton);

    if (fromButton == LOW) {
      setMasterSwitch(!masterSwitch);
      beep(Single);
    }
    break;
  }
  }

  delay(100);

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  while (!client.available()) {
    delay(1);
  }
  rest.handle(client);
}
