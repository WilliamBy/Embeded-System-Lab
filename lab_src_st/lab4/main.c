#include <stdio.h>
#include "../common/common.h"

#define COLOR_BACKGROUND FB_COLOR(0x0, 0x0, 0x0)
#define COLOR_BAR FB_COLOR(0xff, 0xff, 0xff)
#define COLOR_TEXT FB_COLOR(0xff, 0xff, 0xff)
#define BAR_H 60
#define ERASER_X 10
#define ERASER_Y 10
#define ERASER_W 40
#define ERASER_H 40
#define CROSS_X (SCREEN_WIDTH - 40)
#define CROSS_Y 10
#define CROSS_W 40
#define CROSS_H 40
static int color_finger[] = {FB_COLOR(0xff, 0x00, 0x04), FB_COLOR(0xae, 0x00, 0xff),
							 FB_COLOR(0xff, 0xe1, 0x00), FB_COLOR(0x26, 0xff, 0x00), FB_COLOR(0x00, 0xff, 0xd5)};
static int touch_fd;
static fb_image *eraser_img;
static fb_image *cross_img;
static int move_num = 3;
static int old_x, old_y;
void draw_ui() // draw clear button, background and refresh screen
{
	fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);
	fb_draw_rect(0, 0, SCREEN_WIDTH, BAR_H, COLOR_BAR);
	fb_draw_image(ERASER_X, ERASER_Y, eraser_img, 0);
	fb_draw_image(CROSS_X, CROSS_Y, cross_img, 0);
}
static void touch_event_cb(int fd)
{
	int type, x, y, finger;
	type = touch_read(fd, &x, &y, &finger);
	switch (type)
	{
	case TOUCH_PRESS:
		// printf("TOUCH_PRESS：x=%d,y=%d,finger=%d\n",x,y,finger);
		if ((x >= ERASER_X) && (x < ERASER_X + ERASER_W) && (y >= ERASER_Y) && (y < ERASER_Y + ERASER_H))
		{
			draw_ui();
		}
		else if ((x >= CROSS_X) && (x < CROSS_X + CROSS_W) && (y >= CROSS_Y) && (y < CROSS_Y + CROSS_H))
		{
			fb_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BACKGROUND);
			fb_draw_text(20, 60, "Drawboard closed.", 40, COLOR_BAR);
			fb_update();
			fb_free_image(cross_img);
			fb_free_image(eraser_img);
			exit(0);
		}
		else
		{
			fb_draw_round(x, y, 8, color_finger[finger]);
		}
		break;
	case TOUCH_MOVE:
		// if (old_x == x && old_y == y)	// 避免重复渲染
		// {
		// 	break;
		// }
		// else
		// {
		// 	old_x = x;
		// 	old_y = y;
		// }
		// printf("TOUCH_MOVE：x=%d,y=%d,finger=%d\n", x, y, finger);
		if (y > BAR_H)
		{
			fb_draw_round(x, y, 8, color_finger[finger]);
			// fb_draw_pixel(x, y, color_finger[finger]);
		}
		break;
	case TOUCH_RELEASE:
		// printf("TOUCH_RELEASE：x=%d,y=%d,finger=%d\n",x,y,finger);
		break;
	case TOUCH_ERROR:
		printf("close touch fd\n");
		close(fd);
		task_delete_file(fd);
		break;
	default:
		return;
	}
	fb_update();
	return;
}

int main(int argc, char *argv[])
{
	fb_init("/dev/fb0");
	font_init("/home/pi/font.ttc");
	cross_img = fb_read_png_image("/home/pi/cross40.png");
	eraser_img = fb_read_png_image("/home/pi/eraser40.png");
	draw_ui();
	fb_update();

	//打开多点触摸设备文件, 返回文件fd
	touch_fd = touch_init("/dev/input/event0");
	//添加任务, 当touch_fd文件可读时, 会自动调用touch_event_cb函数
	task_add_file(touch_fd, touch_event_cb);

	task_loop(); //进入任务循环
	fb_free_image(eraser_img);
	fb_free_image(cross_img);
	return 0;
}
