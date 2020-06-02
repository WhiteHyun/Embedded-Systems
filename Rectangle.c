#include <stdio.h>
#include <stdlib.h>    /* for exit */
#include <unistd.h>    /* for open/close .. */
#include <fcntl.h>     /* for O_RDONLY */
#include <sys/ioctl.h> /* for ioctl */
#include <sys/types.h>
#include <linux/fb.h> /* for fb_var_screeninfo, FBIOGET_VSCREENINFO */

#define FBDEVFILE "/dev/fb2"

int main()
{
    int fbfd;
    int ret;
    struct fb_var_screeninfo fbvar;

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
    while (1)
    {
        int xpos1, ypos1;
        int xpos2, ypos2;
        int offset;
        int rpixel;
        int t, tt;

        /* random number between 0 and xres-1 */
        xpos1 = (int)((fbvar.xres * 1.0 * rand()) / (RAND_MAX + 1.0));
        xpos2 = (int)((fbvar.xres * 1.0 * rand()) / (RAND_MAX + 1.0));

        /* random number between 0 and yres */
        ypos1 = (int)((fbvar.yres * 1.0 * rand()) / (RAND_MAX + 1.0));
        ypos2 = (int)((fbvar.yres * 1.0 * rand()) / (RAND_MAX + 1.0));

        if (xpos1 > xpos2)
        {
            t = xpos1;
            xpos1 = xpos2;
            xpos2 = t;
        }

        if (ypos1 > ypos2)
        {
            t = ypos1;
            ypos1 = ypos2;
            ypos2 = t;
        }
        /* random number between 0 and 65535(2^16-1) */
        rpixel = (int)(65536.0 * rand() / (RAND_MAX + 1.0));

        for (t = ypos1; t <= ypos2; t++)
        {
            offset = t * fbvar.xres * (16 / 8) + xpos1 * (16 / 8);

            if (lseek(fbfd, offset, SEEK_SET) < 0)
            {
                perror("fbdev lseek");
                exit(1);
            }
            for (tt = xpos1; tt <= xpos2; tt++)
                write(fbfd, &rpixel, 2);
        }
    }

    return 0;
}
