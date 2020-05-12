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

#define CMD_CURSURMOVE 0x10 //0001 0000 (S/C = 0, R/L = 0)

void write4bits(unsigned char command);                                //4비트 읽어씀
void sendDataCmd4(unsigned char data);                                 //8비트 데이터를 4비트씩 끊어서 보냄
void putCmd4(unsigned char cmd);                                       //명령어 입력
void putChar(char c);                                                  //문자 CLCD에 넣기위한 함수
void initialize_textlcd(int *inputSet, int *outputSet);                //초기 init 함수
void waitForEnter(char **expression, char *inputChar, char *inputSet); //입력 받을 때 사용할 함수
void calculate(bool plusOrMinus, char **start, char **end);            //주어진 식에 대해 계산하는 함수
void printResult(char *expression, int sum);                           //계산출력용 함수

int main()
{
    char expression[32] = {
        NULL,
    };

    int sum = 0;

    int inputSet[13] = {BTN_PLUS, BTN_MINUS, BTN_EQUAL, BTN_0, BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9};
    char inputChar[13] = {'+', '-', '=', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int outputSet[6] = {LCD_D4, LCD_D5, LCD_D6, LCD_D7, LCD_RS, LCD_EN};
    wiringPiSetup();
    // printf("BTN_0 : %d\n", digitalRead(BTN_0));
    // printf("BTN_1 : %d\n", digitalRead(BTN_1));
    // printf("BTN_2 : %d\n", digitalRead(BTN_2));
    // printf("BTN_3 : %d\n", digitalRead(BTN_3));
    // printf("BTN_4 : %d\n", digitalRead(BTN_4));
    // printf("BTN_5 : %d\n", digitalRead(BTN_5));
    // printf("BTN_6 : %d\n", digitalRead(BTN_6));
    // printf("BTN_7 : %d\n", digitalRead(BTN_7));
    // printf("BTN_8 : %d\n", digitalRead(BTN_8));
    // printf("BTN_9 : %d\n", digitalRead(BTN_9));
    // printf("BTN_MINUS : %d\n", digitalRead(BTN_MINUS));
    // printf("BTN_PLUS : %d\n", digitalRead(BTN_PLUS));
    // printf("BTN_EQUAL : %d\n", digitalRead(BTN_EQUAL));

    initialize_textlcd();
    /*Calculate*/
    while (true)
    {
        for (int i = 0; i < 32; i++)
            expression[i] = NULL;
        waitForEnter(&expression, inputChar, inputSet);
        printResult(expression, sum);
    }
    return 0;
}
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

void initialize_textlcd(int *inputSet, int *outputSet)
{
    //CLCD 초기화
    for (int i = 0; i < 6; i++)
    {
        pinMode(outputSet[i], OUTPUT);
        digitalWrite(outputSet[i], 0);
    }
    delay(35);
    for (int i = 0; i < 13; i++)
    {
        pinMode(outputSet[i], INPUT);
        pullUpDnControl(outputSet[i], PUD_DOWN);
    }
    putCmd4(0x28); // 4비트 2줄 Setting (DL = 0, N = 1)
    putCmd4(0x28);
    putCmd4(0x28);
    putCmd4(0x0e); // 디스플레이온 커서 온  (D = 1, C = 1, B = 0)
    putCmd4(0x02); // 커서 홈
    delay(2);
    putCmd4(0x01); // 표시 클리어
    delay(2);
}

void waitForEnter(char **expression, char *inputChar, char *inputSet)
{
    int count = 0;
    int index = 0;
    putCmd4(0x01); // 표시 클리어

    //Wait for push
    while (!digitalRead(BTN_EQUAL))
    {
        for (int i = 0; i < 13; i++)
        {
            if (digitalRead(inputSet[i]) == HIGH)
            {
                putChar(inputChar[i]);
                (*expression)[index++] = inputChar[i];
                putCmd4(CMD_CURSURMOVE | 4);
                count++;
                break;
            }
        }
        if (count == 16)
            putCmd4(0xC0); //2행1열로 커서 이동
    }
    (*expression)[index] = inputChar[2];
    putChar(inputChar[2]);
    printf("successfuly Entered\n");
    printf("expression %s\n", (*expression));
}

void calculate(bool plusOrMinus, char **start, char **end)
{
    if (plusOrMinus)
    {
        sum += strtol(start, &(*end));
    }
    else
    {
        sum -= strtol(start, &(*end));
    }
}

void printResult(char *expression, int sum)
{
    int i;
    char *start = expression;
    bool plusOrMinus = true;
    char *end; //이전 숫자의 끝 부분을 저장하기 위한 포인터
    for (i = 0; expression[i] != NULL; i++)
    {
        if (strcmp(expression[i], '+') == 0)
        {
            calculate(plusOrMinus, &start, &end);
            plusOrMinus = true;
            start = expression + i + 1;
            continue;
        }
        else if (strcmp(expression[i], '-') == 0)
        {
            calculate(plusOrMinus, &start, &end);
            plusOrMinus = false;
            start = expression + i + 1;
            continue;
        }
        else if (strcmp(expression[i], '=') == 0)
        {
            calculate(plusOrMinus, &start, &end);
            break;
        }
    }

    for (i = 0; expression[i] != NULL; i++)
        expression[i] = NULL;
    itoa(sum, expression, 10);
    for (i = 0; expression[i] != NULL; i++)
    {
        putChar(expression[i]);
        putCmd4(CMD_CURSURMOVE | 4);
    }
}
