#include <stdio.h>
#include <fcntl.h>
#include "../common/common.h"

#define COLOR_BACKGROUND FB_COLOR(0x0, 0x0, 0x0)
#define COLOR_BAR FB_COLOR(0xff, 0xff, 0xff)
#define COLOR_TEXT FB_COLOR(0xff, 0xff, 0xff)
#define COLOR_GREY FB_COLOR(0x54, 0x54, 0x54)
#define BAR_H 60
#define ICON_SIZE 40
#define MARGIN 10
#define PLUS_X 10
#define MINUS_X 60
#define RESET_X 110
#define EXIT_X 670
#define IN_SQUARE(x, y, sx, sy, size) ((x >= sx) && (x < sx + size) && (y >= sy) && (y < sy + size))

enum image_type
{
    png,
    jpg,
    insupport
};

static int touch_fd;
static fb_image *minus_img;
static fb_image *plus_img;
static fb_image *reset_img;
static fb_image *exit_img;
static fb_image *show_img;
static fb_image *src_img;
static enum image_type img_type;
static char *filepath;
static int loc_x, loc_y;           // 图片定位
static int old_x, old_y, drag = 0; // old_x old_y - 坐标旧值, drag - 是否拖动标记
static int scale_level = 0; // 缩放尺度，负数代表缩小，正数代表放大
void draw_ui() // draw clear button, background and refresh screen
{
    fb_draw_rect(0, 0, SCREEN_WIDTH, BAR_H, COLOR_BAR);
    fb_draw_image(PLUS_X, MARGIN, plus_img, 0);
    fb_draw_image(MINUS_X, MARGIN, minus_img, 0);
    fb_draw_image(RESET_X, MARGIN, reset_img, 0);
    fb_draw_image(EXIT_X, MARGIN, exit_img, 0);
}
enum image_type get_image_type(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        return insupport;
    }
    char png_type[8] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    char file_head[8];
    read(fd, file_head, 8);
    switch (file_head[0])
    {
    case 0xff:
        if (file_head[1] == 0xd8)
            return jpg; // jpeg
    case 0x89:
        if (file_head[1] == png_type[1] && file_head[2] == png_type[2] && file_head[3] == png_type[3] && file_head[4] == png_type[4] &&
            file_head[5] == png_type[5] && file_head[6] == png_type[6] && file_head[7] == png_type[7])
            return png; // png
    default:
        return insupport;
    }
}
void clear_draw()
{
    fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);
}
fb_image *fb_read_image(char *file, enum image_type type)
{
    switch (type)
    {
    case jpg:
        return fb_read_jpeg_image(file);
    case png:
        return fb_read_png_image(file);
    default:
        return NULL;
    }
}
static void touch_event_cb(int fd)
{
    int type, x, y, finger;
    type = touch_read(fd, &x, &y, &finger);
    if (finger != 0)
    {
        return;
    }
    fb_image *new_img = NULL;
    switch (type)
    {
    case TOUCH_PRESS:
        if (IN_SQUARE(x, y, PLUS_X, MARGIN, ICON_SIZE)) // 放大
        {
            if ((new_img = zoom_image(src_img, ++scale_level)) != NULL)
            {
                fb_free_image(show_img);
                show_img = new_img;
                clear_draw();
                fb_draw_image(loc_x, loc_y, show_img, 0);
                draw_ui();
            }
        }
        else if (IN_SQUARE(x, y, MINUS_X, MARGIN, ICON_SIZE)) // 缩小
        {
            if ((new_img = zoom_image(src_img, --scale_level)) != NULL)
            {
                fb_free_image(show_img);
                show_img = new_img;
                clear_draw();
                fb_draw_image(loc_x, loc_y, show_img, 0);
                draw_ui();
            }
        }
        else if (IN_SQUARE(x, y, RESET_X, MARGIN, ICON_SIZE)) // 重置图片大小
        {
            loc_x = 0;
            loc_y = BAR_H;
            fb_free_image(show_img);
            show_img = fb_copy_image(src_img);
            clear_draw();
            fb_draw_image(loc_x, loc_y, show_img, 0);
            draw_ui();
        }
        else if (IN_SQUARE(x, y, EXIT_X, MARGIN, ICON_SIZE)) // 离开
        {
            clear_draw();
            fb_update();
            fb_free_image(plus_img);
            fb_free_image(minus_img);
            fb_free_image(reset_img);
            fb_free_image(exit_img);
            fb_free_image(show_img);
            exit(0);
        }
        else if (y > BAR_H) // 开始拖动
        {
            drag = 1;
        }
        break;
    case TOUCH_MOVE:
        if (drag == 1)
        {
            int dx = x - old_x, dy = y - old_y;
            loc_x = (dx == 0) ? loc_x : (loc_x + dx);
            loc_y = (dy == 0) ? loc_y : (loc_y + dy);
            clear_draw();
            fb_draw_image(loc_x, loc_y, show_img, 0);
            draw_ui();
        }
        break;
    case TOUCH_RELEASE:
        drag = 0;
        break;
    case TOUCH_ERROR:
        printf("close touch fd\n");
        close(fd);
        task_delete_file(fd);
        break;
    default:
        return;
    }
    old_x = x;
    old_y = y;
    fb_update();
    return;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "no input image\n");
        return 0;
    }
    filepath = argv[1];
    if ((img_type = get_image_type(filepath)) == insupport)
    {
        fprintf(stderr, "unsupported image type: ");
        fprintf(stderr, filepath);
        fprintf(stderr, "\n");
        exit(1);
    }
    if ((src_img = fb_read_image(filepath, img_type)) == NULL)
    {
        fprintf(stderr, "cannot find image: ");
        fprintf(stderr, filepath);
        fprintf(stderr, "\n");
        exit(1);
    }
    show_img = fb_copy_image(src_img);
    fb_init("/dev/fb0");
    font_init("/home/pi/font.ttc");
    plus_img = fb_read_png_image("/home/pi/plus40.png");
    minus_img = fb_read_png_image("/home/pi/minus40.png");
    reset_img = fb_read_png_image("/home/pi/reset40.png");
    exit_img = fb_read_png_image("/home/pi/exit40.png");
    loc_x = 0;
    loc_y = BAR_H;
    clear_draw();
    fb_draw_image(loc_x, loc_y, show_img, 0);
    draw_ui();
    fb_update();

    //打开多点触摸设备文件, 返回文件fd
    touch_fd = touch_init("/dev/input/event0");
    //添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
    task_add_file(touch_fd, touch_event_cb);

    task_loop(); //进入任务循环
    fb_free_image(plus_img);
    fb_free_image(minus_img);
    fb_free_image(reset_img);
    fb_free_image(exit_img);
    fb_free_image(show_img);
    return 0;
}
