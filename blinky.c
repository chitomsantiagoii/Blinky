/*
    Blinky.c
*/

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ACT_LED_TRIGGER         "/sys/class/leds/led0/trigger"
#define PWR_LED_TRIGGER         "/sys/class/leds/led1/trigger"

#define ACT_LED_BRIGHTNESS      "/sys/class/leds/led0/brightness"
#define PWR_LED_BRIGHTNESS      "/sys/class/leds/led1/brightness"

#define ACT_LED_TRIGGER_CPU0    "cpu0"
#define ACT_LED_TRIGGER_MMC0    "mmc0"

#define PWR_LED_TRIGGER_GPIO    "gpio"
#define PWR_LED_TRIGGER_OFF     "off"

#define LED_OFF                 "0"
#define LED_ON                  "1"

int _ENABLE_LOGGING                 = 0;
int _USE_ACT_LED_FOR_SPACING_BLINK  = 0;
int _USE_ACT_LED_FOR_COUNTING_BLINK = 0;
int _NUMBER                         = 300;
int _MULTIPLIER                     = 1;

struct Timestamp
{
    time_t  seconds;
    long    milliseconds;
    char    timestring[32];
};

int _round(double number)
{
    return (number >= 0) ? (int)(number + 0.5) : (int)(number - 0.5);
}

struct Timestamp getTimestamp()
{
    char   timebuffer[32]     = {0};
    struct timeval  tv        = {0};
    struct tm      *tmval     = NULL;
    struct tm       gmtval    = {0};
    struct timespec curtime   = {0};
    struct Timestamp timestamp;
    int i = 0;

    // Get current time
    clock_gettime(CLOCK_REALTIME, &curtime);

    // Set the fields
    timestamp.seconds      = curtime.tv_sec;
    timestamp.milliseconds = _round(curtime.tv_nsec / 1.0e6);

    if((tmval = gmtime_r(&timestamp.seconds, &gmtval)) != NULL)
    {
        // Build the first part of the time
        strftime(timebuffer, sizeof timebuffer, "%Y-%m-%d %H:%M:%S", &gmtval);

        // Add the milliseconds part and build the time string
        snprintf(timestamp.timestring, sizeof timestamp.timestring, "%s.%03ld", timebuffer, timestamp.milliseconds);
    }

    return timestamp;
}

// length of time to sleep, in miliseconds
static void sleep_in_milisec(int milisec)
{
    struct timespec req = {0};
    int sec = 0;
    int msec = 0;

    milisec *= _MULTIPLIER;

    if (milisec >= 1000)
    {
        sec = milisec / 1000;
        msec = milisec % 1000;
    }
    else
    {
        sec = 0;
        msec = milisec;
    }

    if (_ENABLE_LOGGING == 1 || _ENABLE_LOGGING > 2)
        printf("sec=%d, msec=%d\n", sec, msec);

    req.tv_sec = sec;
    req.tv_nsec = msec * 1000000L;
    nanosleep(&req, (struct timespec *)NULL);
}

static void writelog(char *log)
{
    if (_ENABLE_LOGGING >= 2)
    {
        struct Timestamp timestamp = getTimestamp();
        printf("%s\t\t\t%s\n", log, timestamp.timestring);
    }
}

static int LED_SetValue(char *led, char *value)
{
    char buffer[100] = {0};
    ssize_t bytes_written = 0;
    int fd = 0;

    fd = open(led, O_WRONLY);
    if (-1 == fd)
    {
        fprintf(stderr, "Failed to open LED port for writing!\n");
        return(-1);
    }

    bytes_written = snprintf(buffer, sizeof(buffer), "%s", value);
    write(fd, buffer, bytes_written);
    close(fd);
    return(0);
}

static void LED_ACT_ON()
{
    LED_SetValue(ACT_LED_BRIGHTNESS, LED_ON);
}

static void LED_ACT_OFF()
{
    LED_SetValue(ACT_LED_BRIGHTNESS, LED_OFF);
}

static void LED_PWR_ON()
{
    LED_SetValue(PWR_LED_BRIGHTNESS, LED_ON);
}

static void LED_PWR_OFF()
{
    LED_SetValue(PWR_LED_BRIGHTNESS, LED_OFF);
}

static int LED_Set()
{
    int ret = 0;

    if (ret == 0) ret = LED_SetValue(ACT_LED_TRIGGER, ACT_LED_TRIGGER_CPU0);
    if (ret == 0) ret = LED_SetValue(PWR_LED_TRIGGER, PWR_LED_TRIGGER_GPIO);
    if (ret == 0)
    {
        LED_ACT_OFF();
        LED_PWR_OFF();
    }

    return(ret);
}

static void LED_Reset()
{
    int ret = 0;

    if (ret == 0) ret = LED_SetValue(ACT_LED_TRIGGER, ACT_LED_TRIGGER_MMC0);
    if (ret == 0) ret = LED_SetValue(PWR_LED_TRIGGER, PWR_LED_TRIGGER_OFF);
    if (ret == 0)
    {
        LED_ACT_OFF();
        LED_PWR_ON();
    }
}

static void CountingBlink()
{
    char *led = _USE_ACT_LED_FOR_COUNTING_BLINK ? ACT_LED_BRIGHTNESS : PWR_LED_BRIGHTNESS;

    LED_SetValue(led, LED_ON);
    sleep_in_milisec(200);

    LED_SetValue(led, LED_OFF);
    sleep_in_milisec(200);
}

static void SpacingBlink()
{
    char *led = _USE_ACT_LED_FOR_SPACING_BLINK ? ACT_LED_BRIGHTNESS : PWR_LED_BRIGHTNESS;

    LED_SetValue(led, LED_ON);
    sleep_in_milisec(1000);

    LED_SetValue(led, LED_OFF);
    sleep_in_milisec(1000);
}

static void spblink_5()
{
    writelog("Spacing blink: 1");
    SpacingBlink();

    writelog("Spacing blink: 2");
    SpacingBlink();

    writelog("Spacing blink: 3");
    SpacingBlink();

    writelog("Spacing blink: 4");
    SpacingBlink();

    writelog("Spacing blink: 5");
    SpacingBlink();
}

static void spblink_2()
{
    writelog("Spacing blink: 1");
    SpacingBlink();

    writelog("Spacing blink: 2");
    SpacingBlink();
}

static void countblink(int count)
{
    char buffer[100] = {0};
    int bytes_written = snprintf(buffer, sizeof(buffer), "%s%d", "Counting blink: ", count);

    writelog(buffer);

    for (int i=0; i < count; i++)
        CountingBlink();
}

int main(int argc, char *argv[])
{
    if ((argc > 1) && (strcmp(argv[1], "-?") == 0))
    {
        printf("\n");
        printf("\tARGV1 = Use ACT (0=False, 1=True) else use PWR LED for Spacing Blink.\n");
        printf("\tARGV2 = Use ACT (0=False, 1=True) else use PWR LED for Counting Blink.\n");
        printf("\tARGV3 = Enable Logging (0=None, 1=Minimal, 2=Detailed, 3=All)\n");
        printf("\tARGV4 = Sleep multiplier.\n");
        printf("\tARGV5 = Fibonacci Count.\n");
        printf("\n\n");

        return(0);
    }

    // Set global variables from arguments
    _USE_ACT_LED_FOR_SPACING_BLINK  = (argc > 1) ? atoi(argv[1]) : _USE_ACT_LED_FOR_SPACING_BLINK;
    _USE_ACT_LED_FOR_COUNTING_BLINK = (argc > 2) ? atoi(argv[2]) : _USE_ACT_LED_FOR_COUNTING_BLINK;
    _ENABLE_LOGGING                 = (argc > 3) ? atoi(argv[3]) : _ENABLE_LOGGING;
    _MULTIPLIER                     = (argc > 4) ? atoi(argv[4]) : _MULTIPLIER;
    _NUMBER                         = (argc > 5) ? atoi(argv[5]) : _NUMBER;

    int fib1 = 0;
    int fib2 = 1;
    int fib3 = 1;

    // Set LED ports
    if (LED_Set() != 0)
        // Exit! No need to continue if not success
        return(0);

    // Start Sequence
    spblink_5();

    while (fib3 <= _NUMBER)
    {
        spblink_2();
        countblink(fib3);

        // get next Fibonacci number
        fib3 = fib1 + fib2;
        fib1 = fib2;
        fib2 = fib3;
    }

    // End Sequence
    spblink_5();

    // Reset LEDs
    LED_Reset();

    // Exit
    return(0);
}

