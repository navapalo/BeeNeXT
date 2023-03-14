// จุดรับข้อมูลที่ส่งเข้ามาจาก MCU อื่น
BEENEXT_DATA(){
  String key = BeeNeXT.key();
  if( key == "LED") {                 // เช็คว่า key ที่ส่งมาตรงกับ "LED" หรือไม่
    bool value = BeeNeXT.toBool();    // เปลี่ยนให้ value ที่ส่งมา ไปเป็นข้อมูลแบบ boolean  ; อาจจะเปลี่ยนไปค่าแบบต่างๆได้เช่น BeeNeXT.toString(), BeeNeXT.toInt(), BeeNeXT.toFloat(), BeeNeXT.toBool()  ได้ตามต้องการ
    digitalWrite(LED_PIN, value); 
  }else
  if( key == "TEMP" ) {
    float temp = BeeNeXT.toFloat();
    Serial.printf("[TEMP] %.2f\n", temp);
  }else
  if( key == "HUMID" ){
    float humid = BeeNeXT.toFloat();
    Serial.printf("[HUMID] %.2f\n", humid);    
  }
}

// มีหรือไม่ก็ได้
// BEENEXT_CONNECTED() {
//   Serial.println("[BeeNeXT] connected");
// }

// มีหรือไม่ก็ได้
// BEENEXT_DISCONNECTED() {
//   Serial.println("[BeeNeXT] disconnected");
// }

