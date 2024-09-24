/*
 * Programación del nodo sensor cisterna 
 * Por: Equipo 5 
 * Integrantes: Alejandro Alejandro Carrillo, Ernesto Rafael Leon Cornelio y Miquel Antonio Wister Ovando
 * Fecha: 04 de agosto de 2022
 * 
 * En las siguientes lineas de programacion se configura la red de internet, el protocolo mqtt, los topicos para
 * la publicacion y suscripcion de los datos obtenidos por la red de sensores, se declararon las constantes y variables,
 * los pines de entrada y salida.
 */

// ******************************Bibliotecas*********************************************
#include <ESP8266WiFi.h> //Biblioteca de la esp8266 que permite la comunicacion WiFi
#include <PubSubClient.h> //Biblioteca que permite publicar y suscribirse a los topicos mediate mqtt

// *********************** Datos de Red **************************************************
const char* ssid = "auschwitz";// Pon aquí el nombre de la red a la que deseas conectarte
const char* password = "sa01bi10na"; // Escribe la contraseña de dicha red
const char* mqtt_server = "192.168.1.71"; // Escribe la direccion ip del broker mqtt

// *************************** pines del sensor ultrasonico *******************************

const int Trigger = D7; // se declara el pin trigger en el microcontrolador para conectar el pin trig del sensor 
const int Echo = D8; // se declara el pin Echo en el microcontrolador para conectar el pin echo del sensor

WiFiClient espClient; // Este objeto maneja las variables necesarias para una conexion WiFi
PubSubClient client(espClient); // Declramos el cliente que se encargara de a publicacion y recepcion de mensajes

// variables que se necesitan para estructurar un mensaje por medio de mqtt
long lastMsg = 0;
char msg[50];
int value = 0;

// *************************** Iniciar el WiFi *******************************
void setup_wifi() {

  delay(10);
  // Empezamos por conectarnos a una red WiFi
  Serial.println();
  Serial.print("Conectando a .....");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Esta es la función que realiz la conexión a WiFi

  while (WiFi.status() != WL_CONNECTED)// Este bucle espera a que se realice la conexión
  {
    delay(500);
    Serial.print("."); // Indicador de progreso
  }

  randomSeed(micros());

// Cuando se haya logrado la conexión, el programa avanzará, por lo tanto, puede informarse lo siguiente
  Serial.println("");
  Serial.println("Conectado a la red WiFi!");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

 // Esta función permite tomar acciones en caso de que se reciba un mensaje correspondiente a un tema al cual se hará una suscripción
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido bajo el topico de [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  }


void reconnect() {
 // Bucle hasta que estemos reconectados
  while (!client.connected()) {
    Serial.print("Intentando cnexion mqtt...");
    // creamos un id aleatorio
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // intentando conectar
    if (client.connect(clientId.c_str())) {
      Serial.println("conexion mqtt exitosa");
     // Una vez conectado, publicar un anuncio...
      client.publish("salida", "hello world");
      // nos suscribimos
      client.subscribe("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // con este odo no nos suscribimos a ningun cliente por eso se puso cualquier cosa en este campo
    } else {
      Serial.print("Fallo la conexion ");
      Serial.print(client.state());
      Serial.println(" Se intentara de nuevo en 5 segundos ");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT); // Inicializar el pin BUILTIN_LED como salida
  Serial.begin(115200);
  
  pinMode(Trigger, OUTPUT); //Declaramos el pin Trigger como salida
  pinMode(Echo, INPUT);  //// Inicializar el pin Echi como entrada
  digitalWrite(Trigger, LOW);//Inicializamos el pin con 0
  
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// Cuerpo del programa, bucle principal
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (millis() - lastMsg > 2000){
   lastMsg = millis();
   value++;
   String mes = String(value);
   mes.toCharArray(msg,50);
   client.publish ("Valor",msg);
   Serial.println ("Mensaje enviado->" + String(value));

  long t; //timepo que demora en llegar el eco
  long d; //distancia en centimetros

  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);          //Enviamos un pulso de 10us
  digitalWrite(Trigger, LOW);
  
  t = pulseIn(Echo, HIGH); //obtenemos el ancho del pulso
  d = t/59;             //escalamos el tiempo a una distancia en cm
  
  Serial.print("Distancia: ");
  Serial.print(d);      //Enviamos serialmente el valor de la distancia
  Serial.print("cm");
  Serial.println();
  delay(100);          //Hacemos una pausa de 100ms

// Preparamos el mensaje por mqtt con los valores del sensor ultrasonico
 d;
   String dmes = String(d);
   dmes.toCharArray(msg,50);
   client.publish ("distance2",msg);
   Serial.println ("Mensaje enviado->" + String(d));

  }

  }
