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


// ******************************Bibliotecas**********************************************

#include <ESP8266WiFi.h> //Biblioteca de la esp8266 que permite la comunicacion WiFi
#include <PubSubClient.h> //Biblioteca que permite publicar y suscribirse a los topicos mediate mqtt


// *********************** Datos de Red **************************************************

const char* ssid = "auschwitz";// Pon aquí el nombre de la red a la que deseas conectarte
const char* password = "sa01bi10na"; // Escribe la contraseña de dicha red
const char* mqtt_server = "192.168.1.71"; // Escribe la direccion ip del broker mqtt

//*************** configuracion del sensor TDS (sensor de calidad)************************

#define TdsSensorPin A0       // Declaramos el pin analogico de microcontrolador para recibir los datos del sensor
#define VREF 3.3              // voltaje de referencia analógico (voltios) del ADC
#define SCOUNT  30            // suma de punto de muestra

int analogBuffer[SCOUNT];     // almacenar el valor analógico en la matriz, leer desde ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 23;       // temperatura actual para la compensación NOTA: esta temperatura se establece ya que lo ideal es usar este sensor con otro de temperatura para agua

// *********************** algoritmo de filtrado mediano**********************************

int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}

// *************************** pines del sensor ultrasonico *******************************

const int Trigger = D7; // se declara el pin trigger en el microcontrolador para conectar el pin trig del sensor 
const int Echo = D8; // se declara el pin Echo en el microcontrolador para conectar el pin echo del sensor

// **************** pin del relevador conectado a la bomba de agua ************************

const int bomba = D6; // se declara el pin bomba en el microcontrolador para conectar el pin de señal del relevador

// *************************** pines del caudalimetro *******************************

#define SENSOR  2 // Declaramos el 2 del microontrolador que en este caso en el nodemcu v1 es el pin D4, donde se conecta el pi de señal del caudalimetro

// constantes para obtener el flujo de agua del caudalimetro 
 
long currentMillis = 0;
long previousMillis = 0;
int interval = 1000;
boolean ledState = LOW;
float calibrationFactor = 4.5;
volatile byte pulseCount;
byte pulse1Sec = 0;
float flowRate;
unsigned long flowMilliLitres;
unsigned int totalMilliLitres;
float flowLitres;
float totalLitres;

void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}

WiFiClient espClient; // Este objeto maneja las variables necesarias para una conexion WiFi
PubSubClient client(espClient); // Declramos el cliente que se encargara de a publicacion y recepcion de mensajes

// variables que se necesitan para estructurar un mensaje por medio de mqtt
long lastMsg = 0;
char msg[50];
int value = 0;


// *************************** Iniciar el WiFi *******************************

void setup_wifi() {

// Empezamos por conectarnos a una red WiFi
  delay(10);
  Serial.println();
  Serial.print("Conectando a .....");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Esta es la función que realiz la conexión a WiFi

  while (WiFi.status() != WL_CONNECTED) // Este bucle espera a que se realice la conexión 
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
  Serial.println();

// establecemos las acciones a realizar del topico bomba
  if ((char)payload[0] == '0') // declaramos en 0 e estado del topico  {
    digitalWrite(bomba, LOW);   // en caso de recibir un 0 la bomba no se activa
  } else {
    digitalWrite(bomba, HIGH);  // en caso de recibir un un valo diferente de 0 se activa la bomba en este caso un 1
  }
 
  }


void reconnect() {
  // Bucle para reconectar a la red
  while (!client.connected()) {
    Serial.print("Intentando cnexion mqtt...");
    // Creamos un cliente al azar esp8266
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // intento de conexión
    if (client.connect(clientId.c_str())) {
      Serial.println("conexion mqtt exitosa");
      // Una vez conectado, publicar un anuncio...
      client.publish("salida", "hello world");
      // ... nos resubcribimos
      client.subscribe("Bomba"); // Declaramos el topico Bomba
    } else {
      Serial.print("Fallo la conexion ");
      Serial.print(client.state());
      Serial.println(" Se intentara de nuevo en 5 segundos ");
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);  //Inicializar el pin BUILTIN_LED como salida
   pinMode(bomba, OUTPUT); //Inicializar el pin bomba como salida
  Serial.begin(115200);

 pinMode(TdsSensorPin,INPUT);// Inicializar el pin TdsSensorPin como entrada
  
  pinMode(Trigger, OUTPUT); //Inicializar el pin Trigger como salida
  pinMode(Echo, INPUT);  //Inicializar el pin Echo como entrada
  digitalWrite(Trigger, LOW);//Inicializamos el pin con 0

  pinMode(SENSOR, INPUT_PULLUP); //Inicializar el pin SENSOR como entrada con resistencia pull up ya que el caudalimetro genera interrupciones
 
  pulseCount = 0;
  flowRate = 0.0;
  flowMilliLitres = 0;
  totalMilliLitres = 0;
  previousMillis = 0;
 
  attachInterrupt(digitalPinToInterrupt(SENSOR), pulseCounter, FALLING); // interrupciones que genera el sensor 

  
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

  long t; //timepo que demora en llegar el eco
  long d; //distancia en centimetros
  
//*********************** sensor ultrasonico a prueba de agua *******************************
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

// armamos el mensaje a transmitir por mqtt del sensor ultrasonico
 d;
   String dmes = String(d);
   dmes.toCharArray(msg,50);
   client.publish ("distance",msg);
   Serial.println ("Mensaje enviado->" + String(d));

 //************************ sensor (caudalimetro) ***************************

 currentMillis = millis();
  if (currentMillis - previousMillis > interval) 
  {
    
    pulse1Sec = pulseCount;
    pulseCount = 0;

    /*Debido a que este bucle puede no completarse en exactamente intervalos de 1 segundo, calculamos
    el número de milisegundos que han pasado desde la última ejecución y uso
    eso para escalar la salida. También aplicamos el factor de calibración para escalar la salida
    basado en el número de pulsos por segundo por unidad de medida (litros/minuto eneste caso) 
    proveniente del sensor.*/
 
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
 
    /* Divida el caudal en litros/minuto por 60 para determinar cuántos litros tiene
     pasó por el sensor en este intervalo de 1 segundo, luego multiplique por 1000 para
    convertir a mililitros.*/
    
    flowMilliLitres = (flowRate / 60) * 1000;
    flowLitres = (flowRate / 60);
 
    // Sumar los mililitros pasados ​​en este segundo al total acumulado
    totalMilliLitres += flowMilliLitres;
    totalLitres += flowLitres;
    
    // Imprime el caudal de este segundo en litros/minuto
    Serial.print("Flow rate: ");
    Serial.print(float(flowRate));  
    Serial.print("L/min");
    Serial.println();     

// armamos el mensaje por mqtt del caudalimetro
     String flowRatemes = String(flowRate);
   flowRatemes.toCharArray(msg,50);
   client.publish ("caudal",msg);

  // //************************ sensor TDS (calidad de agua) ***************************

 static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //cada 40 milisegundos, lee el valor analógico del ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //leer el valor analógico y almacenarlo en el búfer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
     // lee el valor analógico más estable por el algoritmo de filtrado mediano y convierte a valor de voltaje
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 1024.0;
      
      //fórmula de compensación de temperatura: fFinalResult(25^C) = fFinalResult(actual)/(1.0+0.02*(fTP-25.0));
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperatura de compensacion
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convertir valor de voltaje a valor tds
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      
      
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
      
 // armamos el mensaje por mqtt del sensor tds
      String tdsValuemes = String(tdsValue);
   tdsValuemes.toCharArray(msg,5);
   client.publish ("calidad",msg);
    }
  }
  
}
  }
}
  
