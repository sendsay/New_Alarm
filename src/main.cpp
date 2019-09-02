#include <Arduino.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

/*
..#######..########........##..........##....##.....##....###....########.
.##.....##.##.....##.......##.........##.....##.....##...##.##...##.....##
.##.....##.##.....##.......##........##......##.....##..##...##..##.....##
.##.....##.########........##.......##.......##.....##.##.....##.########.
.##.....##.##.....##.##....##......##.........##...##..#########.##...##..
.##.....##.##.....##.##....##.....##...........##.##...##.....##.##....##.
..#######..########...######.....##.............###....##.....##.##.....##
*/
SoftwareSerial SIM800(2, 3);                // Setup SIM800
String _response = "";                      // Answer data from SIM800
long lastUpdate = millis();                 // Time last update
long waitCheckAlarm = 0;                    // Wait LED on when check LED alarm
bool flagCheckAlarm = false;                // Flag when check alarm
long updatePeriod   = 60000;                // Check period

String phones = "+37062460972, +37062925050";   // White list
String sendList[] = {"+37062460972"}; // send list 

#define MOV_PIR 11                          // Move sensor
#define CHECK_ALARM 13                      // Check sensor Led
#define ALARM 7                             // Alarm siren
bool alarmFlag = false;                     // Alarm mode flag     
#define ADDRES_FLAG 250                     // Adress flag of mode  
bool sendFlag = false;                       // send SMS flag
int movePin = 0;                            // Move pin

String smsText = "ALARM!!! ALARM!!! ALARM!!!";  // Sms text when Alarm 
bool hasmsg = false;

// #define DEBUG                               // Debug flag;

/*
 .########.##.....##.##....##..######..########.####..#######..##....##..######.
 .##.......##.....##.###...##.##....##....##.....##..##.....##.###...##.##....##
 .##.......##.....##.####..##.##..........##.....##..##.....##.####..##.##......
 .######...##.....##.##.##.##.##..........##.....##..##.....##.##.##.##..######.
 .##.......##.....##.##..####.##..........##.....##..##.....##.##..####.......##
 .##.......##.....##.##...###.##....##....##.....##..##.....##.##...###.##....##
 .##........#######..##....##..######.....##....####..#######..##....##..######.
 */
String waitResponse()
{                                   // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                // Переменная для хранения результата
  long _timeout = millis() + 10000; // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)
  {
  }; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (SIM800.available())
  {                              // Если есть, что считывать...
    _resp = SIM800.readString(); // ... считываем и запоминаем
  }
  else
  {                               // Если пришел таймаут, то...
    Serial.println("Timeout..."); // ... оповещаем об этом и...
  }
  _resp.trim();
  return _resp + "\n"; // ... возвращаем результат. Пусто, если проблема
}

//********************
String sendATCommand(String cmd, bool waiting)
{
  String _resp = "";   // Переменная для хранения результата
  Serial.println(cmd); // Дублируем команду в монитор порта
  _resp = SIM800.println(cmd); // Отправляем команду модулю
  if (waiting)
  {                         // Если необходимо дождаться ответа...
    _resp = waitResponse(); // ... ждем, когда будет передан ответ
    Serial.println(_resp);  // Дублируем ответ в монитор порта
  }
  _resp.trim();
  return _resp + "\n"; // Возвращаем результат. Пусто, если проблема
}

//**********************
void sendSMS(String phone, String message)
{
  sendATCommand("AT+CMGS=\"" + phone + "\"", true);           // Переходим в режим ввода текстового сообщения
  sendATCommand(message + "\r\n" + (String)((char)26), true); // После текста отправляем перенос строки и Ctrl+
  
}

//**********************
void parseSMS(String msg) {                                   // Парсим SMS
  String msgheader  = "";
  String msgbody    = "";
  String msgphone   = "";

  msg = msg.substring(msg.indexOf("+CMGR: "));
  msgheader = msg.substring(0, msg.indexOf("\r"));            // Выдергиваем телефон

  msgbody = msg.substring(msgheader.length() + 2);
  msgbody = msgbody.substring(0, msgbody.lastIndexOf("OK"));  // Выдергиваем текст SMS
  msgbody.trim();

  int firstIndex = msgheader.indexOf("\",\"") + 3;
  int secondIndex = msgheader.indexOf("\",\"", firstIndex);
  msgphone = msgheader.substring(firstIndex, secondIndex);

  if (msgphone.length() > 6 && phones.indexOf(msgphone) > -1) { // Если телефон в белом списке, то...

   Serial.println("White list phon e nubmber");
  }
  else {
    Serial.println("Unknown phonenumber");
    }
}

void checkSMS() {
    if (lastUpdate + updatePeriod < millis() ) {                    // Пора проверить наличие новых сообщений
    do {
      _response = sendATCommand("AT+CMGL=\"REC UNREAD\",1", true);// Отправляем запрос чтения непрочитанных сообщений
      if (_response.indexOf("+CMGL: ") > -1) {                    // Если есть хоть одно, получаем его индекс
        int msgIndex = _response.substring(_response.indexOf("+CMGL: ") + 7, _response.indexOf("\"REC UNREAD\"", _response.indexOf("+CMGL: ")) - 1).toInt();
        char i = 0;                                               // Объявляем счетчик попыток
        do {
          i++;                                                    // Увеличиваем счетчик
          _response = sendATCommand("AT+CMGR=" + (String)msgIndex + ",1", true);  // Пробуем получить текст SMS по индексу
          _response.trim();                                       // Убираем пробелы в начале/конце
          if (_response.endsWith("OK")) {                         // Если ответ заканчивается на "ОК"
            if (!hasmsg) hasmsg = true;                           // Ставим флаг наличия сообщений для удаления
            sendATCommand("AT+CMGR=" + (String)msgIndex, true);   // Делаем сообщение прочитанным
            sendATCommand("\n", true);                            // Перестраховка - вывод новой строки
            parseSMS(_response);                                  // Отправляем текст сообщения на обработку
            break;                                                // Выход из do{}
          }
          else {                                                  // Если сообщение не заканчивается на OK
            Serial.println ("Error answer");                      // Какая-то ошибка
            sendATCommand("\n", true);                            // Отправляем новую строку и повторяем попытку
          }
        } while (i < 10);
        break;
      }
      else {
        lastUpdate = millis();                                    // Обнуляем таймер
        if (hasmsg) {
          sendATCommand("AT+CMGDA=\"DEL READ\"", true);           // Удаляем все прочитанные сообщения
          hasmsg = false;
        }
        break;
      }
    } while (1);
  }
}

void checkSMSsend() {
  if (_response.startsWith("+CMGS:"))
  {                                                                     // Пришло сообщение об отправке SMS
    int index = _response.lastIndexOf("\r\n");                          // Находим последний перенос строки, перед статусом
    String result = _response.substring(index + 2, _response.length()); // Получаем статус
    result.trim();                                                      // Убираем пробельные символы в начале/конце

    if (result == "OK")
    { // Если результат ОК - все нормально

      Serial.println("Message was sent. OK");
      
    }
    else
    { // Если нет, нужно повторить отправку

      Serial.println("Message was not sent. Error");
    
    }
  }
}

void checkSendSMS(String resp){
  if (resp.startsWith("+CMGS:")) {                                                                     // Пришло сообщение об отправке SMS
    int index = resp.lastIndexOf("\r\n");                          // Находим последний перенос строки, перед статусом
    String result = resp.substring(index + 2, resp.length()); // Получаем статус
    result.trim();                                                      // Убираем пробельные символы в начале/конце

    if (result == "OK")
    { // Если результат ОК - все нормально
      Serial.println("Message was sent. OK :");        
    }
    else
    { // Если нет, нужно повторить отправку
      Serial.println("Message was not sent. Error");
    }
  }
}

void checkCall() {
  if (_response.startsWith("RING")) {                                                  // Есть входящий вызов
    int phoneindex = _response.indexOf("+CLIP: \""); // Есть ли информация об определении номера, если да, то phoneindex>-1
    String innerPhone = "";                          // Переменная для хранения определенного номера
    if (phoneindex >= 0)
    {                                                                                    // Если информация была найдена
      phoneindex += 8;                                                                   // Парсим строку и ...
      innerPhone = _response.substring(phoneindex, _response.indexOf("\"", phoneindex)); // ...получаем номер
      Serial.println("Number: " + innerPhone);                                           // Выводим номер в монитор порта
    }
    // Проверяем, чтобы длина номера была больше 6 цифр, и номер должен быть в списке
    if (innerPhone.length() >= 7 && phones.indexOf(innerPhone) >= 0)
    {
      sendATCommand("ATA", true); // Если да, то отвечаем на вызов
    }
    else
    {
      sendATCommand("ATH", true); // Если нет, то отклоняем вызов
    }
  }
}

void checkDTMF() {
  if (_response.startsWith("+DTMF:")) {     
    String symbol = _response.substring(7, 8);

    if (symbol=="1") {      //***Alarm ON
      if (alarmFlag == false) {
        sendATCommand("AT+VTS=1", false);        
         delay(1000);
        sendATCommand("ATH", false);
        delay(200);
        symbol = "";

        while (movePin) {
          movePin = digitalRead(MOV_PIR);
        }
        delay(200);
        Serial.println("Alarm ON!");

        alarmFlag = true;
        EEPROM.update(ADDRES_FLAG, 1);
      }
    }

    if (symbol=="0") {      //***Alarm OFF
      if (alarmFlag == true) {
        sendATCommand("AT+VTS=0", false);
        delay(200);
        Serial.println("Alarm OFF!");
        delay(1000);
        sendATCommand("ATH", false);
        delay(200);
        symbol = "";
        alarmFlag = false;
        digitalWrite(ALARM, HIGH);
        EEPROM.update(ADDRES_FLAG, 0);
      }
    }   
  }
}


/*
..######..########.########.##.....##.########.
.##....##.##..........##....##.....##.##.....##
.##.......##..........##....##.....##.##.....##
..######..######......##....##.....##.########.
.......##.##..........##....##.....##.##.......
.##....##.##..........##....##.....##.##.......
..######..########....##.....#######..##.......
*/
void setup()
{

 // pinMode(MOV_PIR, INPUT);

  pinMode(ALARM, OUTPUT);
  digitalWrite(ALARM, HIGH);
  pinMode(CHECK_ALARM, OUTPUT);
  digitalWrite(CHECK_ALARM, HIGH);

  Serial.begin(9600);
  SIM800.begin(9600);
  SIM800.setTimeout(100);

  Serial.println("Begin!");

  sendATCommand("ATE0", true);          // Echo off
  sendATCommand("AT", true);            // Adjust speed
  sendATCommand("AT+CMGF=1", true);     // Text Mode sms
  sendATCommand("AT+DDET=1", true);     // DTMF on
  sendATCommand("AT+COLP=1", true);     // Line identify
  sendATCommand("AT+CLIP=1", true);     // Caller ID
  sendATCommand("AT+VTD=5", true);

  lastUpdate = millis();

  alarmFlag = EEPROM.read(ADDRES_FLAG);   // Read and set flag mode
  Serial.println("Begin");
}

/*
.##........#######...#######..########.
.##.......##.....##.##.....##.##.....##
.##.......##.....##.##.....##.##.....##
.##.......##.....##.##.....##.########.
.##.......##.....##.##.....##.##.......
.##.......##.....##.##.....##.##.......
.########..#######...#######..##.......
*/



void loop() {

//*** Check sms
  checkSMS();

  if (SIM800.available()) {                            
    _response = waitResponse(); 
    _response.trim();          
    Serial.println(_response);

//*** SMS
  checkSendSMS(_response);

//*** CALL
  checkCall();

//*** DTMF
  checkDTMF();
}

//*** send to serial
  if (Serial.available())
  {                              // Ожидаем команды по Serial...
    SIM800.write(Serial.read()); // ...и отправляем полученную команду модему
  };

/*
....###....##..........###....########..##.....##
...##.##...##.........##.##...##.....##.###...###
..##...##..##........##...##..##.....##.####.####
.##.....##.##.......##.....##.########..##.###.##
.#########.##.......#########.##...##...##.....##
.##.....##.##.......##.....##.##....##..##.....##
.##.....##.########.##.....##.##.....##.##.....##
*/

  movePin = digitalRead(MOV_PIR);
  if (movePin) {

    digitalWrite(CHECK_ALARM, HIGH);

    if (alarmFlag) {            

        while  (sendFlag ) {       
          for (int i = 0; i < (sizeof(sendList)/sizeof(sendList[0])); i++) {

          delay(500);   
          sendSMS(sendList[i], smsText);
          
            do  {
              if (SIM800.available())
              {                             
                _response = waitResponse(); 
                _response.trim();           
              
                if (_response.startsWith("+CMGS:"))
                {                                                                     // Пришло сообщение об отправке SMS
                  int index = _response.lastIndexOf("\r\n");                          // Находим последний перенос строки, перед статусом
                  String result = _response.substring(index + 2, _response.length()); // Получаем статус
                  result.trim();                                                     // Убираем пробельные символы в начале/конце
                  if (result == "OK")
                  { // Если результат ОК - все нормально
                    Serial.println("Message was sent. OK :");  
                    break;
                  }
                  else
                  { // Если нет, нужно повторить отправку
                    Serial.println("Message was not sent. Error");
                    break;
                  }
                }
              }            
            
            } while (_response != "OK");            
        }
        sendFlag = false;
        Serial.println("Alarm LOUD!!!");
        digitalWrite(ALARM, LOW);  
        
        movePin = digitalRead(MOV_PIR);
      }     
    }   
  } else {
    digitalWrite(ALARM, HIGH);
    digitalWrite(CHECK_ALARM, LOW);
    sendFlag = true;
  }  

}

/*
.########.##....##.########.
.##.......###...##.##.....##
.##.......####..##.##.....##
.######...##.##.##.##.....##
.##.......##..####.##.....##
.##.......##...###.##.....##
.########.##....##.########.
*/