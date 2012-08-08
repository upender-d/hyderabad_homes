/**********************************************************************
 *
 * BOX3D IO and conversions
 *
 **********************************************************************/

#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/geo_decls.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define SHOW_DIGS_DOUBLE 15
#define MAX_DIGS_DOUBLE (SHOW_DIGS_DOUBLE + 6 + 1 + 3 +1)

/* forward defs */
Datum BOX3D_in(PG_FUNCTION_ARGS);
Datum BOX3D_out(PG_FUNCTION_ARGS);
Datum BOX3D_extent_out(PG_FUNCTION_ARGS);
Datum BOX3D_extent_to_BOX3D(PG_FUNCTION_ARGS);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS);
Datum BOX3D_to_LWGEOM(PG_FUNCTION_ARGS);
Datum BOX3D_expand(PG_FUNCTION_ARGS);
Datum BOX3D_to_BOX2DFLOAT4(PG_FUNCTION_ARGS);
Datum BOX3D_to_BOX(PG_FUNCTION_ARGS);
Datum BOX3D_xmin(PG_FUNCTION_ARGS);
Datum BOX3D_ymin(PG_FUNCTION_ARGS);
Datum BOX3D_zmin(PG_FUNCTION_ARGS);
Datum BOX3D_xmax(PG_FUNCTION_ARGS);
Datum BOX3D_ymax(PG_FUNCTION_ARGS);
Datum BOX3D_zmax(PG_FUNCTION_ARGS);
Datum BOX3D_combine(PG_FUNCTION_ARGS);

/**
 *  BOX3D_in - takes a string rep of BOX3D and returns internal rep
 *
 *  example:
 *     "BOX3D(x1 y1 z1,x2 y2 z2)"
 * or  "BOX3D(x1 y1,x2 y2)"   z1 and z2 = 0.0
 *
 *
 */

PG_FUNCTION_INFO_V1(BOX3D_in);
Datum BOX3D_in(PG_FUNCTION_ARGS)
{
	char *str = PG_GETARG_CSTRING(0);
	int nitems;
	BOX3D *box = (BOX3D *) palloc(sizeof(BOX3D));
	box->zmin = 0;
	box->zmax = 0;


	/*printf( "box3d_in gets '%s'\n",str); */

	if (strstr(str,"BOX3D(") !=  str )
	{
		pfree(box);
		elog(ERROR,"BOX3D parser - doesnt start with BOX3D(");
		PG_RETURN_NULL();
	}

	nitems = sscanf(str,"BOX3D(%le %le %le ,%le %le %le)",
	                &box->xmin, &box->ymin, &box->zmin,
	                &box->xmax, &box->ymax, &box->zmax);
	if (nitems != 6 )
	{
		nitems = sscanf(str,"BOX3D(%le %le ,%le %le)",
		                &box->xmin, &box->ymin, &box->xmax, &box->ymax);
		if (nitems != 4)
		{
			pfree(box);
			elog(ERROR,"BOX3D parser - couldnt parse.  It should look like: BOX3D(xmin ymin zmin,xmax ymax zmax) or BOX3D(xmin ymin,xmax ymax)");
			PG_RETURN_NULL();
		}
	}

	if (box->xmin > box->xmax)
	{
		float tmp = box->xmin;
		box->xmin = box->xmax;
		box->xmax = tmp;
	}
	if (box->ymin > box->ymax)
	{
		float tmp = box->ymin;
		box->ymin = box->ymax;
		box->ymax = tmp;
	}
	if (box->zmin > box->zmax)
	{
		float tmp = box->zmin;
		box->zmin = box->zmax;
		box->zmax = tmp;
	}
	PG_RETURN_POINTER(box);
}


/**
 *  Takes an internal rep of a BOX3D and returns a string rep.
 *
 *  example:
 *     "BOX3D(xmin ymin zmin, xmin ymin zmin)"
 */
PG_FUNCTION_INFO_V1(BOX3D_out);
Datum BOX3D_out(PG_FUNCTION_ARGS)
{
	BOX3D  *bbox = (BOX3D *) PG_GETARG_POINTER(0);
	int size;
	char *result;

	if (bbox == NULL)
	{
		result = palloc(5);
		strcat(result,"NULL");
		PG_RETURN_CSTRING(result);
	}


	/*double digits+ "BOX3D"+ "()" + commas +null */
	size = MAX_DIGS_DOUBLE*6+5+2+4+5+1;

	result = (char *) palloc(size);

	sprintf(result, "BOX3D(%.15g %.15g %.15g,%.15g %.15g %.15g)",
	        bbox->xmin, bbox->ymin, bbox->zmin,
	        bbox->xmax,bbox->ymax,bbox->zmax);

	PG_RETURN_CSTRING(result);
}

/**
 *  Takes an internal rep of a BOX3D and returns a string rep.
 *  but beginning with BOX(...) and with only 2 dimensions. This
 *  is a temporary hack to allow ST_Extent() to return a result
 *  with the precision of BOX2DFLOAT4 but with the BOX2DFLOAT4
 *  output format.
 *
 *  example:
 *     "BOX(xmin ymin, xmax ymax)"
 */
PG_FUNCTION_INFO_V1(BOX3D_extent_out);
Datum BOX3D_extent_out(PG_FUNCTION_ARGS)
{
	BOX3D  *bbox = (BOX3D *) PG_GETARG_POINTER(0);
	int size;
	char *result;

	if (bbox == NULL)
	{
		result = palloc(5);
		strcat(result,"NULL");
		PG_RETURN_CSTRING(result);
	}


	/*double digits+ "BOX3D"+ "()" + commas +null */
	size = MAX_DIGS_DOUBLE*6+5+2+4+5+1;

	result = (char *) palloc(size);

	sprintf(result, "BOX(%.15g %.15g,%.15g %.15g)",
	        bbox->xmin, bbox->ymin,
	        bbox->xmax,bbox->ymax);

	PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(BOX3D_extent_to_BOX3D);
Datum BOX3D_extent_to_BOX3D(PG_FUNCTION_ARGS)
{
	BOX3D *in = (BOX3D *)PG_GETARG_POINTER(0);
	BOX3D *out = palloc(sizeof(BOX3D));
	out->xmin = in->xmin;
	out->xmax = in->xmax;
	out->ymin = in->ymin;
	out->ymax = in->ymax;

	PG_RETURN_POINTER(out);
}

PG_FUNCTION_INFO_V1(BOX3D_to_BOX2DFLOAT4);
Datum BOX3D_to_BOX2DFLOAT4(PG_FUNCTION_ARGS)
{
	BOX3D *in = (BOX3D *)PG_GETARG_POINTER(0);
	BOX2DFLOAT4 *out = box3d_to_box2df(in);
	PG_RETURN_POINTER(out);
}

PG_FUNCTION_INFO_V1(BOX3D_to_BOX);
Datum BOX3D_to_BOX(PG_FUNCTION_ARGS)
{
	BOX3D *in = (BOX3D *)PG_GETARG_POINTER(0);
	BOX *box = palloc(sizeof(BOX));

	box3d_to_box_p(in, box);
	PG_RETURN_POINTER(box);
}

PG_FUNCTION_INFO_V1(BOX3D_to_LWGEOM);
Datum BOX3D_to_LWGEOM(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	POINTARRAY *pa;
	int wantbbox = 0;
	PG_LWGEOM *result;
	uchar *ser;


	/**
	 * Alter BOX3D cast so that a valid geometry is always
	 * returned depending upon the size of the BOX3D. The
	 * code makes the following assumptions:
	 *     - If the BOX3D is a single point then return a
	 *     POINT geometry
	 *     - If the BOX3D represents either a horizontal or
	 *     vertical line, return a LINESTRING geometry
	 *     - Otherwise return a POLYGON
	 */

	if (box->xmin == box->xmax &&
	        box->ymin == box->ymax)
	{
		/* Construct and serialize point */
		LWPOINT *point = make_lwpoint2d(-1, box->xmin, box->ymin);
		ser = lwpoint_serialize(point);
	}
	else if (box->xmin == box->xmax ||
	         box->ymin == box->ymax)
	{
		LWLINE *line;
		POINT2D *pts = palloc(sizeof(POINT2D)*2);

		/* Assign coordinates to POINT2D array */
		pts[0].x = box->xmin;
		pts[0].y = box->ymin;
		pts[1].x = box->xmax;
		pts[1].y = box->ymax;

		/* Construct point array */
		pa = pointArray_construct((uchar *)pts, 0, 0, 2);

		/* Construct and serialize linestring */
		line = lwline_construct(-1, NULL, pa);
		ser = lwline_serialize(line);
	}
	else
	{
		LWPOLY *poly;
		POINT2D *pts = palloc(sizeof(POINT2D)*5);

		/* Assign coordinates to POINT2D array */
		pts[0].x = box->xmin;
		pts[0].y = box->ymin;
		pts[1].x = box->xmin;
		pts[1].y = box->ymax;
		pts[2].x = box->xmax;
		pts[2].y = box->ymax;
		pts[3].x = box->xmax;
		pts[3].y = box->ymin;
		pts[4].x = box->xmin;
		pts[4].y = box->ymin;

		/* Construct point array */
		pa = pointArray_construct((uchar *)pts, 0, 0, 5);

		/* Construct polygon */
		poly = lwpoly_construct(-1, NULL, 1, &pa);

		/* Serialize polygon */
		ser = lwpoly_serialize(poly);
	}

	/* Construct PG_LWGEOM  */
	result = PG_LWGEOM_construct(ser, -1, wantbbox);

	PG_RETURN_POINTER(result);
}

/** Expand given box of 'd' units in all directions */
void
expand_box3d(BOX3D *box, double d)
{
	box->xmin -= d;
	box->ymin -= d;
	box->zmin -= d;

	box->xmax += d;
	box->ymax += d;
	box->zmax += d;
}

PG_FUNCTION_INFO_V1(BOX3D_expand);
Datum BOX3D_expand(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	double d = PG_GETARG_FLOAT8(1);
	BOX3D *result = (BOX3D *)palloc(sizeof(BOX3D));

	memcpy(result, box, sizeof(BOX3D));
	expand_box3d(result, d);

	PG_RETURN_POINTER(result);
}

/**
 * convert a PG_LWGEOM to BOX3D
 *
 * NOTE: the bounding box is *always* recomputed as the cache
 * is a box2d, not a box3d...
 *
 */
PG_FUNCTION_INFO_V1(LWGEOM_to_BOX3D);
Datum LWGEOM_to_BOX3D(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	BOX3D *result;

	result = compute_serialized_box3d(SERIALIZED_FORM(lwgeom));
	if ( ! result ) PG_RETURN_NULL();

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX3D_xmin);
Datum BOX3D_xmin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(LWGEOM_Mind(box->xmin, box->xmax));
}

PG_FUNCTION_INFO_V1(BOX3D_ymin);
Datum BOX3D_ymin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(LWGEOM_Mind(box->ymin, box->ymax));
}

PG_FUNCTION_INFO_V1(BOX3D_zmin);
Datum BOX3D_zmin(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(LWGEOM_Mind(box->zmin, box->zmax));
}

PG_FUNCTION_INFO_V1(BOX3D_xmax);
Datum BOX3D_xmax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(LWGEOM_Maxd(box->xmin, box->xmax));
}

PG_FUNCTION_INFO_V1(BOX3D_ymax);
Datum BOX3D_ymax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(LWGEOM_Maxd(box->ymin, box->ymax));
}

PG_FUNCTION_INFO_V1(BOX3D_zmax);
Datum BOX3D_zmax(PG_FUNCTION_ARGS)
{
	BOX3D *box = (BOX3D *)PG_GETARG_POINTER(0);
	PG_RETURN_FLOAT8(LWGEOM_Maxd(box->zmin, box->zmax));
}


PG_FUNCTION_INFO_V1(BOX3D_combine);
Datum BOX3D_combine(PG_FUNCTION_ARGS)
{
	Pointer box3d_ptr = PG_GETARG_POINTER(0);
	Pointer geom_ptr = PG_GETARG_POINTER(1);
	BOX3D *a,*b;
	PG_LWGEOM *lwgeom;
	BOX3D *box, *result;

	if  ( (box3d_ptr == NULL) && (geom_ptr == NULL) )
	{
		PG_RETURN_NULL();
	}

	result = (BOX3D *)palloc(sizeof(BOX3D));

	if (box3d_ptr == NULL)
	{
		lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
		box = compute_serialized_box3d(SERIALIZED_FORM(lwgeom));
		if ( ! box )
		{
			PG_FREE_IF_COPY(lwgeom, 1);
			PG_RETURN_NULL(); /* must be the empty geom */
		}
		memcpy(result, box, sizeof(BOX3D));
		PG_RETURN_POINTER(result);
	}

	/* combine_bbox(BOX3D, null) => BOX3D */
	if (geom_ptr == NULL)
	{
		memcpy(result, (char *)PG_GETARG_DATUM(0), sizeof(BOX3D));
		PG_RETURN_POINTER(result);
	}

	lwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	box = compute_serialized_box3d(SERIALIZED_FORM(lwgeom));
	if ( ! box ) /* must be the empty geom */
	{
		PG_FREE_IF_COPY(lwgeom, 1);
		memcpy(result, (char *)PG_GETARG_DATUM(0), sizeof(BOX3D));
		PG_RETURN_POINTER(result);
	}

	a = (BOX3D *)PG_GETARG_DATUM(0);
	b = box;

	result->xmax = LWGEOM_Maxd(a->xmax, b->xmax);
	result->ymax = LWGEOM_Maxd(a->ymax, b->ymax);
	result->zmax = LWGEOM_Maxd(a->zmax, b->zmax);
	result->xmin = LWGEOM_Mind(a->xmin, b->xmin);
	result->ymin = LWGEOM_Mind(a->ymin, b->ymin);
	result->zmin = LWGEOM_Mind(a->zmin, b->zmin);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(BOX3D_construct);
Datum BOX3D_construct(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *min = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	PG_LWGEOM *max = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	BOX3D *result = palloc(sizeof(BOX3D));
	LWGEOM *minpoint, *maxpoint;
	POINT3DZ minp, maxp;

	minpoint = lwgeom_deserialize(SERIALIZED_FORM(min));
	maxpoint = lwgeom_deserialize(SERIALIZED_FORM(max));

	if ( TYPE_GETTYPE(minpoint->type) != POINTTYPE ||
	        TYPE_GETTYPE(maxpoint->type) != POINTTYPE )
	{
		elog(ERROR, "BOX3D_construct: args must be points");
		PG_RETURN_NULL();
	}

	errorIfSRIDMismatch(minpoint->SRID, maxpoint->SRID);

	getPoint3dz_p(((LWPOINT *)minpoint)->point, 0, &minp);
	getPoint3dz_p(((LWPOINT *)maxpoint)->point, 0, &maxp);

	result->xmax = maxp.x;
	result->ymax = maxp.y;
	result->zmax = maxp.z;

	result->xmin = minp.x;
	result->ymin = minp.y;
	result->zmin = minp.z;

	PG_RETURN_POINTER(result);
}

/** min(a,b) */
double
LWGEOM_Mind(double a, double b)
{
	if (a<b)
		return a;
	return b;
}

/** max(a,b) */
double
LWGEOM_Maxd(double a, double b)
{
	if (b>a)
		return b;
	return a;
}

