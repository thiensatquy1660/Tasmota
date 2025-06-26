#ifdef USE_I2C
#ifdef USE_DHT20

#include <Wire.h>
#include "driver_i2c.h"       // I2C helper functions: I2cSetDevice, I2cEnabled, I2cSetActiveFound
#include "tasmota_helpers.h"  // Conversion and display helpers: ConvertTemp, ConvertHumidity, TempHumDewShow, AddLogMissed

#define XSNS_49        49
#define XI2C_42        42
#define DHT20_ADDR     0x38
#define DHT20_TIMEOUT  1000  // milliseconds

struct DHT20_t {
  float   temperature = NAN;
  float   humidity    = NAN;
  uint8_t valid       = 0;
  uint8_t count       = 0;
  char    name[6]     = "DHT20";
} Dht20;

// CRC8 calculation (polynomial 0x31)
static uint8_t DHT20_crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0xFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
    }
  }
  return crc;
}

bool Dht20Read(void) {
  // Age-out previous valid flag
  if (Dht20.valid) {
    Dht20.valid--;
  }

  // 1) Trigger single-shot measurement
  Wire.beginTransmission(DHT20_ADDR);
  Wire.write(0xAC);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  // 2) Poll status until ready or timeout
  uint32_t start = millis();
  while (millis() - start < DHT20_TIMEOUT) {
    Wire.requestFrom(DHT20_ADDR, (uint8_t)1);
    if (Wire.available()) {
      uint8_t status = Wire.read();
      if ((status & 0x80) == 0) break;  // Bit7=0 => ready
    }
    delay(5);
  }

  // 3) Read full frame (status + 6 data bytes + CRC)
  Wire.requestFrom(DHT20_ADDR, (uint8_t)7);
  if (Wire.available() < 7) {
    return false;
  }
  uint8_t raw[7];
  for (uint8_t i = 0; i < 7; i++) {
    raw[i] = Wire.read();
  }

  // 4) CRC check on first 6 bytes
  if (DHT20_crc8(raw, 6) != raw[6]) {
    return false;
  }

  // 5) Unpack 20-bit humidity and temperature
  uint32_t raw_rh = ((uint32_t)raw[1] << 12) |
                    ((uint32_t)raw[2] << 4)  |
                    ((raw[3] & 0xF0) >> 4);
  uint32_t raw_t  = ((uint32_t)(raw[3] & 0x0F) << 16) |
                    ((uint32_t)raw[4] << 8)        |
                    (raw[5]);

  float rh = (raw_rh * 100.0f) / 1048576.0f;
  float tc = (raw_t  * 200.0f) / 1048576.0f - 50.0f;

  // 6) Apply user conversions (°C/°F, offsets)
  Dht20.humidity    = ConvertHumidity(rh);
  Dht20.temperature = ConvertTemp(tc);

  if (isnan(Dht20.humidity) || isnan(Dht20.temperature)) {
    return false;
  }

  Dht20.valid = SENSOR_MAX_MISS;
  return true;
}

void Dht20Detect(void) {
  Wire.begin();  // initialize I2C on default pins
  if (!I2cSetDevice(DHT20_ADDR)) return;
  if (Dht20Read()) {
    I2cSetActiveFound(DHT20_ADDR, Dht20.name);
    Dht20.count = 1;
  }
}

void Dht20EverySecond(void) {
  if (TasmotaGlobal.uptime & 1) {  // every 2 seconds
    if (!Dht20Read()) {
      AddLogMissed(Dht20.name, Dht20.valid);
    }
  }
}

void Dht20Show(bool json) {
  if (Dht20.valid) {
    TempHumDewShow(json,
                   (0 == TasmotaGlobal.tele_period),
                   Dht20.name,
                   Dht20.temperature,
                   Dht20.humidity);
  }
}

bool Xsns49(uint32_t function) {
  if (!I2cEnabled(XI2C_42)) {
    return false;
  }
  if (function == FUNC_INIT) {
    Dht20Detect();
  } else if (Dht20.count) {
    switch (function) {
      case FUNC_EVERY_SECOND: Dht20EverySecond(); break;
      case FUNC_JSON_APPEND:  Dht20Show(true);     break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:   Dht20Show(false);    break;
#endif
    }
  }
  return true;
}

#endif  // USE_DHT20
#endif  // USE_I2C
