/* FARMesp ------------------------------------------------------------
  D0=16; D1=5; D2=4; D3=0; D4=2; D5=14; D6=12; D7=13; D8=15; A0=17
-----------------------------------------------------------------------
Desenvolvido por Rodrigo Carvalho
Rod-c-s
FARMesp V1.1
28/02/2021

---------------------------------------------------------------------*/

#include <ESP8266WiFi.h>
#include<ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

#include<DHT.h>
#define DHTpin 0
#define DHTTYPE DHT11// substitua o DHT11 por DHT22 caso esteje usando este 


// chaves de acesso da rede
char* ssid = "*******" ;//armazena ssid da sua rede(substitua os **** pelo seu login)
char* pass = "*******";//armazena a senha da sua rede (substitua os **** pela sua senha)

//constantes de portas
const int sensorSolo = 17;//informa qual sera a constante da porta de entrada do sensor
const int bomba = 5;//informa qual sera a constante da porta da bomba de agua
const int vccSensor = 16;// informa qual sera a constante da porta

//  consts numericas
const int porcent = 4.74;// armazena o valor correspondente para obter a porcentagem a partir da leitura do sensor
const int segundos = 1000;// armazena o valor correspondente para um segundo em ms
const int minuto = 60000;// armazena o valor correspondente para um minuto em ms

//=================================================================================================================//
//
//   EDITE OS VALORES ABAIXO PARA DEFINIR AS CONFIGURAÇÕES DO SOLO
//
//=================================================================================================================//

int valorMin = 25; // valor em porcentagem

//=================================================================================================================//

int tempoIrrigando = 6;// Tempo em segundos que a bomba fica ligada a cada vez que a umidade do solo for detectada como baixa

//=================================================================================================================//

int tempoEntIrrigacao = 1; // Tempo em minutos entre verificações

//=================================================================================================================//



//floats
float umidade = 0;//umidade do ar
float hum = 0;//umidade do solo
float temperatura = 0;//temperatura do ar

WiFiClientSecure client;
ESP8266WebServer server(80);
DHT dht(DHTpin, DHTTYPE);


/*=================================================================================================================/
  +++   CASO QUEIRA REGISTRAR OS DADOS OBTIDOS PELO GOOGLE SHEETS (EXCEL DA GOOGLE) UTILIZANDO O GOOGLE FORMS:    +++
  parte 1:
  1º Crie um planilha no google drive(indentificando com o nome para facilitar a identificação
  2º Crie um Forms no google drive
  3º Configure as peguntas para recebam respostas de forma discertativa (Resposta curta)
  4º Vá em "Respostas" e selecione o destino da resposta como a planilha criada anteiormente

  parte 2:
  5ºAbra o formulário
  6ºClique com o direito e selecione "Exibir código fonte da pagina"
  7ºutilize a ferramenta de pesquisa no navegador(Ctrl+F) e pesquise o nome do campo do fomulario
  8ºEncontre o código parecido com esse:
    FB_PUBLIC_LOAD_DATA_ = [null,[null,[[490158642,”Nome do campo“,null,0,[[NumeroDoCampo,null,0,null,[[1,9,[“”];
  9ºCopie esses numeros(NumeroDoCampo) do formulario e insira no valor correspondente na String abaixo (********)

  10º Verifique se os dados informados correspondem aos dos campos abaixo

  /=================================================================================================================*/

String forms1 = "GET /forms/d/e/1FAIpQLSdz5nJ9VMvICIn8jXRHdUnEPOzbNoNei5uGgkk9LTk8ktjKyQ/formResponse?ifq&entry.********=";//umidade do solo
String forms2 = "GET /forms/d/e/1FAIpQLSdz5nJ9VMvICIn8jXRHdUnEPOzbNoNei5uGgkk9LTk8ktjKyQ/formResponse?ifq&entry.********=";//umidade do ar
String forms3 = "GET /forms/d/e/1FAIpQLSdz5nJ9VMvICIn8jXRHdUnEPOzbNoNei5uGgkk9LTk8ktjKyQ/formResponse?ifq&entry.********=";//temperaturado ar

  /=================================================================================================================*/

void setup() {
  Serial.begin (115200);
  WiFi.mode(WIFI_STA);
  delay(10);

  Serial.println("FARMesp");
  Serial.println(".... Starting Setup");
  Serial.println(" ");

  Serial.println("Conectando a rede");
  Serial.println(ssid);
  Serial.println(" ");

  WiFi.begin(ssid, pass);// conecta-se à rede

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.print("IP:");
  Serial.println(WiFi.localIP());


  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("Servidor HTTP inicializado");


  pinMode(bomba, OUTPUT);
  pinMode(vccSensor, OUTPUT);

  dht.begin();

  ativaIrrig();

}

void loop() {
  server.handleClient();

  ativaIrrig();

  delay(tempoEntIrrigacao * minuto);
}

void handle_NotFound() { //Função para lidar com o erro 404
  server.send(404, "text/plain", "Não encontrado");

}

void handle_OnConnect() {
  temperatura = dht.readTemperature();  //Realiza a leitura da temperatura
  umidade = dht.readHumidity(); //Realiza a leitura da umidade
  getSoilData();

  Serial.print("Temperatura: ");
  Serial.print(temperatura); //Imprime no monitor serial o valor da temperatura lida
  Serial.println(" ºC");
  Serial.print("Umidade: ");
  Serial.print(umidade); //Imprime no monitor serial o valor da umidade lida
  Serial.println(" %");
  Serial.print("Umidade do solo: ");
  Serial.print(hum); //Imprime no monitor serial o valor da umidade lida
  Serial.println(" %");
  server.send(200, "text/html", EnvioHTML(temperatura, umidade, hum)); //Envia as informações usando o código 200, especifica o conteúdo como "text/html" e chama a função EnvioHTML

}

String EnvioHTML(float Temperaturastat, float Umidadestat, float Humstat) { //Exibindo a página da web em HTML
  String ptr = "<!DOCTYPE html> <html>\n"; //Indica o envio do código HTML
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n"; //Torna a página da Web responsiva em qualquer navegador Web
  ptr += "<meta http-equiv='refresh' content='2'>";//Atualizar browser a cada 2 segundos
  ptr += "<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";
  ptr += "<title>FARMesp V1.1 </title>\n"; //Define o título da página

  //Configurações de fonte do título e do corpo do texto da página web
  ptr += "<style>html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #000000;}\n";
  ptr += "body{margin-top: 50px;}\n";
  ptr += "h1 {margin: 50px auto 30px;}\n";
  ptr += "h2 {margin: 40px auto 20px;}\n";
  ptr += "p {font-size: 24px;color: #000000;margin-bottom: 10px;}\n";
  ptr += "</style>\n";
  ptr += "</head>\n";
  ptr += "<body>\n";
  ptr += "<div id=\"webpage\">\n";
  ptr += "<h1>Monitor de Temperatura, Umidade e Umidade do Solo</h1>\n";
  ptr += "<h2>NODEMCU ESP8266 Web Server</h2>\n";

  //Exibe as informações de temperatura e umidade na página web
  ptr += "<p><b>Temperatura: </b>";
  ptr += (float)Temperaturastat;
  ptr += "ºC</p>";
  ptr += "<p><b>Umidade: </b>";
  ptr += (float)Umidadestat;
  ptr += " %</p>";
  ptr += "<p><b>Umidade do solo: </b>";
  ptr += (float)Humstat;
  ptr += " %</p>";

  ptr += "</div>\n";
  ptr += "</body>\n";
  ptr += "</html>\n";
  return ptr;


}


void getSoilData(void) {
  digitalWrite(vccSensor, HIGH);

  delay(1000);

  hum = (100 - (( analogRead(sensorSolo) - 550) / porcent));

  Serial.print("Umidade do solo");
  Serial.print(hum);
  Serial.print("%");
  delay(1000);
  digitalWrite(vccSensor, LOW);
  delay(10);


  client.setInsecure();

  if (client.connect("docs.google.com", 443) == 1)//Tenta se conectar ao servidor do Google docs na porta 443 (HTTPS)
  {
    String toSend = forms1;//Atribuimos a String auxiliar na nova String que sera enviada
    toSend += hum;//Adicionamos um valor aleatorio
    toSend += "&submit=Submit HTTP/1.1";//Completamos o metodo GET para nosso formulario.
    client.println(toSend);//Enviamos o GET ao servidor-
    client.println("Host: docs.google.com");//-
    client.println();//-
    client.stop();//Encerramos a conexao com o servidor
    Serial.println("Dados de umidade do solo enviados.");//Mostra no monitor que foi enviado
  } else {
    Serial.println("Erro ao se conectar");//Se nao for possivel conectar no servidor, ira avisar no monitor.

    delay(50);
  }

  if (client.connect("docs.google.com", 443) == 1)//Tenta se conectar ao servidor do Google docs na porta 443 (HTTPS)
  {
    String toSend = forms2;//Atribuimos a String auxiliar na nova String que sera enviada
    toSend += umidade;//Adicionamos um valor aleatorio
    toSend += "&submit=Submit HTTP/1.1";//Completamos o metodo GET para nosso formulario.
    client.println(toSend);//Enviamos o GET ao servidor-
    client.println("Host: docs.google.com");//-
    client.println();//-
    client.stop();//Encerramos a conexao com o servidor
    Serial.println("Dados de umidade do ar enviados.");//Mostra no monitor que foi enviado
  }
  else {
    Serial.println("Erro ao se conectar");//Se nao for possivel conectar no servidor, ira avisar no monitor.
    delay(50);
  }


  if (client.connect("docs.google.com", 443) == 1)//Tenta se conectar ao servidor do Google docs na porta 443 (HTTPS)
  {
    String toSend = forms3;//Atribui a String auxiliar na nova String que sera enviada
    toSend += temperatura;//Adicion um valor da temperatura
    toSend += "&submit=Submit HTTP/1.1";//Completamos o metodo GET para nosso formulario.
    client.println(toSend);//Envia o GET ao servidor-
    client.println("Host: docs.google.com");//-
    client.println();//-
    client.stop();//Encerra a conexao com o servidor
    Serial.println("Dados de temperatura enviados.");//Mostra no monitor que foi enviado
  }
  else {
    Serial.println("Erro ao se conectar");//Se nao for possivel conectar no servidor, ira avisar no monitor.

    delay(50);

  }
}

void ativaIrrig() {
  delay(100);
  digitalWrite(vccSensor, HIGH);
  delay(2000);

  while ((100 - (( analogRead(sensorSolo) - 550) / porcent)) <= valorMin ) {
    delay(10);
    Serial.print("bomba ligada");
    digitalWrite(bomba, HIGH);
    delay(tempoIrrigando * segundos);
    digitalWrite(bomba, LOW);
    digitalWrite(vccSensor, LOW);
    Serial.print("bomba desligada");


  }
}
