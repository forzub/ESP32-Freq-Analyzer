#include <SPI.h>
#include <TFT_eSPI.h>
#include "arduinoFFT.h"
#include <driver/i2s.h>

// вычисляем полосу частот в 1 семпле после вычислений SAMPLING_FREQUENCY / SAMPLES.
// 16000 Гц / 1024 семпла, что примерно соответствует 16 Гц на 1 семпл.
// умножаем 16 Гц на 1024 семпла для кратности числу семплов, получаем SAMPLING_FREQUENCY = 16384 Гц

// После вычислений частот количество семплов частот будет SAMPLES / 2 (512 семплов)
// Этому значению будет соответствовать SAMPLING_FREQUENCY / 2 (8192 Гц)

#define SAMPLES             1024          // Must be a power of 2
#define SAMPLING_FREQUENCY  16384 / 2
#define NOISE  500


// Connections to INMP441 I2S microphone
#define I2S_WS 25
#define I2S_SD 33
#define I2S_SCK 32

// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0

// Define input buffer length
#define bufferLen SAMPLES
int32_t sBuffer[bufferLen];

#define DISPLAY_WIDTH 240            // Ширина дисплея
#define DISPLAY_HEIGHT 200           // Высота дисплея
#define MAX_BARS 80                  // Максимальное количество баров

double vReal[SAMPLES];
double vImag[SAMPLES];

// ===============================================
void i2s_install() {
  const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t) (I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLING_FREQUENCY,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,  // INMP441 отдаёт 32-битные данные
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // INMP441 передаёт звук в левом канале
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 512
    };
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}
// ==================== Set I2S pin configuration =============================
void i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };
  i2s_set_pin(I2S_PORT, &pin_config);
}

TFT_eSPI tft = TFT_eSPI();
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

double F0 = 80;        // Начальная частота
double Fmax = 500;    // Конечная частота
double Fst = 145;     // диапазон выделеный серым - начало
double Ffn = 245;     // диапазон выделеный серым - конец

int numBars = MAX_BARS;
int barsWidth = 0;
int binsInBar = 0;
int startZoneBar = 0;
int endZoneBar = 0;
uint16_t totalBins = 0;
uint16_t startIndex = 0;
uint16_t endIndex = 0;
// =================================================

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  // Вычисляем общее количество бинов (samples) для анализа
  int frequencyStep = SAMPLING_FREQUENCY / SAMPLES;     // количестов Гц в 1 семпле
  startIndex = (uint16_t)(F0 / frequencyStep); // номер семпла с частотой F0
  endIndex = (uint16_t)(Fmax / frequencyStep); // номер семпла с частотой Fmax
  totalBins = endIndex - startIndex;           // общее количество семплов для диапазона частот [F0..Fmax]
  
  Serial.print("frequencyStep ");  Serial.println(frequencyStep);
  Serial.print("startIndex ");  Serial.println(startIndex);
  Serial.print("endIndex ");  Serial.println(endIndex);
  

  // Определяем оптимальное количество баров
  if(totalBins < MAX_BARS){numBars  = totalBins;}       // определяем сколько бар. спойлер - не больше 80.
  barsWidth = round(DISPLAY_WIDTH / numBars);           // опреденляем ширину в пх для 1 бара.
  numBars = round(DISPLAY_WIDTH / barsWidth);           // определяем точное количество баров 80, 60, 48, 40, 34, 30
  
  // Уточнение верхней границы endIndex
  if(totalBins < numBars){totalBins = numBars;}
  binsInBar = (int)(totalBins / numBars);
  totalBins = binsInBar * numBars;
  
  // Fmax = totalBins * frequencyStep;
  Fmax = (startIndex + totalBins) * frequencyStep;
  F0 = startIndex * frequencyStep;

  // определяем границы "серой зоны"
  startZoneBar = (int)((Fst / frequencyStep - startIndex) / binsInBar);
  endZoneBar = startZoneBar + (int)((Ffn / frequencyStep - Fst / frequencyStep ) / binsInBar);

  Serial.print("totalBins ");  Serial.println(totalBins);
  Serial.print("binsInBar ");  Serial.println(binsInBar);
  Serial.print("F0 ");  Serial.println(F0);
  Serial.print("Fmax ");  Serial.println(Fmax);
  Serial.print("numBars ");  Serial.println(numBars);
  Serial.print("startZoneBar ");  Serial.println(startZoneBar);
  Serial.print("endZoneBar ");  Serial.println(endZoneBar);

  tft.setCursor(10, 225);  tft.println( (float)F0/1000, 2 );
  tft.setCursor(68, 225);  tft.println( ((float)F0 + (float)(Fmax - F0) / 3) / 1000, 2);
  tft.setCursor(127, 225);  tft.println( ((float)F0 + (float)(Fmax - F0) / 1.5) / 1000, 2);
  tft.setCursor(190, 225);  tft.println( (float)Fmax/1000, 2 );

  tft.drawLine(0,220,240,220,TFT_WHITE);
  tft.drawLine(80,220, 80,215,TFT_WHITE);
  tft.drawLine(160,220, 160,215,TFT_WHITE);
  tft.fillRect(startZoneBar * barsWidth, 0, (endZoneBar - startZoneBar) * barsWidth , 200 ,TFT_DARKGREY);

// Set up I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
  delay(500);

}

// ==============================================
void loop() {

  // Get I2S data and place in data buffer
 
  size_t bytesIn = 0;
  esp_err_t result = i2s_read(I2S_PORT, sBuffer, bufferLen * sizeof(int32_t), &bytesIn, portMAX_DELAY);

  int16_t samples_read = 0;
  if (result == ESP_OK) {
    samples_read = bytesIn / 4;  // 1 сэмпл = 4 байта (32 бита)
    
    if (samples_read > 0) {
      for (int16_t i = 0; i < samples_read; ++i) {
        vReal[i] = sBuffer[i] >> 8;  // Обрезаем до 24 бит
        vImag[i] = 0;
      }
    }
  }

  // Compute FFT
  FFT.dcRemoval();
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();



  int maxValue = 1370382;

  for(int i = startIndex; i < round(startIndex + totalBins); i++){
    if((int)vReal[i] < NOISE ){ vReal[i] = 0;}
    if((int)vReal[i] > maxValue){ maxValue = (int)vReal[i]; }
  }
 
  
  int curSample = startIndex;

  for (int i = 0; i < numBars; i++){
    int sampleValue = 0;
    for(int k=0; k < binsInBar; k++){
      sampleValue = sampleValue + (int)vReal[curSample];
      curSample++;
    }
    sampleValue = sampleValue / binsInBar;
    sampleValue = map(sampleValue, 0, maxValue, 0, 200);
    if(sampleValue < 0){sampleValue = 0;}
    if(sampleValue > 200){sampleValue = 200;}

    tft.fillRect(i*barsWidth, 200 - sampleValue, barsWidth - 1, sampleValue, TFT_WHITE);

    if(i > startZoneBar && i < endZoneBar){
      tft.fillRect(i*barsWidth, 0, barsWidth - 1, 200 - sampleValue, TFT_DARKGREY);
      }
    else{
      tft.fillRect(i*barsWidth, 0, barsWidth - 1, 200 - sampleValue, TFT_BLACK);
      }
  }
  
 
}

  