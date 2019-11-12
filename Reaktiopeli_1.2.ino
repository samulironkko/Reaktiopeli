#include <EEPROM.h>
#include "pitches.h"

enum {
  STATE_MENU,
  STATE_GAME,
  STATE_GAME_OVER
};

int randomNumbers[255];
int buttonPresses[255];
int score = 0;
int level = 0;
int prevLed = 0;
int hiscore = 0;
int r = 0;
int b = 0;
float timerValue = 2200.0;
long timer = 0;
int prevButtonState[] = { HIGH, HIGH, HIGH, HIGH };
int state = STATE_MENU;
int lastButtonPress = 0;
int silentMode = 0;

//Määrittää kaiuttimen pinnin
const int tonePin = 13;

//Määrittää ledien pinnit
const int leds[] = {
  5, //1. led
  4, //2. led
  3, //3. led
  2, //4. led
};

//Määrittää nappien pinnit
const int buttons[] = {
  A0, //1. nappi
  A1, //2. nappi
  A2, //3. nappi
  A3, //4. nappi
};

//Pinni joka yhdistyy ST_CP:hen 74HC595 piirillä
const int latchPin = 8;

//Pinni joka yhdistyy SH_CP:hen 74HC595 piirillä
const int clockPin = 12;

//Pinni joka yhdistyy DS:ään 74HC595 piirillä
const int dataPin = 11;

//Määrittää ledien äänet
const int toneFreq[] = {NOTE_CS4, NOTE_DS4, NOTE_FS4, NOTE_GS4};

//Määrittää näyttöihin
//liitettävät transistorit
const int enableDisplay[] = {
  6, //1. näyttö
  7, //2. näyttö
  9, //3. näyttö
  10 //4. näyttö
};

//Määrittää numerot
//Q6=A,Q5=B,jne
int digits[10] = {
  B1101111, //0 ABCDEF-
  B0100001, //1 -BC----
  B1111100, //2 AB-DE-G
  B1111001, //3 ABCD--G
  B0110011, //4 -BC--FG
  B1011011, //5 A-CD-FG
  B1011111, //6 A-CDEFG
  B1100001, //7 ABC----
  B1111111, //8 ABCDEFG
  B1111011 //9 ABCD-FG
};

//Melodia joka soitetaan jos pelaaja tekee uuden highscoren
void highscoreMelody(int score)
{
  const int highscoreMelody[] = {
    NOTE_C5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_F5, NOTE_A5, END
  };

  //äänen/nuotin kesto melodiassa esim. 4=quarter note ja 8=8th note
  int noteDurationsMelody[] = {
    2, 2, 2, 4, 2, 6
  };

  int speed = 90; //määritellään kauanko ääni/nuotti soi, mitä suurempi arvo sitä hitaampi nuotti

  //määritellään for looppi game over melodian soittamiseen. käydään taulukon arvot läpi ja lopetetaan, kun tullaan taulukossa kohdalle END eli -1
  for (int thisNote = 0; highscoreMelody[thisNote] != -1; thisNote++)
  {
    int noteDuration = speed * noteDurationsMelody[thisNote];
    long timer = millis();
    while (millis() - timer < noteDuration) {
      tone(tonePin, highscoreMelody[thisNote], noteDuration);
      updateDisplay(score, true);
    }
    //delay(noteDuration);

    noTone(tonePin);

  }
}

//melodia joka soi väärän painalluksen jälkeen/silloin, kun peli loppuu. game over melodia.
void endingMelody(int score)
{
  const int endMelody[] = {
    NOTE_GS3, NOTE_DS3, NOTE_CS3, NOTE_GS2, END
  };

  //äänen/nuotin kesto melodiassa esim. 4=quarter note ja 8=8th note
  int noteDurationsMelody[] = {
    4, 4, 4, 8,
  };

  int speed = 90; //määritellään kauanko ääni/nuotti soi, mitä suurempi arvo sitä hitaampi nuotti

  //määritellään for looppi game over melodian soittamiseen. käydään taulukon arvot läpi ja lopetetaan, kun tullaan taulukossa kohdalle END eli -1
  for (int thisNote = 0; endMelody[thisNote] != -1; thisNote++)
  {
    int noteDuration = speed * noteDurationsMelody[thisNote];
    long timer = millis();
    while (millis() - timer < noteDuration) {
      tone(tonePin, endMelody[thisNote], noteDuration);
      updateDisplay(score, true);
    }
    //delay(noteDuration);

    noTone(tonePin);

  }
}

void writeHiscore()
{
  EEPROM.write(0, hiscore);
}

void readHiscore()
{
  hiscore = EEPROM.read(0);
}


//Päivittää näyttöjä
//Välkyttää nopeasti näyttöjä yksi kerrallaan
void updateDisplay(int score, boolean enable) {
  int s = score;

  for (int transNumber = 0; transNumber < 4; transNumber++) {

    //Jakojäännöksellä ja jakamisella saadaan useampi numeroiset luvut
    //yksittäisiksi numeroiksi
    int digit = s % 10;
    s = s / 10;

    //Sammuttaa kaikki näytöt
    for (int i = 0; i < 4; i++) {
      digitalWrite(enableDisplay[i], LOW);
    }

    delayMicroseconds(500);

    //Lasketaan latchPin alas ja pidetään niin kauan
    //kuin siirretään tietoa
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, digits[digit]);

    //Nostetaan latchPin takaisin ylös
    //ja ilmoitetaan samalla että piirin ei tarvitse enää
    //odottaa tietoa
    digitalWrite(latchPin, HIGH);

    delayMicroseconds(500);

    //Sytyttää yhden näytön
    if (enable) {
      digitalWrite(enableDisplay[transNumber], HIGH);
    }
    delayMicroseconds(2000);
  }

}

void setup() {
  Serial.begin(9600);
  //Määrittää ledipinnit outputiksi
  pinMode(leds[0], OUTPUT);
  pinMode(leds[1], OUTPUT);
  pinMode(leds[2], OUTPUT);
  pinMode(leds[3], OUTPUT);

  //Määrittää nappien pinnit input_pullupiksi
  pinMode(buttons[0], INPUT_PULLUP);
  pinMode(buttons[1], INPUT_PULLUP);
  pinMode(buttons[2], INPUT_PULLUP);
  pinMode(buttons[3], INPUT_PULLUP);

  //Määrittää 74HC595 piirin pinnit outputiksi
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  //Määrittää transistorien pinnit outputiksi
  pinMode(enableDisplay[0], OUTPUT);
  pinMode(enableDisplay[1], OUTPUT);
  pinMode(enableDisplay[2], OUTPUT);
  pinMode(enableDisplay[3], OUTPUT);
}

void gameMenu() {

  silentMode = 0;
  readHiscore();

  //Kirjoittaa näytölle hiscoren
  updateDisplay(hiscore, true);

  //Tarkkailee, että painetaanko jotain nappia
  for (int i = 0; i < 4; i++) {
    int button = digitalRead(buttons[i]);
    if (button == LOW) {
      updateDisplay(hiscore, false);
      if (i == 0) {
        silentMode = 1;
      }
      delay(2000);
      startNewGame();
    }
  }
}


//Alustetaan muuttujat uutta peliä varten
void startNewGame() {
  state = STATE_GAME;
  score = 0;
  r = 0;
  b = 0;
  timerValue = 1500.0;
  timer = 0;
  level = 0;
  prevLed = -1;
  lastButtonPress = 0;
  randomSeed(millis());

  for (int i = 0; i < 4; i++) {
    prevButtonState[i] = HIGH;
  }

}
void playGame() {

  //Katsoo onko ajastin mennyt umpeen
  if (millis() - timer > timerValue) {

    //Sammutta ledit
    for (int i = 0; i < 4; i++) {
      digitalWrite(leds[i], LOW);
    }

    //Säätää ajastinta pelin eri tilanteessa
    //eri tavalla
    if (level < 10) {
      timerValue = timerValue * 0.95;
    } else if (10 < level < 20) {
      timerValue = timerValue * 0.99;
    } else if (30 < level < 40 || level > 50 ) {
      timerValue = timerValue * 0.995;
    }

    //Arpoo satunnaisen luvun ja sijoittaa sen lukujonoon
    int led = random(4);

    //Vähentää saman ledin mahdollisuutta
    if (led == prevLed && random(10) < 7) {
      led = random(4);
    }

    prevLed = led;

    randomNumbers[r] = led;
    r++;
    level++;

    //Sytyttää ledin
    digitalWrite(leds[led], HIGH);

    //Käynnistää ajastimen
    timer = millis();

    //Soittaa napille kuuluuvan äänen
    //jos ei ole hiljaisessa tilassa
    if (silentMode == 0) {
      tone(tonePin, toneFreq[led], timerValue * 0.6);
    }
  }

  //Päivittää näytölle pistetilanteen
  updateDisplay(score, true);

  //Tarkastaa onko jotain nappia painettu
  for (int i = 0; i < 4; i++) {

    //Debounce katsoo että viime painalluksesta on mennyt 200ms
    //ennenkuin hyväksyy seuraavan painalluksen
    if (millis() - lastButtonPress > 50) {

      int but = digitalRead(buttons[i]);

      //Jos on, lisää napin arvon lukujonoon
      if (but == LOW && prevButtonState[i] == HIGH) {
        buttonPresses[b] = i;

        //Vertaa lukujonojen samaa kohtaa
        //ja jos ne täsmää, antaa pisteen pelaajalle
        if (randomNumbers[b] == buttonPresses[b]) {
          score++;
          b++;

          //Tarkastaa onko pelaaja jäljessä
          if (level == score) {
            //Sammuttaa ledit
            for (int i = 0; i < 4; i++) {
              digitalWrite(leds[i], LOW);
            }
          }

          //Muuten peli loppuu
        } else {
          gameOver();
        }

        //Viimeisin painallus alustetaan
        lastButtonPress = millis();
      }

      //Siirretään napin tila edelliseksi napin tilaksi
      prevButtonState[i] = but;
    }
  }
}

void gameOver() {

  //Muuttuja johon merkataan, onko tehty uusi highscore
  int newHighscore = 0;

  //Sytyttää ledit
  for (int i = 0; i < 4; i++) {
    digitalWrite(leds[i], HIGH);
  }

  //Soittaa game over soinnut
  //jos ei ole hiljaisessa tilassa
  if (silentMode == 0) {
    endingMelody(score);
  }

  //Jos score isompi kuin hiscore, niin päivittää highscoren
  if (score > hiscore) {
    hiscore = score;
    writeHiscore();
    newHighscore = 1;
  }

  //Jos ei tehty uutta highscorea,
  //tulostaa näytölle scorea ja highscorea vuorotellen
  if (newHighscore == 0) {
    for (int i = 0; i < 5; i++) {
      long timer = millis();

      while (millis() - timer < 1000) {
        updateDisplay(score, true);
      }
      timer = millis();
      while (millis() - timer < 1000) {
        updateDisplay(hiscore, true);
      }
    }
  }

  //Jos on tehty uusi highscore,
  //välkytellään ledejä ja soitetaan
  //highscore melodia
  if (newHighscore == 1) {

    //Soittaa highscore melodian
    //jos peli ei ole hiljaisessa tilassa
    if (silentMode == 0) {
      highscoreMelody(score);
    }

    for (int i = 0; i < 4; i++) {
      digitalWrite(leds[i], LOW);
    }

    for (int i = 0; i < 10; i++) {
      for (int i = 0; i < 4; i++) {
        long timer = millis();

        while (millis() - timer < 100) {
          digitalWrite(leds[i], HIGH);
          updateDisplay(score, true);
        }
        digitalWrite(leds[i], LOW);
      }

      for (int i = 3; i >= 0; i--) {
        long timer = millis();

        while (millis() - timer < 100) {
          digitalWrite(leds[i], HIGH);
          updateDisplay(score, true);
        }
        digitalWrite(leds[i], LOW);
      }
    }
  }

  //Sammuttaa ledit
  for (int i = 0; i < 4; i++) {
    digitalWrite(leds[i], LOW);
  }

  //Siirtyy takaisin menuun
  state = STATE_MENU;
}
void loop() {
  //Jos state on state_menu, käynnistetään menu
  if (state == STATE_MENU) {
    gameMenu();
  }
  //Jos state on state_game, käynnistetään peli
  else if (state == STATE_GAME) {
    playGame();
  }
  //Muuten peli loppuu
  else {
    gameOver();
  }
}
