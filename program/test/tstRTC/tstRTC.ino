//  Пример считывания текущего времени:                     //
  #include <Wire.h>                                       // Библиотека iarduino_RTC будет использовать методы и функции библиотеки Wire.
                                                            //
#include "iarduino_RTC.h"                                   // Подключаем библиотеку iarduino_RTC для работы с модулями реального времени.

// RTC
#define CLK 47
#define DAT 48
#define PIN_RST 49

iarduino_RTC watch(RTC_DS1302, PIN_RST, CLK, DAT);   // Объявляем объект watch для работы с RTC модулем на базе чипа DS1302, указывая выводы Arduino подключённые к выводам модуля RST, CLK, DAT.

void setup(){                                               //
    delay(300);                                             // Ждем готовности модуля отвечать на запросы.
    Serial.begin(9600);                                     // Инициируем передачу данных в монитор последовательного порта на скорости 9600 бод.
    watch.begin();                                          // Инициируем работу с модулем.
}                                                           //
void loop(){                                                //
    if(millis()%1000==0){                                   // Если прошла 1 секунда.
      Serial.println(watch.gettime("d-m-Y, H:i:s, D"));     // Выводим время.
      delay(10);                                             // Приостанавливаем скетч на 1 мс, чтоб не выводить время несколько раз за 1мс.
    }                                                       //
}                                                           //
                                                            //
//  ======================================================  //
//    ОПИСАНИЯ ПАРАМЕТРОВ ФУНКЦИЙ:
//
//  • Подключение библиотеки:
//    #include <iarduino_RTC.h>
//    iarduino_RTC watch(название модуля [, вывод SS/RST [, вывод CLK [, вывод DAT]]]);
//        - Если модуль работает на шине I2C или SPI, то достаточно указать 1 параметр, например: iarduino_RTC watch(RTC_DS3231);
//        - Если модуль работает на шине SPI, а аппаратный вывод SS занят, то номер назначенного вывода SS для модуля указывается вторым параметром, например: iarduino_RTC watch(RTC_DS1305,22);
//        - Если модуль работает на трехпроводной шине, то указываются номера всех выводов, например: iarduino_RTC watch(RTC_DS1302, 2, 3, 4); // RST, CLK, DAT.
//  
//  • Для работы с модулями, в библиотеке реализованы 7 функции:
//        - Инициировать модуль  begin();
//        - Указать время        settime(секунды [, минуты [, часы [, день [, месяц [, год [, день недели]]]]]]);
//        - Получить время       gettime(["строка с параметрами"]);
//        - Мигать времем        blinktime(0-не_мигать / 1-мигают_сек / 2-мигают_мин / 3-мигают_час / 4-мигают_дни / 5-мигают_мес / 6-мигает_год / 7-мигают_дни_недели / 8-мигает_полдень);
//        - Разгрузить шину      period(минуты);
//        - Получить Unix время  gettimeUnix();
//        - Указать Unix время   settimeUnix(секунды);
//  
//  • Функция begin():
//        - Функция инициирует модуль: проверяет регистры модуля, запускает генератор модуля и т.д.
//  
//  • Функция settime(секунды [, минуты [, часы [, день [, месяц [, год [, день недели]]]]]]):
//        - Функция записывает время в модуль.
//        - Год указывается без учёта века, в формате 0-99.
//        - Часы указываются в 24-часовом формате, от 0 до 23.
//        - День недели указывается в виде числа: 0-воскресенье, 1-понедельник, 2-вторник ..., 6-суббота.
//        - Если предыдущий параметр надо оставить без изменений, то можно указать отрицательное или заведомо большее значение.
//        - Все параметры, кроме секунд, являются необязательными. Если параметр не указан, значит он не будет изменён.
//        - Пример: watch.settime(-1, 10);               // Установить 10 минут, а секунды, часы и дату, оставить без изменений.
//        - Пример: watch.settime(0, 5, 13);             // Установить 13 часов, 5 минут, 0 секунд, а дату оставить без изменений.
//        - Пример: watch.settime(-1, -1, -1, 9, 2, 17); // Установить дату 09.02.2017, а время и день недели оставить без изменений.
//        - Пример: watch.settime(__TIMESTAMP__);        // Установить дату и время указанную на компьютере с которого выполняется прошивка.
//  
//  • Функция gettime(["строка с параметрами"]):
//        - Функция получает и возвращает строку заменяя описанные ниже символы на текущее время:
//        - Пример: watch.gettime("d-m-Y, H:i:s, D"); // Вернуть строку с датой и временем, например: "01-10-2015, 14:00:05, Thu".
//        - Пример: watch.gettime("s");               // Вернуть строку с секундами, например: "05".
//        - Указанные символы идентичны символам для функции date() в PHP:
//      s - Вернуть секунды                       от      00    до       59  (два знака).
//      i - Вернуть минуты                        от      00    до       59  (два знака).
//      h - Вернуть часы в 12-часовом формате     от      01    до       12  (два знака).
//      H - Вернуть часы в 24-часовом формате     от      00    до       23  (два знака).
//      d - Вернуть день месяца                   от      01    до       31  (два знака).
//      w - Вернуть день недели                   от       0    до        6  (один знак: 0-воскресенье, 6-суббота).
//      D - Вернуть день недели наименование      от     Mon    до      Sun  (три знака: Mon Tue Wed Thu Fri Sat Sun).
//      m - Вернуть месяц                         от      01    до       12  (два знака).
//      M - Вернуть месяц наименование            от     Jan    до      Dec  (три знака: Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec).
//      Y - Вернуть год                           от    2000    до     2099  (четыре знака).
//      y - Вернуть год                           от      00    до       99  (два знака).
//      a - Вернуть полдень                               am   или       pm  (два знака, в нижнем регистре).
//      A - Вернуть полдень                               AM   или       PM  (два знака, в верхнем регистре).
//  
//    Если требуется получить время в виде цифр, то можно вызвать функцию gettime() без параметра, после чего получить время из переменных:
//        - Пример: watch.gettime();
//                  Serial.print(watch.Hours);   Serial.print(":");  // Вывести часы.
//                  Serial.print(watch.minutes); Serial.print(":");  // Вывести минуты.
//                  Serial.print(watch.seconds); Serial.println(""); // Вывести секунды.
//        seconds - Переменная содержит секунды     0-59.
//        minutes - Переменная содержит минуты      0-59.
//        hours   - Переменная содержит часы        1-12.
//        Hours   - Переменная содержит часы        0-23.
//        midday  - Переменная содержит полдень     0-1 (0-am, 1-pm).
//        day     - Переменная содержит день месяца 1-31.
//        weekday - Переменная содержит день недели 0-6 (0-воскресенье, 6-суббота).
//        month   - Переменная содержит месяц       1-12.
//        year    - Переменная содержит год         0-99.
//        Unix    - Переменная содержит секунды     прошедшие с начала эпохи Unix.
//  
//  • Функция blinktime(параметр [, частота ]):
//      - Данная функция указывает функции gettime("строка с параметрами"); мигать одним из параметров времени (заменять параметр пробелами).
//      - Эта функция может быть полезна, для отображения на дисплее, устанавливаемого параметра времени.
//        Например, при установке минут, они начинают мигать, и Вам понятно, что именно Вы устанавливаете.
//      - Первым аргументом функции указывается параметр в виде числа от 0 до 8:
//    0 - не мигать.
//    1 - мигают сек.
//    2 - мигают мин.
//    3 - мигают час.
//    4 - мигают дни.
//    5 - мигают мес.
//    6 - мигает год.
//    7 - мигают дни недели.
//    8 - мигает полдень.
//      - Второй аргумент функции является необязательным, он указвает частоту миганий в Гц, по умолчанию 1Гц.
//      - Пример: watch.blinktime(6);    // При выводе на дисплей будет мигать год с частотой по умолчанию 1Гц.
//      - Пример: watch.blinktime(6, 2); // При выводе на дисплей будет мигать год с частотой 2Гц.
//  
//  • Функция period(минуты):
//      - Устанавливает минимальный период обращения к модулю в минутах (от 0 до 255).
//      - Данная функция указывает функции gettime() откуда брать текущее время: из модуля (не чаще заданного периода), или рассчитать в библиотеке (без обращения к модулю).
//      - Пример: watch.period(10); // Теперь функция gettime() будет получать время от модуля только 1 раз в 10 минут.
//        Ответом на все остальные запросы к функции gettime(), будет рассчитанное время: сумма времени полученного от модуля и времени прошедшего с момента его получения.
//  
//  • Функция gettimeUnix():
//      - Функция возвращает число равное количеству секунд прошедших с начала эпохи Unix (с полуночи 1 января 1970 года).
//  
//  • Функция settimeUnix(секунды):
//      - Функция записывает время в модуль.
//      - В качестве единственного параметра функции указывается количество секунд прошедших с начала эпохи Unix.
//      - Пример: watch.settimeUnix(1577836800); // Установить время на 1577836800 сек больше даты 01.01.1970г 00:00:00, что соответствует дате 01.01.2020г 00:00:00.
//  
//  • Функция settimezone(часовой пояс):
//      - функция принимает часовой пояс для корректной работы функций gettimeUnix() и settimeUnix().
//      - В качестве единственного параметра функции указывается часовой пояс от -12 до 12.
//      - Пример: watch.settimezone(-7); // Установить часовой пояс -7. Если функцию не вызывать, то будет действовать часовой пояс по умолчанию 0.
//      - Пример: watch.settimezone( 7); // Установить часовой пояс +7. Если функцию не вызывать, то будет действовать часовой пояс по умолчанию 0.