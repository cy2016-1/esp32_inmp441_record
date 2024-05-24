#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <driver/i2s.h>

// WiFi和MQTT服务器详情
const char* ssid = "your_SSID";            // WiFi名称
const char* password = "your_PASSWORD";    // WiFi密码
const char* mqtt_server = "your_MQTT_SERVER"; // MQTT服务器地址
const char* mqtt_topic = "audio/data";     // MQTT主题

// MQTT客户端设置
WiFiClient espClient;
PubSubClient client(espClient);

// 音频缓冲区大小
const size_t bufferSize = 1024;
uint8_t buffer[bufferSize];

// 函数原型
void setup_wifi();
void connect();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void i2s_init();
size_t i2s_read(uint8_t* data, size_t size);

void setup() {
    Serial.begin(115200);
    // 连接WiFi
    setup_wifi();
    // 设置MQTT客户端
    client.setServer(mqtt_server, 9983);
    client.setCallback(mqttCallback);
    // 初始化I2S以进行音频处理
    i2s_init();
}

void loop() {
    // 如果需要，重新连接MQTT
    if (!client.connected()) {
        connect();
    }
    client.loop();
    // 记录并发送音频数据
    size_t bytesRead = i2s_read(buffer, bufferSize);
    if (bytesRead > 0) {
        client.publish(mqtt_topic, buffer, bytesRead);
    }
}

void setup_wifi() {
    delay(10);
    Serial.println();
    Serial.print("正在连接到WIFI ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(600);
        Serial.print("-");
    }
    Serial.println("WiFi 已连接");
    Serial.println("IP 地址: ");
    Serial.println(WiFi.localIP());
}

void connect() {
    while (!client.connected()) {
        Serial.print("尝试MQTT连接...");
        if (client.connect("ESP32Client")) {
            Serial.println("已连接");
            client.subscribe(mqtt_topic);
        } else {
            Serial.print("连接失败, 错误代码=");
            Serial.print(client.state());
            Serial.println(" 5秒后重试");
            delay(5000);
        }
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // 处理收到的MQTT消息 扩展开发*** 设置休眠,执行命令等
    Serial.print("消息到达 [");
    Serial.print(topic);
    Serial.print("] ");
    for (unsigned int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
}

void i2s_init() {
    // 配置I2S
    i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 2,
        .dma_buf_len = 1024,
        .use_apll = false
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = 26,
        .ws_io_num = 25,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = 22
    };
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
}

size_t i2s_read(uint8_t* data, size_t size) {
    size_t bytesRead;
    i2s_read(I2S_NUM_0, data, size, &bytesRead, portMAX_DELAY);
    return bytesRead;
}
