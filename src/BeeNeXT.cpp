#include "BeeNeXT.h"

// #if defined(BEENEXT) || BLYNKGO_USE_BEENEXT

void BeeNeXT_NoOpCbk() {}
BEENEXT_DATA()          __attribute__((weak, alias("BeeNeXT_NoOpCbk")));
SERIAL_DATA()           __attribute__((weak, alias("BeeNeXT_NoOpCbk")));
BEENEXT_CONNECTED()     __attribute__((weak, alias("BeeNeXT_NoOpCbk")));
BEENEXT_DISCONNECTED()  __attribute__((weak, alias("BeeNeXT_NoOpCbk")));

BeeNeXT_class BeeNeXT;

BeeNeXT_class::~BeeNeXT_class(){
  this->end();
}

void BeeNeXT_class::begin(HardwareSerial *serial){
  this->end();    // serial ของเดิม ไม่ว่าจะ hard serial หรือ soft serial ที่ใช้อยู่เดิม ให้ ยกเลิกไปก่อน

#if CONFIG_IDF_TARGET_ESP32S3
#if defined(BEENEXT_4_3) ||  defined(BEENEXT_4_3C) ||  defined(BEENEXT_4_3IPS)
  pinMode(18,OUTPUT);
  digitalWrite(18, LOW); // ทำขา 18 เป็น GND
#endif
  Serial2.begin(9600);  // RX19 ; TX20
  _hw_serial = &Serial2;
#else
  _hw_serial = (serial == NULL)? &Serial : serial;  // ไม่มีการ serial begin() มาก่อนเอาเอง
#endif
  _hw_serial->setTimeout(50);

  this->set_heartbeat(1000);
  this->set_heartbeat_checker();

  static SoftTimer timer;
  timer.delay(1000,[](){ if(!BeeNeXT.connected()) BeeNeXT_onDisconnected(); });  
}

void BeeNeXT_class::begin(HardwareSerial &serial ){
  this->begin(&serial);
}

#if BEENEXT_USE_SOFTWARESERIAL && (CONFIG_IDF_TARGET_ESP32S3==0)
void BeeNeXT_class::begin(unsigned long baud, uint8_t rx, uint8_t tx){
  this->end();
  if(_sw_serial==NULL)
    _sw_serial =  new SoftwareSerial();
  if(_sw_serial != NULL) {
    _is_swserial_alloced = true;
    _sw_serial->begin(baud, rx, tx);
    _sw_serial->setTimeout(50);
  }

  this->set_heartbeat(1000);
  this->set_heartbeat_checker();

  static SoftTimer timer;
  timer.delay(1000,[](){ if(!BeeNeXT.connected()) BeeNeXT_onDisconnected(); });  
}

void BeeNeXT_class::begin(uint8_t rx, uint8_t tx){
  this->begin(9600, rx,tx);
}

void BeeNeXT_class::begin(SoftwareSerial *softserial) {
  this->end();
  _sw_serial = softserial;
  _sw_serial->setTimeout(50);
}

void BeeNeXT_class::begin(SoftwareSerial &softserial){
  this->begin(&softserial);
}

#endif //#if BEENEXT_USE_SOFTWARESERIAL

// #if defined(CONFIG_IDF_TARGET_ESP32S3)
// void BeeNeXT_class::begin(unsigned long baud, uint8_t rx, uint8_t tx){
//   Serial.println("[BeeNeXT] use Serial2");
//   Serial2.begin(baud, SERIAL_8N1, rx, tx);
//   this->begin(&Serial2);
// }

// void BeeNeXT_class::begin(uint8_t rx, uint8_t tx){
//   this->begin(115200, rx, tx);
// }
// #endif // #if defined(ESP32)



void BeeNeXT_class::end(){
  if( _hw_serial != NULL) { 
    _hw_serial->end(); 
    _hw_serial = NULL;
  }
#if BEENEXT_USE_SOFTWARESERIAL && (CONFIG_IDF_TARGET_ESP32S3==0)
  if( _sw_serial != NULL ){
    _sw_serial->end();
    if(_is_swserial_alloced){
      free(_sw_serial); _sw_serial = NULL;
      _is_swserial_alloced = false;
    }
  }
#endif
}

void  BeeNeXT_class::extract_key_value(){
  int index = _data.indexOf(":");
  if( index > 0) {
    _key   = _data.substring(0, index);
    _value = _data.substring(index+1);
  }else{
    _key   = "";
    _value = "";
  }
}

void BeeNeXT_class::set_heartbeat(uint32_t heartbeat_interval){
  _timer_heartbeat.setInterval(heartbeat_interval,[](){
    BeeNeXT.send("_bhb_", true);
  }, true);
}

void BeeNeXT_class::set_heartbeat_checker(){
  _timer_heartbeat_checker.setInterval(BEENEXT_CONNECTION_TIMEOUT,[](){
    if(millis()> BeeNeXT._millis_heartbeat + BEENEXT_CONNECTION_TIMEOUT){
      if(BeeNeXT._bee_connected) {
        BeeNeXT._bee_connected = false;
        BeeNeXT_onDisconnected();
        BeeNeXT.set_heartbeat(1000);
      }
    }
  }, true);
}

void BeeNeXT_class::update(){
  if(_hw_serial != NULL) {
    if(_hw_serial->available()){
      _hw_serial->flush();
      String data = _hw_serial->readStringUntil('\n'); _data.trim();
      // Serial.print("[1] ");
      // Serial.println(data);
      if(data.startsWith("[BN]")){
        data.replace("[BN]", "");
        _data = data;
        this->extract_key_value();
        if( _key == "_bhb_"){
          _millis_heartbeat = millis();
          if( (bool) _value.toInt() != _bee_connected ) {
            _bee_connected = _value.toInt();
            if(_bee_connected ) {
              BeeNeXT_onConnected();
              this->set_heartbeat(BEENEXT_CONNECTION_TIMEOUT);
            }else {
              BeeNeXT_onDisconnected();
              this->set_heartbeat(1000);
            }
          }
        }else{
          BeeNeXT_onData();
        }
        _data  = "";
        _key   = "";
        _value = "";
      }else{
        _data = data;
        BeeNeXT_onSerialData();
        _data  = "";
        _key   = "";
        _value = "";
      }
    }
  }
#if BEENEXT_USE_SOFTWARESERIAL && (CONFIG_IDF_TARGET_ESP32S3==0)
  else if(_sw_serial !=NULL ){
    if(_sw_serial->available()){
      _sw_serial->flush();
      String data = _sw_serial->readStringUntil('\n'); _data.trim();
      // Serial.print("[2] ");
      // Serial.println(data);
      if(data.startsWith("[BN]")){
        data.replace("[BN]", "");
        _data = data;
        this->extract_key_value();
        if( _key == "_bhb_"){
          _millis_heartbeat = millis();
          if( (bool) _value.toInt() != _bee_connected ) {
            _bee_connected = _value.toInt();
            if(_bee_connected ) {
              BeeNeXT_onConnected();
              this->set_heartbeat(BEENEXT_CONNECTION_TIMEOUT);
            }else {
              BeeNeXT_onDisconnected();
              this->set_heartbeat(1000);
            }
          }
        }else{
          BeeNeXT_onData();
        }

        _data  = "";
        _key   = "";
        _value = "";
      }else{
        _data = data;
        BeeNeXT_onSerialData();
        _data  = "";
        _key   = "";
        _value = "";
      }
    }
  }
#endif

#if BEENEXT_USE_SOFTTIMER
  SoftTimer::run();
#endif

}

void BeeNeXT_class::command(uint16_t cmd) { 
  if(cmd>=0 && cmd < CMD_MAX) {
    String _cmd = String("cmd:")+String(cmd);
    Serial.println(_cmd);
    this->println(_cmd); 
  }
}

void BeeNeXT_class::event_send(beenect_event_t event){
  switch(event){
    case EVENT_BEENEXT_CONNECTED:
      BeeNeXT_onConnected();
      break;
    case EVENT_BEENEXT_DISCONNECTED:
      BeeNeXT_onDisconnected();
      break;
    case EVENT_BEENEXT_DATA:
      BeeNeXT_onData();
      break;
    case EVENT_SERIAL_DATA:
      BeeNeXT_onSerialData();
      break;
  }
}
//------------------------------------------------------------
//virtual function
size_t BeeNeXT_class::write(uint8_t data){
  if(_hw_serial){
    _hw_serial->write('[');_hw_serial->write('B');_hw_serial->write('N');_hw_serial->write(']');
    size_t sz = _hw_serial->write(data);
    return sz+4;
  }
#if BEENEXT_USE_SOFTWARESERIAL && (CONFIG_IDF_TARGET_ESP32S3==0)
  else if( _sw_serial){
    _sw_serial->write('[');_sw_serial->write('B');_sw_serial->write('N');_sw_serial->write(']');
    size_t sz = _sw_serial->write(data);
    return sz+4;
  }
#endif
  return 0;
}

size_t BeeNeXT_class::write(const uint8_t *buffer, size_t size){
  if(_hw_serial){
    _hw_serial->write('[');_hw_serial->write('B');_hw_serial->write('N');_hw_serial->write(']');
    size_t sz = _hw_serial->write(buffer, size);
    return sz + 4;
  }
#if BEENEXT_USE_SOFTWARESERIAL && (CONFIG_IDF_TARGET_ESP32S3==0)
  else if(_sw_serial){
    // Serial.printf("[BeeNeXT] SoftSerial printf : [BN]%.*s", size, buffer);
    _sw_serial->write('[');_sw_serial->write('B');_sw_serial->write('N');_sw_serial->write(']');
    size_t sz = _sw_serial->write(buffer, size);
    return sz + 4;
  }
#endif
  return 0;
}


// #endif //#if defined(BEENEXT) || BLYNKGO_USE_BEENEXT
