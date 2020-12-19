
/* PJ105 photo-resistor configuration console
   Giovanni Blu Mitolo 2020
 This is a basic example to show how PJ105 can be used practically.
 The Serial monitor is used to configure and print data from PJ105 */

#define PJON_PACKET_MAX_LENGTH 20
#define PJON_MAX_PACKETS 2
#include <PJONSoftwareBitBang.h>

PJONSoftwareBitBang bus(PJON_MASTER_ID);

uint8_t packet[20];
String string_number;
int req_index = 0;
uint8_t recipient_id = PJON_NOT_ASSIGNED;

void setup() {
  bus.strategy.set_pin(12);
  bus.begin();
  bus.set_receiver(receiver_function);
  bus.set_error(error_handler);
  Serial.begin(115200);
  Serial.println("PJ105 photo-resistor sensor configuration console - Giovanni Blu Mitolo 2020");
  print_help();
};

void receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
  /* Make use of the payload before sending something, the buffer where payload points to is
     overwritten when a new message is dispatched */
  if(length == 6) {
    Serial.print("Module info: ");
    for(uint8_t i = 0; i < 5; i++)
      Serial.print((char)payload[i]);
    Serial.print(" - Software version: ");
    Serial.println(payload[5], DEC);
  } else {
    uint16_t v = (uint16_t)((payload[0] << 8) | (payload[1] & 0xFF));
    Serial.print("Photo-resistor analog read value: ");
    Serial.print(v);
    Serial.print(" Photo-resistor voltage: ");
    Serial.println((v / 1023.0) * 5.0);
  }
};

void error_handler(uint8_t code, uint8_t data) {
  if(code == PJON_CONNECTION_LOST) {
    Serial.print("Connection lost - PJ105 ID ");
    Serial.println((uint8_t)bus.packets[data].content[0]);
  }
  if(code == PJON_PACKETS_BUFFER_FULL) {
    Serial.print("Packet buffer is full, has now a length of ");
    Serial.println(data, DEC);
    Serial.println("Possible wrong bus configuration!");
    Serial.println("higher PJON_MAX_PACKETS if necessary.");
  }
  if(code == PJON_CONTENT_TOO_LONG) {
    Serial.print("Content is too long, length: ");
    Serial.println(data);
  }
};

void print_help() {
  Serial.println();
  Serial.println("Use the Serial console input field to configure the PJ105 module.");
  Serial.println("Input commands guide: ");
  Serial.println("?!        -> Help and device info");
  Serial.println("E!        -> Request a photo-resistor reading");
  Serial.println("I1-254!   -> Configure module id");
  Serial.println("R0-255!   -> Configure recipient id");
  Serial.println("T0-65535! -> Configure transmission interval");
  Serial.println("Q!        -> Block incoming configuration (caution)");
  Serial.println("X!        -> Configuration reset");
  Serial.println();
  Serial.println("All commands must end with ! and you must select \"No line ending\"");
  Serial.println();
}

void loop() {
  bus.receive(1000);
  bus.update();

  if(Serial.available() > 0) {
    char req_byte = Serial.read();
    packet[req_index] = (uint8_t)req_byte;
    if(req_index && req_byte != '!')
      string_number += req_byte;

    if(req_byte == '!') {
      uint16_t result = 0;
      uint8_t packet_length = 0;
      // Photoresistor reading request
      if((char)packet[0] == 'E') {
        bus.send(recipient_id, "E", 1);
        Serial.println("Photoresistor reading requested");
        Serial.flush();
      }
      // Module id update
      if((char)packet[0] == 'M') {
        packet_length = 2;
        packet[1] = (uint8_t)string_number.toInt();
        bus.send(recipient_id, packet, packet_length);
        recipient_id = packet[1];
        Serial.print("Setting PJ105 device id: ");
        Serial.println(packet[1]);
        Serial.flush();
      }
      // Block further incoming configuration
      if((char)packet[0] == 'Q') {
        packet_length = 1;
        bus.send(recipient_id, packet, packet_length);
        Serial.println("Blocking further incoming configuration!");
        Serial.println("Flash the PJ105 module with PJ105.ino to unblock!");
        Serial.print(packet[1]);
        Serial.flush();
      }
      // Device id update
      if((char)packet[0] == 'I') {
        packet_length = 2;
        packet[1] = (uint8_t)string_number.toInt();
        bus.send(recipient_id, packet, packet_length);
        recipient_id = packet[1];
        Serial.print("Setting device id: ");
        Serial.println(packet[1]);
        Serial.flush();
      }
      // Recipient id update
      if((char)packet[0] == 'R') {
        packet[1] = (uint8_t)string_number.toInt();
        packet_length = 2;
        bus.send(recipient_id, packet, packet_length);
        Serial.print("Recipient device id update: ");
        Serial.println(packet[1]);
        bus.set_id(packet[1]);
        Serial.flush();
      }
      // Interval configuration update
      if((char)packet[0] == 'T')  {
        packet[1] = string_number.toInt() >> 8;
        packet[2] = string_number.toInt() & 0xFF;
        packet_length = 3;
        bus.send(recipient_id, packet, packet_length);
        Serial.print("Setting state change interval: ");
        Serial.println(string_number.toInt());
        Serial.flush();
      }
      // Configuration reset to default
      if((char)packet[0] == 'X') {
        packet_length = 1;
        bus.send(recipient_id, packet, packet_length);
        Serial.print("Executing configuration reset!");
        Serial.flush();
      }
      // Print Help
      if((char)packet[0] == '?') {
        print_help();
        bus.send(recipient_id, "?", 1);
      }

      req_index = 0;
      string_number = "";
      return;
    }
    req_index++;
  }
};
