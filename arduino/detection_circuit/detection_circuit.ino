#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <arduino.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN D0
#define SDA_PIN D8
#define PIR_PIN D2
#define BUZZER_PIN D1
#define GREENLED_PIN D4
#define REDLED_PIN D3

#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"

#define API_KEY "API_KEY"
#define DATABASE_URL "DATABASE_URL"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

MFRC522 mfrc522(SDA_PIN, RST_PIN); 

int count = 0;
int calibrationTime = 5;
int authorizationDuration = 5;
bool val = false;         
bool alarm = false;
bool prevVal = false;
bool leaving = false;
bool signupOK = false;
bool authorized = false;
bool enable_alarm = true;
String rfid_data = "";

void leaving_logic();
void non_authorized_logic();
void authorized_logic();
bool get_firebase_bool(String label, bool value);
void set_firebase_bool(String label, bool value);
void countdown(int duration);
void intruder_alarm();
void access_alarm();
bool detect_movement(bool val, bool prevVal);
String read_RFID_data();
void calibrate_PIR();
void setup_firebase();
void setup_wifi();

void setup() {
  pinMode(GREENLED_PIN, OUTPUT); 
  pinMode(REDLED_PIN, OUTPUT); 
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);  

  Serial.begin(9600);  
  SPI.begin();         
  mfrc522.PCD_Init();  
  setup_wifi();
  setup_firebase();
  calibrate_PIR();

  Serial.println("Approximate your card to the reader...");
  Serial.println();
}
void loop() {
  if (Firebase.ready() && signupOK){

    val = digitalRead(PIR_PIN);
    prevVal = detect_movement(val, prevVal);
    enable_alarm = get_firebase_bool("enable_alarm", enable_alarm);

    if(val && !authorized){
      intruder_alarm();
    }

    if (!mfrc522.PICC_IsNewCardPresent()) {
      return;
    }
    if (!mfrc522.PICC_ReadCardSerial()) {
      return;
    }
    rfid_data = read_RFID_data();

    if (rfid_data.substring(1) == "27 5F 1E B2")  //change here the UID of the card/cards that you want to give access
    {
      authorized_logic();
    } 
    else{
      non_authorized_logic();
    }

    if(leaving){
      leaving_logic();
    }
  }
  delay(500);
}

void setup_wifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void setup_firebase(){
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Signed up.");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (Firebase.ready() && signupOK){
    if (Firebase.RTDB.setBool(&fbdo, "authentication", false)) {
      //Serial.println("Ok!");
    }
    else {
      Serial.println("ERROR");
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.setBool(&fbdo, "alarm_status", false)) {
      //Serial.println("Ok!");
    }
    else {
      Serial.println("ERROR");
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.setBool(&fbdo, "enable_alarm", true)) {
      //Serial.println("Ok!");
    }
    else {
      Serial.println("ERROR");
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.setBool(&fbdo, "movement_detection", false)) {
      //Serial.println("Ok!");
    }
    else {
      Serial.println("ERROR");
      Serial.println(fbdo.errorReason());
    }
  }
}

void calibrate_PIR(){
  Serial.print("calibrating PIR sensor ");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("PIR sensor calibrated!");
  delay(50);
}

String read_RFID_data(){
  //Serial.println("UID tag :");
  String content = "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    //Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  //Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  return content;
}

bool detect_movement(bool val, bool prevVal){
  if (val != prevVal) { 
    if (val) { 
      //digitalWrite(GREENLED_PIN, HIGH);
      //Serial.println("Hey I got you!!!");
      set_firebase_bool("movement_detection", val);
    }
    else { 
      //digitalWrite(LED_BUILTIN, LOW);
      //Serial.println("All quiet");
      set_firebase_bool("movement_detection", val);
    }
    prevVal = val;
  }
  return prevVal;
}

void access_alarm(){
  digitalWrite(GREENLED_PIN, HIGH);
  tone(BUZZER_PIN, 208);  // Send 208Hz sound signal...
  delay(500);
  tone(BUZZER_PIN, 392);  // Send 392Hz sound signal...
  delay(500);
  noTone(BUZZER_PIN);  // Stop sound...
  delay(1000);         // ...for 1sec
  digitalWrite(GREENLED_PIN, LOW);
}

void intruder_alarm(){
  alarm = true;
  set_firebase_bool("alarm_status", alarm);
  //Serial.println("Intruder detected !!!");
  if(enable_alarm){
    for (int i = 0; i < 5; i++) {
      digitalWrite(REDLED_PIN, HIGH);
      tone(BUZZER_PIN, 208);  // Send 208Hz sound signal...
      delay(250);
      digitalWrite(REDLED_PIN, LOW);
      tone(BUZZER_PIN, 392);  // Send 392Hz sound signal...
      delay(250);
    }
    noTone(BUZZER_PIN);
  }
  delay(1000);
}

void countdown(int duration){
  for (int i = 1; i < duration+1; i++) {
    Serial.print(" ");
    Serial.print(i);
    Serial.print(",");
    delay(1000);
  }
}

void set_firebase_bool(String label, bool value){
  if (Firebase.RTDB.setBool(&fbdo, label, value)) {
    //Serial.println("OK");
    //Serial.println(fbdo.dataPath());
    //Serial.println(fbdo.dataType());
  }
  else {
    Serial.println("ERROR");
    Serial.println(fbdo.errorReason());
  }
}

bool get_firebase_bool(String label, bool value){
  if (Firebase.RTDB.getBool(&fbdo, label)) {
    if (fbdo.dataType() == "boolean") {
      value = fbdo.boolData();
      //Serial.println(value);
    }
  }else {
    Serial.println(fbdo.errorReason());
  }
  return value;
}

void authorized_logic(){
  count++;
  if (!authorized && !leaving) {
    authorized = true;
    alarm = false;
    Serial.println("Authorized access");
    set_firebase_bool("alarm_status", alarm);
    set_firebase_bool("authentication", authorized);
    access_alarm();
  }
  if (authorized && !leaving && !(count%2)) {
    authorized = true;
    alarm = false;
    leaving = true;
    set_firebase_bool("alarm_status", alarm);
    set_firebase_bool("authentication", authorized);
    Serial.println("Leaving!");
    access_alarm();
  }
}

void non_authorized_logic(){
  authorized = false;
  set_firebase_bool("authentication", authorized);
  Serial.println("Unrecognized Access");
}

void leaving_logic(){
  Serial.print("Countdown started...");
  countdown(authorizationDuration);
  authorized = false;
  leaving = false;
  set_firebase_bool("authentication", authorized);
  Serial.println();
  Serial.println("Authorization expired");
}
