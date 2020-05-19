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
#define BTN_PLUS 5   //#102
#define BTN_MINUS 6  //#103
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
int Input(int *inputSet);                                            //버튼 입력값을 받는 함수
void waitForEnter(char *expression, char *inputChar, int *inputSet); //입력 받은 것을 처리하는 함수
int calculate(bool plusOrMinus, char *start, int sum);               //주어진 식에 대해 계산하는 함수
void printResult(char *expression);                                  //계산출력용 함수
void errorPrint(int errno);                                          //에러 처리함수

int buffer = 0;     //화면 공간을 사용할 때마다 값이 1씩 증가할 변수, 32개가 넘어갈 경우 오버플로우를 출력할 때 사용함
bool debug = false; //디버그용(메인에서 들어오는 입력값에 따라 디버깅모드일지 유저모드일지 판별함)
int main(int argc, char **argv)
{
    if (argc > 2)
    {
        printf("Error! Only one parameter can be received.");
        return 0;
    }
    else if (argc == 2)
    {
        if (strcmp(argv[1], "1") != 0)
        {
            printf("Error! Only '1' can be input.");
            return 0;
        }
        else
            debug = true;
    }
    int inputSet[13] = {BTN_PLUS, BTN_MINUS, BTN_EQUAL, BTN_0, BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9};
    char inputChar[13] = {'+', '-', '=', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int outputSet[6] = {LCD_D4, LCD_D5, LCD_D6, LCD_D7, LCD_RS, LCD_EN};
    char expression[32] = {
        0,
    };
    int i;
    wiringPiSetup();
    initialize_textlcd(inputSet, outputSet);

    /* Calculate */
    while (true)
    {
        buffer = 0;              //CLCD 출력버퍼를 비움
        for (i = 0; i < 32; i++) //수식값을 NULL로 초기화
            expression[i] = 0;
        //아무 입력이 들어온 경우 가감산기 작동 시작
        while (!((!digitalRead(BTN_PLUS)) || (!digitalRead(BTN_MINUS)) || digitalRead(BTN_EQUAL) ||
                 digitalRead(BTN_0) || digitalRead(BTN_1) || digitalRead(BTN_2) ||
                 digitalRead(BTN_3) || digitalRead(BTN_4) || digitalRead(BTN_5) ||
                 digitalRead(BTN_6) || digitalRead(BTN_7) || digitalRead(BTN_8) || digitalRead(BTN_9)))
        {
            delay(35);
        }
        while ((!digitalRead(BTN_PLUS)) || (!digitalRead(BTN_MINUS)) || digitalRead(BTN_EQUAL) ||
               digitalRead(BTN_0) || digitalRead(BTN_1) || digitalRead(BTN_2) ||
               digitalRead(BTN_3) || digitalRead(BTN_4) || digitalRead(BTN_5) ||
               digitalRead(BTN_6) || digitalRead(BTN_7) || digitalRead(BTN_8) || digitalRead(BTN_9))
        {
            delay(35);
        }
        //수식을 입력받는 함수
        waitForEnter(expression, inputChar, inputSet);
        //수식을 계산하여 출력해주는 함수
        printResult(expression);
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
    pinMode(BTN_MINUS, INPUT);
    pullUpDnControl(BTN_MINUS, PUD_UP);
    pinMode(BTN_PLUS, INPUT);
    pullUpDnControl(BTN_PLUS, PUD_UP);
    for (i = 2; i < 13; i++)
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

int Input(int *inputSet)
{
    int flag = 0x0000;
    int i, state = 0;
    while (true)
    {
        if (!((!digitalRead(BTN_PLUS)) || (!digitalRead(BTN_MINUS)) || digitalRead(BTN_0) ||
              digitalRead(BTN_1) || digitalRead(BTN_2) || digitalRead(BTN_3) ||
              digitalRead(BTN_4) || digitalRead(BTN_5) || digitalRead(BTN_6) ||
              digitalRead(BTN_7) || digitalRead(BTN_8) || digitalRead(BTN_9)))
        {
            if (state == 0)
            {
                for (i = 0; i < 13; i++)
                {
                    if (i < 2 && !digitalRead(inputSet[i]))
                    {
                        flag |= 1 << i;
                        break;
                    }
                    else if (i >= 2 && digitalRead(inputSet[i]))
                    {
                        flag |= 1 << i;
                        break;
                    }
                }
                state = 1;
            }
        }
        else if ((!digitalRead(BTN_PLUS)) || (!digitalRead(BTN_MINUS)) || digitalRead(BTN_0) ||
                 digitalRead(BTN_1) || digitalRead(BTN_2) || digitalRead(BTN_3) ||
                 digitalRead(BTN_4) || digitalRead(BTN_5) || digitalRead(BTN_6) ||
                 digitalRead(BTN_7) || digitalRead(BTN_8) || digitalRead(BTN_9))
        {
            if (state == 1)
            {
                delay(35);
                state = 0;
            }
        }
        else if (digitalRead(BTN_EQUAL))
        {
            break;
        }
    }

    return flag;
}

void waitForEnter(char *expression, char *inputChar, int *inputSet)
{
    int flag; //0b0 0000 0000 0000
    //연산자 중복 확인값
    bool overlap = false;
    int i;
    int index = 0;
    putCmd4(0x01); // 표시 클리어6

    //Wait for push
    while (true)
    {
        //입력을 받을 때까지 반복
        // while ((flag = Input(inputSet)) == 0)
        // {
        //     delay(35);
        // }
        flag = Input(inputSet);
        for (i = 0; i < 13; i++)
        {
            //연산자일 경우
            if (i < 2 && ((flag & (0x0001 << i)) != 0))
            {
                if (overlap) //직전에 이미 연산자 입력을 받았을 경우
                    goto fail;
                overlap = true;
                if (debug)
                    printf("'%c' input\n", inputChar[i]);
                break;
            }
            //등호('=')를 입력받았을 경우
            if (i == 2 && ((flag & (0x0001 << i)) != 0))
            {
                if (overlap)
                    goto fail;
                //등호 입력 받은 것을 CLCD 및 수식에다가 출력
                if (debug)
                    printf("'=' input\n");
                break;
            }
            //피연산자 구분
            else if (i > 2 && ((flag & (0x0001 << i)) != 0))
            {
                overlap = false;
                if (debug)
                    printf("'%c' input\n", inputChar[i]);
                break;
            }
        }
        /*값 할당 및 CLCD 출력*/
        putChar(inputChar[i]);
        expression[index++] = inputChar[i];
        buffer++;
        delay(35);

        /*출력 제어 공간*/
        //1줄을 꽉 채운 경우
        if (buffer == 16)
            putCmd4(0xC0); //2행1열로 커서 이동
        //overflow error
        else if (buffer > 32)
        {
            printf("buffer = %d\n", buffer);
            buffer = 0;
            errorPrint(OVERFLOW);
            return;
        }
        //등호를 입력받았을 경우 버튼입력 무한 반복문 탈출
        if (i == 2)
            break;
    }
    if (debug)
    {
        printf("successfuly Entered\n");
        printf("expression %s\n", expression);
    }
    return;
fail:
    //문자열로 된 수식을 다 지움
    for (i = 0; expression[i] != 0; i++)
        expression[i] = 0;
    buffer = 0;
    //에러 출력
    errorPrint(INVALID_OPERATION);
    return;
}

int calculate(bool plusOrMinus, char *start, int sum)
{
    if (plusOrMinus)
        sum += strtol(start, NULL, 10);
    else
        sum -= strtol(start, NULL, 10);
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
    if (debug)
    {
        printf("before expression\n");
        if (strlen(expression) >= 32)
            printf("OverFlow Error\n");
        else
            for (i = 0; expression[i] != 0; i++)
                printf("%c", expression[i]);
        printf("\n");
    }
    for (i = 0; expression[i] != 0; i++)
    {
        if (expression[i] == '+')
        {
            sum = calculate(plusOrMinus, start, sum);
            plusOrMinus = true;
            start = expression + i + 1; //'+' 이후의 위치를 가리킴
            continue;
        }
        else if (expression[i] == '-')
        {
            if (i == 0)
                continue;
            sum = calculate(plusOrMinus, start, sum);
            plusOrMinus = false;
            start = expression + i + 1; //'-' 이후의 위치를 가리킴
            continue;
        }
        else if (expression[i] == '=')
        {
            if (debug)
                printf("===========equal input============\n");
            sum = calculate(plusOrMinus, start, sum);
            break;
        }
    }
    if (debug)
        printf("sum = %d\n", sum);
    for (i = 0; expression[i] != 0; i++)
        expression[i] = 0;
    sprintf(expression, "%d", sum);

    //결과값 CLCD에 출력
    for (i = 0; expression[i] != 0; i++)
    {
        if (debug)
            printf("expression[%d] = %c\n", i, expression[i]);
        putChar(expression[i]);
        buffer++;
        if (buffer == 16)
            putCmd4(0xC0); //2행1열로 커서 이동
        //overflow error
        else if (buffer > 32)
        {
            if (debug)
                printf("Interrupt, buffer = %d\n", buffer);
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