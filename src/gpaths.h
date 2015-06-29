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
		.points = (GPoint[]){{-6,-10}, {6,-10}, {6, 10}, {0, 60}, {-6, 10}}
	}
};

struct _gpath wide_line = {
	NULL,
	{
		.num_points = 4,
		.points = (GPoint[]){{-4,60}, {4,60}, {4,65}, {-4,65}}
	}
};

struct _gpath thin_line = {
	NULL,
	{
		.num_points = 2,
		.points = (GPoint[]){{0,60}, {0,65}}
	}
};

#endif /* GPATHS_H */
