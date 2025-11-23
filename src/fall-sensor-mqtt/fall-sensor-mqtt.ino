#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

const char* ssid = "Gengibre";
const char* password = "vomitodegato123";

const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const char* topic_fall  = "sims/sensor/queda";
const char* topic_reset = "sims/sensor/reset";

WiFiClient espClient;
PubSubClient client(espClient);

#define PIN_LED     26
#define PIN_BUZZER  25
#define PIN_BUTTON  27

Adafruit_MPU6050 mpu;
float LIMIAR_FALL = 22.0;

bool activeAlarm = false;


void setup_wifi() {
  delay(100);
  Serial.println("Conectando ao WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  if (String(topic) == topic_reset) {
    Serial.println("Reset remoto recebido via MQTT");
    activeAlarm = false;
    alarmOff();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando ao MQTT...");
    if (client.connect("sensorQuedaESP32")) {
      Serial.println("conectado!");
      client.subscribe(topic_reset);
    } else {
      Serial.print("falha rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 3s");
      delay(3000);
    }
  }
}

void setupSensor() {
  if (!mpu.begin()) {
    Serial.println("MPU6050 não encontrado!");
    while (1) delay(10);
  }
  Serial.println("MPU6050 ok!");
}

bool detectFall() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accTotal = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  );

  Serial.print("Aceleração total: ");
  Serial.println(accTotal);

  return accTotal > LIMIAR_FALL;
}

void publishFallMessage() {
  const char* msg = "{\"evento\":\"queda_detectada\"}";
  client.publish(topic_fall, msg);
  Serial.println("MQTT → queda detectada");
}

void publishResetMessage() {
  const char* msg = "{\"evento\":\"reset_alarme\"}";
  client.publish(topic_reset, msg);
  Serial.println("MQTT → reset alarme");
}

void alarmOn() {
  digitalWrite(PIN_LED, HIGH);
  digitalWrite(PIN_BUZZER, HIGH);
  publishFallMessage();
}

void alarmOff() {
  digitalWrite(PIN_LED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  publishResetMessage();
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  Wire.begin(21, 22);
  setupSensor();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Só detecta nova queda se não estiver em alarme
  if (!activeAlarm && detectFall()) {
    activeAlarm = true;
    alarmOn();
    delay(3000); // evita múltiplos eventos seguidos
  }

  // Botão cancela o alarme
  if (activeAlarm && digitalRead(PIN_BUTTON) == LOW) {
    activeAlarm = false;
    alarmOff();
    delay(500); // debounce simples
  }
}
