#include <Servo.h>
#include <XBee.h>
#include <SoftwareSerial.h>

#define MESSAGE_DELAY_PERIOD  5000 // may take this out in favor of turned repot

const uint8_t MSG_ALIVE = 0xB0,
              MSG_TRIP1 = 0xB1,
              MSG_UNTRIP1 = 0xB2,
              MSG_TRIP2 = 0xB3,
              MSG_UNTRIP2 = 0xB4;

Servo wheels; // servo for turning the wheels
Servo esc; // not actually a servo, but controlled like one!
int pos = 90;
int speed = 90;
int count = 0;
int triggerCount = 0;
bool trigger = false;
bool complete = false;
int forwardTime = 1000;
int backwardTime = 600;
int pauseTime = 500;
int servoMoveTime = 600;
int straightTime = 1500;
int timeStamp = millis();

XBee xbee = XBee();
SoftwareSerial xbeeSerial(2, 3);
ZBRxResponse rxResponse = ZBRxResponse();
ZBTxRequest txRequest;

void setup() {
  delay(3000);
  wheels.attach(8); // initialize wheel servo to Digital IO Pin #8
  esc.attach(9); // initialize ESC to Digital IO Pin #9
  wheels.write(pos);
  esc.write(speed);

  Serial.begin(57600);
  xbeeSerial.begin(57600);
  xbee.begin(xbeeSerial);
  delay(2000);
}

void loop() {
  readAndHandlePackets();
  if (trigger && millis() - timeStamp > 5000) {
    left_forward();
    pause_vehicle();
    right_backward();
    pause_vehicle();
    straight_forward();
    pause_vehicle();
    timeStamp = millis();
  }
  if (trigger && millis() - timeStamp < 5000) trigger = false;
}

void left_forward(void) {
  wheels.write(130);
  delay(servoMoveTime);
  for (speed = 90; speed > 60; speed--) {
    esc.write(speed);
  }
  delay(forwardTime);
}

void right_backward(void) {
  wheels.write(50);
  delay(servoMoveTime);
  for (speed = 90; speed < 120; speed++) {
    esc.write(speed);
  }
  delay(backwardTime);
}

void pause_vehicle(void) {
  esc.detach();
  delay(pauseTime);
  esc.attach(9);
  esc.write(90);
}

void straight_forward(void) {
  wheels.write(90);
  delay(servoMoveTime);
  for (speed = 90; speed > 60; speed--) {
    esc.write(speed);
  }
  delay(straightTime);
}

void serialLog(bool in, uint32_t address64, uint8_t payload) {
  if (in)  Serial.print("MSG_IN");
  else Serial.print("                                       MSG_OUT");
  Serial.print(":");
  Serial.print(address64, HEX);
  Serial.print(":");
  switch (payload) {
    case MSG_ALIVE: Serial.println("ALIVE"); break;
    case MSG_TRIP1: Serial.println("TRIP1"); break;
    case MSG_TRIP2: Serial.println("TRIP2"); break;
    case MSG_UNTRIP1: Serial.println("UNTRIP1");  break;
    case MSG_UNTRIP2: Serial.println("UNTRIP2");  break;
  }
}

void readAndHandlePackets(void) {
  if (xbee.readPacket(1) && xbee.getResponse().getApiId() == ZB_RX_RESPONSE) {
    xbee.getResponse().getZBRxResponse(rxResponse);
    switch (rxResponse.getData(0)) {
      case MSG_TRIP1:
        trigger = true;
        complete = false;
        break;
      case MSG_UNTRIP1:
        trigger = false;
        break;
      case MSG_TRIP2:
        triggerCount++;
        break;
      case MSG_UNTRIP2:
        if (triggerCount > 1) {
          complete = true;
        }
        break;

    }
  }
}
