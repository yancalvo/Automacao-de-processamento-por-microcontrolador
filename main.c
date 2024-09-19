#include <WiFi.h>
#include <esp_camera.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <base64.h>

// Credenciais do WiFi
const char* ssid = "nome";
const char* password = "senha";

// Google Cloud Vision API
const char* visionApiEndpoint = "https://vision.googleapis.com/v1/images:annotate?key=chave_da_api";
const char* apiKey = "chave_da_api"; //placeholder para chave da API

// Google Sheets API endpoint
const char* sheetsApiEndpoint = "https://sheets.googleapis.com/v4/spreadsheets/id_da_planilha/values/Sheet1!A1:append?valueInputOption=USER_ENTERED";

// Variável de configuração da câmera
camera_config_t config;

void setup() 
{
  Serial.begin(115200);
  
  // Inicializar WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Conectando...");
  }
  Serial.println("Conectado");

  // Configuração da câmera (OV2640)
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;  // Imagem será capturada em JPEG
  
  // Inicialização de câmera
  if (esp_camera_init(&config) != ESP_OK) 
  {
    Serial.println("Camera initialization failed");
    return;
  }
  Serial.println("Camera initialized");
}

void loop() 
{
  // Captura de imagem
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) 
  {
    Serial.println("Captura falhou");
    return;
  }

  // Converte imagem para Base64
  String base64Image = base64::encode((uint8_t*)fb->buf, fb->len);

  // Liberação de RAM
  esp_camera_fb_return(fb);

  // Utilizando Google Vision OCR para fazer a análise da captura
  String ocrResult = sendToOCR(base64Image);

  // Resultado de análise é enviado para o Google Sheets
  if (!ocrResult.isEmpty()) 
  {
    sendToGoogleSheets(ocrResult);
  }
}

// Enviar captura para o Google Vision OCR
String sendToOCR(String base64Image) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(visionApiEndpoint);  
    
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{ \"requests\": [ { \"image\": { \"content\": \"" + base64Image + "\" }, \"features\": [ { \"type\": \"TEXT_DETECTION\" } ] } ] }";
    
    // Envio de solicitação
    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("OCR Response: " + response);
      
      // Extração de texto
      DynamicJsonDocument doc(4096);  
      deserializeJson(doc, response);
      String ocrText = doc["responses"][0]["textAnnotations"][0]["description"].as<String>();
      
      http.end();
      return ocrText;
    } else {
      Serial.printf("Error code: %d\n", httpResponseCode);
      http.end();
      return "";
    }
  } else {
    Serial.println("WiFi não conectado");
    return "";
  }
}

// Enviar resultado para o Google Sheets
void sendToGoogleSheets(String data) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(sheetsApiEndpoint + "&key=" + apiKey);
    
    http.addHeader("Content-Type", "application/json");

    String jsonPayload = "{ \"values\": [ [\"" + data + "\"] ] }";
    
    // Envio de solicitação
    int httpResponseCode = http.POST(jsonPayload);
    if (httpResponseCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    } else {
      Serial.printf("Error code: %d\n", httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi não conectado");
  }
}

