#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>

#define LCD_D4 2
#define LCD_D5 3
#define LCD_D6 1
#define LCD_D7 4
#define LCD_RS 7
#define LCD_EN 0

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
{ //CLCD 초기화
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

    putCmd4(0x28); // 4비트 2줄
    putCmd4(0x28);
    putCmd4(0x28);
    putCmd4(0x0e); // 디스플레이온 커서 온
    putCmd4(0x02); // 커서 홈
    delay(2);
    putCmd4(0x01); // 표시 클리어
    delay(2);
}

int main(int argc, char **argv)
{
    char buf1[100] = "Welcome to";
    char buf2[100] = "Embedded World";
    int i;
    int len1 = strlen(buf1);
    int len2 = strlen(buf2);

    if (argc == 2)
    {
        len1 = strlen(argv[1]);
        len2 = 0;
        strcpy(buf1, argv[1]);
    }
    else if (argc >= 3)
    {
        len1 = strlen(argv[1]);
        len2 = strlen(argv[2]);
        strcpy(buf1, argv[1]);
        strcpy(buf2, argv[2]);
    }

    wiringPiSetup();

    initialize_textlcd();
    for (i = 0; i < len1; i++)
        putChar(buf1[i]);
    putCmd4(0xC0);
    for (i = 0; i < len2; i++)
        putChar(buf2[i]);
}
