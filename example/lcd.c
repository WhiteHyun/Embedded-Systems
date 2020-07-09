#include <stdio.h>
#include <stdlib.h>    /*for exit*/
#include <unistd.h>    /*for open/close..*/
#include <fcntl.h>     /*for O_RDWR*/
#include <sys/ioctl.h> /*for ioctl*/
#include <linux/fb.h>  /*for fb_var_screeninfo, FBIOGET_VSCREENINFO*/
#define FBDEVFILE "/dev/fb2"
int main()
{
    int fbfd;
    int ret;
    struct fb_var_screeninfo fbvar;
    struct fb_fix_screeninfo fbfix;

    fbfd = open(FBDEVFILE, O_RDWR); //frame buffer의 node인 /dev/fb를 open
    if (fbfd < 0)
    {
        perror("fbdevopen");
        exit(1);
    }
    ret = ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar);
    if (ret < 0)
    {
        perror("fbdevioctl(FSCREENINFO)");
        exit(1);
    }
    ret = ioctl(fbfd, FBIOGET_FSCREENINFO, &fbfix);
    if (ret < 0)
    {
        perror("fbdevioctl(FSCREENINFO)");
        exit(1);
    }
    // x-resolution
    printf("x-resolution: %d\n", fbvar.xres);
    //y-resolution
    printf("y-resolution: %d\n", fbvar.yres);
    //virtual x-resolution
    printf("x-resolution(virtual): %d\n", fbvar.xres_virtual);
    //virtual y-resolution
    printf("y-resolution(virtual): %d\n", fbvar.yres_virtual);
    //bpp (bits per pixel)
    printf("bpp: %d\n", fbvar.bits_per_pixel);
    //the size of frame buffer memory
    printf("lengthof frame buffer memory: %d\n", fbfix.smem_len);
    close(fbfd);
    return 0;
}
