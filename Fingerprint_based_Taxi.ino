#include <Adafruit_Fingerprint.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define proximity 7
#define alc_sensor_1 6
#define alc_sensor_2 5
#define  ignit_key 4

// On Leonardo/Micro or others with hardware serial, use those! #0 is yellow wire, #1 is white
// uncomment this line:
// #define mySerial Serial1
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (YELLOW wire)
// pin #3 is OUT from Arduino  (WHITE wire)
// comment these two lines if using the hardware serial

SoftwareSerial mySerial(2,3);
SoftwareSerial gsmSerial(12,13);
LiquidCrystal_I2C lcd(0x3F, 16, 2);
 

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

 

uint8_t id;
int stat = 0;
char data;
bool access;
bool vehicle_status;
int wrong_person_counter = 0;

void setup()  
{
  pinMode(proximity,INPUT_PULLUP);
  pinMode(alc_sensor_1,INPUT_PULLUP);
  pinMode(alc_sensor_2,INPUT_PULLUP);
  pinMode(ignit_key,INPUT_PULLUP);
  lcd.begin();
  lcd.backlight();
  lcd.print("WELCOME");
  gsmSerial.begin(9600);
  gsmSerial.println("GSM module detected ");
  Serial.begin(9600);
  send_gsm_cmd("AT");  
  delay(500);
  send_gsm_cmd("ATE0");
  delay(500);
  send_gsm_cmd("AT+CMGF=1");
  delay(500);
   while (!Serial);  // For Yun/Leo/Micro/Zero/...
   delay(100);
   Serial.println("\n\nAdafruit Fingerprint sensor enrollment");
   // set the data rate for the sensor serial port
   finger.begin(57600);
   if (finger.verifyPassword()) 
   {
     Serial.println("Found fingerprint sensor!");
     delay(1000);
   }
   else 
   {
     Serial.println("No Fingerprint sensor :(");
     delay(1000);
     while (1) { delay(1); }
   }
   finger.getTemplateCount();
    if (finger.templateCount == 0) 
    {
      Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
      delay(500);
    } 
}

uint8_t readnumber(void)
{
 uint8_t num = 0;
 while (num == 0) 
 {
   while (! Serial.available());
   num = Serial.parseInt();
 }
 return num;
}


void loop()
{
  access = false;
  lcd.clear();
  lcd.setCursor(0,0);
  while(! Serial.available())
  {
   check_finger();
   if(wrong_person_counter > 2)
   {
      wrong_person_counter = 0;
      send_msg_to("9008842547");
      send_message("Unknown person detected.");
      delay(2000);
      lcd.clear();
   }
   while(access == true)
   {
   
     Serial.println("Driver has reached the car.");
     if(digitalRead(ignit_key) == 0)
     {
        if(vehicle_status == 0)
        {
            vehicle_status = 1;
            Serial.println("Vehicle ON");
            lcd.print("Vehicle On");
            while(digitalRead(ignit_key) == 0); //wait for key release
            delay(2000);
        }
        else
        {
            vehicle_status = 0;
            access = 0;
            Serial.println("Vehicle OFF : ignition OFF");
            lcd.println("Vehicle Off");
            while(digitalRead(ignit_key) == 0); //wait for key release
            delay(2000);
         }
     }
     if(vehicle_status == 1) // if vehicle ON
     {
           if(digitalRead(proximity) == 1) //No driver in seat
           {
               access = 0;
               vehicle_status = 0;
               Serial.println("Vehicle OFF : No driver found");
               delay(2000);
           }
           else if(digitalRead(alc_sensor_1) == 0 || digitalRead(alc_sensor_2) == 0)
           {
               access = 0;
               vehicle_status = 0;
               Serial.println("Vehicle OFF : Alcohol detetced");
               lcd.print("Alcohol present");
               gsmSerial.println("Send SMS");
               
               delay(2000);
           }

           }
       }
       lcd.clear();
     }
  lcd.clear();
  data = Serial.read();
  if(data == 'Y')
  {
    get_finger_id();
    return;
  }
}


void get_finger_id()                     
{
 Serial.println("Ready to enroll a fingerprint!");
 Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
 id = readnumber();
 if (id == 0) 
 {// ID #0 not allowed, try again!
    return;
 }
 Serial.print("Enrolling ID #");
 Serial.println(id);
 while (!  getFingerprintEnroll() );
}

uint8_t getFingerprintEnroll() 
{
  int p = -1;
 Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
 while (p != FINGERPRINT_OK) 
 {
   p = finger.getImage();
   switch (p) 
   {
   case FINGERPRINT_OK:
     Serial.println("Image taken");
     break;
   case FINGERPRINT_NOFINGER:
     Serial.println(".");
     break;
   case FINGERPRINT_PACKETRECIEVEERR:
     Serial.println("Communication error");
     break;
   case FINGERPRINT_IMAGEFAIL:
     Serial.println("Imaging error");
     break;
   default:
     Serial.println("Unknown error");
     break;
   }
 }
// OK success!
 p = finger.image2Tz(1);
 switch (p) 
 {
   case FINGERPRINT_OK:
     Serial.println("Image converted");
     break;
   case FINGERPRINT_IMAGEMESS:
     Serial.println("Image too messy");
     return p;
   case FINGERPRINT_PACKETRECIEVEERR:
     Serial.println("Communication error");
     return p;
   case FINGERPRINT_FEATUREFAIL:
     Serial.println("Could not find fingerprint features");
     return p;
   case FINGERPRINT_INVALIDIMAGE:
     Serial.println("Could not find fingerprint features");
     return p;
   default:
     Serial.println("Unknown error");
     return p;
 }
 Serial.println("Remove finger");
 delay(2000);
 p = 0;
 while (p != FINGERPRINT_NOFINGER)
 {
   p = finger.getImage();
 }
 Serial.print("ID "); Serial.println(id);
 p = -1;
 Serial.println("Place same finger again");
 while (p != FINGERPRINT_OK) 
 {
   p = finger.getImage();
   switch (p) 
   {
    case FINGERPRINT_OK:
     Serial.println("Image taken");
     break;
   case FINGERPRINT_NOFINGER:
     Serial.print(".");
     break;
   case FINGERPRINT_PACKETRECIEVEERR:
     Serial.println("Communication error");
     break;
   case FINGERPRINT_IMAGEFAIL:
     Serial.println("Imaging error");
     break;
   default:
     Serial.println("Unknown error");
     break;
   }
 }
 p = finger.image2Tz(2);
 switch (p) 
 {
   case FINGERPRINT_OK:
     Serial.println("Image converted");
     break;
   case FINGERPRINT_IMAGEMESS:
     Serial.println("Image too messy");
     return p;
   case FINGERPRINT_PACKETRECIEVEERR:
     Serial.println("Communication error");
     return p;
   case FINGERPRINT_FEATUREFAIL:
     Serial.println("Could not find fingerprint features");
     return p;
   case FINGERPRINT_INVALIDIMAGE:
     Serial.println("Could not find fingerprint features");
     return p;
   default:
     Serial.println("Unknown error");
     return p;
 }
 // OK converted!
 Serial.print("Creating model for #");  Serial.println(id);
 p = finger.createModel();
 if (p == FINGERPRINT_OK) 
 {
   Serial.println("Prints matched!");
 } 
 else if (p == FINGERPRINT_PACKETRECIEVEERR)
 {
   Serial.println("Communication error");
   return p;
 }
 else if (p == FINGERPRINT_ENROLLMISMATCH)
 {
  Serial.println("Fingerprints did not match");
   return p;
 } 
 else 
 {
  Serial.println("Unknown error");
   return p;
 } 
 Serial.print("ID "); Serial.println(id);
 p = finger.storeModel(id);
 if (p == FINGERPRINT_OK) 
 {
   Serial.println("Stored!");
 } 
 else if (p == FINGERPRINT_PACKETRECIEVEERR) 
 {
   Serial.println("Communication error");
   return p;
 }
 else if (p == FINGERPRINT_BADLOCATION) 
 {
   Serial.println("Could not store in that location");
   return p;
 }
 else if (p == FINGERPRINT_FLASHERR) 
 {
   Serial.println("Error writing to flash");
   return p;
 }
 else 
 {
   Serial.println("Unknown error");
   return p;
 }  
}

uint8_t check_finger()
{
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    lcd.print("Unknown person detected");
    wrong_person_counter = wrong_person_counter +1;
    Serial.print("wrong_person_counter : ");
    Serial.println(wrong_person_counter);
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  lcd.print("Found ID:");
  lcd.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence); 
  access = true;
  lcd.setCursor(0,1);
  return finger.fingerID;
}

void send_msg_to(char *string_ptr_2)
{
    gsmSerial.print("AT+CMGS=");
    gsmSerial.write(0x22);
    while(*string_ptr_2)
    gsmSerial.write(*string_ptr_2++);
    gsmSerial.write(0x22);
    gsmSerial.write(0x0D);
    delay(250);
    gsmSerial.print("\n \r");
}

void send_message(char *string_ptr_3)
{
    while(*string_ptr_3)
    gsmSerial.write(*string_ptr_3++);
    gsmSerial.write(0x1A);
 
    delay(250);
 
    gsmSerial.print("\n \r");
}

void send_gsm_cmd(char *string_ptr_1)
{
  while(*string_ptr_1)
  gsmSerial.print(*string_ptr_1++);

  gsmSerial.write(0x0D);

  delay(250);
 
  gsmSerial.print("\n \r");
}

