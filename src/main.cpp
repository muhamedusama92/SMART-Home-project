// Template ID, Device Name and Auth Token are provided by the Blynk.Cloud
#define BLYNK_TEMPLATE_ID "TMPLI74KDz67"
#define BLYNK_DEVICE_NAME "Smart Home"
#define BLYNK_AUTH_TOKEN "HpIkC8w8Q3Tp23zCoCzxEheQHI5SnMpS"
// Comment this out to disable prints and save space
#define BLYNK_PRINT Serial

#include <Arduino.h>
#include <ESP32Servo.h>
#include <LiquidCrystal_I2C.h>
#include <I2CKeyPad.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char auth[] = BLYNK_AUTH_TOKEN;
// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "SMART Home";
char pass[] = "12345678";

BlynkTimer timer;

#define ReceptionLED 18
#define BedroomLED 4
#define BathroomLED 19
#define KitchenLED 5

//********************** Rain System Definitions *************************
#define rainSensor 13
#define windowMotor 12
#define windowSwitch 14
Servo window; // create servo object to control Windows
bool rain;
char windowState = 0;
bool windowSwitchByApp = false;
bool RainState(bool R);
void OpenWindow();
void CloseWindow();

//******************* Garage System Definitions **************************
#define trigPin 32 // Ultrasonic trig
#define echoPin 34 // Ultrasonic echo
#define garageMotor 27
#define garageButton 15
Servo GarageDoor; // create servo object to control the Garage door
long duration;
float distanceCm;
float soundSpeed = 0.034;
unsigned long ultasonicTimer = 0;
void CloseGarage();
void OpenGarage();
char garageState = 0;
bool garageDoorByApp;

//************************ Weather System Definitions **********************
LiquidCrystal_I2C Lcd(0x27, 16, 2);
#define tempPin 23
#define Fan 16
DHT dht(tempPin, DHT11);
bool fanState = LOW;
int Temp;
unsigned long LM35_Timer = 0;

//************************* Firefighter System Definitions *******************
#define flameSensor 35
#define waterPump 17
#define fireBuzzer 26
#define fireButton 33
char FireState(char F);
char fire;
int fireValue;

//************************* Password System Definitions *******************
#define DoorButton 39
#define doorMotor 25

char keymap[19] = "123A456B789C*0#DNf"; // creat keymap of the keypad
I2CKeyPad keyPad(0x20);
LiquidCrystal_I2C passwordLCD(0x26, 16, 2);
Servo MainDoor; // create servo object to control the Main door

char password[5] = "5A2C";
char entered[10];
unsigned char enteredCount = 0; // counter of entered numbers form keypad

bool mainDoorByApp;
bool correct; // tells if password correct or not
char Try = 0; // counter of wrong times
bool passwordFlag = 0;
char locked = 1;    // tells if door is locked or not
bool block = false; // tells block state
unsigned long blockTimer = 0;
unsigned long waitingTimer = 0;
unsigned long DoorTimer = 0;
char waiting = 0;
bool CheckThePassword();
void GetPassword();
void OpenTheDoor();
void CloseTheDoor();
void StartScreen();

// This functions are called every time the Virtual Pins state changes
BLYNK_WRITE(V0) { digitalWrite(ReceptionLED, param.asInt()); }
BLYNK_WRITE(V1) { digitalWrite(BedroomLED, param.asInt()); }
BLYNK_WRITE(V2) { digitalWrite(BathroomLED, param.asInt()); }
BLYNK_WRITE(V3) { digitalWrite(KitchenLED, param.asInt()); }
BLYNK_WRITE(V4)
{
  if (param.asInt())
    windowSwitchByApp = HIGH;
  else
    windowSwitchByApp = LOW;
}
BLYNK_WRITE(V5)
{
  if (param.asInt())
    garageDoorByApp = HIGH;
  else
    garageDoorByApp = LOW;
}
BLYNK_WRITE(V6)
{
  if (param.asInt())
    mainDoorByApp = HIGH;
  else
    mainDoorByApp = LOW;
}
BLYNK_WRITE(V10)
{
  digitalWrite(Fan, param.asInt());
  fanState = param.asInt();
}

// This function is called every time the device is connected to the Blynk.Cloud
BLYNK_CONNECTED()
{
  Blynk.syncVirtual(V0);
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V3);
  Blynk.syncVirtual(V4);
  Blynk.syncVirtual(V5);
  Blynk.syncVirtual(V6);
  Blynk.syncVirtual(V10);
}
// This function sends ESP32's uptime every second to Virtual Pins .
void myTimerEvent()
{
  Blynk.virtualWrite(V7, fireValue);
  Blynk.virtualWrite(V8, Temp);
  Blynk.virtualWrite(V9, RainState(rain));
  /* if (windowState == LOW)
     Blynk.virtualWrite(V4, LOW);
   else
     Blynk.virtualWrite(V4, HIGH);*/
  if (fanState == LOW)
    Blynk.virtualWrite(V10, LOW);
  else
    Blynk.virtualWrite(V10, HIGH);
}

void setup()
{
  Serial.begin(9600);
  Wire.begin();
  Blynk.begin(auth, ssid, pass);
  timer.setInterval(500L, myTimerEvent);

  pinMode(ReceptionLED, OUTPUT);
  pinMode(BedroomLED, OUTPUT);
  pinMode(BathroomLED, OUTPUT);
  pinMode(KitchenLED, OUTPUT);

  //**************** Rain System Setup **************************
  pinMode(rainSensor, INPUT);
  pinMode(windowSwitch, INPUT);
  window.setPeriodHertz(50);
  window.attach(windowMotor, 600, 2700); // attaches the servo motor to Window pin and detect the min & max rang of rotation
  window.write(0);

  //*************** Garage System Setup ***********************
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(garageButton, INPUT);
  GarageDoor.setPeriodHertz(50);
  GarageDoor.attach(garageMotor, 600, 2700); // attaches the servo motor to garage pin and detect the min & max rang of rotation
  GarageDoor.write(180);

  //**************** Weather System Setup *******************
  pinMode(Fan, OUTPUT);
  dht.begin();
  Lcd.init();
  Lcd.backlight();
  Lcd.begin(16, 2);
  Lcd.setCursor(3, 0);
  Lcd.print("Welcome to");
  Lcd.setCursor(1, 1);
  Lcd.print("Weather System");
  delay(2000);
  Lcd.clear();
  ultasonicTimer = millis();

  //**************** Firefighter System Setup **************************
  pinMode(flameSensor, INPUT);
  pinMode(fireButton, INPUT);
  pinMode(waterPump, OUTPUT);
  pinMode(fireBuzzer, OUTPUT);
  digitalWrite(waterPump, HIGH);
  digitalWrite(fireBuzzer, LOW);

  //**************** Password System Setup **************************
  pinMode(DoorButton, INPUT);
  passwordLCD.init();
  passwordLCD.backlight();
  passwordLCD.begin(16, 2);
  passwordLCD.setCursor(2, 0);
  passwordLCD.print("SMART Locker");
  delay(2000);
  passwordLCD.clear();
  passwordLCD.setCursor(6, 0);
  passwordLCD.print("Enter");
  passwordLCD.setCursor(2, 1);
  passwordLCD.print("The Password");
  delay(2000);
  StartScreen();
  keyPad.loadKeyMap(keymap);
  MainDoor.setPeriodHertz(50);
  MainDoor.attach(doorMotor, 600, 2700); // attaches the servo motor to Door pin and detect the min & max rang of rotation
  MainDoor.write(180);
  waitingTimer = millis();
}

void loop()
{
  Blynk.run();
  timer.run();
  //********************** Rain System Loop *******************************
  if (FireState(fire) == 1)
    OpenWindow();
  else if (RainState(rain))
  {
    Lcd.setCursor(8, 1);
    Lcd.print("Raining  ");
    Serial.print("Rain");
    Serial.println(" Window is closed");
    CloseWindow();
  }
  else
  {
    Lcd.setCursor(8, 1);
    Lcd.print("Sunny   ");
    Serial.print("NO Rain");
    if ((!digitalRead(windowSwitch)) || (windowSwitchByApp == LOW))
    {
      switch (windowState)
      {
      case 1:
        CloseWindow();
        break;

      default:
        OpenWindow();
        break;
      }
      while (!digitalRead(windowSwitch))
        ;
    }
  }

  //********************** Garage System Loop *********************************
  digitalWrite(trigPin, HIGH); // Sets the trigPin on HIGH state for 10 micro seconds
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH); // Reads the echoPin, returns the sound wave travel time in microseconds
  delay(50);
  distanceCm = (duration * soundSpeed) / 2; // Calculate the distance
  if (FireState(fire) == 1)
    OpenGarage();
  else if ((digitalRead(garageButton)) || (garageDoorByApp == HIGH))
  {
    switch (garageState)
    {
    case 1:
      CloseGarage();
      break;
    default:
      OpenGarage();
      break;
    }
  }
  else if (distanceCm < 14)
  {
    OpenGarage();
  }
  else if (millis() - ultasonicTimer > 15000UL)
  {
    CloseGarage();
    ultasonicTimer = millis();
  }
  //********************* Weather System Loop ********************
  if (LM35_Timer == 0 || millis() - LM35_Timer >= 5000UL)
  {
    Temp = dht.readTemperature();
    Lcd.setCursor(5, 0);
    Lcd.print(Temp);
    Lcd.print("  C  ");
    if (Temp >= 30)
    {
      Lcd.setCursor(2, 1);
      Lcd.print("Hot    ");
      Serial.print(Temp);
      Serial.println("Hot");
      digitalWrite(Fan, LOW);
      fanState = LOW;
    }
    else
    {
      if (Temp >= 20)
      {
        Lcd.setCursor(2, 1);
        Lcd.print("Fine    ");
        Serial.print(Temp);
        Serial.println("Fine");
      }
      else
      {
        Lcd.setCursor(2, 1);
        Lcd.print("Cold    ");
        Serial.print(Temp);
        Serial.println("Cold");
      }
    }
    LM35_Timer = millis();
  }
  //********************** Firefighter System Loop *******************************
  fireValue = analogRead(flameSensor);
  if (FireState(fire) == 1)
  {
    do
    {
      digitalWrite(waterPump, LOW);
      digitalWrite(fireBuzzer, HIGH);
      Serial.println("Fire !");
    } while (FireState(fire) != 3);
  }
  else
  {
    digitalWrite(waterPump, HIGH);
    Serial.println("Safe");
    if (digitalRead(fireButton) == HIGH)
    {
      digitalWrite(fireBuzzer, LOW);
    }
  }

  //********************** Password System Loop *******************************
  if ((digitalRead(DoorButton)) || (mainDoorByApp == HIGH))
  {
    switch (locked)
    {
    case 1:
    {
      OpenTheDoor();
      enteredCount = 0;
      Try = 0;
      block = false;
      waiting = 0;
      passwordFlag = 0;
      DoorTimer = millis();
    }
    break;
    default:
    {
      CloseTheDoor();
      delay(2000);
      StartScreen();
    }
    break;
    }
    while (digitalRead(DoorButton))
      ;
  }
  else if (locked)
  {
    if (block)
    {
      if (millis() - waitingTimer >= 1000UL)
      {
        passwordLCD.setCursor(7, 1);
        passwordLCD.print(" s");
        passwordLCD.setCursor(6, 1);
        passwordLCD.print(30 - waiting);
        waiting++;
        waitingTimer = millis();
      }
      if (millis() - blockTimer >= 30000UL)
      {
        block = false;
        waiting = 0;
        passwordFlag = 0;
        StartScreen();
      }
    }
    else
    {
      GetPassword();
      if (CheckThePassword())
      {
        passwordLCD.clear();
        passwordLCD.setCursor(4, 0);
        passwordLCD.print("Correct");
        delay(2000);
        OpenTheDoor();
        DoorTimer = millis();
      }
      else
      {
        if (Try == 4)
        {
          passwordLCD.clear();
          passwordLCD.setCursor(0, 0);
          passwordLCD.print("Try Again After");
          block = true;
          blockTimer = millis();
          Try = 0;
        }
        else if (passwordFlag == 1)
        {
          passwordLCD.clear();
          passwordLCD.setCursor(5, 0);
          passwordLCD.print("Wrong");
          passwordLCD.setCursor(3, 1);
          passwordLCD.print("Try Again");
          passwordFlag = 0;
          delay(2000);
          StartScreen();
        }
      }
    }
  }
  else
  {
    if (millis() - DoorTimer >= 15000UL)
    {
      CloseTheDoor();
      delay(2000);
      StartScreen();
    }
  }
}

//******************** Rain System Functions ****************************
bool RainState(bool R) // Function to Return the State of Rain
{
  if (digitalRead(rainSensor) == 0)
    R = HIGH;
  else
    R = LOW;
  return R;
}
void OpenWindow()
{
  if (windowState == 0)
  {
    for (int W = 0; W <= 90; W += 5)
    {                  // goes from 0 degrees to 90 degrees
      window.write(W); // tell servo to go to position in variable 'W'
      delay(20);
    }
  }
  windowState = 1;
}
void CloseWindow()
{
  if (windowState == 1)
  {
    for (int W = 90; W >= 0; W -= 5)
    {                  // goes from 90 degrees to 0 degrees
      window.write(W); // tell servo to go to position in variable 'W'
      delay(20);
    }
  }
  windowState = 0;
}

//***************** Garage System Functions **************************
void CloseGarage()
{
  if (garageState == 1)
  {
    for (int G = 90; G <= 180; G += 5)
    {                      // goes from 90 degrees to 180 degrees
      GarageDoor.write(G); // tell servo to go to position in variable 'G'
      delay(20);
    }
  }
  garageState = 0;
}
void OpenGarage()
{
  if (garageState == 0)
  {
    for (int G = 180; G >= 90; G -= 5)
    {                      // goes from 180 degrees to 90 degrees
      GarageDoor.write(G); // tell servo to go to position in variable 'G'
      delay(20);
    }
  }
  garageState = 1;
}

//***************** Firefighter System Functions **************************
char FireState(char F)
{
  if (analogRead(flameSensor) < 1000)
    F = 1; // variable 'F' tells the level of Fire
  else if (analogRead(flameSensor) < 4000)
    F = 2;
  else
    F = 3;
  return F;
}

//***************** Password System Functions **************************
void StartScreen()
{
  passwordLCD.clear();
  passwordLCD.setCursor(0, 1);
  passwordLCD.print("Enter:#  Delet:*");
}

/*********Function to get numbers form the keybad***********/
void GetPassword()
{
  if (keyPad.isPressed())
  {
    char key = keyPad.getChar();
    if ((key != '#') & (key != '*'))
    {
      entered[enteredCount] = key;
      passwordLCD.setCursor(enteredCount, 0);
      passwordLCD.print(entered[enteredCount]);
      enteredCount++;
      while (keyPad.isPressed())
        ;
    }
    if (key == '*')
    {
      if (enteredCount != 0)
        enteredCount--;
      entered[enteredCount] = 0;
      passwordLCD.setCursor(enteredCount, 0);
      passwordLCD.print(" ");
      while (keyPad.isPressed())
        ;
    }
  }
}

/*********Function to check if the password correct or not***********/
bool CheckThePassword()
{
  bool check = false;
  if (keyPad.isPressed())
  {
    char key = keyPad.getChar();
    if (key == '#')
    {
      if (!strcmp(entered, password))
      {
        check = true;
        Try = 0;
      }
      else
      {
        check = false;
        passwordFlag = 1;
        Try++;
      }
      while (enteredCount > 0)
      {
        entered[enteredCount] = 0;
        enteredCount--;
      }
      while (keyPad.isPressed())
        ;
    }
  }
  return check;
}

void OpenTheDoor()
{
  for (int D = 180; D >= 110; D -= 5)
  {                    // goes from 180 degrees to 110 degrees
    MainDoor.write(D); // tell servo to go to position in variable 'D'
    delay(20);
  }
  passwordLCD.clear();
  passwordLCD.setCursor(2, 1);
  passwordLCD.print("Door Opened");
  locked = 0;
}

void CloseTheDoor()
{
  for (int D = 110; D <= 180; D += 5)
  {                    // goes from 110 degrees to 180 degrees
    MainDoor.write(D); // tell servo to go to position in variable 'D'
    delay(20);
  }
  passwordLCD.clear();
  passwordLCD.setCursor(2, 1);
  passwordLCD.print("Door Closed");
  locked = 1;
}