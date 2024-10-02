#include <TimeLib.h> 
#include "Arduino.h"
#include "WiFi.h"
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
//Provide the token generation process info.
#include <addons/TokenHelper.h>
//Replace with your network credentials
const char* ssid = "ssid";
const char* password = "password";
// Insert Firebase project API Key
#define API_KEY "API_KEY"
#define STORAGE_BUCKET_ID "STORAGE_BUCKET_ID"
#define DATABASE_URL "DATABASE_URL"

// OV2640 camera module pins (CAMERA_MODEL_AI_THINKER)
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

boolean okRead = false;
boolean alarm_status = false;
boolean enable_alarm = false;
unsigned int count = 1;
unsigned int gallery_max_photo = 50;
String fileName = "";
String previousFileName = "";
String bucketPhoto = "";
//Define Firebase Data objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;
FB_Storage storage;


void fcsUploadCallback(FCS_UploadStatusInfo info);
void capturePhotoSaveLittleFS();
void deleteFile(const char* filename);
void deleteAllFiles(const char* directory);
void initCamera();
void initWiFi();
void initLittleFS();
void deleteAllFiles(const char* directory);
void deleteFileFromStorage(const char* filePath);
void deleteFile(const char* filename);

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  // Turn-off the 'brownout detector'
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  initCamera();
  initWiFi();
  initLittleFS();
  deleteAllFiles("/");
  //Firebase
  // Assign the api key
  configF.api_key = API_KEY;
  configF.database_url = DATABASE_URL;

  if (Firebase.signUp(&configF, &auth, "", "")){
    Serial.println("signed up.");
  }
  else{
    Serial.printf("%s\n", configF.signer.signupError.message.c_str());
  }
  //Assign the callback function for the long running token generation task
  configF.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready()){
    if (Firebase.RTDB.getBool(&fbdo, "alarm_status")) {
      if (fbdo.dataType() == "boolean") {
        alarm_status = fbdo.boolData();
        okRead = true;
      }
    }
    else {
      Serial.println("Error reading alarm status from Firebase.");
      Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getBool(&fbdo, "enable_alarm")) {
      if (fbdo.dataType() == "boolean") {
        enable_alarm = fbdo.boolData();
      }
    }
    else {
      Serial.println("Error reading enable alarm from Firebase.");
      Serial.println(fbdo.errorReason());
    }
    if (alarm_status && okRead && enable_alarm) {    
      capturePhotoSaveLittleFS(count);
      delay(1);
      
      //The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
      if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, fileName, mem_storage_type_flash, bucketPhoto, "image/jpeg", fcsUploadCallback)){
        //Serial.printf("Upload successful. Download URL: %s\n", fbdo.downloadURL().c_str());
        deleteFile(fileName.c_str());
        count++;
        if(count > gallery_max_photo){
          previousFileName = "/data/" + String(count - gallery_max_photo) + ".jpg";
          deleteFileFromStorage(previousFileName.c_str());
        }
      }
      else{
        Serial.println("Error uploading photo to Firebase Storage.");
        Serial.println(fbdo.errorReason());
      }
      okRead = false;
    }
  }
}

// Capture Photo and Save it to LittleFS
void capturePhotoSaveLittleFS(unsigned int count) {
  fileName = "/" + String(count) + ".jpg";
  const char* file_path = fileName.c_str();
  // Dispose first pictures because of bad quality
  camera_fb_t* fb = NULL;
  // Skip first 3 frames (increase/decrease number as needed).
  for (int i = 0; i < 4; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
    fb = NULL;
  }
    
  // Take a new photo
  fb = NULL;  
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
  }  

  // Photo file name
  //Serial.printf("Picture file name: %s\n", file_path);
  File file = LittleFS.open(file_path, FILE_WRITE);

  // Insert the data in the photo file
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    /*Serial.print("The picture has been saved in ");
    Serial.print(file_path);
    Serial.print(" - Size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");*/
  }
  // Close the file
  file.close();
  esp_camera_fb_return(fb);
  // Set the BUCKET_PHOTO variable to the concatenated string
  bucketPhoto = "/data" + fileName;
}

void initWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    // Delete all files ending in ".jpg"
    Serial.println("An Error has occurred while mounting LittleFS");
    ESP.restart();
  }
  else {
    Serial.println("LittleFS mounted successfully");
  }
}

void initCamera() {
 // OV2640 camera module
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
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  } 
}

// The Firebase Storage upload callback function
void fcsUploadCallback(FCS_UploadStatusInfo info) {
    if (info.status == firebase_fcs_upload_status_init){
        //Serial.printf("Uploading file %s (%d) to %s\n", info.localFileName.c_str(), info.fileSize, info.remoteFileName.c_str());
    }
    else if (info.status == firebase_fcs_upload_status_upload)
    {
        //Serial.printf("Uploaded %d%s, Elapsed time %d ms\n", (int)info.progress, "%", info.elapsedTime);
    }
    else if (info.status == firebase_fcs_upload_status_complete)
    {
        //Serial.println("Upload completed\n");
        FileMetaInfo meta = fbdo.metaData();
        Serial.printf("Name: %s\n", meta.name.c_str());/*
        Serial.printf("Bucket: %s\n", meta.bucket.c_str());
        Serial.printf("contentType: %s\n", meta.contentType.c_str());
        Serial.printf("Size: %d\n", meta.size);
        Serial.printf("Generation: %lu\n", meta.generation);
        Serial.printf("Metageneration: %lu\n", meta.metageneration);
        Serial.printf("ETag: %s\n", meta.etag.c_str());
        Serial.printf("CRC32: %s\n", meta.crc32.c_str());
        Serial.printf("Tokens: %s\n", meta.downloadTokens.c_str());
        Serial.printf("Download URL: %s\n\n", fbdo.downloadURL().c_str());*/
    }
    else if (info.status == firebase_fcs_upload_status_error){
        Serial.printf("Upload failed, %s\n", info.errorMsg.c_str());
    }
}

// Function to delete a specific file from LittleFS
void deleteFile(const char* filename) {
  if (LittleFS.remove(filename)) {
    //Serial.printf("File '%s' deleted successfully.\n", filename);
  } else {
    Serial.printf("Failed to delete file '%s'.\n", filename);
  }
}

void deleteAllFiles(const char* directory) {
  Serial.printf("Files in directory '%s':\n", directory);
  File dir = LittleFS.open(directory);
  File file = dir.openNextFile();
  String filePath = "";
  while (file) {
    if (!file.isDirectory()) {
      filePath = String(directory) + String(file.name());
      Serial.println(file.name());
    }
    file = dir.openNextFile();
    deleteFile(filePath.c_str());
  }
}

void deleteFileFromStorage(const char* filePath) {
  if (storage.deleteFile(&fbdo, STORAGE_BUCKET_ID, filePath)) {
    Serial.printf("File '%s' deleted successfully.\n", filePath);
  } else {
    Serial.printf("Failed to delete file '%s'.\n", filePath);
    Serial.println(fbdo.errorReason());
  }
}
