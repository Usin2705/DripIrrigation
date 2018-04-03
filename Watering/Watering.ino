#include <HTTPClient.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "Arduino.h"
#include <EEPROM.h>
#include <PubSubClient.h>
#include "TestedFunc.h"
#include "FreeRTOS\event_groups.h"
#include "FreeRTOS\queue.h"
#include "esp_wifi.h"
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>


// Update these with values suitable for your network.

WiFiClientSecure espClient;//pubsubclient
PubSubClient mqttClient(espClient);
EventGroupHandle_t tasksEventGrp;

#define BIT_0	( 1 << 0 ) //No wifi (unblock wifi initialize task)
#define BIT_1	( 1 << 1 ) //No mqtt (unblock mqtt initialize task)
#define CHECK_WIFI_FLAG BIT_0
#define CHECK_MQTT_FLAG BIT_1

char ssid[] = "FreeWaiFai";
char password[] = "usin27051989";
char mqtt_server[] = "iot.eclipse.org";

void setup() {
	tasksEventGrp = xEventGroupCreate();
	Serial.begin(115200);
	xTaskCreate(taskOne, "TaskOne",  7000,  NULL, 1, NULL);
	xTaskCreate(mqttTask, "mqttTask", 7000, NULL, 1, NULL);
	xTaskCreate(wifiInitializer, "mqttTask", 7000, NULL, 1, NULL);	
	pinMode(36, INPUT);
}

void taskOne(void * parameter)
{
	vTaskDelay(5500);
	while (1) {
		//getTest();
		//postTest();
		//postTestCloudbase();
		//getLongWebTest();
		//vTaskDelete(NULL);
	}
}

void loop() {
	printLocalTime();
	vTaskDelay(30000);
}

void reconnect() {
	// Loop until we're reconnected
	xEventGroupWaitBits(tasksEventGrp, CHECK_MQTT_FLAG, pdFALSE, pdTRUE, portMAX_DELAY);
	if (!mqttClient.connected()) {
		Serial.print("Attempting MQTT connection...");
		// Create a random mqttClient ID
		String clientId = "ESP8266Clientasdfasdfasdfasdf32zadf a3223 ";
		clientId += String(random(0xffff), HEX);
		if (mqttClient.connect(clientId.c_str())) {
			Serial.println("connected");
			mqttClient.subscribe("ESP32_LPC1549_CMD");
			xEventGroupClearBits(tasksEventGrp, CHECK_MQTT_FLAG);
		}
		else {
			Serial.print("failed, rc=");
			Serial.println(mqttClient.state());
		}
	}
}

void wifiInitializer(void* parameter) {
	WiFi.mode(WIFI_STA);
	while (1) {
		xEventGroupWaitBits(tasksEventGrp, CHECK_WIFI_FLAG, pdFALSE, pdTRUE, portMAX_DELAY);
		if (!WiFi.isConnected()) {
			WiFi.disconnect();
			vTaskDelay(pdMS_TO_TICKS(500));
			WiFi.begin(ssid, password);
			//WiFi.waitForConnectResult can use this
			for (int i = 0; i < 7; i++) {
				Serial.print('.');
				vTaskDelay(pdMS_TO_TICKS(1000));
				if (WiFi.isConnected()) {
					esp_wifi_set_ps(WIFI_PS_MODEM);
					Serial.println();
					Serial.println("Connected to WiFi");
					setupTime();
					xEventGroupClearBits(tasksEventGrp, CHECK_WIFI_FLAG);
					xEventGroupSetBits(tasksEventGrp, CHECK_MQTT_FLAG);
					break;
				}
			}
		}
		else {
			xEventGroupClearBits(tasksEventGrp, CHECK_WIFI_FLAG);
			xEventGroupSetBits(tasksEventGrp, CHECK_MQTT_FLAG);
		}
	}
}

void mqttTask(void* parameter) {
	TickType_t lastTime = 0;
	uint32_t minPassed = 0;
	espClient.setCACert(test_root_ca);
	mqttClient.setServer(mqtt_server, 8883);
	mqttClient.setCallback(callback);
	while (1) {
		if (!mqttClient.loop()) {
			xEventGroupSetBits(tasksEventGrp, CHECK_WIFI_FLAG);
			reconnect();
		}
		if (((xTaskGetTickCount() - lastTime)/xPortGetTickRateHz()) >= 60) {
			adc1_config_width(ADC_WIDTH_BIT_10);
			adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

			// SENSOR READING
			gpio_config_t io_conf;
			io_conf.intr_type = GPIO_INTR_DISABLE;
			// *** If GPIO_MODE_OUTPUT, then gpio_get_level(..) for either pin
			// ALWAYS returns 0. Seems you must use GPIO_MODE_INPUT_OUTPUT
			io_conf.mode = GPIO_MODE_OUTPUT;
			io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
			io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
			io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
			ESP_ERROR_CHECK(gpio_config(&io_conf));
			ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_0, 1));
			// give 100 milisecond delay so the sensor can stabilize
			vTaskDelay(100);
			int lemongrassSensorValue = adc1_get_voltage(ADC1_CHANNEL_0);
			int chinacelerySensorValue = adc1_get_voltage(ADC1_CHANNEL_3);
			int rosemarySensorValue = adc1_get_voltage(ADC1_CHANNEL_6);
			int lemongrassPC = map(lemongrassSensorValue, 1023, 320, 0, 100);
			int chineseceleryPC = map(chinacelerySensorValue, 1023, 320, 0, 100);
			int rosemaryPC = map(rosemarySensorValue, 1023, 320, 0, 100);
			Serial.printf("Sensor value %d%%,%d%%,%d%%\n", lemongrassPC, chineseceleryPC, rosemaryPC);
			// turn off GPIO
			ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_0, 0));

			lastTime = xTaskGetTickCount();
			struct tm timeinfo; getLocalTime(&timeinfo);	char timeInfoString[85]; char mqttString[150];			
			strftime(timeInfoString, sizeof(timeInfoString), "%c %Z ", &timeinfo);
			minPassed++;
			snprintf(mqttString, 120, "Lemon Grass: %d%% | Chinese Celery: %d%% | Rosemary: %d%%", lemongrassPC, chineseceleryPC, rosemaryPC);
			Serial.println("####################################################");
			Serial.println(mqttString);
			Serial.println("####################################################");
			mqttClient.publish("ESP32_LPC1549_HOST_IN", mqttString);
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	char payloadstring[MQTT_MAX_PACKET_SIZE + 1]; 
	//for 4096 max can only send approximate 4086 byte back (sometime more sometime less), 
	//maybe should keep payload less than 4000 bytes(safety first)
	memset(payloadstring, '\0', sizeof(payloadstring)); //is this optimal
	memcpy(payloadstring, payload, length);
	/*for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();*/
	mqttClient.publish("ESP32_LPC1549_HOST_OUT", payloadstring);
	//Serial.printf("Send echo success: %d\n", mqttClient.publish("ESP32_LPC1549_HOST_OUT", payloadstring));
}

/*
MQTT 4086 msg length (from send side), esp32 echo back length is 4091 (5 byte from 4096)
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_ending
*/
