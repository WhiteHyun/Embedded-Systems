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
enum BTN_WiringPi
{
    BTN_DOT_Q_Z = 24, //버튼 .qz 에 대한 wiringPi 넘버링
    BTN_A_B_C = 23,   //버튼 abc 에 대한 wiringPi 넘버링
    BTN_D_E_F = 27,   //버튼 def 에 대한 wiringPi 넘버링
    BTN_G_H_I = 22,   //버튼 ghi 에 대한 wiringPi 넘버링
    BTN_J_K_L = 21,   //버튼 jkl 에 대한 wiringPi 넘버링
    BTN_M_N_O = 26,   //버튼 mno 에 대한 wiringPi 넘버링
    BTN_P_R_S = 7,    //버튼 prs 에 대한 wiringPi 넘버링
    BTN_T_U_V = 6,    //버튼 tuv 에 대한 wiringPi 넘버링
    BTN_W_X_Y = 11,   //버튼 wxy 에 대한 wiringPi 넘버링
    MOVE_LEFT = 1,    //LCD버튼 <= 에 대한 wiringPi 넘버링
    DELETE = 4,       //LCD버튼 DELETE 에 대한 wiringPi 넘버링
    MOVE_RIGHT = 5    //LCD버튼 => 에 대한 wiringPi 넘버링
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
    MAX_BUF_SIZE = 80,                                    //LCD에 적을 수 있는 문자의 최대 수
    BTN_SIZE = 12,                                        //버튼의 개수
    CHAR_PIXEL_SIZE = 24,                                 //문자 픽셀 크기
    NUM_CHAR = 27,                                        //문자의 개수
    LINE_GAP = 8,                                         //라인 간 격차
    CHAR_GAP = 4,                                         //문자 간 격차
    OFFSET_X_ENDLINE = (CHAR_PIXEL_SIZE + CHAR_GAP) * 10, //해당 줄에서의 가장 뒤
    OFFSET_Y_ENDLINE = (CHAR_PIXEL_SIZE + LINE_GAP) * 6,  //LCD에서 가장 밑 칸
    SCREEN_MAX_CHAR = 77                                  //SCREEN에 최대로 표기될 수 있는 문자의 수
};

/* 중첩상태 열거형 지정 */
enum buffer_state
{
    _null = -1,  //값이 존재하지 않음
    _default = 0 //초기 값
};

/* 커서의 상태를 나타내는 열거형 지정 */
enum cursor_move_state
{
    LEFT = true,
    RIGHT = false
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
    time_t originalTime;     //커서 플리킹 사이클 체크변수
    bool isFlickering;       //커서 플리킹 체크변수
    unsigned short offset_x; //커서 x의 위치를 나타내는 변수
    unsigned short offset_y; //커서 y의 위치를 나타내는 변수
    int pointer;             //버퍼 배열에서 커서위치를 가리키는 변수
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
    bool isChanged;  //버퍼의 값이 수정되었는지를 나타내주는 변수
};
/* LCD에 보여줄 정보를 가지고 있는 구조체 */
struct Screen
{
    int alphabet[NUM_CHAR][CHAR_PIXEL_SIZE]; //알파벳 구성 값('a~z', '.')
    struct Buffer buffer[MAX_BUF_SIZE];      //80자의 문자를 구성할 버퍼의 정보
};

/*
 * 화면 초기화 메소드, cursor의 위치를 가지고 draw함
 * --First parameter(flag)--
 * true: 전체 화면 클리어
 * false: cursor가 가리키는 화면 클리어
 * --second parameter (alphabet)--
 * null: 위치된 cursor쪽에 문자를 그림
 * not null: 알파벳 정보를 가지고 알파벳을 그림
 * Third parameter: LCD의 정보를 담은 객체 포인터
 * Fourth parameter: 커서의 정보를 담은 객체 포인터
 */
void SetScreen(bool flag, int *alphabet, struct TFT_LCD_Info *LCD_info, struct Cursor *cursor)
{
    unsigned short pixel = BLACK;
    int vertical, horizontal, offset;
    int alphabet_flag = 0x0001;
    int i = 0;
    if (flag) //전체 스크린 클리어
    {
        for (vertical = 0; vertical < LCD_info->fbvar.yres; vertical++)
        {
            for (horizontal = 0; horizontal < LCD_info->fbvar.xres; horizontal++)
            {
                offset = vertical * LCD_info->fbvar.xres + horizontal;
                *(LCD_info->pfbdata + offset) = pixel;
            }
        }
    }
    else //지정된 픽셀
    {
        if (alphabet == 0) //클리어
        {
            for (vertical = cursor->offset_y; vertical < cursor->offset_y + CHAR_PIXEL_SIZE; vertical++)
            {
                for (horizontal = cursor->offset_x; horizontal < cursor->offset_x + CHAR_PIXEL_SIZE; horizontal++)
                {
                    offset = vertical * LCD_info->fbvar.xres + horizontal;
                    *(LCD_info->pfbdata + offset) = pixel;
                }
            }
        }
        else //알파벳 그림
        {
            for (vertical = cursor->offset_y; vertical < cursor->offset_y + CHAR_PIXEL_SIZE; vertical++)
            {
                for (horizontal = cursor->offset_x; horizontal < cursor->offset_x + CHAR_PIXEL_SIZE; horizontal++)
                {
                    offset = vertical * LCD_info->fbvar.xres + horizontal;
                    if ((alphabet[i] & alphabet_flag) != 0)
                        *(LCD_info->pfbdata + offset) = (unsigned short)WHITE;
                    else
                        *(LCD_info->pfbdata + offset) = pixel;
                    alphabet_flag <<= 1;
                }
                i++;
                alphabet_flag = 1;
            }
        }
    }
}
/*
 * 커서를 깜빡이게 만들어주는 메소드
 * 1초마다 한 번씩 깜빡이게 만들어 둚
 * First parameter: 커서의 정보를 담은 객체 포인터
 * Second parameter: LCD의 정보를 담은 객체
 * Third parameter: 스크린의 정보를 담은 객체
 */
void Cursor_Blink(struct Cursor *cursor, struct TFT_LCD_Info LCD_info, struct Screen screen)
{
    int draw_offset; //임시 LCD 커서 출력 오프셋
    int i;
    time_t currentTime = time(NULL);
    if (currentTime - cursor->originalTime < 1) //1초 이상 지나지 않은 경우 단순리턴
        return;
    else //1초 이상 지나면 커서 깜빡임
    {
        cursor->originalTime = currentTime;
        if (cursor->isFlickering == false) //깜빡이고있지 않을 때
        {
            cursor->isFlickering = true;                //Blink!
            if (screen.buffer[cursor->pointer].focused) //Focusing Cursor Draw
            {
                for (i = 0; i < 24; i++)
                {
                    draw_offset = (cursor->offset_y + 23) * LCD_info.fbvar.xres + cursor->offset_x + i;
                    *(LCD_info.pfbdata + draw_offset) = WHITE; /* draw pixel */
                }
            }
            else //Unfocusing Cursor Draw
            {
                for (i = 0; i < 24; i++)
                {
                    draw_offset = (cursor->offset_y + i) * LCD_info.fbvar.xres + cursor->offset_x;
                    *(LCD_info.pfbdata + draw_offset) = WHITE; /* draw pixel */
                }
            }
        }
        else //깜빡이고 있을 때
        {
            cursor->isFlickering = false; //Cursor off!
            for (i = 0; i < 24; i++)
            {
                //Erase focused Cursor and unfocused cursor
                draw_offset = (cursor->offset_y + 23) * LCD_info.fbvar.xres + cursor->offset_x + i;
                *(LCD_info.pfbdata + draw_offset) = BLACK; /* draw pixel */
                draw_offset = (cursor->offset_y + i) * LCD_info.fbvar.xres + cursor->offset_x;
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
    cursor->isFlickering = false;
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
         0x1E0078, 0x1E0078, 0x7FFF8, 0x7FFF8, 0x1FFF8, 0x78,
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
         0, 0, 0xC0030, 0xE00F0, 0xE00F0, 0xF00F0,
         0xF00F0, 0xF00F0, 0xF80F0, 0xF80F0, 0xFFFF0, 0xEFFF0,
         0xEFFE0, 0xC7FC0, 0, 0, 0, 0},
        {//v
         0, 0, 0, 0, 0, 0,
         0, 0x60060, 0x700E0, 0x781E0, 0x381C0, 0x18180,
         0x1C380, 0xC300, 0xE700, 0x7E00, 0x3C00, 0x3C00,
         0x1800, 0x1800, 0, 0, 0, 0},
        {//w
         0, 0, 0, 0, 0, 0,
         0, 0x100008, 0x180018, 0x180018, 0x1C0038, 0x1C0038,
         0xC1838, 0xE3C70, 0x73CE0, 0x7FFE0, 0x3E7C0, 0x1E780,
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
            screen->alphabet[i][j] = alphabet[i][j]; //스크린 알파벳에 값 할당
        }
    }
    /* 스크린 구조체 내 버퍼 초기화 */
    for (i = 0; i < MAX_BUF_SIZE; i++)
    {
        screen->buffer[i].buttonNum = _null;
        screen->buffer[i].overlap = _null;
        screen->buffer[i].focused = false;
        screen->buffer[i].valueExist = false;
        screen->buffer[i].isChanged = false;
    }
}

/* 버튼 입력을 받고 그 정보를 리턴하는 메소드
 * flag 값으로 리턴됨
 * First parameter: 버튼의 정보들
 * Second parameter: 커서의 정보를 담은 객체 포인터
 * Third parameter: LCD의 정보를 담은 객체
 * Fourth parameter: 스크린의 정보를 담은 객체
 */
int Button_Input(int *buttons, struct Cursor *cursor_info, struct TFT_LCD_Info LCD_info, struct Screen screen)
{
    int inputBtnFlag = 0x0000;
    int i, state = 0; //state: 입력 유무 상태를 저장할 변수
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
        Cursor_Blink(cursor_info, LCD_info, screen);
    }
    return inputBtnFlag;
}

/*
 * Focus되어있는지 유무를 판별하는 메소드
 * 만약 Focus되어있으면 참 반환
 * 아니면 거짓을 반환
 * First parameter: 스크린의 정보를 담은 객체 포인터
 * Second parameter: 커서를 가리키고있는 포인터
 */
bool isFocused(struct Screen *screen, int pointer)
{
    if (screen->buffer[pointer].focused == true)
        return true;
    else
        return false;
}

/*
 * 커서 동작제어 메소드
 * --First parameter(flag)--             
 * true: 왼쪽이동        
 * false:오른쪽이동      
 * Second parameter: 커서의 정보를 담은 객체 포인터
 * Third parameter: 스크린의 정보를 담은 객체 포인터
 * --Fourth parameter(isPrintCursor)--
 * true: LCD출력전용커서
 * false: 실제 값변경커서
 */
void CursorMoved(bool flag, struct Cursor *cursor, struct Screen *screen, bool isPrintCursor)
{
    if (flag == LEFT) //왼쪽이동
    {
        //문자가 focusing 되어있을 때 (LCD출력 커서는 해당하지 않음)
        if (isFocused(screen, cursor->pointer) && !isPrintCursor)
        {
            screen->buffer[cursor->pointer].focused = false;
        }
        //커서쪽의 문자가 focusing 되어있지 않을 때
        else if (cursor->pointer == 0) //커서가 이미 처음 부분일 경우
        {
            /* Do nothing */
        }
        else //커서 이동
        {
            cursor->pointer--;
            if (cursor->offset_x == 0) //커서가 앞부분인 경우
            {
                //y축을 한칸 낮추고 x축 끝에 커서를 둚
                cursor->offset_y -= (CHAR_PIXEL_SIZE + LINE_GAP);
                cursor->offset_x = OFFSET_X_ENDLINE;
            }
            else //중간부분인 경우
            {    //왼쪽으로 이동
                cursor->offset_x -= (CHAR_PIXEL_SIZE + CHAR_GAP);
            }
        }
    }
    else //오른쪽 이동
    {
        //문자가 focusing 되어있을 때
        if (isFocused(screen, cursor->pointer) && !isPrintCursor)
        {
            screen->buffer[cursor->pointer].focused = false;
        }
        if (cursor->pointer == MAX_BUF_SIZE - 1)
        {
            /* Do nothing */
        }
        else //커서 이동
        {
            /*
             * 현재 가리키고 있는 커서가 null값인 경우 스페이싱 처리 및 커서 다음 위치로 이동시킴
             * 현재 가리키고 있는 커서가 null이 아닐 때,그냥 오른쪽 이동
             */
            //띄어쓰기인 경우(현재 위치에 값이 존재하지 않는 경우)
            if (screen->buffer[cursor->pointer].valueExist == false && !isPrintCursor)
            {
                screen->buffer[cursor->pointer].valueExist = true;
            }

            /* 커서값 변경 */
            cursor->pointer++;
            if (cursor->offset_x == OFFSET_X_ENDLINE) // 커서가 스크린 가장 뒤쪽 위치에 가있는 경우
            {
                //다음 줄 첫번째 위치로 이동
                cursor->offset_y += (CHAR_PIXEL_SIZE + LINE_GAP);
                cursor->offset_x = 0;
            }
            else //커서가 중간에 위치되어있는 경우
            {
                cursor->offset_x += (CHAR_PIXEL_SIZE + CHAR_GAP);
            }
        }
    }
}

/* 
 * 입력받은 버튼을 처리하는 메소드
 * First parameter: 버튼 정보를 담은 객체 포인터
 * Second parameter: 커서 정보를 담은 객체 포인터
 * Third parameter: LCD-Screen 정보를 담은 객체 포인터
 * Fourth parameter: 입력받은 버튼의 정보
 */
int Button_Process_Function(struct Button *buttonInfo, struct Cursor *cursor, struct Screen *screen, int flag)
{
    int i, j;
    for (i = 0; i < BTN_SIZE; i++)
    {
        //문자버튼을 입력한 경우
        if (i < 9 && ((flag & (0x0001 << i)) != 0))
        {
            if (screen->buffer[cursor->pointer].valueExist) //값이 이미 존재하는 경우
            {
                if (isFocused(screen, cursor->pointer) && screen->buffer[cursor->pointer].buttonNum == i) //포커싱되어있으면서 동시에 같은 버튼을 누른 경우
                {
                    screen->buffer[cursor->pointer].overlap = (screen->buffer[cursor->pointer].overlap + 1) % 3;
                    screen->buffer[cursor->pointer].isChanged = true;
                }
                else //INSERT하는 공간
                {
                    //버퍼 공간의 여유가 있는 경우
                    if (screen->buffer[MAX_BUF_SIZE - 1].valueExist == false)
                    {
                        for (j = MAX_BUF_SIZE - 2; j >= cursor->pointer; j--) //값을 뒤로 당기는 반복문
                        {
                            if (screen->buffer[j].valueExist == false)
                                continue;
                            else
                            {
                                screen->buffer[j + 1].buttonNum = screen->buffer[j].buttonNum;
                                screen->buffer[j + 1].overlap = screen->buffer[j].overlap;
                                screen->buffer[j + 1].valueExist = screen->buffer[j].valueExist;
                                screen->buffer[j + 1].isChanged = true;
                            }
                        }
                        if (isFocused(screen, cursor->pointer))
                        {
                            screen->buffer[cursor->pointer].focused = false; //포커싱 풀어줌
                            //커서 다음 공간에 값 할당
                            screen->buffer[cursor->pointer + 1].buttonNum = i;
                            screen->buffer[cursor->pointer + 1].focused = true;
                            screen->buffer[cursor->pointer + 1].overlap = _default;
                            screen->buffer[cursor->pointer + 1].valueExist = true;
                            screen->buffer[cursor->pointer + 1].isChanged = true;
                            CursorMoved(RIGHT, cursor, screen, false); //값 입력했으므로 커서도 한 칸 이동해야함
                        }
                        else
                        {
                            screen->buffer[cursor->pointer].buttonNum = i;
                            screen->buffer[cursor->pointer].focused = true;
                            screen->buffer[cursor->pointer].overlap = _default;
                            screen->buffer[cursor->pointer].valueExist = true;
                            screen->buffer[cursor->pointer].isChanged = true;
                        }
                    }
                    else //버퍼 공간의 여유가 없는 경우
                    {
                        screen->buffer[cursor->pointer].focused = false;
                        /* Do nothing */
                    }
                }
            }
            else //값이 뒤에도 존재하지 않으므로 해당 커서쪽에 값 할당
            {
                screen->buffer[cursor->pointer].buttonNum = i;
                screen->buffer[cursor->pointer].focused = true;
                screen->buffer[cursor->pointer].overlap = _default;
                screen->buffer[cursor->pointer].valueExist = true;
                screen->buffer[cursor->pointer].isChanged = true;
            }
            break;
        }
        //커서왼쪽이동버튼을 누른 경우
        else if (i == 9 && ((flag & (0x0001 << i)) != 0))
        {
            CursorMoved(LEFT, cursor, screen, false);
            break;
        }
        //커서오른쪽이동버튼을 누른 경우
        else if (i == 11 && ((flag & (0x0001 << i)) != 0))
        {
            CursorMoved(RIGHT, cursor, screen, false);
            break;
        }
        //DELETE버튼을 누른 경우
        else if (i == 10 && ((flag & (0x0001 << i)) != 0))
        {
            screen->buffer[cursor->pointer].focused = false;
            j = cursor->pointer;
            while (j < (MAX_BUF_SIZE - 1) && screen->buffer[j + 1].valueExist != false)
            {
                screen->buffer[j].buttonNum = screen->buffer[j + 1].buttonNum;
                screen->buffer[j].overlap = screen->buffer[j + 1].overlap;
                screen->buffer[j].valueExist = screen->buffer[j + 1].valueExist;
                screen->buffer[j].isChanged = true;
                j++;
            }
            screen->buffer[j].buttonNum = _null;
            screen->buffer[j].overlap = _null;
            screen->buffer[j].focused = false;
            screen->buffer[j].valueExist = false;
            screen->buffer[j].isChanged = true;
            break;
        }
    }
}

/* 
 * 처리된 문자를 스크린 상에 출력해주는 메소드
 * First parameter: 버튼의 정보를 담은 객체 포인터
 * Second parameter: 커서의 정보를 담은 객체 포인터
 * Third parameter: LCD 정보를 담은 객체 포인터
 * Fourth parameter: 스크린 정보를 담은 객체 포인터
 */
void LCDPrint(struct Button *button_info, struct Cursor *cursor, struct TFT_LCD_Info *LCD_info, struct Screen *screen)
{
    struct Cursor drawCursor;
    int num_button;
    int num_overlap;
    int i;
    drawCursor.offset_x = cursor->offset_x;
    drawCursor.offset_y = cursor->offset_y;
    drawCursor.pointer = cursor->pointer;
    //스크린을 벗어난 이동의 경우(DOWN)
    if (drawCursor.pointer == SCREEN_MAX_CHAR - 1 && screen->buffer[drawCursor.pointer + 1].valueExist == false && drawCursor.offset_x == OFFSET_X_ENDLINE && drawCursor.offset_y == OFFSET_Y_ENDLINE)
    {
        cursor->offset_x = OFFSET_X_ENDLINE;
        cursor->offset_y -= (CHAR_PIXEL_SIZE + LINE_GAP);
        drawCursor.offset_x = 0;
        drawCursor.offset_y = 0;
        for (i = 0; i < SCREEN_MAX_CHAR; i++)
        {
            drawCursor.pointer = 11 + i;
            screen->buffer[drawCursor.pointer].isChanged = false;
            if (drawCursor.pointer >= MAX_BUF_SIZE) //이미 버퍼 배열을 넘은 경우 나머지 공간은 null처리 해줌
            {
                SetScreen(false, NULL, LCD_info, &drawCursor);
            }
            else
            {
                //띄어쓰기나 아무 값도 없을 때
                if (screen->buffer[drawCursor.pointer].buttonNum == _null && screen->buffer[drawCursor.pointer].overlap == _null)
                {
                    SetScreen(false, NULL, LCD_info, &drawCursor);
                }
                //문자 값일 때
                else
                {
                    num_button = screen->buffer[drawCursor.pointer].buttonNum * 3;
                    num_overlap = screen->buffer[drawCursor.pointer].overlap;
                    SetScreen(false, screen->alphabet[num_button + num_overlap], LCD_info, &drawCursor);
                }
            }
            CursorMoved(RIGHT, &drawCursor, screen, true);
        }
    }
    //스크린을 벗어난 이동의 경우(UP)
    else if (drawCursor.offset_x == 0 && drawCursor.offset_y == 0 && drawCursor.pointer != 0)
    {
        cursor->offset_x = 0;
        cursor->offset_y += (CHAR_PIXEL_SIZE + LINE_GAP);
        drawCursor.offset_x = 0;
        drawCursor.offset_y = 0;
        for (i = 0; i < SCREEN_MAX_CHAR; i++)
        {
            screen->buffer[drawCursor.pointer].isChanged = false;
            drawCursor.pointer = i;
            //띄어쓰기나 아무 값도 없을 때
            if (screen->buffer[drawCursor.pointer].buttonNum == _null && screen->buffer[drawCursor.pointer].overlap == _null)
            {
                SetScreen(false, NULL, LCD_info, &drawCursor);
            }
            //문자 값일 때
            else
            {
                num_button = screen->buffer[drawCursor.pointer].buttonNum * 3;
                num_overlap = screen->buffer[drawCursor.pointer].overlap;
                SetScreen(false, screen->alphabet[num_button + num_overlap], LCD_info, &drawCursor);
            }
            CursorMoved(RIGHT, &drawCursor, screen, true);
        }
    }
    else
    {
        /* 커서 위치 양 옆을 다시 redraw(이전 커서 깜빡임 지움) */
        if (drawCursor.pointer > 0)
        {
            CursorMoved(LEFT, &drawCursor, screen, true);
            if (screen->buffer[drawCursor.pointer].buttonNum != _null)
            {
                num_button = screen->buffer[drawCursor.pointer].buttonNum * 3;
                num_overlap = screen->buffer[drawCursor.pointer].overlap;
                SetScreen(false, screen->alphabet[num_button + num_overlap], LCD_info, &drawCursor);
            }
            else
            {
                SetScreen(false, NULL, LCD_info, &drawCursor);
            }
            CursorMoved(RIGHT, &drawCursor, screen, true);
        }
        if (drawCursor.pointer < MAX_BUF_SIZE - 1)
        {
            CursorMoved(RIGHT, &drawCursor, screen, true);
            if (screen->buffer[drawCursor.pointer].buttonNum != _null)
            {
                num_button = screen->buffer[drawCursor.pointer].buttonNum * 3;
                num_overlap = screen->buffer[drawCursor.pointer].overlap;
                SetScreen(false, screen->alphabet[num_button + num_overlap], LCD_info, &drawCursor);
            }
            else
            {
                SetScreen(false, NULL, LCD_info, &drawCursor);
            }
            CursorMoved(LEFT, &drawCursor, screen, true);
        }

        /* 바뀐 값들 Redraw */
        while (screen->buffer[drawCursor.pointer].isChanged && drawCursor.offset_x <= OFFSET_X_ENDLINE && drawCursor.offset_y <= OFFSET_Y_ENDLINE)
        {
            screen->buffer[drawCursor.pointer].isChanged = false;

            //띄어쓰기나 null상태 일 경우
            if (screen->buffer[drawCursor.pointer].buttonNum == _null && screen->buffer[drawCursor.pointer].overlap == _null)
            {
                SetScreen(false, NULL, LCD_info, &drawCursor);
            }
            else
            {
                /* 버튼, 중첩값 변수에 할당 */
                num_button = screen->buffer[drawCursor.pointer].buttonNum * 3;
                num_overlap = screen->buffer[drawCursor.pointer].overlap;
                /*Draw Alphabet*/
                SetScreen(false, screen->alphabet[num_button + num_overlap], LCD_info, &drawCursor);
            }
            CursorMoved(RIGHT, &drawCursor, screen, true);
        }
    }
}

int main()
{
    struct TFT_LCD_Info LCD_info;
    struct Cursor cursor_info;
    struct Button button_info;
    struct Screen screen;
    int dataReady; //버튼 입력 flag 변수

    /* Setting Phase */
    wiringPiSetup();
    Init_Button_Cursor(&button_info, &cursor_info);
    if (Init_TFT(&LCD_info) < 0)
        exit(1);
    Init_Screen(&screen);
    SetScreen(true, NULL, &LCD_info, &cursor_info); //화면 클리어
    printf("Init Successfully!");
    /* Process Phase */
    while (true)
    {
        dataReady = Button_Input(button_info.buttons, &cursor_info, LCD_info, screen); //버튼 입력에 대한 정보를 가져옴
        Button_Process_Function(&button_info, &cursor_info, &screen, dataReady);       //입력받은 버튼을 가지고 처리 수행
        LCDPrint(&button_info, &cursor_info, &LCD_info, &screen);                      //처리된 값을 가지고 LCD에 출력
    }
    SetScreen(true, NULL, &LCD_info, &cursor_info); //화면 클리어
    /* Free Memory Phase */
    munmap(LCD_info.pfbdata, LCD_info.fbvar.xres * LCD_info.fbvar.yres * 16 / 8);
    close(LCD_info.fbfd);
    return 0;
}
