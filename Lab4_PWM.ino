// Author: Brian Wu
// Written 10/15/22
// FOR ESP32
/*
Lab 4 of CE4370. Involves using hardware timers and pulse width modulation (PWM).
This program now has 4 states. The ESP32 will initially start off, and then once a button is pressed
switch states to fade in, where the LED will slowly fade into the specified color (blue) for a duration of 3 seconds.
Then the LED will stay on for 6 seconds.
The LED will then fade out over the interval of 4 seconds.
Then the LED will go back to the off state.
A timer interrupt and button interrupt will be linked to the same handler function.
The handler function will switch between states.
The states being:
1. STATE_OFF
2. STATE_FADE_IN
3. STATE_STEADY_ON
4. STATE_FADE_OUT

*/

#define interruptPin 0
#define PRESS_DELAY 200 // defined in microseconds
#define ledR 16
#define ledG 17
#define ledB 18

#define TIMER_SELECT 0 // Select timer 0 out of 0-3
#define TIMER_DIVISION 80 // Set to divide by 80. 80MHZ is clock speed
#define TIMER_MODE 1 // Set to "up" or "true"

// Defines an enum STATE_T that can contain 5 states.
typedef enum
{
    STATE_OFF = 0,
    STATE_FADE_IN = 1,
    STATE_STEADY_ON = 2,
    STATE_FADE_OUT = 3
} STATE_T;

// Function Prototypes
static void turnLEDtoHue(uint8_t red, uint8_t green, uint8_t blue);
void IRAM_ATTR handle_state();
void IRAM_ATTR debounceHandle_state();

// Global Variables
// RTC_DATA_ATTR qualifier so that the data will be saved within the RTC memory, which means that it will be able
// to accessed in deep sleep mode
volatile static STATE_T state = STATE_OFF;
volatile unsigned long lastDebounceTime = 0;
static int numHandles = 0;
hw_timer_t * timer = NULL;
 

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    delay(10);

    Serial.println("\nBooting up ESP32...");
    
    timer = timerBegin(0,80,true); // Instantiate the timer. Set the clock speed to 80MHz / 80 = 1MHz
                                   // Use timer 0

    ledcAttachPin(ledR, 1); // assign RGB led pins to channels
    ledcAttachPin(ledG, 2);
    ledcAttachPin(ledB, 3);

    // Initialize channels
    // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
    // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);

    // Sets up the ability to range in values from 0 - 255 in resolution.
    // Sets up the frequency as well.
    ledcSetup(1, 12000, 8); // 12 kHz PWM, 2^8, 8-bit resolution
    ledcSetup(2, 12000, 8);
    ledcSetup(3, 12000, 8);

    pinMode(interruptPin, INPUT_PULLUP); // The switch is always "1", until pressed which will turn it "0"

    attachInterrupt(digitalPinToInterrupt(interruptPin), debounceHandle_state, RISING);
    turnLEDtoHue(0, 0, 0);            // Sets the color to "off"

}

void loop()
{
    // put your main code here, to run repeatedly:

    // Debugging code to make sure that the interrupt is not called several times because of switch bouncing
    // Serial.print("The State is: ");
    // Serial.println(state);
    // Serial.print("The number of times handle_state has been called is: ");
    // Serial.println(numHandles);
    // delay(500);
    switch (state)
    {
    case STATE_OFF:
        turnLEDtoHue(0, 0, 0); // Turns the LED OFF
        break;
    case STATE_FADE_IN:
        // 
        // Set time interrupt function
        break;
    case STATE_STEADY_ON:
        break;
    case STATE_FADE_OUT:
        break;
}

// Turns the value into RGB. Turns on the LED
static void turnLEDtoHue(uint8_t red, uint8_t green, uint8_t blue)
{
    ledcWrite(1, 255 - red);
    ledcWrite(2, 255 - green);
    ledcWrite(3, 255 - blue);
}

static void fadeLedin(uint8_t red, uint8_t green, uint8_t blue, )
{

    

}

// Primary Interrupt function to handle switch bouncing (switch noise)
// IRAM_ATTR stores the function in flash ram so it is fast as possible
void IRAM_ATTR debounceHandle_state()
{
    // Makes it so that the actual interrupt function will not be called until a certain delay
    // has passed between the last bounce
    if ((millis() - lastDebounceTime) >= PRESS_DELAY)
    {
        handle_state();
        lastDebounceTime = millis();
    }
}

// Actual interrupt function that switches the states.
void IRAM_ATTR handle_state()
{
    switch (state)
    {
    case STATE_OFF:
        state = STATE_ON;
        break;
    case STATE_FADE_IN:
        state = STATE_FLASHING;
        break;
    case STATE_STEADY_ON:
        state = STATE_FADE_OUT;
        break;
    case STATE_FADE_OUT:
        state = STATE_OFF;
        break;
    }

    numHandles++; // Counts the number of times this function is called for debugging purposes
    Serial.print("The number of times the handle_state function has been called is: ");
    Serial.println(numHandles);
    Serial.print("The new state is at ");
    Serial.println(state);
}

