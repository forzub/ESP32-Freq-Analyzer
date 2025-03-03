# ESP32-Freq-Analyzer
Эквалайзер, анализ аудио частот, FFT. 
# Задача. 
Обнаружение звукового сигнала определенной частоты.
Искомая частота - 195Гц ± 60 Гц.
# Элементы.
Esp32. В качестве выводного устройства - экран TFT ST7789. На первом этапе в качестве источника звука - аудиовыход из компьютера.

# Подключение аудиосигнала.
На первом этапе в качестве источника звука - аудиовыход из компьютера.
Схема подключенияи аудиовыхода для Ардуино:
![image](https://github.com/user-attachments/assets/6a77b3cf-2e2b-44ce-a04e-f3010b631f6a)
Делителем напряжения мы занижаем опорное напряжение на AREF, что бы точнее определять слабый сигнал от аудиовыхода.
Для подключения к ESP32 делитель не нужен. Необходимо использовать аналоговые порты с А32 по А36 включительно и А39.
Остальные не рекомендуется.

# TFT ST7789.
Схема подключения:
![image](https://github.com/user-attachments/assets/24ae4ebc-824d-4910-a259-f82b38bc41ee)

ST7789 | ESP32
----------
GND   ->  GND
VCC   ->  3v3
SCK   ->  D18
SDA   ->  D23
RES   ->  D4
DC    ->  D2

библиотека TFT_eSPI
в файле User_Setup.h разкоментируем строки:
#define ST7789_DRIVER          // Full configuration option, define additional parameters below for this display
#define TFT_SDA_READ           // This option is for ESP32 ONLY, tested with ST7789 and GC9A01 display only
#define TFT_RGB_ORDER TFT_BGR  // Colour order Blue-Green-Red
#define TFT_WIDTH  240         // ST7789 240 x 240 and 240 x 320
#define TFT_HEIGHT 240         // GC9A01 240 x 240

// For ESP32 Dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)

добавляем шрифты, если нужно.
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH

описание библиотеки: https://doc-tft-espi.readthedocs.io/
описание цветов: https://doc-tft-espi.readthedocs.io/tft_espi/colors/

# Логика прошивки и анализ частот.
Библиотека, брал отсюда:
https://github.com/kosme/arduinoFFT
Со стандартной какие-то проблемы.
по теореме Найквеста если мы делаем замеры с частотой F0, то анализировать мы сможем только частоты значением ниже половины значения F0.
поэтому частота SAMPLING_FREQ должна быть в 2 раза больше той, которую мы исследуем.
находим максимальную частоту, с которй ESP32 сканирует АЦП. делаем миллион считываний и засекаем время этой процедуры. Из полученных данных высчитываем: 
Один замер длится 43,46 мкс, следовательно частота - 22,99 кГц. Следовательно, можем исследовать частоты до 10 кГц.
Один замер называется - sample.

исходные данные.
#define SAMPLES         1024         // Must be a power of 2 (должно быть 2,4,8,16,32,64,128,256,512,1024,8192,16384. не соответствие этим значениям приводит к бесконечному циклу)
#define SAMPLING_FREQ   4096         // Гц, должно быть 22000 или меньше из-за времени преобразования АЦП. Определяет максимальную частоту, которую можно проанализировать с помощью БПФ Fmax=sampleF/2.
#define AUDIO_IN_PIN    32           // Пин АЦП
#define NOISE           500          // Сигнал, уровень которого ниже этого значения считается шумом.
#define NUM_BANDS       60           // Количество полос - столбцов в графическом эквалайзере.

сейчас #define SAMPLING_FREQ   1024, т.е. ищем частоты до 512 Гц.
после получения данных амплитуд по частотам vReal[SAMPLES] для отображения берем половину этого массива (хз, так надо)

к примеру:
#define SAMPLES         1024         
#define SAMPLING_FREQ   1024                   
#define NOISE           500          
#define NUM_BANDS       60

после обработки: данные хранятся в массиве vReal[SAMPLES / 2] => 512 значений, крайнее из которых соответствует 512 Гц.
Значит каждый семпл содержит интервал - 1 Гц.
Распределяем семплы по графическим столбцам. в каждом столбце - 8 семплов, и следовательно каждый столбец покрывает полосу 8 Гц.
Амплитуду для каждого графического столбца считаем как среднее арифметическое суммы значений всех 8 семплов.
