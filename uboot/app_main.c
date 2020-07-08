/******************************************************************************
 * Main function used here.
 *
 * FileName: miio_main.c
 *
 * Description: entry file of user application
 *
 *
 * Time: 2016.10.
 *
*******************************************************************************/


/*********************************************************************
 * INCLUDES
 */
#include "miio_instance.h"
#include "miio_arch.h"
#include "jsmi.h"

#include "nvs_flash.h"


arch_os_thread_handle_t myhandle;
arch_os_queue_handle_t myquehan;

static int app_online_hook_default(miio_handle_t handle, void *ctx)
{
	LOG_INFO_TAG("app_main", "%s called", __FUNCTION__);
	return MIIO_OK;
}

static int app_offline_hook_default(miio_handle_t handle, void *ctx)
{
	LOG_INFO_TAG("app_main", " %s called", __FUNCTION__);
	return MIIO_OK;
}

static int app_restore_hook_default(miio_handle_t handle, void *ctx)
{
	LOG_INFO_TAG("app_main", " %s called", __FUNCTION__);
	return MIIO_OK;
}

static miio_hooks_t s_app_hooks = {
	.info = NULL,
	.ext_rpc = NULL,
	.online = app_online_hook_default,
	.offline = app_offline_hook_default,
	.restore = app_restore_hook_default,
	.reboot = NULL,
	.ctx = NULL
};
	extern void MQTT_SetBit(void);
	extern void MQTT_ResetBit(void);


typedef enum {
    MIIO_WIFI_EVENT_CONFIG_ROUTER = 0,
    MIIO_WIFI_EVENT_SCAN_DONE,
    MIIO_WIFI_EVENT_STA_START,
    MIIO_WIFI_EVENT_STA_STOP,
    MIIO_WIFI_EVENT_STA_CONNECTED,
    MIIO_WIFI_EVENT_STA_DISCONNECTED,
    MIIO_WIFI_EVENT_STA_GOT_IP,
	MIIO_WIFI_EVENT_STA_LOST_IP,
	USER_WIFI_EVENT_SCAN,
	USER_WIFI_EVENT_SCAN_DONE,
}user_wifi_event_type_t;

typedef struct{
	user_wifi_event_type_t type;
}user_wifi_event_t;


int usr_net_event_post(user_wifi_event_type_t type)
{
	user_wifi_event_t event = {
		.type = type
	};
	return arch_os_queue_send(myquehan, &event, ARCH_OS_WAIT_FOREVER);
}


static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
	case SYSTEM_EVENT_SCAN_DONE:
		usr_net_event_post(MIIO_WIFI_EVENT_SCAN_DONE);
		break;
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
       MQTT_SetBit();
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        MQTT_ResetBit();
        break;

    default:
        break;
    }

    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "TP-LINK_HJC",
            .password = "j.c-h@qq.com",
            .channel = 11,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}


arch_os_function_return_t task_monitor1(void *arg)
{

	while(1)
	{
		user_wifi_event_t event;
		if(MIIO_OK == arch_os_queue_recv(myquehan, &event, MIIO_MONITOR_INTERVAL_S*1000))
		{
			switch(event.type)
			{
				
			case MIIO_WIFI_EVENT_SCAN_DONE:
				
				break;
			default:
				break;
			}
		}
	}
}

extern void mqtt_main(void);
/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void app_main(void)
{

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

	//miio_handle_t miio_handle = NULL;

	//miio_handle = miio_instance_create();
	//if(NULL == miio_handle){
	//	LOG_ERROR_TAG("app_main", "miio create error!");
		//return;
	//}
	
	arch_os_queue_create(&myquehan, 4, sizeof(user_wifi_event_t));

	//miio_hooks_register(miio_handle, &s_app_hooks);
	arch_os_thread_create(&myhandle, "netMonitorTask", task_monitor1, 3072, NULL, ARCH_OS_PRIORITY_DEFAULT);
	
	initialise_wifi();

	mqtt_main();
}






