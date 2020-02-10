/*
  This file is part of the Arduino_LSM9DS1 library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

  ///////////////////////////////////////////////
  Acceleromter calibration : https://www.st.com/content/ccc/resource/technical/document/application_note/a0/f0/a0/62/3b/69/47/66/DM00119044.pdf/files/DM00119044.pdf/jcr:content/translations/en.DM00119044.pdf
*/

#include "LSM9DS1.h"

#define LSM9DS1_ADDRESS            0x6b

#define LSM9DS1_WHO_AM_I           0x0f
#define LSM9DS1_CTRL_REG1_G        0x10
#define LSM9DS1_STATUS_REG         0x17
#define LSM9DS1_OUT_X_G            0x18
#define LSM9DS1_CTRL_REG6_XL       0x20
#define LSM9DS1_CTRL_REG8          0x22
#define LSM9DS1_OUT_X_XL           0x28

// magnetometer
#define LSM9DS1_ADDRESS_M          0x1e

#define LSM9DS1_CTRL_REG1_M        0x20
#define LSM9DS1_CTRL_REG2_M        0x21
#define LSM9DS1_CTRL_REG3_M        0x22
#define LSM9DS1_STATUS_REG_M       0x27
#define LSM9DS1_OUT_X_L_M          0x28

// temperature sensor
#define LSM9DS_OUT_TEMP_L          0x15
#define LSM9DS_OUT_TEMP_H          0x16

// calibration positions
#define axlZup 0
#define axlZdown 1
#define axlYup 2
#define axlYdown 3
#define axlXup 4
#define axlXdown 5

LSM9DS1Class::LSM9DS1Class(TwoWire& wire) :
  _wire(&wire)
{
}

LSM9DS1Class::~LSM9DS1Class()
{
}

int LSM9DS1Class::begin()
{
  for(int i = 0;i<4;i++){
    for(int j = 0;j<3;j++){
      i==j && i < 4 ? _axlCalibrationParameters[i][j]=1. : _axlCalibrationParameters[i][j]=0.;
    }
  }
  for(int i = 0;i < 4; i++) {
    for(int j = 0;j < 3;j++) {
      Serial.print(_axlCalibrationParameters[i][j]);
      Serial.print("\t");
    }
    Serial.println("");
  }

  _wire->begin();

  // reset
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG8, 0x05);
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M, 0x0c);

  //delay(10);

  if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_WHO_AM_I) != 0x68) {
    end();

    return 0;
  }

  if (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_WHO_AM_I) != 0x3d) {
    end();

    return 0;
  }

  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, 0x78); // 119 Hz, 2000 dps, 16 Hz BW
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, 0x70); // 119 Hz, 4G

  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M, 0xb4); // Temperature compensation enable, medium performance, 20 Hz
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M, 0x00); // 4 Gauss
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG3_M, 0x00); // Continuous conversion mode

  return 1;
}

void LSM9DS1Class::end()
{
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG3_M, 0x03);
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, 0x00);
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, 0x00);

  _wire->end();
}

int LSM9DS1Class::readAcceleration(float& x, float& y, float& z)
{
  int16_t data[3];

  if (!readRegisters(LSM9DS1_ADDRESS, LSM9DS1_OUT_X_XL, (uint8_t*)data, sizeof(data))) {
    x = NAN;
    y = NAN;
    z = NAN;

    return 0;
  }

  float tmp[4] = {0.,0.,0.,1.};

  for(int i=0;i<3;i++) tmp[i] = data[i] * 4.0 / 32768.0;

  // dot product between read values and calibration parameters
  float res[3] = {0.};
  for(int c = 0; c < 3; ++c) {
    for(int k = 0; k < 4; ++k) {
      res[c] += tmp[k] * _axlCalibrationParameters[k][c];
    }
  }

  x = res[0];
  y = res[1];
  z = res[2];

  return 1;
}

int LSM9DS1Class::accelerationAvailable()
{
  if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_STATUS_REG) & 0x01) {
    return 1;
  }

  return 0;
}

float LSM9DS1Class::accelerationSampleRate()
{
  return 119.0F;
}

void LSM9DS1Class::recordAcceleration(int position) {
  int num_samples = 119;
  float buff[3] = {0.};

  for(int sample=0;sample<num_samples;sample++) {
    while (!accelerationAvailable()) {}
    float data[3];
    readAcceleration(data[0],data[1],data[2]);
    for(int i = 0; i < 3; i++) {
      buff[i] += data[i];
    }
  }
  for(int i = 0; i < 3; i++) {
    _axlMeasuredValues[position][i] = 0.;
    _axlMeasuredValues[position][i] = buff[i] / num_samples;
  }

  _axlMeasuredValues[position][3] = 1.;

  Serial.print("Recorded ");
  switch (position) {
  case axlZup:
    Serial.print("axlZUp : ");
    break;
  case axlZdown:
    Serial.print("axlZdown : ");
    break;
  case axlYup:
    Serial.print("axlYup : ");
    break;
  case axlYdown:
    Serial.print("axlYdown : ");
    break;
  case axlXup:
    Serial.print("axlXup : ");
    break;
  case axlXdown:
    Serial.print("axlXdown : ");
    break;
  default:
    break;
  }

  for(int i = 0; i < 3; i++) {
    Serial.print(_axlMeasuredValues[position][i]);
    Serial.print("\t");
  }
  Serial.println("");
}

void LSM9DS1Class::calibrateAcceleration() {
  //transpose measured values
  float axlMeasuredValues_T[4][6];
  for(int row=0; row < 6; row++){
    for(int col=0; col < 3; col++){
      axlMeasuredValues_T[col][row]=0.;
      axlMeasuredValues_T[col][row]=_axlMeasuredValues[row][col];
    }
  }

  float dot_prod[4][4] = {0.};
  for(int r = 0; r < 4; r++){
    for(int c = 0; c < 4; c++){
      dot_prod[r][c] = 0.;
      for(int k = 0; k < 6; k++){
        dot_prod[r][c] += axlMeasuredValues_T[r][k] * _axlMeasuredValues[k][c];
      }
    }
  }

  float reshaped[16] = {0.};
  float inverted[16] = {0.};
  int idx=0;
  for(int r=0; r < 4; r++){
    for(int c=0; c < 4; c++){
      reshaped[idx] = 0;
      inverted[idx] = 0;
      reshaped[idx++] = dot_prod[r][c];
    }
  }
  invertMatrixAcceleration(reshaped, inverted);
  idx=0;
  float inverted_reshaped[4][4];
  for(int r=0; r < 4; r++){
    for(int c=0; c < 4; c++){
      inverted_reshaped[r][c] = 0.;
      inverted_reshaped[r][c] = inverted[idx++];
    }
  }
  
  float dot_prod2[4][6] = {0.};
  for(int r = 0; r < 4; r++){
    for(int c = 0; c < 6; c++){
      dot_prod2[r][c] = 0.0;
      for(int k = 0; k < 4; k++){
        dot_prod2[r][c] += inverted_reshaped[r][k] * axlMeasuredValues_T[k][c];
      }
    }
  }

  // set acceleration calibration parameters
  float Y[6][3] = {
      {0.,0.,1.},
      {0.,0.,-1.},
      {0.,1.,0.},
      {0.,-1.,0.},
      {1.,0.,0.},
      {-1.,0.,0.},
  };
  for(int r = 0; r < 4; r++){
    for(int c = 0; c < 3; c++){
      _axlCalibrationParameters[r][c] = 0;
      for(int k = 0; k < 6; k++){
        _axlCalibrationParameters[r][c] += dot_prod2[r][k] * Y[k][c];
      }
    }
  }
  Serial.println("Accelerometer calibrated");
  printParams();
}

void LSM9DS1Class::invertMatrixAcceleration(const float m[16], float invOut[16]) {
  float inv[16], det;
  int i;

  inv[0] = m[5]  * m[10] * m[15] -
            m[5]  * m[11] * m[14] -
            m[9]  * m[6]  * m[15] +
            m[9]  * m[7]  * m[14] +
            m[13] * m[6]  * m[11] -
            m[13] * m[7]  * m[10];

  inv[4] = -m[4]  * m[10] * m[15] +
            m[4]  * m[11] * m[14] +
            m[8]  * m[6]  * m[15] -
            m[8]  * m[7]  * m[14] -
            m[12] * m[6]  * m[11] +
            m[12] * m[7]  * m[10];

  inv[8] = m[4]  * m[9] * m[15] -
            m[4]  * m[11] * m[13] -
            m[8]  * m[5] * m[15] +
            m[8]  * m[7] * m[13] +
            m[12] * m[5] * m[11] -
            m[12] * m[7] * m[9];

  inv[12] = -m[4]  * m[9] * m[14] +
              m[4]  * m[10] * m[13] +
              m[8]  * m[5] * m[14] -
              m[8]  * m[6] * m[13] -
              m[12] * m[5] * m[10] +
              m[12] * m[6] * m[9];

  inv[1] = -m[1]  * m[10] * m[15] +
            m[1]  * m[11] * m[14] +
            m[9]  * m[2] * m[15] -
            m[9]  * m[3] * m[14] -
            m[13] * m[2] * m[11] +
            m[13] * m[3] * m[10];

  inv[5] = m[0]  * m[10] * m[15] -
            m[0]  * m[11] * m[14] -
            m[8]  * m[2] * m[15] +
            m[8]  * m[3] * m[14] +
            m[12] * m[2] * m[11] -
            m[12] * m[3] * m[10];

  inv[9] = -m[0]  * m[9] * m[15] +
            m[0]  * m[11] * m[13] +
            m[8]  * m[1] * m[15] -
            m[8]  * m[3] * m[13] -
            m[12] * m[1] * m[11] +
            m[12] * m[3] * m[9];

  inv[13] = m[0]  * m[9] * m[14] -
            m[0]  * m[10] * m[13] -
            m[8]  * m[1] * m[14] +
            m[8]  * m[2] * m[13] +
            m[12] * m[1] * m[10] -
            m[12] * m[2] * m[9];

  inv[2] = m[1]  * m[6] * m[15] -
            m[1]  * m[7] * m[14] -
            m[5]  * m[2] * m[15] +
            m[5]  * m[3] * m[14] +
            m[13] * m[2] * m[7] -
            m[13] * m[3] * m[6];

  inv[6] = -m[0]  * m[6] * m[15] +
            m[0]  * m[7] * m[14] +
            m[4]  * m[2] * m[15] -
            m[4]  * m[3] * m[14] -
            m[12] * m[2] * m[7] +
            m[12] * m[3] * m[6];

  inv[10] = m[0]  * m[5] * m[15] -
            m[0]  * m[7] * m[13] -
            m[4]  * m[1] * m[15] +
            m[4]  * m[3] * m[13] +
            m[12] * m[1] * m[7] -
            m[12] * m[3] * m[5];

  inv[14] = -m[0]  * m[5] * m[14] +
              m[0]  * m[6] * m[13] +
              m[4]  * m[1] * m[14] -
              m[4]  * m[2] * m[13] -
              m[12] * m[1] * m[6] +
              m[12] * m[2] * m[5];

  inv[3] = -m[1] * m[6] * m[11] +
            m[1] * m[7] * m[10] +
            m[5] * m[2] * m[11] -
            m[5] * m[3] * m[10] -
            m[9] * m[2] * m[7] +
            m[9] * m[3] * m[6];

  inv[7] = m[0] * m[6] * m[11] -
            m[0] * m[7] * m[10] -
            m[4] * m[2] * m[11] +
            m[4] * m[3] * m[10] +
            m[8] * m[2] * m[7] -
            m[8] * m[3] * m[6];

  inv[11] = -m[0] * m[5] * m[11] +
              m[0] * m[7] * m[9] +
              m[4] * m[1] * m[11] -
              m[4] * m[3] * m[9] -
              m[8] * m[1] * m[7] +
              m[8] * m[3] * m[5];

  inv[15] = m[0] * m[5] * m[10] -
            m[0] * m[6] * m[9] -
            m[4] * m[1] * m[10] +
            m[4] * m[2] * m[9] +
            m[8] * m[1] * m[6] -
            m[8] * m[2] * m[5];

  det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

  det = 1.0 / det;
  for (i = 0; i < 16; i++)
      invOut[i] = inv[i] * det; 
}

void LSM9DS1Class::printParams() {
  for(int i = 0;i < 4; i++) {
    for(int j = 0;j < 3;j++) {
      Serial.print(_axlCalibrationParameters[i][j]);
      Serial.print("\t");
    }
    Serial.println("");
  }
}

String LSM9DS1Class::axlParam2str(int position, int digits) {
  return String(String(_axlCalibrationParameters[position][0],digits) + "|" + String(_axlCalibrationParameters[position][1],digits) + "|" + String(_axlCalibrationParameters[position][2],digits) + "\n");
    
}

String LSM9DS1Class::axlParams2str(int digits) {
  String str = "";
  for(int i = 0; i < 4; i++) {
    str += axlParam2str(i, digits);
  }
  return str;
}

String LSM9DS1Class::axlMeasuredValue(int position, int digits) {
  return String(String(_axlMeasuredValues[position][0],digits) + "|" + String(_axlMeasuredValues[position][1],digits) + "|" + String(_axlMeasuredValues[position][2],digits) + "\n");
}

String LSM9DS1Class::axlMeasuredValues(int digits) {
  String str = "";
  for(int i = 0; i < 6; i++) {
    str += axlMeasuredValue(i, digits);
  }
  return str;
}

int LSM9DS1Class::readGyroscope(float& x, float& y, float& z)
{
  int16_t data[3];

  if (!readRegisters(LSM9DS1_ADDRESS, LSM9DS1_OUT_X_G, (uint8_t*)data, sizeof(data))) {
    x = NAN;
    y = NAN;
    z = NAN;

    return 0;
  }

  x = data[0] * 2000.0 / 32768.0;
  y = data[1] * 2000.0 / 32768.0;
  z = data[2] * 2000.0 / 32768.0;

  return 1;
}

int LSM9DS1Class::gyroscopeAvailable()
{
  if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_STATUS_REG) & 0x02) {
    return 1;
  }

  return 0;
}

float LSM9DS1Class::gyroscopeSampleRate()
{
  return 119.0F;
}

int LSM9DS1Class::readMagneticField(float& x, float& y, float& z)
{
  int16_t data[3];

  if (!readRegisters(LSM9DS1_ADDRESS_M, LSM9DS1_OUT_X_L_M, (uint8_t*)data, sizeof(data))) {
    x = NAN;
    y = NAN;
    z = NAN;

    return 0;
  }

  x = data[0] * 4.0 * 100.0 / 32768.0;
  y = data[1] * 4.0 * 100.0 / 32768.0;
  z = data[2] * 4.0 * 100.0 / 32768.0;

  return 1;
}

int LSM9DS1Class::magneticFieldAvailable()
{
  if (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_STATUS_REG_M) & 0x08) {
    return 1;
  }

  return 0;
}

float LSM9DS1Class::magneticFieldSampleRate()
{
  return 20.0;
}

int LSM9DS1Class::readRegister(uint8_t slaveAddress, uint8_t address)
{
  _wire->beginTransmission(slaveAddress);
  _wire->write(address);
  if (_wire->endTransmission() != 0) {
    return -1;
  }

  if (_wire->requestFrom(slaveAddress, 1) != 1) {
    return -1;
  }

  return _wire->read();
}

int LSM9DS1Class::readRegisters(uint8_t slaveAddress, uint8_t address, uint8_t* data, size_t length)
{
  _wire->beginTransmission(slaveAddress);
  _wire->write(0x80 | address);
  if (_wire->endTransmission(false) != 0) {
    return -1;
  }

  if (_wire->requestFrom(slaveAddress, length) != length) {
    return 0;
  }

  for (size_t i = 0; i < length; i++) {
    *data++ = _wire->read();
  }

  return 1;
}

int LSM9DS1Class::writeRegister(uint8_t slaveAddress, uint8_t address, uint8_t value)
{
  _wire->beginTransmission(slaveAddress);
  _wire->write(address);
  _wire->write(value);
  if (_wire->endTransmission() != 0) {
    return 0;
  }

  return 1;
}

#ifdef ARDUINO_ARDUINO_NANO33BLE
LSM9DS1Class IMU(Wire1);
#else
LSM9DS1Class IMU(Wire);
#endif
