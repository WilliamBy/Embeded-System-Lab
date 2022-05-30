#include "common.h"
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>


static int LCD_FB_FD;
static int *LCD_FB_BUF = NULL;
static int *LCD_FB_FRONT, *LCD_FB_BACK;
struct fb_var_screeninfo LCD_FB_VAR;
static int DRAW_BUF[SCREEN_WIDTH*SCREEN_HEIGHT];

static struct area {
	int x1, x2, y1, y2;
} update_area = {0,0,0,0};

#define AREA_SET_EMPTY(pa) do {\
	(pa)->x1 = SCREEN_WIDTH;\
	(pa)->x2 = 0;\
	(pa)->y1 = SCREEN_HEIGHT;\
	(pa)->y2 = 0;\
} while(0)

void fb_init(char *dev)
{
	int fd;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var;

	if(LCD_FB_BUF != NULL) return; /*already done*/

	//First: Open the device
	if((fd = open(dev, O_RDWR)) < 0){
		printf("Unable to open framebuffer %s, errno = %d\n", dev, errno);
		return;
	}
	if(ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix) < 0){
		printf("Unable to FBIOGET_FSCREENINFO %s\n", dev);
		return;
	}
	if(ioctl(fd, FBIOGET_VSCREENINFO, &fb_var) < 0){
		printf("Unable to FBIOGET_VSCREENINFO %s\n", dev);
		return;
	}

	printf("framebuffer info: bits_per_pixel=%u,size=(%d,%d),virtual_pos_size=(%d,%d)(%d,%d),line_length=%u,smem_len=%u\n",
		fb_var.bits_per_pixel, fb_var.xres, fb_var.yres, fb_var.xoffset, fb_var.yoffset,
		fb_var.xres_virtual, fb_var.yres_virtual, fb_fix.line_length, fb_fix.smem_len);

	//Second: mmap
	int *addr;
	addr = mmap(NULL, fb_fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if((int)addr == -1){
		printf("failed to mmap memory for framebuffer.\n");
		return;
	}

	if((fb_var.xoffset != 0) ||(fb_var.yoffset != 0))
	{
		fb_var.xoffset = 0;
		fb_var.yoffset = 0;
		if(ioctl(fd, FBIOPAN_DISPLAY, &fb_var) < 0) {
			printf("FBIOPAN_DISPLAY framebuffer failed\n");
		}
	}


	LCD_FB_FD = fd;
	LCD_FB_BUF = addr;
	LCD_FB_FRONT = addr;
	LCD_FB_BACK = addr + fb_var.xres*fb_var.yres;
	LCD_FB_VAR = fb_var;

	//set empty
	AREA_SET_EMPTY(&update_area);
	return;
}

static void _copy_area(int *dst, int *src, struct area *pa)
{
	int x, y, w, h;
	x = pa->x1; w = pa->x2-x;
	y = pa->y1; h = pa->y2-y;
	src += y*SCREEN_WIDTH + x;
	dst += y*SCREEN_WIDTH + x;
	while(h-- > 0){
		memcpy(dst, src, w*4);
		src += SCREEN_WIDTH;
		dst += SCREEN_WIDTH;
	}
}

static int _check_area(struct area *pa)
{
	if(pa->x2 == 0) return 0; //is empty

	if(pa->x1 < 0) pa->x1 = 0;
	if(pa->x2 > SCREEN_WIDTH) pa->x2 = SCREEN_WIDTH;
	if(pa->y1 < 0) pa->y1 = 0;
	if(pa->y2 > SCREEN_HEIGHT) pa->y2 = SCREEN_HEIGHT;

	if((pa->x2 > pa->x1) && (pa->y2 > pa->y1))
		return 1; //no empty

	//set empty
	AREA_SET_EMPTY(pa);
	return 0;
}

void fb_update(void)
{
	if(_check_area(&update_area) == 0) return; //is empty
	_copy_area(LCD_FB_FRONT, DRAW_BUF, &update_area);
	AREA_SET_EMPTY(&update_area); //set empty
	return;
}

/*======================================================================*/

static void * _begin_draw(int x, int y, int w, int h)
{
	int x2 = x+w;
	int y2 = y+h;
	if(update_area.x1 > x) update_area.x1 = x;
	if(update_area.y1 > y) update_area.y1 = y;
	if(update_area.x2 < x2) update_area.x2 = x2;
	if(update_area.y2 < y2) update_area.y2 = y2;
	return DRAW_BUF;
}

void fb_draw_pixel(int x, int y, int color)
{
	if(x<0 || y<0 || x>=SCREEN_WIDTH || y>=SCREEN_HEIGHT) return;
	int *buf = _begin_draw(x,y,1,1);
/*---------------------------------------------------*/
	*(buf + y*SCREEN_WIDTH + x) = color;
/*---------------------------------------------------*/
	return;
}

void fb_draw_rect(int x, int y, int w, int h, int color)
{
	if(x < 0) { w += x; x = 0;}
	if(x+w > SCREEN_WIDTH) { w = SCREEN_WIDTH-x;}
	if(y < 0) { h += y; y = 0;}
	if(y+h >SCREEN_HEIGHT) { h = SCREEN_HEIGHT-y;}
	if(w<=0 || h<=0) return;
	int *buf = _begin_draw(x,y,w,h);
/*---------------------------------------------------*/
	// printf("you need implement fb_draw_rect()\n"); exit(0);
	// Add your code here
	for (int yy = 0; yy < h; yy++)
	{
		for (int xx = 0; xx < w; xx++)
		{
			*(buf + (y + yy) * SCREEN_WIDTH + (x + xx)) = color;
		}
	}
	/*---------------------------------------------------*/
	return;
}

void fb_draw_line(int x1, int y1, int x2, int y2, int color)
{
/*---------------------------------------------------*/
	// printf("you need implement fb_draw_line()\n"); exit(0);
	// Add your code here
	// Bresenha 算法
	int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;

    while (fb_draw_pixel(x1, y1, color), x1 != x2 || y1 != y2) {
        int e2 = err;
        if (e2 > -dx) { err -= dy; x1 += sx; }
        if (e2 <  dy) { err += dx; y1 += sy; }
    }
/*---------------------------------------------------*/
	return;
}

void fb_draw_image(int x, int y, fb_image *image, int color)
{
	if(image == NULL) return;
	int ix = 0; // image x
	int iy = 0; //image y
	int w = image->pixel_w; //draw width
	int h = image->pixel_h; //draw height

	if(x<0) {w+=x; ix-=x; x=0;}
	if(y<0) {h+=y; iy-=y; y=0;}
	
	if(x+w > SCREEN_WIDTH) {
		w = SCREEN_WIDTH - x;
	}
	if(y+h > SCREEN_HEIGHT) {
		h = SCREEN_HEIGHT - y;
	}
	if((w <= 0)||(h <= 0)) return;

	int *buf = _begin_draw(x,y,w,h);
/*---------------------------------------------------------------*/
	char *dst = (char *)(buf + y*SCREEN_WIDTH + x);
	char *src = image->content + iy*image->line_byte + ix*4;
/*---------------------------------------------------------------*/

	int alpha;
	int ww;

	if(image->color_type == FB_COLOR_RGB_8880) /*lab3: jpg*/
	{
		// printf("you need implement fb_draw_image() FB_COLOR_RGB_8880\n"); exit(0);
		// Add your code here
		for (ww = 0; ww < h; ++ww)
		{
			memcpy(dst, src, w * 4);
			dst += SCREEN_WIDTH * 4;
			src += image->line_byte;
		}
		return;
	}

	if(image->color_type == FB_COLOR_RGBA_8888) /*lab3: png*/
	{
		// printf("you need implement fb_draw_image() FB_COLOR_RGBA_8888\n"); exit(0);
		// Add your code here
		char R1, G1, B1, alpha;
		char *from = src;
		char *p = dst;
		for (int i = 0; i < w; i++)
		{
			for (int j = 0; j < h; j++)
			{
				p = dst + j * SCREEN_WIDTH * 4 + i * 4;
				from = src + j * image->line_byte + i * 4;
				alpha = from[3];
				B1 = from[0];
				G1 = from[1];
				R1 = from[2];
				switch (alpha)
				{
				case 0:
					break;
				case 255:
					p[0] = B1;
					p[1] = G1;
					p[2] = R1;
					break;
				default:
					p[0] += (((B1 - p[0]) * alpha) >> 8);
					p[1] += (((G1 - p[1]) * alpha) >> 8);
					p[2] += (((R1 - p[2]) * alpha) >> 8);
				}
			}
		}
		return;
	}

	if(image->color_type == FB_COLOR_ALPHA_8) /*lab3: font*/
	{
		// printf("you need implement fb_draw_image() FB_COLOR_ALPHA_8\n"); exit(0);
		// Add your code here
		int line_bytes = image->line_byte;
		char R1, G1, B1, alpha;
		char *from = src;
		char *p = dst;
		for (int i = 0; i < w; i++)
		{
			for (int j = 0; j < h; j++)
			{
				p = dst + j * SCREEN_WIDTH * 4 + i * 4;
				from = src + j * line_bytes + i;
				alpha = *from;
				R1 = (color & 0xff0000) >> 16;
				G1 = (color & 0xff00) >> 8;
				B1 = (color & 0xff);
				switch (alpha)
				{
				case 0:
					break;
				case 255:
					p[0] = B1;
					p[1] = G1;
					p[2] = R1;
					break;
				default:
					p[0] += (((B1 - p[0]) * alpha) >> 8);
					p[1] += (((G1 - p[1]) * alpha) >> 8);
					p[2] += (((R1 - p[2]) * alpha) >> 8);
				}
			}
		}
		return;
	}
/*---------------------------------------------------------------*/
	return;
}

void fb_draw_border(int x, int y, int w, int h, int color)
{
	if(w<=0 || h<=0) return;
	fb_draw_rect(x, y, w, 1, color);
	if(h > 1) {
		fb_draw_rect(x, y+h-1, w, 1, color);
		fb_draw_rect(x, y+1, 1, h-2, color);
		if(w > 1) fb_draw_rect(x+w-1, y+1, 1, h-2, color);
	}
}

/** draw a text string **/
void fb_draw_text(int x, int y, char *text, int font_size, int color)
{
	fb_image *img;
	fb_font_info info;
	int i=0;
	int len = strlen(text);
	while(i < len)
	{
		img = fb_read_font_image(text+i, font_size, &info);
		if(img == NULL) break;
		fb_draw_image(x+info.left, y-info.top, img, color);
		fb_free_image(img);

		x += info.advance_x;
		i += info.bytes;
	}
	return;
}

// draw a straight line with offerd direction (1 accord with vertical and 0 with horizonal)
void fb_draw_straight_line(int x, int y, int len, int direction, int color)
{
	if (direction == 1)
	{
		for (int yy = 0; yy < len; yy++)
		{
			fb_draw_pixel(x, y + yy, color);
		}
	}
	else
	{
		for (int xx = 0; xx < len; xx++)
		{
			fb_draw_pixel(x + xx, y, color);
		}
	}
	return;
}

// draw a round with offered radius
void fb_draw_round(int x, int y, int r, int color)
{
	/*上半球*/
	int xi = x, yi = y - r;	// 当前迭代的点
	int offset_x = 0, offset_y = r; // 当前迭代的点到中轴线的距离
	while(offset_y >= 0)
	{
		fb_draw_straight_line(xi, yi, 1 + 2 * offset_x, 0, color);
		xi--;
		if ((xi - x) * (xi - x) + (yi - y) * (yi - y) - r * r < 0)
		{
			// 下一个迭代的点位于正左方
			offset_x++;
		}
		else
		{
			// 下一个迭代的点位于正下方
			offset_y--;
			xi++;
			yi++;
		}
	}
	/*下半球*/
	xi = x, yi = y + r;	// 当前迭代的点
	offset_x = 0, offset_y = r; // 当前迭代的点到中轴线的距离
	while(offset_y >= 0)
	{
		fb_draw_straight_line(xi, yi, 1 + 2 * offset_x, 0, color);
		xi--;
		if ((xi - x) * (xi - x) + (yi - y) * (yi - y) - r * r < 0)
		{
			// 下一个迭代的点位于正左方
			offset_x++;
		}
		else
		{
			// 下一个迭代的点位于正上方
			offset_y--;
			xi++;
			yi--;
		}
	}
	return;
}

void fb_draw_thick_line(int x1, int y1, int x2, int y2, int r, int color)
{
	int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
	int dy = abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2;

	fb_draw_pixel(x1, y1, color);

	while (x1 != x2 || y1 != y2)
	{
		int e2 = err;
		if (e2 > -dx)
		{
			err -= dy;
			x1 += sx;
		}
		if (e2 < dy)
		{
			err += dx;
			y1 += sy;
		}
		fb_draw_round(x1, y1, r, color);
	}
}

fb_image* zoomin_image(fb_image *img)
{
	if (img == NULL)
	{
		return NULL;
	}
	if (img->pixel_h < SCREEN_HEIGHT * 3 || img->pixel_w < SCREEN_WIDTH * 3)
	{
		return NULL;
	}
	int old_h = img->pixel_h, old_w = img->pixel_w, old_lb = img->line_byte, new_lb = old_lb * 2;
	fb_image* new_img = fb_new_image(img->color_type, old_w * 2, old_h * 2, new_lb);
	if (new_img == NULL)
	{
		exit(-1);
	}
	int32_t *old = (int32_t*)img->content;
	int32_t *new = (int32_t*)new_img->content;
	int32_t *old_ptr, *new_ptr;
	for (int row = 0; row < old_h; row++)
	{
		old_ptr = old + (row * old_w);
		new_ptr = new + (2 * row * new_img->pixel_w);
		for (int col = 0; col < old_w; col++)
		{
			*(new_ptr++) = *old_ptr;
			*(new_ptr++) = *(old_ptr++);
		}	// 横向x2
		memcpy(new_ptr, new_ptr - new_img->pixel_w, new_lb);	// 纵向x2
	}
	return new_img;
}

fb_image *zoomout_image(fb_image *img)
{
	if (img == NULL)
	{
		return NULL;
	}
	if (img->pixel_h < SCREEN_HEIGHT / 5 || img->pixel_w < SCREEN_WIDTH / 5)
	{
		return NULL;
	}
	int old_h = img->pixel_h, old_w = img->pixel_w, old_lb = img->line_byte, new_lb = old_lb / 2;
	fb_image *new_img = fb_new_image(img->color_type, old_w / 2, old_h / 2, (old_w / 2) * 4);
	if (new_img == NULL)
	{
		exit(-1);
	}
	int32_t *old = (int32_t *)img->content;
	int32_t *new = (int32_t *)new_img->content;
	int32_t *old_ptr, *new_ptr;
	for (int row = 0; row < new_img->pixel_h; row++)
	{
		old_ptr = old + (2 * row * old_w);
		new_ptr = new + (row * new_img->pixel_w);
		for (int col = 0; col < new_img->pixel_w; col++)
		{
			*(new_ptr++) = *old_ptr;
			old_ptr += 2;
		}
	}
	return new_img;
}

fb_image* zoom_image(fb_image *img, int level)
{
	fb_image *new_img;
	if (level < 0)
	{
		for (int i = level; i < 0; i++)
		{
			new_img = zoomout_image(img);
		}
	}
	else
	{
		for (int i = level; i < 0; i++)
		{
			new_img = zoomout_image(img);
		}
	}
}