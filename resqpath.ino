/*************************************************************
   SMART TRAFFIC MANAGEMENT – 2 SIGNALS
   Components: ESP32 + NEO-6M + RC522 + 2×IR + 2×LED Sets + Blynk
*************************************************************/

#define BLYNK_TEMPLATE_ID "Your_Template_ID"
#define BLYNK_TEMPLATE_NAME "Smart Traffic"
#define BLYNK_AUTH_TOKEN "Your_Auth_Token"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Your_WiFi_Name";
char pass[] = "Your_WiFi_Password";

// ---------- RFID ----------
#define SS_PIN 21
#define RST_PIN 22
MFRC522 rfid(SS_PIN, RST_PIN);

// ---------- GPS ----------
TinyGPSPlus gps;
HardwareSerial neogps(1);

// ---------- IR SENSORS ----------
#define IR1 32
#define IR2 33

// ---------- SIGNAL 1 ----------
#define R1 25
#define Y1 26
#define G1 27

// ---------- SIGNAL 2 ----------
#define R2 14
#define Y2 12
#define G2 13

bool ambSig1 = false;
bool ambSig2 = false;

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  neogps.begin(9600, SERIAL_8N1, 16, 17);
  Blynk.begin(auth, ssid, pass);

  SPI.begin();
  rfid.PCD_Init();

  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  pinMode(R1, OUTPUT); pinMode(Y1, OUTPUT); pinMode(G1, OUTPUT);
  pinMode(R2, OUTPUT); pinMode(Y2, OUTPUT); pinMode(G2, OUTPUT);

  digitalWrite(R1, HIGH);
  digitalWrite(R2, HIGH);
}

// ---------- MAIN LOOP ----------
void loop() {
  Blynk.run();
  readGPS();
  checkRFID();
  checkIR();
  normalCycle();
}

// ---------- GPS ----------
void readGPS() {
  while (neogps.available() > 0) {
    if (gps.encode(neogps.read()) && gps.location.isValid()) {
      float lat = gps.location.lat();
      float lon = gps.location.lng();
      Blynk.virtualWrite(V1, lat);
      Blynk.virtualWrite(V2, lon);
    }
  }
}

// ---------- RFID ----------
void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  Serial.print("RFID UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) Serial.print(rfid.uid.uidByte[i], HEX);
  Serial.println();

  // Example: use tag’s first byte to choose signal
  if (rfid.uid.uidByte[0] == 0xA3) { ambSig1 = true; ambSig2 = false; greenSignal1(); }
  else if (rfid.uid.uidByte[0] == 0xB2) { ambSig2 = true; ambSig1 = false; greenSignal2(); }

  rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
}

// ---------- IR ----------
void checkIR() {
  if (ambSig1 && digitalRead(IR1) == LOW) { ambSig1 = false; normalCycle(); }
  if (ambSig2 && digitalRead(IR2) == LOW) { ambSig2 = false; normalCycle(); }
}

// ---------- NORMAL CYCLE ----------
void normalCycle() {
  if (!ambSig1 && !ambSig2) {
    digitalWrite(G1, LOW); digitalWrite(Y1, LOW); digitalWrite(R1, HIGH);
    digitalWrite(G2, LOW); digitalWrite(Y2, LOW); digitalWrite(R2, HIGH);
    Blynk.virtualWrite(V3, "Signal 1: RED");
    Blynk.virtualWrite(V4, "Signal 2: RED");
  }
}

// ---------- SIGNAL 1 GREEN ----------
void greenSignal1() {
  digitalWrite(R1, LOW); digitalWrite(Y1, LOW); digitalWrite(G1, HIGH);
  digitalWrite(R2, HIGH); digitalWrite(G2, LOW);
  Blynk.virtualWrite(V3, "Signal 1: GREEN (AMBULANCE)");
  Blynk.virtualWrite(V4, "Signal 2: RED");
  delay(5000);
  digitalWrite(G1, LOW); digitalWrite(Y1, HIGH); delay(2000);
  digitalWrite(Y1, LOW); digitalWrite(R1, HIGH);
}

// ---------- SIGNAL 2 GREEN ----------
void greenSignal2() {
  digitalWrite(R2, LOW); digitalWrite(Y2, LOW); digitalWrite(G2, HIGH);
  digitalWrite(R1, HIGH); digitalWrite(G1, LOW);
  Blynk.virtualWrite(V4, "Signal 2: GREEN (AMBULANCE)");
  Blynk.virtualWrite(V3, "Signal 1: RED");
  delay(5000);
  digitalWrite(G2, LOW); digitalWrite(Y2, HIGH); delay(2000);
  digitalWrite(Y2, LOW); digitalWrite(R2, HIGH);
}
