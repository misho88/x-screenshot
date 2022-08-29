#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xutil.h>
#include <png.h>

/**
 * get dimensions of window on display, store in *width and *height
 */
void
get_dimensions(Display * display, Window window, int * width, int * height)
{
	XWindowAttributes window_attributes;
	XGetWindowAttributes(display, window, &window_attributes);
	*width = window_attributes.width;
	*height = window_attributes.height;
}

/**
 * take a screenshot of window and display and return as a XImage*
 */
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

/**
 * describe a rectangle stored regularly but non-contiguously in memory
 */
struct mem2d {
	unsigned char * origin;
	ssize_t n_rows, n_cols, row_stride, col_stride;
};

/**
 * average all the elements of a memory map
 */
int
mean(struct mem2d m)
{
	int sum = 0;
	ssize_t max_i = m.n_rows * m.row_stride;
	ssize_t max_j = m.n_cols * m.col_stride;
	for (ssize_t i = 0; i < max_i; i += m.row_stride)
		for (ssize_t j = 0; j < max_j; j += m.col_stride)
			sum += m.origin[i + j];
	return sum / (m.n_rows * m.n_cols);
}

/**
 * extract a rectangular subdomain from a memory map
 */
struct mem2d
region(struct mem2d m, ssize_t i, ssize_t j, ssize_t n_rows, ssize_t n_cols)
{
	return (struct mem2d){
		.origin = m.origin + i * m.row_stride + j * m.col_stride,
		.n_rows = n_rows,
		.n_cols = n_cols,
		.row_stride = m.row_stride,
		.col_stride = m.col_stride
	};
}

/**
 * scale down rectangle in so that it fits out
 */
void
shrink(struct mem2d in, struct mem2d out)
{
	if (in.n_rows < out.n_rows || in.n_cols < out.n_rows)
		exit(101);  // this should only happen if something is messed up internally

	ssize_t row_scale = in.n_rows / out.n_rows;
	ssize_t col_scale = in.n_cols / out.n_cols;
	if (row_scale == 1 && col_scale == 1) // this is just a copy, so keep it simple
		for (ssize_t i = 0; i < out.n_rows; i++)
			for (ssize_t j = 0; j < out.n_cols; j++)
				out.origin[
					i * out.row_stride + j * out.col_stride
				] = in.origin[
					i * in.row_stride + j * in.col_stride
				];
	else
		for (ssize_t i = 0; i < out.n_rows; i++)
			for (ssize_t j = 0; j < out.n_cols; j++)
				out.origin[
					i * out.row_stride + j * out.col_stride
				] = mean(
					region(in, row_scale * i, col_scale * j, row_scale, col_scale)
				);
}

/**
 * write image m to stdout as a PNG
 *
 * m.col_stride must be 3 for libpng to accept this
 */
void
write_png(struct mem2d m)
{
	if (m.col_stride != 3)
		exit(101);  // this should only happen if something is messed up internally

	unsigned char ** lines = malloc(m.n_rows * sizeof(unsigned char *));
	if (lines == NULL) { perror("malloc"); exit(1); }
	for (int i = 0; i < m.n_rows; i++)
		lines[i] = m.origin + i * m.row_stride;
	
	png_structp write_struct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_set_compression_level(write_struct, 3);

	png_infop info_struct = png_create_info_struct(write_struct);

	if (!write_struct || !info_struct || setjmp(png_jmpbuf(write_struct)))
		exit(1);

	png_init_io(write_struct, stdout);
	png_set_IHDR(
		write_struct, info_struct,
		m.n_cols, m.n_rows, 8,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE
	);
	png_write_info(write_struct, info_struct);

	png_write_image(write_struct, lines);
	png_free_data(write_struct, info_struct, PNG_FREE_ALL, -1);
	png_destroy_write_struct(&write_struct, NULL);

	free(lines);
}

/* set up argument parsing */

#include "xargparse.h"

#define arguments(_) \
	_( 0 , NULL     , char const *, program,    , xap_string) \
	_('h', "help"   , bool        , help   ,    , xap_toggle) \
	_('v', "version", bool        , version,    , xap_toggle) \
	_('W', "width"  , int         , width  ,    , xap_int   ) \
	_('H', "height" , int         , height ,    , xap_int   ) \
	_( 1 , NULL     , int         , id     ,    , xap_int   )

#define stop_after(_) \
	_('h', "help"   ) \
	_('v', "version")

#define required(_) \
	_( 0 , NULL )

#define display_hints(_) \
	_( 0 , NULL     , "x-screenshot", "" /* or XAP_NO_HELP */        ) \
	_( 1 , NULL     , "int"         , "the window ID (omit for root)") \
	_('h', "help"   , ""            , "show this help and exit"      ) \
	_('v', "version", ""            , "show the version and exit"    ) \
	_('W', "width"  , "int"         , "the max width of the result"  ) \
	_('H', "height" , "int"         , "the max height of the result" )

struct args xap_struct(arguments);
xap_define_parser(parse, struct args, arguments, stop_after, required);
xap_define_fprint_usage(fprint_usage, arguments, required, display_hints);
xap_define_fprint_help(fprint_help, arguments, display_hints);

int
main(int argc, char ** argv)
{
	/* parse arguments */
	struct args args = { .id = -1, .width = -1, .height = -1 };
	xap_error_context_t ctx = parse(&argc, argv, &args);
	if (ctx.error) {
		xap_fprint_error_context(&ctx, stderr);
		fprint_usage(stderr);
		return 1;
	}
	if (args.help) {
		fprint_usage(stdout);
		fprint_help(stdout);
		return 0;
	}
	if (args.version) {
		printf("Version 0.0.1 (x-screenshot)\n");
		return 0;
	}
	if (argc) {
		fprintf(stderr, "unknown arguments: ");
		xap_fprint_args(argc, argv, stderr);
		fprint_usage(stderr);
		return 1;
	}

	/* grab a screenshot */
	Display * display = XOpenDisplay(NULL);
	Window window = args.id < 0 ? DefaultRootWindow(display) : (Window)args.id;
	XImage * image = get_screenshot(display, window);

	/* prepare output image */
	if (args.width  < 0) args.width  = image->width ;
	if (args.height < 0) args.height = image->height;
	int width, height;
	if (args.width * image->height < args.height * image->width) {
		width = args.width < image->width ? args.width : image->width;
		height = image->height * args.width / image->width;
	}
	else {
		height = args.height < image->height ? args.height : image->height;
		width = image->width * args.height / image->height;
	}
	unsigned char * shrunken = malloc(width * height * 3);
	if (shrunken == NULL) { perror("malloc"); exit(1); }

	/* fill output image */
	#pragma omp parallel for
	for (int c = 0; c < 3; c++)
		shrink((struct mem2d){
			.origin = (unsigned char *)image->data + (int [3]){ 2, 1, 0 }[c],
			.n_rows = image->height,
			.n_cols = image->width,
			.row_stride = image->width * 4,
			.col_stride = 4
		}, (struct mem2d){
			.origin = shrunken + c,
			.n_rows = height,
			.n_cols = width,
			.row_stride = width * 3,
			.col_stride = 3
		});

	/* write output image */
	write_png((struct mem2d){
		.origin = shrunken,
		.n_rows = height,
		.n_cols = width,
		.row_stride = width * 3,
		.col_stride = 3
	});

	/* clean up */
	free(shrunken);
	XDestroyImage(image);
	return 0;
}
