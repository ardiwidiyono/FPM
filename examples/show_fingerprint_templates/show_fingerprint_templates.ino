#include <SoftwareSerial.h>
#include <FPM.h>

#define BUFF_SZ         512

#define TEMPLATE_TO_MOVE            4
#define NEW_TEMPLATE_LOCATION       8

// Download a template, delete it, print it, and upload it to a different flash location

// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE/YELLOW wire)
SoftwareSerial mySerial(2, 3);

FPM finger;

void setup()  
{
  Serial.begin(9600);
  Serial.println("fingertest");
  mySerial.begin(57600);
  
  if (finger.begin(&mySerial)) {
    Serial.println("Found fingerprint sensor!");
    Serial.print("Capacity: "); Serial.println(finger.capacity);
    Serial.print("Packet length: "); Serial.println(finger.packetLen);
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) yield();
  }
  Serial.println("Send any character to continue...");
  
  while (Serial.available() == 0) yield();
  
  getTemplate(TEMPLATE_TO_MOVE);  // download template at #4 (if it exists) to the buffer; first enroll a finger at that location
  deleteTemplate(TEMPLATE_TO_MOVE); // delete template at #4
  sendTemplate(NEW_TEMPLATE_LOCATION);   // upload template in buffer to #8; run fingerprint match example to verify that the new template is now at #8
}

void loop()
{

}

uint8_t buff[BUFF_SZ];

void getTemplate(uint16_t id)
{
  uint8_t p = finger.loadModel(id);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.print("template "); Serial.print(id); Serial.println(" loaded");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return;
    default:
      Serial.print("Unknown error "); Serial.println(p);
      return;
  }

  // OK success!

  p = finger.getModel();
  switch (p) {
    case FINGERPRINT_OK:
      break;
   default:
      Serial.print("Unknown error "); Serial.println(p);
      return;
  }
  
  bool last;
  int count = 0;
  uint16_t buflen = BUFF_SZ;
  uint16_t pos = 0;
  
  while (true){
    bool ret = finger.readRaw(buff + pos, ARRAY_TYPE, &last, &buflen);
    if (ret){
      count++;
      pos += buflen;
      buflen = BUFF_SZ - pos;
      if (last)
        break;
    }
    else {
      Serial.println("Error receiving packet");
      return;
    }
    yield();
  }

  Serial.println("---------------------------------------------");
  for (int i = 0; i < 32; i++){
    for (int j = 0; j < 16; j++){
      Serial.print(buff[i*16 + j], HEX);
      Serial.print(" ");
    }
    Serial.println();
    yield();
  }
  Serial.println("--------------------------------------------");

  Serial.print(count * finger.packetLen); Serial.println(" bytes read");
}

void deleteTemplate(uint16_t id){
  int p = finger.deleteModel(id, 1);
  switch (p){
    case FINGERPRINT_OK:
      Serial.print("Template "); Serial.print(id); Serial.println(" deleted");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Comms error");
      break;
    case FINGERPRINT_BADLOCATION:
      Serial.println("Could not delete from that location");
      break;
    case FINGERPRINT_FLASHERR:
      Serial.println("Error writing to flash");
      break;
    default:
      Serial.println("Unknown error");
  }
  return;
}

void sendTemplate(uint16_t id){
  int p = finger.uploadModel();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Starting template upload");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Comms error");
      return;
    case FINGERPRINT_PACKETRESPONSEFAIL:
      Serial.println("Did not receive packet");
      return;
    default:
      Serial.println("Unknown error");
      return;
  }

  yield();
  finger.writeRaw(buff, BUFF_SZ);

  p = finger.storeModel(id);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.print("Template stored at ID "); Serial.println(id);
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Comms error");
      break;
    case FINGERPRINT_BADLOCATION:
      Serial.println("Could not store in that location");
      break;
    case FINGERPRINT_FLASHERR:
      Serial.println("Error writing to flash");
      break;
    default:
      Serial.println("Unknown error");
  }
  return;
}

