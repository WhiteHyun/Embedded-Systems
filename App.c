#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wiringPi.h>
// 201601639 ==> #99, SLCK, MISO, MOSI, #101, #100, #108

//LED_UP Pin - wiringPi pin 26 is Odroid-C1 GPIO #99.
//LED_RIGHTUP Pin - wiringPi pin 14 is Odroid-C1 GPIO SLCK.
//LED_RIGHTDOWN Pin - wiringPi pin 13 is Odroid-C1 GPIO MISO.
//LED_DOWN Pin - wiringPi pin 12 is Odroid-C1 GPIO MOSI.
//LED_LEFTDOWN Pin - wiringPi pin 21 is Odroid-C1 GPIO #101.
//LED_LEFTUP Pin - wiringPi pin 22 is Odroid-C1 GPIO #100.
//LED_CENTER Pin - wiringPi pin 23 is Odroid-C1 GPIO #108.

#define LED_UP 26
#define LED_RIGHTUP 14
#define LED_RIGHTDOWN 13
#define LED_DOWN 12
#define LED_LEFTDOWN 21
#define LED_LEFTUP 22
#define LED_CENTER 23

/* Number define */
#define HEX_0 0x3F
#define HEX_1 0x06
#define HEX_2 0x5B
#define HEX_3 0x4F
#define HEX_4 0x66
#define HEX_5 0x6D
#define HEX_6 0x7D
#define HEX_7 0x27
#define HEX_8 0x7F
#define HEX_9 0x6F
#define HEX_A 0x77
#define HEX_B 0x7C
#define HEX_C 0x39
#define HEX_D 0x5E
#define HEX_E 0x79
#define HEX_F 0x71

#define LED_MAX 0x7F
void LED_Off(int *ledList);
void LED_On(int *ledList, int hex_number);

int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        printf("Error! You must type at least one argument!\n");
        return 0;
    }
    srand((unsigned)time(NULL));    //for receiving random numbers.
    int argument_1 = atoi(argv[1]); //Received the first argument and convert to int-type
    int argument_2, randNum, i;     //argument_2 = second argument, randNum = random number storage variable

    /* Array representing 7 LEDs */
    int ledList[7] = {LED_UP, LED_RIGHTUP, LED_RIGHTDOWN,
                      LED_DOWN, LED_LEFTDOWN, LED_LEFTUP, LED_CENTER};

    /* Each index contains a value that can light up each hexadecimal number with a led */
    int ledHexList[16] = {HEX_0, HEX_1, HEX_2, HEX_3,
                          HEX_4, HEX_5, HEX_6, HEX_7,
                          HEX_8, HEX_9, HEX_A, HEX_B,
                          HEX_C, HEX_D, HEX_E, HEX_F};

    /* Array that verifies that no overlap values are received */
    int randOverlapCheck[16] = {
        0,
    };
    printf("App running...\n");
    printf("Argument 1 = %d\n", argument_1); //To check the argument_1 value
    /* Argument 1 Error */
    if (argument_1 < 1 || argument_1 > 2)
    {
        printf("Error! Only 1 and 2 are available for options.\n");
        return 0;
    }
    /* Argument 2 Error */
    else if (argument_1 == 1)
    {
        if (argc > 2)
        {
            printf("Error! You can type only ./App 1\n");
            return 0;
        }
    }
    else if (argument_1 == 2)
    {
        argument_2 = strtol(argv[2], NULL, 0); //Received the second argument and convert to int-type

        /*Option Error*/
        if (argv[2][0] != '0' || argv[2][1] != 'x')
        {
            printf("Error! It's not a hex-number.\n");
            return 0;
        }
        else if (!((argv[2][2] >= '0' && argv[2][2] <= '9') || (argv[2][2] >= 'A' && argv[2][2] <= 'F') || (argv[2][2] >= 'a' && argv[2][2] <= 'f')) ||
                 !((argv[2][3] >= '0' && argv[2][3] <= '9') || (argv[2][3] >= 'A' && argv[2][3] <= 'F') || (argv[2][3] >= 'a' && argv[2][3] <= 'f') ||
                   argv[2][3] == 0))
        {
            printf("Error! You must type hex-number.\n");
            return 0;
        }
        if (argc > 3)
        {
            printf("Error! You must type less than 2 arguments.\n");
            return 0;
        }

        if (argument_2 > LED_MAX)
        {
            printf("Error! A number higher than 0x7F is not allowed.\n");
            return 0;
        }
    }
    wiringPiSetup(); //note the setup method chosen
    for (i = 0; i < 7; i++)
        pinMode(ledList[i], OUTPUT);

    if (argument_1 == 1)
    {
        for (i = 0; i < 16; i++) //output in sequence
        {
            LED_On(ledList, ledHexList[i]);
            delay(500);
            LED_Off(ledList);
        }
        for (i = 0; i < 16; i++) //output in rand
        {
            randNum = rand() % 16;
            if (randOverlapCheck[randNum] == 1) //if overlap values tried output.
            {
                i--;
                continue;
            }
            randOverlapCheck[randNum] = 1;
            LED_On(ledList, ledHexList[randNum]);
            delay(500);
            LED_Off(ledList); //turn off LED.
        }
    }
    else
    {
        printf("Argument 2 = 0x%x\n", argument_2); //To check the argument_2 value
        LED_On(ledList, argument_2);
    }

    for (i = 3; i > 0; i--)
    {
        printf("Terminated %d seconds left\n", i);
        delay(1000);
    }
    LED_Off(ledList); //turn off LED.
    return 0;
}

void LED_Off(int *ledList) //Function to turn off all LED
{
    int i;
    for (i = 0; i < 7; i++)
        digitalWrite(ledList[i], LOW);
}

void LED_On(int *ledList, int hex_number) //Function to turn on each LED
{
    int i, j;
    j = 0;
    for (i = 0x01; i <= LED_MAX; i <<= 1)
    {
        if ((hex_number & i) == i)
            digitalWrite(ledList[j], HIGH);
        j++;
    }
}
