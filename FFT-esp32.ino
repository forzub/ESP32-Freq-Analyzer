#include <SPI.h>
#include <TFT_eSPI.h>
#include "arduinoFFT.h"

TFT_eSPI tft = TFT_eSPI();

#define SAMPLES         1024          // Must be a power of 2
// #define SAMPLING_FREQ   4096         // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define SAMPLING_FREQ   8192         // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AMPLITUDE       1000          // Depending on your audio source level, you may need to alter this value. Can be used as a 'sensitivity' control.
#define AUDIO_IN_PIN    32            // Signal in on this pin
#define NOISE           500           // Used as a crude noise filter, values below this are ignored
#define NUM_BANDS       60  
#define START_FQ        145
#define FINISH_FQ       245         


unsigned int sampling_period_us;
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned long newTime;

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQ);


int savedValue = 0;
int countSamples = 0;
int bandSamples = 0;
int startBand = 0;
int finishBand = 0;


void setup() {
  Serial.begin(115200);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);

  
  int factor = 16;
  sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQ));
  
  countSamples = SAMPLES / factor;             // реальное количество частот
  bandSamples = countSamples / NUM_BANDS; // целое значение количества семплов в графическом столбце.
  countSamples = bandSamples * NUM_BANDS; // приведенное к целому отображаемое количество частот (кратно количеству графических столбцов)
  // последний семпл соответствует частоте FREQ_MAX = (SAMPLING_FREQ / 2), Гц 
  // значит каждый семпл vReal[i] содержит полосу шириной ONE_SAMPLE_FREQ = FREQ_MAX / countSamples, Гц
  // каждая графическая полоса содержит частотную полосу шириной ONE_BAND_FREQ = ONE_SAMPLE_FREQ * bandSamples, Гц
  // значит значение START_FQ будет находится в полосе №: START_BAND = START_FQ / ONE_BAND_FREQ;
  // а значение FINISH_FQ - в полосе №: FINISH_BAND = FINISH_FQ / ONE_BAND_FREQ;
  int freqMax = SAMPLING_FREQ / factor;
  int oneSampleFreq = freqMax / countSamples;
  int oneBandFreq = oneSampleFreq * bandSamples;
  startBand = START_FQ / oneBandFreq;
  finishBand = FINISH_FQ / oneBandFreq;

  tft.setCursor(10, 225);  tft.println(0);
  tft.setCursor(50, 225);  tft.println((float)freqMax/3000);
  tft.setCursor(130, 225);  tft.println((float)freqMax/1500);
  tft.setCursor(200, 225);  tft.println((float)freqMax/1000);

  tft.drawLine(0,220,240,220,TFT_WHITE);
  tft.drawLine(80,220, 80,215,TFT_WHITE);
  tft.drawLine(160,220, 160,215,TFT_WHITE);
  tft.fillRect(startBand * 4, 0, (finishBand - startBand) * 4 , 200 ,TFT_DARKGREY);
}


// ==============================================
void loop() {
  
  // Sample the audio pin
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros();
    vReal[i] = analogRead(AUDIO_IN_PIN); // A conversion takes about 43.48uS on an ESP32
    vImag[i] = 0;
    while ((micros() - newTime) < sampling_period_us) { /* chill */ }
  }

  // Compute FFT
  FFT.dcRemoval();
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();




  int maxValue = 0;

  for(int i = 0; i < (countSamples); i++){
    if((int)vReal[i] < NOISE){ vReal[i] = 0; }
    if((int)vReal[i] > maxValue){maxValue = (int)vReal[i];}
  }
  
  int curSample = 0;

  for (int i = 0; i < NUM_BANDS; i++){
    int sampleValue = 0;
    for(int k=0; k < bandSamples; k++){
      sampleValue = sampleValue + vReal[curSample];
      curSample++;
    }
    sampleValue = sampleValue / bandSamples;
    sampleValue = map(sampleValue, 0, maxValue, 0, 200);
  
    tft.fillRect(i*4, 200 - sampleValue, 2, sampleValue, TFT_WHITE);

    if(i > startBand && i < finishBand){
      tft.fillRect(i*4, 0, 2, 200 - sampleValue, TFT_DARKGREY);
      }
    else{
      tft.fillRect(i*4, 0, 2, 200 - sampleValue, TFT_BLACK);
      } 
  }
 
}

  
