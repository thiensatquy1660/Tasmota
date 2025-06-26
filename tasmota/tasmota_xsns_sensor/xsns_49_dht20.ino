#include <Arduino.h>
#include "tasmota.h"
#ifdef USE_DHT20
#include <Wire.h>

// === Sensor identification and I2C address ===
#define XSNS_DHT20_IDX      49      // Unique index for this sensor slot
#define DHT20_I2C_ADDR      0x38    // 7-bit I2C address for DHT20/AHT20

// === Command definitions for DHT20 ===
// Initialization (soft reset + calibration)
#define DHT20_INIT_CMD      0xBE
#define DHT20_INIT_ARG1     0x08
#define DHT20_INIT_ARG2     0x00
// Measurement request
#define DHT20_MEAS_CMD      0xAC
#define DHT20_MEAS_ARG1     0x33
#define DHT20_MEAS_ARG2     0x00

// === Timing (from datasheet) ===
// After init command, allow up to 40 ms for reset/calibration
#define DHT20_INIT_DELAY_MS  40
// After measurement command, allow up to 75 ms for conversion
#define DHT20_MEAS_DELAY_MS  75

// === Register this sensor with Tasmota core ===
// Parameters: index, label, driver function pointer
static Sensor Dht20Sensor(XSNS_DHT20_IDX, PSTR("DHT20"), Dht20Driver);

// === Driver implementation ===
bool Dht20Driver(uint8_t Function) {
  switch (Function) {
    case FUNC_PRE_INIT:
      // Called early in setup(): initialize I2C bus
      Wire.begin();  // Uses SDA/SCL pins as configured globally
      break;

    case FUNC_INIT:
      // Perform soft reset and calibration sequence
      Wire.beginTransmission(DHT20_I2C_ADDR);
      Wire.write(DHT20_INIT_CMD);
      Wire.write(DHT20_INIT_ARG1);
      Wire.write(DHT20_INIT_ARG2);
      Wire.endTransmission();
      Delay(DHT20_INIT_DELAY_MS);
      break;

    case FUNC_READ:
    {
      // === Send measurement command ===
      Wire.beginTransmission(DHT20_I2C_ADDR);
      Wire.write(DHT20_MEAS_CMD);
      Wire.write(DHT20_MEAS_ARG1);
      Wire.write(DHT20_MEAS_ARG2);
      Wire.endTransmission();
      // Wait for device to complete measurement
      Delay(DHT20_MEAS_DELAY_MS);

      // === Read 6 bytes from sensor ===
      // Byte 0: status
      // Bytes 1-3: humidity data (20-bit: [buf1][buf2][upper 4 bits of buf3])
      // Bytes 3-5: temperature data (20-bit: [lower 4 bits of buf3][buf4][buf5])
      uint8_t buf[6];
      if (Wire.requestFrom(DHT20_I2C_ADDR, 6) != 6) {
        // I2C read error
        return false;
      }
      for (uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();

      // === Decode humidity ===
      uint32_t rawH = (uint32_t(buf[1]) << 12) |
                      (uint32_t(buf[2]) << 4) |
                      (buf[3] >> 4);
      // Convert to relative humidity (%)
      float humidity = rawH * 100.0f / 1048576.0f;

      // === Decode temperature ===
      uint32_t rawT = (uint32_t(buf[3] & 0x0F) << 16) |
                      (uint32_t(buf[4]) << 8) |
                      buf[5];
      // Convert to temperature in Celsius
      float temperature = rawT * 200.0f / 1048576.0f - 50.0f;

      // === Store values in Tasmota sensor array ===
      SensorValue(XSNS_DHT20_IDX, 0) = temperature;  // channel 0: °C
      SensorValue(XSNS_DHT20_IDX, 1) = humidity;     // channel 1: %RH
      break;
    }

    default:
      break;
  }
  return true;
}
#endif // USE_DHT20