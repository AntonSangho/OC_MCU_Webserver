#include <SoftwareSerial.h>

// ESP01 연결 설정 (RX:2, TX:3)
SoftwareSerial esp01(2, 3);
String ipAddress = "";

void setup() {
  // 시리얼 통신 시작
  Serial.begin(9600);
  esp01.begin(9600);
  
  Serial.println("\n=== 초기화 시작 ===");
  
  // ESP01 리셋
  sendCommand("AT+RST", 3000);
  clearBuffer();
  
  // WiFi 모드 설정
  sendCommand("AT+CWMODE=1", 2000);
  clearBuffer();
  
  // WiFi 연결
  Serial.println("WiFi 연결 중...");
  sendCommand("AT+CWJAP=\"ssid\",\"password\"", 10000);
  clearBuffer();
  
  // WiFi 연결 확인을 위해 대기
  delay(5000);
  
  // IP 주소 확인
  Serial.println("IP 주소 확인 중...");
  getIP();
  
  // 멀티커넥션 모드 설정
  sendCommand("AT+CIPMUX=1", 2000);
  clearBuffer();
  
  // 서버 시작
  sendCommand("AT+CIPSERVER=1,80", 2000);
  clearBuffer();
  
  Serial.println("=== 초기화 완료 ===");
  if(ipAddress != "") {
    Serial.println("웹 브라우저에서 접속하세요: http://" + ipAddress);
  } else {
    Serial.println("IP 주소를 가져오지 못했습니다. 리셋 버튼을 눌러주세요.");
  }
  Serial.println("\n=== 서버 대기 중... ===");
}

void getIP() {
  esp01.println("AT+CIFSR");
  delay(2000);
  
  while(esp01.available()) {
    String line = esp01.readStringUntil('\n');
    if(line.indexOf("+CIFSR:STAIP,\"") != -1) {
      int startPos = line.indexOf("\"") + 1;
      int endPos = line.indexOf("\"", startPos);
      ipAddress = line.substring(startPos, endPos);
      Serial.println("IP 주소: " + ipAddress);
      return;
    }
  }
  Serial.println("IP 주소를 찾을 수 없습니다.");
}

void clearBuffer() {
  while(esp01.available()) {
    esp01.read();
  }
}

void loop() {
  // 데이터 수신 확인
  while(esp01.available()) {
    String response = esp01.readStringUntil('\n');
    
    // 새로운 연결 로그
    if(response.indexOf("CONNECT") != -1) {
      Serial.println("\n=== 새로운 클라이언트 연결됨 ===");
    }
    
    // 연결 종료 로그
    if(response.indexOf("CLOSED") != -1) {
      Serial.println("=== 클라이언트 연결 종료 ===\n");
    }
    
    // IPD(데이터 수신) 로그
    if(response.indexOf("+IPD,") != -1) {
      Serial.println("=== 클라이언트 요청 수신 ===");
      Serial.println("수신 데이터: " + response);
      
      // 연결 ID 추출
      int connectionId = response.charAt(response.indexOf("+IPD,") + 5) - '0';
      Serial.println("연결 ID: " + String(connectionId));
      
      // HTML 응답 전송
      String html = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
      html += "<html><head>";
      html += "<title>ESP01 Web Server</title>";
      html += "<style>";
      html += "body { font-family: Arial; text-align: center; margin-top: 50px; }";
      html += "h1 { color: #2c3e50; }";
      html += ".ip { color: #7f8c8d; margin-top: 20px; }";
      html += "</style>";
      html += "</head><body>";
      html += "<h1>Hello, World!</h1>";
      html += "<div class='ip'>Server IP: " + ipAddress + "</div>";
      html += "</body></html>";
      
      // 데이터 전송 시작
      Serial.println("=== HTML 응답 전송 중 ===");
      String cmd = "AT+CIPSEND=";
      cmd += connectionId;
      cmd += ",";
      cmd += html.length();
      
      Serial.println("전송 명령어: " + cmd);
      esp01.println(cmd);
      delay(500);
      
      Serial.println("HTML 데이터 전송");
      esp01.print(html);
      delay(500);
      
      // 연결 종료
      String closeCommand = "AT+CIPCLOSE=";
      closeCommand += connectionId;
      Serial.println("연결 종료 명령어: " + closeCommand);
      esp01.println(closeCommand);
    }
  }
}

void sendCommand(String command, int timeout) {
  Serial.println("명령어 전송: " + command);
  esp01.println(command);
  
  long int time = millis();
  while((time + timeout) > millis()) {
    while(esp01.available()) {
      char c = esp01.read();
      Serial.write(c);
    }
  }
  Serial.println();
}