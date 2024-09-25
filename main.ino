#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h> // For OLED display

// WiFi credentials
const char* ssid = "sima_ftadi";
const char* password = "@Saiman12345";

// AI21 API endpoint for chat (using j2-ultra)
const char* ai21_endpoint = "https://api.ai21.com/studio/v1/j2-ultra/chat";

// AI21 API Key (replace with your own key)
String apiKey = "UWmBJbehYEHUWXPwyFUb5wXW26ALeXJQ";

// OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Function to make API call to AI21 Labs chat model
String ai21ChatRequest(String userMessage) {
  // Ensure WiFi is connected before proceeding
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Attempting to reconnect...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    int retry_count = 0;
    while (WiFi.status() != WL_CONNECTED && retry_count < 10) {
      delay(1000);
      Serial.print(".");
      retry_count++;
    }

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Failed to reconnect to WiFi.");
      return "Error: WiFi not connected.";
    }
    Serial.println("Reconnected to WiFi.");
  }

  // Set up HTTP client and make the API request
  HTTPClient http;
  http.begin(ai21_endpoint);

  // Setting up headers
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + apiKey);
  http.addHeader("accept", "application/json");

  // JSON payload
  String payload = "{\"numResults\": 1, \"temperature\": 0.2, \"messages\": [{\"text\": \"" + userMessage + "\", \"role\": \"user\"}], \"system\": \"You are an AI assistant chat bot. Your responses must be just one very small line with maximum 10 words and concise.\"}";

  // Send POST request
  int httpResponseCode = http.POST(payload);

  // Handle response or retry
  if (httpResponseCode > 0) {
    String response = http.getString();
    // Parse the response using ArduinoJson
    DynamicJsonDocument doc(4096);
    deserializeJson(doc, response);

    if (doc.containsKey("outputs") && doc["outputs"].size() > 0) {
      String content = doc["outputs"][0]["text"].as<String>(); // Get the text from outputs array
      return content;
    } else if (doc.containsKey("error")) {
      String errorMessage = doc["error"]["message"].as<String>();
      Serial.println("API Error: " + errorMessage);
      return "Error: " + errorMessage;
    }

    return "Error: Unknown response format.";
  } else {
    // Handle HTTP request failure
    Serial.println("Error in HTTP request: " + String(httpResponseCode));
    String response = http.getString();
    Serial.println("Response body: " + response);

    // Retry logic after failed request
    Serial.println("Retrying the API request...");
    delay(5000); // Slight delay before retry
    WiFi.reconnect(); // Ensure WiFi reconnect
    httpResponseCode = http.POST(payload); // Retry request

    if (httpResponseCode > 0) {
      String retryResponse = http.getString();
      DynamicJsonDocument doc(4096);
      deserializeJson(doc, retryResponse);

      if (doc.containsKey("outputs") && doc["outputs"].size() > 0) {
        String content = doc["outputs"][0]["text"].as<String>();
        return content;
      } else if (doc.containsKey("error")) {
        String errorMessage = doc["error"]["message"].as<String>();
        Serial.println("API Error after retry: " + errorMessage);
        return "Error after retry: " + errorMessage;
      }
    } else {
      Serial.println("Retry failed. Error in HTTP request: " + String(httpResponseCode));
      return "Error: HTTP request failed.";
    }
  }

  http.end(); // Close HTTP connection
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64 OLED
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Connecting to WiFi...");
  display.display();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED && retry_count < 10) { // Retry up to 10 times
    delay(1000);
    Serial.print(".");
    display.print(".");
    display.display();
    retry_count++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi.");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("Connected to WiFi");
    display.display();
  } else {
    Serial.println("Failed to connect to WiFi after multiple attempts.");
    display.clearDisplay();
    display.setCursor(0, 10);
    display.println("WiFi failed!");
    display.display();
  }
}

void loop() {
  if (Serial.available() > 0) {
    // Read the input from the Serial Monitor
    String userMessage = Serial.readStringUntil('\n');
    userMessage.trim(); // Remove any extra newlines or spaces

    // Send the input to AI21 Labs and get a response
    String response = ai21ChatRequest(userMessage);

    // Print the AI21 response to the Serial Monitor
    Serial.println("Chitragupta says: ");
    Serial.println(response);

    // Display the response on OLED
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Chitragupta says:");
    display.setCursor(0, 20);
    display.println(response);
    display.display();

    // Wait for a while before accepting new input to avoid spamming the API
    delay(6000);  // 6-second delay to reduce the frequency of API calls
  }
}
