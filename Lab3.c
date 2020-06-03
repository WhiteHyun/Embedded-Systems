#include <stdio.h>
#include <time.h>      /* for time */
#include <stdlib.h>    /* for exit */
#include <unistd.h>    /* for open/close .. */
#include <fcntl.h>     /* for O_RDWR */
#include <sys/ioctl.h> /* for ioctl */
#include <sys/mman.h>  /* for mmap */
#include <linux/fb.h>  /* for fb_var_screeninfo, FBIOGET_VSCREENINFO */
#include <wiringPi.h>
#define FBDEVFILE "/dev/fb2"

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
#define BLACK 0
#define WHITE 0xFFFF
#define MAX_BUF_SIZE 80
#define BTN_SIZE 12
typedef unsigned char ubyte;

//색 정보를 16bit로 변환해 주는 함수
unsigned short makepixel(ubyte r, ubyte g, ubyte b)
{
    return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void Cursor_Blink(time_t *originalTime)
{
    int currentTime = time(NULL);
    if (currentTime - (*originalTime) < 1000)
        return;
    else
    {
        (*originalTime) = currentTime;
        /* TODO: 커서 깜빡임 구현 */
    }
}

void Init_Button(int *buttonSet, int length)
{
    int i;
    //CLCD 초기화
    delay(35);
    for (i = 0; i < length; i++)
    {
        pinMode(buttonSet[i], INPUT);
        pullUpDnControl(buttonSet[i], PUD_DOWN);
    }
}

int Init_TFT(int *fbfd, struct fb_var_screeninfo *fbvar, int *pfbdata)
{
    int ret = -1;
    (*fbfd) = open(FBDEVFILE, O_RDWR);
    if ((*fbfd) < 0)
    {
        perror("fbdev open");
        goto done;
    }

    ret = ioctl((*fbfd), FBIOGET_VSCREENINFO, &(*fbvar));
    if (ret < 0)
    {
        perror("fbdev ioctl");
        goto done;
    }

    if (fbvar->bits_per_pixel != 16)
    {
        fprintf(stderr, "bpp is not 16\n");
        goto done;
    }
    (*pfbdata) = (unsigned short *)mmap(0, fbvar->xres * fbvar->yres * 16 / 8, PROT_READ | PROT_WRITE, MAP_SHARED, (*fbfd), 0);
    if ((unsigned)(*pfbdata) == (unsigned)-1)
    {
        perror("fbdev mmap");
        goto done;
    }
    ret = 0;
done:
    return ret;
}

void Init_LCDImage(int ***alphabet, int alphabetIndex, int alphabetValue)
{
    /*
     * a = {0, 0, 0x3FF80, 0xFFF80, 0xFFF80, 0xF0000, 0xF0000, 0xF0000, 0xF0000, 0xFFFE0, 0xFFFE0, 0xFFFF8, 0xF0078, 0xF0078, 0xF0078, 0xF0078, 0xFFFF8, 0xFFFF8, 0xFFFE0, 0xFFFE0, 0, 0, 0, 0 }
     * 
     */
    int i, j;
    /* TODO: 알파벳 폰트 구현 */
}

int Button_Input(int *buttons, int length, time_t *t)
{
    int inputBtnFlag = 0x0000;
    int i, state = 0; //state: 입력 유무 상태를 저장할 변수
    while (true)
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
        Cursor_Blink(t);
    }
    return inputBtnFlag;
}

void String_Process_Function()
{
    /* 
     * TODO: 입력받은 문자 처리
     * 버튼 중첩을 어떻게 구현할 것인가?
     * 커서의 위치는 어떻게 지정되어야하는가?
     * 새 문자를 출력할 때 기존 버튼을 누른건지 새 버튼을 누른건지에 대한 인식처리는 어떻게 해야하는가?
     */
}

void LCDPrint(int *buttons, int length, time_t *t)
{
    /* 
     * TODO: 처리된 문자 출력 구현
     * 화면을 넘어가게 되는 즉시 다음 줄로 넘어가게 구현
     * 커서의 깜빡임 처리는 어떻게 구현되어야 하는가? 따로 메소드를 생성하여 구현?
     * delete의 경우 맨 마지막 문자를 지워야하는데 어떻게 구현해야하는가?
     * insert의 경우 그 뒤의 문자들을 어떻게 처리해야하는가?
     */
}

int main()
{
    int buttons[BTN_SIZE] = {BTN_DOT_Q_Z, BTN_A_B_C, BTN_D_E_F, BTN_G_H_I, BTN_J_K_L, BTN_M_N_O, BTN_P_R_S, BTN_T_U_V, BTN_W_X_Y, MOVE_LEFT, DELETE, MOVE_RIGHT};
    int buttonCount[BTN_SIZE] = {
        0,
    };
    char buffer[MAX_BUF_SIZE] = {
        0,
    };
    int alphabet[27][24] = {
        0,
    };
    struct fb_var_screeninfo fbvar;
    int fbfd;
    int offset;
    int dataReady;                   //버튼 입력 flag 변수
    time_t currentTime = time(NULL); //깜빡이는 커서 체크용 변수
    unsigned short pixel;
    unsigned short *pfbdata; //시작주소를 가리킴

    /* Setting */
    wiringPiSetup();
    Init_Button(buttons, BTN_SIZE);
    if (Init_TFT(&fbfd, &fbvar, &pfbdata) < 0)
        exit(1);
    Init_LCDImage(&alphabet, 27, 24);
    while (1)
    {
        /* TODO: 처리 구현 */
        dataReady = Button_Input(buttons, BTN_SIZE, &currentTime);
    }

    /* Free Memory */
    munmap(pfbdata, fbvar.xres * fbvar.yres * 16 / 8);
    close(fbfd);
    return 0;
}
