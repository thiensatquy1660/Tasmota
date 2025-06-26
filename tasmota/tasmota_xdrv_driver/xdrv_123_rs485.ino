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

// Default configurable TX/RX pins (can be overridden at compile-time)
#ifndef RS485_TX_PIN
  #define RS485_TX_PIN    1      // Default TX = GPIO1 (D1)
#endif

#ifndef RS485_RX_PIN
  #define RS485_RX_PIN    3      // Default RX = GPIO3 (D3)
#endif

/*********************************************************************************************
 *  Local helpers
 *********************************************************************************************/

// Forward declarations (implemented inside xsns_83_neopool.ino)
extern void NeoPoolInit(uint8_t tx_pin, uint8_t rx_pin);   // sensor initialisation
extern void NeoPoolPoll(void);                 // called every 250 ms to drive state-machine
extern bool NeopoolMqttShow(bool json);        // JSON export
extern uint32_t CmndNeopoolExec(void);         // console command dispatcher

/*********************************************************************************************
 *  Driver implementation (Tasmota hook table style)
 *  Each case is executed by TasmotaCore() in tasmota.ino
 *********************************************************************************************/
bool Xdrv123(uint8_t function) {
  switch (function) {
    case FUNC_INIT: {                    // once at boot
      NeoPoolInit(RS485_TX_PIN, RS485_RX_PIN);  // Init with defined TX/RX pins
      break;
    }

    case FUNC_EVERY_250MS: {            // time-slice scheduler (4 Hz)
      NeoPoolPoll();                    // non-blocking Modbus handling
      break;
    }

    case FUNC_COMMAND: {                // triggered from console / MQTT cmnd
      return CmndNeopoolExec();         // "NP <args>" family lives in sensor file
    }

    case FUNC_JSON_APPEND: {            // when Tasmota builds tele/STATE JSON
      return NeopoolMqttShow(true);     // append {"NeoPool":{…}}
    }

    default:
      break;
  }
  return false;                          // allow other drivers to run
}

#endif  // USE_NEOPOOL
#endif  // USE_RS485_DRV
