/*
  xsns_49_dht20.ino – DHT20 (AHT20 compatible) I²C temperature & humidity sensor
  Copyright 2025

  Datasheet: addr 0x38, single-shot measure cmd 0xAC 33 00
  rawRH 20 bit →  RH (%)  = rawRH * 100 / 2²⁰
  rawT  20 bit →  T (°C)  = rawT  * 200 / 2²⁰ – 50     :contentReference[oaicite:0]{index=0}
*/

#ifdef USE_I2C
#ifdef USE_DHT20          // <-- enable in user_config_override.h

#define XSNS_49       49
#define XI2C_42       42            // next free I²C device id
#define DHT20_ADDR    0x38

struct DHT20_t {
  float   temperature = NAN;
  float   humidity    = NAN;
  uint8_t valid       = 0;
  uint8_t count       = 0;
  char    name[6]     = "DHT20";
} Dht20;

/* ---------------- low-level read ---------------- */
bool Dht20Read(void) {

  if (Dht20.valid) {                 // ageing flag
    Dht20.valid--;
  }

  // 1) trigger single measurement
  Wire.beginTransmission(DHT20_ADDR);
  Wire.write(0xAC); Wire.write(0x33); Wire.write(0x00);
  if (Wire.endTransmission() != 0) { return false; }

  delay(85);                         // datasheet: ≥80 ms for conversion

  // 2) read 7-byte status + data frame
  Wire.requestFrom(DHT20_ADDR, 7);
  if (Wire.available() != 7) { return false; }

  uint8_t status = Wire.read();
  if (status & 0x80) { return false; }   // busy bit

  uint8_t d[6];
  for (uint8_t i = 0; i < 6; i++) { d[i] = Wire.read(); }

  /* 20-bit unpacking — see datasheet */
  uint32_t raw_rh = ((uint32_t)d[0] << 12) | ((uint32_t)d[1] << 4) | (d[2] >> 4);
  uint32_t raw_t  = ((uint32_t)(d[2] & 0x0F) << 16) | ((uint32_t)d[3] << 8) | d[4];

  float rh = (raw_rh * 100.0f) / 1048576.0f;           // 2²⁰ = 1 048 576
  float tc = (raw_t  * 200.0f) / 1048576.0f - 50.0f;

  /* convert using Tasmota helpers so °C/°F & offset work everywhere */
  Dht20.humidity    = ConvertHumidity(rh);
  Dht20.temperature = ConvertTemp(tc);

  if (isnan(Dht20.temperature) || isnan(Dht20.humidity)) { return false; }

  Dht20.valid = SENSOR_MAX_MISS;      // good read – reset miss counter
  return true;
}

/* -------------- lifecycle hooks --------------- */
void Dht20Detect(void) {
  if (!I2cSetDevice(DHT20_ADDR)) { return; }
  if (Dht20Read()) {
    I2cSetActiveFound(DHT20_ADDR, Dht20.name);
    Dht20.count = 1;
  }
}

void Dht20EverySecond(void) {
  if (TasmotaGlobal.uptime & 1) {         // every 2 s (same cadence as DHT12)
    if (!Dht20Read()) { AddLogMissed(Dht20.name, Dht20.valid); }
  }
}

void Dht20Show(bool json) {
  if (Dht20.valid) {
    TempHumDewShow(json, (0 == TasmotaGlobal.tele_period),
                   Dht20.name, Dht20.temperature, Dht20.humidity);
  }
}

/* -------------- plugin entry point ------------- */
bool Xsns49(uint32_t function) {

  if (!I2cEnabled(XI2C_42)) { return false; }
  bool result = false;

  if (FUNC_INIT == function) {
    Dht20Detect();
  }
  else if (Dht20.count) {
    switch (function) {
      case FUNC_EVERY_SECOND: Dht20EverySecond(); break;
      case FUNC_JSON_APPEND:  Dht20Show(true);     break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_SENSOR:   Dht20Show(false);    break;
#endif
    }
  }
  return result;
}

#endif   // USE_DHT20
#endif   // USE_I2C
