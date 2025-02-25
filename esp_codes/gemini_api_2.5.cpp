#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <string>
#include "esp_camera.h"
#include "mbedtls/Base64.h"

// Freenove ESP32 CAM PINS
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  21
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    19
#define Y4_GPIO_NUM    18
#define Y3_GPIO_NUM    5
#define Y2_GPIO_NUM    4
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// WiFi credentials
const char* ssid = "SSIDWIFIHERE";
const char* password = "WIFI PASSWORDHERE";

// Create WebServer object on port 80
WebServer server(80);

// Buffer for storing received texts
const int MAX_TEXTS = 10;
String receivedTexts[MAX_TEXTS];
int textIndex = 0;

// API Key for the request
const char* apiKey = "YOUR_GEMINI_API_KEY";

// API endpoint
const char* endpoint = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash:generateContent";

// HTML page with added video stream
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Camera Stream & Speech Control</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 20px auto;
            padding: 20px;
        }
        #camera-container {
            margin: 20px 0;
            text-align: center;
        }
        #camera-stream {
            max-width: 100%;
            height: auto;
            border: 1px solid #ccc;
            border-radius: 5px;
        }
        #status {
            margin: 20px 0;
            padding: 10px;
            border-radius: 5px;
        }
        .listening {
            background-color: #e8f5e9;
            color: #2e7d32;
        }
        .error {
            background-color: #ffebee;
            color: #c62828;
        }
        #transcription {
            width: 100%;
            height: 100px;
            padding: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
        }
        #response {
            margin-top: 20px;
            padding: 15px;
            border: 1px solid #ccc;
            border-radius: 5px;
            min-height: 50px;
            background-color: #f0f0f0;
        }
        button {
            margin-top: 10px;
            padding: 10px 15px;
            border: none;
            border-radius: 5px;
            background-color: #007bff;
            color: white;
            cursor: pointer;
        }
        button:disabled {
            background-color: #ddd;
            cursor: not-allowed;
        }
    </style>
</head>
<body>
    <h1>ESP32 Camera Stream & Speech Control</h1>
    <div id="camera-container">
        <img id="camera-stream" src="/stream" alt="Camera Stream">
    </div>
    <div id="status">Waiting to start...</div>
    <textarea id="transcription"></textarea>
    <button id="record-button">Record</button>
    <button id="send-button" disabled>Send</button>
    <div id="response"></div>

    <script>
        class SpeechHandler {
            constructor() {
                this.recognition = new (window.SpeechRecognition || window.webkitSpeechRecognition)();
                this.setupRecognition();
                this.isListening = false;

                this.statusElement = document.getElementById('status');
                this.transcriptionElement = document.getElementById('transcription');
                this.responseElement = document.getElementById('response');
                this.sendButton = document.getElementById('send-button');
                this.recordButton = document.getElementById('record-button');

                this.bindEventListeners();
            }

            setupRecognition() {
                this.recognition.continuous = true;
                this.recognition.interimResults = true;
                this.recognition.lang = 'en-US';

                this.recognition.onstart = () => {
                    this.isListening = true;
                    this.updateStatus('Listening...', 'listening');
                };

                this.recognition.onend = () => {
                    this.isListening = false;
                    this.updateStatus('Stopped listening', '');
                };

                this.recognition.onresult = (event) => {
                    let finalTranscript = '';
                    for (let i = event.resultIndex; i < event.results.length; i++) {
                        const transcript = event.results[i][0].transcript;
                        if (event.results[i].isFinal) {
                            finalTranscript += transcript + ' ';
                        }
                    }
                    if (finalTranscript) {
                        this.transcriptionElement.value += finalTranscript;
                    }
                };

                this.recognition.onerror = (event) => {
                    this.updateStatus('Error: ' + event.error, 'error');
                };
            }

            bindEventListeners() {
                this.recordButton.addEventListener('click', () => this.startRecording());
                this.sendButton.addEventListener('click', () => this.sendToServer(this.transcriptionElement.value.trim()));
                this.transcriptionElement.addEventListener('input', () => {
                    this.sendButton.disabled = this.transcriptionElement.value.trim() === '';
                });
            }

            startRecording() {
                if (!this.isListening) {
                    try {
                        this.recognition.start();
                    } catch (error) {
                        this.updateStatus('Error starting recognition: ' + error.message, 'error');
                    }
                }
            }

           
            async sendToServer(text) {
    if (!text) {
        this.updateStatus('No text to send', 'error');
        return;
    }

    this.sendButton.disabled = true;

    try {
        const response = await fetch('/speech', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ text })
        });

        if (!response.ok) {
            throw new Error('Network response was not ok');
        }

              const data = await response.json(); // Parse the JSON response

        if (data && data.candidates && data.candidates.length > 0 && data.candidates[0].content && data.candidates[0].content.parts && data.candidates[0].content.parts.length > 0) {
            const responseText = data.candidates[0].content.parts[0].text; // Extract the text
            const parserJsonTrim = responseText.replace(/```json|```/g, '').trim();
            const parsedObject = JSON.parse(parserJsonTrim);
            const reasonText = parsedObject.reason;
            this.responseElement.textContent = reasonText; // Display the text content
        } else {
            this.responseElement.textContent = "No response text found in API response."; // Handle case where text is missing
            console.warn("No response text found in API response:", data); // Log warning
        }

        this.updateStatus('Text sent successfully!', 'listening');
    } catch (error) {
        this.updateStatus('Error: ' + error.message, 'error');
    } finally {
        this.sendButton.disabled = false;
    }
}

            updateStatus(message, className) {
                this.statusElement.textContent = message;
                this.statusElement.className = className;
            }
        }

        // Start the speech handler when the page loads
        window.onload = () => {
            const speechHandler = new SpeechHandler();

            // Set up automatic stream refresh
            const stream = document.getElementById('camera-stream');
            stream.onerror = function () {
                setTimeout(() => {
                    stream.src = '/stream?' + new Date().getTime();
                }, 1000);
            };
        };
    </script>
</body>
</html>

)rawliteral";

// // Function to handle camera stream
// void handleStream() {
//     WiFiClient client = server.client();
    
//     client.println("HTTP/1.1 200 OK");
//     client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
//     client.println();
    
//     while (true) {
//         camera_fb_t * fb = esp_camera_fb_get();
//         if (!fb) {
//             Serial.println("Camera capture failed");
//             esp_camera_fb_return(fb);
//             break;
//         }
        
//         client.println("--frame");
//         client.println("Content-Type: image/jpeg");
//         client.println("Content-Length: " + String(fb->len));
//         client.println();
//         client.write(fb->buf, fb->len);
//         client.println();
        
//         esp_camera_fb_return(fb);
        
//         if (!client.connected()) {
//             break;
//         }
//     }
// }

TaskHandle_t streamTaskHandle = NULL;

// Function that will run on separate core
void streamCameraTask(void *pvParameters) {
    WiFiClient client = *(WiFiClient *)pvParameters;
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println();
    
    while (client.connected()) {
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            break;
        }
        
        client.println("--frame");
        client.println("Content-Type: image/jpeg");
        client.println("Content-Length: " + String(fb->len));
        client.println();
        client.write(fb->buf, fb->len);
        client.println();
        
        esp_camera_fb_return(fb);
        delay(3000);  // Small delay to prevent watchdog triggers
    }
    
    client.stop();
    vTaskDelete(NULL);
}

void handleStream() {
    WiFiClient* client = new WiFiClient(server.client());
    
    // Create task for streaming on Core 0
    xTaskCreatePinnedToCore(
        streamCameraTask,    // Task function
        "StreamTask",        // Task name
        10000,              // Stack size
        (void *)client,     // Parameters to pass
        1,                  // Priority
        &streamTaskHandle,  // Task handle
        0                   // Core to run on (0)
    );
}

std::string parseJsonAndExtractText(const char* response) {
    DynamicJsonDocument doc(4096); // Increased buffer size
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return ""; // Or handle the error as needed
    }

    if (doc.containsKey("candidates") && doc["candidates"].is<JsonArray>()) {
        JsonArray candidates = doc["candidates"].as<JsonArray>();
        if (candidates.size() > 0) {
            JsonObject candidate = candidates[0].as<JsonObject>();
            if (candidate.containsKey("content") && candidate["content"].is<JsonObject>()) {
                JsonObject content = candidate["content"].as<JsonObject>();
                 if (content.containsKey("parts") && content["parts"].is<JsonArray>()) {
                   JsonArray parts = content["parts"].as<JsonArray>();
                    if (parts.size() > 0) {
                         JsonObject part = parts[0].as<JsonObject>();
                        if(part.containsKey("text") && part["text"].is<std::string>()){
                         std::string text = part["text"].as<std::string>();

                         // Remove ```json and ```
                            size_t start_pos = text.find("```json");
                            if (start_pos != std::string::npos) {
                                text.erase(start_pos, 7);
                             }
                            size_t end_pos = text.rfind("```");
                            if (end_pos != std::string::npos) {
                                text.erase(end_pos, 3);
                             }


                            // Remove escape encoding
                            std::string output;
                            for (size_t i = 0; i < text.length(); ++i) {
                                if (text[i] == '\\') {
                                    if (i + 1 < text.length()) {
                                         if(text[i+1] == 'n'){
                                             output += '\n';
                                        }else if(text[i+1] == '"'){
                                            output += '"';
                                        }
                                        i++;
                                    }else{
                                      output+=text[i];
                                    }
                                } else {
                                    output += text[i];
                                }
                            }
                             return output;
                        }
                    }
                 }
                
            }
        }
    }


    return ""; 
}

std::string parseJsonAndExtractCommand(const char* response) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, response);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return ""; // Or handle the error as needed
    }

    if (doc.containsKey("candidates") && doc["candidates"].is<JsonArray>()) {
        JsonArray candidates = doc["candidates"].as<JsonArray>();
        if (candidates.size() > 0) {
            JsonObject candidate = candidates[0].as<JsonObject>();
            if (candidate.containsKey("content") && candidate["content"].is<JsonObject>()) {
                JsonObject content = candidate["content"].as<JsonObject>();
                 if (content.containsKey("parts") && content["parts"].is<JsonArray>()) {
                   JsonArray parts = content["parts"].as<JsonArray>();
                    if (parts.size() > 0) {
                         JsonObject part = parts[0].as<JsonObject>();
                        if(part.containsKey("text") && part["text"].is<const char*>()){
                             const char* text_cstr = part["text"].as<const char*>();
                            std::string text = text_cstr;
                             // Find and extract the command
                            size_t start_pos = text.find("\"command\":");
                            if (start_pos != std::string::npos) {
                                start_pos += 10; // Move past '"command":'

                                size_t end_pos = text.find("}", start_pos);
                                if(end_pos == std::string::npos){
                                   end_pos = text.find("\"", start_pos+1); //if no } is found we search for a double quote
                                }else{
                                   end_pos = text.find("\"", start_pos); // otherwise we look for a double quote
                                }

                                if (end_pos != std::string::npos) {
                                    
                                     std::string command = text.substr(start_pos+1, end_pos-start_pos - 1);
                                       return command;
                                   }
                             }
                        }
                    }
                }
            }
        }
    }
    return "";
}

std::string getvalueextract(const char* json_str) {
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, json_str);

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return ""; // Return an empty string on failure
    }

    if (doc.containsKey("command")) { // Check if the "command" key exists
        const char* commandValue = doc["command"];
        return std::string(commandValue); // Convert char* to std::string

    } else {
        Serial.println("Error: 'command' key not found.");
        return ""; // Return empty string if 'command' key doesn't exist
    }

}

void makeRequest(String userInput, String ImageBase64) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        // Construct the URL with the API key
        String url = String(endpoint) + "?key=" + apiKey;

        // Construct the JSON payload with dynamic input
        String payload = R"({
            "contents": [
                {
                    "role": "user",
                    "parts": [
                        {
                            "text": ")" + userInput + R"("
                        },
                        {
                        "inline_data": {
                            "mime_type": "image/jpeg",
                            "data": ")" + ImageBase64 + R"(",
                            }
                        }
                    ]
                }
            ],
            "systemInstruction": {
                "role": "user",
                "parts": [
                    {
                        "text": "youre an asistent robot, your name is eola, asnwer with this format {\"reason\": text, \"command\": command} reason is the text from prompt, command is controlling motor. your command is only forward, backward, left, right, stop, manual, and follow. example prompt \"who is elaina\" {\"reason\":\"elaina is wangy wangy\",\"command\":\"stop\"}. prompt \"go left and turn back\" {\"reason\":\"going left and then back\", \"command\":\"left, back\"}",
                    }
                ]
            },
            "generationConfig": {
                "temperature": 1,
                "topK": 40,
                "topP": 0.95,
                "maxOutputTokens": 8192,
                "responseMimeType": "text/plain"
            }
        })";

        // Begin the HTTP POST request
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        // Send the request and get the response
        int httpResponseCode = http.POST(payload);
        if (httpResponseCode > 0) {
            String response = http.getString();
            if (response.isEmpty()) {
                server.send(500, "application/json", "{\"status\":\"error\", \"message\":\"Empty API response\"}");
                return;
            }

const char* response_cstr = response.c_str();
            // Parse the JSON response
            DynamicJsonDocument doc(4096); // Increased buffer size
            DeserializationError error = deserializeJson(doc, response);

  std::string extracted_text = parseJsonAndExtractText(response_cstr);
  const char* string_text = extracted_text.c_str();
  std::string extract_value = getvalueextract(string_text);

  if(extracted_text.length() > 0){
      Serial.println(extract_value.c_str());
  }else{
  }

            if (error) {
                Serial.println(error.c_str());
                server.send(500, "application/json", "{\"status\":\"error\", \"message\":\"JSON parsing error: " + String(error.c_str()) + "\"}");
                return;
            }

  
                  
                  if(!response.isEmpty()){
                       // Move dot based on responseText
                       server.send(200, "application/json", response); // Send the raw API response directly
                      return;
                  }

            // If parsing fails or no content is found, send an error response
            server.send(500, "application/json", "{\"status\":\"error\", \"message\":\"Error processing API response\"}");
        }
        else {
            server.send(500, "application/json", "{\"status\":\"error\", \"message\":\"HTTP Request Error\"}");
        }

        // End the HTTP connection
        http.end();
    }
    else {
        server.send(500, "application/json", "{\"status\":\"error\", \"message\":\"Wi-Fi not connected\"}");
    }
}

void handleRoot() {
    server.send(200, "text/html", index_html);
}

void handleSpeech() {
    if (server.hasArg("plain")) {
        String message = server.arg("plain");

        // Capture the last images
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
    }
       // Calculate required buffer size for base64
    size_t out_len;
    mbedtls_base64_encode(NULL, 0, &out_len, fb->buf, fb->len);

    // Allocate buffer for base64 output
    unsigned char *output_buffer = (unsigned char *)malloc(out_len);
    if (!output_buffer) {
        Serial.println("Failed to allocate memory");
        esp_camera_fb_return(fb);
    }

    // Perform base64 encoding
    size_t encoded_size;
    int result = mbedtls_base64_encode(output_buffer, out_len, &encoded_size, fb->buf, fb->len);
    
    String base64Image;
    if (result == 0) {
        base64Image = String((char *)output_buffer);
    } else {
        Serial.println("Base64 encoding failed");
    }

    // Cleanup
    free(output_buffer);
    esp_camera_fb_return(fb);
        // Parse JSON
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, message);

        if (error) {
            server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
            return;
        }

        // Get the text from JSON
        String text = doc["text"].as<String>();

        // Store in circular buffer
        receivedTexts[textIndex] = text;
        textIndex = (textIndex + 1) % MAX_TEXTS;

        // Print to Serial for debugging
        makeRequest(text, base64Image);
   

    } else {
        server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"No data received\"}");
    }
}

void handleNotFound() {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";

    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }

    server.send(404, "text/plain", message);
}
void setup() {
    Serial.begin(115200);
    
    // Initialize camera
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_VGA;  // Changed to VGA for better streaming performance
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 1;

    if (psramFound()) {
        config.jpeg_quality = 10;
        config.fb_count = 2;
        config.grab_mode = CAMERA_GRAB_LATEST;
    }

    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: " + WiFi.localIP().toString());

    // Set up server routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/stream", HTTP_GET, handleStream);
        server.on("/speech", HTTP_POST, []() {
        handleSpeech();
    });
    server.onNotFound(handleNotFound);

    // Start server
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
    delay(2);
}
