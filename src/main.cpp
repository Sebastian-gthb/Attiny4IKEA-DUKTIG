/*
ATiny85 Pinout

                          +--, ,--+
RESET--5/A0--PCINT5--PB5--|1  U  8|--VCC +
       3/A3--PCINT3--PB3--|2     7|--PB2--PCINT2--2/A1--SCK---SCL
 OC1B--4/A2--PCINT4--PB4--|3     6|--PB1--PCINT1--1-----MISO--D0
                     GND--|4     5|--PB0--PCINT0--0-----MOSI--DI
                          +-------+
*/


#include <Arduino.h>
#define Kochfeld1 2
#define Kochfeld2 3
#define Taster1 0
#define Taster2 1
#define Zaehlwert 30000        // 5min = 30000  zum Test hatte ich immer 600
#define debounceinit 170     //B10101010 <-- weder alles 0 noch alles 1 ... eigentlich wuerde auch B00000010 ausreichen, da die 1 und 0 dann nach links geshiftet wird
int Zaehler=0;
int Zaehler1=0;
int Zaehler2=0;
bool Taster1hist = false, Taster2hist = false;             //letzter Zustand des Tasters (false=losgelassen, true=gedrueckt)
bool Kochfeld1stat=false, Kochfeld2stat=false;                    //Status der Kochfelder
byte debounce1=debounceinit, debounce2=debounceinit;


ISR (PCINT0_vect) {}  // Interrupt Service Routine is calling by external PINs. Which PINs are configured with PCMSK register.


void schlafein() {
  //ENABLE SLEEP - this enables the sleep mode
  MCUCR |=   B00010000;        //power down mode (SM1=1,SM0=0)     BODS|PUD|SE|SM1|SM0|BODSE|ISC01|ISC00
  MCUCR |=   B00100000;        //enable sleep (SE=1)               BODS|PUD|SE|SM1|SM0|BODSE|ISC01|ISC00
  __asm__  __volatile__("sleep");//in line assembler to go to sleep
  // <---- Mikrokontroler schlaeft hier!

  // <---- evtl noch irgendwas nach dem aufwachen machen/deaktivieren, damit nix komisches passiert?
  MCUCR &=   B11011111;        //disable sleep (SE=0)               BODS|PUD|SE|SM1|SM0|BODSE|ISC01|ISC00
}


void setup() {
  //Energiesparoptionen
  ADCSRA &= B01111111; //deaktiviere ADC mit bit 7 im ADCSRA Register = ADEN = ADC Enabled ... zum reaktivieren ADCSRA |= B10000000;
  //pinMode(0, OUTPUT);  //setze alle PINs die nicht genutzt werden auf Output low: 3, 4, 5, 6, 11, 14, 15, 16, 17, 18, 19
  //pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);     //LED Herdplatte 1
  pinMode(3, OUTPUT);     //LED Herdplatte 2
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  pinMode(0, INPUT_PULLUP);   //Taster 1 fuer Herdplatte 1
  pinMode(1, INPUT_PULLUP);   //Taster 2 fuer Herdplatte 2

  PCMSK |= B00000011; // digitalPIN 0 und 1                  – | – |PCINT5|PCINT4|PCINT3|PCINT2|PCINT1|PCINT0
  GIFR  |= B00100000; // clear any outstanding interrupts    – |INTF0|PCIF| –| – | – | – | –
  GIMSK |= B00100000; // enable pin change interrupts        – |INT0|PCIE| – | – | – | – | –

}


void loop() {
  // put your main code here, to run repeatedly:
  /*
  Plan:

  Wenn Taster1 gedrückt wird aufwachen und (8bit Entprellung)
    und Zähler1=0 LED1=an
      Wenn Taster1 losgelassen (8bit Entprellung) Zähler1 setzen und starten
    oder Zähler1>0 LED1=aus stoppe Zähler1 und setze ihn auf 0
  Wenn Zähler1=0 LED1 aus

  Wenn Taster2 gedrückt wird aufwachen und (8bit Entprellung)
    und Zähler2=0 LED2=an
      Wenn Taster2 losgelassen (8bit Entprellung) Zähler2 setzen und starten
    oder Zähler2>0 LED2=aus stoppe Zähler2 und setze ihn auf 0
  Wenn Zähler1=0 LED aus

  Wenn Zähler1=0 und Zähler2=0 und LED1=aus und LED2=aus --> Tiefschlaf

  --> Reduzierung auf einen Zaehler... lette Aktivität startet den Zaehler und anch 5min geht alles aus.
  
  Kochfelder sind immer fuer 5min an

  */


  // Die Debouncer haben die Aufgabe die letzten 8 Zustaende des Tasters zu protokollieren, um das Prellen eines Tasters herauszufiltern.
  // Wichtig ist das delay zwischen jeder Abfrage. Das muss so lang sein, dass kein Bouncing als loslassen erkannt wird, aber so kurz, dass
  // ein kurzes Antippen des Tasters trotzdem als Tastendruck erkannt wird. 10ms Abtstrate/pro Durchlauf sollte passen.
  // Ist der Taster 8 mal hintereinander ein (=255) oder aus (=0), dann hat der Taster ausgeprellt.
  // Zusaetzlich wird das so verriegelt, dass nach einem aktiven Phase erst wieder eine inaktive Phase kommen muss um ein loslassen oder ei erneutes Druecken zu erkennen.

  // Debouncer protokollieren die Tasterzustaende
  debounce1 = debounce1 << 1;
  debounce2 = debounce2 << 1;
  if (digitalRead(Taster1)==LOW) { debounce1++; }       //setze immer erstes Bit auf 1, wenn der Taster geschlossen ist
  if (digitalRead(Taster2)==LOW) { debounce2++; }       //setze immer erstes Bit auf 1, wenn der Taster geschlossen ist
  
  // erkenne ob der Taster gedrueckt wurde
  if (debounce1==255) {                 // Taster war 8 mal hinteeinander aktiv
    if (!Taster1hist) {                 // und der Taster war vorher nicht gedrueckt
      Taster1hist=true;                 // vermerke, dass der Taster nun als gedrueckt zaehlt
      Kochfeld1stat=!Kochfeld1stat;     // negiere den Kochfeldstatus
      digitalWrite(Kochfeld1, Kochfeld1stat);   //aktiviere bzw. deaktiviere das Kochfeld
      Zaehler=Zaehlwert;
    }
  }
  if (debounce2==255) {                 // Taster war 8 mal hinteeinander aktiv
    if (!Taster2hist) {                 // und der Taster war vorher nicht gedrueckt
      Taster2hist=true;                 // vermerke, dass der Taster nun als gedrueckt zaehlt
      Kochfeld2stat=!Kochfeld2stat;     // negiere den Kochfeldstatus
      digitalWrite(Kochfeld2, Kochfeld2stat);   //aktiviere bzw. deaktiviere das Kochfeld
      Zaehler=Zaehlwert;
    }
  }

  // erkenne ob der Taster losgelassen wurde
  if (debounce1==0) {                   // Taster war 8 mal hinteeinander inaktiv
    if (Taster1hist) {                  // und der Taster war vorher gedrueckt
      Taster1hist=false;                 // vermerke, dass der Taster nun als nicht gedrueckt zaehlt
    }
  }
  if (debounce2==0) {                   // Taster war 8 mal hinteeinander inaktiv
    if (Taster2hist) {                  // und der Taster war vorher gedrueckt
      Taster2hist=false;                 // vermerke, dass der Taster nun als nicht gedrueckt zaehlt
    }
  }


  if (Zaehler == 0) {               // war der Zaehler auf 1 und nach dem dekrementieren jetzt auf 0...
    digitalWrite(Kochfeld1, LOW);      // deaktiviere Kochfeld 1
    digitalWrite(Kochfeld2, LOW);      // deaktiviere Kochfeld 2
    Kochfeld1stat=false;
    Kochfeld2stat=false;
    schlafein();                          // geh in Schlafmodus... nix mehr zu tun
    Zaehler=50;                     //nach Aufwachen den Zaehler auf einen kleinen Wert setzen um ein paar Loops zu laufen und Tastendruck festzustellen
  }

  Zaehler--;                        // dekrementiere den Zaehler
  

  delay(10);

}
