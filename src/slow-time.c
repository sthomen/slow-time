#include <pebble.h>

#include "debug.h"
#include "gpaths.h"

Window *window_main;
GBitmap *watch_back;
BitmapLayer *layer_watchface;
Layer *layer_arm;

struct tm *now=NULL;

/************************************************************************
 * Time
 ***********************************************************************/

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	now=tick_time;

	if (units_changed & MINUTE_UNIT) {
		layer_mark_dirty(layer_arm);
	}
}

/************************************************************************
 * Arm
 ***********************************************************************/

static void layer_arm_update(Layer *this, GContext *ctx)
{
	int16_t total_minutes=0;
	GRect bounds;

	bounds=layer_get_bounds(this);

	if (now!=NULL) {
		total_minutes=now->tm_hour * 60 + now->tm_min;
	}

	gpath_rotate_to(arm.path, TRIG_MAX_ANGLE / 1440 *  total_minutes);
	gpath_move_to(arm.path, GPoint(bounds.size.w/2, bounds.size.h/2));

#ifdef PBL_COLOR
	graphics_context_set_fill_color(ctx, GColorDarkGray);
#else
	graphics_context_set_fill_color(ctx, GColorBlack);
#endif
	graphics_context_set_stroke_color(ctx, GColorBlack);

	gpath_draw_filled(ctx, arm.path);

#ifdef PBL_COLOR
	gpath_draw_outline(ctx, arm.path);
#else
	graphics_context_set_fill_color(ctx, GColorWhite);
#endif
	graphics_fill_circle(ctx, GPoint(bounds.size.w/2, bounds.size.h/2), 7);
	graphics_draw_circle(ctx, GPoint(bounds.size.w/2, bounds.size.h/2), 7);
}

/************************************************************************
 * Main window
 ***********************************************************************/

static void window_main_load(Window *window)
{
	Layer *root;
	GRect bounds;

	root=window_get_root_layer(window_main);

	bounds = layer_get_bounds(root);

	/* create gpaths */

	arm.path=gpath_create(&arm.path_info);
	wide_line.path=gpath_create(&wide_line.path_info);
	thin_line.path=gpath_create(&thin_line.path_info);

	/* watchface */

	layer_watchface=bitmap_layer_create(bounds);
	layer_add_child(root, bitmap_layer_get_layer(layer_watchface));
	watch_back=gbitmap_create_with_resource(RESOURCE_ID_WATCH_BACK);
	bitmap_layer_set_bitmap(layer_watchface, watch_back);

	/* arm */

	layer_arm = layer_create(bounds);
	layer_add_child(root, layer_arm);
	layer_set_update_proc(layer_arm, layer_arm_update);
}

static void window_main_unload(Window *window)
{
	layer_destroy(layer_arm);
	bitmap_layer_destroy(layer_watchface);
	gbitmap_destroy(watch_back);

	/* clean up gpaths */

	gpath_destroy(arm.path);
	gpath_destroy(wide_line.path);
	gpath_destroy(thin_line.path);
}

/************************************************************************
 * General
 ***********************************************************************/

static void init()
{
	time_t t;

	static WindowHandlers window_handlers={
		.load = window_main_load,
		.unload = window_main_unload
	};

	window_main = window_create();
	window_set_window_handlers(window_main, window_handlers);
	window_stack_push(window_main, true);

	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	t=time(NULL);
	now=localtime(&t);
}

static void deinit()
{
	window_destroy(window_main);
	tick_timer_service_unsubscribe();
	free(now);
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
}

