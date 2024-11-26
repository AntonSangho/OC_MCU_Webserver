#include <SoftwareSerial.h>
#include <Adafruit_AHTX0.h>

// 기본 설정
SoftwareSerial esp01(2, 3);  // (RX, TX)
Adafruit_AHTX0 aht;
String ipAddress;

void setup() {
  Serial.begin(9600);
  esp01.begin(9600);
  
  Serial.println(F("\n=== 초기화 시작 ==="));

  // 1. AHT20 센서 초기화
  Serial.println(F("센서 초기화..."));
  for(int i=0; i<3; i++) {  // 3번 시도
    if (aht.begin()) {
      Serial.println(F("센서 준비 완료!"));
      break;
    }
    Serial.println(F("센서 초기화 재시도..."));
    delay(1000);
    if(i == 2) {
      Serial.println(F("센서 오류! 점검이 필요합니다."));
      while(1) delay(1000);
    }
  }
  
  // 2. WiFi 설정
  setupWiFi();
  
  Serial.println(F("\n=== 준비 완료 ==="));
  if(ipAddress.length() > 0) {
    Serial.println("IP 주소: " + ipAddress);
    Serial.println("접속 주소: http://" + ipAddress);
  }
}

// WiFi 설정 함수
void setupWiFi() {
  // 1. ESP01 리셋
  Serial.println(F("\n1. ESP01 리셋"));
  if(!sendCommandWithCheck("AT+RST", "OK", 3000)) {
    Serial.println(F("ESP01 리셋 실패"));
    return;
  }
  delay(2000);
  
  // 2. 모드 설정
  Serial.println(F("\n2. 모드 설정"));
  if(!sendCommandWithCheck("AT+CWMODE=1", "OK", 2000)) {
    Serial.println(F("모드 설정 실패"));
    return;
  }
  
  // 3. WiFi 연결
  Serial.println(F("\n3. WiFi 연결"));
  if(!sendCommandWithCheck("AT+CWJAP=\"tresc3-2.4G\",\"tresc333\"", "OK", 10000)) {
    Serial.println(F("WiFi 연결 실패"));
    return;
  }
  
  // 4. IP 확인
  Serial.println(F("\n4. IP 확인"));
  getIP();
  
  // 5. 서버 설정
  Serial.println(F("\n5. 서버 설정"));
  if(!sendCommandWithCheck("AT+CIPMUX=1", "OK", 2000)) {
    Serial.println(F("멀티플렉싱 설정 실패"));
    return;
  }
  if(!sendCommandWithCheck("AT+CIPSERVER=1,80", "OK", 2000)) {
    Serial.println(F("서버 시작 실패"));
    return;
  }
}

// 응답 확인하며 명령어 전송
bool sendCommandWithCheck(const char* cmd, const char* expectedResponse, int timeout) {
  esp01.println(cmd);
  Serial.print(F("명령어 전송: "));
  Serial.println(cmd);
  
  unsigned long startTime = millis();
  String response = "";
  
  while (millis() - startTime < timeout) {
    if (esp01.available()) {
      response += (char)esp01.read();
      if (response.indexOf(expectedResponse) != -1) {
        Serial.println(response);
        return true;
      }
    }
  }
  Serial.println(F("응답 시간 초과"));
  return false;
}

// IP 주소 확인 함수
void getIP() {
  clearBuffer();
  Serial.println(F("IP 주소 확인 중..."));
  esp01.println(F("AT+CIFSR"));
  
  delay(2000);
  String response = "";
  
  while(esp01.available()) {
    char c = esp01.read();
    response += c;
  }
  
  if (response.indexOf("+CIFSR:STAIP,\"") != -1) {
    int startPos = response.indexOf("+CIFSR:STAIP,\"") + 14;
    int endPos = response.indexOf("\"", startPos);
    if (endPos != -1) {
      ipAddress = response.substring(startPos, endPos);
      Serial.println("IP 주소 찾음: " + ipAddress);
    }
  }
}

void loop() {
  if (esp01.available()) {
    String line = esp01.readStringUntil('\n');
    
    if(line.indexOf(F("+IPD,")) != -1) {
      handleWebRequest(line);
    }
  }
}

// 웹 요청 처리 함수
void handleWebRequest(String line) {
  int id = line.charAt(line.indexOf(F("+IPD,")) + 5) - '0';
  Serial.println(F("\n=== 웹 요청 수신 ==="));
  
  // 센서 데이터 읽기 시도
  sensors_event_t humidity, temp;
  if (!aht.getEvent(&humidity, &temp)) {
    Serial.println(F("센서 읽기 실패"));
    return;
  }
  
  // 시리얼 모니터에 출력
  Serial.print(F("온도: "));
  Serial.print(temp.temperature, 1);
  Serial.print(F("°C, 습도: "));
  Serial.print(humidity.relative_humidity, 1);
  Serial.println(F("%"));
  
  // HTML 응답 생성
  String html = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  html += F("<meta charset='utf-8'><meta http-equiv='refresh' content='2'>");
  html += F("<style>body{font-family:Arial;text-align:center;margin:40px;}</style>");
  html += F("<h2>실시간 센서 데이터</h2>");
  html += F("<p style='font-size:20px'>온도: ");
  html += String(temp.temperature, 1);
  html += F("°C</p><p style='font-size:20px'>습도: ");
  html += String(humidity.relative_humidity, 1);
  html += F("%</p>");
  
  // 데이터 전송
  sendData(id, html);
}

// 데이터 전송 함수
void sendData(int id, String data) {
  String sendCommand = "AT+CIPSEND=";
  sendCommand += id;
  sendCommand += ",";
  sendCommand += data.length();
  
  esp01.println(sendCommand);
  delay(100);
  esp01.print(data);
  delay(100);
  esp01.println("AT+CIPCLOSE=" + String(id));
}

// 버퍼 클리어
void clearBuffer() {
  while(esp01.available()) {
    esp01.read();
  }
}