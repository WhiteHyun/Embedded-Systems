#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wiringPi.h>

#define LCD_D4 2
#define LCD_D5 3
#define LCD_D6 1
#define LCD_D7 4
#define LCD_RS 7
#define LCD_EN 0
#define BTN_PLUS 5
#define BTN_MINUS 6
#define BTN_0 27     //#98
#define BTN_1 22     //#100
#define BTN_2 23     //#108
#define BTN_3 26     //#99
#define BTN_4 14     //SCLK
#define BTN_5 21     //#101
#define BTN_6 11     //#118
#define BTN_7 12     //MOSI
#define BTN_8 13     //MISO
#define BTN_9 10     //CEO
#define BTN_EQUAL 24 //#97

void write4bits(unsigned char command)
{
    digitalWrite(LCD_D4, (command & 1));
    command >>= 1;
    digitalWrite(LCD_D5, (command & 1));
    command >>= 1;
    digitalWrite(LCD_D6, (command & 1));
    command >>= 1;
    digitalWrite(LCD_D7, (command & 1));
    digitalWrite(LCD_EN, 1);
    delayMicroseconds(10);
    digitalWrite(LCD_EN, 0);
    delayMicroseconds(10);
}

void sendDataCmd4(unsigned char data)
{
    write4bits(((data >> 4) & 0x0f));
    write4bits((data & 0x0f));
    delayMicroseconds(100);
}

void putCmd4(unsigned char cmd)
{
    digitalWrite(LCD_RS, 0);
    sendDataCmd4(cmd);
}

void putChar(char c)
{

    digitalWrite(LCD_RS, 1);

    sendDataCmd4(c);
}

void initialize_textlcd()
{
    //CLCD 초기화
    pinMode(LCD_RS, OUTPUT);
    pinMode(LCD_EN, OUTPUT);
    pinMode(LCD_D4, OUTPUT);
    pinMode(LCD_D5, OUTPUT);
    pinMode(LCD_D6, OUTPUT);
    pinMode(LCD_D7, OUTPUT);
    digitalWrite(LCD_RS, 0);
    digitalWrite(LCD_EN, 0);
    digitalWrite(LCD_D4, 0);
    digitalWrite(LCD_D5, 0);
    digitalWrite(LCD_D6, 0);
    digitalWrite(LCD_D7, 0);
    delay(35);

    pinMode(BTN_PLUS, INPUT);
    pinMode(BTN_MINUS, INPUT);
    pinMode(BTN_7, INPUT);
    pinMode(BTN_8, INPUT);
    pinMode(BTN_9, INPUT);

    pullUpDnControl(BTN_PLUS, PUD_DOWN);
    pullUpDnControl(BTN_MINUS, PUD_DOWN);
    pullUpDnControl(BTN_7, PUD_DOWN);
    pullUpDnControl(BTN_8, PUD_DOWN);
    pullUpDnControl(BTN_9, PUD_DOWN);

    putCmd4(0x28); // 4비트 2줄 Setting (DL = 0, N = 1)
    putCmd4(0x28);
    putCmd4(0x28);
    putCmd4(0x0e); // 디스플레이온 커서 온  (D = 1, C = 1, B = 0)
    putCmd4(0x02); // 커서 홈
    delay(2);
    putCmd4(0x01); // 표시 클리어
    delay(2);
}
void waitForEnter()
{
    //Wait for push
    while (digitalRead(BTN_7) || digitalRead(BTN_8) || digitalRead(BTN_9) || digitalRead(BTN_MINUS) || digitalRead(BTN_PLUS))
        delay(1);
    //Wait for release
    while (!(digitalRead(BTN_7) || digitalRead(BTN_8) || digitalRead(BTN_9) || digitalRead(BTN_MINUS) || digitalRead(BTN_PLUS)))
        delay(1);
    printf("successfuly Entered\n");
}
int main(int argc, char **argv)
{
    wiringPiSetup();
    initialize_textlcd();
    printf("BTN_0 : %d\n", digitalRead(BTN_0));
    printf("BTN_1 : %d\n", digitalRead(BTN_1));
    printf("BTN_2 : %d\n", digitalRead(BTN_2));
    printf("BTN_3 : %d\n", digitalRead(BTN_3));
    printf("BTN_4 : %d\n", digitalRead(BTN_4));
    printf("BTN_5 : %d\n", digitalRead(BTN_5));
    printf("BTN_6 : %d\n", digitalRead(BTN_6));
    printf("BTN_7 : %d\n", digitalRead(BTN_7));
    printf("BTN_8 : %d\n", digitalRead(BTN_8));
    printf("BTN_9 : %d\n", digitalRead(BTN_9));
    printf("BTN_MINUS : %d\n", digitalRead(BTN_MINUS));
    printf("BTN_PLUS : %d\n", digitalRead(BTN_PLUS));
    printf("BTN_EQUAL : %d\n", digitalRead(BTN_EQUAL));
    // if (digitalRead(BTN_7) == HIGH)
    // {
    //     putChar('7');
    // }
    // else if (digitalRead(BTN_8) == HIGH)
    // {
    //     putChar('8');
    // }
    // else if (digitalRead(BTN_9) == HIGH)
    // {
    //     putChar('9');
    // }
    // else if (digitalRead(BTN_MINUS) == HIGH)
    // {
    //     putChar('-');
    // }
    // else if (digitalRead(BTN_PLUS) == HIGH)
    // {
    //     putChar('+');
    // }
}