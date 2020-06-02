#include <stdio.h>
#include <stdlib.h>    /* for exit */
#include <unistd.h>    /* for open/close .. */
#include <fcntl.h>     /* for O_RDWR */
#include <sys/ioctl.h> /* for ioctl */
#include <sys/mman.h>  /* for mmap */
#include <linux/fb.h>  /* for fb_var_screeninfo, FBIOGET_VSCREENINFO */

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
    /* red pixel @ (50,50) */
    offset = 50 * fbvar.xres + 50;
    pixel = makepixel(255, 0, 0); /* red pixel */
    *(pfbdata + offset) = pixel;

    /* green pixel @ (100,50) */
    offset = 50 * fbvar.xres + 100;
    pixel = makepixel(0, 255, 0); /* green pixel */
    *(pfbdata + offset) = pixel;  /* draw pixel */

    /* blue pixel @ (50,100) */
    offset = 100 * fbvar.xres + 50;
    pixel = makepixel(0, 0, 255); /* blue pixel */
    *(pfbdata + offset) = pixel;  /* draw pixel */

    /* white pixel @ (100,100) */
    offset = 100 * fbvar.xres + 100;
    pixel = makepixel(255, 255, 255); /* white pixel */
    *(pfbdata + offset) = pixel;      /* draw pixel */
    munmap(pfbdata, fbvar.xres * fbvar.yres * 16 / 8);
    close(fbfd);
    return 0;
}
