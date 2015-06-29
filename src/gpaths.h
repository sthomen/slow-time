#ifndef GPATHS_H
#define GPATHS_H

struct _gpath {
	GPath *path;
	const GPathInfo path_info;
};

struct _gpath arm = {
	NULL,
	{
		.num_points = 5,
		.points = (GPoint[]){{-6,-10}, {6,-10}, {6, 10}, {0, 72}, {-6, 10}}
	}
};

struct _gpath wide_line = {
	NULL,
	{
		.num_points = 4,
		.points = (GPoint[]){{-1,0}, {1,0}, {1,10}, {-1,10}}
	}
};

struct _gpath thin_line = {
	NULL,
	{
		.num_points = 2,
		.points = (GPoint[]){{0,0}, {0,10}}
	}
};

#endif /* GPATHS_H */
