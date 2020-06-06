/*
  camera.cpp -  camera functions class

  Copyright (c) 2014 Luc Lebosse. All rights reserved.

  This code is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This code is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with This code; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../../include/esp3d_config.h"
#ifdef CAMERA_DEVICE
#include "camera.h"
#include "../../core/settings_esp3d.h"
#include "../network/netservices.h"
#include "../../core/esp3doutput.h"
#include "../network/netconfig.h"
#include "esp_http_server.h"
#include <esp_camera.h>
#include "fd_forward.h"
//#include <soc/soc.h> //not sure this one is needed
#include <soc/rtc_cntl_reg.h>
#include <driver/i2c.h>

#define DEFAULT_FRAME_SIZE FRAMESIZE_SVGA
#define HTTP_TASK_PRIORITY  5
#define PART_BUFFER_SIZE 64
#define JPEG_COMPRESSION 80
#define MIN_WIDTH_COMPRESSION 400
#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
Camera esp3d_camera;

//to break the loop
static void disconnected_uri(httpd_handle_t hd, int sockfd)
{
    log_esp3d("Camera stream disconnected");
    esp3d_camera.connect(false);
}

static esp_err_t stream_handler(httpd_req_t *req)
{
    log_esp3d("Camera stream reached");
    if (!esp3d_camera.started()) {
        const char* resp = "Camera not started";
        log_esp3d("Camera not started");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_FAIL;
    }
    esp3d_camera.connect(true);
#ifdef ESP_ACCESS_CONTROL_ALLOW_ORIGIN
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
#endif //ESP_ACCESS_CONTROL_ALLOw_ORIGIN
    camera_fb_t * fb = NULL;
    esp_err_t res = ESP_OK;
    size_t _jpg_buf_len = 0;
    uint8_t * _jpg_buf = NULL;
    char * part_buf[PART_BUFFER_SIZE];
    dl_matrix3du_t *image_matrix = NULL;
    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if(res != ESP_OK) {
        esp3d_camera.connect(false);
        const char* resp = "Stream type failed";
        log_esp3d("Stream type failed");
        httpd_resp_send(req, resp, strlen(resp));
        return res;
    }
    uint8_t retry = 0;
    while(true) {
        if (!esp3d_camera.isconnected()) {
            const char* resp = "Camera is not connected";
            log_esp3d("Camera is not connected");
            httpd_resp_send(req, resp, strlen(resp));
            return ESP_FAIL;
        }
        log_esp3d("Camera capture ongoing");
        fb = esp_camera_fb_get();
        if (!fb) {
            log_esp3d("Camera capture failed");
            if ( retry < 3) {
                log_esp3d("Retry %d",retry );
                retry ++;
                continue;
            } else {
                res = ESP_FAIL;
            }
        } else {
            if(fb->width > MIN_WIDTH_COMPRESSION) {
                if(fb->format != PIXFORMAT_JPEG) {
                    bool jpeg_converted = frame2jpg(fb, JPEG_COMPRESSION, &_jpg_buf, &_jpg_buf_len);
                    esp_camera_fb_return(fb);
                    fb = NULL;
                    if(!jpeg_converted) {
                        log_esp3d("JPEG compression failed");
                        res = ESP_FAIL;
                    }
                } else {
                    _jpg_buf_len = fb->len;
                    _jpg_buf = fb->buf;
                }
            } else {
                image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);

                if (!image_matrix) {
                    log_esp3d("dl_matrix3du_alloc failed");
                    res = ESP_FAIL;
                } else {
                    if(!fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item)) {
                        log_esp3d("fmt2rgb888 failed");
                        res = ESP_FAIL;
                    } else {
                        if (fb->format != PIXFORMAT_JPEG) {
                            if(!fmt2jpg(image_matrix->item, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 90, &_jpg_buf, &_jpg_buf_len)) {
                                log_esp3d("fmt2jpg failed");
                                res = ESP_FAIL;
                            }
                            esp_camera_fb_return(fb);
                            fb = NULL;
                        } else {
                            _jpg_buf = fb->buf;
                            _jpg_buf_len = fb->len;
                        }
                    }
                    dl_matrix3du_free(image_matrix);
                }
            }
        }

        if(res == ESP_OK) {
            size_t hlen = snprintf((char *)part_buf, PART_BUFFER_SIZE, _STREAM_PART, _jpg_buf_len);
            res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
        }
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
        }
        if(res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }
        if(fb) {
            esp_camera_fb_return(fb);
            fb = NULL;
            _jpg_buf = NULL;
        } else if(_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if(res != ESP_OK) {
            break;
        }
    }
    esp3d_camera.connect(false);
    return res;
}

Camera::Camera()
{
    _started = false;
    _initialised = false;
    _connected  = false;
}

Camera::~Camera()
{
    end();
}


int Camera::command(const char * param, const char * value)
{
    int res = 0;
    int val = atoi(value);
    sensor_t * s = esp_camera_sensor_get();
    if (s == nullptr) {
        res = -1;
    }
#if CAM_LED_PIN != -1
    if (!strcmp(param, "light")) {
        digitalWrite(CAM_LED_PIN, val==1?HIGH:LOW);
    } else
#endif //CAM_LED_PIN
        if(!strcmp(param, "framesize")) {
            if(s->pixformat == PIXFORMAT_JPEG) {
                res = s->set_framesize(s, (framesize_t)val);
            }
        } else if(!strcmp(param, "quality")) {
            res = s->set_quality(s, val);
        } else if(!strcmp(param, "contrast")) {
            res = s->set_contrast(s, val);
        } else if(!strcmp(param, "brightness")) {
            res = s->set_brightness(s, val);
        } else if(!strcmp(param, "saturation")) {
            res = s->set_saturation(s, val);
        } else if(!strcmp(param, "gainceiling")) {
            res = s->set_gainceiling(s, (gainceiling_t)val);
        } else if(!strcmp(param, "colorbar")) {
            res = s->set_colorbar(s, val);
        } else if(!strcmp(param, "awb")) {
            res = s->set_whitebal(s, val);
        } else if(!strcmp(param, "agc")) {
            res = s->set_gain_ctrl(s, val);
        } else if(!strcmp(param, "aec")) {
            res = s->set_exposure_ctrl(s, val);
        } else if(!strcmp(param, "hmirror")) {
            res = s->set_hmirror(s, val);
        } else if(!strcmp(param, "vflip")) {
            res = s->set_vflip(s, val);
        } else if(!strcmp(param, "awb_gain")) {
            res = s->set_awb_gain(s, val);
        } else if(!strcmp(param, "agc_gain")) {
            res = s->set_agc_gain(s, val);
        } else if(!strcmp(param, "aec_value")) {
            res = s->set_aec_value(s, val);
        } else if(!strcmp(param, "aec2")) {
            res = s->set_aec2(s, val);
        } else if(!strcmp(param, "dcw")) {
            res = s->set_dcw(s, val);
        } else if(!strcmp(param, "bpc")) {
            res = s->set_bpc(s, val);
        } else if(!strcmp(param, "wpc")) {
            res = s->set_wpc(s, val);
        } else if(!strcmp(param, "raw_gma")) {
            res = s->set_raw_gma(s, val);
        } else if(!strcmp(param, "lenc")) {
            res = s->set_lenc(s, val);
        } else if(!strcmp(param, "special_effect")) {
            res = s->set_special_effect(s, val);
        } else if(!strcmp(param, "wb_mode")) {
            res = s->set_wb_mode(s, val);
        } else if(!strcmp(param, "ae_level")) {
            res = s->set_ae_level(s, val);
        } else {
            res = -1;
        }
    return res;
}

bool Camera::initHardware(bool forceinit)
{
    if (forceinit) {
        _initialised = false;
    }
    if (_initialised) {
        return _initialised;
    }
    log_esp3d("Disable brown out");
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
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
    config.xclk_freq_hz = 10000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.jpeg_quality = 5;
    config.fb_count = 1;
    config.frame_size = DEFAULT_FRAME_SIZE;
    if(!psramFound()) {
        _initialised = false;
        log_esp3d("psram is not enabled");
        return false;
    }
    if (!stopHardware()) {
        log_esp3d("Stop camera failed");
    }
    log_esp3d("Init camera");
#if CAM_PULLUP1 != -1
    pinMode(CAM_PULLUP1, INPUT_PULLUP);
#endif //CAM_PULLUP1
#if CAM_PULLUP2 != -1
    pinMode(CAM_PULLUP2, INPUT_PULLUP);
#endif //CAM_PULLUP2
#if CAM_LED_PIN != -1
    pinMode(CAM_LED_PIN, OUTPUT);
    digitalWrite(CAM_LED_PIN, LOW);
#endif //CAM_LED_PIN
    //initialize the camera

//https://github.com/espressif/esp32-camera/issues/66#issuecomment-526283681
#if CAMERA_DEVICE == CAMERA_MODEL_AI_THINKER
    log_esp3d("Specific config for CAMERA_MODEL_AI_THINKER");
    gpio_config_t gpio_pwr_config;
    gpio_pwr_config.pin_bit_mask = (1ULL << 32);
    gpio_pwr_config.mode = GPIO_MODE_OUTPUT;
    gpio_pwr_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_pwr_config.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_pwr_config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&gpio_pwr_config);
    gpio_set_level(GPIO_NUM_32,0);
#endif //CAMERA_DEVICE == CAMERA_MODEL_AI_THINKER 
    log_esp3d("Init camera config");
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        log_esp3d("Camera init failed with error 0x%x", err);
        _initialised = false;
    } else {
        _initialised = true;
    }
    return _initialised;
}

bool Camera::stopHardware()
{
    end();
    _initialised = false;
    log_esp3d("deinit camera");
    esp_err_t err = esp_camera_deinit();
    if(err == ESP_OK) {
        //seems sometimes i2c install failed when doing camera init so let's remove if already installed
        if(i2c_driver_delete(I2C_NUM_1)!= ESP_OK) {
            log_esp3d("I2C 1 delete failed");
        }
        return true;
    } else {
        log_esp3d("Camera deinit failed with error 0x%x", err);
        return false;
    }
}

//need to be call by device and by network
bool Camera::begin(bool forceinit)
{
    end();
    log_esp3d("Begin camera");
    if (!initHardware(forceinit) ) {
        log_esp3d("Init hardware failed");
        return false;
    }
    delay(1000);
    log_esp3d("Init camera sensor settings");
    sensor_t * s = esp_camera_sensor_get();
    if (s != nullptr) {
        //initial sensors are flipped vertically and colors are a bit saturated
        if (s->id.PID == OV3660_PID) {
            s->set_brightness(s, 1);//up the blightness just a bit
            s->set_saturation(s, -2);//lower the saturation
        }

        s->set_framesize(s, DEFAULT_FRAME_SIZE);

#if defined(CAMERA_DEVICE_FLIP_HORIZONTALY)
        s->set_hmirror(s, 1);
#endif //CAMERA_DEVICE_FLIP_HORIZONTALY
#if defined(CAMERA_DEVICE_FLIP_VERTICALY)
        s->set_vflip(s, 1);
#endif //CAMERA_DEVICE_FLIP_VERTICALY
    } else {
        log_esp3d("Cannot access camera sensor");
    }
    if (NetConfig::started() && (NetConfig::getMode()!= ESP_BT)) {
        ESP3DOutput output(ESP_SERIAL_CLIENT);
        httpd_config_t httpdconfig = HTTPD_DEFAULT_CONFIG();
        httpdconfig.close_fn =&disconnected_uri;
        httpd_uri_t stream_uri = {
            .uri = "/stream",
            .method = HTTP_GET,
            .handler = stream_handler,
            .user_ctx = NULL
        };
        _port = Settings_ESP3D::read_uint32(ESP_CAMERA_PORT);
        httpdconfig.server_port = _port;
        httpdconfig.ctrl_port = httpdconfig.server_port +1;
        httpdconfig.task_priority = HTTP_TASK_PRIORITY;
        log_esp3d("Starting camera server");
        if (httpd_start(&stream_httpd, &httpdconfig) == ESP_OK) {
            String stmp = "Camera server started port " + String(httpdconfig.server_port);
            output.printMSG(stmp.c_str());
            log_esp3d("Registering /stream");
            if (httpd_register_uri_handler(stream_httpd, &stream_uri) != ESP_OK) {
                log_esp3d("Registering /stream failed");
            }
        } else {
            log_esp3d("Starting camera server failed");
            output.printERROR("Starting camera server failed");
            return false;
        }
        _started = true;
    }
    for (int j = 0; j < 5; j++) {
        camera_fb_t * fb = esp_camera_fb_get();  // start the camera ... warm it up
        if (fb == nullptr) {
            log_esp3d("Failed to get fb");
        }
        esp_camera_fb_return(fb);
        delay(20);
    }
    return _started;
}

void Camera::end()
{
    if (_started) {
        _started = false;
        _connected  = false;
        log_esp3d("unregister /stream");
        if (ESP_OK != httpd_unregister_uri(stream_httpd, "/stream")) {
            log_esp3d("Error unregistering /stream");
        }
        log_esp3d("Stop httpd");
        if (ESP_OK != httpd_stop(stream_httpd)) {
            log_esp3d("Error stopping stream server");
        }
    }
}

void Camera::handle()
{
    //so far nothing to do
}

uint8_t Camera::GetModel()
{
    return CAMERA_DEVICE;
}

const char *Camera::GetModelString()
{
#if defined(CUSTOM_CAMERA_NAME)
    return CUSTOM_CAMERA_NAME;
#else
    switch(CAMERA_DEVICE) {
    case CAMERA_MODEL_WROVER_KIT:
        return "WROVER Kit";
        break;
    case CAMERA_MODEL_ESP_EYE:
        return "ESP Eye";
        break;
    case CAMERA_MODEL_M5STACK_PSRAM:
        return "M5Stack with PSRam";
        break;
    case CAMERA_MODEL_M5STACK_WIDE:
        return "M5Stack wide";
        break;
    case CAMERA_MODEL_AI_THINKER:
        return "ESP32 Cam";
        break;
    default:
        return "Unknow Camera";
    }
#endif //CUSTOM_CAMERA_NAME 
}
#endif //CAMERA_DEVICE
