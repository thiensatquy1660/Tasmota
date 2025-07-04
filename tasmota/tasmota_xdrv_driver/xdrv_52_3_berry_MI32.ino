/*
  xdrv_52_3_berry_MI32.ino - Berry scripting language, native functions

  Copyright (C) 2021 Christian Baars & Stephan Hadinger, Berry language by Guan Wenliang https://github.com/Skiars/berry

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifdef USE_BERRY

#include <berry.h>

#if defined(USE_MI_ESP32) && !defined(USE_BLE_ESP32)

#include "include/xsns_62_esp32_mi.h"

/*********************************************************************************************\
 * Native functions mapped to Berry functions
 *
 *
\*********************************************************************************************/
extern "C" {

/********************************************************************
**  MI32 - sensor specific functions
********************************************************************/

  extern uint32_t MI32numberOfDevices();
  extern char * MI32getDeviceName(uint32_t slot);
  extern void MI32setBatteryForSlot(uint32_t slot, uint8_t value);
  extern void MI32setHumidityForSlot(uint32_t slot, float value);
  extern void MI32setTemperatureForSlot(uint32_t slot, float value);
  extern uint8_t * MI32getDeviceMAC(uint32_t slot);

  struct {
    const char * data = nullptr;
    size_t size = 0;
    void* callback = nullptr;
  } be_MI32Widget;

  int be_MI32_devices(void) {
    return MI32numberOfDevices();
  }

  void be_MI32_set_bat(int slot, int bat_val){
    MI32setBatteryForSlot(slot,bat_val);
  }

  const char* be_MI32_get_name(int slot){
    return  MI32getDeviceName(slot);
  }

  uint8_t *be_MI32_get_MAC(int32_t slot, size_t *size){
    *size = 6;
    uint8_t * buffer = MI32getDeviceMAC(slot);
    if(buffer == nullptr){
      *size = 0;
    }
    return buffer;
  }

  void be_MI32_set_hum(int slot, int hum_val){
    MI32setHumidityForSlot(slot,hum_val);
  }

  void be_MI32_set_temp(int slot, int temp_val){
    MI32setTemperatureForSlot(slot,temp_val);
  }

  bool be_MI32_widget(const char* sbuf, void* function){
    if (function){
      be_MI32Widget.callback = function;
    }
    if(be_MI32Widget.size == 0){
      be_MI32Widget.data = sbuf;
      be_MI32Widget.size = strlen(sbuf);
      return true;
    }
    return false;
  }


/********************************************************************
**  BLE - generic BLE functions
********************************************************************/
  extern bool MI32checkBLEinitialization();
  extern void MI32BerryLoop();
  extern void MI32setBerryAdvCB(void* function, uint8_t *buffer);
  extern void MI32setBerryConnCB(void* function, uint8_t *buffer);
  extern void MI32setBerryServerCB(void* function, uint8_t *buffer);
  extern bool MI32runBerryConnection(uint8_t operation, bbool response, int32_t *arg1);
  extern bool MI32setBerryCtxSvc(const char *Svc, bbool discoverAttributes);
  extern bool MI32setBerryCtxChr(const char *Chr);
  extern bool MI32setBerryCtxMAC(uint8_t *MAC, uint8_t type);
  extern bool MI32addMACtoWatchList(uint8_t *MAC, uint8_t type);
  extern void MI32setBerryStoreRec(uint8_t *buffer, size_t size);

  int be_BLE_init(bvm *vm);
  int be_BLE_init(bvm *vm) {
    if (MI32checkBLEinitialization() == true){
      be_return(vm);
    }
    be_raise(vm, "ble_error", "BLE: device not initialized");
    be_return_nil(vm);
  }

  void be_BLE_loop(void){
    MI32BerryLoop();
  }

  void be_BLE_store(uint8_t *buf, size_t size){    
    MI32setBerryStoreRec(buf, size);
  }

  void be_BLE_reg_conn_cb(void* function, uint8_t *buffer);
  void be_BLE_reg_conn_cb(void* function, uint8_t *buffer){
    MI32setBerryConnCB(function,buffer);
  }

  void be_BLE_reg_server_cb(void* function, uint8_t *buffer);
  void be_BLE_reg_server_cb(void* function, uint8_t *buffer){
    MI32setBerryServerCB(function,buffer);
  }

  void be_BLE_reg_adv_cb(void* function, uint8_t *buffer);
  void be_BLE_reg_adv_cb(void* function, uint8_t *buffer){
    if(function == 0){
      MI32setBerryAdvCB(NULL,NULL);
    }
    else if(buffer){
      MI32setBerryAdvCB(function,buffer);
    }
  }

  bool be_BLE_MAC_size(struct bvm *vm, size_t size){
    if(size != 6){
       be_raisef(vm, "ble_error", "BLE: wrong size of MAC");
       return false;
    }
    return true;
  }

  void be_BLE_set_MAC(struct bvm *vm, uint8_t *buf, size_t size, uint8_t type);
  void be_BLE_set_MAC(struct bvm *vm, uint8_t *buf, size_t size, uint8_t type){
    if(!be_BLE_MAC_size(vm, size)){
      return;
    }
    uint8_t _type = 0;
    if(type){
      _type = type;
    }
    if (MI32setBerryCtxMAC(buf,_type)) return;

    be_raisef(vm, "ble_error", "BLE: could not set MAC");
  }

  void be_BLE_set_service(struct bvm *vm, const char *Svc, bbool discoverAttributes);
  void be_BLE_set_service(struct bvm *vm, const char *Svc, bbool discoverAttributes){
    bool _discoverAttributes = false;
    if(discoverAttributes){
      _discoverAttributes = discoverAttributes ;
    }
    if (MI32setBerryCtxSvc(Svc,_discoverAttributes)) return;

    be_raisef(vm, "ble_error", "BLE: could not set service");
  }

  void be_BLE_set_characteristic(struct bvm *vm, const char *Chr);
  void be_BLE_set_characteristic(struct bvm *vm, const char *Chr){

    if (MI32setBerryCtxChr(Chr)) return;

    be_raisef(vm, "ble_error", "BLE: could not set characteristic");
  }

  void be_BLE_run(struct bvm *vm, uint8_t operation, bbool response, int32_t arg1);
  void be_BLE_run(struct bvm *vm, uint8_t operation, bbool response, int32_t arg1){
    int32_t argc = be_top(vm); // Get the number of arguments
    bool _response = false;
    if(response){
      _response = response;
    }
    int32_t *ptr_arg1 = nullptr;
    int32_t _arg1 = arg1;
    if(argc == 3){
      ptr_arg1 = &_arg1;
    }
    if (MI32runBerryConnection(operation, _response, ptr_arg1)) return;

    be_raisef(vm, "ble_error", "BLE: could not run operation");
  }

  void be_BLE_adv_watch(struct bvm *vm, uint8_t *buf, size_t size, uint8_t type);
  void be_BLE_adv_watch(struct bvm *vm, uint8_t *buf, size_t size, uint8_t type){
    if(!be_BLE_MAC_size(vm, size)){
      return;
    }
    uint8_t _type = 0;
    if(type){
      _type = type;
    }
    if(MI32addMACtoWatchList(buf, _type)) return;

  be_raisef(vm, "ble_error", "BLE: could not add MAC to watch list");
  }

  // helper function
  NimBLEConnInfo be_BLE_get_ConnInfo(NimBLEClient *device);
  NimBLEConnInfo be_BLE_get_ConnInfo(NimBLEClient *device){
    if(!device){
      return NimBLEDevice::getServer()->getPeerInfo(0);
    } else {
      return device->getConnInfo();
    }
  }

  // from esp-nimble/ble_sm.c
  int ble_sm_read_bond(uint16_t conn_handle, struct ble_store_value_sec *out_bond)
  {
      struct ble_store_key_sec key_sec;
      struct ble_gap_conn_desc desc;
      int rc;

      rc = ble_gap_conn_find(conn_handle, &desc);
      if (rc != 0) {
          return rc;
      }

      memset(&key_sec, 0, sizeof key_sec);
      key_sec.peer_addr = desc.peer_id_addr;

      rc = ble_store_read_peer_sec(&key_sec, out_bond);
      return rc;
  }

  // BLE.info(void) -> map
  int32_t be_BLE_info(struct bvm *vm);
  int32_t be_BLE_info(struct bvm *vm) {
    be_newobject(vm, "map");
    char _Role[16];
    GetTextIndexed(_Role, sizeof(_Role), MI32.role, HTTP_MI32_PARENT_BLE_ROLE);
    char _role[16];
    LowerCase(_role,_Role);
    be_map_insert_str(vm, "role", _role);
    be_map_insert_str(vm, "local_addr", NimBLEDevice::toString().c_str());
    be_map_insert_int(vm, "power", NimBLEDevice::getPower());
    be_map_insert_int(vm, "MTU", NimBLEDevice::getMTU());
#ifdef CONFIG_BT_NIMBLE_EXT_ADV // TODO - BLE 5 is very unsharp
    be_map_insert_int(vm, "version", 5);
#else
    be_map_insert_int(vm, "version", 4);
#endif
// #ifdef CONFIG_BT_NIMBLE_PERSIST
    be_map_insert_int(vm, "bonds", NimBLEDevice::getNumBonds());
// #else
//     be_map_insert_nil(vm, "bonds");
// #endif
    if(MI32.mode.connected == 1 || MI32.ServerTask != nullptr){
      NimBLEClient* _device = nullptr;
      if(MI32.mode.connected == 1){
        _device = NimBLEDevice::getClientByHandle(MI32.connID);
      }
      NimBLEConnInfo _info = be_BLE_get_ConnInfo(_device);

      be_pushstring(vm, "connection");
      be_newobject(vm, "map");

      be_map_insert_str(vm, "peer_addr", _info.getAddress().toString().c_str());
      be_map_insert_str(vm, "peerID_addr", _info.getIdAddress().toString().c_str());
      if(_device != nullptr) be_map_insert_int(vm, "RSSI", _device->getRssi()); // ESP32 is client
      be_map_insert_int(vm, "MTU", _info.getMTU());
      be_map_insert_bool(vm, "bonded", _info.isBonded());
      be_map_insert_bool(vm, "master", _info.isMaster());
      be_map_insert_bool(vm, "encrypted", _info.isEncrypted());
      be_map_insert_bool(vm, "authenticated", _info.isAuthenticated());
      if(_device == nullptr) {
        auto _remote_client = NimBLEDevice::getServer()->getClient(_info);
        if(_remote_client != nullptr){
          auto _name = _remote_client->getValue(NimBLEUUID((uint16_t)0x1800), NimBLEUUID((uint16_t)0x2A00)); //GAP, name
          if(_name){
            be_map_insert_str(vm, "name", _name.c_str()); // ESP32 is server
          }
        } else {
          be_map_insert_str(vm, "name", "");
        }
      }

      ble_store_value_sec value_sec;
      ble_sm_read_bond(_info.getConnHandle(), &value_sec);
      if(value_sec.irk_present == 1){
            char IRK[33];
            ToHex_P(value_sec.irk,16,IRK,33);
            be_map_insert_str(vm, "IRK",IRK );
      }

      be_pop(vm, 1);
      be_data_insert(vm, -3);
      be_pop(vm, 2);

    }

    be_pop(vm, 1);
    be_return(vm);
  }
} //extern "C"


#endif // USE_MI_ESP32

#endif  // USE_BERRY

/*
BLE.set_svc
BLE.set_chr

BLE.set_MAC
BLE.run(op, optional: bool response)

be_BLE_op:
# client
1 read
2 write
3 subscribe
4 unsubscribe - maybe later
5 disconnect
6 discover services
7 discover characteristics


11 read once, then disconnect
12 write once, then disconnect
13 subscribe once, then disconnect
14 unsubscribe once, then disconnect - maybe later

#server
__commands
201 add/set advertisement
202 add/set scan response

211 add/set characteristic

__response
221 onRead
222 onWrite
223 unsubscribed
224 subscribed to notifications
225 subscribed to indications
226 subscribed to notifications and indications
227 onConnect
228 onDisconnect
229 onStatus
230 onAuthenticated


BLE.conn_cb(cb,buffer)
BLE.adv_cb(cb,buffer)
BLE.serv_cb(cb,buffer)
BLE.adv_watch(MAC)

MI32.devices()
MI32.get_name(slot)
MI32.get_MAC(slot)
MI32.set_bat(slot,int)
MI32.set_hum(slot,float)
MI32.set_temp(slot,float)
MI32.widget(string[,cb])

*/