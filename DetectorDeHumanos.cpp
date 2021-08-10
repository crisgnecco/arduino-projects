#include <LiquidCrystal.h>

// Macro para realizar un log termianl e informar los cambios de estados

#define SERIAL_DEBUG_ENABLED 1

#if SERIAL_DEBUG_ENABLED
  #define DebugPrint(str)\
      {\
        Serial.println(str);\
      }
#else
  #define DebugPrint(str)
#endif

#define DebugPrintEstado(estado,evento)\
      {\
        String est = estado;\
        String evt = evento;\
        String str;\
        str = "-----------------------------------------------------";\
        DebugPrint(str);\
        str = "EST-> [" + est + "]: " + "EVT-> [" + evt + "].";\
        DebugPrint(str);\
        str = "-----------------------------------------------------";\
        DebugPrint(str);\
      }
//----------------------------------------------

// Llamo al metodo de inicializacion de la bib de LCD. indico pines
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

// Constantes para el LCD

#define CHARACTERS_ROW 			16
#define CHARACTERS_COLUMN 		2

#define ROW_1	0
#define COL_1	0
#define ROW_2	0
#define COL_2	1

// Constante para el Monitor Serial (velocidad en bps)

#define SPEED_SERIAL 9600

// Constantes para el ultrasonido

#define DELAY_ULTRASONIC_LOW 		2
#define DELAY_ULTRASONIC_HIGH		10
#define SPEED_SOUND 				0.01723
// Constante que define la velocidad del sonido en cm / microsegundos

// Constantes pines sensores y actuadores

#define PIN_RELAY 6
#define PIN_PRESION A0
#define PIN_ULTRASONICO 7
#define PIN_BUZZER 5

// Umbrales

#define UMBRAL_DIFERENCIA_TIMEOUT 1
#define UMBRAL_PRESION 250
#define UMBRAL_PROXIMIDAD 100

// Constantes notas musicales

#define NOTE_SOL4 392
#define NOTE_LA4 440
#define NOTE_SI4 494
#define NOTE_DO5 523
#define NOTE_RE5 587
#define NOTE_MI5 659
#define NOTE_FA5 698
#define NOTE_SOL5 784

#define A_MILISEGUNDOS 1000
#define FACTOR_TIEMPO_ENTRE_NOTAS 10
#define MIN_COUNTER_NOTES 0

// Constante para distancia sensor proximidad

#define MIN_DIST 0

// Constantes para estados y eventos.

#define MAX_STATES 3
#define MAX_EVENTS 2
#define MIN_STATES 0
#define MIN_EVENTS 0

enum states
{
    ST_INIT,
    ST_ESPERANDO,
    ST_MUSIC_ON
} current_state;

String states_s[] = {"ST_INIT", "ST_ESPERANDO", "ST_MUSIC_ON"};

enum events
{
    EV_CONT,
    EV_DETECTADO
} new_event;

String events_s[] = {"EV_CONT", "EV_DETECTADO"};

// Defino un tipo de dato transition: funcion que no recibe ni devuelve nada
typedef void (*transition)();

/* Tabla donde la interseccion entre estado y evento contiene una funcion del tipo "transition"
 La cual sera la accion a tomar ante cierto evento en cierto estado. */
transition state_table[MAX_STATES][MAX_EVENTS] =
    {
        {init_, nada},          // ST_INIT
        {nada, echar},          // ST_ESPERANDO
        {desactivar, playMusic} // ST_MUSIC_ON

        //EV_CONT	 , 	EV_DETECTADO
};

// ------------------------------------------------------------------------ //

bool timeout;
long lct;

/*------Variables para funcion playMusic -------*/

// Define la melodía a tocar. Cada línea define una nota
int melody[] = {
    NOTE_SOL4,
    NOTE_SOL4,
    NOTE_LA4,
    NOTE_SOL4,
    NOTE_DO5,
    NOTE_SI4,

    NOTE_SOL4,
    NOTE_SOL4,
    NOTE_LA4,
    NOTE_SOL4,
    NOTE_RE5,
    NOTE_DO5,

    NOTE_SOL4,
    NOTE_SOL4,
    NOTE_SOL5,
    NOTE_MI5,
    NOTE_DO5,
    NOTE_SI4,
    NOTE_LA4,

    NOTE_FA5,
    NOTE_FA5,
    NOTE_MI5,
    NOTE_DO5,
    NOTE_RE5,
    NOTE_DO5};

// Define la duracion de cada nota
int noteDurations[] = {
    4, 4, 8, 8, 8, 12, 4, 4, 8, 8, 8, 12, 4, 4, 8, 8, 8, 8, 12, 4, 4, 8, 8, 8, 12
};

int counterNote = MIN_COUNTER_NOTES;
long lctMusic;
boolean timeoutMusic = false; //arranca apagado

// Obtengo el numero de notas
int num_notas = sizeof(melody) / sizeof(int);

/*-----------------------------------------------*/

// variable usada para sensor de proximidad
int distCm;

long readUltrasonicDistance(int triggerPin, int echoPin)
{
    pinMode(triggerPin, OUTPUT); // Clear the trigger
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(DELAY_ULTRASONIC_LOW);

    // setea en estado HIGH por 10 microseconds
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(DELAY_ULTRASONIC_HIGH);
    digitalWrite(triggerPin, LOW);
    pinMode(echoPin, INPUT);

    // Reads the echo pin, and returns the sound wave travel time in microseconds
    return pulseIn(echoPin, HIGH);
}

void playMusic()
{
    //Tengo la duracion de las notas en milisegundos
    unsigned long noteDuration = A_MILISEGUNDOS * noteDurations[counterNote];

    /* 
    	La duracion entre notas sera la duracion de la nota multiplicado por un factor
       	para que la melodia sea lo mas parecida posible a la melodia original
    */
    unsigned long pauseBetweenNotes = noteDurations[counterNote] * FACTOR_TIEMPO_ENTRE_NOTAS;

    /* Aplicamos temporizador por software */
    long ctMusic = millis();
    int diferenciaMusic = (ctMusic - lctMusic);

    timeoutMusic = (diferenciaMusic > pauseBetweenNotes) ? (true) : (false);

    if (counterNote < num_notas)
    {
		/* Siempre que el la cancion no haya terminado y el tiempo
        	entre notas se haya cumplio genero el tono y aumento el contado */
        if (timeoutMusic)
        {

            timeoutMusic = false;
            lctMusic = ctMusic;

            tone(PIN_BUZZER, melody[counterNote], noteDuration);
            counterNote++;
        }
      	
    }
    else
    {
        counterNote = MIN_COUNTER_NOTES;
    }
}

void nada()
{
}

void desactivar()
{
    noTone(PIN_BUZZER);
    counterNote = MIN_COUNTER_NOTES;

    // Apago el relay
    digitalWrite(PIN_RELAY, LOW);
    lcd.clear();
    current_state = ST_ESPERANDO;
}

void init_()
{
    current_state = ST_ESPERANDO;
}

bool verificarEstadoSensorPresionYProximidad()
{

    // leo presion
    int valorPresion = analogRead(PIN_PRESION);

    /* Mido el tiempo de ping y lo convierto en cm. El valor de SPEED_SOUND es 0.01723 y corresponde a la velocidad del sonido en cm/microsegundos
      d [ cm ] = velDelSonido [ cm /micro seg] . t [micro seg] */
    int distanciaCm = SPEED_SOUND * readUltrasonicDistance(PIN_ULTRASONICO, PIN_ULTRASONICO);

    if (valorPresion > UMBRAL_PRESION && distanciaCm < UMBRAL_PROXIMIDAD)
    {
        new_event = EV_DETECTADO;
        return true;
    }

    return false;
}

void echar()
{

    // Se setea el cursor en las coordenadas del LCD para escribir texto.
    lcd.setCursor(ROW_1, COL_1);
    lcd.print("Hay");
    lcd.setCursor(ROW_2, COL_2);
    lcd.print("Intruso");

    // Prendo el relay
    digitalWrite(PIN_RELAY, HIGH);

    current_state = ST_MUSIC_ON;
}

void get_new_event()
{

    /* Temporizador por software */
    long ct = millis();
    int diferencia = (ct - lct);
    timeout = (diferencia > UMBRAL_DIFERENCIA_TIMEOUT) ? (true) : (false);

    if ((timeout == true && verificarEstadoSensorPresionYProximidad() == true))
    {
        return;

    }else{

        new_event = EV_CONT;
    }

}
//----------------------------------------------

void maquina_estados_detector_de_intrusos()
{
    get_new_event();

    // verificar si es un estado valido
    if ((new_event >= MIN_EVENTS) && (new_event < MAX_EVENTS) && (current_state >= MIN_STATES) && (current_state < MAX_STATES))
    {
    
      	DebugPrintEstado(states_s[current_state], events_s[new_event]);
        state_table[current_state][new_event]();
    }
  	else
  	{
    	Serial.println("Estado y Evento desconocido");
  	}

    // Consumo el evento
    new_event = EV_CONT;
}

/* Funciones propias de Arduino */

void setup()
{
    // Valor de velocidad de modulacion de la comunicacion serial con la computadora.
    Serial.begin(SPEED_SERIAL);

    pinMode(PIN_PRESION, INPUT);
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);

    //indico cantidad de caracteres de fila y columna, respectivamente
    lcd.begin(CHARACTERS_ROW, CHARACTERS_COLUMN);
    distCm = MIN_DIST;
    timeout = false;
    lct = millis();

    current_state = ST_INIT;
}

void loop()
{
    maquina_estados_detector_de_intrusos();
}

//----------------------------------------------