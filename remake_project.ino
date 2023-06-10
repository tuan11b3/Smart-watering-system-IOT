#include "DHT.h"                                                   // Thu vien dht11
#include <LiquidCrystal_I2C.h>                                     // thu vien I2C
#include <C:\Users\ASUS\Documents\firstArduino\libraries\pitch.h>  // thu vien note
#include <ESP8266WiFi.h>                                           //thu vien ESP8266WiFi
#include <BlynkSimpleEsp8266.h>                                    //thu vien Blynk
#include <SimpleTimer.h>                                           //thu vien cho timer.run()
#include <stdlib.h>                                                // thu vien cho itoa

// Dinh nghia hang cac chan Digital va Analog
#define waterPump 0 // chan digital doc cam bien relay
#define sensor A0 //chan Analog doc cam bien dat
//#define buttonInterrupt 12  //Chan Interrupt
#define BUZZER_PIN 13       //Chân được nối với loa hoặc buzzer
#define BLYNK_PRINT Serial
#define auto_Led 14 //chan digital sang khi o che do Auto
#define manual_Led 15//chan digital sang khi o che do Manual

// Khai bao cac ham
void loadBegin(); // ham loading va am nhac khi khoi dong
void ICACHE_RAM_ATTR OnPump(); // ham interrupt, bat trang thai nut 4 chan
void OutP_M(String, int);//ham in message va icon
void autoMode();//ham dieu khien may bom o che do Auto
void manualMode();// ham dieu khien may bom o che do Manual
void sensorDataSend();// Ham gui du lieu len Blink
void analog_M_LCD();// Ham in du lieu len lcd

DHT dht2(2, DHT11);//Khai bao object thuoc class DHT
String message; // khai bao bien message de in len man hinh lcd
LiquidCrystal_I2C lcd(0x27, 16, 2); // khai bao object thuoc class LiquidCrystal_I2C
// khai bao va gan du lieu auth, ssid, pass cho cac bien
char auth[] = "d6wnsgJaPfWOCEmoewYLEuKcveXHvw9M";  
char ssid[] = "HemCut";                            
char pass[] = "room01203";                         
BlynkTimer timer;  // dinh nghia BlynkerTimer object, ho tro 12 timers
float value;
float percent;  // phan tram do am dat
float temp;     // nhiet do khong khi
float humi;     // phan tram do am khong khi
int c = 0;  // khai bao bien va gan la Off, trang thai cua may bom
int dem = 0; // dem so lan xuat hien, thong tin temp, humi, moist 4 lan * 500 = 2 giay. Thong tin tuoi 2*500 = 1 giay
int value1, button = 0, Ref1 = 50, Ref2 = 25; // khai bao cac bien, lay gia tri tu Blynk: trang thai auto hay manual, button state, do am kk, nhiet do kk

// Khai báo mang kieu byte, co gia tri tu 0 den 255
byte tuoi[8] = {
  0b00100,
  0b01110,
  0b11001,
  0b11101,
  0b11111,
  0b01110,
  0b00000,
  0b00000
};

byte koTuoi[8] = {
  0b11111,
  0b10001,
  0b10111,
  0b10001,
  0b10111,
  0b10001,
  0b11111,
  0b00000
};

// cac note bai merryChristmas
int melody[] = {
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5,
  NOTE_E5,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
  NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5,
  NOTE_D5, NOTE_G5
};

// thoi luong cac note 4 la 1/4 note = 250, 8 la 1/8 note
int durations[] = {
  8, 8, 4,
  8, 8, 4,
  8, 8, 8, 8,
  2,
  8, 8, 8, 8,
  8, 8, 8, 16, 16,
  8, 8, 8, 8,
  4, 4
};

// cac note bai Nokia
int melody2[] = {

  // Nokia Ringtone
  // Score available at https://musescore.com/user/29944637/scores/5266155

  NOTE_E5,
  8,
  NOTE_D5,
  8,
  NOTE_FS4,
  4,
  NOTE_GS4,
  4,
  NOTE_CS5,
  8,
  NOTE_B4,
  8,
  NOTE_D4,
  4,
  NOTE_E4,
  4,
  NOTE_B4,
  8,
  NOTE_A4,
  8,
  NOTE_CS4,
  4,
  NOTE_E4,
  4,
  NOTE_A4,
  2,
};


// Doc gia tri auto hay manual cua App (chi khi bi thay doi, hay cap nhat)
BLYNK_WRITE(V6) {
  buzzer_on_Pump();
  value1 = param.asInt();  // doc gia tri virtual pin
  if (value1 != 1) {
    digitalWrite(waterPump, LOW);
    c = 0;
  }
}

// Doc gia tri nguong do am kk cua App(chi khi bi thay doi, hay cap nhat)
BLYNK_WRITE(V5) {
  Ref1 = param.asInt();
}

// Doc gia tri nguong nhiet do kk cua App(chi khi bi thay doi, hay cap nhat)
BLYNK_WRITE(V4) {
  Ref2 = param.asInt();
}

// Doc gia tri relay cua App(chi khi bi thay doi, hay cap nhat)
BLYNK_WRITE(V1) {
  if (value1 != 1) {
    button = param.asInt();
    manualMode();
  }
}

// ham o che do tuoi tu dong, tuoi dua theo cac dieu kien nhietDo va doAm kk


//ham setup
void setup() {
  Serial.begin(9600); //thiết lập giao tiếp nối tiếp giữa bo Arduino và laptop
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80); // ham begin -> connect Wifi, set Auth, ssid, pass, co gang ket noi voi sever ( ngat neu qua 30s)
  pinMode(BUZZER_PIN, OUTPUT);// che do cho chan
  pinMode(waterPump, OUTPUT);
  pinMode(auto_Led, OUTPUT);// Led cho de do Auto 
  pinMode(manual_Led, OUTPUT);// Led cho che do Manual
  digitalWrite(waterPump, button);
  digitalWrite(auto_Led, HIGH);// ghi trang thai cho chan digital (HIGH or LOW)
  digitalWrite(manual_Led, HIGH);
  //attachInterrupt(buttonInterrupt, OnPump, CHANGE);// chan Interrupt va ham OnPump se duoc goi

  lcd.init();  // khoi tao man hinh LED
  lcd.backlight();
  lcd.createChar(0, tuoi); // tao icon giot nuoc
  lcd.createChar(1, koTuoi); // tao icon E

  timer.setInterval(250L, sensorDataSend); // duLieu tu cac cam bien duoc gui len App moi 1/4s
  timer.setInterval(500L, analog_M_LCD);// in du lieu va message len lcd moi 1/2s

// set cac du lieu cam bien, trang thai button tren App
  Blynk.virtualWrite(V1, 0);
  Blynk.virtualWrite(V6, 0);
  Blynk.virtualWrite(V5, 50);
  Blynk.virtualWrite(V4, 25);
  Blynk.virtualWrite(V0, 0);
  Blynk.virtualWrite(V2, 0);
  Blynk.virtualWrite(V3, 0);

  loadBegin();
}

// ham void loop
void loop() {
  Blynk.run(); //ham chính chịu trách nhiệm duy trì kết nối, gửi dữ liệu, nhận dữ liệu voi Blynk App
  timer.run();  // đối tượng hẹn gio.
}

void autoMode() {
  char Ref1_str[5];
  char Ref2_str[5];

  itoa(Ref1, Ref1_str, 10);
  itoa(Ref2, Ref2_str, 10);

  if (percent < Ref1 && temp < Ref2) {
    message = "Can tuoi";
    c = 1;
  } else if (percent < Ref1 && temp >= Ref2) {
    message = "Nhiet do > " + String(Ref2_str);
    c = 0;
  } else if (percent >= Ref1 && temp >= Ref2) {
    message = "Do am>" +  String(Ref1_str) +  "nh_Do>" + String(Ref2_str);
    c = 0;
  } else if (percent >= Ref1 && temp < Ref2) {
    message = "Do am > " + String(Ref1_str);
    c = 0;
  } else {
    message = "nan ERROR";
    c = 0;
  }
  if (c) digitalWrite(waterPump, HIGH);
  else digitalWrite(waterPump, LOW);
}

// Ham bat may bom neu button o trang thai On va nguoc lai
void manualMode() {
  if (button == 1) {
    digitalWrite(waterPump, HIGH);
  } else {
    digitalWrite(waterPump, LOW);
  }
}

//Ham dua message ra man hinh lcd
void OutP_M(String message, int c) {
  lcd.clear();
  lcd.print(message);
  lcd.setCursor(0, 1);
  if (c)
    lcd.write(0);
  else {
    lcd.write(1);
    lcd.setCursor(2, 3);
    lcd.print("ko nen tuoi");
  }
}

//doc du lieu tu cam bien, roi  gui du lieu len App
void sensorDataSend() {
  temp = dht2.readTemperature(); // doc du lieu tu cam bien do am dat
  humi = dht2.readHumidity();
  value = analogRead(sensor); // ham chuyen tin hieu analog sang tin hieu digital tu 0 1023
  percent = map(value, 0, 1023, 0, 100);// dua digital sang ty le 100
  percent = 100 - percent;  // khi do am  = 0, gia tri analog la -> 1023 ,nghia la 100 => ta phai lay 100 de tru
  Blynk.virtualWrite(V0, percent);// gui gia tri len App
  Blynk.virtualWrite(V2, temp);
  Blynk.virtualWrite(V3, humi);
}

// ham loading, phat nhac khoi dong
void loadBegin() {
  int size = sizeof(durations) / sizeof(int);  // so luong cac note
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.print("Loading");
  int position = 0;
  lcd.setCursor(0, 1);
  for (int note = 0; note < size; note++) {
    int duration = 1000 / durations[note]; // thoi luong moi note la 1s
    tone(BUZZER_PIN, melody[note], duration);
    position += 1;
    if (position >= 15) {
      lcd.clear();
      position = 0;
    } else {
      lcd.print(".");
    }

    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);

    //stop the tone playing:
    noTone(BUZZER_PIN);
  }
}

/*
// ham interrupt
void ICACHE_RAM_ATTR OnPump() {
    Blynk.virtualWrite(V6, 0);
  if(digitalRead(waterPump))
  digitalWrite(waterPump, LOW);
  else
  digitalWrite(waterPump, HIGH);
}
*/

// ham in message va gia tri cac cam bien len LCD duoc goi moi 1/2s
void analog_M_LCD() {
  dem += 1;
  if (dem > 6) { dem = 0; }

  if (dem % 8 < 5) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Moisture: ");
    lcd.print(percent);
    lcd.print("%");

    lcd.setCursor(0, 1);
    lcd.print("T:");
    lcd.print(int(temp));
    lcd.print("oC");

    lcd.setCursor(8, 1);
    lcd.print("H:");
    lcd.print(int(humi));
    lcd.print("%");
  } else if (dem % 8 >= 5) {
    lcd.clear();
    if (value1) {
      digitalWrite(auto_Led, HIGH);
      digitalWrite(manual_Led, LOW);
      autoMode();
      OutP_M(message, c);
    } else {
      digitalWrite(auto_Led, LOW);
      digitalWrite(manual_Led, HIGH);
      if (button == 1)
        lcd.print("Bat thu cong");
      else
        lcd.print("Tat thu cong");
    }
  }
}

// nhac Nokia khi thay doi che do Auto hay Manual
void buzzer_on_Pump() {
  int tempo = 180;
  int notes = sizeof(melody2) / sizeof(melody2[0]) / 2;
  int wholenote = (60000 * 4) / tempo;
  int divider = 0, noteDuration = 0;

  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    // tinhs khoang thoi gian moi note
    divider = melody2[thisNote + 1];
    if (divider > 0) {
      // note binh thuon, chi xu ly
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // ghi chú chấm được thể hiện với thời lượng âm!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5;  // tăng thời lượng lên một nửa cho ghi chú chấm
    }

    // chỉ chơi nốt trong 90% thời lượng, để lại 10% tạm dừng
    tone(BUZZER_PIN, melody2[thisNote], noteDuration * 0.9);

    // Đợi khoảng thời gian cụ thể trước khi chơi nốt tiếp theo.
    delay(noteDuration);

    // dừng việc tạo dạng sóng trước ghi chú tiếp theo.
    noTone(BUZZER_PIN);
  }
}
