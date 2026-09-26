# Lab16 Smart Water Quality (ESP32)

ระบบเฝ้าระวังและปรับคุณภาพน้ำอัตโนมัติด้วย ESP32 — วัด pH / EC / อุณหภูมิน้ำ / TDS / ความเค็มผ่านเซนเซอร์ Modbus RTU (RS-485) ตัดสินคุณภาพน้ำเป็น 3 ระดับ (ดี / เฝ้าระวัง / ผิดปกติ) และควบคุมรีเลย์ 3 ตัว (ปั๊มปรับ pH, ปั๊มปุ๋ยปรับ EC, ปั๊มเติมอากาศ) ผ่านหน้าเว็บ Dashboard, จอ OLED และ MQTT

## โครงสร้างไฟล์

| ไฟล์ | หน้าที่ |
|---|---|
| `Lab16_SmartWaterQuality.ino` | โค้ดหลัก |
| `pins_config.h` | แผนผังขา GPIO |
| `dashboard_html.h` | หน้าเว็บ Dashboard (เก็บใน PROGMEM) |
| `fonts_data.h` | ฟอนต์ Prompt แบบ woff2 ฝังใน flash |
| `secrets.h.example` | ตัวอย่างค่า WiFi/MQTT — คัดลอกเป็น `secrets.h` แล้วแก้ค่าเอง |

## เริ่มใช้งาน

1. คัดลอก `secrets.h.example` เป็น `secrets.h` แล้วแก้ SSID/รหัสผ่าน WiFi และ MQTT topic base ของตัวเอง
   (ไฟล์ `secrets.h` ถูกกันไว้ใน `.gitignore` ไม่ขึ้น GitHub)
2. ติดตั้งไลบรารี: `PubSubClient`, `ModbusMaster`, `DHT sensor library`,
   `Adafruit Unified Sensor`, `Adafruit SSD1306`, `Adafruit GFX Library`
3. **ถอดสาย RS-485 A+/B- ออกก่อนอัปโหลดโปรแกรมทุกครั้ง** (ใช้ UART0 ร่วมกับ USB)
4. อัปโหลดขึ้นบอร์ด ESP32 Devkit V2 แล้วต่อ WiFi ชื่อ `WaterQual-xxxx` เพื่อเปิด Dashboard

## ⚠️ ความปลอดภัย

- หน้าเว็บไม่มีระบบล็อกอิน และ MQTT broker สาธารณะไม่มีการยืนยันตัวตน
- **ใช้ในเครือข่ายภายในเท่านั้น ห้ามเปิด port ออกอินเทอร์เน็ตโดยตรง**
- ปั๊มจ่ายสารเคมี (pH/ปุ๋ย) อันตรายกว่าปั๊มน้ำทั่วไป ตรวจสอบเกณฑ์ก่อนใช้งานจริงเสมอ
