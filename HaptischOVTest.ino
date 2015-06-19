#include <SPI.h>
#include <MFRC522.h>
#include <pitches.h>

#define RST_PIN 9
#define SS_PIN 10

/////////////////////////////////////////////////////////////////////////

//                          6           2           9         1          5          4           7           8           3

int testOn [] =             {3,         3,          2,        3,         3,         3,          2,          2,         3};
int testOff [] =            {2,         2,          4,        2,         2,         2,          4,          4,         2};
int testDuration [] =       {50,        100,        30,       100,        50,       50,         30,        30,        100};
int testInterval [] =       {50,        50,         50,       50,        50,        50,         50,        50,         50};
String testSituation [] =   {"error",   "uitcheck", "error",  "incheck", "incheck", "uitcheck", "incheck", "uitcheck", "error"};

int testPosition = 0;
/////////////////////////////////////////////////////////////////////////

const int redLed =  7;      // the number of the LED pin
const int greenLed = 6;
const int speaker = 4;
const int motorPin = 3;
const int buttonPin = 2;

const String accessCard = "EDFCC25";

// Variables will change:
int ledState = LOW;             // ledState used to set the LED

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.


byte readCard[4];    // Stores scanned ID read from RFID Module

int melody[] = {
  1000
};

int noteDurations[] = {
  2
};

int buttonState = 0;         // variable for reading the pushbutton status
bool state = false;



MFRC522::MIFARE_Key key;

/**
 * Initialize.
 */
void setup() {

  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(buttonPin, INPUT);

  note(1000, 4);

  Serial.begin(9600); // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("Scan a MIFARE Classic PICC to demonstrate read and write."));
  Serial.print(F("Using key (for A and B):"));
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();

  Serial.println(F("BEWARE: Data will be written to the PICC, in sector #1"));
}

/**
 * Main loop.
 */
void loop() {

  String cardID = getID();
  cardID.toUpperCase();

  if (cardID != "") {
    Serial.println(testPosition + 1);
    if (testSituation[testPosition] == "incheck") {
      checkIn(testOn[testPosition], testOff[testPosition], testDuration[testPosition]);
    }
    else if (testSituation[testPosition] == "uitcheck") {
      checkOut(testOn[testPosition], testOff[testPosition], testDuration[testPosition], testInterval[testPosition]);
    }
    else {
      checkError(testOn[testPosition], testOff[testPosition], testDuration[testPosition], testInterval[testPosition]);
    }
    testPosition++;
    if (testPosition >= 9) {
      testPosition = 0;
    }
  }
}

/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

String getID() {
  String cardID;
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return "";
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return "";
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  Serial.println(F("Scanned PICC's UID:"));
  for (int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    cardID += String(readCard[i], HEX);
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return cardID;
}

void defaultLoop() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent())
    return;

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial())
    return;

  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  // Check for compatibility
  if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
          &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
          &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("This sample only works with MIFARE Classic cards."));
    return;
  }

  // In this sample we use the second sector,
  // that is: sector #1, covering block #4 up to and including block #7
  byte sector         = 1;
  byte blockAddr      = 4;
  byte dataBlock[]    = {
    0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
    0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
    0x08, 0x09, 0xff, 0x0b, //  9, 10, 255, 12,
    0x0c, 0x0d, 0x0e, 0x0f  // 13, 14,  15, 16
  };
  byte trailerBlock   = 7;
  byte status;
  byte buffer[18];
  byte size = sizeof(buffer);

  // Authenticate using key A
  Serial.println(F("Authenticating using key A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Show the whole sector as it currently is
  Serial.println(F("Current data in sector:"));
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  // Read data from the block
  Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
  Serial.println(F(" ..."));
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();
  Serial.println();

  // Authenticate using key B
  Serial.println(F("Authenticating again using key B..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Write data to the block
  Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
  Serial.println(F(" ..."));
  dump_byte_array(dataBlock, 16); Serial.println();
  status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.println();

  // Read data from the block (again, should now be what we have written)
  Serial.print(F("Reading data from block ")); Serial.print(blockAddr);
  Serial.println(F(" ..."));
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();

  // Check that data in block is what we have written
  // by counting the number of bytes that are equal
  Serial.println(F("Checking result..."));
  byte count = 0;
  for (byte i = 0; i < 16; i++) {
    // Compare buffer (= what we've read) with dataBlock (= what we've written)
    if (buffer[i] == dataBlock[i])
      count++;
  }
  Serial.print(F("Number of bytes that match = ")); Serial.println(count);
  if (count == 16) {
    Serial.println(F("Success :-)"));
  } else {
    Serial.println(F("Failure, no match :-("));
    Serial.println(F("  perhaps the write didn't work properly..."));
  }
  Serial.println();

  // Dump the sector data
  Serial.println(F("Current data in sector:"));
  mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  Serial.println();

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

void note(int freq, int duration) {
  // to calculate the note duration, take one second
  // divided by the note type.
  //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 1000 / duration;
  tone(speaker, freq, noteDuration);
  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 1.30;
  delay(pauseBetweenNotes);
  // stop the tone playing:
  noTone(speaker);
}

void doPulse(int on, int off, int duration) {
  for (int i = 0; i < duration; i++) {
    digitalWrite(motorPin, HIGH);
    delay(on);
    digitalWrite(motorPin, LOW);
    delay(off);
  }
}

void checkIn(int on, int off, int duration) {
  digitalWrite(greenLed, HIGH);
  note(1000, 3);
  doPulse(on, off, duration);
  delay(500);
  digitalWrite(greenLed, LOW);
}

void checkOut(int on, int off, int duration, int interval) {
  digitalWrite(greenLed, HIGH);
  note(1000, 4);
  note(1000, 4);
  doPulse(on, off, duration);
  delay(interval);
  doPulse(on, off, duration);
  delay(interval);
  delay(300);
  digitalWrite(greenLed, LOW);
}

void checkError(int on, int off, int duration, int interval) {
  digitalWrite(redLed, HIGH);
  note(1000, 6);
  note(800, 4);
  note(1000, 6);
  doPulse(on, off, duration);
  delay(interval);
  doPulse(on, off, duration * 0.75);
  delay(interval);
  doPulse(on, off, duration);
  delay(interval);
  delay(150);
  digitalWrite(redLed, LOW);
}

