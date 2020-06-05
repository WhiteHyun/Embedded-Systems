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
    CHAR_PIXEL_SIZE = 24,
    NUM_CHAR = 27
};

/* 중첩상태 열거형 지정 */
enum buffer_state
{
    _null = -1,
    _spacing = 0,
    _default = 1
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
    unsigned short offset_x;     //실제 커서 x의 위치를 나타내는 변수
    unsigned short offset_y;     //실제 커서 y의 위치를 나타내는 변수
    int pointer;                 //버퍼 배열에서 커서위치를 가리키는 변수
};
/* 버튼구성 구조체 */
struct Button
{
    int buttons[BTN_SIZE]; //버튼 wiringPi 넘버로 구성
};
/* 버퍼(24x24픽셀 공간에서의 값)의 정보를 담을 구조체 */
struct Buffer
{
    int buttonNum;   //어느 버튼으로 정보가 저장되어있는지를 확인해주는 메인 변수
    int overlap;     //중첩되어 눌려져있는지를 체크하는 변수
    bool focused;    //포커스 유무 판단 변수
    bool valueExist; //버퍼의 값이 존재하는지의 유무
};
/* LCD에 보여줄 정보를 가지고 있는 구조체 */
struct Screen
{
    int alphabet[NUM_CHAR][CHAR_PIXEL_SIZE]; //알파벳 구성 값('a~z', '.')
    struct Buffer buffer[MAX_BUF_SIZE];      //80자의 문자를 구성할 버퍼의 정보
};

/*
 * 커서를 깜빡이게 만들어주는 메소드
 * 1초마다 한 번씩 깜빡이게 만들어 둚
 */
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
void Init_Button_Cursor(struct Button *buttonInfo, struct Cursor *cursor)
{
    int i;
    int buttonSet[BTN_SIZE] = {BTN_DOT_Q_Z, BTN_A_B_C, BTN_D_E_F, BTN_G_H_I, BTN_J_K_L, BTN_M_N_O, BTN_P_R_S, BTN_T_U_V, BTN_W_X_Y, MOVE_LEFT, DELETE, MOVE_RIGHT};
    /* init Button */
    for (i = 0; i < BTN_SIZE; i++)
    {
        buttonInfo->buttons[i] = buttonSet[i];
        pinMode(buttonInfo->buttons[i], INPUT);
        pullUpDnControl(buttonInfo->buttons[i], PUD_UP);
    }
    /* init Cursor */
    cursor->originalTime = time(NULL);
    cursor->isFlickering = 0;
    cursor->offset_x = 0;
    cursor->offset_y = 0;
    cursor->pointer = 0;
}

/*
 * TFT-LCD의 정보들을 초기화
 */
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

/*
 * Screen의 버퍼와 알파벳을 설정 및 초기화함
 */
void Init_Screen(struct Screen *screen)
{
    int alphabet[NUM_CHAR][CHAR_PIXEL_SIZE] = {
        {//dot
         0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0xF0, 0xF0,
         0xF0, 0xF0, 0, 0, 0, 0},
        {//q
         0, 0, 0, 0x7FF80, 0x7FF80, 0x1FFFE0,
         0x1E0078, 0x1E0078, 0x1E0078, 0x1E0078, 0x1E0078, 0x1E0078,
         0x1E0078, 0x1E0078, 0x1FFFE0, 0x1FFFE0, 0x1FFF80, 0x1E0000,
         0x1E0000, 0x1E0000, 0x1E0000, 0x1E0000, 0x1E0000, 0},
        {//z
         0, 0, 0, 0, 0, 0,
         0x7FFE0, 0x7FFE0, 0x78000, 0x78000, 0x3C000, 0x1F000,
         0xF800, 0x1F00, 0x1F80, 0x7C0, 0x1E0, 0x3E0,
         0x7FFE0, 0x7FFE0, 0, 0, 0, 0},
        {//a
         0, 0, 0x3FF80, 0xFFF80, 0xFFF80, 0xF0000,
         0xF0000, 0xF0000, 0xF0000, 0xFFFE0, 0xFFFE0, 0xFFFF8,
         0xF0078, 0xF0078, 0xF0078, 0xF0078, 0xFFFF8, 0xFFFF8,
         0xFFFE0, 0xFFFE0, 0, 0, 0, 0},
        {//b
         0, 0x70, 0x70, 0x70, 0x70, 0x3FFF0,
         0xFFFF0, 0xFFFF0, 0x3C0070, 0x3C0070, 0x3C0070, 0x3C0070,
         0x3C0070, 0x3C0070, 0x3C0070, 0x3C0070, 0x3C0070, 0xFFFF0,
         0x3FFF0, 0x3FFF0, 0, 0, 0, 0},
        {//c
         0, 0x7FFE0, 0x7FFE0, 0x1FFFF8, 0x1FFFF8, 0x1E0078,
         0x1E0078, 0x78, 0x78, 0x78, 0x78, 0x78,
         0x78, 0x78, 0x1E0078, 0x1E0078, 0x1FFFF8, 0x1FFFF8,
         0x7FFF0, 0x7FFE0, 0, 0, 0, 0},
        {//d
         0, 0xE0000, 0xE0000, 0xE0000, 0xE0000, 0xFFFC0,
         0xFFFF0, 0xFFFF0, 0xE003C, 0xE003C, 0xE003C, 0xE003C,
         0xE003C, 0xE003C, 0xE003C, 0xE003C, 0xE003C, 0xFFFF0,
         0xFFFC0, 0xFFFC0, 0, 0, 0, 0},
        {//e
         0, 0, 0, 0x7FFC0, 0x7FFC0, 0xFFFE0,
         0xE00E0, 0xE00E0, 0xE00E0, 0xE00E0, 0xFFFE0, 0x7FFE0,
         0x3FFE0, 0xE0, 0xE0, 0xE0, 0xE0, 0x7FFE0,
         0x7FF80, 0x7FF80, 0, 0, 0, 0},
        {//f
         0, 0x3E00, 0x7E00, 0xFF80, 0xC780, 0x780,
         0x780, 0x1FFC0, 0x1FFC0, 0x1FFC0, 0x780, 0x780,
         0x780, 0x780, 0x780, 0x780, 0x780, 0x780,
         0x780, 0x780, 0, 0, 0, 0},
        {//g
         0, 0, 0, 0, 0xFFF80, 0xFFFC0,
         0xFFFE0, 0xE0070, 0xE0070, 0xE0070, 0xE0070, 0xE0070,
         0xE0070, 0xE0070, 0xE0070, 0xFFFE0, 0xFFF80, 0xFFF00,
         0xF0000, 0xF0000, 0xFFF80, 0x3FF80, 0x3FF80, 0},
        {//h
         0, 0x3C, 0x3C, 0x3C, 0x3C, 0x3C,
         0x3C, 0x3FFFC, 0x7FFFC, 0x1FFFFC, 0x1E003C, 0x1E003C,
         0x1E003C, 0x1E003C, 0x1E003C, 0x1E003C, 0x1E003C, 0x1E003C,
         0x1E003C, 0x1E003C, 0, 0, 0, 0},
        {//i
         0, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0,
         0, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00,
         0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00,
         0x3C00, 0x3C00, 0, 0, 0, 0},
        {//j
         0, 0, 0, 0x1E000, 0x1E000, 0x1E000,
         0, 0x1E000, 0x1E000, 0x1E000, 0x1E000, 0x1E000,
         0x1E000, 0x1E000, 0x1E000, 0x1E000, 0x1E000, 0x1E000,
         0x1E000, 0x1E000, 0x1FFC0, 0xFFC0, 0x7FC0, 0},
        {//k
         0, 0, 0x70, 0x70, 0x70, 0x70,
         0x70, 0x78070, 0x7C070, 0x3E070, 0x3F870, 0x7E70,
         0x1FF0, 0x1FF0, 0x7FF0, 0x7F870, 0x7E070, 0xF8070,
         0xF0070, 0xE0070, 0, 0, 0, 0},
        {//l
         0, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00,
         0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00,
         0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00, 0x3C00,
         0x3C00, 0x3C00, 0, 0, 0, 0},
        {//m
         0, 0, 0, 0, 0, 0,
         0x3C7F0, 0x7C7F8, 0xFFFF8, 0xFFFF8, 0x1C3C38, 0x1C3C38,
         0x1C3C38, 0x1C3C38, 0x1C3C38, 0x1C3C38, 0x1C3C38, 0x1C3C38,
         0x1C3C38, 0x1C3C38, 0, 0, 0, 0},
        {//n
         0, 0, 0, 0, 0, 0,
         0xFFC0, 0x1FFE0, 0x3FFF0, 0x3FFF0, 0x700F0, 0xF00F0,
         0xF00F0, 0xF00F0, 0xF00F0, 0xF00F0, 0xF00F0, 0xF00F0,
         0xF00F0, 0xF00F0, 0, 0, 0, 0},
        {//o
         0, 0, 0, 0, 0, 0x1FFE0,
         0x3FFE0, 0x7FFF0, 0x7FFF0, 0x70070, 0x70070, 0x70070,
         0x70070, 0x70070, 0x70070, 0x70070, 0x7FFF0, 0x7FFF0,
         0x3FFE0, 0x1FFC0, 0, 0, 0, 0},
        {//p
         0, 0, 0, 0x1FFE0, 0x1FFE0, 0x7FFF8,
         0x1E0078, 0x1E0078, 0x1E0078, 0x1E0078, 0x1E0078, 0x1E0078,
         0x1E0078, 0x1E0078, 0x7FFF8, 0x7FFF8, ‭0x1FFF8‬, 0x78,
         0x78, 0x78, 0x78, 0x78, 0x78, 0},
        {//r
         0, 0, 0, 0, 0, 0,
         0, 0, 0x3F3C0, 0x3F3C0, 0x3FFC0, 0xFC0,
         0xFC0, 0x3C0, 0x3C0, 0x3C0, 0x3C0, 0x3C0,
         0x3C0, 0x3C0, 0, 0, 0, 0},
        {//s
         0, 0, 0, 0, 0, 0x3FFC0,
         0x3FFE0, 0x3FFF8, 0x3FFF8, 0x78, 0x78, 0x3FFF8,
         0x7FFE0, 0xFFFE0, 0xF0000, 0xF0000, 0xFFFE0, 0x7FFE0,
         0x3FFE0, 0x1FFE0, 0, 0, 0, 0},
        {//t
         0, 0, 0, 0x1E00, 0x1E00, 0x1E00,
         0x7FFF0, 0x7FFF0, 0x7FFF0, 0x1E00, 0x1E00, 0x1E00,
         0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x3FE00,
         0x3FE00, 0x3F800, 0, 0, 0, 0},
        {//u
         0, 0, 0, 0, 0, 0,
         0, 0, 0xC0030, 0xE00F0, 0xE00F0, 0xF00F00,
         0xF00F00, 0xF00F00, 0xF80F0, 0xF80F0, 0xFFFF0, 0xEFFF0,
         0xEFFE0, 0xC7FC0, 0, 0, 0, 0},
        {//v
         0, 0, 0, 0, 0, 0,
         0, 0x60060, 0x60060, 0x781E0, 0x381C0, 0x18180,
         0x18180, 0x8100, 0xE700, 0x7E00, 0x3C00, 0x3C00,
         0x1800, 0x1800, 0, 0, 0, 0},
        {//w
         0, 0, 0, 0, 0, 0,
         0, 0, 0x80010, 0x80010, 0xC0030, 0xC0030,
         0xC0030, 0xC1830, 0x63CE0, 0x63CE0, 0x3E7C0, 0x1E780,
         0xC300, 0x8100, 0, 0, 0, 0},
        {//x
         0, 0, 0, 0, 0, 0,
         0, 0, 0x1C0E0, 0x1C0E0, 0x7380, 0x7380,
         0x3F00, 0x3F00, 0x3F00, 0x3F00, 0x7380, 0x7380,
         0x1C0E0, 0x1C0E0, 0, 0, 0, 0},
        {//y
         0, 0, 0, 0, 0, 0,
         0, 0x1C0E0, 0x1C0E0, 0x1C0E0, 0x1C0E0, 0x1C0E0,
         0x1C0E0, 0x1C0E0, 0x1FFC0, 0x1FF80, 0x18000, 0x18000,
         0x18000, 0x18000, 0x1C000, 0x1FF00, 0x7E00, 0}};
    int i, j;
    for (i = 0; i < NUM_CHAR; i++)
    {
        for (j = 0; j < CHAR_PIXEL_SIZE; j++)
        {
            screen->alphabet[i][j] = alphabet[i][j];
        }
    }
    for (i = 0; i < MAX_BUF_SIZE; i++)
    {
        screen->buffer[i].buttonNum = _null;
        screen->buffer[i].overlap = _null;
        screen->buffer[i].focused = false;
        screen->buffer[i].valueExist = false;
    }
}

/* 버튼 입력을 받고 그 정보를 리턴하는 메소드
 * flag 값으로 리턴됨
 */
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

/*
 * Focus되어있는지 유무를 판별하는 메소드
 * 만약 Focus되어있으면 참 반환
 * 아니면 거짓을 반환
 */
bool isFocused(struct Screen *screen, int pointer)
{
    if (screen->buffer[pointer].focused == true)
        return true;
    else
        return false;
}
/*
 * 중복되어 눌려져 있는 것이 있는지 확인하는 메소드
 * EX: (a를 기존에 입력받았을 때 또 a에 대한 버튼을 누른 경우) 참 반환
 * 아니면 거짓을 반환
 */
bool isOverlaped(struct Screen *screen, int pointer)
{

    if (screen->buffer[pointer].overlap != _null)
        return true;
    else
        return false;
}

/* 
 * TODO: 입력받은 문자 처리
 * 버튼 중첩을 어떻게 구현할 것인가? ==>overlap 변수를 두어 해결할 것임
 * 커서의 위치는 어떻게 지정되어야하는가?   ==> Cursor 구조체의 offset을 두어 해결할 것
 * 새 문자를 출력할 때 기존 버튼을 누른건지 새 버튼을 누른건지에 대한 인식처리는 어떻게 해야하는가?
 * Insert할 때 어떻게 처리하여야하는가?
 * LCDPrint 메소드에 정보를 잘 전달해주기 위해 무엇을 하여야 하는가?
*/
int Button_Process_Function(struct Button *buttonInfo, struct Cursor *cursor, struct Screen *screen, int flag)
{

    int i, j, index;
    for (i = 0; i < BTN_SIZE; i++)
    {
        //문자버튼을 입력한 경우
        if (i < 9 && ((flag & (0x0001 << i)) != 0))
        {
            if (screen->buffer[cursor->pointer].valueExist) //값이 이미 존재하는 경우
            {
                if (isFocused(screen, cursor->pointer) && screen->buffer[cursor->pointer].buttonNum == buttonInfo->buttons[i]) //포커싱되어있으면서 동시에 같은 버튼을 누른 경우
                {
                    screen->buffer[cursor->pointer].overlap++;
                }
                else //INSERT하는 공간
                {
                    /*
                     * TODO: INSERT
                     * 그 뒤의 문자들을 뒤로 한 칸씩 넘김, 이 때 버퍼가 FULL인 경우 아무 것도 하지 않음(구현자 마음 ^^)
                     * 
                     */
                }
            }
            else //값 넣어줌
            {
                screen->buffer[cursor->pointer].buttonNum = buttonInfo->buttons[i];
                screen->buffer[cursor->pointer].focused = true;
                screen->buffer[cursor->pointer].overlap = _default;
                screen->buffer[cursor->pointer].valueExist = true;
            }
            break;
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

/* 
 * TODO: 처리된 문자 출력 구현
 * 화면을 넘어가게 되는 즉시 다음 줄로 넘어가게 구현
 * delete의 경우 맨 마지막 문자를 지워야하는데 어떻게 인식시키고 출력해줄 수 있는가?
 * insert의 경우 그 뒤의 문자들을 어떻게 처리해야하는가?
 * 커서를 옮길경우 전의 커서 깜빡임 처리는 어떻게 해야하는가?
 */
void LCDPrint(struct Button *button_info, struct Screen *screen)
{
    return;
}

int main()
{
    char buffer[MAX_BUF_SIZE] = {
        0,
    };
    struct TFT_LCD_Info LCD_info;
    struct Cursor cursor_info;
    struct Button button_info;
    struct Screen screen;
    int dataReady; //버튼 입력 flag 변수

    /* Setting */
    wiringPiSetup();
    Init_Button_Cursor(&button_info, &cursor_info);
    if (Init_TFT(&LCD_info) < 0)
        exit(1);
    Init_Screen(&screen);

    while (1)
    {
        /* TODO: 처리 구현 */
        dataReady = Button_Input(button_info.buttons, &cursor_info, LCD_info);
        Button_Process_Function(&button_info, &cursor_info, &screen, dataReady);
        LCDPrint(&button_info, dataReady);
    }

    /* Free Memory */
    munmap(LCD_info.pfbdata, LCD_info.fbvar.xres * LCD_info.fbvar.yres * 16 / 8);
    close(LCD_info.fbfd);
    return 0;
}
