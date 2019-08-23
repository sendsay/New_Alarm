#include <Arduino.h>
#include <SoftwareSerial.h>

/*
..#######..########........##..........##....##.....##....###....########.
.##.....##.##.....##.......##.........##.....##.....##...##.##...##.....##
.##.....##.##.....##.......##........##......##.....##..##...##..##.....##
.##.....##.########........##.......##.......##.....##.##.....##.########.
.##.....##.##.....##.##....##......##.........##...##..#########.##...##..
.##.....##.##.....##.##....##.....##...........##.##...##.....##.##....##.
..#######..########...######.....##.............###....##.....##.##.....##
*/

SoftwareSerial SIM800(2, 3);
String _response = "";                      // Переменная для хранения ответа модуля

 /*
 .########.##.....##.##....##..######..########.####..#######..##....##..######.
 .##.......##.....##.###...##.##....##....##.....##..##.....##.###...##.##....##
 .##.......##.....##.####..##.##..........##.....##..##.....##.####..##.##......
 .######...##.....##.##.##.##.##..........##.....##..##.....##.##.##.##..######.
 .##.......##.....##.##..####.##..........##.....##..##.....##.##..####.......##
 .##.......##.....##.##...###.##....##....##.....##..##.....##.##...###.##....##
 .##........#######..##....##..######.....##....####..#######..##....##..######.
 */

String waitResponse() {                         // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                            // Переменная для хранения результата
  long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (SIM800.available()) {                     // Если есть, что считывать...
    _resp = SIM800.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    Serial.println("Timeout...");               // ... оповещаем об этом и...
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}

String sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // Переменная для хранения результата
  Serial.println(cmd);                          // Дублируем команду в монитор порта
  SIM800.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = waitResponse();                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    // if (_resp.startsWith(cmd)) {  // Убираем из ответа дублирующуюся команду
    //   _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    // }
    Serial.println(_resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
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

void setup() {
  Serial.begin(9600);                       // Скорость обмена данными с компьютером
  SIM800.begin(9600);                       // Скорость обмена данными с модемом
  Serial.println("Start!");
  
  sendATCommand("AT", true);                // Автонастройка скорости
  // sendATCommand("AT+CLVL?", true);          // Запрашиваем громкость динамика
  // sendATCommand("AT+CMGF=1", true);         // Включить TextMode для SMS
  sendATCommand("AT+DDET=1,0,1", true);     // Включить DTMF - в этой строке умышленно допущена ошибка - недопустимый параметр
  sendATCommand("AT+COLP=1", true);               

  do {
    _response = sendATCommand("AT+CLIP=1", true);  // Включаем АОН
    _response.trim();                       // Убираем пробельные символы в начале и конце
  } while (_response != "OK");              // Не пускать дальше, пока модем не вернет ОК

  Serial.println("CLI enabled");            // Информируем, что АОН включен
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
  
  if (SIM800.available())   {                   // Если модем, что-то отправил...
    _response = waitResponse();                 // Получаем ответ от модема для анализа
     _response.trim();
    Serial.println(_response);                  // Если нужно выводим в монитор порта
    
    if (_response.startsWith("RING")) {
      sendATCommand("ATA", true);
    }

  }
  if (Serial.available())  {                    // Ожидаем команды по Serial...
    SIM800.write(Serial.read());                // ...и отправляем полученную команду модему
  };



}