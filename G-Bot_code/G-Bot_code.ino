#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"
#include <Servo.h>
#include "SD.h"
#include "TMRpcm.h"
#include "SPI.h"
#include <RTClib.h>

RTC_DS3231 rtc;
TMRpcm music;

// Define the target alarm times (hour, minute, second)
byte targetHour1 = 16;
byte targetMinute1 = 20;
byte targetSecond1 = 0;

byte targetHour2 = 13;
byte targetMinute2 = 4;
byte targetSecond2 = 30;

byte targetHour3 = 18;
byte targetMinute3 = 15;
byte targetSecond3 = 0;

//SIM800 TX is connected to Arduino D8
#define SIM800_TX_PIN 18
//SIM800 RX is connected to Arduino D7
#define SIM800_RX_PIN 19
//Create software serial object to communicate with SIM800
SoftwareSerial serialSIM800(SIM800_TX_PIN,SIM800_RX_PIN);

Servo dispenser;

#define pump 3
#define irLeft 4
#define irRight 5
#define ma1 6
#define ma2 7
#define mb1 8
#define mb2 30
#define en1 12
#define en2 13
#define trigPin 26
#define echoPin 24
#define pump 3
#define servoSwitchPin 29
#define SD_ChipSelectPin 53

VR myVR(10,11);    // 10:RX 11:TX


uint8_t records[7]; // save record
uint8_t buf[64];

bool start = 1;
bool dispenseWater = false;
bool sosmessage = false;

#define water  (0)
#define help   (1)


/**
  @brief   Print signature, if the character is invisible,
           print hexible value instead.
  @param   buf     --> command length
           len     --> number of parameters
*/
void printSignature(uint8_t *buf, int len)
{
  int i;
  for(i=0; i<len; i++){
    if(buf[i]>0x19 && buf[i]<0x7F){
      Serial.write(buf[i]);
    }
    else{
      Serial.print("[");
      Serial.print(buf[i], HEX);
      Serial.print("]");
    }
  }
}

/**
  @brief   Print signature, if the character is invisible,
           print hexible value instead.
  @param   buf  -->  VR module return value when voice is recognized.
             buf[0]  -->  Group mode(FF: None Group, 0x8n: User, 0x0n:System
             buf[1]  -->  number of record which is recognized.
             buf[2]  -->  Recognizer index(position) value of the recognized record.
             buf[3]  -->  Signature length
             buf[4]~buf[n] --> Signature
*/
void printVR(uint8_t *buf)
{
  Serial.println("VR Index\tGroup\tRecordNum\tSignature");

  Serial.print(buf[2], DEC);
  Serial.print("\t\t");

  if(buf[0] == 0xFF){
    Serial.print("NONE");
  }
  else if(buf[0]&0x80){
    Serial.print("UG ");
    Serial.print(buf[0]&(~0x80), DEC);
  }
  else{
    Serial.print("SG ");
    Serial.print(buf[0], DEC);
  }
  Serial.print("\t");

  Serial.print(buf[1], DEC);
  Serial.print("\t\t");
  if(buf[3]>0){
    printSignature(buf+4, buf[3]);
  }
  else{
    Serial.print("NONE");
  }
  Serial.println("\r\n");
}

///////////////////////////////////////////////////////////////// void setup ////////////////////////////////////////////////////////
void setup(){
 
  dispenser.attach(servoPin);  // attaches the servo on pin 9 to the servo object

  pinMode(irLeft,INPUT);
  pinMode(irRight,INPUT);
  pinMode(ma1,OUTPUT);
  pinMode(ma2,OUTPUT);
  pinMode(mb1,OUTPUT);
  pinMode(mb2,OUTPUT);
  pinMode(en1,OUTPUT);
  pinMode(en2,OUTPUT);
  pinMode(pump,OUTPUT);
  pinMode(servoSwitchPin,INPUT);
  // turn off pump since its active low
  digitalWrite(pump,HIGH);
  digitalWrite(en1,255);
  digitalWrite(en2,255);

  music.speakerPin = 9; //Auido out on pin 9

  /** initialize voice recogniser*/
  myVR.begin(9600);
 
  Serial.begin(115200);
  while(!Serial);
 
  // Being serial communication with Arduino and SIM800
  serialSIM800.begin(115200);
  delay(1000);

  serialSIM800.print("AT+CMGF=1\r");  //text mode
  delay(100);

  serialSIM800.print("AT+CNMI=2,2,0,0,0\r");  //recieve mode
  delay(100);

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  if(myVR.clear() == 0){
    Serial.println("Recognizer cleared.");
  }else{
    Serial.println("Not find VoiceRecognitionModule.");
    Serial.println("Please check connection and restart Arduino.");
    while(1);
  }
 
  if(myVR.load((uint8_t)help) >= 0){
    Serial.println("help loaded");
  }
 
  if(myVR.load((uint8_t)water) >= 0){
    Serial.println("water loaded");
  }

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1) delay(10);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  rtc.disable32K();
  rtc.writeSqwPinMode(DS3231_OFF);

  
  DateTime time = rtc.now();
  //Full Timestamp
  Serial.println(String("DateTime::TIMESTAMP_TIME:\t")+time.timestamp(DateTime::TIMESTAMP_TIME));

  // Set the alarm times
  DateTime alarmTime1 = DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), targetHour1, targetMinute1, targetSecond1);
  // DateTime alarmTime2 = DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), targetHour2, targetMinute2, targetSecond2);
  // DateTime alarmTime3 = DateTime(rtc.now().year(), rtc.now().month(), rtc.now().day(), targetHour3, targetMinute3, targetSecond3);
  
  // Set the alarm
  if (!rtc.setAlarm1(alarmTime1, DS3231_A1_Minute)) {
    Serial.println("Error, alarm wasn't set!");
  }
  else {
    Serial.print("Alarm set for ");
    Serial.print(targetHour1);
    Serial.print(":");
    Serial.print(targetMinute1);
    Serial.print(":");
    Serial.println(targetSecond1);
    Serial.println("");
    Serial.print("Alarm set for ");
    Serial.print(targetHour2);
    Serial.print(":");
    Serial.print(targetMinute2);
    Serial.print(":");
    Serial.println(targetSecond2);
    Serial.println("");
    Serial.print("Alarm set for ");
    Serial.print(targetHour3);
    Serial.print(":");
    Serial.print(targetMinute3);
    Serial.print(":");
    Serial.println(targetSecond3);
    Serial.println("");
  }
  music.setVolume(5);    //   0 to 7. Set volume level
  music.quality(1);        //  Set 1 for 2x oversampling Set 0 for normal
  music.play("medicine.wav",0);  
}

///////////////////////////////////////////////////////////////// void loop ////////////////////////////////////////////////////////
int notAlarm = 1;
void loop(){
  while(notAlarm){
    // DateTime time = rtc.now();
    //Full Timestamp
    // Serial.println(String("DateTime::TIMESTAMP_TIME:\t")+time.timestamp(DateTime::TIMESTAMP_TIME));
    // Check alarm status
    if (rtc.alarmFired(1)) {
      rtc.clearAlarm(1);
      Serial.println(" - Alarm 1 triggered");
      music.play("medicine.wav",0);
      notAlarm = 0;
    }
    // else if (rtc.alarmFired(2)) {
    //   rtc.clearAlarm(2);
    //   Serial.println(" - Alarm 2 triggered");
    //   notAlarm = 0;
    // }
    // else if (rtc.alarmFired(3)) {
      // rtc.clearAlarm(3);
      // Serial.println(" - Alarm 3 triggered");
      // notAlarm = 0;
    // }
    //rotate servo
    // delay(1000);
    // Serial.println(digitalRead(servoPin));
    // if(digitalRead(servoPin)){
    //   Serial.println("dispensing");
    //   dispenser.write(0);
    //   delay(1000);
    //   dispenser.write(90);
    //   delay(500);
    // }
    // listen for voices
    int ret;
    ret = myVR.recognize(buf, 50);
    if(ret>0){
      switch(buf[1]){
        case water:
          Serial.println("dispensing water");
          music.play("water.wav",0);
          notAlarm = 0;
          dispenseWater = true;
          break;  
        case help:
          Serial.println("send sos message");
          music.play("emergency.wav",0);
          SendMessage();
          sosmessage = true;
          break;
        default:
          Serial.println("Record function undefined");
          break;
      }
      /** voice recognized */
      printVR(buf);
    }
  }  
  start = 1;
  // move towards patient
  while(start){
    if(getDistance()<20) stop();
    else if(digitalRead(irLeft)==1){
      turnLeft();
    }
    else if(digitalRead(irRight)==1){
      turnRight();
    }else{
      moveFwd();
    }
    if(digitalRead(irLeft)==1&&digitalRead(irRight)==1){
      start = 0;
    }
  }
  stop();
  // check if asked only for water
  if(!dispenseWater){
    dispenser.write(0);
    delay(1000);
    dispenser.write(90);
    delay(500);
    dispenseWater = false;
  }
  digitalWrite(pump,LOW); //turn on pump
  delay(5000);
  digitalWrite(pump,HIGH); // turn off pump
  delay(2000);
  turnAround();
  start = 1;

  // go back to station
  while(start){
    if(getDistance()<20) stop();
    else if(digitalRead(irLeft)==1){
      turnLeft();
    }
    else if(digitalRead(irRight)==1){
      turnRight();
    }else{
      moveFwd();
    }
    if(digitalRead(irLeft)==1&&digitalRead(irRight)==1){
     start=0;
    }
  }
  turnAround();
  stop();
}

////////////////////////////////////////////////// ultrasonic /////////////////////////////////////////////////
int getDistance(){
   // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  long duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  int distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print("Distance: ");
  Serial.println(distance);
  return distance;
}

/////////////////////////////////////////////////////////// motor controls /////////////////////////////////////////
void moveFwd(){
  Serial.println("fwd");
  digitalWrite(ma1,HIGH);
  digitalWrite(ma2,LOW);
  digitalWrite(mb1,HIGH);
  digitalWrite(mb2,LOW);
}

void turnLeft(){
  Serial.println("lft");
 
  digitalWrite(ma1,HIGH);
  digitalWrite(ma2,LOW);
  digitalWrite(mb1,LOW);
  digitalWrite(mb2,HIGH);
}

void turnRight(){
  Serial.println("rt");
  digitalWrite(ma1,LOW);
  digitalWrite(ma2,HIGH);
  digitalWrite(mb1,HIGH);
  digitalWrite(mb2,LOW);
}

void stop(){
  Serial.println("stp");
  digitalWrite(ma1,LOW);
  digitalWrite(ma2,LOW);
  digitalWrite(mb1,LOW);
  digitalWrite(mb2,LOW);
}
void turnAround(){
  Serial.println("turn");
  digitalWrite(ma1,HIGH);
  digitalWrite(ma2,LOW);
  digitalWrite(mb1,LOW);
  digitalWrite(mb2,HIGH);
  delay(2500);
}

///////////////////////////////////////////////// SendMessage /////////////////////////////////////////////////
void SendMessage()
{
   Serial.println("Setting the GSM in text mode");
   serialSIM800.println("AT+CMGF=1\r");
   delay(2000);
   Serial.println("Sending SMS to the desired phone number!");
   serialSIM800.println("AT+CMGS=\"+918281254129\"\r");
   // Replace x with mobile number
   delay(2000);
   serialSIM800.println("Patient is in need of medical assistance");    // SMS Text
   delay(500);
   serialSIM800.println((char)26);               // ASCII code of CTRL+Z
   delay(2000);
}