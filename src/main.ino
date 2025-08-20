#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
// --- Configurações Wi-Fi ---
const char* ssid = "Rede";
const char* password = "123456789";

// --- Configurações MQTT ---
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* topicoSensor = "Jg/temperatura";

// --- Configuração do DHT11 ---
#define DHTPIN 22
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastMsg = 0; //Armazena o tempo da última mensagem enviada
const long interval = 2000; //Intervalo entre publicações(2 segundos)

//Criação dos objetos de rede e MQTT
WiFiClient espClient;//Objeto do cliente Wi-FI
PubSubClient client(espClient); //Objeto do cliente MQTT utilizando o cliente wifi como parâmetro
void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\n✅ Wi-Fi conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
}


// Função para reconectar ao Broker MQTT (Servidor), se desconectado.
void reconnect(){
  // ID Randomico
  String clientId = "Esp32_" + String(random(0xffff), HEX);
  while(!client.connected()){
    Serial.println("Tentando conectar ao MQTT...");
    if(client.connect(clientId.c_str())){ // Tenta fazer a conexao com o ID Esp32Client 
      Serial.println("Conectado ao Broker.");
    }else{
      Serial.println("Falha ao conectar-se... Tentando novamente em 5s...");
      Serial.print("Código da Falha: ");
      Serial.println(client.state()); // Exibe codigo do erro
      delay(5000);  // Espera para tentar novamente
    }
  }
}

void setup(){
  Serial.begin(115200);
  dht.begin();//Inicializa o sensor DHT22
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop(){
  if (!client.connected()){
    reconnect();
  }
  client.loop(); //Mantém a conexão MQTT ativa

  unsigned long now = millis();
  if(now - lastMsg > interval){
    lastMsg = now;

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if(isnan(h) || isnan(t)){
      Serial.println("Erro ao ler Sensor DHT22");
      return;
    }

    // Criando payload JSON
    String payload = "{";
    payload += "\"temperatura\": " + String(t, 1) + ","; // Um dígito decimal
    payload += "\"umidade\": " + String(h, 1); // Um dígito decimal
    payload += "}";

    Serial.print("Publicando JSON: "); // Mostra o que será enviado
    Serial.println(payload);

    client.publish(topicoSensor, payload.c_str());
  }
}