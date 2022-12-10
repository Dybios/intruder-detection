#include "BluetoothSerial.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>
#include "auth_list.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif


#define MPU_DETECT_THRESHOLD 1
#define MPU_DETECT_DURATION 500

#define BT_NAME "My Bike"
#define INT_PIN 23
#define BUZZER_PIN 19
// NVMEM references tag
#define PREF_NAMESPACE "intrusion"
#define PREF_TIMESTAMP "timestamp"

BluetoothSerial BikeBT;
Preferences prefs;
Adafruit_MPU6050 mpu;

// BT conn status
bool deviceConnected = false;
bool deviceAuthenticated = false;

// Parameters received for processing
sensors_event_t a, g, t;
char allData[64];
volatile bool intrusion = false;
//float engineTemp;

// Prototypes
uint8_t init_mpu_motion(uint16_t detectTresh, uint16_t detectDuration);
uint8_t detect_intrusion_sw(uint32_t timestampVal);
void write_str_bt(BluetoothSerial *bt_device, char *strToWrite);
void ring_buzzer();
void write_timestamp_nvmem();
uint32_t read_timestamp_nvmem();

/* BT callback function to check conn & disconn */
void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if(event == ESP_SPP_SRV_OPEN_EVT){
    uint8_t count;
    deviceConnected = true;
    Serial.println("Device Connected.");
    // Authenticate device
    for (count = 0; count < 6; count++) {
      if (param->srv_open.rem_bda[count] == master_dev_id[count]) {
        deviceAuthenticated = true;
      }
      else {
        deviceAuthenticated = false;
        break;
      }
    }
  }
  if(event == ESP_SPP_CLOSE_EVT){
    deviceConnected = false;
    Serial.println("Device Disonnected.");
  }
}

/* MPU ISR for intruder event via HW interrupt */
void IRAM_ATTR mpu_isr() {
  if (!deviceConnected || (deviceConnected && !deviceAuthenticated)) {
    Serial.println("Intrusion Detected!");
    Serial.println("Writing intrusion data to memory...");
    write_timestamp_nvmem();
    intrusion = true;
  }
}

void setup() {
  uint8_t ret;
    
  pinMode(INT_PIN, INPUT_PULLUP); //INT_PIN is set to active low 
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  /* Setup MPU */
  Serial.begin(115200);
  ret = init_mpu_motion(MPU_DETECT_THRESHOLD, MPU_DETECT_DURATION);
  if (ret) {
    Serial.println("MPU Error.");
  }
  attachInterrupt(INT_PIN, mpu_isr, FALLING);  // Attach hw interrupt to INT pin

  /* Setup BT */
  BikeBT.register_callback(callback);
  BikeBT.begin(BT_NAME); //Bluetooth device name
}

void loop() {  
  uint8_t ret;
  uint32_t lastTimestampVal;
  char timestampStr[8];
  /* Get new sensor events with the readings */
  mpu.getEvent(&a, &g, &t);
  
  if (intrusion) {
    Serial.println("Ringing buzzer 5 times...");
    ring_buzzer();
    intrusion = false;
  }
  
  // detect_intrusion_sw(gyro.timestamp); // If needed to detect int in SW

  // Begin sending data over BT serial
  if (deviceConnected && deviceAuthenticated) { 
    sprintf(allData, "%f,%f,%f,%f,%f,%f,%f\n", a.acceleration.x, a.acceleration.y, a.acceleration.z, \
                      g.gyro.x, g.gyro.y, g.gyro.z, t.temperature);

    if (BikeBT.read() == 'i') {
      lastTimestampVal = read_timestamp_nvmem();
      sprintf(timestampStr, "%u\n", lastTimestampVal);
      write_str_bt(&BikeBT, timestampStr);
      delay(1000);
    }

    write_str_bt(&BikeBT, allData);
  }

  delay(20);
}

uint8_t init_mpu_motion(uint16_t detectTresh, uint16_t detectDuration) {
  uint8_t ret = 0;
  
  /* Setup MPU */
  if (!mpu.begin()) {
    ret = 1;
  }
  //setupt motion detection
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(detectTresh);
  mpu.setMotionDetectionDuration(detectDuration);
  mpu.setInterruptPinLatch(false);  // Clear INT pin after 50us
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  return ret;
}


// Detect intrusion via SW, write to NVMem to store the latest intrusion data with timestamp
/*uint8_t detect_intrusion_sw(uint32_t timestampVal) {
  uint8_t ret = 0;
  if(mpu.getMotionInterruptStatus() && (!deviceConnected)) {
    prefs.putBool(PREF_INTRUSION, true);
    prefs.putUInt(PREF_TIMESTAMP, timestampVal);
    prefs.end();

    for (uint8_t count = 0; count < 5; count++) {
      digitalWrite(BUZZER_PIN, HIGH);
      delay(500);
      digitalWrite(BUZZER_PIN, LOW);
      delay(500);
    }
  }
  return ret;
}
*/

void write_timestamp_nvmem() {
  prefs.begin(PREF_NAMESPACE, false); 
  prefs.putUInt(PREF_TIMESTAMP, a.timestamp);
  prefs.end();
}

uint32_t read_timestamp_nvmem() {
  uint32_t timestamp;
  prefs.begin(PREF_NAMESPACE, false); 
  timestamp = prefs.getUInt(PREF_TIMESTAMP, 0);
  prefs.end();
  return timestamp;
}

void ring_buzzer() {
  for (uint8_t count = 0; count < 5; count++) {
    digitalWrite(BUZZER_PIN, LOW);
    delay(500);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(500);
  }
}

void write_str_bt(BluetoothSerial *bt_device, char *strToWrite) {
  uint16_t strSize = strlen(strToWrite);

  for (uint16_t count = 0; count < strSize; count++) {
    bt_device->write(strToWrite[count]);
  }
}
