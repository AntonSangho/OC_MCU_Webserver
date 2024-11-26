#include <SoftwareSerial.h>
#include <Adafruit_AHTX0.h>

SoftwareSerial esp01(2, 3);
Adafruit_AHTX0 aht;
String ipAddress;

void setup() {
  Serial.begin(9600);
  esp01.begin(9600);
  
  Serial.println(F("\n=== 초기화 시작 ==="));

  // AHT20 초기화
  if (!aht.begin()) {
    Serial.println(F("센서 오류!"));
    while (1) delay(10);
  }
  
  // 1. 리셋
  Serial.println(F("\n1. ESP01 리셋"));
  sendCommand("AT+RST");
  delay(2000);
  
  // 2. 모드 설정
  Serial.println(F("\n2. 모드 설정"));
  sendCommand("AT+CWMODE=1");
  delay(1000);
  
  // 3. WiFi 연결
  Serial.println(F("\n3. WiFi 연결"));
  sendCommand("AT+CWJAP=\"tresc3-2.4G\",\"tresc333\"");
  delay(8000);
  
  // 4. IP 확인
  Serial.println(F("\n4. IP 확인"));
  delay(2000);
  getIP();
  
  // 5. 서버 설정
  Serial.println(F("\n5. 서버 설정"));
  sendCommand("AT+CIPMUX=1");
  delay(1000);
  sendCommand("AT+CIPSERVER=1,80");
  delay(1000);
  
  Serial.println(F("\n=== 준비 완료 ==="));
  if(ipAddress.length() > 0) {
    Serial.println("IP 주소: " + ipAddress);
    Serial.println("웹 브라우저에서 접속하세요: http://" + ipAddress);
  }
}

void getIP() {

  clearBuffer();
  Serial.println(F("IP 주소 확인 중..."));
  esp01.println(F("AT+CIFSR"));
  
  delay(3000);
  
  String response = "";
  bool foundIP = false;
  //unsigned long startTime = millis();
  
while(esp01.available()) {
  char c = esp01.read();
  response += c;
  Serial.write(c);
  delay(1);
}

Serial.println(F("\n수신된 응답:"));
Serial.println(response);
  
// IP 주소 파싱
int index = response.indexOf(F("+CIFSR:STAIP,\""));
if(index != -1) {
  int startPos = index + 14; // "+CIFSR:STAIP,\"" 길이
  int endPos = response.indexOf("\"", startPos);
  
  if(endPos != -1) {
    ipAddress = response.substring(startPos, endPos);
    Serial.println(F("\nIP 주소 파싱 성공:"));
    Serial.println(ipAddress);
    foundIP = true;
  }
}
  
if(!foundIP) {
  Serial.println(F("\nIP 주소를 찾지 못했습니다. 다시 시도합니다."));
  delay(1000);
  esp01.println(F("AT+CIFSR"));
  delay(3000);
  
  response = "";
  while(esp01.available()) {
    char c = esp01.read();
    response += c;
    Serial.write(c);
    delay(1);
  }
  
  index = response.indexOf(F("+CIFSR:STAIP,\""));
  if(index != -1) {
    int startPos = index + 14;
    int endPos = response.indexOf("\"", startPos);
    
    if(endPos != -1) {
      ipAddress = response.substring(startPos, endPos);
      Serial.println(F("\n두 번째 시도 IP 주소 파싱 성공:"));
      Serial.println(ipAddress);
    }
  }
}
  
  // 최종 결과 출력
  if(ipAddress.length() > 0) {
    Serial.println(F("\nIP 주소 설정 완료: "));
    Serial.println(ipAddress);
  } else {
    Serial.println(F("\nIP 주소 가져오기 실패"));
  }
}

void clearBuffer() {
  while(esp01.available()) {
    esp01.read();
  }
}

void loop() {
  if (esp01.available()) {
    String line = esp01.readStringUntil('\n');
    
    if(line.indexOf(F("+IPD,")) != -1) {
      int id = line.charAt(line.indexOf(F("+IPD,")) + 5) - '0';
      Serial.println(F("\n=== 웹 요청 수신 ==="));
      Serial.println("클라이언트 ID: " + String(id));
      
      // 센서 읽기
      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp);
      
      // 측정값 출력
      Serial.print(F("온도: "));
      Serial.print(temp.temperature, 1);
      Serial.print(F("°C, 습도: "));
      Serial.print(humidity.relative_humidity, 1);
      Serial.println(F("%"));
      
      // HTML 전송
      String html = F("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
      html += F("<meta charset='utf-8'><meta http-equiv='refresh' content='2'>");
      html += F("<h2>센서 데이터</h2>");
      html += F("<p>온도: ");
      html += String(temp.temperature, 1);
      html += F("°C</p><p>습도: ");
      html += String(humidity.relative_humidity, 1);
      html += F("%</p>");
      
      // 전송
      esp01.print(F("AT+CIPSEND="));
      esp01.print(id);
      esp01.print(F(","));
      esp01.println(html.length());
      delay(100);
      
      esp01.print(html);
      delay(100);
      
      esp01.print(F("AT+CIPCLOSE="));
      esp01.println(id);
    }
  }
}

void sendCommand(const char* cmd) {
  esp01.println(cmd);
  Serial.print(F("명령어 전송: "));
  Serial.println(cmd);
  
  delay(500);
  while(esp01.available()) {
    String response = esp01.readStringUntil('\n');
    if(response.length() > 0) {
      Serial.println(response);
    }
  }
}