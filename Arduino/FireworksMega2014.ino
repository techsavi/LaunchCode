
/****************** LED *********************/
const unsigned long ledPatternOff       = 0x00000000;
const unsigned long ledPatternOn        = 0xffffffff;
const unsigned long ledPatternBlink1000 = 0x0000ffff;
const unsigned long ledPatternBlink500  = 0x00ff00ff;
const unsigned long ledPatternBlink250  = 0x0f0f0f0f;
const unsigned long ledPatternBlink125  = 0x33333333;
const unsigned long ledPatternCount1    = 0x00000001;
const unsigned long ledPatternCount2    = 0x00000011;
const unsigned long ledPatternCount3    = 0x00000111;
const unsigned long ledPatternCount4    = 0x00001111;
const unsigned long ledPatternCylon1    = 0xc003c003;
const unsigned long ledPatternCylon2    = 0x300c300c;
const unsigned long ledPatternCylon3    = 0x0c300c30;
const unsigned long ledPatternCylon4    = 0x03c003c0;

const int ledSin = 4;
const int ledClk = 5;
const int ledLat = 6;

unsigned long ledPattern[24];
unsigned int ledDelay = 62; // ms for each 2s repeat

unsigned char XBeeInBuf[256];
unsigned int XBeeInBufLen = 0;

unsigned int powerSw = 0;
unsigned int power12 = 0;

/******* Igniter *******/
const int igniteReadBankSelect1 = 10;
const int igniteReadBankSelect2 = 11;
const int igniteFirstPoint = 22;
const int igniteReadDelay = 1000; // 1 sec update rate
unsigned int igniteReadValue[20];
unsigned long igniteOffTime[20];

const unsigned int igniteActiveMax = 1;
unsigned int igniteActiveCount = 0;

const unsigned int fuseOpenThresh = 100;
const unsigned int fuseShortThresh = 900;
const unsigned int switchThresh = 750;

struct queueEntry {
  unsigned int index;
  unsigned int ms;
};

const unsigned int igniteQueueMax = 20;
queueEntry igniteQueue[igniteQueueMax];
unsigned int igniteQueueCount = 0;


String systemStatus;
String inputBuffer;

/****************** LED *********************/
void ledSetup()
{
  pinMode(ledSin, OUTPUT);
  pinMode(ledClk, OUTPUT);
  pinMode(ledLat, OUTPUT);
  
  for (int i = 0; i < 20; i++)
    ledPattern[i] = ledPatternBlink1000;
  ledPattern[20] = ledPatternCylon1;
  ledPattern[21] = ledPatternCylon2;
  ledPattern[22] = ledPatternCylon3;
  ledPattern[23] = ledPatternCylon4;
}

void ledUpdate()
{
  static unsigned long ledLastTime = 0;
  static unsigned int ledPos = 0;
  unsigned long time = millis();
  if (time - ledLastTime > ledDelay)
  {
    ledLastTime = time;
  } else {
    return;
  }
  
  digitalWrite(ledClk, LOW);

  for (int i = 23; i >= 0; i--)
  {
    bool On = ((ledPattern[i] >> ledPos) & 0x00000001) != 0;
    digitalWrite(ledSin, On ? HIGH : LOW);
    digitalWrite(ledClk, HIGH);
    digitalWrite(ledClk, LOW);
  }

  digitalWrite(ledLat, HIGH);
  digitalWrite(ledLat, LOW);
  if (++ledPos > 31) ledPos = 0;
}

/******* Igniter *******/
void igniteQueueAdd(unsigned int i, unsigned int t)
{
  if (igniteQueueCount >= igniteQueueMax) return;
  
  queueEntry e;
  e.index = i;
  e.ms = t;
  igniteQueue[igniteQueueCount++] = e;
}

void igniteSetup()
{
  // enable feedback selectors, turn off
  pinMode(igniteReadBankSelect1, OUTPUT);
  digitalWrite(igniteReadBankSelect1, LOW);
  pinMode(igniteReadBankSelect1, OUTPUT);
  digitalWrite(igniteReadBankSelect1, LOW);

  // enable output pins and set low
  for (int i = 0; i < 20; i++)
  {
    pinMode(igniteFirstPoint + i, OUTPUT);
    digitalWrite(igniteFirstPoint + i, LOW);
    igniteReadValue[i] = 0;
    igniteOffTime[i] = 0;
  }
}

void igniteReadStates()
{
  int i;

  // do not refresh more frequently than read delay time to minimize ignitor on-time
  static unsigned long igniteReadLastTime = 0;
  unsigned long time = millis();  
  if (time - igniteReadLastTime < igniteReadDelay) return;
  igniteReadLastTime = time;

  power12 = analogRead(A14);
  powerSw = analogRead(A15);
  if (powerSw < switchThresh)
  {
  }

  digitalWrite(igniteReadBankSelect1, LOW);
  for (i = 0; i < 10; i++)
  {
    digitalWrite(igniteFirstPoint + i, HIGH);      // turn on
    delayMicroseconds(50);                         // wait
    int val = igniteReadValue[i] = analogRead(A0 + i);       // read value          
    if (igniteOffTime[i] == 0) {
      digitalWrite(igniteFirstPoint + i, LOW);       // turn off if not active
      if (val < fuseOpenThresh)
        ledPattern[i] = ledPatternCount1;
      else if (val > fuseShortThresh)
        ledPattern[i] = ledPatternCount2;
      else ledPattern[i] = ledPatternOn;
    }
  }
  digitalWrite(igniteReadBankSelect1, HIGH);
  digitalWrite(igniteReadBankSelect2, LOW);
  for (i = 0; i < 10; i++)
  {
    digitalWrite(igniteFirstPoint + i + 10, HIGH);      // turn on
    delayMicroseconds(50);                         // wait
    int val = igniteReadValue[i + 10] = analogRead(A0 + i);  // read value          
    if (igniteOffTime[i + 10] == 0) {
      digitalWrite(igniteFirstPoint + i + 10, LOW);       // turn off
      if (val < fuseOpenThresh)
        ledPattern[i + 10] = ledPatternCount1;
      else if (val > fuseShortThresh)
        ledPattern[i + 10] = ledPatternCount2;
      else ledPattern[i + 10] = ledPatternOn;
    }
  }
  digitalWrite(igniteReadBankSelect2, HIGH);
}

void igniteMonitor()
{
  unsigned long time = millis();
  for (int i = 0; i < 20; i++)
  {
    // skip if already off
    if (igniteOffTime[i] == 0) continue;
    
    // check if time to turn off
    if (time > igniteOffTime[i])
    {
       digitalWrite(igniteFirstPoint + i, LOW);
       igniteOffTime[i] = 0;
       igniteActiveCount--;
       ledPattern[i] = ledPatternBlink1000;
       continue;
    }
    
    // check feedback to see if igniter burned out and shut off
    digitalWrite((i < 10) ? igniteReadBankSelect1 : igniteReadBankSelect2, LOW);
    delayMicroseconds(50);
    unsigned int val = analogRead(A0 + (i % 10));  
    digitalWrite((i < 10) ? igniteReadBankSelect1 : igniteReadBankSelect2, HIGH);
    if (val < fuseOpenThresh) {
       digitalWrite(igniteFirstPoint + i, LOW);
       igniteOffTime[i] = 0;
       igniteActiveCount--;
       ledPattern[i] = ledPatternBlink500;
    }
  }
}

void igniteProcess()
{
  char s[60];
  sprintf(s, "activeCount %d, queueCount %d", igniteActiveCount, igniteQueueCount);
  if (igniteActiveCount >= igniteActiveMax) return;
  if (igniteQueueCount == 0) return;
 
  // pull a request off the queue and turn on ignitor
  queueEntry entry = igniteQueue[0];
  igniteQueueCount--;
  for (unsigned int i = 0; i < igniteQueueCount; i++)
    igniteQueue[i] = igniteQueue[i+1];
    
  // turn on output and mark time for off, set to fast blink
  if ((igniteReadValue[entry.index] > fuseOpenThresh) && (igniteReadValue[entry.index] < fuseShortThresh )) {
    igniteActiveCount++;
    digitalWrite(entry.index + igniteFirstPoint, HIGH);
    unsigned long timeoff = millis() + entry.ms;
    igniteOffTime[entry.index] = timeoff;
    ledPattern[entry.index] = ledPatternBlink125;
    sprintf(s, "Output %d on for %dms\n", entry.index, entry.ms);
    Serial.print(s);
  } else {
    sprintf(s, "Failed to ignite Fuse %d Value = %d", entry.index, igniteReadValue[entry.index]);
    Serial.println(s);
  }
}

void StatusBuild()
{
  char s[16];
  systemStatus = "";
  for (int i = 0; i < 20; i++)
  {
    int on = (igniteOffTime[i] != 0) ? 1 : 0;
//    sprintf(s, "%d,%d,%d,",i,on,igniteReadValue[i]);
    sprintf(s, "%d,%d", i, igniteReadValue[i]);
    systemStatus += s;
    if (i < 19) systemStatus += ",";
  }
//  sprintf(s, "%d,%d", power12, powerSw);
//  systemStatus += s;
}

void serialOutput()
{
  // refresh output 5 seconds
  static unsigned long lastTime = 0;
  unsigned long time = millis();  
  if (time - lastTime < 2000)  return;
  lastTime = time;

  // print the results to the serial monitor:
  StatusBuild();
  Serial.println(systemStatus);
}


String ProcessMessage(String command)
{
  if (command.length() < 1) return "[0]";
  command.toUpperCase();
  char first = command.charAt(0);
  if (first == 'L') {
    char buf[10];
    memset(buf, 0, sizeof(buf));
    command.substring(1).toCharArray(buf, sizeof(buf));
    int idx = atoi(buf);
    igniteQueueAdd(idx, 5000);
    Serial.print("Launch");
    Serial.println(buf);
    return "[1]";
  } else if (first == 'S') {
    StatusBuild();
    return systemStatus;
  } else {
    return "[0]";
  }
}

void parseInput()
{
  int cterm = inputBuffer.indexOf("]");
  if (cterm < 0) {
    return; // no command yet
  }
  int cbegin = inputBuffer.indexOf("[");
  
  String command = inputBuffer.substring(cbegin + 1, cterm);
  inputBuffer = inputBuffer.substring(cterm + 1);
  String reply = ProcessMessage(command);
  Serial2.print(reply);
}

void serialInput()
{
  char c = Serial2.read();
  if (c < 0) return;
  inputBuffer += c;
/*
  static unsigned long lastserial = 0;
  unsigned long t = millis();
  if (t > lastserial + 1000)
  {
    Serial.println(inputBuffer);
  }
*/ 
  parseInput();
}

bool XBeeSendString(String outstr)
{
  unsigned int outstrlen = outstr.length();
  unsigned int packetlen = outstrlen + 14;
  unsigned char buf[256];
  if (packetlen > sizeof(buf) - 4) return false;
  memset(buf, 0, sizeof(buf));

  buf[0] = 0x7e; // frame start
  
  // set frame size
  buf[1] = (packetlen >> 8) & 0xff;
  buf[2] = (packetlen & 0xff);
  
  buf[3] = 0x10; // frame type
  buf[4] = 0x01; // frame ID
  // 64 bit address 0 to send to coordinator, set 16 bit address to unknown
  buf[13] = 0xff;
  buf[14] = 0xfe;
  // leave broadcast radius, options default 0
  
  // copy message contents
  for (unsigned int i = 0; i < outstrlen; i++)
    buf[17 + i] = outstr[i];
    
  // compute checksum
  unsigned char sum = 0;
  for (unsigned int i = 3; i < packetlen + 3; i++)
    sum += buf[i];
  sum = 0xff - sum;
  buf[packetlen + 3] = sum;
 
  // Send message to radio
/*
  Serial.println("Replying");
  for (unsigned int i = 0; i < packetlen + 4; i++) {
    char s[10]; sprintf(s, "0x%02x ", buf[i]); Serial.print(s);
  }
*/

  Serial1.write(buf, packetlen + 4);
}

bool XBeeCheckFrame()
{
  // must start with 0x7e
  if (XBeeInBuf[0] != 0x7e)
  {
    char s[32];
    sprintf(s, "Missing Frame Start 0x%02x", XBeeInBuf[0]);
    memcpy(XBeeInBuf, XBeeInBuf + 1, --XBeeInBufLen);
    Serial.println(s);
    return false;
  }
  
  // check that entire packet received
  if (XBeeInBufLen < 4) 
  {
//    Serial.println("Not Enough Chars");
    return false;
  }
  
  int len = ((int)XBeeInBuf[1]) * 0x0100 + XBeeInBuf[2];
  // check if exceeds buffer, bad message
  if (len > sizeof(XBeeInBuf) - 4) 
  {
    XBeeInBufLen -= 3;  
    memcpy(XBeeInBuf, XBeeInBuf + 3, XBeeInBufLen);  
    return false;
  } 
    
  if (XBeeInBufLen < len + 4)
  {
//     Serial.println("Packet Incomplete");
     return false;
  }

  // verify frame type is Rx Packet
  switch (XBeeInBuf[3]) {
    case 0x91:
    case 0x90: break; // ok continue with rest of function
    case 0x8b: // transmission ack
    {
      if (XBeeInBufLen < 11) return false; // not all here yet
      if (XBeeInBuf[8] != 0) { // failed to transmit last message
        char s[32];
        sprintf(s, "Transmit Error 0x%02x", XBeeInBuf[8]);
        Serial.println(s);
        XBeeInBufLen -= 11;
        memcpy(XBeeInBuf, XBeeInBuf + 11, XBeeInBufLen);
        return false;
      }
    } break;
    default: {
      // discard frame, wait for next one
      XBeeInBufLen = 0;
      Serial.println("Unsupported Packet");
      return false;
    } break;
  }
  
  // optional check source if coordinator
  
  // check if broadcase
  // if (XBeeInBuf[20] != 0x20) {}
  
  // verify checksum
  unsigned char sum = 0;
  for (unsigned int i = 3; i < len + 3; i++) {
    sum += XBeeInBuf[i];
  } 
  sum = 0xff - sum;
  if (sum != XBeeInBuf[len + 3])
  {
    // bad packet, discard
    char s[32];
    sprintf(s, "Checksum Fail %02x vs %02x", sum, XBeeInBuf[len + 3]);
    Serial.println(s);
    memcpy(XBeeInBuf, XBeeInBuf + len + 4, XBeeInBufLen - len - 4);
    XBeeInBufLen -= (len + 4);
    return false;
  }
  
  // else Good Packet! get data
  String payload;
  unsigned int offset = (XBeeInBuf[3] == 0x90) ? 15 : 21;
  for (unsigned int i = offset; i < len + 3; i++)
  {
//    char s[16];
//    sprintf(s, "0x%02x ", XBeeInBuf[i]);
//    Serial.print(s);
    payload += (char)XBeeInBuf[i];
  }
  memcpy(XBeeInBuf, XBeeInBuf + len + 4, XBeeInBufLen - len - 4);
  XBeeInBufLen -= (len + 4);
  
//  Serial.println(payload);
  
  int cterm = payload.indexOf("]");
  int cbegin = payload.indexOf("[");
  if ((cterm > cbegin) && (cbegin >= 0))
  {
    String command = payload.substring(cbegin + 1, cterm);
    String reply = ProcessMessage(command);
    XBeeSendString(reply);
  }
  return true;
}

void XBeeInput()
{
  int c = Serial1.read();
  if (c >= 0) { 
//    char s[10];
//    sprintf(s, "%02x ", c);
//    Serial.print(s);

    if (XBeeInBufLen >= sizeof(XBeeInBuf))
    {
      Serial.println("XBee Buffer Overflow");
    } else {
      XBeeInBuf[XBeeInBufLen++] = c;
      XBeeCheckFrame();
    }
  }
}

void setup() {
  Serial.begin(9600); 
  Serial1.begin(9600);
  Serial2.begin(9600);
  igniteSetup();
  ledSetup();
}

void testSequence()
{
  static int pos = 0;
  if (igniteQueueCount < igniteQueueMax)
  {
    queueEntry e;
    e.index = pos++;
    e.ms = 4000;
    igniteQueue[igniteQueueCount++] = e;
    if (pos >= 20) pos = 0;
  }
}

void loop() {
//  testSequence();
  
  igniteMonitor();
  igniteProcess();
  igniteReadStates();
  ledUpdate();
  
//  serialOutput();
/*
  static unsigned long lastserial = 0;
  unsigned long t = millis();
  if (t > lastserial + 1000)
  {
    lastserial = t;
    Serial1.print("BoardOut1\n");
    Serial2.print("BoardOut2\n");
    int c2 = Serial2.read();
    while (c2 >= 0) {
      Serial.write(c2);
      c2 = Serial2.read();
    }
  }
*/
  XBeeInput();
  serialInput();
}
