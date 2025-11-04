/* ==========================
   ESP8266 Wi-Fi Robot Car + Autopilot Mode + Distance Display
   Components:
   - ESP8266 (NodeMCU/Wemos D1 Mini)
   - L298N Motor Driver
   - 4 DC Motors (2 left, 2 right)
   - Servo (for ultrasonic scanning)
   - HC-SR04 Ultrasonic Sensor
   ========================== */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// ---------- CONFIG ----------
#define USE_AP_MODE true
const char* STA_SSID = "HOGWART";
const char* STA_PASS = "dracarys!!";

const char* AP_SSID = "Rigel";
const char* AP_PASS = "Rigel1234";

// ---------- MOTOR PINS ----------
const uint8_t ENA = 5;   // D1 - Left motor speed
const uint8_t IN1 = 14;  // D5
const uint8_t IN2 = 12;  // D6
const uint8_t ENB = 4;   // D2 - Right motor speed
const uint8_t IN3 = 13;  // D7
const uint8_t IN4 = 16;  // D0

// ---------- AUTOPILOT SENSOR PINS ----------
#define TRIG_PIN 0    // D3
#define ECHO_PIN 15   // D8
#define SERVO_PIN 2   // D4

// ---------- PWM SETTINGS ----------
const int PWM_FREQ = 1000;
const int PWM_RANGE = 1023;
int currentSpeed = 700;

ESP8266WebServer server(80);
Servo servoMotor;

bool autopilotMode = false;  // Flag for autopilot

// ---------- MOTOR FUNCTIONS ----------
void setSpeed(int s) {
  if (s < 0) s = 0;
  if (s > PWM_RANGE) s = PWM_RANGE;
  currentSpeed = s;
  analogWrite(ENA, currentSpeed);
  analogWrite(ENB, currentSpeed);
}

void motorsStop() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void forwardCmd() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setSpeed(currentSpeed);
}

void backwardCmd() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  setSpeed(currentSpeed);
}

void leftCmd() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  setSpeed(currentSpeed);
}

void rightCmd() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  setSpeed(currentSpeed);
}

// ---------- ULTRASONIC ----------
long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  long distance = duration * 0.034 / 2;           // in cm

  // --- NEW FEATURE: Print distance to Serial Monitor ---
  Serial.print("Measured Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  // -----------------------------------------------------

  return distance;
}

// ---------- AUTOPILOT LOGIC ----------
void runAutopilot() {
  if (!autopilotMode) return;

  // Scan forward
  servoMotor.write(90);
  delay(100);
  long frontDist = readDistance();

  if (frontDist > 40 || frontDist == 0) {
    rightCmd();
    return;
  }

  motorsStop();
  delay(100);

  // Scan left
  servoMotor.write(150);
  delay(300);
  long leftDist = readDistance();

  // Scan right
  servoMotor.write(30);
  delay(300);
  long rightDist = readDistance();

  // Center back
  servoMotor.write(90);
  delay(100);

  if (leftDist > rightDist) {
    backwardCmd();
    delay(400);
  } else {
    forwardCmd();
    delay(400);
  }
  motorsStop();
  delay(200);
}

// ---------- WEB PAGE ----------
void handleRoot() {
  String page = R"rawliteral(
<!doctype html>
<html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP Robot Control</title>
<style>
body{font-family:Arial;margin:0;padding:10px;background:#f0f0f0}
.container{max-width:480px;margin:auto;background:#fff;padding:12px;border-radius:10px;box-shadow:0 2px 8px rgba(0,0,0,0.2)}
h2{text-align:center;color:#333}
button{min-width:100px;padding:12px;font-size:18px;border:none;border-radius:8px;margin:6px;cursor:pointer}
.forward{background:#4CAF50;color:white}
.back{background:#f44336;color:white}
.left{background:#2196F3;color:white}
.right{background:#FF9800;color:white}
.stop{background:#555;color:white}
.auto{background:#9C27B0;color:white}
input[type=range]{width:100%}
.label{text-align:center;font-weight:bold}
.status{text-align:center;margin-top:10px;font-weight:bold;color:#222}
footer{text-align:center;font-size:12px;color:#777;margin-top:12px}
</style>
</head>
<body>
<div class="container">
<h2>Your Buddy | RIGEL</h2>
<div style="text-align:center">
<button class="forward" onclick="send('right')">Forward</button><br>
<button class="left" onclick="send('forward')">Left</button>
<button class="right" onclick="send('back')">Right</button>
<button class="back" onclick="send('left')">Backward</button><br>
<button class="stop" onclick="send('stop')">Stop</button><br>
<button class="auto" onclick="toggleAuto()">Autopilot</button>
</div>
<div class="label">Speed: <span id="spdVal">700</span></div>
<input type="range" min="0" max="1023" value="700" id="speed" oninput="updateSpeed(this.value)">
<div class="status" id="autoStatus">Autopilot: OFF</div>
<footer>Connected to <span id="ip">...</span></footer>
</div>
<script>
let auto = false;
function send(cmd){fetch('/move?dir='+cmd).catch(()=>alert('Command failed'));}
function updateSpeed(v){document.getElementById('spdVal').innerText=v;fetch('/speed?val='+v);}
fetch('/whoami').then(r=>r.text()).then(t=>document.getElementById('ip').innerText=t);

function toggleAuto(){
  auto=!auto;
  document.getElementById('autoStatus').innerText="Autopilot: "+(auto?"ON":"OFF");
  fetch('/autopilot?state='+(auto?'on':'off'));
}

document.addEventListener('keydown',e=>{
  const k=e.key.toLowerCase();
  if(k=='w')send('forward');
  if(k=='s')send('back');
  if(k=='a')send('left');
  if(k=='d')send('right');
  if(k==' ')send('stop');
});
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", page);
}

void handleWhoami() {
  String ip = (WiFi.getMode() == WIFI_AP) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  server.send(200, "text/plain", ip);
}

void handleMove() {
  if (!server.hasArg("dir")) { server.send(400, "text/plain", "missing dir"); return; }
  if (autopilotMode) { server.send(200, "text/plain", "Autopilot active"); return; }
  String dir = server.arg("dir");
  if (dir == "forward") forwardCmd();
  else if (dir == "back") backwardCmd();
  else if (dir == "left") leftCmd();
  else if (dir == "right") rightCmd();
  else if (dir == "stop") motorsStop();
  else { server.send(400, "text/plain", "bad dir"); return; }
  server.send(200, "text/plain", "ok");
}

void handleSpeed() {
  if (!server.hasArg("val")) { server.send(400, "text/plain", "missing val"); return; }
  int v = server.arg("val").toInt();
  setSpeed(v);
  server.send(200, "text/plain", String(currentSpeed));
}

void handleAutopilot() {
  if (!server.hasArg("state")) { server.send(400, "text/plain", "missing state"); return; }
  String state = server.arg("state");
  if (state == "on") {
    autopilotMode = true;
    Serial.println("Autopilot ON");
  } else {
    autopilotMode = false;
    motorsStop();
    Serial.println("Autopilot OFF");
  }
  server.send(200, "text/plain", "ok");
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n\n=== ESP8266 ROBOT START ===");

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  analogWriteFreq(PWM_FREQ);
  analogWriteRange(PWM_RANGE);
  motorsStop();

  servoMotor.attach(SERVO_PIN);
  servoMotor.write(90);

  if (USE_AP_MODE) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.print("AP SSID: "); Serial.println(AP_SSID);
    Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin(STA_SSID, STA_PASS);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < 40) {
      delay(500);
      Serial.print(".");
      tries++;
    }
    if (WiFi.status() == WL_CONNECTED)
      Serial.println(WiFi.localIP());
    else {
      WiFi.mode(WIFI_AP);
      WiFi.softAP(AP_SSID, AP_PASS);
    }
  }

  server.on("/", handleRoot);
  server.on("/whoami", handleWhoami);
  server.on("/move", handleMove);
  server.on("/speed", handleSpeed);
  server.on("/autopilot", handleAutopilot);
  server.begin();

  Serial.println("HTTP server started");
}

// ---------- LOOP ----------
void loop() {
  server.handleClient();

  if (autopilotMode) {
    static unsigned long lastAuto = 0;
    if (millis() - lastAuto > 500) {
      runAutopilot();
      lastAuto = millis();
    }
  }
}
