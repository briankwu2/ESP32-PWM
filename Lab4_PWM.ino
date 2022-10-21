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
#define STATE_FADE_IN_DELAY 3 // 3 Seconds
#define STATE_STEADY_ON_DELAY 6 // 6 Seconds
#define STATE_FADE_OUT_DELAY 4  // 4 Seconds

#define TIMER_SELECT 0 // Select timer 0 out of 0-3
#define TIMER_DIVISION 80 // Set to divide by 80. 80MHZ is clock speed
#define TIMER_MODE 1 // Set to "up" or "true"
#define TIMER_PARTITION 256 // Partitions 1 second into this many partitions

// Defines an enum STATE_T that can contain 5 states.
typedef enum
{
    STATE_OFF = 0,
    STATE_FADE_IN = 1,
    STATE_STEADY_ON = 2,
    STATE_FADE_OUT = 3
} STATE_T;

// Function Prototypes
void turnLEDtoHue(uint8_t red, uint8_t green, uint8_t blue);
void IRAM_ATTR handle_state();
void IRAM_ATTR debounceHandle_state();
void IRAM_ATTR timerInterrupt();

// Global Variables
volatile static STATE_T state; 
volatile unsigned long lastDebounceTime = 0;
hw_timer_t * timer = NULL;
volatile static int timePartition; 

void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    delay(10);
    Serial.println("\nBooting up ESP32...");
    
    pinMode(interruptPin, INPUT_PULLUP); // The switch is always "1", until pressed which will turn it "0"
    delay(1000); // To prevent handle_state from being accidentally called because it senses "rising edge"

    attachInterrupt(digitalPinToInterrupt(interruptPin), debounceHandle_state, RISING);

    timer = timerBegin(TIMER_SELECT,TIMER_DIVISION, TIMER_MODE); // Instantiate the timer. Set the clock speed to 80MHz / 80 = 1MHz
                                   // Use timer 0

    ledcAttachPin(ledR, 1); // assign RGB led pins to channels
    ledcAttachPin(ledG, 2);
    ledcAttachPin(ledB, 3);

    // Sets up the ability to range in values from 0 - 255 in resolution.
    // Sets up the frequency as well.
    ledcSetup(1, 12000, 8); // 12 kHz PWM, 2^8, 8-bit resolution
    ledcSetup(2, 12000, 8);
    ledcSetup(3, 12000, 8);

    timerAttachInterrupt(timer, &timerInterrupt, true); // Set a timer interrupt timer to update time partition. Autoreloads
    timerAlarmWrite(timer, 1000000/ TIMER_PARTITION, true); // Set the timer interrupt to trigger every 1/256 of a second.
    turnLEDtoHue(0, 0, 0);            // Sets the color to "off"

    state = STATE_OFF;
    timePartition = 0;
}

/**
 * @brief Main loop that performs a different function based on the current state.
 * Read the program synopsis for more information on what the states do.
 */
void loop()
{
    switch (state)
    {
        case STATE_OFF:
            turnLEDtoHue(0, 0, 0); // Turns the LED OFF
            break;
        case STATE_FADE_IN:
            if (!timerAlarmEnabled(timer))
                timerAlarmEnable(timer); // Starts the timer interrupt 
            if (timePartition <= 256 * STATE_FADE_IN_DELAY)
            {
                turnLEDtoHue(0,0,(timePartition / STATE_FADE_IN_DELAY) - 1);
            }
            else if (timePartition > 256 * STATE_FADE_IN_DELAY)
            {
                timerAlarmDisable(timer);
                timePartition = 0;
                handle_state(); // Change to next state after the delay has passed 
            }
            break;
        case STATE_STEADY_ON:
            if (!timerAlarmEnabled(timer))
                timerAlarmEnable(timer); // Starts the timer interrupt 
            if (timePartition <= 256 * STATE_STEADY_ON_DELAY)
            {
                turnLEDtoHue(0,0, 255);
            }
            else if (timePartition > 256 * STATE_STEADY_ON_DELAY)
            {
                timerAlarmDisable(timer);
                timePartition = 0;
                handle_state(); // Change to next state after the delay has passed 
            }
            break;
        case STATE_FADE_OUT:
            if (!timerAlarmEnabled(timer))
                timerAlarmEnable(timer); // Starts the timer interrupt 
            if (timePartition <= 256 * STATE_FADE_OUT_DELAY)
            {
                turnLEDtoHue(0,0, (255 - timePartition / STATE_FADE_OUT_DELAY));
            }
            else if (timePartition > 256 * STATE_FADE_OUT_DELAY)
            {
                timerAlarmDisable(timer);
                timePartition = 0;
                handle_state(); // Change to next state after the delay has passed 
            }
            break;
        default:;
    }

    // Code to test hw_timer_t
}


/**
 * @brief Turns the value into RGB. Turns on the LED to the specified value of RGB. Ranges from 0 to 256
 * 
 * @param red Turns on the red LED to the specified parameter
 * @param green Turns on the green LED to the specified parameter
 * @param blue Turns on the blue LED to the specified parameter
 */
void turnLEDtoHue(uint8_t red, uint8_t green, uint8_t blue)
{
    ledcWrite(1, 256 - red);
    ledcWrite(2, 256 - green);
    ledcWrite(3, 256 - blue);
}

/**
 * @brief 
 * Timer interrupt function that counts every 1/TIMER_PARTITION of a second. Occurs every 1MHz / TIMER_PARTITION second(s).
 */
void IRAM_ATTR timerInterrupt() 
{
    noInterrupts();
    timePartition++; // Updates the timer to count 1/TIMER_PARTITION of a second. (1/256 in this case)
    interrupts();
}

/**
 * @brief Is the button interrupt function that debounces the button, and then calls the real interrupt function.
 */
void IRAM_ATTR debounceHandle_state()
{
    noInterrupts();
    // Makes it so that the actual interrupt function will not be called until a certain delay
    // has passed between the last bounce
    if ((millis() - lastDebounceTime) >= PRESS_DELAY)
    {
        handle_state();
        lastDebounceTime = millis();
    }
    interrupts();
}

/**
 * @brief Changes the state according to the current state.
 * 
 */
void IRAM_ATTR handle_state()
{
    switch (state)
    {
    case STATE_OFF:
        state = STATE_FADE_IN;
        break;
    case STATE_FADE_IN:
        state = STATE_STEADY_ON;
        break;
    case STATE_STEADY_ON:
        state = STATE_FADE_OUT;
        break;
    case STATE_FADE_OUT:
        state = STATE_OFF;
        break;
    }
}

