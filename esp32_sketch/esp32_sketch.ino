#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "dl_lib.h"
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory


// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


// most pins can't be set to INPUT without causing errors to do with the SD card. 12 and 13 definitely don't work. 16 seems to also not work some times.
// pin 3 is actually the UORXD pin, but it can be used if we don't need to receive serial data.
const int motionSensorPin = 3; 

// keeps track of how many timelapse photos we have taken
int timelapsePictureNumber = 0;

// keeps track of how many intruder photos we have taken
int intruderPictureNumber = 0;

// random number that groups photos together
int picturesGroup;

// time interval between each timelapse photo
int timelapseInterval = 60000;

// time when the execution started
unsigned long startTime;

// time when the execution started
unsigned long lastTimelapsePhotoTime;

// returns the config for the camera  
camera_config_t getCameraConfig() {  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  return config;
}


void setup() {
  Serial.begin(115200);
  Serial.println("Booting up...");

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector 
  //Serial.setDebugOutput(true);   
  
  // Init Camera
  camera_config_t cameraConfig = getCameraConfig();
  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }
  
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
  
  picturesGroup = random(999);
  pinMode(motionSensorPin, INPUT);
  
  Serial.println("Calibrating motion sensor...");
  delay(30000);
  Serial.println("Starting...");
  
  startTime = millis();
  lastTimelapsePhotoTime = startTime;
}

// returns the path for the next photo, of type either timelapse or intruder
String getImagePath(String imageType) {  
  String imagePath = "/";
  imagePath +=  picturesGroup;
  if (imageType == "intruder") { 
    imagePath += "-intruder-";  
    imagePath +=  intruderPictureNumber;
  } else {
    imagePath += "-timelapse-";  
    imagePath +=  timelapsePictureNumber;    
  }
  imagePath +=  ".jpg";  
  return imagePath;
}

// takes a photo and saves it onto the SD card
void takePicture(String path) {
  camera_fb_t * fb = NULL;

  fb = esp_camera_fb_get(); 
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  fs::FS &fs = SD_MMC; 
  Serial.printf("Picture file name: %s\n", path.c_str());
  
  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  } 
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  file.close();
  
  esp_camera_fb_return(fb);
}


unsigned long currentTime;  

void loop() {
  camera_fb_t * fb = NULL;

  currentTime = millis();
  Serial.print(startTime);
  Serial.print(" - ");
  Serial.print(lastTimelapsePhotoTime);
  Serial.print(" - ");
  Serial.println(currentTime);

  if (currentTime - lastTimelapsePhotoTime > timelapseInterval) {

    // timelapse photos are taken at regular intervals
    lastTimelapsePhotoTime = currentTime;
    takePicture(getImagePath("timelapse"));     
    timelapsePictureNumber++;
    
  } else if (digitalRead(motionSensorPin) == HIGH) {

    // intruder photos are taken when the PIR sensor detects some motion
    takePicture(getImagePath("intruder"));   
    intruderPictureNumber++;
  }

  // decrease this delay to get more intruder photos.
  delay(200);
}
