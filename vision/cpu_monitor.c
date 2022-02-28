#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

static void test_task(void *param) {
   	// char pbuffer[2000];
    while(1) {
		printf("-----free heap:%u ------\r\n", esp_get_free_heap_size());
		printf("-----inter free heap:%u-----\r\n",esp_get_free_internal_heap_size());
		// vTaskGetRunTimeStats(pbuffer);
		// printf("%s", pbuffer);
		// printf("----------------------------------------------\r\n");
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

void app_cpu_monitor() {
	xTaskCreatePinnedToCore(test_task, "test_task", 4096, NULL, 5, NULL,  1); 
}
