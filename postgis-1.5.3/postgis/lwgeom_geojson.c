/**********************************************************************
 * $Id: lwgeom_geojson.c 5310 2010-02-22 22:03:25Z colivier $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/
/** @file
* GeoJSON output routines.
* Originally written by Olivier Courtin (Camptocamp)
*
**********************************************************************/

#include "postgres.h"

#include "lwgeom_pg.h"
#include "liblwgeom.h"
#include "lwgeom_export.h"

Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS);

static char *asgeojson_point(LWPOINT *point, char *srs, BOX3D *bbox, int precision);
static char *asgeojson_line(LWLINE *line, char *srs, BOX3D *bbox, int precision);
static char *asgeojson_poly(LWPOLY *poly, char *srs, BOX3D *bbox, int precision);
static char * asgeojson_multipoint(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision);
static char * asgeojson_multiline(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision);
static char * asgeojson_multipolygon(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision);
static char * asgeojson_collection(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision);
static size_t asgeojson_inspected_size(LWGEOM_INSPECTED *insp, BOX3D *bbox, int precision);
static size_t asgeojson_inspected_buf(LWGEOM_INSPECTED *insp, char *output, BOX3D *bbox, int precision);

static size_t pointArray_to_geojson(POINTARRAY *pa, char *buf, int precision);
static size_t pointArray_geojson_size(POINTARRAY *pa, int precision);


/**
 * Encode Feature in GeoJson
 */
PG_FUNCTION_INFO_V1(LWGEOM_asGeoJson);
Datum LWGEOM_asGeoJson(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	char *geojson;
	text *result;
	int SRID;
	int len;
	int version;
	int option = 0;
	bool has_bbox = 0;
	int precision = MAX_DOUBLE_PRECISION;
	char * srs = NULL;

	/* Get the version */
	version = PG_GETARG_INT32(0);
	if ( version != 1)
	{
		elog(ERROR, "Only GeoJSON 1 is supported");
		PG_RETURN_NULL();
	}

	/* Get the geometry */
	if (PG_ARGISNULL(1) ) PG_RETURN_NULL();
	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	/* Retrieve precision if any (default is max) */
	if (PG_NARGS() >2 && !PG_ARGISNULL(2))
	{
		precision = PG_GETARG_INT32(2);
		if ( precision > MAX_DOUBLE_PRECISION )
			precision = MAX_DOUBLE_PRECISION;
		else if ( precision < 0 ) precision = 0;
	}

	/* Retrieve output option
	 * 0 = without option (default)
	 * 1 = bbox
	 * 2 = short crs
	 * 4 = long crs
	 */
	if (PG_NARGS() >3 && !PG_ARGISNULL(3))
		option = PG_GETARG_INT32(3);

	if (option & 2 || option & 4)
	{
		SRID = lwgeom_getsrid(SERIALIZED_FORM(geom));
		if ( SRID != -1 )
		{
			if (option & 2) srs = getSRSbySRID(SRID, true);
			if (option & 4) srs = getSRSbySRID(SRID, false);
			if (!srs)
			{
				elog(ERROR, "SRID %i unknown in spatial_ref_sys table", SRID);
				PG_RETURN_NULL();
			}
		}
	}

	if (option & 1) has_bbox = 1;

	geojson = geometry_to_geojson(SERIALIZED_FORM(geom), srs, has_bbox, precision);
	PG_FREE_IF_COPY(geom, 1);
	if (srs) pfree(srs);

	len = strlen(geojson) + VARHDRSZ;
	result = palloc(len);
	SET_VARSIZE(result, len);
	memcpy(VARDATA(result), geojson, len-VARHDRSZ);

	pfree(geojson);

	PG_RETURN_POINTER(result);
}


/**
 * Takes a GEOMETRY and returns a GeoJson representation
 */
char *
geometry_to_geojson(uchar *geom, char *srs, bool has_bbox, int precision)
{
	int type;
	LWPOINT *point;
	LWLINE *line;
	LWPOLY *poly;
	LWGEOM_INSPECTED *insp;
	BOX3D * bbox = NULL;
	char * ret = NULL;

	type = lwgeom_getType(geom[0]);

	if (has_bbox) bbox = compute_serialized_box3d(geom);

	switch (type)
	{
	case POINTTYPE:
		point = lwpoint_deserialize(geom);
		ret = asgeojson_point(point, srs, bbox, precision);
		break;

	case LINETYPE:
		line = lwline_deserialize(geom);
		ret = asgeojson_line(line, srs, bbox, precision);
		break;

	case POLYGONTYPE:
		poly = lwpoly_deserialize(geom);
		ret = asgeojson_poly(poly, srs, bbox, precision);
		break;

	case MULTIPOINTTYPE:
		insp = lwgeom_inspect(geom);
		ret = asgeojson_multipoint(insp, srs, bbox, precision);
		break;

	case MULTILINETYPE:
		insp = lwgeom_inspect(geom);
		ret = asgeojson_multiline(insp, srs, bbox, precision);
		break;

	case MULTIPOLYGONTYPE:
		insp = lwgeom_inspect(geom);
		ret = asgeojson_multipolygon(insp, srs, bbox, precision);
		break;

	case COLLECTIONTYPE:
		insp = lwgeom_inspect(geom);
		ret = asgeojson_collection(insp, srs, bbox, precision);
		break;

	default:
		if (bbox)
		{
			lwfree(bbox);
			bbox = NULL;
		}
		lwerror("GeoJson: '%s' geometry type not supported.",
		        lwgeom_typename(type));
	}

	if (bbox)
	{
		lwfree(bbox);
		bbox = NULL;
	}

	return ret;
}



/**
 * Handle SRS
 */
static size_t
asgeojson_srs_size(char *srs)
{
	int size;

	size = sizeof("'crs':{'type':'name',");
	size += sizeof("'properties':{'name':''}},");
	size += strlen(srs) * sizeof(char);

	return size;
}

static size_t
asgeojson_srs_buf(char *output, char *srs)
{
	char *ptr = output;

	ptr += sprintf(ptr, "\"crs\":{\"type\":\"name\",");
	ptr += sprintf(ptr, "\"properties\":{\"name\":\"%s\"}},", srs);

	return (ptr-output);
}



/**
 * Handle Bbox
 */
static size_t
asgeojson_bbox_size(bool hasz, int precision)
{
	int size;

	if (!hasz)
	{
		size = sizeof("\"bbox\":[,,,],");
		size +=	2 * 2 * (MAX_DIGS_DOUBLE + precision);
	}
	else
	{
		size = sizeof("\"bbox\":[,,,,,],");
		size +=	2 * 3 * (MAX_DIGS_DOUBLE + precision);
	}

	return size;
}

static size_t
asgeojson_bbox_buf(char *output, BOX3D *bbox, bool hasz, int precision)
{
	char *ptr = output;

	if (!hasz)
		ptr += sprintf(ptr, "\"bbox\":[%.*f,%.*f,%.*f,%.*f],",
		               precision, bbox->xmin, precision, bbox->ymin,
		               precision, bbox->xmax, precision, bbox->ymax);
	else
		ptr += sprintf(ptr, "\"bbox\":[%.*f,%.*f,%.*f,%.*f,%.*f,%.*f],",
		               precision, bbox->xmin, precision, bbox->ymin, precision, bbox->zmin,
		               precision, bbox->xmax, precision, bbox->ymax, precision, bbox->zmax);

	return (ptr-output);
}



/**
 * Point Geometry
 */

static size_t
asgeojson_point_size(LWPOINT *point, char *srs, BOX3D *bbox, int precision)
{
	int size;

	size = pointArray_geojson_size(point->point, precision);
	size += sizeof("{'type':'Point',");
	size += sizeof("'coordinates':}");

	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(TYPE_HASZ(point->type), precision);

	return size;
}

static size_t
asgeojson_point_buf(LWPOINT *point, char *srs, char *output, BOX3D *bbox, int precision)
{
	char *ptr = output;

	ptr += sprintf(ptr, "{\"type\":\"Point\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, TYPE_HASZ(point->type), precision);

	ptr += sprintf(ptr, "\"coordinates\":");
	ptr += pointArray_to_geojson(point->point, ptr, precision);
	ptr += sprintf(ptr, "}");

	return (ptr-output);
}

static char *
asgeojson_point(LWPOINT *point, char *srs, BOX3D *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_point_size(point, srs, bbox, precision);
	output = palloc(size);
	asgeojson_point_buf(point, srs, output, bbox, precision);
	return output;
}



/**
 * Line Geometry
 */

static size_t
asgeojson_line_size(LWLINE *line, char *srs, BOX3D *bbox, int precision)
{
	int size;

	size = sizeof("{'type':'LineString',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(TYPE_HASZ(line->type), precision);
	size += sizeof("'coordinates':[]}");
	size += pointArray_geojson_size(line->points, precision);

	return size;
}

static size_t
asgeojson_line_buf(LWLINE *line, char *srs, char *output, BOX3D *bbox, int precision)
{
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"LineString\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, TYPE_HASZ(line->type), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");
	ptr += pointArray_to_geojson(line->points, ptr, precision);
	ptr += sprintf(ptr, "]}");

	return (ptr-output);
}

static char *
asgeojson_line(LWLINE *line, char *srs, BOX3D *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_line_size(line, srs, bbox, precision);
	output = palloc(size);
	asgeojson_line_buf(line, srs, output, bbox, precision);

	return output;
}



/**
 * Polygon Geometry
 */

static size_t
asgeojson_poly_size(LWPOLY *poly, char *srs, BOX3D *bbox, int precision)
{
	size_t size;
	int i;

	size = sizeof("{\"type\":\"Polygon\",");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(TYPE_HASZ(poly->type), precision);
	size += sizeof("\"coordinates\":[");
	for (i=0, size=0; i<poly->nrings; i++)
	{
		size += pointArray_geojson_size(poly->rings[i], precision);
		size += sizeof("[]");
	}
	size += sizeof(",") * i;
	size += sizeof("]}");

	return size;
}

static size_t
asgeojson_poly_buf(LWPOLY *poly, char *srs, char *output, BOX3D *bbox, int precision)
{
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"Polygon\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, TYPE_HASZ(poly->type), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");
	for (i=0; i<poly->nrings; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		ptr += sprintf(ptr, "[");
		ptr += pointArray_to_geojson(poly->rings[i], ptr, precision);
		ptr += sprintf(ptr, "]");
	}
	ptr += sprintf(ptr, "]}");

	return (ptr-output);
}

static char *
asgeojson_poly(LWPOLY *poly, char *srs, BOX3D *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_poly_size(poly, srs, bbox, precision);
	output = palloc(size);
	asgeojson_poly_buf(poly, srs, output, bbox, precision);

	return output;
}



/**
 * Multipoint Geometry
 */

static size_t
asgeojson_multipoint_size(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision)
{
	LWPOINT * point;
	int size;
	int i;

	size = sizeof("{'type':'MultiPoint',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(TYPE_HASZ(insp->type), precision);
	size += sizeof("'coordinates':[]}");

	for (i=0; i<insp->ngeometries; i++)
	{
		point = lwgeom_getpoint_inspected(insp, i);
		size += pointArray_geojson_size(point->point, precision);
		lwpoint_release(point);
	}
	size += sizeof(",") * i;

	return size;
}

static size_t
asgeojson_multipoint_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, BOX3D *bbox, int precision)
{
	LWPOINT *point;
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"MultiPoint\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, TYPE_HASZ(insp->type), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");

	for (i=0; i<insp->ngeometries; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		point=lwgeom_getpoint_inspected(insp, i);
		ptr += pointArray_to_geojson(point->point, ptr, precision);
		lwpoint_release(point);
	}
	ptr += sprintf(ptr, "]}");

	return (ptr - output);
}

static char *
asgeojson_multipoint(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_multipoint_size(insp, srs, bbox, precision);
	output = palloc(size);
	asgeojson_multipoint_buf(insp, srs, output, bbox, precision);

	return output;
}



/**
 * Multiline Geometry
 */

static size_t
asgeojson_multiline_size(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision)
{
	LWLINE * line;
	int size;
	int i;

	size = sizeof("{'type':'MultiLineString',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(TYPE_HASZ(insp->type), precision);
	size += sizeof("'coordinates':[]}");

	for (i=0 ; i<insp->ngeometries; i++)
	{
		line = lwgeom_getline_inspected(insp, i);
		size += pointArray_geojson_size(line->points, precision);
		size += sizeof("[]");
		lwline_release(line);
	}
	size += sizeof(",") * i;

	return size;
}

static size_t
asgeojson_multiline_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, BOX3D *bbox, int precision)
{
	LWLINE *line;
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"MultiLineString\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, TYPE_HASZ(insp->type), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");

	for (i=0; i<insp->ngeometries; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		ptr += sprintf(ptr, "[");
		line = lwgeom_getline_inspected(insp, i);
		ptr += pointArray_to_geojson(line->points, ptr, precision);
		ptr += sprintf(ptr, "]");

		lwline_release(line);
	}

	ptr += sprintf(ptr, "]}");

	return (ptr - output);
}

static char *
asgeojson_multiline(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_multiline_size(insp, srs, bbox, precision);
	output = palloc(size);
	asgeojson_multiline_buf(insp, srs, output, bbox, precision);

	return output;
}



/**
 * MultiPolygon Geometry
 */

static size_t
asgeojson_multipolygon_size(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision)
{
	LWPOLY *poly;
	int size;
	int i, j;

	size = sizeof("{'type':'MultiPolygon',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(TYPE_HASZ(insp->type), precision);
	size += sizeof("'coordinates':[]}");

	for (i=0; i < insp->ngeometries; i++)
	{
		poly = lwgeom_getpoly_inspected(insp, i);
		for (j=0 ; j <poly->nrings ; j++)
		{
			size += pointArray_geojson_size(poly->rings[j], precision);
			size += sizeof("[]");
		}
		lwpoly_release(poly);
		size += sizeof("[]");
	}
	size += sizeof(",") * i;
	size += sizeof("]}");

	return size;
}

static size_t
asgeojson_multipolygon_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, BOX3D *bbox, int precision)
{
	LWPOLY *poly;
	int i, j;
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"MultiPolygon\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, TYPE_HASZ(insp->type), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");
	for (i=0; i<insp->ngeometries; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		ptr += sprintf(ptr, "[");
		poly = lwgeom_getpoly_inspected(insp, i);
		for (j=0 ; j < poly->nrings ; j++)
		{
			if (j) ptr += sprintf(ptr, ",");
			ptr += sprintf(ptr, "[");
			ptr += pointArray_to_geojson(poly->rings[j], ptr, precision);
			ptr += sprintf(ptr, "]");
		}
		ptr += sprintf(ptr, "]");
		lwpoly_release(poly);
	}
	ptr += sprintf(ptr, "]}");

	return (ptr - output);
}

static char *
asgeojson_multipolygon(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_multipolygon_size(insp, srs, bbox, precision);
	output = palloc(size);
	asgeojson_multipolygon_buf(insp, srs, output, bbox, precision);

	return output;
}



/**
 * Collection Geometry
 */

static size_t
asgeojson_collection_size(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision)
{
	int i;
	int size;
	LWGEOM_INSPECTED *subinsp;
	uchar *subgeom;

	size = sizeof("{'type':'GeometryCollection',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(TYPE_HASZ(insp->type), precision);
	size += sizeof("'geometries':");

	for (i=0; i<insp->ngeometries; i++)
	{
		subgeom = lwgeom_getsubgeometry_inspected(insp, i);
		subinsp = lwgeom_inspect(subgeom);
		size += asgeojson_inspected_size(subinsp, NULL, precision);
		lwinspected_release(subinsp);
	}
	size += sizeof(",") * i;
	size += sizeof("]}");

	return size;
}

static size_t
asgeojson_collection_buf(LWGEOM_INSPECTED *insp, char *srs, char *output, BOX3D *bbox, int precision)
{
	int i;
	char *ptr=output;
	LWGEOM_INSPECTED *subinsp;
	uchar *subgeom;

	ptr += sprintf(ptr, "{\"type\":\"GeometryCollection\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, TYPE_HASZ(insp->type), precision);
	ptr += sprintf(ptr, "\"geometries\":[");

	for (i=0; i<insp->ngeometries; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		subgeom = lwgeom_getsubgeometry_inspected(insp, i);
		subinsp = lwgeom_inspect(subgeom);
		ptr += asgeojson_inspected_buf(subinsp, ptr, NULL, precision);
		lwinspected_release(subinsp);
	}

	ptr += sprintf(ptr, "]}");

	return (ptr - output);
}

static char *
asgeojson_collection(LWGEOM_INSPECTED *insp, char *srs, BOX3D *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_collection_size(insp, srs, bbox, precision);
	output = palloc(size);
	asgeojson_collection_buf(insp, srs, output, bbox, precision);

	return output;
}



static size_t
asgeojson_inspected_size(LWGEOM_INSPECTED *insp, BOX3D *bbox, int precision)
{
	int type = lwgeom_getType(insp->serialized_form[0]);
	size_t size = 0;
	LWPOINT *point;
	LWLINE *line;
	LWPOLY *poly;

	switch (type)
	{
	case POINTTYPE:
		point=lwgeom_getpoint_inspected(insp, 0);
		size = asgeojson_point_size(point, NULL, bbox, precision);
		lwpoint_release(point);
		break;

	case LINETYPE:
		line=lwgeom_getline_inspected(insp, 0);
		size = asgeojson_line_size(line, NULL, bbox, precision);
		lwline_release(line);
		break;

	case POLYGONTYPE:
		poly=lwgeom_getpoly_inspected(insp, 0);
		size = asgeojson_poly_size(poly, NULL, bbox, precision);
		lwpoly_release(poly);
		break;

	case MULTIPOINTTYPE:
		size = asgeojson_multipoint_size(insp, NULL, bbox, precision);
		break;

	case MULTILINETYPE:
		size = asgeojson_multiline_size(insp, NULL, bbox, precision);
		break;

	case MULTIPOLYGONTYPE:
		size = asgeojson_multipolygon_size(insp, NULL, bbox, precision);
		break;

	default:
		lwerror("GeoJson: geometry not supported.");
	}

	return size;
}


static size_t
asgeojson_inspected_buf(LWGEOM_INSPECTED *insp, char *output, BOX3D *bbox, int precision)
{
	LWPOINT *point;
	LWLINE *line;
	LWPOLY *poly;
	int type = lwgeom_getType(insp->serialized_form[0]);
	char *ptr=output;

	switch (type)
	{
	case POINTTYPE:
		point=lwgeom_getpoint_inspected(insp, 0);
		ptr += asgeojson_point_buf(point, NULL, ptr, bbox, precision);
		lwpoint_release(point);
		break;

	case LINETYPE:
		line=lwgeom_getline_inspected(insp, 0);
		ptr += asgeojson_line_buf(line, NULL, ptr, bbox, precision);
		lwline_release(line);
		break;

	case POLYGONTYPE:
		poly=lwgeom_getpoly_inspected(insp, 0);
		ptr += asgeojson_poly_buf(poly, NULL, ptr, bbox, precision);
		lwpoly_release(poly);
		break;

	case MULTIPOINTTYPE:
		ptr += asgeojson_multipoint_buf(insp, NULL, ptr, bbox, precision);
		break;

	case MULTILINETYPE:
		ptr += asgeojson_multiline_buf(insp, NULL, ptr, bbox, precision);
		break;

	case MULTIPOLYGONTYPE:
		ptr += asgeojson_multipolygon_buf(insp, NULL, ptr, bbox, precision);
		break;

	default:
		if (bbox) lwfree(bbox);
		lwerror("GeoJson: geometry not supported.");
	}

	return (ptr-output);
}


static size_t
pointArray_to_geojson(POINTARRAY *pa, char *output, int precision)
{
	int i;
	char *ptr;
	char x[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char y[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];
	char z[MAX_DIGS_DOUBLE+MAX_DOUBLE_PRECISION+1];

	ptr = output;

	if (!TYPE_HASZ(pa->dims))
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT2D pt;
			getPoint2d_p(pa, i, &pt);

			if (fabs(pt.x) < MAX_DOUBLE)
				sprintf(x, "%.*f", precision, pt.x);
			else
				sprintf(x, "%g", pt.x);
			trim_trailing_zeros(x);

			if (fabs(pt.y) < MAX_DOUBLE)
				sprintf(y, "%.*f", precision, pt.y);
			else
				sprintf(y, "%g", pt.y);
			trim_trailing_zeros(y);

			if ( i ) ptr += sprintf(ptr, ",");
			ptr += sprintf(ptr, "[%s,%s]", x, y);
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++)
		{
			POINT4D pt;
			getPoint4d_p(pa, i, &pt);

			if (fabs(pt.x) < MAX_DOUBLE)
				sprintf(x, "%.*f", precision, pt.x);
			else
				sprintf(x, "%g", pt.x);
			trim_trailing_zeros(x);

			if (fabs(pt.y) < MAX_DOUBLE)
				sprintf(y, "%.*f", precision, pt.y);
			else
				sprintf(y, "%g", pt.y);
			trim_trailing_zeros(y);

			if (fabs(pt.z) < MAX_DOUBLE)
				sprintf(z, "%.*f", precision, pt.z);
			else
				sprintf(z, "%g", pt.z);
			trim_trailing_zeros(z);

			if ( i ) ptr += sprintf(ptr, ",");
			ptr += sprintf(ptr, "[%s,%s,%s]", x, y, z);
		}
	}

	return (ptr-output);
}



/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_geojson_size(POINTARRAY *pa, int precision)
{
	if (TYPE_NDIMS(pa->dims) == 2)
		return (MAX_DIGS_DOUBLE + precision + sizeof(","))
		       * 2 * pa->npoints + sizeof(",[]");

	return (MAX_DIGS_DOUBLE + precision + sizeof(",,"))
	       * 3 * pa->npoints + sizeof(",[]");
}
