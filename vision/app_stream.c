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
#include "tcpip_adapter.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_camera.h"
#include "app_camera.h"
#include "fb_gfx.h"
#include "dl_lib_matrix3d.h"

#define STREAM_PORT     5754

static const char *TAG = "app_stream";

static void tcp_stream_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;

#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(STREAM_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 dest_addr;
    bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(STREAM_PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        while(true) {
            ESP_LOGE(TAG, "[Stream Server] Unable to create [Stream] socket: errno %d", errno);
            sleep(1);
        }
    }
    ESP_LOGI(TAG, "[Stream Server] Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        while(true) {
            ESP_LOGE(TAG, "[Stream Server] Socket unable to bind: errno %d", errno);
            sleep(1);
        }
    }
    ESP_LOGI(TAG, "[Stream Server] Socket bound, port %d", STREAM_PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        while(true) {
            ESP_LOGE(TAG, "[Stream Server] Error occurred during listen: errno %d", errno);
            sleep(1);
        }
    }
    ESP_LOGI(TAG, "[Stream Server] Socket listening");

    while (1) {
        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        {
            int enable = 1;
            int lost_time = 1;
            int interval = 1;
            int cnt = 1;
            if(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) != 0) {
                ESP_LOGE(TAG, "[Stream Server] Unable to SETUP [keepalive]: errno %d", errno);
            }
            if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &lost_time, sizeof(lost_time)) != 0) {
                ESP_LOGE(TAG, "[Stream Server] Unable to SETUP [keepidle]: errno %d", errno);
            }
            if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) != 0) {
                ESP_LOGE(TAG, "[Stream Server] Unable to SETUP [keepidle]: errno %d", errno);
            }
            if(setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt)) != 0) {
                ESP_LOGE(TAG, "[Stream Server] Unable to SETUP [keepidle]: errno %d", errno);
            }
        }
        if (sock < 0) {
            ESP_LOGE(TAG, "[Stream Server] Unable to accept connection: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "[Stream Server] Socket accepted");

        char head[81] = {0};
        head[0] = 1;
        int err = 0;
        bool is_gray = false;
        sensor_t* s = esp_camera_sensor_get();
        while (1) {
            static char fg_get_failed_num = 0;
            vTaskDelay(50/ portTICK_PERIOD_MS);
            // ESP_LOGE(TAG, "get camera fb\r\n");
            camera_fb_t *fb = esp_camera_fb_get();
            if (!s) {
                ESP_LOGE(TAG, "[Stream Server] Get Sensor Failed");
            }

            if((!fb)||(fb==NULL)) {
                ESP_LOGE(TAG, "[Stream Server] Camera Capture Failed");
                fg_get_failed_num++;
                esp_camera_fb_return(fb);
                if(fg_get_failed_num > 2) {
                    ESP_LOGE(TAG, "[Stream Server] return camera_fb_buffer");
                    fg_get_failed_num = 0;
                }
            }
            else {
                // ESP_LOGI(TAG, "get camera capture successful.");
                memcpy(head + 1, s, 52);
                memcpy(head + 1 + 52, fb, 28);
                err = send(sock, head, 81, 0);
                if (err < 0) {
                    ESP_LOGE(TAG, "[Stream Server] Error occurred during sending PIXFORMAT: errno %d", errno);
                    if(fb) esp_camera_fb_return(fb);
                    break;
                }
				else {
					//ESP_LOGI(TAG, "send head successful.");
				}

                fg_get_failed_num = 0;
                if(is_gray) {
                    //err= send(sock, gray->item, gray->w * gray->h * gray->c, 0);
                }
                else {
                    err = send(sock, fb->buf, fb->len, 0);
                }
               	esp_camera_fb_return(fb);


                if (err < 0) {
                    ESP_LOGE(TAG, "[Stream Server] Error occurred during sending STATUS: errno %d", errno);
                    break;
                }
				else {
					//ESP_LOGI(TAG, "send buf successful.");
				}
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "[Stream Server] Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_stream()
{
    xTaskCreatePinnedToCore(tcp_stream_task, "tcp_stream_task", 8192, NULL, 3, NULL,  1); 
}
