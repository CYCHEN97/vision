/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_camera.h"
#include "app_camera.h"
#include "fb_gfx.h"
#include "dl_lib_matrix3d.h"

#include "vision_compute.h"

static const char *TAG = "app_vision_detector";

bool cal_offset_init(void){
	int ret = 0;
	int init_cnt=0;
    sensor_t * s = NULL;
	while(1)
	{
		init_cnt ++;
		if(init_cnt>3)
		{
			ESP_LOGE("","NO camera, plase check camera!\r\n");
			return false;
		}
			vTaskDelay(100 / portTICK_PERIOD_MS);
			s = esp_camera_sensor_get();
			if(NULL == s) {
				ESP_LOGE(TAG, "Can not Get Sensor.");
				continue;
			}

			ret = s->set_framesize(s, FRAMESIZE_CIF);
			if(ESP_OK != ret) {
				ESP_LOGE(TAG, "Set Framesize Error");
				continue;
			}

 			ret = s->set_res_raw(s, 2, 0, 0, 0, 70, 40, 256, 256, 256, 256, 0, 0);
			if(ESP_OK != ret) {
				ESP_LOGE(TAG, "Set Window Error");
				continue;
			}
			ESP_LOGE("","cal offset init\r\n");
		return true;
	}
}

Point2f offset;
bool cal_centr_offset(void)
{
	// if(cal_offset_init()==false)
	// 	return false;
	int err_cnt=0;
	int ret = 0;
    camera_fb_t *fb = NULL;

	while(err_cnt<3)
	{
		for(ret=0;ret<3;ret++)   //读三次图像数据，保证图像是最新数据
		{
			if(fb)
				esp_camera_fb_return(fb);
			fb=esp_camera_fb_get();
			vTaskDelay(50/portTICK_PERIOD_MS);
		}
        if(!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
        } else { 
			ret = get_offset_spot2center(fb->buf, fb->len, &offset);
			if(ret == 0) {
				ESP_LOGI(TAG, "Offset: %.2f, %.2f\n", offset.x, offset.y);
				esp_camera_fb_return(fb);
				return true;
			}
			else {
				ESP_LOGE(TAG, "Get Offset Failed. err(%d)", ret);
			}
            esp_camera_fb_return(fb);
        }
		err_cnt++;
	}
	return false;	
}


