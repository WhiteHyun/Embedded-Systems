#include <stdio.h>
#include <stdbool.h>   /* for true/false */
#include <time.h>      /* for time */
#include <stdlib.h>    /* for exit */
#include <unistd.h>    /* for open/close .. */
#include <fcntl.h>     /* for O_RDWR */
#include <sys/ioctl.h> /* for ioctl */
#include <sys/mman.h>  /* for mmap */
#include <linux/fb.h>  /* for fb_var_screeninfo, FBIOGET_VSCREENINFO */
#include <wiringPi.h>
#define FBDEVFILE "/dev/fb2"

/* 버튼 열거형 지정 */
enum BTN
{
    BTN_DOT_Q_Z = 24,
    BTN_A_B_C = 23,
    BTN_D_E_F = 27,
    BTN_G_H_I = 22,
    BTN_J_K_L = 21,
    BTN_M_N_O = 26,
    BTN_P_R_S = 7,
    BTN_T_U_V = 6,
    BTN_W_X_Y = 11,
    MOVE_LEFT = 1,
    DELETE = 4,
    MOVE_RIGHT = 5
};
/* 색상 열거형 지정 */
enum color
{
    BLACK = 0,
    WHITE = 0xFFFF
};
/* 크기 열거형 지정 */
enum size
{
    MAX_BUF_SIZE = 80,
    BTN_SIZE = 12,
    CHAR_PIXEL_SIZE = 24
};

/* TFT-LCD에 대한 주소나 정보들을 저장해놓은 구조체 */
struct TFT_LCD_Info
{
    struct fb_var_screeninfo fbvar; //LCD정보들을 가지고 있음
    int fbfd;
    unsigned short *pfbdata; //LCD의 시작주소를 가리킴
};
/* 커서의 동작정보를 담은 구조체 */
struct Cursor
{
    time_t originalTime;         //커서 플리킹 사이클 체크변수
    unsigned short isFlickering; //커서 플리킹 체크변수
    unsigned short offset_x;     //커서 x의 위치를 나타내는 변수
    unsigned short offset_y;     //커서 y의 위치를 나타내는 변수
};
/* 버튼구성 및 버튼중첩을 담은 구조체 */
struct Button
{
    int buttons[BTN_SIZE];  //버튼 wiringPi 넘버로 구성
    int overlap[BTN_SIZE];  //중첩되어 눌려져있는지를 체크하는 변수
    bool focused[BTN_SIZE]; //포커스 유무 판단 변수
};
void Cursor_Blink(struct Cursor *cursor, struct TFT_LCD_Info LCD_info)
{
    int draw_offset; //임시 LCD포인터
    int i;
    time_t currentTime = time(NULL);
    if (currentTime - cursor->originalTime < 1) //1초 이상 지나지 않은 경우 단순리턴
        return;
    else //1초 이상 지나면 커서 깜빡임
    {
        cursor->originalTime = currentTime;
        if (cursor->isFlickering == 0) //깜빡이고있지 않을 때
        {
            cursor->isFlickering = 1; //Blink!
            for (i = 0; i < 24; i++)
            {
                draw_offset = (cursor->offset_y + 23) * LCD_info.fbvar.xres + cursor->offset_x + i;
                *(LCD_info.pfbdata + draw_offset) = WHITE; /* draw pixel */
            }
        }
        else //깜빡이고 있을 때
        {
            cursor->isFlickering = 0; //Cursor off!
            for (i = 0; i < 24; i++)
            {
                draw_offset = (cursor->offset_y + 23) * LCD_info.fbvar.xres + cursor->offset_x + i;
                *(LCD_info.pfbdata + draw_offset) = BLACK; /* draw pixel */
            }
        }
    }
}

/*
 * 버튼과 커서의 정보들을 시작 전 초기화해주는 메소드
 * 초반 커서의 시작 지점은 0으로 설정함
 * 버튼은 눌러져 있지 않을 때를 1로 설정함
 */
void Init_ButtonAndCursor(struct Button *buttonInfo, struct Cursor *cursor)
{
    int i;
    int buttonSet[BTN_SIZE] = {BTN_DOT_Q_Z, BTN_A_B_C, BTN_D_E_F, BTN_G_H_I, BTN_J_K_L, BTN_M_N_O, BTN_P_R_S, BTN_T_U_V, BTN_W_X_Y, MOVE_LEFT, DELETE, MOVE_RIGHT};
    /* init Button */
    for (i = 0; i < BTN_SIZE; i++)
    {
        buttonInfo->overlap[i] = 0;
        buttonInfo->focused[i] = false;
        buttonInfo->buttons[i] = buttonSet[i];
        pinMode(buttonInfo->buttons[i], INPUT);
        pullUpDnControl(buttonInfo->buttons[i], PUD_UP);
    }
    /* init Cursor */
    cursor->isFlickering = 0;
    cursor->offset_x = 0;
    cursor->offset_y = 0;
    cursor->originalTime = time(NULL);
}

int Init_TFT(struct TFT_LCD_Info *LCD_info)
{
    int ret = -1;
    LCD_info->fbfd = open(FBDEVFILE, O_RDWR);
    if (LCD_info->fbfd < 0)
    {
        perror("fbdev open");
        goto done;
    }

    ret = ioctl(LCD_info->fbfd, FBIOGET_VSCREENINFO, &(LCD_info->fbvar));
    if (ret < 0)
    {
        perror("fbdev ioctl");
        goto done;
    }

    if (LCD_info->fbvar.bits_per_pixel != 16)
    {
        fprintf(stderr, "bpp is not 16\n");
        goto done;
    }
    LCD_info->pfbdata = (unsigned short *)mmap(0, LCD_info->fbvar.xres * LCD_info->fbvar.yres * 16 / 8, PROT_READ | PROT_WRITE, MAP_SHARED, LCD_info->fbfd, 0);
    if ((unsigned)LCD_info->pfbdata == (unsigned)-1)
    {
        perror("fbdev mmap");
        goto done;
    }
    ret = 0;
done:
    return ret;
}

void Init_LCDImage(int alphabetIndex, int alphabetValue)
{
    /*
     * a = {0, 0, 0x3FF80, 0xFFF80, 0xFFF80, 0xF0000, 0xF0000, 0xF0000, 0xF0000, 0xFFFE0, 0xFFFE0, 0xFFFF8, 0xF0078, 0xF0078, 0xF0078, 0xF0078, 0xFFFF8, 0xFFFF8, 0xFFFE0, 0xFFFE0, 0, 0, 0, 0 }
     * 
     */
    int i, j;
    /* TODO: 알파벳 폰트 구현 */
}

int Button_Input(int *buttons, struct Cursor *cursor_info, struct TFT_LCD_Info LCD_info)
{
    int inputBtnFlag = 0x0000;
    int i, state = 0; //state: 입력 유무 상태를 저장할 변수
    printf("Button Input\n");
    while (true)
    {
        //버튼을 눌렀을 때
        if (!digitalRead(BTN_DOT_Q_Z) || !digitalRead(BTN_A_B_C) || !digitalRead(BTN_D_E_F) ||
            !digitalRead(BTN_G_H_I) || !digitalRead(BTN_J_K_L) || !digitalRead(BTN_M_N_O) ||
            !digitalRead(BTN_P_R_S) || !digitalRead(BTN_T_U_V) || !digitalRead(BTN_W_X_Y) ||
            !digitalRead(MOVE_LEFT) || !digitalRead(DELETE) || !digitalRead(MOVE_RIGHT))
        {
            delay(10);
            if (state == 0) //입력 받은 상태가 아닌 경우
            {
                for (i = 0; i < BTN_SIZE; i++)
                {
                    if (!digitalRead(buttons[i]))
                    {
                        inputBtnFlag |= 1 << i;
                        break;
                    }
                }
                state = 1; //입력 받은 상태 저장
            }
        }
        //버튼을 때었을 때
        else if (digitalRead(BTN_DOT_Q_Z) && digitalRead(BTN_A_B_C) && digitalRead(BTN_D_E_F) &&
                 digitalRead(BTN_G_H_I) && digitalRead(BTN_J_K_L) && digitalRead(BTN_M_N_O) &&
                 digitalRead(BTN_P_R_S) && digitalRead(BTN_T_U_V) && digitalRead(BTN_W_X_Y) &&
                 digitalRead(MOVE_LEFT) && digitalRead(DELETE) && digitalRead(MOVE_RIGHT))
        {
            if (state == 1) //입력을 받아놓은 상태인 경우
            {
                delay(10);
                break;
            }
        }
        Cursor_Blink(cursor_info, LCD_info);
    }
    return inputBtnFlag;
}

void Button_Process_Function(struct Button buttonInfo, int flag)
{
    /* 
     * TODO: 입력받은 문자 처리
     * 버튼 중첩을 어떻게 구현할 것인가? ==>overlap 변수를 두어 해결할 것임
     * 커서의 위치는 어떻게 지정되어야하는가?   ==> Cursor 구조체의 offset을 두어 해결할 것
     * 새 문자를 출력할 때 기존 버튼을 누른건지 새 버튼을 누른건지에 대한 인식처리는 어떻게 해야하는가?
     * Insert할 때 어떻게 처리하여야하는가?
     * LCDPrint 메소드에 정보를 잘 전달해주기 위해 무엇을 하여야 하는가?
     */
    int i;
    char *currentString[MAX_BUF_SIZE] = {
        0,
    };
    for (i = 0; i < 13; i++)
    {
        //문자버튼을 입력한 경우
        if (i < 9 && ((flag & (0x0001 << i)) != 0))
        {
            printf("'%d' input\n", buttonInfo.buttons[i]);
            break;
            /*
             * TODO: 만약 문자 입력으로 인한 focus가 되어 있지 않을 때
             * 뒤에 문자가 입력되어 있지 않는 경우(현재 입력하고자 하는 공간에 문자가 없는 경우)문자 기입
             * 문자가 존재하는 경우(INSERT)
             * 그 뒤의 문자들을 뒤로 한 칸씩 넘김, 이 때 공간이 FULL인 경우 아무 것도 하지 않을지 스크린 뒤에 글을 쓰게끔 할지 미지수..
             * 만약 문자 입력으로 인한 focus가 되어 있을 때
             * 그 focus된 문자를 또 입력하는 경우
             * 그에 맞는 문자로 전환, 커서 이동은 없음
             * focus된 문자를 입력하는 것이 아닌 경우
             * 커서 자동으로 1 증가 후 문자 작성. 이 때도 커서의 위치나 뒤의 문자가 있는지에 대해서도 잘 살펴보아야함. 커서 위치는 goto문으로 쓸지 고민!
             */
        }
        //커서왼쪽이동버튼을 누른 경우
        else if (i == 9 && ((flag & (0x0001 << i)) != 0))
        {
        moveLeft:
            /* 
             * TODO: 만약 문자 입력으로 인한 focus가 되어 있지 않은 상태일 때
             * x축이 0을 해당하고있으면서 y도 0인 경우 아무것도 하지 않음
             * x축이 0이면서 y축이 어느정도의 공간을 차지하고 있는 경우 y축을 한칸 낮추고 x축 끝에 커서를 둚
             * 둘 다 아니면 왼쪽으로 이동
             * 문자가 focus가 되어있는 경우
             * focus를 풀고 커서의 이동은 아무것도 하지 않음
             */
        }
        //커서오른쪽이동버튼을 누른 경우
        else if (i == 11 && ((flag & (0x0001 << i)) != 0))
        {
        moveRight:
            /*
             * TODO: 만약 focus가 되어 있지 않은 상태에서 
             * x축이 끝자락에 있으면서 y축도 끝자락인 경우 아무것도 하지 않게 할건지 한 줄을 건너 뛴 것처럼 보이게 만들건지 선택해야함!(어떻게 구현할까?)
             * x축이 끝자락이면서 y축이 끝자락에 있지 않는 경우 y축을 한칸 높이고 x축 맨 앞에 커서를 둚
             * 만약 커서가 입력된 문자보다 "두 칸" 더 많이 이동하려 하는 경우일 때도 아무것도 하지 않아야 함
             * 문자가 focus가 되어있는 경우 
             * focus를 풀고 커서의 이동은 아무것도 하지 않음
             */
        }
        //DELETE버튼을 누른 경우
        else if (i == 10 && ((flag & (0x0001 << i)) != 0))
        {
            /*
             * TODO: 해당 커서에 있는 문자가 없으면 아무 것도 하지 않음
             * 문자가 있고 뒤에 문자가 없는 경우, 해당 문자만 지우고 커서 한 칸 감소. 이 때 감소할 때에도 커서의 위치지정을 잘 해야 함 goto문으로?
             * 문자가 있고 뒤에도 문자가 있는 경우, 해당 문자 지우고 문자들 한 칸씩 앞으로 당겨 씀. 커서 위치는 그대로 내버려 둚.
             */
        }
    }
}

void LCDPrint(struct Button *button_info)
{
    /* 
     * TODO: 처리된 문자 출력 구현
     * 화면을 넘어가게 되는 즉시 다음 줄로 넘어가게 구현
     * delete의 경우 맨 마지막 문자를 지워야하는데 어떻게 인식시키고 출력해줄 수 있는가?
     * insert의 경우 그 뒤의 문자들을 어떻게 처리해야하는가?
     * 커서를 옮길경우 전의 커서 깜빡임 처리는 어떻게 해야하는가?
     */
}

int main()
{
    char buffer[MAX_BUF_SIZE] = {
        0,
    };
    int a[24] = {0, 0, 0x3FF80, 0xFFF80, 0xFFF80, 0xF0000, 0xF0000, 0xF0000, 0xF0000, 0xFFFE0, 0xFFFE0, 0xFFFF8, 0xF0078, 0xF0078, 0xF0078, 0xF0078, 0xFFFF8, 0xFFFF8, 0xFFFE0, 0xFFFE0, 0, 0, 0, 0};
    struct TFT_LCD_Info LCD_info;
    struct Cursor cursor_info;
    struct Button button_info;
    int dataReady; //버튼 입력 flag 변수

    /* Setting */
    wiringPiSetup();
    Init_ButtonAndCursor(&button_info, &cursor_info);
    if (Init_TFT(&LCD_info) < 0)
        exit(1);

    Init_LCDImage(27, 24); //아직 미구현되어있음
    printf(" isFlickering=%d\n offsetX=%d\n offsetY=%d\n originalTime=%ld\n", cursor_info.isFlickering, cursor_info.offset_x, cursor_info.offset_y, cursor_info.originalTime);
    while (1)
    {
        /* TODO: 처리 구현 */
        dataReady = Button_Input(button_info.buttons, &cursor_info, LCD_info);
        Button_Process_Function(button_info, dataReady);
        LCDPrint(&button_info);
    }

    /* Free Memory */
    munmap(LCD_info.pfbdata, LCD_info.fbvar.xres * LCD_info.fbvar.yres * 16 / 8);
    close(LCD_info.fbfd);
    return 0;
}
