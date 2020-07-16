#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "tcpip_adapter.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"


#include <dht.h>
#include <esp_http_server.h>
#include <esp_http_client.h>

static const char* TAG = "example";

esp_err_t _http_event_handle(esp_http_client_event_t* evt) {
	switch (evt->event_id) {
		case HTTP_EVENT_ERROR:
			ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_DATA:
			ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
			break;
		default:
			break;
	}
	return ESP_OK;
}


_Noreturn void temp_send_task(void* args) {
	char char_temp[15] = {0};
	float temperature = 0;
	float humidity = 0;
	esp_http_client_config_t config = {
					.url = "http://165.22.21.64:18580/temp",
					.event_handler = _http_event_handle,
					.method = HTTP_METHOD_POST
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_http_client_set_header(client, "Content-Type", "application/json");
	while (1) {
		dht_read_float_data(DHT_TYPE_DHT11, GPIO_NUM_4, &humidity, &temperature);
		sprintf(char_temp, "{\"temp\":%d}", (int)temperature);
	  esp_http_client_set_post_field(client, char_temp, strlen(char_temp));
		esp_http_client_perform(client);

		ESP_LOGI(TAG, "sent publish successful, temp=%s", char_temp);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

void led(int state) {
	ESP_LOGI(TAG, "LED STATE CHANGED state = %d", state);
	gpio_set_level(2, state);
}

static esp_err_t on_get_handler(httpd_req_t* req) {
	led(1);
	httpd_resp_send(req, "OK", 2);

	return ESP_OK;
}

static esp_err_t off_get_handler(httpd_req_t* req) {
	led(0);
	httpd_resp_send(req, "OK", 2);

	return ESP_OK;
}

static const httpd_uri_t on = {
				.uri       = "/led/on",
				.method    = HTTP_GET,
				.handler   = on_get_handler,
};

static const httpd_uri_t off = {
				.uri       = "/led/off",
				.method    = HTTP_GET,
				.handler   = off_get_handler,
};

static esp_err_t res_handler(httpd_req_t* req) {
	int fd = httpd_req_to_sockfd(req);
	led(0);
	httpd_resp_send(req, "OK", 2);

	return ESP_OK;
}

static const httpd_uri_t res = {
				.uri       = "/",
				.method    = HTTP_GET,
				.handler   = res_handler,
};

static httpd_handle_t start_webserver(void) {
	httpd_handle_t server = NULL;
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.server_port = 8080;

	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (httpd_start(&server, &config) == ESP_OK) {
		ESP_LOGI(TAG, "Registering URI handlers");
		httpd_register_uri_handler(server, &on);
		httpd_register_uri_handler(server, &off);
		return server;
	}

	ESP_LOGI(TAG, "Error starting server!");
	return NULL;
}

static void stop_webserver(httpd_handle_t server) {
	httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	httpd_handle_t* server = (httpd_handle_t*)arg;
	if (*server) {
		ESP_LOGI(TAG, "Stopping webserver");
		stop_webserver(*server);
		*server = NULL;
	}
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	httpd_handle_t* server = (httpd_handle_t*)arg;
	if (*server == NULL) {
		ESP_LOGI(TAG, "Starting webserver");
		*server = start_webserver();
	}
}


void app_main() {
	static httpd_handle_t server = NULL;

	ESP_ERROR_CHECK(nvs_flash_init());
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(example_connect());

	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));

	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
	server = start_webserver();
	xTaskCreate(temp_send_task, "temp_send_task", 2048, NULL, tskIDLE_PRIORITY, NULL);
}
