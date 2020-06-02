#include <stdio.h>
#include <stdlib.h>    /* for exit */
#include <unistd.h>    /* for open/close .. */
#include <fcntl.h>     /* for O_RDWR */
#include <sys/ioctl.h> /* for ioctl */
#include <sys/mman.h>  /* for mmap */
#include <linux/fb.h>  /* for fb_var_screeninfo, FBIOGET_VSCREENINFO */
//#include <wiringPi.h>
#define FBDEVFILE "/dev/fb2"

typedef unsigned char ubyte;

//색 정보를 16bit로 변환해 주는 함수
unsigned short makepixel(ubyte r, ubyte g, ubyte b)
{
    return (unsigned short)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

int main()
{
    int fbfd;
    int ret;
    int vertical, horizontal;
    struct fb_var_screeninfo fbvar;
    unsigned short pixel;
    int offset;
    unsigned short *pfbdata;
    fbfd = open(FBDEVFILE, O_RDWR);
    if (fbfd < 0)
    {
        perror("fbdev open");
        exit(1);
    }

    ret = ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar);
    if (ret < 0)
    {
        perror("fbdev ioctl");
        exit(1);
    }

    if (fbvar.bits_per_pixel != 16)
    {
        fprintf(stderr, "bpp is not 16\n");
        exit(1);
    }
    pfbdata = (unsigned short *)mmap(0, fbvar.xres * fbvar.yres * 16 / 8, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

    if ((unsigned)pfbdata == (unsigned)-1)
    {
        perror("fbdev mmap");
        exit(1);
    }

    /* black pixel*/
    offset = 50 * fbvar.xres + 50;
    pixel = makepixel(0, 0, 0); /* red pixel */
    *(pfbdata + offset) = pixel;

    for (vertical = 0; vertical < fbvar.yres; ++vertical)
    {
        for (horizontal = 0; horizontal < fbvar.xres; ++horizontal)
        {
            offset = vertical * fbvar.xres + horizontal;
            *(pfbdata + offset) = pixel;
        }
    }

    munmap(pfbdata, fbvar.xres * fbvar.yres * 16 / 8);
    close(fbfd);
    return 0;
}
