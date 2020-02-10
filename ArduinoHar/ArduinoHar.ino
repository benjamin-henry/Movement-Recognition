#include <Ben_LSM9DS1.h>
#include <threadedtimer.h>
#include <ArduinoBLE.h>

char UUID[] = "917649A0-D98E-11E5-9EEC-0002A5D5C51B";
BLEService BLE_UART_SERVICE(UUID);
BLEStringCharacteristic BLE_TX(UUID, BLERead | BLENotify, 128);
BLEStringCharacteristic BLE_RX(UUID, BLEWrite, 50);
BLECharacteristic axlStreamCharacteristic(UUID, BLERead | BLENotify, 12);

ThreadedTimer thread_inertial;
int task_inertial;
int inertial_fs = 20; //Hz or 50ms
bool inertial_flag = 0;
float axl_data[3] = {0.};

bool device_locked = true;
bool calib_mode = false;

char buffer[128];
int cnt = 0;
bool ready = 0;

bool write_msg = 0;
String msg_to_send = "";

bool axl_stream_enabled = true;
bool axl_stream_new_value = false;

String device_serial_number = "LMS9DS1-BLE";

void inertial_tick() {
  if (!calib_mode) {
    if (IMU.accelerationAvailable()) {
      IMU.readAcceleration(axl_data[0], axl_data[1], axl_data[2]);
      axl_stream_new_value = 1;
    }
  }
}

void get_serial_data() {
  while (Serial.available()) {
    char c = Serial.read();
    delay(2);
    buffer[cnt++] = c;
    if (c == '\n' || cnt == sizeof(buffer) - 1) {
      buffer[cnt] = '\0';
      cnt = 0;
      ready = 1;
    }
  }
}

void tx_buff(String msg) {
  msg_to_send = msg;
  write_msg = 1;
}

void ble_rx2buff(String msg) {
  int n = msg.length();
  msg.toCharArray(buffer, n + 1);
}

void parseLine(char buff[]) {
  char *token;
  token = strtok(buff, ":");
  if (!strcmp(token, "ulk")) {
    token = strtok(NULL, ":");
    if (!strcmp(token, "RFYVtLE2pbWe83TI")) {
      device_locked = 0;
      tx_buff("ulk:");
    }
  }
  if (!strcmp(token, "lck")) {
    device_locked = 1;
    tx_buff("lck:");
  }
  if (!strcmp(token, "sn")) {
    token = strtok(NULL, ":");
    if (!strcmp(token, "get")) {
      tx_buff(String(String("sn:get:") + device_serial_number));
    }
    if (!strcmp(token, "set")) {
      if (!device_locked) {
        token = strtok(NULL, ":");
        device_serial_number = token;
      }
      else {
        tx_buff("lck:");
      }
    }
  }
  if (!strcmp(token, "stream")) {
    token = strtok(NULL, ":");
    if (device_locked) {
      tx_buff("lck:");
    }
    else {
      if (!strcmp(token, "on")) {
        axl_stream_enabled = true;
        tx_buff("stream:on:");
      }
      else if (!strcmp(token, "off")) {
        axl_stream_enabled = false;
        tx_buff("stream:off:");
      }
    }
  }
  if (!strcmp(token, "calib")) {
    if (device_locked) {
      tx_buff("lck:");
    }
    else {
      token = strtok(NULL, ":");
      calib_mode = 1;
      if (!strcmp(token, "exit")) {
        calib_mode = 0;
      }
      if (!strcmp(token, "axl")) {
        token = strtok(NULL, ":");
        if (!strcmp(token, "pos")) {
          token = strtok(NULL, ":");
          if (!strcmp(token, "zup")) {
            IMU.recordAcceleration(0); //register axl values with Z axis up
            tx_buff(String("axl:pos:zup:" + IMU.axlMeasuredValue(0, 3)));
          }
          else if (!strcmp(token, "zdown")) {
            IMU.recordAcceleration(1); //register axl values with Z axis down
            tx_buff(String("axl:pos:zdown:" + IMU.axlMeasuredValue(1, 3)));
          }
          else if (!strcmp(token, "yup")) {
            IMU.recordAcceleration(2); //register axl values with Y axis up
            tx_buff(String("axl:pos:yup:" + IMU.axlMeasuredValue(2, 3)));
          }
          else if (!strcmp(token, "ydown")) {
            IMU.recordAcceleration(3); //register axl values with Y axis down
            tx_buff(String("axl:pos:ydown:" + IMU.axlMeasuredValue(3, 3)));
          }
          else if (!strcmp(token, "xup")) {
            IMU.recordAcceleration(4); //register axl values with X axis up
            tx_buff(String("axl:pos:xup:" + IMU.axlMeasuredValue(4, 3)));
          }
          else if (!strcmp(token, "xdown")) {
            IMU.recordAcceleration(5); //register axl values with X axis down
            tx_buff(String("axl:pos:xdown:" + IMU.axlMeasuredValue(5, 3)));
          }
        }
        if (!strcmp(token, "go")) {
          IMU.calibrateAcceleration();
          tx_buff(String("axl:params:" + IMU.axlParams2str(5)));
        }
        if (!strcmp(token, "params")) {
          token = strtok(NULL, ":");
          tx_buff(String("axl:params:" + IMU.axlParams2str(5)));
        }
      }
    }
    calib_mode = 0;
  }
  /*while ( token != NULL ) {
    token = strtok(NULL, ":");
    }*/
}

void setup() {
  Serial.begin(115200);
  //while (!Serial);
  //Serial.setTimeout(10);
  while (!IMU.begin());
  task_inertial = thread_inertial.every(int(1000 / inertial_fs), inertial_tick);

  while (!BLE.begin());
  BLE.setLocalName("HAR device");
  BLE.setAdvertisedService(BLE_UART_SERVICE);
  BLE_UART_SERVICE.addCharacteristic(BLE_TX);
  BLE_UART_SERVICE.addCharacteristic(BLE_RX);
  BLE_UART_SERVICE.addCharacteristic(axlStreamCharacteristic);
  BLE.addService(BLE_UART_SERVICE);
  BLE.advertise();
}

void loop() {
  BLEDevice central = BLE.central();
  if (central.connected()) {
    if (axl_stream_enabled && axl_stream_new_value) {
      axlStreamCharacteristic.setValue((byte *) &axl_data, 12);
      axl_stream_new_value = 0;
    }
    if (write_msg) {
      BLE_TX.writeValue(msg_to_send);
      msg_to_send = "";
      write_msg = 0;
    }
    if (BLE_RX.written()) {
      String received_msg = BLE_RX.value();
      ble_rx2buff(received_msg);
      parseLine(buffer);
      memset(buffer, 0, sizeof(buffer));
    }
  }
  /*if (ready) {
    parseLine();
    memset(buffer, 0, sizeof(buffer));
    ready = false;
    } else
    get_serial_data();*/
}
