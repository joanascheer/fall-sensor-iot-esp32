# Sensor de Queda IoT com ESP32 + MPU6050 + MQTT

Este repositório contém o protótipo de um sistema vestível para detecção de quedas em idosos, desenvolvido como parte da disciplina **Objetos Inteligentes Conectados – 2025/2 (Mackenzie)**. O projeto utiliza um ESP32, um sensor MPU6050 e comunicação MQTT para enviar alertas em tempo real.

---

## 📌 Funcionamento do Projeto

O sistema monitora continuamente a aceleração medida pelo MPU6050. Quando o valor ultrapassa o limiar configurado, o ESP32 interpreta o evento como queda, aciona um LED e um buzzer e publica a mensagem JSON:

```json
{"evento":"queda_detectada"}
```

O alerta pode ser cancelado pressionando um botão tátil, que envia:

```json
{"evento":"reset_alarme"}
```

O envio e recebimento das mensagens é feito pelo protocolo MQTT.

---

## 📂 Estrutura do Repositório

```
/docs
  /images
    - montagem-wokwi.png
    - fluxograma.png
    - mqtt.png
/src
  /fall-sensor-mqtt
    - fall_sensor_mqtt.ino
    - fall_sensor_mqtt_WOKWI.ino
README.md
artigo_final.pdf
```
---

## 🛠 Hardware Utilizado

- ESP32 DevKit V1
- MPU6050 (acelerômetro + giroscópio)
- Buzzer
- LED
- Botão tátil
- Caixa Patola PB-064
- Bateria LiPo 3.7V (opcional)
- Carregador TP4056
- Fios jumpers

---
## 🔌 Interfaces e Protocolo MQTT

Broker utilizado:

```yaml
broker.hivemq.com
Porta: 1883
```

Tópicos:
- ``sims/sensor/queda``
- ``sims/sensor/reset``

O módulo de comunicação utiliza:
- WiFi.h
- PubSubClient.h

Fluxo MQTT:
- Conexão Wi-Fi
- Conexão com broker MQTT
- Publicação JSON
- Recepção de comandos (reset)
---
## 💻 Software e Código
No trecho:
```csharp
const char* ssid = "Sua rede";
const char* password = "Sua senha";
```
adicionar dados correspondentes à sua internet.

Configuração do broker para comunicação MQTT:

```csharp
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const char* topic_fall  = "sims/sensor/queda";
const char* topic_reset = "sims/sensor/reset";
```

Através do código ```float LIMIAR_FALL = 22.0;``` foi determinado o limiar para considerar a movimentação uma queda.

Aqui fazemos a configuração da conexão Wi-Fi
```csharp
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
```

Este calback MQTT permite reset remoto para implementação futura
```csharp
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];

  if (String(topic) == topic_reset) {
    Serial.println("Reset remoto recebido via MQTT");
    activeAlarm = false;
    alarmOff();
  }
}
```

Conexão MQTT
```csharp
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
```
Configuração do MPU6050
```csharp
void setupSensor() {
  if (!mpu.begin()) {
    Serial.println("MPU6050 não encontrado!");
    while (1) delay(10);
  }
  Serial.println("MPU6050 ok!");
}
```
Através do código abaixo, determinamos em que condição o sistema considera a movimentação uma queda:
```csharp
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
```
Com o código:
```csharp
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
```
publicamos os eventos de queda e reset do buzzer.

No loop principal, há uma verificação de conexão, e logo após a lógica, onde há a detecção de queda apenas se o alarme estiver desligado para evitar eventos seguidos.
A seguir, configuração do botão para parar o alarme.
```csharp
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (!activeAlarm && detectFall()) {
    activeAlarm = true;
    alarmOn();
    delay(3000);
  }

  if (activeAlarm && digitalRead(PIN_BUTTON) == LOW) {
    activeAlarm = false;
    alarmOff();
    delay(500);
  }
}
```

## ▶ Vídeo Demonstração

Link do vídeo (não listado no YouTube):
https://youtu.be/KmSFvVBLw5U

---
## 📈 Resultados

#### Desempenhos obtidos:

##### Tempo sensor → atuadores

- Média: 3,15 ms

##### Tempo MQTT → cliente

- Média: 49 ms

Tabelas referentes estão no artigo final.

## 📜 Licença

Uso acadêmico e educacional.