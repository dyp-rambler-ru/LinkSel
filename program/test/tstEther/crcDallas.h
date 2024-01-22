// библиотека вычисления контрольной суммы по алгоритму Dallas
//  см.  https://alexgyver.ru/lessons/crc/
// При расчёте CRC перед отправкой нужно исключить байт самого CRC (последний), даже если он нулевой. 
// То есть : byte thisHash = crc8((byte*)&data, sizeof(data) - 1);  , где data - переменная со структурой данных, для которых считается контрольная сумма
// например :
// структура данных посылки
// struct MyData {
//   byte channel;
//   int val_i;
//   float val_f;
//   byte crc;  // байт crc
// };
//  MyData data;
// после применения нужно присвоить data.crc = thisHash;
//
// При расчёте CRC на стороне приёмника можно прогнать все данные полностью, вместе с байтом CRC. В этом случае функция вернёт 0
// , если данные верны!

#pragma once
#include <Arduino.h>

namespace CrcDallas {
byte crc8(byte *buffer, byte size);  // функция вычисления контрольной суммы

}
