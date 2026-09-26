/* ============================================================================
 *   SMART WATER QUALITY SENSOR
 *   ระบบเฝ้าระวังและปรับคุณภาพน้ำอัตโนมัติ  (BOI-SUT)
 *
 *   Lab16 · Board : ESP32 Devkit V2 (3 Relay + OLED + RS485)
 * ============================================================================
 *
 *  โปรเจกต์นี้ต่างจาก Lab15 ที่เป็น "Smart Farm รวมทุกอย่าง"
 *  Lab16 ตัดเรื่องอื่นออกและยก **คุณภาพน้ำเป็นตัวหลักของระบบ**
 *
 *    เปลี่ยน : คุณภาพน้ำ (pH / EC / TDS / ความเค็ม / อุณหภูมิน้ำ) เป็นค่าหลัก
 *    เปลี่ยน : รีเลย์ 3 ตัวเปลี่ยนบทบาทเป็นชุดปรับคุณภาพน้ำ
 *    เพิ่ม   : ระบบตัดสินคุณภาพน้ำเป็น 3 ระดับ  ดี / เฝ้าระวัง / ผิดปกติ
 *    เพิ่ม   : สเกลแสดงตำแหน่งค่าเทียบช่วงที่เหมาะสม ทั้งบนจอ OLED และหน้าเว็บ
 *    ตัดออก : ลูกลอยตรวจน้ำแห้ง ISO2 และ interlock ล็อกปั๊ม
 *
 *  --------------------------- เซนเซอร์ที่ใช้ ---------------------------
 *    Modbus ID 3   Water Quality      pH · EC · อุณหภูมิน้ำ · TDS · ความเค็ม  ◀ ตัวหลัก
 *    Modbus ID 1   RS-SD-N01-TR       ความชื้นดิน                             (ตัวประกอบ)
 *    Modbus ID 2   PR-300AL-RA-N01    ความเข้มแสงอาทิตย์ W/m2                 (ตัวประกอบ)
 *    GPIO15        DHT11              อุณหภูมิและความชื้นอากาศ                (ตัวประกอบ)
 *
 *    เซนเซอร์ทั้ง 3 ตัวบนสาย RS-485 เส้นเดียว ต้องตั้ง baud ตรงกันและ ID ไม่ซ้ำ
 *    รายละเอียด register map ทั้งหมดอยู่ในหัวไฟล์ของ Lab15
 *
 *  ------------------------- บทบาทของรีเลย์ 3 ตัว -------------------------
 *    R1  pH Doser        ปั๊มจ่ายสารปรับ pH (กรด/เบส)
 *    R2  Nutrient Doser  ปั๊มจ่ายปุ๋ย ใช้ปรับค่า EC
 *    R3  Aerator         ปั๊มเติมอากาศ / เวียนน้ำ
 *
 *    !! ปั๊มจ่ายสารเคมีอันตรายกว่าปั๊มน้ำธรรมดามาก !!
 *    จ่ายเกินไปไม่กี่นาทีก็ทำให้น้ำเสียทั้งถังและพืชตายได้
 *    ระบบจึงตั้งเวลาตัดอัตโนมัติของโหมดสั่งเองไว้สั้นมากเพียง 2 นาที
 *    (Lab ก่อน ๆ ตั้งไว้ 30 นาทีเพราะเป็นปั๊มน้ำที่จ่ายเกินแล้วไม่เสียหาย)
 *
 *  --------------------------- วิธีใช้งาน ---------------------------
 *    1. เปิดบอร์ด รอจอขึ้น "Ready"
 *    2. ที่มือถือ เปิด WiFi เลือกเครือข่ายชื่อ  WaterQual-xxxx
 *       รหัสผ่านตามค่า AP_PASS ด้านล่าง
 *    3. หน้า Dashboard เด้งขึ้นมาเอง (captive portal)
 *       ถ้าไม่เด้ง เปิดเบราว์เซอร์ไปที่  http://192.168.4.1
 *    4. ปุ่มบนบอร์ด : SW1 หน้าก่อน · SW2 หน้าถัดไป · SW3 ล็อก/ปลดล็อกการหมุนหน้า
 *
 *  ------------------------ ข้อจำกัดสำคัญของบอร์ดนี้ ------------------------
 *  RS485 ใช้ UART0 (GPIO1/GPIO3) ซึ่งเป็นสายเดียวกับพอร์ต USB
 *  โปรแกรมนี้จึงไม่มีคำสั่ง Serial.print() แม้แต่บรรทัดเดียว
 *  ช่องทางดูสถานะมี 3 ทาง คือ จอ OLED / หน้าเว็บ Dashboard / MQTT
 *  และต้องถอดสาย A+/B- ออกก่อนอัปโหลดโปรแกรมทุกครั้ง
 *
 *  ไลบรารีที่ต้องติดตั้ง
 *    PubSubClient / ModbusMaster / DHT sensor library
 *    Adafruit Unified Sensor / Adafruit SSD1306 / Adafruit GFX Library
 *
 *  !! ความปลอดภัย !!
 *  หน้าเว็บไม่มีระบบล็อกอิน และ broker สาธารณะไม่มีการยืนยันตัวตน
 *  ใช้ในเครือข่ายภายในเท่านั้น ห้ามเปิด port ออกอินเทอร์เน็ตโดยตรง
 * ==========================================================================*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>      // ใช้ทำ captive portal (มากับ ESP32 core)
#include <ESPmDNS.h>        // ใช้ทำชื่อ smartfarm.local (มากับ ESP32 core)
#include <Preferences.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ModbusMaster.h>
#include <ctype.h>
#include <time.h>
#include "pins_config.h"
#include "dashboard_html.h"
#include "fonts_data.h"      // ฟอนต์ Prompt ฝังใน flash ราว 62 KB
#include "secrets.h"         // WiFi/MQTT ของจริง ไม่ commit ขึ้น GitHub (ดู secrets.h.example)


// ======================== ค่าที่ต้องแก้ก่อนใช้งาน ==========================

/* ---------------------- WiFi AP ที่บอร์ดปล่อยเอง ----------------------
 * AP_SSID_PREFIX จะถูกต่อท้ายด้วยเลข 4 หลักจาก MAC เช่น SmartFarm-3A7F
 * เพื่อให้บอร์ดหลายตัวในห้องเรียนไม่ชื่อซ้ำกัน
 * AP_PASS ต้องยาวอย่างน้อย 8 ตัวอักษร ไม่งั้น WPA2 จะไม่ทำงานและ AP จะเปิดไม่ติด */
const char *AP_SSID_PREFIX = "WaterQual";
const char *AP_PASS        = SECRET_AP_PASS;
constexpr uint8_t AP_CHANNEL  = 1;        // ช่องสัญญาณเริ่มต้น (จะย้ายตาม STA เองถ้าต่อบ้านได้)
constexpr uint8_t AP_MAX_CONN = 4;        // รับมือถือได้พร้อมกันกี่เครื่อง

const IPAddress AP_IP(192, 168, 4, 1);    // IP ของบอร์ดในวง AP
const IPAddress AP_MASK(255, 255, 255, 0);

const char *MDNS_NAME = "waterqual";      // เปิดได้ที่ http://waterqual.local (เฉพาะวง STA)

/* ---------------------- WiFi บ้าน (STA) ----------------------
 * STA_ENABLE = 1  ต่อ WiFi บ้านไปด้วย เพื่อใช้ NTP และ MQTT
 * STA_ENABLE = 0  ปล่อย AP อย่างเดียว ใช้ในโรงเรือนที่ไม่มีเราเตอร์
 *                 (โหมดตั้งเวลาจะไม่ทำงานเพราะไม่มีเวลาจริง และ MQTT จะไม่ส่ง) */
#define STA_ENABLE 1
const char *WIFI_SSID = SECRET_WIFI_SSID;
const char *WIFI_PASS = SECRET_WIFI_PASS;
/* รอบการลองต่อ WiFi บ้านใหม่ แยกเป็น 2 กรณี
 *   ไม่เคยต่อได้เลย  -> SSID นั้นอาจไม่มีอยู่จริง ลองห่าง ๆ ทุก 5 นาที กัน AP กระตุก
 *   เคยต่อได้แล้วหลุด -> น่าจะแค่เราเตอร์รีบูต ลองถี่ขึ้นทุก 30 วินาที */
constexpr uint32_t STA_RETRY_SLOW_MS = 300000;
constexpr uint32_t STA_RETRY_FAST_MS = 30000;

const char *MQTT_HOST = SECRET_MQTT_HOST;
constexpr uint16_t MQTT_PORT = 1883;

#define TOPIC_BASE SECRET_TOPIC_BASE

// ----- NTP โซนเวลาไทย -----
const char *NTP_SERVER1 = "th.pool.ntp.org";
const char *NTP_SERVER2 = "pool.ntp.org";
const char *NTP_SERVER3 = "time.google.com";
const char *TZ_BANGKOK  = "ICT-7";      // -7 หมายถึง UTC+7 (รูปแบบ POSIX กลับด้าน)

// ----- Topic ส่งออก -----
const char *TOPIC_TEMP   = TOPIC_BASE "/temperature";
const char *TOPIC_HUMI   = TOPIC_BASE "/humidity";
const char *TOPIC_SOIL   = TOPIC_BASE "/soil";
const char *TOPIC_RAD    = TOPIC_BASE "/radiation";   // W/m2
const char *TOPIC_PH     = TOPIC_BASE "/ph";
const char *TOPIC_EC     = TOPIC_BASE "/ec";          // µS/cm
const char *TOPIC_WTEMP  = TOPIC_BASE "/watertemp";   // องศาเซลเซียส
const char *TOPIC_TDS    = TOPIC_BASE "/tds";
const char *TOPIC_SALT   = TOPIC_BASE "/salinity";
const char *TOPIC_VERDICT= TOPIC_BASE "/verdict";    // GOOD / WATCH / BAD (retained)
const char *TOPIC_STATUS = TOPIC_BASE "/status";
const char *TOPIC_ST_R1  = TOPIC_BASE "/status/relay1";
const char *TOPIC_ST_R2  = TOPIC_BASE "/status/relay2";
const char *TOPIC_ST_R3  = TOPIC_BASE "/status/relay3";

// ----- Topic รับคำสั่ง -----
const char *TOPIC_CMD_R1 = TOPIC_BASE "/cmd/relay1";
const char *TOPIC_CMD_R2 = TOPIC_BASE "/cmd/relay2";
const char *TOPIC_CMD_R3 = TOPIC_BASE "/cmd/relay3";
const char *TOPIC_CMDALL = TOPIC_BASE "/cmd/all";
const char *TOPIC_CMDSUB = TOPIC_BASE "/cmd/#";

/* ---------------- เซนเซอร์ความชื้นดิน RS485 (Modbus RTU) ----------------
 * ค่าเหล่านี้ยืนยันจากการทดสอบจริงใน Lab11 แล้ว
 * และ "ไม่ตรงกับคู่มือ" ถึง 3 จุด ดูรายละเอียดในหัวไฟล์ Lab11 */
#define RS485          Serial              // UART0 = GPIO1 (TX) / GPIO3 (RX)
constexpr uint32_t RS485_BAUDRATE = 4800;  // baud ของสายรวม ทุกตัวต้องตั้งให้ตรงกันนี้

// ----- ID 1 : เซนเซอร์ความชื้นดิน RS-SD-N01-TR -----
constexpr uint8_t  SOIL_SLAVE_ID  = 1;
constexpr uint16_t SOIL_REG       = 0x0000;// เครื่องจริงใช้ 0x0000 (คู่มือเขียน 0x0001)
constexpr float    SOIL_DIVISOR   = 10.0;  // เครื่องจริงคูณ 10 (คู่มือเขียน 100)

// ----- ID 2 : ความเข้มแสงอาทิตย์ PR-300AL-RA-N01 -----
constexpr uint8_t  RAD_SLAVE_ID   = 2;
constexpr uint16_t RAD_REG        = 0x0003;
constexpr float    RAD_DIVISOR    = 1.0;   // คู่มือระบุว่าเป็น "actual value" ไม่ต้องหาร
constexpr uint16_t RAD_REG_ID     = 0x07D0;// รีจิสเตอร์ตั้ง Slave ID
constexpr uint16_t RAD_REG_BAUD   = 0x07D1;// 0=2400 1=4800 2=9600

// ----- ID 3 : คุณภาพน้ำ pH / EC / อุณหภูมิ / TDS / ความเค็ม -----
constexpr uint8_t  WQ_SLAVE_ID    = 3;
constexpr uint16_t WQ_REG         = 0x0000;// อ่าน 5 รีจิสเตอร์ต่อเนื่องในครั้งเดียว
constexpr uint8_t  WQ_QTY         = 5;
constexpr uint16_t WQ_REG_ID      = 0x0030;// รีจิสเตอร์ตั้ง Slave ID
constexpr uint16_t WQ_REG_BAUD    = 0x0031;// ใส่ค่า baud ตรง ๆ เช่น 4800
constexpr uint32_t WQ_FACTORY_BAUD = 9600; // baud ที่ออกจากโรงงาน ใช้ตอนตั้งค่า

/* ---------------------- โหมดตั้งค่าเซนเซอร์ (Commissioning) ----------------------
 * ใช้ครั้งเดียวตอนติดตั้งเซนเซอร์ใหม่ ต่อเซนเซอร์เข้าสายเพียง "ตัวเดียว" เท่านั้น
 *
 *   0 = ทำงานปกติ
 *   2 = ตั้ง PR-300AL-RA-N01  ให้เป็น Slave ID 2  (คุยที่ 4800 ตาม baud โรงงาน)
 *   3 = ตั้ง Water Quality    ให้เป็น Slave ID 3 และเปลี่ยน baud 9600 -> 4800
 *
 * ผลลัพธ์แสดงบนจอ OLED ทำเสร็จแล้วตั้งกลับเป็น 0 แล้วอัปโหลดใหม่
 *
 * ทำไมต้องมีโหมดนี้ในโปรแกรม
 *   ปกติต้องใช้ซอฟต์แวร์ config ของผู้ผลิตกับตัวแปลง USB-RS485 แยก
 *   แต่บอร์ดนี้มีวงจร RS-485 อยู่แล้ว จึงตั้งค่าได้เองเลย ไม่ต้องหาอุปกรณ์เพิ่ม */
#define COMMISSION_MODE  0

/* ------------------------- ค่าจำลองเมื่อไม่มีเซนเซอร์ -------------------------
 * กำหนดว่ารีเลย์ตัวไหนคือปั๊มน้ำและตัวไหนคือพัดลม เพื่อให้ค่าจำลองตอบสนองถูกตัว
 * นับจาก 0 คือ Relay1 */
/* บทบาทของรีเลย์ในระบบปรับคุณภาพน้ำ (นับจาก 0)
 * ค่าจำลองจะตอบสนองตามบทบาทนี้ ทำให้ทดสอบตรรกะควบคุมได้ครบโดยไม่ต้องมีปั๊มจริง
 *   จ่ายสารปรับ pH ทำงาน  -> ค่า pH ค่อย ๆ เข้าหากลาง
 *   จ่ายปุ๋ยทำงาน          -> ค่า EC เพิ่มขึ้น
 *   เติมอากาศทำงาน         -> อุณหภูมิน้ำลดลงเล็กน้อย */
constexpr uint8_t R_PH_DOSER  = 0;      // R1 ปั๊มจ่ายสารปรับ pH
constexpr uint8_t R_NUTRIENT  = 1;      // R2 ปั๊มจ่ายปุ๋ย
constexpr uint8_t R_AERATOR   = 2;      // R3 ปั๊มเติมอากาศ / เวียนน้ำ

const char *RELAY_ROLE[3] = { "pH Doser", "Nutrient", "Aerator" };
const char *RELAY_ROLE_TH[3] = { "จ่ายสารปรับ pH", "จ่ายปุ๋ย", "เติมอากาศ" };


constexpr float SIM_SOIL_START = 45.0;
constexpr float SIM_SOIL_DRY   = 0.4;   // % ที่ลดลงต่อรอบ เมื่อไม่ได้รดน้ำ
constexpr float SIM_TEMP_BASE  = 30.0;
constexpr float SIM_HUMI_BASE  = 62.0;

/* ค่าจำลองของเซนเซอร์ใหม่
 * แสงอาทิตย์จำลองตามเวลาจริงจากนาฬิกา NTP เป็นรูประฆังคว่ำ
 * สว่างสุดเที่ยงวัน มืดสนิทกลางคืน ทำให้ทดสอบตรรกะสั่งงานตามแสงได้สมจริง
 * ถ้านาฬิกายังไม่ซิงค์ จะใช้ค่ากลางคงที่ไปก่อน */
constexpr float SIM_RAD_PEAK   = 950.0; // W/m2 สูงสุดตอนเที่ยงวันฟ้าโปร่ง
/* ------------------------ เกณฑ์ตัดสินคุณภาพน้ำ ------------------------
 * ระบบแปลงตัวเลขดิบให้เป็นคำตัดสิน 3 ระดับ  ดี / เฝ้าระวัง / ผิดปกติ
 * เพราะ "pH 8.7" อย่างเดียวไม่บอกอะไรกับคนที่ไม่ได้จำช่วงที่เหมาะสมไว้
 *
 * ช่วง OK  = อยู่ในเกณฑ์ที่ต้องการ
 * ช่วง WARN = ยังพอรับได้แต่ควรเฝ้าระวัง (ระหว่างขอบ OK กับขอบ BAD)
 * นอกช่วง WARN = ผิดปกติ ต้องแก้ไข
 *
 * ค่า pH อ้างอิงเกณฑ์น้ำใช้ทั่วไปตามคู่มือเซนเซอร์ (6.5-8.5)
 * ค่า EC ตั้งตามพืชที่ปลูก ปรับได้ตามชนิดพืชและช่วงการเจริญเติบโต */
constexpr float PH_OK_LO   = 6.5,  PH_OK_HI   = 8.5;
constexpr float PH_BAD_LO  = 5.5,  PH_BAD_HI  = 9.5;
constexpr float EC_OK_LO   = 800,  EC_OK_HI   = 1800;   // µS/cm
constexpr float EC_BAD_LO  = 400,  EC_BAD_HI  = 2500;
constexpr float WT_OK_LO   = 18.0, WT_OK_HI   = 32.0;   // อุณหภูมิน้ำ °C
constexpr float WT_BAD_LO  = 12.0, WT_BAD_HI  = 38.0;

constexpr float SIM_PH_BASE    = 6.8;
constexpr float SIM_EC_BASE    = 850.0; // µS/cm
constexpr float SIM_WTEMP_BASE = 27.0;

// ----- รอบเวลาการทำงาน -----
constexpr uint32_t DHT_INTERVAL_MS   = 2000;
/* รอบการอ่านเซนเซอร์ Modbus
 *
 * ระบบอ่าน "ทีละตัว" วนไปเรื่อย ๆ (round-robin) ไม่อ่านทั้ง 3 ตัวในรอบเดียว
 *
 * เหตุผลสำคัญ : ModbusMaster รอ timeout นานถึง 2 วินาทีเมื่อเซนเซอร์ไม่ตอบ
 * ถ้าอ่านทั้ง 3 ตัวติดกันในรอบเดียวแล้วไม่มีตัวไหนตอบ ระบบจะถูกบล็อกถึง 6 วินาที
 * หน้าเว็บจะกระตุกจนใช้ไม่ได้ และ MQTT จะตอบ ping ไม่ทันจนถูกตัด
 *
 * อ่านทีละตัวทุก 2 วินาที -> เซนเซอร์แต่ละตัวได้อัปเดตทุก 6 วินาที
 * ซึ่งเพียงพอ เพราะเซนเซอร์แสงตอบสนองช้าถึง 10 วินาทีอยู่แล้ว
 * และความชื้นดินกับคุณภาพน้ำเปลี่ยนแปลงในหลักนาที ไม่ใช่วินาที */
constexpr uint32_t MB_INTERVAL_MS = 2000;   // เว้นกี่ ms ก่อนอ่านตัวถัดไป
constexpr uint32_t MB_RETRY_MS    = 30000;  // ตัวที่อยู่ในโหมดจำลอง ลองใหม่ทุกกี่ ms
constexpr uint32_t PUBLISH_INTERVAL  = 5000;
constexpr uint32_t OLED_INTERVAL_MS  = 500;
constexpr uint32_t TIME_INTERVAL_MS  = 250;
constexpr uint32_t MQTT_RETRY_MS     = 5000;
constexpr uint32_t STA_CHECK_MS      = 1000;    // ตรวจสถานะ WiFi บ้านทุกกี่ ms

constexpr uint8_t  MAX_FAIL_STREAK   = 3;       // พลาดกี่ครั้งติดจึงเปลี่ยนเป็นโหมดจำลอง

/* ตัวตัดอัตโนมัติกันรีเลย์ค้าง ใช้เฉพาะโหมด "สั่งเอง" เท่านั้น
 * โหมดตั้งเวลามีเวลาสิ้นสุดของตัวเองอยู่แล้ว
 * ส่วนโหมดอัตโนมัติอาจต้องเปิดพัดลมยาวหลายชั่วโมงตามสภาพอากาศจริง */
/* ปั๊มจ่ายสารเคมีจ่ายเกินไม่กี่นาทีก็ทำให้น้ำเสียทั้งถัง
 * จึงตั้งเวลาตัดอัตโนมัติของโหมดสั่งเองไว้สั้นมากเพียง 2 นาที
 * (Lab ก่อน ๆ ตั้ง 30 นาทีเพราะเป็นปั๊มน้ำที่จ่ายเกินแล้วไม่เสียหาย) */
constexpr float    RELAY_MAX_ON_MIN = 2.0;
constexpr uint32_t RELAY_MAX_ON_MS  = (uint32_t)(RELAY_MAX_ON_MIN * 60000.0);


// ============================ โครงสร้างข้อมูล =============================
/* ประกาศ enum/struct ไว้เหนือฟังก์ชันแรกของไฟล์เสมอ
 * เพราะ Arduino IDE แทรก prototype ของทุกฟังก์ชันไว้เหนือฟังก์ชันแรก */

enum { MODE_MANUAL = 0, MODE_SCHEDULE = 1, MODE_AUTO = 2 };
/* แหล่งข้อมูลที่โหมดอัตโนมัติใช้ตัดสินใจ
 * Lab12 มี 3 ตัวแรก · Lab15 เพิ่ม 3 ตัวหลังจากเซนเซอร์ Modbus ใหม่
 * ลำดับตัวเลขต้องตรงกับ dropdown ในหน้าเว็บ (ตัวแปร SRC_TH ใน dashboard_html.h) */
enum { SRC_TEMP = 0, SRC_HUMI = 1, SRC_SOIL = 2,
       SRC_RAD  = 3, SRC_PH   = 4, SRC_EC   = 5 };
constexpr uint8_t SRC_MAX = 5;

struct RelayCfg {
  uint8_t  mode;
  bool     manualOn;
  uint8_t  onHour;
  uint8_t  onMin;
  uint16_t runMin;
  uint8_t  src;        // SRC_TEMP / SRC_HUMI / SRC_SOIL
  bool     above;
  float    thr;
  float    hyst;
};

struct RelayCtl {
  uint8_t     pin;
  const char *name;
  const char *cmdTopic;
  const char *statusTopic;
  bool        on;
  uint32_t    onAt;
  int16_t     lastDay;
};

/* ------------------------- เซนเซอร์ Modbus บนสายเดียว -------------------------
 * เก็บเป็นอาร์เรย์เพื่อให้เพิ่มเซนเซอร์ตัวใหม่ได้โดยแก้ที่เดียว
 * ไม่ต้องเขียนฟังก์ชัน taskReadXxx() แยกทีละตัวแบบ Lab11-Lab14
 *
 * id     Slave ID บนสาย ต้องไม่ซ้ำกัน
 * reg    รีจิสเตอร์เริ่มต้นที่จะอ่าน
 * qty    อ่านกี่รีจิสเตอร์ต่อเนื่อง (คุณภาพน้ำอ่าน 5 ตัวในครั้งเดียว ประหยัดเวลาบนสาย)
 * real   true = ได้ค่าจากเซนเซอร์จริง · false = กำลังใช้ค่าจำลอง
 * fail   นับครั้งที่อ่านพลาดติดกัน ครบ MAX_FAIL_STREAK จึงสลับไปโหมดจำลอง
 * lastAt เวลาที่อ่านตัวนี้ล่าสุด ใช้กับรอบลองใหม่ตอนอยู่โหมดจำลอง */
struct ModbusDev {
  uint8_t     id;
  const char *name;      // ชื่อสั้นสำหรับแสดงบนจอ (อังกฤษเท่านั้น)
  uint16_t    reg;
  uint8_t     qty;
  bool        real;
  uint8_t     fail;
  uint32_t    lastAt;
  uint32_t    okCount;
  uint32_t    errCount;
  uint8_t     lastErr;   // รหัสข้อผิดพลาดล่าสุดของ ModbusMaster
};

constexpr uint8_t MB_COUNT = 3;
enum { MB_SOIL = 0, MB_RAD = 1, MB_WQ = 2 };

ModbusDev mbDev[MB_COUNT] = {
  { SOIL_SLAVE_ID, "SOIL", SOIL_REG, 1,       false, 0, 0, 0, 0, 0 },
  { RAD_SLAVE_ID,  "RAD",  RAD_REG,  1,       false, 0, 0, 0, 0, 0 },
  { WQ_SLAVE_ID,   "WQ",   WQ_REG,   WQ_QTY,  false, 0, 0, 0, 0, 0 },
};


/* ==================== ปุ่มกดบนบอร์ด สำหรับเลื่อนหน้าจอ ====================
 * ปุ่ม SW1-SW3 ไม่ได้ถูกใช้เลยตั้งแต่ Lab2 · Lab15 นำมาทำเป็นปุ่มนำทางหน้าจอ
 *
 *   SW1 (GPIO34)  ย้อนกลับหน้าก่อนหน้า
 *   SW2 (GPIO35)  ไปหน้าถัดไป
 *   SW3 (GPIO32)  สลับเปิด/ปิดการหมุนหน้าอัตโนมัติ
 *
 * ตั้งใจให้ปุ่มทำได้แค่ "ดูข้อมูล" ไม่ให้สั่งรีเลย์
 * เพราะปุ่มอยู่หน้าตู้ที่คนเดินผ่านกดเล่นได้ ถ้ากดแล้วปั๊มติดจะเป็นเรื่อง
 * การสั่งงานจริงให้ทำผ่านหน้าเว็บหรือ MQTT ซึ่งมีบริบทให้เห็นครบก่อนกด
 *
 * ปุ่มกดจริงหน้าสัมผัสจะ "เด้ง" สลับ 0/1 อยู่ 1-20 ms
 * ถ้าไม่กรอง กด 1 ครั้งจะเลื่อนหน้าไป 5-10 หน้า จึงต้องมี debounce
 * (หลักการเดียวกับ Lab2 และตัวกรองลูกลอยใน Lab14) */
struct UiButton {
  uint8_t  pin;
  bool     stable;        // สถานะที่ยืนยันแล้ว (true = กำลังกด)
  bool     lastRaw;
  uint32_t changedAt;
  bool     pressed;       // ธง "เพิ่งกด" ยกขึ้น 1 ครั้งต่อการกด 1 ที
};

constexpr uint32_t BTN_DEBOUNCE_MS = 40;

UiButton uiBtn[3] = {
  { SW1_PIN, false, false, 0, false },
  { SW2_PIN, false, false, 0, false },
  { SW3_PIN, false, false, 0, false },
};

/* ----------------------------- หน้าจอทั้งหมด -----------------------------
 * แบ่งเป็นหน้าละเรื่อง เพื่อให้แต่ละหน้ามีที่ว่างพอ ตัวอักษรใหญ่ อ่านได้จากระยะไกล
 * ไม่ยัดทุกอย่างลงหน้าเดียวจนล้นและอ่านไม่ออกเหมือน Lab ก่อน ๆ */
enum { PG_OVERVIEW = 0, PG_PH, PG_EC, PG_TEMP, PG_ENV, PG_DOSING, PG_SENSOR, PG_NET, PG_COUNT };

const char *PG_TITLE[PG_COUNT] = {
  "WATER QUALITY", "pH LEVEL", "EC / TDS", "TEMPERATURE",
  "ENVIRONMENT", "DOSING", "SENSOR HEALTH", "NETWORK",
};

constexpr uint32_t PAGE_ROTATE_MS = 5000;   // หมุนหน้าอัตโนมัติทุกกี่ ms
constexpr uint32_t PAGE_HOLD_MS   = 20000;  // กดปุ่มแล้วหยุดหมุนกี่ ms ก่อนหมุนต่อ

uint8_t  uiPage     = PG_OVERVIEW;
bool     uiAutoScan = true;     // หมุนหน้าเองหรือไม่ (SW3 สลับ)
uint32_t uiHoldAt   = 0;        // เวลาที่กดปุ่มล่าสุด ใช้หยุดหมุนชั่วคราว
uint32_t uiRotateAt = 0;

constexpr uint8_t RELAY_COUNT = 3;

RelayCtl relays[RELAY_COUNT] = {
  { RELAY1_PIN, "R1", TOPIC_CMD_R1, TOPIC_ST_R1, false, 0, -1 },
  { RELAY2_PIN, "R2", TOPIC_CMD_R2, TOPIC_ST_R2, false, 0, -1 },
  { RELAY3_PIN, "R3", TOPIC_CMD_R3, TOPIC_ST_R3, false, 0, -1 },
};

/* ค่าตั้งต้นเมื่อยังไม่เคยบันทึกอะไรลง NVS
 * ทุกตัวเริ่มที่โหมดสั่งเองและสถานะปิด เพื่อความปลอดภัยตอนติดตั้งครั้งแรก
 * แต่ตั้งเกณฑ์อัตโนมัติเตรียมไว้ให้แล้ว กดเลือกโหมดก็ใช้ได้ทันที */
RelayCfg cfgs[RELAY_COUNT] = {
  // R1 จ่ายสารปรับ pH : pH สูงเกิน 8.5 -> จ่ายกรด · หน่วง 0.3 กันจ่ายรัว
  { MODE_MANUAL, false,  6,  0,  1, SRC_PH,  true,  8.5,  0.3 },
  // R2 จ่ายปุ๋ย : EC ต่ำกว่า 800 -> เติมปุ๋ย · หน่วง 100 µS/cm
  { MODE_MANUAL, false,  7,  0,  2, SRC_EC,  false, 800.0, 100.0 },
  // R3 เติมอากาศ : ตั้งเวลาเดินทุกวัน 06:00 นาน 30 นาที
  { MODE_SCHEDULE, false, 6, 0, 30, SRC_TEMP, true, 33.0, 1.5 },
};


// ============================== อ็อบเจกต์หลัก =============================
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
DHT dht(DHT_PIN, DHT_TYPE);
ModbusMaster node;

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
WebServer    server(80);
DNSServer    dnsServer;         // ตอบทุกคำถาม DNS ด้วย IP ของบอร์ด -> captive portal
Preferences  prefs;

String clientId;
String apSsid;                  // ชื่อ AP จริงหลังต่อท้ายด้วยเลขจาก MAC

// ----- สถานะ WiFi บ้าน (STA) -----
bool     staConnected     = false;
bool     staEverConnected = false;   // เคยต่อบ้านได้สักครั้งไหม ใช้เลือกรอบลองใหม่
bool     mdnsStarted      = false;
uint32_t staCheckAt       = 0;
uint32_t staRetryAt       = 0;


// ============================== ตัวแปรสถานะ ===============================
uint32_t dhtAt = 0, mbAt = 0, pubAt = 0, oledAt = 0, mqttRetryAt = 0, timeAt = 0;

float temp = NAN, humi = NAN;                      // DHT11
float soil = NAN;                                  // Modbus ID 1
float rad  = NAN;                                  // Modbus ID 2  W/m2
float wqPh = NAN, wqEc = NAN, wqTemp = NAN,        // Modbus ID 3
      wqTds = NAN, wqSalt = NAN;

/* ธงบอกว่าค่านั้นมาจากเซนเซอร์จริงหรือจากการจำลอง
 * DHT แยกไว้ต่างหากเพราะไม่ได้อยู่บนสาย Modbus
 * ส่วนเซนเซอร์ Modbus ทั้ง 3 ตัวเก็บธงไว้ในอาร์เรย์ mbDev[] ด้านล่าง */
bool dhtReal = false;
uint8_t dhtFail = 0;

float simSoil  = SIM_SOIL_START;
float simTemp  = SIM_TEMP_BASE;
float simHumi  = SIM_HUMI_BASE;
float simRad   = 0.0;
float simPh    = SIM_PH_BASE;
float simEc    = SIM_EC_BASE;
float simWtemp = SIM_WTEMP_BASE;

uint32_t pubCount = 0;

struct tm tmNow      = {0};
bool      timeSynced = false;


// ========================= บันทึกค่าตั้งลง NVS ============================
/* บันทึกเฉพาะตอนผู้ใช้กดบันทึกค่าตั้งเท่านั้น
 * ไม่บันทึกทุกครั้งที่รีเลย์เปลี่ยนสถานะ เพราะ flash เขียนซ้ำได้ราว 100,000 ครั้ง */
void saveConfig() {
  prefs.begin("lab12", false);
  prefs.putBytes("cfg", cfgs, sizeof(cfgs));
  prefs.end();
}

void loadConfig() {
  prefs.begin("lab12", true);
  if (prefs.getBytesLength("cfg") == sizeof(cfgs))
    prefs.getBytes("cfg", cfgs, sizeof(cfgs));
  prefs.end();

  /* บังคับให้ทุกตัวเริ่มที่ปิดเสมอหลังบูต
   * ถ้าจำสถานะเปิดไว้แล้วไฟดับตอนกลางคืน พอไฟมาปั๊มจะเดินเองโดยไม่มีใครรู้ */
  for (uint8_t i = 0; i < RELAY_COUNT; i++) cfgs[i].manualOn = false;
}


// ============================ นาฬิกาจาก NTP ===============================
/* ใช้ time() + localtime_r() แทน getLocalTime()
 * เพราะ getLocalTime() มี delay() รออยู่ข้างในนานถึง 5 วินาที */
void taskUpdateTime(uint32_t now) {
  if (now - timeAt < TIME_INTERVAL_MS) return;
  timeAt = now;

  time_t epoch;
  time(&epoch);
  localtime_r(&epoch, &tmNow);
  timeSynced = (tmNow.tm_year > (2016 - 1900));   // ก่อนซิงค์จะเป็นปี 1970
}

void timeStampFull(char *buf, size_t n) {
  if (timeSynced) strftime(buf, n, "%Y-%m-%d %H:%M:%S", &tmNow);
  else            snprintf(buf, n, "no-time");
}


// ======================= ตัวช่วยจัดการ RS485 บนบอร์ดนี้ ===================
/* MAX13487 สลับทิศทางเอง จึงไม่ต้องใช้ preTransmission
 * แต่ postTransmission ยังมีประโยชน์ ใช้ทิ้งเสียงสะท้อนของคำสั่งที่เราเพิ่งส่ง
 * ปลอดภัยเพราะมาตรฐาน Modbus บังคับให้อุปกรณ์ลูกเงียบอย่างน้อย 3.5 ตัวอักษร
 * (ราว 7 ms ที่ 4800) ก่อนตอบกลับ การล้างบัฟเฟอร์ภายใน 2.5 ms จึงไม่กินคำตอบจริง */
void postTransmission() {
  delayMicroseconds(2500);                 // ราว 1 ไบต์ที่ 4800 bps
  while (RS485.available()) RS485.read();
}


// ========================= จัดการรีเลย์และรายงานสถานะ ======================
void publishRelayStatus(RelayCtl &r) {
  if (!mqtt.connected()) return;
  mqtt.publish(r.statusTopic, r.on ? "ON" : "OFF", true);   // true = retained
}

void publishAllRelayStatus() {
  for (uint8_t i = 0; i < RELAY_COUNT; i++) publishRelayStatus(relays[i]);
}

void relaySet(RelayCtl &r, bool on) {
  if (r.on == on) { publishRelayStatus(r); return; }
  r.on = on;
  if (on) r.onAt = millis();
  digitalWrite(r.pin, on ? RELAY_ON : RELAY_OFF);
  publishRelayStatus(r);
}

uint32_t relayRemainSec(uint8_t i) {
  RelayCtl &r = relays[i];
  RelayCfg &c = cfgs[i];
  if (!r.on) return 0;

  uint32_t limitMs = 0;
  if      (c.mode == MODE_SCHEDULE) limitMs = (uint32_t)c.runMin * 60000UL;
  else if (c.mode == MODE_MANUAL)   limitMs = RELAY_MAX_ON_MS;
  else return 0;                    // โหมดอัตโนมัติไม่มีเวลาสิ้นสุดตายตัว

  uint32_t elapsed = millis() - r.onAt;
  if (elapsed >= limitMs) return 0;
  return (limitMs - elapsed + 999) / 1000;
}



// ======================= ตรรกะควบคุมรีเลย์ทั้ง 3 โหมด =====================
void taskControl() {
  /* อ่าน millis() สดตรงนี้เอง ห้ามรับค่า now จาก loop()
   * เพราะ relaySet() ตั้ง onAt ด้วย millis() ที่อาจเกิดหลัง now
   * ทำให้ now - onAt ติดลบแล้ววนกลับเป็นเลขมหาศาล (บั๊กที่เคยเจอใน Lab8) */
  uint32_t now = millis();

  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    RelayCtl &r = relays[i];
    RelayCfg &c = cfgs[i];

    switch (c.mode) {

      case MODE_MANUAL:
        if (r.on != c.manualOn) relaySet(r, c.manualOn);
        break;

      case MODE_SCHEDULE: {
        if (!timeSynced) break;                  // ไม่มีเวลาจริง ไม่กล้าสั่งงาน

        if (r.on) {
          if (now - r.onAt >= (uint32_t)c.runMin * 60000UL) relaySet(r, false);
        } else {
          /* lastDay กันไม่ให้สั่งซ้ำ เพราะเงื่อนไข HH:MM เป็นจริงอยู่นานถึง 60 วินาที
           * ถ้าไม่กัน ระบบจะสั่งเปิดซ้ำหลายพันครั้งภายในนาทีเดียว */
          if (tmNow.tm_hour == c.onHour && tmNow.tm_min == c.onMin &&
              r.lastDay != tmNow.tm_yday) {
            r.lastDay = tmNow.tm_yday;
            relaySet(r, true);
          }
        }
        break;
      }

      case MODE_AUTO: {
        /* เลือกค่าที่จะเอามาตัดสินตามที่ผู้ใช้ตั้งไว้
         * ค่าจำลองก็ใช้ได้เหมือนกัน เพราะเก็บในตัวแปรเดียวกัน
         * ทำให้ทดสอบตรรกะได้ครบโดยไม่ต้องมีเซนเซอร์จริง */
        float v;
        if      (c.src == SRC_TEMP) v = temp;
        else if (c.src == SRC_HUMI) v = humi;
        else if (c.src == SRC_SOIL) v = soil;
        else if (c.src == SRC_RAD)  v = rad;      // W/m2  เช่น แดดจัดเกิน -> กางสแลน
        else if (c.src == SRC_PH)   v = wqPh;     // เช่น pH สูงเกิน -> เติมกรด
        else                        v = wqEc;     // EC ต่ำเกิน -> เติมปุ๋ย
        if (isnan(v)) break;                     // ยังไม่มีค่าใด ๆ

        /* ช่วงหน่วง (hysteresis) แยกจุดเปิดกับจุดปิดออกจากกัน
         * เกณฑ์ 40 ช่วงหน่วง 5 -> เปิดเมื่อดินแห้งถึง 40% ปิดเมื่อชื้นเกิน 45%
         * ถ้าใช้เกณฑ์เดียว ค่าที่แกว่งรอบจุดตัดจะทำให้ปั๊มเปิด-ปิดรัวจนพัง */
        bool want;
        if (c.above) want = r.on ? (v > c.thr - c.hyst) : (v >= c.thr);
        else         want = r.on ? (v < c.thr + c.hyst) : (v <= c.thr);

        if (want != r.on) relaySet(r, want);
        break;
      }
    }
  }
}

/* ตัดรีเลย์ที่เปิดค้างนานเกินกำหนด ใช้เฉพาะโหมดสั่งเองเท่านั้น */
void taskRelayFailsafe() {
  if (RELAY_MAX_ON_MS == 0) return;
  uint32_t now = millis();

  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    if (!relays[i].on || cfgs[i].mode != MODE_MANUAL) continue;
    if (now - relays[i].onAt < RELAY_MAX_ON_MS) continue;
    cfgs[i].manualOn = false;
    relaySet(relays[i], false);
  }
}


// ====================== งานที่ 1 : อ่าน DHT11 หรือจำลอง ====================
/* ค่าจำลองของอุณหภูมิและความชื้นอากาศ
 * แกว่งกลับเข้าหาค่ากลางเสมอ ไม่ปล่อยให้ลอยหนีไปเรื่อย ๆ แบบเลขสุ่มล้วน
 * และลดลงเมื่อพัดลมทำงาน เพื่อให้ทดสอบตรรกะควบคุมได้จริง */
void simulateDHT() {
  float drift = (random(-100, 101) / 100.0) * 0.25;
  // ดึงกลับเข้าหาค่ากลางเสมอ โปรเจกต์นี้ไม่มีพัดลมจึงไม่มีอะไรมาทำให้เย็นลง
  simTemp += drift + (SIM_TEMP_BASE - simTemp) * 0.02;

  simHumi += (random(-100, 101) / 100.0) * 0.4
           + (SIM_HUMI_BASE - simHumi) * 0.02;

  if (simTemp < 18) simTemp = 18;
  if (simTemp > 45) simTemp = 45;
  if (simHumi < 20) simHumi = 20;
  if (simHumi > 95) simHumi = 95;

  temp = simTemp;
  humi = simHumi;
}

void taskReadDHT(uint32_t now) {
  if (now - dhtAt < DHT_INTERVAL_MS) return;
  dhtAt = now;

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    dhtFail = 0;
    dhtReal = true;
    temp = t;
    humi = h;
    simTemp = t;                 // ให้ค่าจำลองเริ่มจากค่าจริงล่าสุด
    simHumi = h;                 // ถ้าเซนเซอร์หลุดทีหลัง ค่าจะไม่กระโดด
    return;
  }

  if (dhtFail < MAX_FAIL_STREAK) dhtFail++;
  if (dhtFail >= MAX_FAIL_STREAK) {
    dhtReal = false;             // พลาดติดกันมากพอแล้ว เปลี่ยนไปใช้ค่าจำลอง
    simulateDHT();
  }
}


// ============= งานที่ 2 : อ่านเซนเซอร์ Modbus ทั้ง 3 ตัว หรือจำลอง =============

/* ---------------------------- ค่าจำลองแต่ละตัว ----------------------------
 * ค่าจำลองทุกตัวเดินอย่างมีเหตุผลตามสภาพจริง ไม่ใช่เลขสุ่มมั่ว
 * เพื่อให้ทดสอบตรรกะควบคุมได้ครบวงจรโดยไม่ต้องมีเซนเซอร์ */

/* ความชื้นดิน : ค่อย ๆ แห้งลงตามการระเหย
 * โปรเจกต์นี้ไม่มีปั๊มรดน้ำ (รีเลย์ทั้ง 3 ตัวเป็นชุดปรับคุณภาพน้ำ)
 * ค่าดินจึงเป็นแค่ข้อมูลประกอบ ไม่ตอบสนองต่อรีเลย์ใด ๆ
 * เมื่อแห้งถึงขั้นต่ำจะวนกลับขึ้นไป เพื่อให้ค่าจำลองไม่ค้างที่ก้นสเกล */
void simulateSoil() {
  simSoil -= SIM_SOIL_DRY;
  if (simSoil <= 12) simSoil = 70;            // สมมติว่ามีคนมารดน้ำ
  simSoil += (random(-100, 101) / 100.0) * 0.3;
  if (simSoil < 5)  simSoil = 5;
  if (simSoil > 95) simSoil = 95;
  soil = simSoil;
}

/* ความเข้มแสง : จำลองเป็นรูประฆังคว่ำตามเวลาจริงจากนาฬิกา NTP
 * สว่างสุดเที่ยงวัน ค่อย ๆ ลดลงหัวค่ำ และเป็นศูนย์กลางคืน
 * ทำให้ทดสอบเงื่อนไข "แดดจัดเกินไปให้กางสแลน" ได้จริงโดยไม่ต้องรอแดด
 *
 * ใช้สูตรพาราโบลารอบเที่ยงวัน แทน sin เพราะอ่านเข้าใจง่ายกว่า
 * และไม่ต้องดึงไลบรารีคณิตศาสตร์เพิ่ม */
void simulateRad() {
  if (!timeSynced) { simRad = SIM_RAD_PEAK * 0.5; rad = simRad; return; }

  float h = tmNow.tm_hour + tmNow.tm_min / 60.0;
  if (h < 6.0 || h > 18.0) {
    simRad = 0;                                  // กลางคืน
  } else {
    float x = (h - 12.0) / 6.0;                  // -1 ตอน 6 โมง, 0 ตอนเที่ยง, +1 ตอน 18 น.
    simRad = SIM_RAD_PEAK * (1.0 - x * x);       // พาราโบลาคว่ำ
    simRad += (random(-100, 101) / 100.0) * 40;  // เมฆผ่านเป็นครั้งคราว
    if (simRad < 0) simRad = 0;
  }
  rad = simRad;
}

// คุณภาพน้ำ : แกว่งรอบค่ากลาง ดึงกลับเข้าหาค่ากลางเสมอ ไม่ลอยหนีไปเรื่อย ๆ
void simulateWq() {
  // แกว่งรอบค่ากลาง และดึงกลับเข้าหาค่ากลางเสมอ ไม่ลอยหนีไปเรื่อย ๆ
  simPh    += (random(-100, 101) / 100.0) * 0.03 + (SIM_PH_BASE    - simPh)    * 0.02;
  simEc    += (random(-100, 101) / 100.0) * 6.0  + (SIM_EC_BASE    - simEc)    * 0.02;
  simWtemp += (random(-100, 101) / 100.0) * 0.15 + (SIM_WTEMP_BASE - simWtemp) * 0.02;

  /* ตอบสนองต่อการทำงานของรีเลย์ เพื่อให้ทดสอบวงจรควบคุมได้ครบ
   *   จ่ายสารปรับ pH -> ดึง pH เข้าหากลางเร็วขึ้น
   *   จ่ายปุ๋ย       -> EC เพิ่มขึ้นชัดเจน
   *   เติมอากาศ      -> อุณหภูมิน้ำลดลงเล็กน้อยจากการระเหย
   * ตั้งเกณฑ์อัตโนมัติไว้แล้วนั่งดูวงจรครบรอบได้เลยโดยไม่ต้องมีปั๊มจริง */
  if (relays[R_PH_DOSER].on) simPh    += (SIM_PH_BASE - simPh) * 0.25;
  if (relays[R_NUTRIENT].on) simEc    += 45.0;
  if (relays[R_AERATOR].on)  simWtemp -= 0.08;

  if (simPh < 4.0)  simPh = 4.0;
  if (simPh > 9.5)  simPh = 9.5;
  if (simEc < 50)   simEc = 50;
  if (simEc > 1990) simEc = 1990;
  if (simWtemp < 10) simWtemp = 10;
  if (simWtemp > 40) simWtemp = 40;

  wqPh    = simPh;
  wqEc    = simEc;
  wqTemp  = simWtemp;
  wqTds   = simEc * 0.5;     // TDS ประมาณครึ่งหนึ่งของ EC ตามความสัมพันธ์จริง
  wqSalt  = simEc * 0.55;
}

void simulateDev(uint8_t i) {
  if      (i == MB_SOIL) simulateSoil();
  else if (i == MB_RAD)  simulateRad();
  else                   simulateWq();
}

/* ====================== ตัดสินคุณภาพน้ำเป็น 3 ระดับ ======================
 * แปลงตัวเลขดิบให้เป็นคำตัดสินที่คนอ่านแล้วรู้ว่าต้องทำอะไร
 * เพราะ "pH 8.7" อย่างเดียวไม่บอกอะไรกับคนที่ไม่ได้จำช่วงที่เหมาะสมไว้
 *
 * คืนค่า 0 = ดี (GOOD) · 1 = เฝ้าระวัง (WATCH) · 2 = ผิดปกติ (BAD)
 * ผลรวมของทั้งระบบใช้ค่าที่แย่ที่สุดของทุกตัว เพราะน้ำเสียตัวเดียวก็ใช้ไม่ได้ */
uint8_t gradeValue(float v, float okLo, float okHi, float badLo, float badHi) {
  if (isnan(v))                return 1;       // ไม่มีค่า ถือว่าเฝ้าระวัง
  if (v >= okLo  && v <= okHi)  return 0;
  if (v >= badLo && v <= badHi) return 1;
  return 2;
}

uint8_t gradePh()    { return gradeValue(wqPh,   PH_OK_LO, PH_OK_HI, PH_BAD_LO, PH_BAD_HI); }
uint8_t gradeEc()    { return gradeValue(wqEc,   EC_OK_LO, EC_OK_HI, EC_BAD_LO, EC_BAD_HI); }
uint8_t gradeWtemp() { return gradeValue(wqTemp, WT_OK_LO, WT_OK_HI, WT_BAD_LO, WT_BAD_HI); }

uint8_t gradeOverall() {
  uint8_t g = gradePh();
  if (gradeEc()    > g) g = gradeEc();
  if (gradeWtemp() > g) g = gradeWtemp();
  return g;
}

// ชื่อระดับสำหรับจอ OLED (อังกฤษเท่านั้น) และสำหรับส่งขึ้น MQTT
const char *gradeName(uint8_t g) { return g == 0 ? "GOOD" : (g == 1 ? "WATCH" : "BAD"); }


/* -------------------- แปลงค่าดิบจากรีจิสเตอร์เป็นค่าจริง --------------------
 * แยกออกมาเป็นฟังก์ชันเดียว เพราะแต่ละเซนเซอร์มีตัวหารและจำนวนรีจิสเตอร์ต่างกัน
 * ค่าตัวหารทั้งหมดอ้างอิงจากคู่มือผู้ผลิต ดูรายละเอียดในหัวไฟล์ */
void decodeDev(uint8_t i) {
  if (i == MB_SOIL) {
    soil = node.getResponseBuffer(0) / SOIL_DIVISOR;
    simSoil = soil;                 // ให้ค่าจำลองเริ่มจากค่าจริง ถ้าหลุดทีหลังจะไม่กระโดด
    return;
  }

  if (i == MB_RAD) {
    rad = node.getResponseBuffer(0) / RAD_DIVISOR;   // คู่มือระบุเป็นค่าจริง ไม่ต้องหาร
    simRad = rad;
    return;
  }

  // ---- คุณภาพน้ำ : อ่านมา 5 รีจิสเตอร์ในครั้งเดียว ----
  wqPh   = node.getResponseBuffer(0) / 100.0;   // คู่มือ : หาร 100
  wqEc   = node.getResponseBuffer(1) / 10.0;    // µS/cm
  /* อุณหภูมิน้ำเป็นค่า "มีเครื่องหมาย" เพราะน้ำติดลบได้
   * คู่มือเขียนว่าเป็น 16 บิตแบบ 反码 (one's complement)
   * แต่อุปกรณ์กลุ่มนี้ในทางปฏิบัติใช้ two's complement ซึ่ง int16_t รองรับตรง ๆ
   * ค่าบวกทั้งสองแบบให้ผลเหมือนกัน ต่างกันเฉพาะค่าติดลบซึ่งต่างกัน 0.1 องศา
   * ถ้าต้องวัดน้ำติดลบจริงจัง ให้ยืนยันกับเครื่องก่อน */
  wqTemp = (int16_t)node.getResponseBuffer(2) / 10.0;
  wqTds  = node.getResponseBuffer(3) / 10.0;
  wqSalt = node.getResponseBuffer(4) / 10.0;

  simPh = wqPh; simEc = wqEc; simWtemp = wqTemp;
}

/* ------------------------- อ่านทีละตัวแบบวนรอบ -------------------------
 * ทุก MB_INTERVAL_MS จะอ่านเซนเซอร์เพียงตัวเดียว แล้วเลื่อนไปตัวถัดไปในรอบหน้า
 * ตัวที่อยู่ในโหมดจำลองจะถูกข้ามไป จนกว่าจะครบรอบลองใหม่ MB_RETRY_MS
 * (กันไม่ให้เสีย timeout 2 วินาทีกับตัวที่ไม่ได้ต่ออยู่ซ้ำ ๆ)
 *
 * ค่าจำลองของตัวที่ถูกข้าม ยังเดินต่อทุกรอบตามปกติ ผู้ใช้จึงไม่รู้สึกว่าค่าค้าง */
void taskReadModbus(uint32_t now) {
  static uint8_t turn = 0;         // ตาของใคร

  if (now - mbAt < MB_INTERVAL_MS) return;
  mbAt = now;

  // ---- เดินค่าจำลองของทุกตัวที่ยังไม่มีเซนเซอร์จริง ----
  for (uint8_t i = 0; i < MB_COUNT; i++)
    if (!mbDev[i].real) simulateDev(i);

  // ---- เลือกตัวที่จะอ่านรอบนี้ ข้ามตัวที่ยังไม่ถึงรอบลองใหม่ ----
  uint8_t pick = MB_COUNT;
  for (uint8_t k = 0; k < MB_COUNT; k++) {
    uint8_t i = (turn + k) % MB_COUNT;
    if (mbDev[i].real || (now - mbDev[i].lastAt >= MB_RETRY_MS)) { pick = i; break; }
  }
  turn = (turn + 1) % MB_COUNT;
  if (pick >= MB_COUNT) return;    // ทุกตัวอยู่ในช่วงพัก ไม่ต้องอ่านอะไร

  ModbusDev &d = mbDev[pick];
  d.lastAt = now;

  /* เปลี่ยน Slave ID ก่อนอ่านทุกครั้ง
   * ModbusMaster เก็บ ID ไว้ตัวเดียว การเรียก begin() ใหม่เป็นวิธีสลับคู่สนทนา
   * ทำงานเร็วมากเพราะแค่ตั้งค่าตัวแปรภายใน ไม่ได้เปิดพอร์ตใหม่ */
  node.begin(d.id, RS485);

  uint8_t r = node.readHoldingRegisters(d.reg, d.qty);
  d.lastErr = r;

  if (r == node.ku8MBSuccess) {
    d.okCount++;
    d.fail = 0;
    d.real = true;
    decodeDev(pick);
    node.clearResponseBuffer();
    return;
  }

  d.errCount++;
  node.clearResponseBuffer();      // ล้างทุกครั้ง กันค่าเก่าค้างแล้วเข้าใจผิดว่าอ่านได้
  if (d.fail < MAX_FAIL_STREAK) d.fail++;
  if (d.fail >= MAX_FAIL_STREAK) {
    d.real = false;
    simulateDev(pick);
  }
}


// ===================== รับข้อความที่ส่งเข้ามาจาก MQTT ======================
int parseCommand(const char *msg) {
  char m[16] = {0};
  for (uint8_t i = 0; i < sizeof(m) - 1 && msg[i]; i++) m[i] = tolower(msg[i]);
  if (!strcmp(m, "on")  || !strcmp(m, "1") || !strcmp(m, "true"))  return  1;
  if (!strcmp(m, "off") || !strcmp(m, "0") || !strcmp(m, "false")) return  0;
  if (!strcmp(m, "toggle"))                                        return -1;
  return -2;
}

void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  /* payload ไม่ใช่ string ที่จบด้วย \0 ต้องใช้ length กำหนดขอบเขตเองเสมอ */
  char msg[32] = {0};
  unsigned int n = (length < sizeof(msg) - 1) ? length : sizeof(msg) - 1;
  memcpy(msg, payload, n);

  int cmd = parseCommand(msg);
  if (cmd == -2) return;

  /* คำสั่งจาก MQTT จะสลับรีเลย์ตัวนั้นมาเป็นโหมดสั่งเองเสมอ
   * ถ้าไม่ทำแบบนี้ ตรรกะอัตโนมัติจะสั่งกลับทันทีในรอบถัดไป */
  if (!strcmp(topic, TOPIC_CMDALL)) {
    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
      cfgs[i].mode     = MODE_MANUAL;
      cfgs[i].manualOn = (cmd == 1);
    }
    taskControl();
    return;
  }

  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    if (strcmp(topic, relays[i].cmdTopic) != 0) continue;
    cfgs[i].mode     = MODE_MANUAL;
    cfgs[i].manualOn = (cmd == -1) ? !relays[i].on : (cmd == 1);
    taskControl();
    return;
  }
}

void ensureMqtt(uint32_t now) {
  if (mqtt.connected()) return;
  if (!staConnected) return;      // ไม่มีเน็ตจากบ้าน ไม่ต้องพยายามต่อ broker
  if (now - mqttRetryAt < MQTT_RETRY_MS) return;
  mqttRetryAt = now;

  if (mqtt.connect(clientId.c_str())) {
    mqtt.subscribe(TOPIC_CMDSUB);
    publishAllRelayStatus();
  }
}


// ========================= WiFi บ้าน (STA) แบบไม่บล็อก =====================
/* Lab12 รอต่อ WiFi บ้านนานถึง 20 วินาทีใน setup() ก่อนจะทำอะไรต่อ
 * Lab13 เปลี่ยนเป็นสั่ง WiFi.begin() แล้วผ่านไปเลย มาตรวจผลทีหลังใน loop()
 * บอร์ดจึงพร้อมรับมือถือผ่าน AP ได้ทันทีหลังบูต ไม่ต้องรอ WiFi บ้าน
 *
 * หมายเหตุสำคัญเรื่อง AP + STA บนวิทยุตัวเดียว
 *   ESP32 มีวงจรวิทยุชุดเดียว AP กับ STA จึงต้องใช้ช่องสัญญาณเดียวกัน
 *   เมื่อ STA ต่อเราเตอร์ได้ AP จะย้ายไปช่องเดียวกับเราเตอร์โดยอัตโนมัติ
 *   มือถือที่ต่อ AP อยู่จะหลุดแวบหนึ่งแล้วต่อกลับเอง เป็นเรื่องปกติ
 *
 *   แต่ถ้า STA "หาเราเตอร์ไม่เจอ" แล้วเราสั่งให้ลองใหม่ถี่ ๆ
 *   วิทยุจะวิ่งสแกนไล่ทุกช่องตลอดเวลา ทำให้ AP กระตุกและมือถือหลุดบ่อย
 *   จึงตั้งให้ลองใหม่ห่างถึง 5 นาที เมื่อต่อไม่ได้ */
void taskWifiSta(uint32_t now) {
#if STA_ENABLE
  if (now - staCheckAt < STA_CHECK_MS) return;
  staCheckAt = now;

  bool nowConnected = (WiFi.status() == WL_CONNECTED);

  // ---- เพิ่งต่อ WiFi บ้านได้ ----
  if (nowConnected && !staConnected) {
    staConnected     = true;
    staEverConnected = true;
    if (!mdnsStarted && MDNS.begin(MDNS_NAME)) {
      MDNS.addService("http", "tcp", 80);   // ให้เปิดได้ที่ http://smartfarm.local
      mdnsStarted = true;
    }
  }

  // ---- เพิ่งหลุดจาก WiFi บ้าน ----
  if (!nowConnected && staConnected) {
    staConnected = false;
    staRetryAt   = now;                     // เริ่มนับเวลาก่อนลองใหม่
  }

  // ---- ยังต่อไม่ได้ : ลองใหม่ตามรอบที่เหมาะกับสถานการณ์ ----
  uint32_t retryMs = staEverConnected ? STA_RETRY_FAST_MS : STA_RETRY_SLOW_MS;
  if (!nowConnected && (now - staRetryAt >= retryMs)) {
    staRetryAt = now;
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }
#endif
}


// ============================== Captive Portal ============================
/* มือถือทุกยี่ห้อจะทดสอบว่า WiFi ที่เพิ่งต่อออกเน็ตได้ไหม
 * โดยยิงคำขอไปยัง URL เฉพาะของแต่ละค่าย แล้วดูว่าได้คำตอบตามที่คาดหรือไม่
 *
 *    Android  ->  /generate_204            คาดว่าจะได้ HTTP 204
 *    iPhone   ->  /hotspot-detect.html     คาดว่าจะได้คำว่า Success
 *    Windows  ->  /connecttest.txt         คาดว่าจะได้ Microsoft Connect Test
 *
 * ถ้าเราตอบไม่ตรงที่คาด (ส่ง redirect ไปหน้า Dashboard แทน)
 * มือถือจะเข้าใจว่านี่คือ WiFi ที่ต้องล็อกอิน แล้วเด้งหน้าเว็บขึ้นมาให้เอง
 * นี่คือกลไกเดียวกับ WiFi ของโรงแรมและสนามบิน
 *
 * ส่วน DNSServer ทำหน้าที่ตอบทุกชื่อโดเมนด้วย IP ของบอร์ด
 * ไม่ว่ามือถือจะถามหา google.com หรืออะไร ก็จะถูกพามาที่บอร์ดทั้งหมด */
void redirectToRoot() {
  server.sendHeader("Location", String("http://") + AP_IP.toString() + "/", true);
  server.send(302, "text/plain", "");
}

void handleNotFound() {
  /* คำขอที่ไม่รู้จักทั้งหมดถูกส่งไปหน้า Dashboard
   * ครอบคลุม URL ทดสอบของทุกค่ายโดยไม่ต้องไล่ลงทะเบียนทีละตัว */
  redirectToRoot();
}


// ============================== Web Server ================================
/* ---- เสิร์ฟไฟล์ฟอนต์จาก PROGMEM ----
 * ส่งเป็นไฟล์แยก ไม่ฝัง base64 ไว้ใน HTML ด้วยเหตุผล 3 ข้อ
 *   1. เบราว์เซอร์แคชไว้ได้ เปิดหน้าครั้งถัดไปไม่ต้องโหลดฟอนต์ซ้ำ
 *   2. HTML หลักยังเล็กและโหลดเร็ว ฟอนต์ค่อยตามมาทีหลัง
 *   3. ส่งเป็น binary ตรง ๆ ไม่เสียพื้นที่ 33 % จากการเข้ารหัส base64
 *
 * Cache-Control ตั้งไว้ 1 ปี เพราะฟอนต์ไม่มีวันเปลี่ยน
 * ถ้าอัปเดตฟอนต์ให้เปลี่ยนชื่อไฟล์ใน @font-face เพื่อบังคับโหลดใหม่ */
void sendFont(const uint8_t *data, size_t len) {
  server.sendHeader("Cache-Control", "public, max-age=31536000, immutable");
  server.send_P(200, "font/woff2", (const char *)data, len);
}

void handleFont400Thai()  { sendFont(FONT_PROMPT_400_THAI,  FONT_PROMPT_400_THAI_LEN);  }
void handleFont400Latin() { sendFont(FONT_PROMPT_400_LATIN, FONT_PROMPT_400_LATIN_LEN); }
void handleFont700Thai()  { sendFont(FONT_PROMPT_700_THAI,  FONT_PROMPT_700_THAI_LEN);  }
void handleFont700Latin() { sendFont(FONT_PROMPT_700_LATIN, FONT_PROMPT_700_LATIN_LEN); }

void handleRoot() {
  // send_P อ่านจาก PROGMEM ส่งออกไปโดยไม่ต้องคัดลอกลง RAM ก่อน
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

void handleStatus() {
  char tbuf[24];
  timeStampFull(tbuf, sizeof(tbuf));

  String j = "{";
  j += "\"time\":\"" + String(tbuf) + "\",";
  j += "\"synced\":" + String(timeSynced ? "true" : "false") + ",";

  // ถ้าค่าเป็น NaN ต้องส่งเลข 0 แทน เพราะ NaN ทำให้ JSON เสีย
  j += "\"temp\":" + String(isnan(temp) ? 0.0 : temp, 1) + ",";
  j += "\"humi\":" + String(isnan(humi) ? 0.0 : humi, 1) + ",";
  j += "\"soil\":" + String(isnan(soil) ? 0.0 : soil, 1) + ",";

  // ธงบอกว่าค่ามาจากเซนเซอร์จริงหรือจากการจำลอง
  j += "\"dhtReal\":"  + String(dhtReal  ? "true" : "false") + ",";
  j += "\"soilReal\":" + String(mbDev[MB_SOIL].real ? "true" : "false") + ",";
  j += "\"soilOk\":"   + String(mbDev[MB_SOIL].okCount)  + ",";
  j += "\"soilErr\":"  + String(mbDev[MB_SOIL].errCount) + ",";

  // ---- เซนเซอร์ Modbus ที่เพิ่มใน Lab15 ----
  j += "\"rad\":"      + String(isnan(rad)    ? 0.0 : rad,    0) + ",";
  j += "\"ph\":"       + String(isnan(wqPh)   ? 0.0 : wqPh,   2) + ",";
  j += "\"ec\":"       + String(isnan(wqEc)   ? 0.0 : wqEc,   1) + ",";
  j += "\"wtemp\":"    + String(isnan(wqTemp) ? 0.0 : wqTemp, 1) + ",";
  j += "\"tds\":"      + String(isnan(wqTds)  ? 0.0 : wqTds,  1) + ",";
  j += "\"salt\":"     + String(isnan(wqSalt) ? 0.0 : wqSalt, 1) + ",";
  j += "\"radReal\":"  + String(mbDev[MB_RAD].real ? "true" : "false") + ",";
  j += "\"wqReal\":"   + String(mbDev[MB_WQ].real  ? "true" : "false") + ",";
  j += "\"radOk\":"    + String(mbDev[MB_RAD].okCount)  + ",";
  j += "\"radErr\":"   + String(mbDev[MB_RAD].errCount) + ",";
  j += "\"wqOk\":"     + String(mbDev[MB_WQ].okCount)   + ",";
  j += "\"wqErr\":"    + String(mbDev[MB_WQ].errCount)  + ",";

  /* รหัสข้อผิดพลาดล่าสุดของแต่ละตัว ส่งเป็นตัวเลขแล้วให้หน้าเว็บแปลเป็นข้อความ
   * ช่วยไล่ปัญหาตอนเสียบเซนเซอร์จริงได้ทันทีโดยไม่ต้องต่อ USB-TTL มาดู Serial
   *   0xE2 = ไม่มีคำตอบเลย  -> ID ผิด / ไฟไม่เข้า / สายหลุด
   *   0xE3 = ข้อมูลเพี้ยน   -> baud ไม่ตรง / สลับสาย A-B
   *   0x02 = ไม่มีรีจิสเตอร์ -> อ่านผิดตำแหน่ง */
  /* คำตัดสินคุณภาพน้ำ และเกณฑ์ที่ใช้ตัดสิน
   * ส่งเกณฑ์ไปด้วยเพื่อให้หน้าเว็บวาดสเกลและแถบสีได้ตรงกับที่โค้ดใช้จริง
   * ถ้าให้หน้าเว็บฮาร์ดโค้ดเกณฑ์เอง แก้ในโค้ดแล้วหน้าเว็บจะไม่ตรงกัน */
  j += "\"grade\":"    + String(gradeOverall()) + ",";
  j += "\"gPh\":"      + String(gradePh())      + ",";
  j += "\"gEc\":"      + String(gradeEc())      + ",";
  j += "\"gWt\":"      + String(gradeWtemp())   + ",";
  j += "\"lim\":{";
  j += "\"phOk\":["   + String(PH_OK_LO,1)  + "," + String(PH_OK_HI,1)  + "],";
  j += "\"phBad\":["  + String(PH_BAD_LO,1) + "," + String(PH_BAD_HI,1) + "],";
  j += "\"ecOk\":["   + String(EC_OK_LO,0)  + "," + String(EC_OK_HI,0)  + "],";
  j += "\"ecBad\":["  + String(EC_BAD_LO,0) + "," + String(EC_BAD_HI,0) + "],";
  j += "\"wtOk\":["   + String(WT_OK_LO,0)  + "," + String(WT_OK_HI,0)  + "],";
  j += "\"wtBad\":["  + String(WT_BAD_LO,0) + "," + String(WT_BAD_HI,0) + "]},";
  j += "\"roles\":[\"" + String(RELAY_ROLE_TH[0]) + "\",\""
                         + String(RELAY_ROLE_TH[1]) + "\",\""
                         + String(RELAY_ROLE_TH[2]) + "\"],";

  j += "\"soilErrCode\":" + String(mbDev[MB_SOIL].lastErr) + ",";
  j += "\"radErrCode\":"  + String(mbDev[MB_RAD].lastErr)  + ",";
  j += "\"wqErrCode\":"   + String(mbDev[MB_WQ].lastErr)   + ",";


  // ---- เครือข่าย : แยก AP ของบอร์ด กับ STA ที่ต่อบ้าน ----
  j += "\"wifi\":" + String(staConnected ? "true" : "false") + ",";
  j += "\"mqtt\":" + String(mqtt.connected() ? "true" : "false") + ",";
  j += "\"ip\":\"" + (staConnected ? WiFi.localIP().toString() : String("-")) + "\",";
  j += "\"rssi\":" + String(staConnected ? WiFi.RSSI() : 0) + ",";
  j += "\"apSsid\":\"" + apSsid + "\",";
  j += "\"apIp\":\"" + WiFi.softAPIP().toString() + "\",";
  j += "\"apClients\":" + String(WiFi.softAPgetStationNum()) + ",";
  j += "\"staSsid\":\"" + String(STA_ENABLE ? WIFI_SSID : "") + "\",";
  j += "\"mdns\":\"" + String(mdnsStarted ? MDNS_NAME : "") + "\",";
  j += "\"uptime\":" + String(millis() / 1000) + ",";
  j += "\"tx\":" + String(pubCount) + ",";

  j += "\"relays\":[";
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    RelayCfg &c = cfgs[i];
    if (i) j += ",";
    j += "{\"id\":"     + String(i + 1);
    j += ",\"on\":"     + String(relays[i].on ? "true" : "false");
    j += ",\"mode\":"   + String(c.mode);
    j += ",\"onH\":"    + String(c.onHour);
    j += ",\"onM\":"    + String(c.onMin);
    j += ",\"runMin\":" + String(c.runMin);
    j += ",\"src\":"    + String(c.src);
    j += ",\"above\":"  + String(c.above ? "true" : "false");
    j += ",\"thr\":"    + String(c.thr, 1);
    j += ",\"hyst\":"   + String(c.hyst, 1);
    j += ",\"remain\":" + String(relayRemainSec(i));
    j += "}";
  }
  j += "],";

  j += "\"mq\":{";
  j += "\"host\":\""     + String(MQTT_HOST) + "\",";
  j += "\"port\":"       + String(MQTT_PORT) + ",";
  j += "\"clientId\":\"" + clientId + "\",";
  j += "\"pub\":[";
  j += "\"" + String(TOPIC_TEMP)   + "\",";
  j += "\"" + String(TOPIC_HUMI)   + "\",";
  j += "\"" + String(TOPIC_SOIL)   + "\",";
  j += "\"" + String(TOPIC_RAD)    + "\",";
  j += "\"" + String(TOPIC_PH)     + "\",";
  j += "\"" + String(TOPIC_EC)     + "\",";
  j += "\"" + String(TOPIC_WTEMP)  + "\",";
  j += "\"" + String(TOPIC_TDS)    + "\",";
  j += "\"" + String(TOPIC_SALT)   + "\",";
  j += "\"" + String(TOPIC_VERDICT)+ "\",";
  j += "\"" + String(TOPIC_STATUS) + "\",";
  j += "\"" + String(TOPIC_ST_R1)  + "\",";
  j += "\"" + String(TOPIC_ST_R2)  + "\",";
  j += "\"" + String(TOPIC_ST_R3)  + "\"],";
  j += "\"sub\":[";
  j += "\"" + String(TOPIC_CMD_R1) + "\",";
  j += "\"" + String(TOPIC_CMD_R2) + "\",";
  j += "\"" + String(TOPIC_CMD_R3) + "\",";
  j += "\"" + String(TOPIC_CMDALL) + "\"]";
  j += "}}";

  server.send(200, "application/json", j);
}

void handleRelayCmd() {
  int id = server.arg("id").toInt();
  if (id < 1 || id > RELAY_COUNT) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  String s = server.arg("state");
  RelayCfg &c = cfgs[id - 1];
  c.mode = MODE_MANUAL;

  if      (s == "on")  c.manualOn = true;
  else if (s == "off") c.manualOn = false;
  else                 c.manualOn = !relays[id - 1].on;

  taskControl();                     // สั่งงานทันที ไม่ต้องรอรอบถัดไป
  server.send(200, "application/json", "{\"ok\":true}");
}

/* ทุกค่าจากหน้าเว็บถูกจำกัดขอบเขตด้วย constrain ก่อนใช้เสมอ
 * เพราะเป็นข้อมูลที่ผู้ใช้แก้ไขได้ ห้ามเชื่อโดยไม่ตรวจ */
void handleConfig() {
  int id = server.arg("id").toInt();
  if (id < 1 || id > RELAY_COUNT) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }

  RelayCfg &c = cfgs[id - 1];

  if (server.hasArg("mode"))   c.mode   = constrain(server.arg("mode").toInt(), 0, 2);
  if (server.hasArg("onH"))    c.onHour = constrain(server.arg("onH").toInt(), 0, 23);
  if (server.hasArg("onM"))    c.onMin  = constrain(server.arg("onM").toInt(), 0, 59);
  if (server.hasArg("runMin")) c.runMin = constrain(server.arg("runMin").toInt(), 1, 720);
  if (server.hasArg("src"))    c.src    = constrain(server.arg("src").toInt(), 0, SRC_MAX);
  if (server.hasArg("above"))  c.above  = (server.arg("above").toInt() == 1);
  if (server.hasArg("thr"))    c.thr    = constrain(server.arg("thr").toFloat(), -20.0f, 120.0f);
  if (server.hasArg("hyst"))   c.hyst   = constrain(server.arg("hyst").toFloat(), 0.5f, 20.0f);

  /* เปลี่ยนโหมดแล้วต้องล้าง lastDay ด้วย
   * ไม่งั้นถ้าสลับไปโหมดอื่นแล้วกลับมาโหมดตั้งเวลาในวันเดียวกัน
   * ระบบจะจำว่าวันนี้ทำไปแล้วและไม่ยอมทำงานจนกว่าจะข้ามวัน */
  if (server.hasArg("mode")) relays[id - 1].lastDay = -1;

  saveConfig();
  taskControl();
  server.send(200, "application/json", "{\"ok\":true}");
}


// ========================= งานที่ 3 : ส่งขึ้น MQTT =========================
void taskPublish(uint32_t now) {
  if (now - pubAt < PUBLISH_INTERVAL) return;
  pubAt = now;
  if (!mqtt.connected()) return;

  publishAllRelayStatus();

  /* bufJson ขยายเป็น 448 เพราะ Lab15 มีฟิลด์เพิ่มอีก 6 ตัว
   * ยังอยู่ในบัฟเฟอร์ 512 ของ PubSubClient ที่ตั้งไว้ใน setup() */
  char buf[16], bufTime[24], bufJson[448];
  timeStampFull(bufTime, sizeof(bufTime));

  if (!isnan(temp))   { snprintf(buf, sizeof(buf), "%.1f", temp);   mqtt.publish(TOPIC_TEMP,  buf); }
  if (!isnan(humi))   { snprintf(buf, sizeof(buf), "%.1f", humi);   mqtt.publish(TOPIC_HUMI,  buf); }
  if (!isnan(soil))   { snprintf(buf, sizeof(buf), "%.1f", soil);   mqtt.publish(TOPIC_SOIL,  buf); }
  if (!isnan(rad))    { snprintf(buf, sizeof(buf), "%.0f", rad);    mqtt.publish(TOPIC_RAD,   buf); }
  if (!isnan(wqPh))   { snprintf(buf, sizeof(buf), "%.2f", wqPh);   mqtt.publish(TOPIC_PH,    buf); }
  if (!isnan(wqEc))   { snprintf(buf, sizeof(buf), "%.1f", wqEc);   mqtt.publish(TOPIC_EC,    buf); }
  if (!isnan(wqTemp)) { snprintf(buf, sizeof(buf), "%.1f", wqTemp); mqtt.publish(TOPIC_WTEMP, buf); }
  if (!isnan(wqTds))  { snprintf(buf, sizeof(buf), "%.1f", wqTds);  mqtt.publish(TOPIC_TDS,   buf); }
  if (!isnan(wqSalt)) { snprintf(buf, sizeof(buf), "%.1f", wqSalt); mqtt.publish(TOPIC_SALT,  buf); }

  /* ส่งคำตัดสินเป็นข้อความแบบ retained
   * ระบบปลายทางจะได้ตั้งการแจ้งเตือนจาก "BAD" ได้ตรง ๆ
   * ไม่ต้องไปคำนวณช่วงค่าซ้ำเองซึ่งอาจใช้เกณฑ์ไม่ตรงกับบอร์ด */
  mqtt.publish(TOPIC_VERDICT, gradeName(gradeOverall()), true);

  /* ส่งธง real/sim ไปกับข้อมูลด้วย
   * ฝั่งที่เก็บข้อมูลจะได้แยกออกว่าค่าไหนวัดจริง ค่าไหนเป็นค่าจำลองตอนทดสอบ
   * ถ้าไม่ส่งไป ข้อมูลทดสอบจะปนกับข้อมูลจริงในฐานข้อมูลโดยแยกไม่ออก
   */
  snprintf(bufJson, sizeof(bufJson),
           "{\"time\":\"%s\",\"temp\":%.1f,\"humi\":%.1f,\"soil\":%.1f,"
           "\"rad\":%.0f,\"ph\":%.2f,\"ec\":%.1f,\"wtemp\":%.1f,\"tds\":%.1f,"
           "\"salt\":%.1f,\"verdict\":\"%s\","
           "\"dhtSrc\":\"%s\",\"soilSrc\":\"%s\",\"radSrc\":\"%s\",\"wqSrc\":\"%s\","
           "\"r1\":%d,\"r2\":%d,\"r3\":%d,\"rssi\":%d,\"uptime\":%u}",
           bufTime,
           isnan(temp)   ? 0.0 : temp,
           isnan(humi)   ? 0.0 : humi,
           isnan(soil)   ? 0.0 : soil,
           isnan(rad)    ? 0.0 : rad,
           isnan(wqPh)   ? 0.0 : wqPh,
           isnan(wqEc)   ? 0.0 : wqEc,
           isnan(wqTemp) ? 0.0 : wqTemp,
           isnan(wqTds)  ? 0.0 : wqTds,
           isnan(wqSalt) ? 0.0 : wqSalt,
           gradeName(gradeOverall()),
           dhtReal                ? "real" : "sim",
           mbDev[MB_SOIL].real    ? "real" : "sim",
           mbDev[MB_RAD].real     ? "real" : "sim",
           mbDev[MB_WQ].real      ? "real" : "sim",
           relays[0].on ? 1 : 0, relays[1].on ? 1 : 0, relays[2].on ? 1 : 0,
           WiFi.RSSI(), now / 1000);

  if (mqtt.publish(TOPIC_STATUS, bufJson)) pubCount++;
}


// ================== งานที่ 4 : หน้าจอ OLED แบบหลายหน้า ====================
/* ผังหน้าจอมาตรฐาน 128x64 ใช้เหมือนกันทุกหน้า
 *
 *   y 0..10   แถบหัว  พื้นทึบ ตัวอักษรกลับสี  ชื่อหน้า + คำตัดสิน/เลขหน้า
 *   y 13..50  เนื้อหา ของแต่ละหน้า
 *   y 53..63  แถบล่าง จุดบอกหน้า หรือข้อความเตือน
 *
 * เมตริกฟอนต์ Adafruit GFX
 *   size 1 = 6x8 px   ใส่ได้ 21 ตัวอักษรต่อบรรทัด
 *   size 2 = 12x16 px ใส่ได้ 10 ตัวอักษรต่อบรรทัด
 * ทุกข้อความคำนวณความกว้างไว้แล้วไม่ให้ล้นขอบจอ */

/* ---- แถบหัว ----
 * มุมขวาแสดง "คำตัดสินคุณภาพน้ำ" แทนเลขหน้า เพราะเป็นข้อมูลที่สำคัญที่สุด
 * ของโปรเจกต์นี้ และต้องเห็นได้ทุกหน้าไม่ต้องรอหมุน
 *   GOOD  = กรอบปกติ
 *   WATCH = กรอบปกติ
 *   BAD   = กลับสีและกะพริบ เพื่อดึงสายตาคนที่เดินผ่านตู้ */
void uiHeader(uint8_t page, uint32_t now) {
  uint8_t g = gradeOverall();
  const char *gn = gradeName(g);
  int16_t gw = strlen(gn) * 6 + 4;          // GOOD/WATCH/BAD ยาวสุด 5 ตัว = 34 px

  oled.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);

  oled.setCursor(2, 2);
  oled.print(PG_TITLE[page]);

  if (g == 2 && (now / 400) % 2) {          // ผิดปกติ -> กลับสีกะพริบ
    oled.fillRect(128 - gw, 0, gw, 11, SSD1306_BLACK);
    oled.setTextColor(SSD1306_WHITE);
  }
  oled.setCursor(128 - gw + 2, 2);
  oled.print(gn);

  oled.setTextColor(SSD1306_WHITE);         // คืนค่าให้เนื้อหาด้านล่าง
}

/* ---- แถบล่าง ----
 * จุดทึบคือหน้าปัจจุบัน จุดกลวงคือหน้าอื่น
 * ถ้าปิดการหมุนอัตโนมัติ มีวงเล็บครอบเพื่อบอกว่าล็อกหน้าอยู่
 * ถ้ามีเรื่องเตือน แทนที่จุดด้วยข้อความเตือนที่กะพริบ */
void uiFooter(uint8_t page, uint32_t now, bool alert, const char *alertMsg) {
  if (alert) {
    if ((now / 500) % 2) {
      oled.setTextSize(1);
      int16_t w = strlen(alertMsg) * 6;
      oled.setCursor((128 - w) / 2, 55);    // จัดกลางจอ
      oled.print(alertMsg);
    }
    return;
  }

  const int16_t x0 = (128 - (PG_COUNT - 1) * 10 - 3) / 2;
  for (uint8_t i = 0; i < PG_COUNT; i++) {
    int16_t cx = x0 + i * 10;
    if (i == page) oled.fillCircle(cx, 58, 2, SSD1306_WHITE);
    else           oled.drawCircle(cx, 58, 1, SSD1306_WHITE);
  }
  if (!uiAutoScan) {
    oled.drawChar(x0 + page * 10 - 8, 55, '[', SSD1306_WHITE, SSD1306_BLACK, 1);
    oled.drawChar(x0 + page * 10 + 4, 55, ']', SSD1306_WHITE, SSD1306_BLACK, 1);
  }
}

/* ---- สเกลแสดงตำแหน่งค่าเทียบช่วงที่เหมาะสม ----
 * นี่คือหัวใจการแสดงผลของโปรเจกต์นี้
 *
 * ตัวเลขเปล่าไม่บอกว่าดีหรือแย่ ต้องจำช่วงที่เหมาะสมไว้เองจึงจะตีความได้
 * สเกลนี้วาดช่วง OK เป็นกรอบทึบ แล้วปักลูกศรชี้ตำแหน่งค่าปัจจุบัน
 * มองแวบเดียวรู้ทันทีว่าอยู่ในช่วง สูงเกิน หรือต่ำเกิน โดยไม่ต้องอ่านเลข
 *
 *   lo..hi     = ขอบซ้าย-ขวาของสเกลทั้งเส้น
 *   okLo..okHi = ช่วงที่ต้องการ วาดเป็นแถบทึบ
 *   v          = ค่าปัจจุบัน วาดเป็นลูกศรสามเหลี่ยมชี้ลง */
void uiScale(int16_t y, float v, float lo, float hi, float okLo, float okHi) {
  const int16_t X0 = 4, W = 120;            // เว้นขอบข้างละ 4 px ให้ลูกศรไม่ตกขอบ

  // เส้นสเกลและขีดปลายทั้งสองข้าง
  oled.drawFastHLine(X0, y + 6, W, SSD1306_WHITE);
  oled.drawFastVLine(X0, y + 4, 5, SSD1306_WHITE);
  oled.drawFastVLine(X0 + W - 1, y + 4, 5, SSD1306_WHITE);

  // แถบช่วงที่เหมาะสม
  int16_t a = X0 + (int16_t)((okLo - lo) / (hi - lo) * W);
  int16_t b = X0 + (int16_t)((okHi - lo) / (hi - lo) * W);
  if (a < X0) a = X0;
  if (b > X0 + W - 1) b = X0 + W - 1;
  if (b > a) oled.fillRect(a, y + 4, b - a, 5, SSD1306_WHITE);

  // ลูกศรชี้ตำแหน่งค่าปัจจุบัน
  if (isnan(v)) return;
  float t = (v - lo) / (hi - lo);
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  int16_t x = X0 + (int16_t)(t * (W - 1));
  oled.fillTriangle(x, y + 3, x - 3, y - 1, x + 3, y - 1, SSD1306_WHITE);
}

// ---- แถบระดับแนวนอนธรรมดา ใช้กับค่าที่มีช่วง 0..100 ----
void uiBar(int16_t y, float pct) {
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  oled.drawRect(0, y, 128, 7, SSD1306_WHITE);
  int16_t w = (int16_t)(pct * 1.24);
  if (w > 0) oled.fillRect(2, y + 2, w, 3, SSD1306_WHITE);
}

// ---- ป้ายบอกที่มาของค่า : REAL = กรอบบาง · SIM = พื้นทึบ (เด่นกว่าโดยตั้งใจ) ----
void uiSrcTag(int16_t y, bool real) {
  const char *t = real ? "REAL" : "SIM";
  int16_t w = strlen(t) * 6 + 4;
  int16_t x = 128 - w;
  oled.setTextSize(1);
  if (real) {
    oled.drawRect(x, y, w, 11, SSD1306_WHITE);
    oled.setCursor(x + 2, y + 2);
    oled.print(t);
  } else {
    oled.fillRect(x, y, w, 11, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
    oled.setCursor(x + 2, y + 2);
    oled.print(t);
    oled.setTextColor(SSD1306_WHITE);
  }
}

// ---- คู่ ชื่อ-ค่า ค่าชิดขวาเสมอ อ่านเทียบกันง่าย ----
void uiKV(int16_t y, const char *k, const char *v) {
  oled.setTextSize(1);
  oled.setCursor(0, y);
  oled.print(k);
  int16_t w = strlen(v) * 6;
  oled.setCursor(128 - w, y);
  oled.print(v);
}

/* ---- ย่อตัวเลขนับให้ยาวไม่เกิน 4 ตัวอักษร ----
 * ตัวนับ ok/err โตขึ้นเรื่อย ๆ เมื่อระบบเดินยาวหลายวัน
 * เกิน 9999 ย่อเป็น "9k+" เพราะจำนวนที่แน่นอนไม่สำคัญเท่าการรู้ว่ามันเยอะ */
void fmtCnt(char *b, size_t n, uint32_t v) {
  if (v > 9999) snprintf(b, n, "9k+");
  else          snprintf(b, n, "%lu", (unsigned long)v);
}


// ============================== เนื้อหาแต่ละหน้า ============================

/* หน้าสรุป : ค่าหลักทั้ง 3 ตัวของคุณภาพน้ำในหน้าเดียว
 * เป็นหน้าที่คนดูบ่อยที่สุด จึงวาง pH เป็นตัวใหญ่สุดและ EC/อุณหภูมิเป็นตัวรอง
 * เครื่องหมายหลังแต่ละค่าบอกระดับของค่านั้นเอง ไม่ใช่ของทั้งระบบ */
void pgOverview(uint32_t now) {
  char b[16];

  oled.setTextSize(2);
  oled.setCursor(0, 14);
  if (isnan(wqPh)) oled.print("pH --.--");
  else             oled.printf("pH %.2f", wqPh);

  // เครื่องหมายระดับของ pH วางท้ายบรรทัด
  oled.setTextSize(1);
  oled.setCursor(112, 18);
  oled.print(gradePh() == 0 ? "OK" : (gradePh() == 1 ? "!?" : "!!"));

  snprintf(b, sizeof(b), "%.0f uS %s", isnan(wqEc) ? 0 : wqEc,
           gradeEc() == 0 ? "OK" : (gradeEc() == 1 ? "!?" : "!!"));
  uiKV(34, "EC", b);

  snprintf(b, sizeof(b), "%.1f C %s", isnan(wqTemp) ? 0 : wqTemp,
           gradeWtemp() == 0 ? "OK" : (gradeWtemp() == 1 ? "!?" : "!!"));
  uiKV(44, "Water", b);
}

// หน้า pH แบบละเอียด : ตัวเลขใหญ่ + สเกล 0-14 พร้อมช่วงที่เหมาะสม
void pgPh(uint32_t now) {
  oled.setTextSize(2);
  oled.setCursor(0, 14);
  if (isnan(wqPh)) oled.print("--.--");
  else             oled.printf("%.2f", wqPh);

  uiSrcTag(14, mbDev[MB_WQ].real);

  // สเกล pH 0-14 ซึ่งเป็นช่วงวัดของเซนเซอร์รุ่นนี้
  uiScale(38, wqPh, 0, 14, PH_OK_LO, PH_OK_HI);

  oled.setTextSize(1);
  oled.setCursor(0, 46);
  oled.print("0");
  oled.setCursor(52, 46);
  oled.printf("ok %.1f-%.1f", PH_OK_LO, PH_OK_HI);
  oled.setCursor(116, 46);
  oled.print("14");
}

// หน้า EC / TDS / ความเค็ม
void pgEc(uint32_t now) {
  char b[16];

  oled.setTextSize(2);
  oled.setCursor(0, 14);
  if (isnan(wqEc)) oled.print("---- uS");
  else             oled.printf("%.0f uS", wqEc);

  uiScale(36, wqEc, 0, 2000, EC_OK_LO, EC_OK_HI);

  oled.setTextSize(1);
  snprintf(b, sizeof(b), "%.0f / %.0f", isnan(wqTds) ? 0 : wqTds,
           isnan(wqSalt) ? 0 : wqSalt);
  uiKV(46, "TDS/Salt", b);
}

// หน้าอุณหภูมิ : เทียบน้ำกับอากาศในหน้าเดียว เพราะสัมพันธ์กัน
void pgTemp(uint32_t now) {
  char b[16];

  oled.setTextSize(2);
  oled.setCursor(0, 14);
  if (isnan(wqTemp)) oled.print("--.- C");
  else               oled.printf("%.1f C", wqTemp);

  oled.setTextSize(1);
  oled.setCursor(0, 32);
  oled.print("water");

  uiScale(44, wqTemp, 10, 40, WT_OK_LO, WT_OK_HI);

  /* อุณหภูมิอากาศวางที่ x=54 ไม่ใช่ 72
   * เพราะค่าติดลบ เช่น "air -10.5C" ยาว 10 ตัวอักษร = 60 px
   * ถ้าเริ่มที่ 72 จะล้นขอบจอ (72+60 = 132 > 128) */
  oled.setCursor(54, 32);
  oled.printf("air %.1fC", isnan(temp) ? 0 : temp);
}

/* หน้าสภาพแวดล้อม : ค่าประกอบทั้งหมดรวมไว้หน้าเดียว
 * ไม่ใช่ตัวหลักของโปรเจกต์ จึงไม่ต้องมีหน้าแยกของตัวเอง */
void pgEnv(uint32_t now) {
  char b[16];

  snprintf(b, sizeof(b), "%.0f %%", isnan(soil) ? 0 : soil);
  uiKV(15, "Soil moisture", b);

  snprintf(b, sizeof(b), "%.0f W/m2", isnan(rad) ? 0 : rad);
  uiKV(27, "Solar", b);

  snprintf(b, sizeof(b), "%.1f C", isnan(temp) ? 0 : temp);
  uiKV(39, "Air temp", b);

  snprintf(b, sizeof(b), "%.0f %%RH", isnan(humi) ? 0 : humi);
  uiKV(47, "Air humidity", b);
}

/* หน้าชุดปรับคุณภาพน้ำ : รีเลย์ 3 ตัวพร้อมบทบาทจริง
 * แสดงชื่อบทบาทแทน R1/R2/R3 เพราะคนหน้าตู้สนใจว่า "ปั๊มอะไรทำงาน"
 * ไม่ใช่ "รีเลย์เบอร์อะไรทำงาน" */
void pgDosing(uint32_t now) {
  oled.setTextSize(1);
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    int16_t y = 14 + i * 12;

    oled.setCursor(0, y + 2);
    oled.print(RELAY_ROLE[i]);            // pH Doser / Nutrient / Aerator

    // ON = พื้นทึบ · OFF = กรอบบาง กวาดตาผ่านแล้วรู้ทันทีว่าตัวไหนทำงาน
    if (relays[i].on) {
      oled.fillRect(56, y, 22, 11, SSD1306_WHITE);
      oled.setTextColor(SSD1306_BLACK);
      oled.setCursor(59, y + 2);
      oled.print("ON");
      oled.setTextColor(SSD1306_WHITE);
    } else {
      oled.drawRect(56, y, 26, 11, SSD1306_WHITE);
      oled.setCursor(59, y + 2);
      oled.print("OFF");
    }

    // โหมด + เวลาที่เหลือของรอบ
    const char *m = (cfgs[i].mode == MODE_MANUAL)   ? "MAN"
                  : (cfgs[i].mode == MODE_SCHEDULE) ? "SCH" : "AUT";
    oled.setCursor(88, y + 2);
    uint32_t rem = relayRemainSec(i);
    if (rem > 0) {
      if (rem > 99) oled.printf("%lum", (unsigned long)((rem + 59) / 60));
      else          oled.printf("%lus", (unsigned long)rem);
    } else {
      oled.print(m);
    }
  }
}

// หน้าสถานะเซนเซอร์ : ดูว่าตัวไหนอ่านได้จริงและสายมีปัญหาหรือไม่
void pgSensor(uint32_t now) {
  oled.setTextSize(1);
  for (uint8_t i = 0; i < MB_COUNT; i++) {
    int16_t y = 14 + i * 12;
    char bo[8], be[8];
    fmtCnt(bo, sizeof(bo), mbDev[i].okCount);
    fmtCnt(be, sizeof(be), mbDev[i].errCount);

    oled.setCursor(0, y);
    oled.printf("ID%u %-4s", mbDev[i].id, mbDev[i].name);

    if (mbDev[i].real) {
      /* ใช้รูปย่อ "ok9k+ e9k+" = 10 ตัวอักษร = 60 px
       * แบบเต็ม "ok 9k+ er 9k+" ยาว 78 px จะล้นขอบเมื่อเริ่มที่ x=54 */
      oled.setCursor(54, y);
      oled.printf("ok%s e%s", bo, be);
    } else {
      // ไม่ได้ต่ออยู่ -> กลับสีให้เห็นชัดว่าเป็นค่าจำลอง
      oled.fillRect(54, y - 1, 24, 10, SSD1306_WHITE);
      oled.setTextColor(SSD1306_BLACK);
      oled.setCursor(56, y);
      oled.print("SIM");
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(82, y);
      oled.printf("e%s", be);
    }
  }
  oled.setCursor(0, 50);
  oled.printf("DHT11 %s", dhtReal ? "REAL" : "SIM");
}

// หน้าเครือข่าย
void pgNet(uint32_t now) {
  oled.setTextSize(1);

  oled.setCursor(0, 14);
  oled.print("AP");
  oled.setCursor(18, 14);
  oled.print(apSsid);
  oled.setCursor(18, 24);
  oled.printf("%u client", WiFi.softAPgetStationNum());

  oled.setCursor(0, 36);
  oled.print("IP");
  oled.setCursor(18, 36);
  if (staConnected) oled.print(WiFi.localIP().toString());
  else              oled.print("no home wifi");

  oled.setCursor(0, 46);
  oled.printf("MQTT %s  Tx %lu",
              mqtt.connected() ? "on" : "off", (unsigned long)pubCount);
}


// ======================= อ่านปุ่มและจัดการการเลื่อนหน้า =====================
/* debounce แบบไม่บล็อก : ค่าต้องนิ่งครบ BTN_DEBOUNCE_MS จึงยอมรับว่ากดจริง
 * แล้วยกธง pressed ขึ้นเฉพาะขอบขาลง ทำให้กดค้างไว้ก็เลื่อนแค่หน้าเดียว */
void taskButtons(uint32_t now) {
  for (uint8_t i = 0; i < 3; i++) {
    UiButton &b = uiBtn[i];
    bool raw = (digitalRead(b.pin) == LOW);      // ปุ่มบนบอร์ดเป็น Active LOW

    if (raw != b.lastRaw) {
      b.lastRaw   = raw;
      b.changedAt = now;
      continue;
    }
    if (raw != b.stable && (now - b.changedAt) >= BTN_DEBOUNCE_MS) {
      b.stable = raw;
      if (b.stable) b.pressed = true;
    }
  }

  if (uiBtn[0].pressed) {                        // SW1 หน้าก่อนหน้า
    uiBtn[0].pressed = false;
    uiPage   = (uiPage + PG_COUNT - 1) % PG_COUNT;
    uiHoldAt = now;
    oledAt   = 0;                                // วาดจอใหม่ทันที ไม่รอรอบ
  }
  if (uiBtn[1].pressed) {                        // SW2 หน้าถัดไป
    uiBtn[1].pressed = false;
    uiPage   = (uiPage + 1) % PG_COUNT;
    uiHoldAt = now;
    oledAt   = 0;
  }
  if (uiBtn[2].pressed) {                        // SW3 ล็อก/ปลดล็อกการหมุนหน้า
    uiBtn[2].pressed = false;
    uiAutoScan = !uiAutoScan;
    uiRotateAt = now;
    oledAt     = 0;
  }
}


// ============================ วาดหน้าจอทั้งหมด =============================
void taskDisplay(uint32_t now) {
  /* หมุนหน้าอัตโนมัติ แต่หยุดชั่วคราวหลังผู้ใช้กดปุ่ม
   * เพื่อให้คนที่กำลังอ่านหน้านั้นมีเวลาอ่านจบ ไม่ถูกเลื่อนหนีไปเอง */
  if (uiAutoScan && (now - uiHoldAt >= PAGE_HOLD_MS)
                 && (now - uiRotateAt >= PAGE_ROTATE_MS)) {
    uiRotateAt = now;
    uiPage = (uiPage + 1) % PG_COUNT;
  }

  if (now - oledAt < OLED_INTERVAL_MS) return;
  oledAt = now;

  /* เรื่องที่ต้องเตือน เรียงตามความสำคัญของโปรเจกต์นี้
   * คุณภาพน้ำผิดปกติมาก่อนทุกอย่าง เพราะเป็นหน้าที่หลักของระบบ */
  bool alert = false;
  const char *alertMsg = "";
  uint8_t g = gradeOverall();

  if (g == 2) {
    alert = true;
    if      (gradePh()    == 2) alertMsg = "pH OUT OF RANGE";
    else if (gradeEc()    == 2) alertMsg = "EC OUT OF RANGE";
    else                        alertMsg = "WATER TEMP ALARM";
  } else if (!mbDev[MB_WQ].real) {
    alert = true;  alertMsg = "NO WQ SENSOR (ID3)";
  } else if (!staConnected && WiFi.softAPgetStationNum() == 0) {
    alert = true;  alertMsg = "AP ready - no client";
  }

  oled.clearDisplay();
  uiHeader(uiPage, now);

  switch (uiPage) {
    case PG_OVERVIEW: pgOverview(now); break;
    case PG_PH:       pgPh(now);       break;
    case PG_EC:       pgEc(now);       break;
    case PG_TEMP:     pgTemp(now);     break;
    case PG_ENV:      pgEnv(now);      break;
    case PG_DOSING:   pgDosing(now);   break;
    case PG_SENSOR:   pgSensor(now);   break;
    default:          pgNet(now);      break;
  }

  uiFooter(uiPage, now, alert, alertMsg);
  oled.display();
}


// แสดงข้อความตอนบูตบนจอ แทนการใช้ Serial.println ที่ใช้ไม่ได้บนบอร์ดนี้
void bootMsg(const char *line1, const char *line2) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("WATER QUALITY");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  oled.setCursor(0, 22);
  oled.println(line1);
  if (line2) { oled.setCursor(0, 34); oled.println(line2); }
  oled.display();
}


// ================================ SETUP ===================================
void setup() {
  /* บน ESP32 core 3.x ต้อง pinMode(OUTPUT) ก่อน แล้วค่อย digitalWrite()
   * ถ้าสลับลำดับ digitalWrite() จะไม่ทำงานเลย และรีเลย์ Active LOW จะติดตอนบูต */
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    pinMode(relays[i].pin, OUTPUT);
    digitalWrite(relays[i].pin, RELAY_OFF);   // RELAY_OFF = HIGH
    relays[i].on = false;
  }
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);


  /* ปุ่มนำทางหน้าจอ SW1-SW3
   * ใช้ INPUT เปล่าทั้งสามตัว เพราะบอร์ดมี R pull-up 10k ภายนอกให้แล้ว
   * และ GPIO34/35 เป็นขา input-only ที่ไม่มี pull-up ภายในให้ใช้อยู่แล้ว */
  pinMode(SW1_PIN, INPUT);
  pinMode(SW2_PIN, INPUT);
  pinMode(SW3_PIN, INPUT);

  /* เปิด UART0 ที่ 4800 สำหรับ Modbus เท่านั้น
   * หลังบรรทัดนี้ห้ามมีคำสั่ง Serial.print() ที่ใดในโปรแกรมอีก */
  RS485.begin(RS485_BAUDRATE, SERIAL_8N1, 3, 1);
  node.begin(SOIL_SLAVE_ID, RS485);
  node.postTransmission(postTransmission);

  randomSeed(esp_random());     // ให้ค่าจำลองไม่ซ้ำเดิมทุกครั้งที่บูต

  // ---------- จอ OLED ----------
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // จอไม่ตอบสนอง กะพริบ LED เตือนแล้วทำงานต่อ ระบบต้องไม่หยุดเพราะจอเสีย
    for (uint8_t i = 0; i < 6; i++) { digitalWrite(LED_PIN, i % 2); delay(150); }
  }
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextWrap(false);

  /* ---------- โหมดตั้งค่าเซนเซอร์ ----------
   * วางไว้หลังเปิดจอ OLED แต่ก่อน WiFi/MQTT ทั้งหมด
   * เพราะโหมดนี้ต้องการแค่ RS-485 กับจอ ไม่ต้องใช้เครือข่ายเลย
   * และฟังก์ชันนี้ไม่คืนค่า จบด้วยการค้างหน้าจอ */
#if COMMISSION_MODE != 0
  commissionRun();
#endif

  bootMsg("Loading config...", NULL);
  dht.begin();
  loadConfig();

  // ---------- WiFi : เปิด AP ของบอร์ดก่อนเป็นอันดับแรก ----------
  /* ต่อชื่อ AP ด้วยเลข 4 หลักท้ายของ MAC เพื่อไม่ให้บอร์ดหลายตัวชื่อซ้ำกัน
   * MAC ของแต่ละชิปไม่ซ้ำกันทั่วโลก จึงเป็นตัวเลขเฉพาะที่เชื่อถือได้ */
  uint32_t macTail = (uint32_t)(ESP.getEfuseMac() >> 24) & 0xFFFF;
  char ssidBuf[32];
  snprintf(ssidBuf, sizeof(ssidBuf), "%s-%04X", AP_SSID_PREFIX, macTail);
  apSsid = ssidBuf;

  bootMsg("Starting AP", apSsid.c_str());

#if STA_ENABLE
  WiFi.mode(WIFI_AP_STA);        // ปล่อย AP และต่อบ้านพร้อมกัน
#else
  WiFi.mode(WIFI_AP);            // ปล่อย AP อย่างเดียว
#endif

  /* ต้องตั้ง IP ก่อนเรียก softAP() เสมอ ไม่งั้นจะได้ค่าเริ่มต้นของ core
   * gateway ตั้งเป็น IP ของบอร์ดเอง เพราะบอร์ดคือประตูออกเพียงทางเดียวในวงนี้ */
  WiFi.softAPConfig(AP_IP, AP_IP, AP_MASK);
  bool apOK = WiFi.softAP(apSsid.c_str(), AP_PASS, AP_CHANNEL, 0, AP_MAX_CONN);

  if (!apOK) {
    /* AP เปิดไม่ติด สาเหตุที่พบบ่อยที่สุดคือ AP_PASS สั้นกว่า 8 ตัวอักษร
     * กะพริบ LED เตือนแล้วทำงานต่อ ระบบควบคุมรีเลย์ยังต้องเดิน */
    bootMsg("AP FAILED", "AP_PASS >= 8 chars?");
    for (uint8_t i = 0; i < 10; i++) { digitalWrite(LED_PIN, i % 2); delay(120); }
  }

  /* DNS ตอบทุกโดเมนด้วย IP ของบอร์ด ทำให้มือถือเด้งหน้าเว็บขึ้นมาเอง
   * "*" หมายถึงจับทุกชื่อโดเมนไม่มีข้อยกเว้น */
  dnsServer.start(53, "*", AP_IP);

#if STA_ENABLE
  /* ต่อ WiFi บ้านแบบไม่รอผล มาเช็คทีหลังใน taskWifiSta()
   * บอร์ดจึงพร้อมรับมือถือผ่าน AP ได้ทันที ไม่ต้องรอ 20 วินาทีเหมือน Lab12 */
  WiFi.setAutoReconnect(false);  // เราจัดการรอบการลองใหม่เองใน taskWifiSta
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  staRetryAt = millis();
#endif

  // ---------- NTP ----------
  /* สั่งไว้ได้เลยแม้ยังไม่มีเน็ต ระบบจะซิงค์ให้เองทันทีที่ STA ต่อบ้านได้
   * ถ้าต่อบ้านไม่ได้เลย นาฬิกาจะไม่ซิงค์ และโหมดตั้งเวลาจะไม่ทำงาน (ตั้งใจให้เป็นแบบนั้น) */
  configTzTime(TZ_BANGKOK, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);

  // ---------- MQTT ----------
  clientId = "esp32-" + String((uint32_t)(ESP.getEfuseMac() >> 16), HEX);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512);

  // ---------- Web Server ----------
  /* ไม่ระบุ method ให้รับได้ทั้ง GET และ POST
   * ทดสอบจากช่อง URL ของเบราว์เซอร์ได้ตรง ๆ เช่น /api/relay?id=1&state=on */
  server.on("/",           handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/relay",  handleRelayCmd);
  server.on("/api/config", handleConfig);

  // ไฟล์ฟอนต์ Prompt จาก PROGMEM ชื่อต้องตรงกับ @font-face ใน dashboard_html.h
  server.on("/fonts/prompt-400-thai.woff2",  handleFont400Thai);
  server.on("/fonts/prompt-400-latin.woff2", handleFont400Latin);
  server.on("/fonts/prompt-700-thai.woff2",  handleFont700Thai);
  server.on("/fonts/prompt-700-latin.woff2", handleFont700Latin);

  server.onNotFound(handleNotFound);   // ทุก URL ที่ไม่รู้จัก -> หน้า Dashboard (captive portal)
  server.begin();

  bootMsg("Ready  ->  connect to", apSsid.c_str());
  delay(1500);
}


// ================================= LOOP ===================================
void loop() {
  uint32_t now = millis();

  dnsServer.processNextRequest();  // ตอบ DNS ให้มือถือในวง AP ต้องเรียกถี่ ๆ
  server.handleClient();           // ตอบคำขอจากเบราว์เซอร์ ต้องเรียกถี่ ๆ
  taskWifiSta(now);                // ดูแลการต่อ WiFi บ้านแบบไม่บล็อก
  ensureMqtt(now);
  mqtt.loop();                     // รับข้อความ MQTT และตอบ ping ให้ broker

  taskUpdateTime(now);
  taskReadDHT(now);
  taskReadModbus(now);    // อ่านเซนเซอร์ Modbus ทีละตัววนรอบ หรือสร้างค่าจำลอง
  taskButtons(now);       // อ่านปุ่ม SW1-SW3 สำหรับเลื่อนหน้าจอ ต้องเรียกถี่
  taskControl();          // ตรรกะควบคุม 3 โหมด
  taskRelayFailsafe();
  taskPublish(now);
  taskDisplay(now);
}
