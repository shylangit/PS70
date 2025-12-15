#include "arduinoFFT.h"

// Pin definitions
#define MIC_PIN A0
#define SPEAKER_PIN D1
#define BUTTON_PIN D7

// FFT Configuration
#define SAMPLES 1024                // Samples per FFT analysis
#define SAMPLING_FREQUENCY 4000     
#define MAGNITUDE_THRESHOLD 1000    // Minimal signal strength to be recorded

// Recording settings
#define RECORDING_DURATION 5000     
#define SAMPLE_INTERVAL 200         // Actual ~256 bc FFT
#define MAX_RECORDINGS 25           // 5 samples per second for 5 seconds

ArduinoFFT<double> FFT = ArduinoFFT<double>();

unsigned int sampling_period_us;
double vReal[SAMPLES];
double vImag[SAMPLES];

// Store recorded frequencies
double recordedFrequencies[MAX_RECORDINGS];
int recordingCount = 0;

// Music note names
const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

void setup() {
    Serial.begin(115200);
    
    // Setup pins
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(SPEAKER_PIN, OUTPUT);
    
    // Setup ADC
    analogReadResolution(12);
    analogSetAttenuation(ADC_6db);
    
    sampling_period_us = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
    
    Serial.println("********Frequency Recorder*******");
    Serial.println("Press button to start recording.");
}

void loop() {
    // Wait for button press
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(50); // Debounce
        if (digitalRead(BUTTON_PIN) == LOW) {
            startRecording();
        }
        // Wait for button release
        while(digitalRead(BUTTON_PIN) == LOW) {
            delay(10);
        }
    }
}

void startRecording() {
    Serial.println("\n>>> RECORDING STARTED <<<");
    recordingCount = 0;
    
    unsigned long startTime = millis();
    unsigned long lastSample = 0;
    
    // Record for 5 seconds, sampling every 200ms
    while (millis() - startTime < RECORDING_DURATION) {
        if (millis() - lastSample >= SAMPLE_INTERVAL) {
            lastSample = millis();
            
            double detectedFreq = detectFrequency();
            
            if (detectedFreq > 0) {
                recordedFrequencies[recordingCount] = detectedFreq;
                recordingCount++;
                Serial.println("Sample " + String(recordingCount) + ": " + String(detectedFreq, 1) + " Hz");
            } else {
                Serial.println("Sample " + String(recordingCount + 1) + ": [too weak]");
            }
        }
    }
    
    Serial.println(">>> RECORDING COMPLETE <<<\n");
    
    // Process and playback
    if (recordingCount > 0) {
        // Plays only the note that plays the most frequently.
        double mostFrequentNote = findMode(recordedFrequencies, recordingCount);
        String noteName = frequencyToNote(mostFrequentNote);
        
        Serial.println("Most frequent frequency: " + String(mostFrequentNote, 1) + " Hz (" + noteName + ")");

        playFrequency(mostFrequentNote);
    } else {
        Serial.println("No valid frequencies detected!");
    }
    
    Serial.println("\n===================================");
    Serial.println("Ready for next recording...");
    Serial.println("===================================\n");
}

double detectFrequency() {
    // Sample audio - collects 1024 readings every 250 ms
    for(int i = 0; i < SAMPLES; i++) {
        unsigned long start = micros();
        vReal[i] = analogRead(MIC_PIN);
        vImag[i] = 0; // Used for FFT math
        while(micros() - start < sampling_period_us);
    }
    
    // Remove DC offset
    double mean = 0;
    for(int i = 0; i < SAMPLES; i++) {
        mean += vReal[i];
    }
    mean /= SAMPLES;
    
    for(int i = 0; i < SAMPLES; i++) {
        vReal[i] -= mean;
    }
    
    // Run FFT - idk how the math works, I just followed examples and docs.
    FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);
    
    // Find dominant frequency
    double maxMag = 0;
    int maxIndex = 0;
    
    for(int i = 2; i < SAMPLES/2; i++) {
        if(vReal[i] > maxMag) {
            maxMag = vReal[i];
            maxIndex = i;
        }
    }
    
    // Only return frequency if signal is strong enough
    // Used AI to help improve accuracy of readings
    if(maxMag > MAGNITUDE_THRESHOLD) {
        // Parabolic interpolation for better accuracy
        double y1 = vReal[maxIndex - 1];
        double y2 = vReal[maxIndex];
        double y3 = vReal[maxIndex + 1];
        
        double delta = 0.5 * (y3 - y1) / (2 * y2 - y1 - y3);
        double interpolatedIndex = maxIndex + delta;
        
        return (interpolatedIndex * SAMPLING_FREQUENCY) / SAMPLES;
    }
    
    return 0; // Signal too weak
}

// AI-generated mode function
double findMode(double frequencies[], int count) {
    if (count == 0) return 0;
    if (count == 1) return frequencies[0];
    
    // Group frequencies (+/- 5Hz tolerance)
    const int TOLERANCE = 5;
    double bins[MAX_RECORDINGS];
    int binCounts[MAX_RECORDINGS];
    int binCount = 0;
    
    // Create frequency bins
    for (int i = 0; i < count; i++) {
        bool foundBin = false;
        
        // Check if frequency fits in existing bin
        for (int j = 0; j < binCount; j++) {
            if (abs(frequencies[i] - bins[j]) <= TOLERANCE) {
                bins[j] = (bins[j] * binCounts[j] + frequencies[i]) / (binCounts[j] + 1);
                binCounts[j]++;
                foundBin = true;
                break;
            }
        }
        
        // Create new bin if needed
        if (!foundBin) {
            bins[binCount] = frequencies[i];
            binCounts[binCount] = 1;
            binCount++;
        }
    }
    
    // Find bin with highest count
    int maxCount = 0;
    int maxBinIndex = 0;
    
    for (int i = 0; i < binCount; i++) {
        if (binCounts[i] > maxCount) {
            maxCount = binCounts[i];
            maxBinIndex = i;
        }
    }
    
    return bins[maxBinIndex];
}

String frequencyToNote(double frequency) {
    if(frequency < 20) {
        return "---";
    }
    
    // A4 = 440 Hz is our reference (MIDI note 69)
    // Formula: n = 12 * log2(f / 440) + 69
    double noteNum = 12 * log(frequency / 440.0) / log(2) + 69;
    int nearestNote = round(noteNum);
    
    if(nearestNote < 0 || nearestNote > 127) {
        return "Out of Range";
    }
    
    // Get note name and octave
    int octave = (nearestNote / 12) - 1;
    int noteIndex = nearestNote % 12;
    
    String result = String(noteNames[noteIndex]) + String(octave);
    
    return result;
}

void playFrequency(double frequency) {
    String noteName = frequencyToNote(frequency);
    
    Serial.println("\n>>> PLAYING BACK: " + String(frequency, 1) + " Hz (" + noteName + ") <<<");
    
    // Pulse tone for 5 seconds
    unsigned long startTime = millis();
    
    while (millis() - startTime < 5000) {
        tone(SPEAKER_PIN, (int)frequency);
        delay(150);
        noTone(SPEAKER_PIN);
        delay(100);
    }
    
    noTone(SPEAKER_PIN);
    Serial.println(">>> PLAYBACK COMPLETE <<<");
}
