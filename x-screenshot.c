#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xutil.h>
#include <png.h>

Window
get_window(int argc, char ** argv, Display * display)
{
	if (argc == 1)
		return DefaultRootWindow(display);

	errno = 0;
	char * str = argv[1], * endptr;
	int base;
	if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
		str += 2;
		base = 16;
	}
	else {
		base = 10;
	}

	Window window = strtol(str, &endptr, base);
	if (errno != 0 || endptr == str) {
		fprintf(stderr, "argument should be a window ID\n");
		exit(1);
	}
	return window;
}

void
get_dimensions(Display * display, Window window, int * width, int * height)
{
	XWindowAttributes window_attributes;
	XGetWindowAttributes(display, window, &window_attributes);
	*width = window_attributes.width;
	*height = window_attributes.height;
}

XImage *
get_screenshot(Display * display, Window window)
{
	int width, height;
	get_dimensions(display, window, &width, &height);

	XImage * image = XGetImage(
		display, window,
		0, 0, width, height,
		AllPlanes, ZPixmap
	);

	if (image == NULL)
		exit(1);

	return image;
}

void
prepare(XImage * image, unsigned char ** data_p, unsigned char *** lines_p)
{
	unsigned char * data = malloc(image->height * image->width * 3);
	if (data == NULL) { perror("malloc"); exit(1); }
	unsigned char ** lines = malloc(image->height * sizeof(unsigned char *));
	if (lines == NULL) { perror("malloc"); exit(1); }

	for (int y = 0; y < image->height; y++)
	{
		lines[y] = data + y * 3 * image->width;
		for (int x = 0; x < image->width ; x++)
		{
			unsigned long pixel = XGetPixel(image, x, y);
			unsigned char * pixbuf = lines[y] + x * 3;
			pixbuf[0] = (pixel & image->  red_mask) >> 16;
			pixbuf[1] = (pixel & image->green_mask) >>  8;
			pixbuf[2] = (pixel & image-> blue_mask) >>  0;
		}
	}
	*data_p = data;
	*lines_p = lines;
}

void
write_png(int width, int height, unsigned char ** lines)
{
	png_structp write_struct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_set_compression_level(write_struct, 3);

	png_infop info_struct = png_create_info_struct(write_struct);

	if (!write_struct || !info_struct || setjmp(png_jmpbuf(write_struct)))
		exit(1);

	png_init_io(write_struct, stdout);
	png_set_IHDR(
		write_struct, info_struct,
		width, height, 8,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE
	);
	png_write_info(write_struct, info_struct);

	png_write_image(write_struct, lines);
	png_free_data(write_struct, info_struct, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&write_struct, NULL);
}

int
main(int argc, char ** argv)
{
	Display * display = XOpenDisplay(NULL);

	Window window = get_window(argc, argv, display);

	XImage * image = get_screenshot(display, window);

	unsigned char * data, **lines;
	prepare(image, &data, &lines);

	write_png(image->width, image->height, lines);

	free(lines);
	free(data);
	XDestroyImage(image);

	return 0;
}
