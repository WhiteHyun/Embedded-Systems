#include <stdio.h>
#include <stdlib.h>    /* for exit */
#include <unistd.h>    /* for open/close .. */
#include <fcntl.h>     /* for O_RDONLY */
#include <sys/ioctl.h> /* for ioctl */
#include <linux/fb.h>  /* for fb_var_screeninfo, FBIOGET_VSCREENINFO */

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
        perror("fbdev ioctl(GET)");
        exit(1);
    }
    fbvar.bits_per_pixel = 16;

    ret = ioctl(fbfd, FBIOPUT_VSCREENINFO, &fbvar);
    if (ret < 0)
    {
        perror("fbdev ioctl(PUT)");
        exit(1);
    }

    close(fbfd);
    return 0;
}
