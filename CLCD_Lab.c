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

#define OVERFLOW 0
#define INVALID_OPERATION 1

void write4bits(unsigned char command);                              //4비트 읽어씀
void sendDataCmd4(unsigned char data);                               //8비트 데이터를 4비트씩 끊어서 보냄
void putCmd4(unsigned char cmd);                                     //명령어 입력
void putChar(char c);                                                //문자 CLCD에 넣기위한 함수
void initialize_textlcd(int *inputSet, int *outputSet);              //초기 init 함수
void waitForEnter(char *expression, char *inputChar, int *inputSet); //입력 받을 때 사용할 함수
int calculate(bool plusOrMinus, char **start, char **end, int sum);  //주어진 식에 대해 계산하는 함수
void printResult(char *expression);                                  //계산출력용 함수
void errorPrint(int errno);                                          //에러 처리함수

int count = 0; //화면에 출력될 32개가 넘어갈 경우 오버플로우를 출력하기 위한 변수

int main()
{
    int i;
    char expression[32] = {
        0,
    };

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

    initialize_textlcd(inputSet, outputSet);

    /* Calculate */
    while (true)
    {
        count = 0;
        for (i = 0; i < 32; i++)
            expression[i] = 0;
        while (!(digitalRead(inputSet[3]) || digitalRead(inputSet[4]) || digitalRead(inputSet[5]) ||
                 digitalRead(inputSet[6]) || digitalRead(inputSet[7]) || digitalRead(inputSet[8]) ||
                 digitalRead(inputSet[9]) || digitalRead(inputSet[10]) || digitalRead(inputSet[11]) || digitalRead(inputSet[12])))
        {
            delay(35);
        }
        while (digitalRead(inputSet[3]) || digitalRead(inputSet[4]) || digitalRead(inputSet[5]) ||
               digitalRead(inputSet[6]) || digitalRead(inputSet[7]) || digitalRead(inputSet[8]) ||
               digitalRead(inputSet[9]) || digitalRead(inputSet[10]) || digitalRead(inputSet[11]) || digitalRead(inputSet[12]))
        {
            delay(35);
        }
        printf("start waitForEnter Function\n");
        waitForEnter(expression, inputChar, inputSet);
        printf("end waitForEnter Function\n");

        printf("start printResult Function\n");
        printResult(expression);
        printf("end printResult Function\n");
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
    int i;
    //CLCD 초기화
    for (i = 0; i < 6; i++)
    {
        pinMode(outputSet[i], OUTPUT);
        digitalWrite(outputSet[i], 0);
    }
    delay(35);
    for (i = 0; i < 13; i++)
    {
        pinMode(inputSet[i], INPUT);
        pullUpDnControl(inputSet[i], PUD_DOWN);
    }
    delay(35);
    putCmd4(0x28); // 4비트 2줄 Setting (DL = 0, N = 1)
    putCmd4(0x28);
    putCmd4(0x28);
    putCmd4(0x0e); // 디스플레이온 커서 온  (D = 1, C = 1, B = 0)
    putCmd4(0x02); // 커서 홈
    delay(2);
    putCmd4(0x01); // 표시 클리어
    putCmd4(0x01);
    delay(2);
    printf("initialize_done\n");
}

void waitForEnter(char *expression, char *inputChar, int *inputSet)
{
    bool overlap = false;
    int flag = 0x0000;
    int i;

    int index = 0;
    putCmd4(0x01); // 표시 클리어

    //Wait for push
    while (!digitalRead(BTN_EQUAL))
    {
        for (i = 0; i < 13; i++)
        {
            if (digitalRead(BTN_EQUAL) == 1) //등호 기호가
                break;
            else if (digitalRead(inputSet[i]) == 1)
            {
                if (digitalRead(BTN_PLUS) == 1 || digitalRead(BTN_MINUS) == 1)
                {
                    if (overlap)
                        goto fail;
                    while (digitalRead(BTN_PLUS) == 1 || digitalRead(BTN_MINUS) == 1)
                        delay(35);
                    printf("%d = %c\n", i, inputChar[i]);
                    putChar(inputChar[i]);
                    expression[index++] = inputChar[i];
                    count++;
                    delay(35);
                    overlap = true;
                    continue;
                }
                //누른 버튼을 뗄 때까지 반복문을 돎
                while (digitalRead(inputSet[i]))
                    delay(35);
                printf("%d = %c\n", i, inputChar[i]);
                putChar(inputChar[i]);
                expression[index++] = inputChar[i];
                count++;
                delay(35);
                overlap = false;
                break;
            }
        }
        if (count == 16)
            putCmd4(0xC0);    //2행1열로 커서 이동
        else if (count >= 32) //overflow error
        {
            printf("count = %d\n", count);
            count = 0;
            errorPrint(OVERFLOW);
            return;
        }
    }
    while (digitalRead(BTN_EQUAL))
    {
        delay(35);
    }
    expression[index] = inputChar[2];
    putChar(inputChar[2]);
    count++;
    if (count == 16)
        putCmd4(0xC0);    //2행1열로 커서 이동
    else if (count >= 32) //overflow error
    {
        printf("count = %d\n", count);
        count = 0;
        errorPrint(OVERFLOW);
        return;
    }
    printf("successfuly Entered\n");
    printf("expression %s\n", expression);
    return;
fail:
    for (i = 0; expression[i] != 0; i++)
        expression[i] = 0;
    count = 0;
    errorPrint(INVALID_OPERATION);
    return;
}

int calculate(bool plusOrMinus, char **start, char **end, int sum)
{
    if (plusOrMinus)
    {
        sum += strtol((*start), &(*end), 10);
    }
    else
    {
        sum -= strtol((*start), &(*end), 10);
    }
    return sum;
}

void printResult(char *expression)
{
    if (expression[0] == 0)
        return;
    int i;
    int sum = 0;
    char *start = expression;
    bool plusOrMinus = true;
    char *end = NULL; //이전 숫자의 끝 부분을 저장하기 위한 포인터

    printf("before expression\n");
    for (i = 0; expression[i] != 0; i++)
        printf("%c", expression[i]);
    printf("\n");

    for (i = 0; expression[i] != 0; i++)
    {
        if (expression[i] == '+')
        {
            sum = calculate(plusOrMinus, &start, &end, sum);
            plusOrMinus = true;
            start = expression + i + 1;
            continue;
        }
        else if (expression[i] == '-')
        {
            if (i == 0)
                continue;
            sum = calculate(plusOrMinus, &start, &end, sum);
            plusOrMinus = false;
            start = expression + i + 1;
            continue;
        }
        else if (expression[i] == '=')
        {
            printf("=======================\n");
            printf("start = %d\n", *start);
            sum = calculate(plusOrMinus, &start, &end, sum);
            break;
        }
    }
    printf("sum = %d\n", sum);
    for (i = 0; expression[i] != 0; i++)
        expression[i] = 0;
    sprintf(expression, "%d", sum);
    for (i = 0; expression[i] != 0; i++)
    {
        printf("expression[%d] = %c\n", i, expression[i]);
        putChar(expression[i]);
        count++;
        if (count == 16)
            putCmd4(0xC0);    //2행1열로 커서 이동
        else if (count >= 32) //overflow error
        {
            printf("count = %d\n", count);
            errorPrint(OVERFLOW);
            return;
        }
    }
}

void errorPrint(int errno)
{
    int i;
    char *err[3] = {"    Overflow",
                    "    Invalid",
                    "   operation"};

    putCmd4(0x02); // 커서 홈
    delay(2);
    putCmd4(0x01); // 표시 클리어
    delay(2);
    if (errno == OVERFLOW)
    {
        printf("OVERFLOW ERROR TRAPED, errno = %d\n", errno);
        for (i = 0; err[0][i] != 0; i++)
            putChar(err[0][i]);
    }
    else if (errno = INVALID_OPERATION)
    {
        printf("INVALID_OPERATION ERROR TRAPED, errno = %d\n", errno);
        for (i = 0; err[1][i] != 0; i++)
            putChar(err[1][i]);
        putCmd4(0xC0);
        for (i = 0; err[2][i] != 0; i++)
            putChar(err[2][i]);
    }
    delay(2000);
    putCmd4(0x01); // 표시 클리어
    putCmd4(0x02); // 커서 홈
}