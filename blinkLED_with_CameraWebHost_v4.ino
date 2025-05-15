#include "esp_camera.h"
#include <WiFi.h>
#include "esp_http_server.h"

// XIAO ESP32 S3 camera pins
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

#define LED_BUILTIN 21


const char *ssid = "XIAO_Camera";
const char *password = "****";    //put your password here
httpd_handle_t camera_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req) {
    camera_fb_t *fb = NULL;
    static const char *STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
    static const char *STREAM_BOUNDARY = "--frame\r\n";
    static const char *STREAM_PART = "Content-Type: image/jpeg\r\n\r\n";

    httpd_resp_set_type(req, STREAM_CONTENT_TYPE);

    while (true) {
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera error");
            continue;
        }
        httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
        httpd_resp_send_chunk(req, STREAM_PART, strlen(STREAM_PART));
        httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        httpd_resp_send_chunk(req, "\r\n", 2);
        esp_camera_fb_return(fb);
    }
    return ESP_OK;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_uri_t uri_get = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = stream_handler,
        .user_ctx = NULL};
    
    if (httpd_start(&camera_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(camera_httpd, &uri_get);
    }
}

void camera_task(void *pvParameters) {
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
    
    /*Clock Frequency*/
    config.xclk_freq_hz = 20000000;  //20 MHz
    //config.xclk_freq_hz = 16000000;  //16 MHz

    /*Frame size*/
    //config.frame_size = FRAMESIZE_UXGA;  //15 fps, Image out size: 1632 x 1220 (UXGA)
    config.frame_size = FRAMESIZE_SVGA;    //30 fps, Image out size:  800 x 600 (SVGA)
    //config.frame_size = FRAMESIZE_CIF;     //60 fps, Image out size: 408 x 304 (CIF)

    /*Pixel format*/
    config.pixel_format = PIXFORMAT_JPEG;
    //config.pixel_format = PIXFORMAT_GRAYSCALE;
    //config.pixel_format = PIXFORMAT_YUV422;

    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    if (psramFound()) {
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.fb_location = CAMERA_FB_IN_DRAM;
    }

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Camera init failed!");
        vTaskDelete(NULL);
    }
    Serial.println("Camera init successful!");
    startCameraServer();
    vTaskDelete(NULL);
}

void wifi_task(void *pvParameters) {
    WiFi.softAP(ssid, password);
    Serial.println("WiFi created. Connect on:");
    Serial.println(WiFi.softAPIP());
    vTaskDelete(NULL);
}

void led_task(void *parameter) {
    // Setup start here
    pinMode(LED_BUILTIN, OUTPUT);
    // End of your setup
    
    // Loop function, run repeatedly
    for (;;) {
        int led = digitalRead(LED_BUILTIN);
        digitalWrite(LED_BUILTIN, !led);
        delay(1000);
    }
}

void setup() {
    Serial.begin(115200);

    xTaskCreatePinnedToCore(
        wifi_task,             // Task function
        "WiFi Task",           // Task name
        8192,                  // Stack size
        NULL,                  // Task input parameters
        1,                     // Task priority, be carefull when changing this
        NULL,                  // Task handle, add one if you want control over the task (resume or suspend the task) 
        0                      // Core to run the task on
    );

    xTaskCreatePinnedToCore(
        camera_task,           // Task function
        "Camera Task",         // Task name
        8192,                  // Stack size
        NULL,                  // Task input parameters
        1,                     // Task priority, be carefull when changing this
        NULL,                  // Task handle, add one if you want control over the task (resume or suspend the task) 
        1                      // Core to run the task on
    );

    xTaskCreatePinnedToCore(
        led_task,              // Task function
        "LED Task",            // Task name
        4096,                  // Stack size
        NULL,                  // Task input parameters
        1,                     // Task priority, be carefull when changing this
        NULL,                  // Task handle, add one if you want control over the task (resume or suspend the task) 
        1                      // Core to run the task on
    );
}

void loop() {
    delay(10000);
}
