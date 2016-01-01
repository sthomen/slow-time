#include <pebble.h>

#include "debug.h"

#define TRACK_WIDTH 20
#define TRACK_OUTER center.x
#define TRACK_INNER (center.x-TRACK_WIDTH)

#define POINTER_WIDTH 30
#define POINTER_HEIGHT 15

#define NUMERAL_OFFSET 10

#define FONT fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD)

Window *window_main;
GRect bounds;			// holds the window bounds for dynamic size (high-res pebbles?)
GPoint center;

BitmapLayer *layer_background;
BitmapLayer *layer_pointer;

GPath *path_triangle=NULL;
GPathInfo *path_triangle_info;

struct tm now;

/************************************************************************
 * Utility
 ***********************************************************************/

static GPathInfo *generate_offset_pointer()
{
	GPathInfo *path=malloc(sizeof(GPathInfo));
	path->num_points=3;
	path->points=malloc(sizeof(GPoint)*3);

	path->points[0].x=0;
	path->points[0].y=TRACK_INNER;

	path->points[1].x=-(POINTER_WIDTH/2);
	path->points[1].y=TRACK_INNER+POINTER_HEIGHT;

	path->points[2].x=POINTER_WIDTH/2;
	path->points[2].y=TRACK_INNER+POINTER_HEIGHT;

	return path;
}

/************************************************************************
 * Callbacks
 ***********************************************************************/

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	memcpy(&now, tick_time, sizeof(struct tm));

	layer_mark_dirty(bitmap_layer_get_layer(layer_pointer));
}

static void update_background(Layer *this, GContext *ctx)
{
	int i;
	GSize ts;
	GRect bb;

	struct {
		char *text;
		GPoint position;
	} numerals[] = {
		{ "0", { center.x, center.y+TRACK_INNER-NUMERAL_OFFSET } },
		{ "6", { center.x-TRACK_INNER+NUMERAL_OFFSET, center.y } },
		{ "12", { center.x, center.y-TRACK_INNER+NUMERAL_OFFSET } },
		{ "18", { center.x+TRACK_INNER-NUMERAL_OFFSET, center.y } }
	};

	// draw background
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, 0);

	// draw track
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_circle(ctx, center, TRACK_OUTER);

	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_circle(ctx, center, TRACK_INNER);

	// draw numerals
	graphics_context_set_text_color(ctx, GColorWhite);

	i=0;
	do {
		ts=graphics_text_layout_get_content_size(numerals[i].text,
			FONT,
			bounds,
			GTextOverflowModeWordWrap,
			GTextAlignmentCenter);

		/* XXX	magic number 1.5 because the font is aligned along the baseline, and this
		 *	brings the Y pos up a little to center the numbers.
		 */
		bb=GRect(numerals[i].position.x-(ts.w/2),numerals[i].position.y-(ts.h/(1.5)),ts.w,ts.h);

		graphics_draw_text(ctx,
			numerals[i].text,
			FONT,
			bb,
			GTextOverflowModeWordWrap, /* don't care */
			GTextAlignmentCenter,
			NULL);
	} while (++i < 4);
}

static void update_pointer(Layer *this, GContext *ctx)
{
	int total_minutes;

	total_minutes=now.tm_hour * 60 + now.tm_min;

	if (path_triangle != NULL)
		gpath_destroy(path_triangle);

	path_triangle=gpath_create(path_triangle_info);

	graphics_context_set_fill_color(ctx, GColorBlack);

	gpath_rotate_to(path_triangle, TRIG_MAX_ANGLE / 1440 * total_minutes);
	gpath_move_to(path_triangle, center);

	gpath_draw_filled(ctx, path_triangle);
}

/************************************************************************
 * Main window
 ***********************************************************************/

static void window_main_load(Window *window)
{
	time_t t;
	Layer *root;

	root=window_get_root_layer(window_main);

	// load window size
	bounds=layer_get_bounds(root);
	center=GPoint(bounds.size.w/2, bounds.size.h/2);
	// XXX this must be run after center is initialized
	path_triangle_info=generate_offset_pointer();

	t=time(NULL);
	memcpy(&now, localtime(&t), sizeof(struct tm));

	// set up background
	layer_background=bitmap_layer_create(bounds);
	layer_add_child(root, bitmap_layer_get_layer(layer_background));
	layer_set_update_proc(bitmap_layer_get_layer(layer_background), update_background);

	// set up pointer layer
	layer_pointer=bitmap_layer_create(bounds);
	layer_add_child(root, bitmap_layer_get_layer(layer_pointer));
	layer_set_update_proc(bitmap_layer_get_layer(layer_pointer), update_pointer);
}

static void window_main_unload(Window *window)
{
	bitmap_layer_destroy(layer_background);
	bitmap_layer_destroy(layer_pointer);
}

/************************************************************************
 * General
 ***********************************************************************/

static void init()
{
	static WindowHandlers window_handlers={
		.load = window_main_load,
		.unload = window_main_unload
	};

	window_main = window_create();
	window_set_window_handlers(window_main, window_handlers);
	window_stack_push(window_main, true);

	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit()
{
	window_destroy(window_main);
	tick_timer_service_unsubscribe();
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
}

