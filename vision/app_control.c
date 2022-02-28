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

#include "esp32_cam_scp.h"

#define CONTROL_PORT    5755

static const char *TAG = "app_control";

/*
static void camera_task(void *pvParameters) {
    int8_t * buf = NULL;
    size_t buf_len = 0;  
            
    vTaskDelay((10 * 1000) / portTICK_PERIOD_MS);
    while(true) {
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera Capture Failed");
        }
        else {
            ESP_LOGI(TAG, "Image Get! len: %d", fb->len);
        }
        esp_camera_fb_return(fb);
        vTaskDelay(30 / portTICK_PERIOD_MS);
    }
}
*/


static void tcp_control_task(void *pvParameters)
{
    uint16_t cmd_len;
    char addr_str[128];
    int addr_family;
    int ip_protocol;


#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(CONTROL_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 dest_addr;
    bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(CONTROL_PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        while(true) {
            ESP_LOGE(TAG, "[Contro Server] Unable to create [Stream] socket: errno %d", errno);
            sleep(1);
        }
    }
    ESP_LOGI(TAG, "[Contro Server] Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        while(true) {
            ESP_LOGE(TAG, "[Contro Server] Socket unable to bind: errno %d", errno);
            sleep(1);
        }
    }
    ESP_LOGI(TAG, "[Contro Server] Socket bound, port %d", CONTROL_PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        while(true) {
            ESP_LOGE(TAG, "[Contro Server] Error occurred during listen: errno %d", errno);
            sleep(1);
        }
    }
    ESP_LOGI(TAG, "[Contro Server] Socket listening");

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
        if (sock < 0) {
            ESP_LOGE(TAG, "[Control Server] Unable to accept connection: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "[Control Server] Socket accepted");

        while (1) {
            cmd_len = 0;
            vTaskDelay(50 / portTICK_PERIOD_MS);
            int len = recv(sock, (char*)&cmd_len, 2, MSG_WAITALL);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "[Control Server] recv failed: errno %d", errno);
                break;
            }
            // Connection closed
            else if (len == 0) {
                ESP_LOGI(TAG, "[Control Server] Connection closed");
                break;
            }
            // Data received
            else {
                if(cmd_len > 256) {
                    ESP_LOGE(TAG, "[Control Server] Receiving exception(Len: %d)", cmd_len);
                    break; // break to shudown the socket
                }
                else {
                    char* rx_buf = (char*)malloc(cmd_len - sizeof(cmd_len));
                    if(!rx_buf) {
                        ESP_LOGE(TAG, "[Control Server] Malloc(%d) failed", cmd_len);
                        break; // break to shutdown the socket
                    }

                    err = recv(sock, rx_buf, cmd_len - sizeof(cmd_len), MSG_WAITALL);
                    do {
                        sensor_t* s = esp_camera_sensor_get();
                        if(!s) {
                            ESP_LOGE(TAG, "[Control Server] Get Sensor Failed.");
                        }
                        else {
                            int ret = 0;
                            ret = esp32camscp_excute_cmd(s, rx_buf);
                            ESP_LOGI(TAG, "CMD: ID: %d, VAL: %d, LEN: %d, RET: %d",
                                    rx_buf[1],
                                    rx_buf[2] == 1 ? *((int8_t*)(rx_buf + 3)) : rx_buf[2] == 2 ? *((int16_t*)(rx_buf + 3)) :*((int*)(rx_buf + 3)),
                                    rx_buf[2],
                                    ret);

                            break;
                        }
                    }
                    while(true);
                    free(rx_buf);
                }
                /*
                // Get the sender's ip address as string
                if (source_addr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (source_addr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "[Contro Server] Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);

                int err = send(sock, rx_buffer, len, 0);
                if (err < 0) {
                    ESP_LOGE(TAG, "[Contro Server] Error occurred during sending: errno %d", errno);
                    break;
                }
                */
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "[Contro Server] Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
    vTaskDelete(NULL);
}

void app_control()
{
    xTaskCreatePinnedToCore(tcp_control_task, "tcp_control_task", 8192, NULL, 3, NULL,  1); 
}
