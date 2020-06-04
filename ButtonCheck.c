#include <stdio.h>
#include <time.h>   /* for time */
#include <stdlib.h> /* for exit */
#include <wiringPi.h>

/* 버튼 매크로 지정 */
#define BTN_DOT_Q_Z 24
#define BTN_A_B_C 23
#define BTN_D_E_F 27
#define BTN_G_H_I 22
#define BTN_J_K_L 21
#define BTN_M_N_O 26
#define BTN_P_R_S 7
#define BTN_T_U_V 6
#define BTN_W_X_Y 11
#define MOVE_LEFT 1
#define DELETE 4
#define MOVE_RIGHT 5
#define BTN_SIZE 12

void Init_Button(int *buttonSet, int length)
{
    int i;
    //CLCD 초기화
    delay(35);
    for (i = 0; i < length; i++)
    {
        pinMode(buttonSet[i], INPUT);
        pullUpDnControl(buttonSet[i], PUD_UP);
    }
    printf("initialize_done\n");
}

int Button_Input(int *buttons, int length)
{
    int inputBtnFlag = 0x0000;
    int i, state = 0; //state: 입력 유무 상태를 저장할 변수
    while (1)
    {
        //버튼을 눌렀을 때
        if (digitalRead(BTN_DOT_Q_Z) || digitalRead(BTN_A_B_C) || digitalRead(BTN_D_E_F) ||
            digitalRead(BTN_G_H_I) || digitalRead(BTN_J_K_L) || digitalRead(BTN_M_N_O) ||
            digitalRead(BTN_P_R_S) || digitalRead(BTN_T_U_V) || digitalRead(BTN_W_X_Y) ||
            digitalRead(MOVE_LEFT) || digitalRead(DELETE) || digitalRead(MOVE_RIGHT))
        {
            delay(10);
            if (state == 0) //입력 받은 상태가 아닌 경우
            {
                for (i = 0; i < 13; i++)
                {
                    if (digitalRead(buttons[i]))
                    {
                        inputBtnFlag |= 1 << i;
                        break;
                    }
                }
                state = 1; //입력 받은 상태 저장
            }
        }
        //버튼을 때었을 때
        else if (!(digitalRead(BTN_DOT_Q_Z) || digitalRead(BTN_A_B_C) || digitalRead(BTN_D_E_F) ||
                   digitalRead(BTN_G_H_I) || digitalRead(BTN_J_K_L) || digitalRead(BTN_M_N_O) ||
                   digitalRead(BTN_P_R_S) || digitalRead(BTN_T_U_V) || digitalRead(BTN_W_X_Y) ||
                   digitalRead(MOVE_LEFT) || digitalRead(DELETE) || digitalRead(MOVE_RIGHT)))
        {
            if (state == 1) //입력을 받아놓은 상태인 경우
            {
                delay(10);
                break;
            }
        }
    }
    return inputBtnFlag;
}

int main()
{
    int buttons[BTN_SIZE] = {BTN_DOT_Q_Z, BTN_A_B_C, BTN_D_E_F, BTN_G_H_I, BTN_J_K_L, BTN_M_N_O, BTN_P_R_S, BTN_T_U_V, BTN_W_X_Y, MOVE_LEFT, DELETE, MOVE_RIGHT};
    int dataReady;
    int i;
    wiringPiSetup();
    Init_Button(buttons, BTN_SIZE);
    // while (1)
    // {
    //     printf("Button_Input ready\n");
    //     dataReady = Button_Input(buttons, BTN_SIZE);
    //     printf("%d Input\n", dataReady);
    // }
    for (i = 0; i < BTN_SIZE; ++i)
    {
        printf("BTN_%d Input : %d\n", i, digitalRead(buttons[i]));
    }
    return 0;
}
