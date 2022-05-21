#include <SPI.h>
#include <SoftwareSerial.h>
#include <SD.h>
#define BUZZER_PIN 10

SoftwareSerial espSerial(2, 3);

// PIANO Stuff
#define SENSOR_COUNT 6
#define THRESHOLD_VALUE 70

struct photo_sensor {
  int pin;
  int cal_value;
  int freq;
};

photo_sensor sensors[SENSOR_COUNT];

// SD Stuff
#define SD_CS_PIN 5

void debug_buzzing(int count) {
  for (int i = 0; i < count; i++) {
    tone(BUZZER_PIN, 400, 100);
    delay(500);
  }
}

void init_piano() {
  sensors[0].pin = A0;
  sensors[1].pin = A1;
  sensors[2].pin = A2;
  sensors[3].pin = A3;
  sensors[4].pin = A4;
  sensors[5].pin = A5;

  sensors[0].freq = 261;
  sensors[1].freq = 293;
  sensors[2].freq = 329;
  sensors[3].freq = 349;
  sensors[4].freq = 392;
  sensors[5].freq = 440;
  
  pinMode(BUZZER_PIN, OUTPUT);
  for (int i = 0; i < SENSOR_COUNT; i++) {
    pinMode(sensors[i].pin, INPUT); 
    sensors[i].cal_value = analogRead(sensors[i].pin);
  }

}

int init_sd() {
  if (!SD.begin(SD_CS_PIN)) {
    debug_buzzing(2);
    Serial.println("Error while opening SD card");
    return 1;
  }
  return 0;
}

void handle_piano() {
  int max_deviation = -1;
  int index = -1;
  
  for (int i = 0; i < SENSOR_COUNT; i++) {
    int current_value = analogRead(sensors[i].pin);
    int deviation = current_value - sensors[i].cal_value;
    if (deviation > max_deviation) {
      max_deviation = deviation;
      index = i;
    }
  }

  if (max_deviation > THRESHOLD_VALUE) {
    tone(BUZZER_PIN, sensors[index].freq, 100);
  }
}

void play_file(char filename[], int retries) {
  File f = SD.open(filename);
  int filesize = f.size();
  int index = 0;

  if (f) {
    Serial.print("Playing file: ");
    Serial.println(filename);
    for(int i = 0; i < strlen(filename); i++) {
      Serial.println((int) filename[i]);
    }
    int notes = index / 2;
    int tempo = 100;
    int wholenote = (60000 * 4) / tempo;
    int divider = 0, noteDuration = 0;
    while (f.available()) {
      int b0 = f.read();
      int b1 = f.read();
      int b2 = f.read();
      int b3 = f.read();

      int note = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;

      b0 = f.read();
      b1 = f.read();
      b2 = f.read();
      b3 = f.read();

      int duration = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;

      divider = duration;
      if (divider > 0) {
        // regular note, just proceed
        noteDuration = (wholenote) / divider;
      } else if (divider < 0) {
        // dotted notes are represented with negative durations!!
        noteDuration = (wholenote) / abs(divider);
        noteDuration *= 1.5; // increases the duration in half for dotted notes
      }
  
      // we only play the note for 90% of the duration, leaving 10% as a pause
      tone(BUZZER_PIN, note, noteDuration * 0.9);
  
      // Wait for the specief duration before playing the next note.
      delay(noteDuration);
  
      // stop the waveform generation before the next note.
      noTone(BUZZER_PIN);
    }
    f.close();
  } else {
    Serial.println("Error while opening file");
    if (retries >= 0) play_file(filename, retries - 1);
    else {
      if (strcmp(filename, "dummy.txt")) {
        debug_buzzing(3);
      }
    }
  }
}

void print_all_files() {
  File dir = SD.open("/");
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    Serial.println(entry.name());
    espSerial.println(entry.name());
    entry.close();
  }
  dir.close();
}

void setup()
{
  Serial.begin(9600);
  espSerial.begin(9600);
  init_piano();
  int sd_ret = init_sd();
  delay(1000);
  if (sd_ret == 0) {
    tone(BUZZER_PIN, 400, 100);
    print_all_files();
  }

}

void loop()
{
  handle_piano();
  if(Serial.available()) { // Debug stuff
    int r = Serial.read();
    if (r == '1') {
      play_file("starwars.bin", 3);
    } else if (r == '2') {
      print_all_files();
    }
  }

  if(espSerial.available()) {
    char filename[15];
    espSerial.readBytesUntil('\n', filename, 15);

    for(int i = 0; i < strlen(filename); i++) {
      if (filename[i] >= 'A' && filename[i] <= 'Z') filename[i] = tolower(filename[i]);
      else if (filename[i] == '\n' || filename[i] == 13) filename[i] = '\0';
    }
    Serial.println(filename);
    play_file(filename, 3);
  }
}
