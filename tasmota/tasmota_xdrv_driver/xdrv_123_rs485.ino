/*
  xdrv_123_rs485.ino - Generic RS485 driver wrapper that bridges Tasmota core with
  Sugar Valley NeoPool sensor module (xsns_83_neopool.ino)

  -----------------------------------------------------------------------------
  Purpose
  -------
  * Initialise the HW UART in RS-485 mode and reuse all existing NeoPool helper
    functions (`NeoPoolInit`, `NeoPoolPoll`, MQTT/JSON exporters, …).
  * Expose a single Tasmota console command family **NP** (or **NEOPOOL**) so
    users can read / write registers without recompiling the sensor file.
  * Keep the file independent: if `USE_NEOPOOL` is disabled at compile-time this
    driver simply won’t be linked in (all calls are placed behind `#ifdef`).

  Usage example (console):
    NP READ 1002        → đọc Input Register #1002
    NP WRITE 40010 123  → ghi Holding Register #40010 giá trị 123

  Compile-time flags
  ------------------
    -D USE_RS485_DRV      Activate this driver
    -D USE_NEOPOOL        Ensure xsns_83_neopool.ino is compiled in

  NOTE:  The driver number **123** is arbitrary but must not clash with another
         xdrv already present in your fork of Tasmota.
*/

#ifdef USE_RS485_DRV      // <- set in user_config_override.h hoặc platformio.ini
#ifdef USE_NEOPOOL        // requires the sensor module

#define XDRV_123               123            // unique driver ID

/*********************************************************************************************
 *  Local helpers
 *  ----------------
 *  Use the core Tasmota declarations for Pin(), AddLog(), LOG_LEVEL_*, PSTR(), etc.
 *********************************************************************************************/

// Forward declarations (implemented inside xsns_83_neopool.ino) (implemented inside xsns_83_neopool.ino)
extern void NeoPoolInit(void);                 // sensor initialisation
extern void NeoPoolPoll(void);                 // called every 250 ms to drive state-machine
extern void NeopoolMqttShow(bool json);        // JSON export
extern void CmndNeopoolExec(void);             // console command dispatcher

/*********************************************************************************************
 *  Driver implementation (Tasmota hook table style)
 *  Each case is executed by TasmotaCore() in tasmota.ino
 *********************************************************************************************/
bool Xdrv123(uint32_t function) {
  switch (function) {
    case FUNC_INIT: {                    // once at boot
      // Require user-defined UART pins via Configure Module
      int tx_pin = Pin(GPIO_SBR_TX);
      int rx_pin = Pin(GPIO_SBR_RX);

      if (tx_pin >= 0 && rx_pin >= 0) {
        Serial.begin(19200, SERIAL_8N1, rx_pin, tx_pin);  // Init UART with selected pins
        NeoPoolInit();                                   // Init NeoPool state machine (uses Serial already)
      } else {
        AddLog(LOG_LEVEL_ERROR, PSTR("RS485 driver requires Serial Tx and Serial Rx GPIOs to be set!"));
      }
      break;
    }

    case FUNC_LOOP: {                   // main loop alternative to FUNC_EVERY_250MS
      NeoPoolPoll();                    // non-blocking Modbus handling
      break;
    }

    case FUNC_COMMAND: {                // triggered from console / MQTT cmnd
      CmndNeopoolExec();                // "NP <args>" family lives in sensor file
      return false;
    }

    case FUNC_JSON_APPEND: {            // when Tasmota builds tele/STATE JSON
      NeopoolMqttShow(true);            // append {"NeoPool":{…}}
      return false;
    }

    default:
      break;
  }
  return false;                          // allow other drivers to run
}

#endif  // USE_NEOPOOL
#endif  // USE_RS485_DRV
