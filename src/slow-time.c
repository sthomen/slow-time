#include <pebble.h>

#include "debug.h"

/* Values in pixels.
 * XXX Always use parens when the values are calculations or there may be
 * arithmetic ordering issues.
 */
#define TRACK_WIDTH 10
#define TRACK_OUTER center.x
#define TRACK_INNER (center.x-(TRACK_WIDTH*2))

#define POINTER_WIDTH 30
#define POINTER_HEIGHT 18

#define MINUTES_MAGIC_NUMBER 1.5
#define DATE_MAGIC_NUMBER 3

#define HOUR_OFFSET (TRACK_INNER-TRACK_WIDTH)

#define FONT fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
#define FONT_HOUR fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)
#define FONT_MINUTES fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD)
#define FONT_DATE fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD)

Window *window_main;
GRect bounds;			// holds the window bounds for dynamic size (high-res pebbles?)
GPoint center;

BitmapLayer *layer_background;
BitmapLayer *layer_pointer;
TextLayer *layer_hour;
TextLayer *layer_minutes;
TextLayer *layer_date;

char hour_string[3];		// two digits and null
char minute_string[3];		// two digits and null
char date_string[7];		// three characters, space two digits and null

static int8_t charge=0;
static int32_t stepspercent=0;
static bool charging=false;

struct tm now;

/************************************************************************
 * Utility
 ***********************************************************************/

static GPathInfo *generate_offset_pointer(int offset)
{
	GPathInfo *path=malloc(sizeof(GPathInfo));
	path->num_points=3;
	path->points=malloc(sizeof(GPoint)*3);

	path->points[0].x=0;
	path->points[0].y=TRACK_INNER+offset;

	path->points[1].x=-(POINTER_WIDTH/2);
	path->points[1].y=TRACK_INNER+POINTER_HEIGHT+offset;

	path->points[2].x=POINTER_WIDTH/2;
	path->points[2].y=TRACK_INNER+POINTER_HEIGHT+offset;

	return path;
}

/************************************************************************
 * Callbacks
 ***********************************************************************/

static void update_hours_position() {
	GRect position=layer_get_bounds(text_layer_get_layer(layer_hour));
//	int32_t angle=TRIG_MAX_ANGLE * (now.tm_hour * 60 + now.tm_min) / 1440;
	int32_t angle=TRIG_MAX_ANGLE * now.tm_hour / 24;

	position.origin.x=(-(sin_lookup(angle) * HOUR_OFFSET) / TRIG_MAX_RATIO) + center.x - position.size.w/2;
	position.origin.y=((cos_lookup(angle) * HOUR_OFFSET) / TRIG_MAX_RATIO) + center.y - position.size.w/2;

	layer_set_frame(text_layer_get_layer(layer_hour), position);
}

static void battery_handler(BatteryChargeState state)
{

	charge=state.charge_percent;
	charging=state.is_charging;

	layer_mark_dirty(bitmap_layer_get_layer(layer_background));
}

static void bluetooth_handler(bool connected)
{
	vibes_short_pulse();
}

#ifdef PBL_HEALTH
static bool health_metric_available(HealthMetric metric, HealthServiceTimeScope scope, time_t start, time_t end) {
	return (health_service_metric_accessible(metric, start, end) & HealthServiceAccessibilityMaskAvailable)
		&& (health_service_metric_averaged_accessible(metric, start, end, scope) & HealthServiceAccessibilityMaskAvailable);
}

static void health_handler(HealthEventType event, void *context) {
	int32_t today, average;

	time_t start = time_start_of_today();
	time_t end = time(NULL);

	const HealthMetric metric = HealthMetricStepCount;
	const HealthServiceTimeScope scope = HealthServiceTimeScopeDaily;

	DEBUG_INFO("health_handler");

	if (health_metric_available(metric, scope, start, end)) {

		today = health_service_sum_today(metric);
		average = health_service_sum_averaged(metric, start, end, scope);

		stepspercent = (int32_t)(((float)today / (float)average) * 100.0);

		DEBUG_INFO("health_handler: today: %d, average: %d, percent: %d", (int)today, (int)average, (int)stepspercent);
	}
}
#endif

static void tick_handler(struct tm *tick_time, TimeUnits units_changed)
{
	memcpy(&now, tick_time, sizeof(struct tm));

	layer_mark_dirty(bitmap_layer_get_layer(layer_pointer));

	snprintf(hour_string, 3, "%d", now.tm_hour);
	strftime(minute_string, 3, "%M", &now);
	strftime(date_string, 7, "%a %d", &now);

	update_hours_position();
}

static void update_background(Layer *this, GContext *ctx)
{
	int battery_angle;
	int steps_angle;
	int steps_overflow=0;

	battery_angle=charge * 3.6;
	steps_angle=stepspercent * 3.6;

	if (stepspercent > 100) {
		steps_overflow=(stepspercent - 100) * 3.6;
	}

	// draw background
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, 0);

	graphics_context_set_fill_color(ctx, GColorWhite);

	graphics_fill_radial(ctx,
		GRect(	center.x-TRACK_OUTER,
			center.y-TRACK_OUTER,
			center.x+TRACK_OUTER,
			center.y+TRACK_OUTER),
		GOvalScaleModeFitCircle,
		TRACK_WIDTH*2,
		DEG_TO_TRIGANGLE(0),
		DEG_TO_TRIGANGLE(360));


	// the circles below are offset by 180 degrees so that they start at the bottom like the hours

	// draw battery state
#ifdef PBL_COLOR
	graphics_context_set_fill_color(ctx, GColorOrange);
#else
	graphics_context_set_fill_color(ctx, GColorBlack);
#endif

	graphics_fill_radial(ctx,
		GRect(	center.x-TRACK_OUTER,
			center.y-TRACK_OUTER,
			center.x+TRACK_OUTER,
			center.y+TRACK_OUTER),
		GOvalScaleModeFitCircle,
		TRACK_WIDTH,
		DEG_TO_TRIGANGLE(180 - battery_angle),
		DEG_TO_TRIGANGLE(180));

	// draw steps graph
#ifdef PBL_COLOR
	graphics_context_set_fill_color(ctx, GColorMalachite);
#else
	graphics_context_set_fill_color(ctx, GColorBlack);
#endif

	graphics_fill_radial(ctx,
		GRect(	center.x-TRACK_OUTER+TRACK_WIDTH,
			center.y-TRACK_OUTER+TRACK_WIDTH,
			center.x+TRACK_OUTER-(TRACK_WIDTH*2),
			center.y+TRACK_OUTER-(TRACK_WIDTH*2)),
		GOvalScaleModeFitCircle,
		TRACK_WIDTH,
		DEG_TO_TRIGANGLE(180 - steps_angle),
		DEG_TO_TRIGANGLE(180));

	// draw steps overflow
#ifdef PBL_COLOR
	graphics_context_set_fill_color(ctx, GColorIndigo);
#else
	graphics_context_set_fill_color(ctx, GColorWhite);
#endif

	graphics_fill_radial(ctx,
		GRect(	center.x-TRACK_OUTER+TRACK_WIDTH,
			center.y-TRACK_OUTER+TRACK_WIDTH,
			center.x+TRACK_OUTER-(TRACK_WIDTH*2),
			center.y+TRACK_OUTER-(TRACK_WIDTH*2)),
		GOvalScaleModeFitCircle,
		TRACK_WIDTH,
		DEG_TO_TRIGANGLE(180 - steps_overflow),
		DEG_TO_TRIGANGLE(180));
}

static void update_pointer(Layer *this, GContext *ctx)
{
	int total_minutes;
	GPath *path_triangle=NULL;
	GPath *path_triangle_inset=NULL;

	total_minutes=now.tm_hour * 60 + now.tm_min;

	path_triangle=gpath_create(generate_offset_pointer(0));

	gpath_rotate_to(path_triangle, TRIG_MAX_ANGLE / 1440 * total_minutes);
	gpath_move_to(path_triangle, center);

	graphics_context_set_fill_color(ctx, GColorBlack);
	gpath_draw_filled(ctx, path_triangle);

	path_triangle_inset=gpath_create(generate_offset_pointer(5));

	gpath_rotate_to(path_triangle_inset, TRIG_MAX_ANGLE / 1440 * total_minutes);
	gpath_move_to(path_triangle_inset, center);

	graphics_context_set_fill_color(ctx, GColorWhite);
	gpath_draw_filled(ctx, path_triangle_inset);
}

/************************************************************************
 * Main window
 ***********************************************************************/

static void window_main_load(Window *window)
{
	Layer *root;

	time_t t;
	GSize ms, ds, hs;

	root=window_get_root_layer(window_main);

	// load window size
	bounds=layer_get_bounds(root);
	center=GPoint(bounds.size.w/2, bounds.size.h/2);

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

	// set up minutes layer

	// Measure the biggest probable numbers for this variable-size font
	ms=graphics_text_layout_get_content_size("00",
		FONT_MINUTES,
		bounds,
		GTextOverflowModeWordWrap,
		GTextAlignmentCenter);

	layer_minutes=text_layer_create(GRect(0,center.y-(ms.h/MINUTES_MAGIC_NUMBER), bounds.size.w, ms.h));
	layer_add_child(root, text_layer_get_layer(layer_minutes));

	text_layer_set_text_alignment(layer_minutes, GTextAlignmentCenter);
	text_layer_set_font(layer_minutes, FONT_MINUTES);
	text_layer_set_background_color(layer_minutes, GColorClear);
	text_layer_set_text_color(layer_minutes, GColorWhite);
	text_layer_set_text(layer_minutes, minute_string);

	// Measure the biggest probable numbers for this variable-size font
	ds=graphics_text_layout_get_content_size("Mmm 00",
		FONT_DATE,
		bounds,
		GTextOverflowModeWordWrap,
		GTextAlignmentCenter);

	layer_date=text_layer_create(GRect(0,center.y+(ms.h/DATE_MAGIC_NUMBER), bounds.size.w, ds.h));
	layer_add_child(root, text_layer_get_layer(layer_date));

	text_layer_set_text_alignment(layer_date, GTextAlignmentCenter);
	text_layer_set_font(layer_date, FONT_DATE);
	text_layer_set_background_color(layer_date, GColorClear);
	text_layer_set_text_color(layer_date, GColorWhite);
	text_layer_set_text(layer_date, date_string);

	hs=graphics_text_layout_get_content_size("00",
		FONT_HOUR,
		bounds,
		GTextOverflowModeWordWrap,
		GTextAlignmentCenter);

	layer_hour=text_layer_create(GRect(0,0,hs.w,hs.h));
	layer_add_child(root, text_layer_get_layer(layer_hour));

	text_layer_set_font(layer_hour, FONT_HOUR);
	text_layer_set_background_color(layer_hour, GColorClear);
	text_layer_set_text_color(layer_hour, GColorWhite);
	text_layer_set_text(layer_hour, hour_string);
}

static void window_main_unload(Window *window)
{
	bitmap_layer_destroy(layer_background);
	bitmap_layer_destroy(layer_pointer);
	text_layer_destroy(layer_minutes);
	text_layer_destroy(layer_date);
	text_layer_destroy(layer_hour);
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

	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	battery_state_service_subscribe(battery_handler);

	BatteryChargeState state=battery_state_service_peek();
	charge=state.charge_percent;

	bluetooth_connection_service_subscribe(bluetooth_handler);

#ifdef PBL_HEALTH
	health_service_events_subscribe(health_handler, NULL);
#endif

	window_main = window_create();
	window_set_window_handlers(window_main, window_handlers);
	window_stack_push(window_main, true);
}

static void deinit()
{
	window_destroy(window_main);
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();

#ifdef PBL_HEALTH
	health_service_events_unsubscribe();
#endif
}

int main(void)
{
	init();
	app_event_loop();
	deinit();
}

