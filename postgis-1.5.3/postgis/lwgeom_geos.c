/**********************************************************************
 * $Id: lwgeom_geos.c 7344 2011-06-09 13:59:42Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 *
 * Copyright 2009 Sandro Santilli <strk@keybit.net>
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "lwgeom_geos.h"
#include "lwgeom_rtree.h"
#include "lwgeom_geos_prepared.h"
#include "lwgeom_functions_analytic.h"

#include <string.h>


/**
** NOTE: Buffer-based GeomUnion has been disabled due to
** limitations in the GEOS code (it would only work against polygons)
** DONE: Implement cascaded GeomUnion and remove old buffer-based code.
*/

/*
** Prototypes for SQL-bound functions
*/
Datum relate_full(PG_FUNCTION_ARGS);
Datum relate_pattern(PG_FUNCTION_ARGS);
Datum disjoint(PG_FUNCTION_ARGS);
Datum touches(PG_FUNCTION_ARGS);
Datum intersects(PG_FUNCTION_ARGS);
Datum crosses(PG_FUNCTION_ARGS);
Datum within(PG_FUNCTION_ARGS);
Datum contains(PG_FUNCTION_ARGS);
Datum containsproperly(PG_FUNCTION_ARGS);
Datum covers(PG_FUNCTION_ARGS);
Datum overlaps(PG_FUNCTION_ARGS);
Datum isvalid(PG_FUNCTION_ARGS);
Datum isvalidreason(PG_FUNCTION_ARGS);
Datum buffer(PG_FUNCTION_ARGS);
Datum intersection(PG_FUNCTION_ARGS);
Datum convexhull(PG_FUNCTION_ARGS);
Datum topologypreservesimplify(PG_FUNCTION_ARGS);
Datum difference(PG_FUNCTION_ARGS);
Datum boundary(PG_FUNCTION_ARGS);
Datum symdifference(PG_FUNCTION_ARGS);
Datum geomunion(PG_FUNCTION_ARGS);
Datum issimple(PG_FUNCTION_ARGS);
Datum isring(PG_FUNCTION_ARGS);
Datum geomequals(PG_FUNCTION_ARGS);
Datum pointonsurface(PG_FUNCTION_ARGS);
Datum GEOSnoop(PG_FUNCTION_ARGS);
Datum postgis_geos_version(PG_FUNCTION_ARGS);
Datum centroid(PG_FUNCTION_ARGS);
Datum polygonize_garray(PG_FUNCTION_ARGS);
Datum LWGEOM_buildarea(PG_FUNCTION_ARGS); /* TODO: rename to match others
*/
Datum linemerge(PG_FUNCTION_ARGS);
Datum coveredby(PG_FUNCTION_ARGS);
Datum hausdorffdistance(PG_FUNCTION_ARGS);
Datum hausdorffdistancedensify(PG_FUNCTION_ARGS);

Datum pgis_union_geometry_array_old(PG_FUNCTION_ARGS);
Datum pgis_union_geometry_array(PG_FUNCTION_ARGS);


/*
** Prototypes end
*/


PG_FUNCTION_INFO_V1(postgis_geos_version);
Datum postgis_geos_version(PG_FUNCTION_ARGS)
{
	const char *ver = GEOSversion();
	text *result;
	result = (text *) palloc(VARHDRSZ  + strlen(ver));
	SET_VARSIZE(result, VARHDRSZ + strlen(ver));
	memcpy(VARDATA(result), ver, strlen(ver));
	PG_RETURN_POINTER(result);
}


#if POSTGIS_GEOS_VERSION >= 32

/**
 *  @brief Compute the Hausdorff distance thanks to the corresponding GEOS function
 *  @example hausdorffdistance {@link #hausdorffdistance} - SELECT st_hausdorffdistance(
 *      'POLYGON((0 0, 0 2, 1 2, 2 2, 2 0, 0 0))'::geometry,
 *      'POLYGON((0.5 0.5, 0.5 2.5, 1.5 2.5, 2.5 2.5, 2.5 0.5, 0.5 0.5))'::geometry);
 */

PG_FUNCTION_INFO_V1(hausdorffdistance);
Datum hausdorffdistance(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1;
	GEOSGeometry *g2;
	double result;
	int retcode;

	POSTGIS_DEBUG(2, "hausdorff_distance called");

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	initGEOS(lwnotice, lwnotice);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	retcode = GEOSHausdorffDistance(g1, g2, &result);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (retcode == 0)
	{
		elog(ERROR,"GEOS HausdorffDistance() threw an error!");
		PG_RETURN_NULL(); /*never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(result);
}

/**
 *  @brief Compute the Hausdorff distance with densification thanks to the corresponding GEOS function
 *  @example hausdorffdistancedensify {@link #hausdorffdistancedensify} - SELECT st_hausdorffdistancedensify(
 *      'POLYGON((0 0, 0 2, 1 2, 2 2, 2 0, 0 0))'::geometry,
 *      'POLYGON((0.5 0.5, 0.5 2.5, 1.5 2.5, 2.5 2.5, 2.5 0.5, 0.5 0.5))'::geometry, 0.5);
 */

PG_FUNCTION_INFO_V1(hausdorffdistancedensify);
Datum hausdorffdistancedensify(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1;
	GEOSGeometry *g2;
	double densifyFrac;
	double result;
	int retcode;


	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));
	densifyFrac = PG_GETARG_FLOAT8(2);

	initGEOS(lwnotice, lwnotice);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	retcode = GEOSHausdorffDistanceDensify(g1, g2, densifyFrac, &result);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (retcode == 0)
	{
		elog(ERROR,"GEOS HausdorffDistanceDensify() threw an error!");
		PG_RETURN_NULL(); /*never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_FLOAT8(result);
}

#endif


/**
 * @brief This is the final function for GeomUnion
 * 			aggregate. Will have as input an array of Geometries.
 * 			Will iteratively call GEOSUnion on the GEOS-converted
 * 			versions of them and return PGIS-converted version back.
 * 			Changing combination order *might* speed up performance.
 */
PG_FUNCTION_INFO_V1(pgis_union_geometry_array);
Datum pgis_union_geometry_array(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	int nelems, i;
	PG_LWGEOM *result = NULL;
	PG_LWGEOM *pgis_geom = NULL;
	const GEOSGeometry * g1 = NULL;
	GEOSGeometry * g2 = NULL;
	GEOSGeometry * geos_result=NULL;
	int SRID=-1;
	size_t offset = 0;
	bits8 *bitmap;
	int bitmask;
#if POSTGIS_DEBUG_LEVEL > 0
	static int call=1;
#endif
	int gotsrid = 0;
	int allpolys=1;

#if POSTGIS_DEBUG_LEVEL > 0
	call++;
	POSTGIS_DEBUGF(2, "GEOS incremental union (call %d)", call);
#endif

	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = DatumGetArrayTypeP(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	bitmap = ARR_NULLBITMAP(array);

	POSTGIS_DEBUGF(3, "unite_garray: number of elements: %d", nelems);

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* One-element union is the element itself */
	if ( nelems == 1 )
	{
		/* If the element is a NULL then we need to handle it separately */
		if (bitmap && (*bitmap & 1) == 0)
			PG_RETURN_NULL();
		else
			PG_RETURN_POINTER((PG_LWGEOM *)(ARR_DATA_PTR(array)));
	}

	/* Ok, we really need geos now ;) */
	initGEOS(lwnotice, lwnotice);

	/*
	** First, see if all our elements are POLYGON/MULTIPOLYGON
	** If they are, we can use UnionCascaded for faster results.
	*/
	offset = 0;
	bitmask = 1;
	gotsrid = 0;
	for ( i = 0; i < nelems; i++ )
	{
		/* Don't do anything for NULL values */
		if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap)
		{
			PG_LWGEOM *pggeom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
			int pgtype = TYPE_GETTYPE(pggeom->type);
			offset += INTALIGN(VARSIZE(pggeom));
			if ( ! gotsrid ) /* Initialize SRID */
			{
				SRID = pglwgeom_getSRID(pggeom);
				gotsrid = 1;
				if ( TYPE_HASZ(pggeom->type) ) is3d = 1;
			}
			else
			{
				errorIfSRIDMismatch(SRID, pglwgeom_getSRID(pggeom));
			}

			if ( pgtype != POLYGONTYPE && pgtype != MULTIPOLYGONTYPE )
			{
				allpolys = 0;
				break;
			}
		}

		/* Advance NULL bitmap */
		if (bitmap)
		{
			bitmask <<= 1;
			if (bitmask == 0x100)
			{
				bitmap++;
				bitmask = 1;
			}
		}
	}

	if ( allpolys )
	{
		/*
		** Everything is polygonal, so let's run the cascaded polygon union!
		*/
		int geoms_size = nelems;
		int curgeom = 0;
		GEOSGeometry **geoms = NULL;
		geoms = palloc( sizeof(GEOSGeometry *) * geoms_size );
		/*
		** We need to convert the array of PG_LWGEOM into a GEOS MultiPolygon.
		** First make an array of GEOS Polygons.
		*/
		offset = 0;
		bitmap = ARR_NULLBITMAP(array);
		bitmask = 1;
		for ( i = 0; i < nelems; i++ )
		{
			/* Don't do anything for NULL values */
			if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap)
			{
				PG_LWGEOM *pggeom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
				int pgtype = TYPE_GETTYPE(pggeom->type);
				offset += INTALIGN(VARSIZE(pggeom));
				if ( pgtype == POLYGONTYPE )
				{
					if ( curgeom == geoms_size )
					{
						geoms_size *= 2;
						geoms = repalloc( geoms, sizeof(GEOSGeom) * geoms_size );
					}
					geoms[curgeom] = (GEOSGeometry *)POSTGIS2GEOS(pggeom);
					curgeom++;
				}
				if ( pgtype == MULTIPOLYGONTYPE )
				{
					int j = 0;
					LWGEOM_INSPECTED *lwgeom = lwgeom_inspect(SERIALIZED_FORM(pggeom));
					for ( j = 0; j < lwgeom->ngeometries; j++ )
					{
						LWPOLY *lwpoly = NULL;
						int k = 0;
						if ( curgeom == geoms_size )
						{
							geoms_size *= 2;
							geoms = repalloc( geoms, sizeof(GEOSGeom) * geoms_size );
						}
						/* This builds a LWPOLY on top of the serialized form */
						lwpoly = lwgeom_getpoly_inspected(lwgeom, j);
						geoms[curgeom] = LWGEOM2GEOS(lwpoly_as_lwgeom(lwpoly));
						/* We delicately free the LWPOLY and POINTARRAY structs, leaving the serialized form below untouched. */
						for ( k = 0; k < lwpoly->nrings; k++ )
						{
							lwfree(lwpoly->rings[k]);
						}
						lwpoly_release(lwpoly);
						curgeom++;
					}
				}
			}

			/* Advance NULL bitmap */
			if (bitmap)
			{
				bitmask <<= 1;
				if (bitmask == 0x100)
				{
					bitmap++;
					bitmask = 1;
				}
			}
		}
		/*
		** Take our GEOS Polygons and turn them into a GEOS MultiPolygon,
		** then pass that into cascaded union.
		*/
		if (curgeom > 0)
		{
			g1 = GEOSGeom_createCollection(GEOS_MULTIPOLYGON, geoms, curgeom);
			if ( g1 ) g2 = GEOSUnionCascaded(g1);
			if ( g2 ) GEOSSetSRID(g2, SRID);
			if ( g2 ) result = GEOS2POSTGIS(g2, is3d);
			/* Clean up the mess. */
			if ( g1 ) GEOSGeom_destroy((GEOSGeometry *)g1);
			if ( g2 ) GEOSGeom_destroy(g2);
		}
		else
		{
			/* All we found were NULLs, so let's return NULL */
			PG_RETURN_NULL();
		}
	}
	else
	{
		/*
		** Heterogeneous result, let's slog through this one union at a time.
		*/
		offset = 0;
		bitmap = ARR_NULLBITMAP(array);
		bitmask = 1;
		for (i=0; i<nelems; i++)
		{
			/* Don't do anything for NULL values */
			if ((bitmap && (*bitmap & bitmask) != 0) || !bitmap)
			{
				PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
				offset += INTALIGN(VARSIZE(geom));

				pgis_geom = geom;

				POSTGIS_DEBUGF(3, "geom %d @ %p", i, geom);

				/* Check is3d flag */
				if ( TYPE_HASZ(geom->type) ) is3d = 1;

				/* Check SRID homogeneity and initialize geos result */
				if ( ! geos_result )
				{
					geos_result = (GEOSGeometry *)POSTGIS2GEOS(geom);
					SRID = pglwgeom_getSRID(geom);
					POSTGIS_DEBUGF(3, "first geom is a %s", lwgeom_typename(TYPE_GETTYPE(geom->type)));
				}
				else
				{
					errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom));

					g1 = POSTGIS2GEOS(pgis_geom);

					POSTGIS_DEBUGF(3, "unite_garray(%d): adding geom %d to union (%s)",
					               call, i, lwgeom_typename(TYPE_GETTYPE(geom->type)));

					g2 = GEOSUnion(g1, geos_result);
					if ( g2 == NULL )
					{
						GEOSGeom_destroy((GEOSGeometry *)g1);
						GEOSGeom_destroy((GEOSGeometry *)geos_result);
						elog(ERROR,"GEOS union() threw an error!");
					}
					GEOSGeom_destroy((GEOSGeometry *)g1);
					GEOSGeom_destroy((GEOSGeometry *)geos_result);
					geos_result = g2;
				}
			}

			/* Advance NULL bitmap */
			if (bitmap)
			{
				bitmask <<= 1;
				if (bitmask == 0x100)
				{
					bitmap++;
					bitmask = 1;
				}
			}

		}

		/* If geos_result is set then we found at least one non-NULL geometry */
		if (geos_result)
		{
			GEOSSetSRID(geos_result, SRID);
			result = GEOS2POSTGIS(geos_result, is3d);
			GEOSGeom_destroy(geos_result);
		}
		else
		{
			/* All we found were NULLs, so let's return NULL */
			PG_RETURN_NULL();
		}

	}

	if ( result == NULL )
	{
		/* Union returned a NULL geometry */
		PG_RETURN_NULL();
	}

	PG_RETURN_POINTER(result);

}


/**
 * @example geomunion {@link #geomunion} SELECT geomunion(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *      'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))'
 * );
 *
 */
PG_FUNCTION_INFO_V1(geomunion);
Datum geomunion(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	int is3d;
	int SRID;
	GEOSGeometry *g1, *g2, *g3;
	PG_LWGEOM *result;

	POSTGIS_DEBUG(2, "in geomunion");

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_HASZ(geom1->type) ) ||
	       ( TYPE_HASZ(geom2->type) );

	SRID = pglwgeom_getSRID(geom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	POSTGIS_DEBUGF(3, "g1=%s", GEOSGeomToWKT(g1));
	POSTGIS_DEBUGF(3, "g2=%s", GEOSGeomToWKT(g2));

	PROFSTART(PROF_GRUN);
	g3 = GEOSUnion(g1,g2);
	PROFSTOP(PROF_GRUN);

	POSTGIS_DEBUGF(3, "g3=%s", GEOSGeomToWKT(g3));

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS union() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}


	GEOSSetSRID(g3, SRID);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	GEOSGeom_destroy(g3);

	if (result == NULL)
	{
		elog(ERROR,"GEOS union() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


/**
 *  @example symdifference {@link #symdifference} - SELECT symdifference(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *      'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
 */
PG_FUNCTION_INFO_V1(symdifference);
Datum symdifference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2, *g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_HASZ(geom1->type) ) ||
	       ( TYPE_HASZ(geom2->type) );

	SRID = pglwgeom_getSRID(geom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	g3 = GEOSSymDifference(g1,g2);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS symdifference() threw an error!");
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		PG_RETURN_NULL(); /*never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, SRID);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS symdifference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(boundary);
Datum boundary(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;
	int SRID;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	SRID = pglwgeom_getSRID(geom1);

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1 );
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_GRUN);
	g3 = (GEOSGeometry *)GEOSBoundary(g1);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS boundary() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, SRID);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));

	PROFSTART(PROF_P2G1);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);

		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS boundary() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	/* compressType(result);   */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(convexhull);
Datum convexhull(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;
	LWGEOM *lwout;
	int SRID;
	BOX2DFLOAT4 bbox;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	SRID = pglwgeom_getSRID(geom1);

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_GRUN);
	g3 = (GEOSGeometry *)GEOSConvexHull(g1);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS convexhull() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}


	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, SRID);

	PROFSTART(PROF_G2P);
	lwout = GEOS2LWGEOM(g3, TYPE_HASZ(geom1->type));
	PROFSTOP(PROF_G2P);

	if (lwout == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"convexhull() failed to convert GEOS geometry to LWGEOM");
		PG_RETURN_NULL(); /* never get here */
	}

	/* Copy input bbox if any */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &bbox) )
	{
		lwout->bbox = box2d_clone(&bbox);
	}

	result = pglwgeom_serialize(lwout);
	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS convexhull() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	lwgeom_release(lwout);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	/* compressType(result);   */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(topologypreservesimplify);
Datum topologypreservesimplify(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	double	tolerance;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	tolerance = PG_GETARG_FLOAT8(1);

	initGEOS(lwnotice, lwnotice);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	g3 = GEOSTopologyPreserveSimplify(g1,tolerance);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS topologypreservesimplify() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}


	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));

	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS topologypreservesimplify() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	PG_FREE_IF_COPY(geom1, 0);
	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(buffer);
Datum buffer(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	double	size;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;
	int quadsegs = 8; /* the default */
	int nargs;
	enum
	{
		ENDCAP_ROUND = 1,
		ENDCAP_FLAT = 2,
		ENDCAP_SQUARE = 3
	};
	enum
	{
		JOIN_ROUND = 1,
		JOIN_MITRE = 2,
		JOIN_BEVEL = 3
	};
	static const double DEFAULT_MITRE_LIMIT = 5.0;
	static const int DEFAULT_ENDCAP_STYLE = ENDCAP_ROUND;
	static const int DEFAULT_JOIN_STYLE = JOIN_ROUND;

	double mitreLimit = DEFAULT_MITRE_LIMIT;
	int endCapStyle = DEFAULT_ENDCAP_STYLE;
	int joinStyle  = DEFAULT_JOIN_STYLE;
	char *param;
	char *params = NULL;


	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	size = PG_GETARG_FLOAT8(1);

	nargs = PG_NARGS();

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	if (nargs > 2)
	{
		/* We strdup `cause we're going to modify it */
		params = pstrdup(PG_GETARG_CSTRING(2));

		POSTGIS_DEBUGF(3, "Params: %s", params);

		for (param=params; ; param=NULL)
		{
			char *key, *val;
			param = strtok(param, " ");
			if ( param == NULL ) break;
			POSTGIS_DEBUGF(3, "Param: %s", param);

			key = param;
			val = strchr(key, '=');
			if ( val == NULL || *(val+1) == '\0' )
			{
				lwerror("Missing value for buffer "
				        "parameter %s", key);
				break;
			}
			*val = '\0';
			++val;

			POSTGIS_DEBUGF(3, "Param: %s : %s", key, val);

			if ( !strcmp(key, "endcap") )
			{
				/* Supported end cap styles:
				 *   "round", "flat", "square"
				 */
				if ( !strcmp(val, "round") )
				{
					endCapStyle = ENDCAP_ROUND;
				}
				else if ( !strcmp(val, "flat") ||
				          !strcmp(val, "butt")    )
				{
					endCapStyle = ENDCAP_FLAT;
				}
				else if ( !strcmp(val, "square") )
				{
					endCapStyle = ENDCAP_SQUARE;
				}
				else
				{
					lwerror("Invalid buffer end cap "
					        "style: %s (accept: "
					        "'round', 'flat', 'butt' "
					        "or 'square'"
					        ")", val);
					break;
				}

			}
			else if ( !strcmp(key, "join") )
			{
				if ( !strcmp(val, "round") )
				{
					joinStyle = JOIN_ROUND;
				}
				else if ( !strcmp(val, "mitre") ||
				          !strcmp(val, "miter")    )
				{
					joinStyle = JOIN_MITRE;
				}
				else if ( !strcmp(val, "bevel") )
				{
					joinStyle = JOIN_BEVEL;
				}
				else
				{
					lwerror("Invalid buffer end cap "
					        "style: %s (accept: "
					        "'round', 'mitre', 'miter' "
					        " or 'bevel'"
					        ")", val);
					break;
				}
			}
			else if ( !strcmp(key, "mitre_limit") ||
			          !strcmp(key, "miter_limit")    )
			{
				/* mitreLimit is a float */
				mitreLimit = atof(val);
			}
			else if ( !strcmp(key, "quad_segs") )
			{
				/* quadrant segments is an int */
				quadsegs = atoi(val);
			}
			else
			{
				lwerror("Invalid buffer parameter: %s (accept: "
				        "'endcap', 'join', 'mitre_limit', "
				        "'miter_limit and "
				        "'quad_segs')", key);
				break;
			}
		}

		pfree(params); /* was pstrduped */

		POSTGIS_DEBUGF(3, "endCap:%d joinStyle:%d mitreLimit:%g",
		               endCapStyle, joinStyle, mitreLimit);

	}

#if POSTGIS_GEOS_VERSION >= 32

	PROFSTART(PROF_GRUN);
	g3 = GEOSBufferWithStyle(g1, size, quadsegs,
	                         endCapStyle, joinStyle, mitreLimit);
	PROFSTOP(PROF_GRUN);

#else /* POSTGIS_GEOS_VERSION < 32 */

	if ( mitreLimit != DEFAULT_MITRE_LIMIT ||
	        endCapStyle != DEFAULT_ENDCAP_STYLE ||
	        joinStyle != DEFAULT_JOIN_STYLE )
	{
		lwerror("The GEOS version this postgis binary "
		        "was compiled against (%d) doesn't support "
		        "specifying a mitre limit != %d or styles different "
		        "from 'round' (needs 3.2 or higher)",
		        DEFAULT_MITRE_LIMIT, POSTGIS_GEOS_VERSION);
	}

	PROFSTART(PROF_GRUN);
	g3 = GEOSBuffer(g1,size,quadsegs);
	PROFSTOP(PROF_GRUN);

#endif /* POSTGIS_GEOS_VERSION < 32 */

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS buffer() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3));

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS buffer() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(intersection);
Datum intersection(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2, *g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_HASZ(geom1->type) ) ||
	       ( TYPE_HASZ(geom2->type) );

	SRID = pglwgeom_getSRID(geom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

	POSTGIS_DEBUG(3, "intersection() START");

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	POSTGIS_DEBUG(3, " constructed geometrys - calling geos");
	POSTGIS_DEBUGF(3, " g1 = %s", GEOSGeomToWKT(g1));
	POSTGIS_DEBUGF(3, " g2 = %s", GEOSGeomToWKT(g2));
	/*POSTGIS_DEBUGF(3, "g2 is valid = %i",GEOSisvalid(g2)); */
	/*POSTGIS_DEBUGF(3, "g1 is valid = %i",GEOSisvalid(g1)); */

	PROFSTART(PROF_GRUN);
	g3 = GEOSIntersection(g1,g2);
	PROFSTOP(PROF_GRUN);

	POSTGIS_DEBUG(3, " intersection finished");

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS Intersection() threw an error!");
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, SRID);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS Intersection() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}

/**
 * @example difference {@link #difference} - SELECT difference(
 *      'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
 *	'POLYGON((5 5, 15 5, 15 7, 5 7, 5 5))');
 */
PG_FUNCTION_INFO_V1(difference);
Datum difference(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2, *g3;
	PG_LWGEOM *result;
	int is3d;
	int SRID;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	is3d = ( TYPE_HASZ(geom1->type) ) ||
	       ( TYPE_HASZ(geom2->type) );

	SRID = pglwgeom_getSRID(geom1);
	errorIfSRIDMismatch(SRID, pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	g3 = GEOSDifference(g1,g2);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS difference() threw an error!");
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, SRID);

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, is3d);
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS difference() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g3);

	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, result);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


/**
	@example pointonsurface - {@link #pointonsurface} SELECT pointonsurface('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
*/
PG_FUNCTION_INFO_V1(pointonsurface);
Datum pointonsurface(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_GRUN);
	g3 = GEOSPointOnSurface(g1);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS pointonsurface() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /* never get here */
	}

	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS pointonsurface() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /* never get here */
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);

	/* compressType(result);  */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

PG_FUNCTION_INFO_V1(centroid);
Datum centroid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom, *result;
	GEOSGeometry *geosgeom, *geosresult;

	PROFSTART(PROF_QRUN);

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	geosgeom = (GEOSGeometry *)POSTGIS2GEOS(geom);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_GRUN);
	geosresult = GEOSGetCentroid(geosgeom);
	PROFSTOP(PROF_GRUN);

	if ( geosresult == NULL )
	{
		GEOSGeom_destroy(geosgeom);
		elog(ERROR,"GEOS getCentroid() threw an error!");
		PG_RETURN_NULL();
	}

	GEOSSetSRID(geosresult, pglwgeom_getSRID(geom));

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(geosresult, TYPE_HASZ(geom->type));
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(geosgeom);
		GEOSGeom_destroy(geosresult);
		elog(ERROR,"Error in GEOS-PGIS conversion");
		PG_RETURN_NULL();
	}
	GEOSGeom_destroy(geosgeom);
	GEOSGeom_destroy(geosresult);

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom, NULL, result);

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(result);
}



/*---------------------------------------------*/


/**
 * @brief Throws an ereport ERROR if either geometry is a COLLECTIONTYPE.  Additionally
 * 		displays a HINT of the first 80 characters of the WKT representation of the
 * 		problematic geometry so a user knows which parameter and which geometry
 * 		is causing the problem.
 */
void errorIfGeometryCollection(PG_LWGEOM *g1, PG_LWGEOM *g2)
{
	int t1 = lwgeom_getType(g1->type);
	int t2 = lwgeom_getType(g2->type);

	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
	int result;
	char* hintmsg;

	if ( t1 == COLLECTIONTYPE)
	{
		result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, SERIALIZED_FORM(g1), PARSER_CHECK_NONE);
		hintmsg = lwmessage_truncate(lwg_unparser_result.wkoutput, 0, strlen(lwg_unparser_result.wkoutput), 80, 1);
		ereport(ERROR,
		        (errmsg("Relate Operation called with a LWGEOMCOLLECTION type.  This is unsupported."),
		         errhint("Change argument 1: '%s'", hintmsg))
		       );
		pfree(hintmsg);
	}
	else if (t2 == COLLECTIONTYPE)
	{
		result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, SERIALIZED_FORM(g2), PARSER_CHECK_NONE);
		hintmsg = lwmessage_truncate(lwg_unparser_result.wkoutput, 0, strlen(lwg_unparser_result.wkoutput), 80, 1);
		ereport(ERROR,
		        (errmsg("Relate Operation called with a LWGEOMCOLLECTION type.  This is unsupported."),
		         errhint("Change argument 2: '%s'", hintmsg))
		       );
		pfree(hintmsg);
	}
}

PG_FUNCTION_INFO_V1(isvalid);
Datum isvalid(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	LWGEOM *lwgeom;
	bool result;
	GEOSGeom g1;
#if POSTGIS_GEOS_VERSION < 33
  BOX2DFLOAT4 box1;
#endif

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

#if POSTGIS_GEOS_VERSION < 33
  /* Short circuit and return FALSE if we have infinite coordinates */
  /* GEOS 3.3+ is supposed to  handle this stuff OK */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) )	
	{
		if ( isinf(box1.xmax) || isinf(box1.ymax) || isinf(box1.xmin) || isinf(box1.ymin) || 
		     isnan(box1.xmax) || isnan(box1.ymax) || isnan(box1.xmin) || isnan(box1.ymin)  )
		{
      lwnotice("Geometry contains an Inf or NaN coordinate");
			PG_RETURN_BOOL(FALSE);
		}
	}
#endif

	initGEOS(lwnotice, lwnotice);
	PROFSTART(PROF_P2G1);

	lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
	if ( ! lwgeom )
	{
		lwerror("unable to deserialize input");
	}
	
	
	g1 = LWGEOM2GEOS(lwgeom);
	if ( ! g1 )
	{
		lwgeom_release(lwgeom);
		PG_RETURN_BOOL(FALSE);
	}
	lwgeom_release(lwgeom);

	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_GRUN);
	result = GEOSisValid(g1);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	if (result == 2)
	{
		elog(ERROR,"GEOS isvalid() threw an error!");
		PG_RETURN_NULL(); /*never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, NULL);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_BOOL(result);
}

/*
** IsValidReason is only available in the GEOS
** C API > version 3.0
*/
PG_FUNCTION_INFO_V1(isvalidreason);
Datum isvalidreason(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom = NULL;
	char *reason_str = NULL;
	int len = 0;
	char *result = NULL;
	const GEOSGeometry *g1 = NULL;
#if POSTGIS_GEOS_VERSION < 33
  BOX2DFLOAT4 box;
#endif

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

#if POSTGIS_GEOS_VERSION < 33
  /* Short circuit and return if we have infinite coordinates */
  /* GEOS 3.3+ is supposed to  handle this stuff OK */
	if ( getbox2d_p(SERIALIZED_FORM(geom), &box) )	
	{
		if ( isinf(box.xmax) || isinf(box.ymax) || isinf(box.xmin) || isinf(box.ymin) || 
		     isnan(box.xmax) || isnan(box.ymax) || isnan(box.xmin) || isnan(box.ymin)  )
		{
			const char *rsn = "Geometry contains an Inf or NaN coordinate";
    	len = strlen(rsn);
    	result = palloc(VARHDRSZ + len);
    	SET_VARSIZE(result, VARHDRSZ + len);
    	memcpy(VARDATA(result), rsn, len);
    	PG_FREE_IF_COPY(geom, 0);
    	PG_RETURN_POINTER(result);			
		}
	}
#endif

	initGEOS(lwnotice, lwnotice);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom);
	if ( ! g1 )
	{
		PG_RETURN_NULL();
	}

	reason_str = GEOSisValidReason(g1);
	GEOSGeom_destroy((GEOSGeometry *)g1);

	if (reason_str == NULL)
	{
		elog(ERROR,"GEOS isvalidreason() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}
	len = strlen(reason_str);
	result = palloc(VARHDRSZ + len);
	SET_VARSIZE(result, VARHDRSZ + len);
	memcpy(VARDATA(result), reason_str, len);
	free(reason_str);

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_POINTER(result);

}

/**
 * overlaps(PG_LWGEOM g1,PG_LWGEOM g2)
 * @param g1
 * @param g2
 * @return  if GEOS::g1->overlaps(g2) returns true
 * @throw an error (elog(ERROR,...)) if GEOS throws an error
 */
PG_FUNCTION_INFO_V1(overlaps);
Datum overlaps(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmax < box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin > box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax < box1.ymin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin > box2.ymax ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	result = GEOSOverlaps(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	if (result == 2)
	{
		elog(ERROR,"GEOS overlaps() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(contains);
Datum contains(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	BOX2DFLOAT4 box1, box2;
	int type1, type2;
	LWGEOM *lwgeom;
	LWPOINT *point;
	RTREE_POLY_CACHE *poly_cache;
	MemoryContext old_context;
	bool result;
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	POSTGIS_DEBUG(3, "contains called.");

	/*
	** short-circuit 1: if geom2 bounding box is not completely inside
	** geom1 bounding box we can prematurely return FALSE.
	** Do the test IFF BOUNDING BOX AVAILABLE.
	*/
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box2.xmin < box1.xmin ) || ( box2.xmax > box1.xmax ) ||
		        ( box2.ymin < box1.ymin ) || ( box2.ymax > box1.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	/*
	** short-circuit 2: if geom2 is a point and geom1 is a polygon
	** call the point-in-polygon function.
	*/
	type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
	type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
	if ((type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE) && type2 == POINTTYPE)
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");
		lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
		point = lwpoint_deserialize(SERIALIZED_FORM(geom2));

		POSTGIS_DEBUGF(3, "Precall point_in_multipolygon_rtree %p, %p", lwgeom, point);

		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom1), fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( type1 == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( type1 == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}
		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result == 1 ) /* completely inside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}
	else
	{
		POSTGIS_DEBUGF(3, "Contains: type1: %d, type2: %d", type1, type2);
	}

	initGEOS(lwnotice, lwnotice);

#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		g1 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		POSTGIS_DEBUG(4, "containsPrepared: cache is live, running preparedcontains");
		result = GEOSPreparedContains( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
#endif
	{
		g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
		g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		POSTGIS_DEBUG(4, "containsPrepared: cache is not ready, running standard contains");
		result = GEOSContains( g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		elog(ERROR,"GEOS contains() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);

}

PG_FUNCTION_INFO_V1(containsproperly);
Datum containsproperly(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *				geom1;
	PG_LWGEOM *				geom2;
	bool 					result;
	BOX2DFLOAT4 			box1, box2;
#ifdef PREPARED_GEOM
	PrepGeomCache *	prep_cache;
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	* short-circuit: if geom2 bounding box is not completely inside
	* geom1 bounding box we can prematurely return FALSE.
	* Do the test IFF BOUNDING BOX AVAILABLE.
	*/
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if (( box2.xmin < box1.xmin ) || ( box2.xmax > box1.xmax ) ||
		        ( box2.ymin < box1.ymin ) || ( box2.ymax > box1.ymax ))
			PG_RETURN_BOOL(FALSE);
	}

	initGEOS(lwnotice, lwnotice);

#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		GEOSGeometry *g = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		result = GEOSPreparedContainsProperly( prep_cache->prepared_geom, g);
		GEOSGeom_destroy(g);
	}
	else
#endif
	{
		GEOSGeometry *g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
		GEOSGeometry *g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		result = GEOSRelatePattern( g1, g2, "T**FF*FF*" );
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		elog(ERROR,"GEOS contains() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}

/*
 * Described at
 * http://lin-ear-th-inking.blogspot.com/2007/06/subtleties-of-ogc-covers-spatial.html
 */
PG_FUNCTION_INFO_V1(covers);
Datum covers(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	bool result;
	BOX2DFLOAT4 box1, box2;
	int type1, type2;
	LWGEOM *lwgeom;
	LWPOINT *point;
	RTREE_POLY_CACHE *poly_cache;
	MemoryContext old_context;
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom2 bounding box is not completely inside
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if (( box2.xmin < box1.xmin ) || ( box2.xmax > box1.xmax ) ||
		        ( box2.ymin < box1.ymin ) || ( box2.ymax > box1.ymax ))
		{
			PG_RETURN_BOOL(FALSE);
		}
	}
	/*
	 * short-circuit 2: if geom2 is a point and geom1 is a polygon
	 * call the point-in-polygon function.
	 */
	type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
	type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
	if ((type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE) && type2 == POINTTYPE)
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");

		lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
		point = lwpoint_deserialize(SERIALIZED_FORM(geom2));

		POSTGIS_DEBUGF(3, "Precall point_in_multipolygon_rtree %p, %p", lwgeom, point);

		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom1), fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( type1 == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( type1 == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result != -1 ) /* not outside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}
	else
	{
		POSTGIS_DEBUGF(3, "Covers: type1: %d, type2: %d", type1, type2);
	}

	initGEOS(lwnotice, lwnotice);

#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, 0 );

	if ( prep_cache && prep_cache->prepared_geom && prep_cache->argnum == 1 )
	{
		GEOSGeometry *g1 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		result = GEOSPreparedCovers( prep_cache->prepared_geom, g1);
		GEOSGeom_destroy(g1);
	}
	else
#endif
	{
		GEOSGeometry *g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
		GEOSGeometry *g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		result = GEOSRelatePattern( g1, g2, "******FF*" );
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		elog(ERROR,"GEOS covers() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);

}



PG_FUNCTION_INFO_V1(within);
Datum within(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
	LWGEOM *lwgeom;
	LWPOINT *point;
	int type1, type2;
	MemoryContext old_context;
	RTREE_POLY_CACHE *poly_cache;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom1 bounding box is not completely inside
	 * geom2 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box1.xmin < box2.xmin ) || ( box1.xmax > box2.xmax ) ||
		        ( box1.ymin < box2.ymin ) || ( box1.ymax > box2.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}
	/*
	 * short-circuit 2: if geom1 is a point and geom2 is a polygon
	 * call the point-in-polygon function.
	 */
	type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
	type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
	if ((type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE) && type1 == POINTTYPE)
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");

		point = lwpoint_deserialize(SERIALIZED_FORM(geom1));
		lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom2));

		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom2), fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( type2 == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( type2 == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result == 1 ) /* completely inside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	result = GEOSWithin(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS within() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


/*
 * Described at:
 * http://lin-ear-th-inking.blogspot.com/2007/06/subtleties-of-ogc-covers-spatial.html
 */
PG_FUNCTION_INFO_V1(coveredby);
Datum coveredby(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;
	LWGEOM *lwgeom;
	LWPOINT *point;
	int type1, type2;
	MemoryContext old_context;
	RTREE_POLY_CACHE *poly_cache;
	char *patt = "**F**F***";

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom1 bounding box is not completely inside
	 * geom2 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box1.xmin < box2.xmin ) || ( box1.xmax > box2.xmax ) ||
		        ( box1.ymin < box2.ymin ) || ( box1.ymax > box2.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}

		POSTGIS_DEBUG(3, "bounding box short-circuit missed.");
	}
	/*
	 * short-circuit 2: if geom1 is a point and geom2 is a polygon
	 * call the point-in-polygon function.
	 */
	type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
	type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
	if ((type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE) && type1 == POINTTYPE)
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");

		point = lwpoint_deserialize(SERIALIZED_FORM(geom1));
		lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom2));

		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, SERIALIZED_FORM(geom2), fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( type2 == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( type2 == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result != -1 ) /* not outside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	result = GEOSRelatePattern(g1,g2,patt);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS coveredby() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(crosses);
Datum crosses(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		        ( box2.ymax < box1.ymin ) || ( box2.ymin > box2.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	result = GEOSCrosses(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS crosses() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(intersects);
Datum intersects(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	uchar *serialized_poly;
	bool result;
	BOX2DFLOAT4 box1, box2;
	int type1, type2, polytype;
	LWPOINT *point;
	LWGEOM *lwgeom;
	MemoryContext old_context;
	RTREE_POLY_CACHE *poly_cache;
#ifdef PREPARED_GEOM
	PrepGeomCache *prep_cache;
#endif

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		        ( box2.ymax < box1.ymin ) || ( box2.ymin > box1.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	/*
	 * short-circuit 2: if the geoms are a point and a polygon,
	 * call the point_outside_polygon function.
	 */
	type1 = lwgeom_getType((uchar)SERIALIZED_FORM(geom1)[0]);
	type2 = lwgeom_getType((uchar)SERIALIZED_FORM(geom2)[0]);
	if ( (type1 == POINTTYPE && (type2 == POLYGONTYPE || type2 == MULTIPOLYGONTYPE)) ||
	        (type2 == POINTTYPE && (type1 == POLYGONTYPE || type1 == MULTIPOLYGONTYPE)))
	{
		POSTGIS_DEBUG(3, "Point in Polygon test requested...short-circuiting.");

		if ( type1 == POINTTYPE )
		{
			point = lwpoint_deserialize(SERIALIZED_FORM(geom1));
			lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom2));
			serialized_poly = SERIALIZED_FORM(geom2);
			polytype = type2;
		}
		else
		{
			point = lwpoint_deserialize(SERIALIZED_FORM(geom2));
			lwgeom = lwgeom_deserialize(SERIALIZED_FORM(geom1));
			serialized_poly = SERIALIZED_FORM(geom1);
			polytype = type1;
		}
		/*
		 * Switch the context to the function-scope context,
		 * retrieve the appropriate cache object, cache it for
		 * future use, then switch back to the local context.
		 */
		old_context = MemoryContextSwitchTo(fcinfo->flinfo->fn_mcxt);
		poly_cache = retrieveCache(lwgeom, serialized_poly, fcinfo->flinfo->fn_extra);
		fcinfo->flinfo->fn_extra = poly_cache;
		MemoryContextSwitchTo(old_context);

		if ( poly_cache->ringIndices )
		{
			result = point_in_multipolygon_rtree(poly_cache->ringIndices, poly_cache->polyCount, poly_cache->ringCounts, point);
		}
		else if ( polytype == POLYGONTYPE )
		{
			result = point_in_polygon((LWPOLY*)lwgeom, point);
		}
		else if ( polytype == MULTIPOLYGONTYPE )
		{
			result = point_in_multipolygon((LWMPOLY*)lwgeom, point);
		}
		else
		{
			/* Gulp! Should not be here... */
			elog(ERROR,"Type isn't poly or multipoly!");
			PG_RETURN_NULL();
		}

		PG_FREE_IF_COPY(geom1, 0);
		PG_FREE_IF_COPY(geom2, 1);
		lwgeom_release((LWGEOM *)lwgeom);
		lwgeom_release((LWGEOM *)point);
		if ( result != -1 ) /* not outside */
		{
			PG_RETURN_BOOL(TRUE);
		}
		else
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwnotice);
#ifdef PREPARED_GEOM
	prep_cache = GetPrepGeomCache( fcinfo, geom1, geom2 );

	if ( prep_cache && prep_cache->prepared_geom )
	{
		if ( prep_cache->argnum == 1 )
		{
			GEOSGeometry *g = (GEOSGeometry *)POSTGIS2GEOS(geom2);
			result = GEOSPreparedIntersects( prep_cache->prepared_geom, g);
			GEOSGeom_destroy(g);
		}
		else
		{
			GEOSGeometry *g = (GEOSGeometry *)POSTGIS2GEOS(geom1);
			result = GEOSPreparedIntersects( prep_cache->prepared_geom, g);
			GEOSGeom_destroy(g);
		}
	}
	else
#endif
	{
		GEOSGeometry *g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
		GEOSGeometry *g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
		result = GEOSIntersects( g1, g2);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
	}

	if (result == 2)
	{
		elog(ERROR,"GEOS intersects() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(touches);
Datum touches(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		        ( box2.ymax < box1.ymin ) || ( box2.ymin > box1.ymax ) )
		{
			PG_RETURN_BOOL(FALSE);
		}
	}

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1 );
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2 );
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	result = GEOSTouches(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS touches() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(disjoint);
Datum disjoint(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom2 bounding box does not overlap
	 * geom1 bounding box we can prematurely return TRUE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( ( box2.xmax < box1.xmin ) || ( box2.xmin > box1.xmax ) ||
		        ( box2.ymax < box1.ymin ) || ( box2.ymin > box1.ymax ) )
		{
			PG_RETURN_BOOL(TRUE);
		}
	}

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	result = GEOSDisjoint(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS disjoin() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}


PG_FUNCTION_INFO_V1(relate_pattern);
Datum relate_pattern(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	char *patt;
	bool result;
	GEOSGeometry *g1, *g2;
	int i;

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);

	patt =  DatumGetCString(DirectFunctionCall1(textout,
	                        PointerGetDatum(PG_GETARG_DATUM(2))));

	/*
	** Need to make sure 't' and 'f' are upper-case before handing to GEOS
	*/
	for ( i = 0; i < strlen(patt); i++ )
	{
		if ( patt[i] == 't' ) patt[i] = 'T';
		if ( patt[i] == 'f' ) patt[i] = 'F';
	}

	result = GEOSRelatePattern(g1,g2,patt);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	pfree(patt);

	if (result == 2)
	{
		elog(ERROR,"GEOS relate_pattern() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}



PG_FUNCTION_INFO_V1(relate_full);
Datum relate_full(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	char *relate_str;
	int len;
	text *result;

	POSTGIS_DEBUG(2, "in relate_full()");

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	initGEOS(lwnotice, lwnotice);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1 );
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2 );

	POSTGIS_DEBUG(3, "constructed geometries ");

	if ((g1==NULL) || (g2 == NULL))
		elog(NOTICE,"g1 or g2 are null");

	POSTGIS_DEBUGF(3, "%s", GEOSGeomToWKT(g1));
	POSTGIS_DEBUGF(3, "%s", GEOSGeomToWKT(g2));

	/*POSTGIS_DEBUGF(3, "valid g1 = %i", GEOSisvalid(g1));*/
	/*POSTGIS_DEBUGF(3, "valid g2 = %i",GEOSisvalid(g2));*/

	POSTGIS_DEBUG(3, "about to relate()");

	relate_str = GEOSRelate(g1, g2);

	POSTGIS_DEBUG(3, "finished relate()");

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (relate_str == NULL)
	{
		elog(ERROR,"GEOS relate() threw an error!");
		PG_RETURN_NULL(); /* never get here */
	}

	len = strlen(relate_str) + VARHDRSZ;

	result= palloc(len);
	SET_VARSIZE(result, len);

	memcpy(VARDATA(result), relate_str, len-VARHDRSZ);

	free(relate_str);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_POINTER(result);
}


PG_FUNCTION_INFO_V1(geomequals);
Datum geomequals(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom1;
	PG_LWGEOM *geom2;
	GEOSGeometry *g1, *g2;
	bool result;
	BOX2DFLOAT4 box1, box2;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	geom2 = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(1));

	errorIfGeometryCollection(geom1,geom2);
	errorIfSRIDMismatch(pglwgeom_getSRID(geom1), pglwgeom_getSRID(geom2));

	/*
	 * short-circuit 1: if geom2 bounding box does not equal
	 * geom1 bounding box we can prematurely return FALSE.
	 * Do the test IFF BOUNDING BOX AVAILABLE.
	 */
	if ( getbox2d_p(SERIALIZED_FORM(geom1), &box1) &&
	        getbox2d_p(SERIALIZED_FORM(geom2), &box2) )
	{
		if ( box2.xmax != box1.xmax ) PG_RETURN_BOOL(FALSE);
		if ( box2.xmin != box1.xmin ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymax != box1.ymax ) PG_RETURN_BOOL(FALSE);
		if ( box2.ymin != box2.ymin ) PG_RETURN_BOOL(FALSE);
	}

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_P2G2);
	g2 = (GEOSGeometry *)POSTGIS2GEOS(geom2);
	PROFSTOP(PROF_P2G2);

	PROFSTART(PROF_GRUN);
	result = GEOSEquals(g1,g2);
	PROFSTOP(PROF_GRUN);

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);

	if (result == 2)
	{
		elog(ERROR,"GEOS equals() threw an error!");
		PG_RETURN_NULL(); /*never get here */
	}

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, geom2, NULL);

	PG_FREE_IF_COPY(geom1, 0);
	PG_FREE_IF_COPY(geom2, 1);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(issimple);
Datum issimple(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	GEOSGeometry *g1;
	int result;

	POSTGIS_DEBUG(2, "issimple called");

	geom = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0)
		PG_RETURN_BOOL(true);

	initGEOS(lwnotice, lwnotice);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom);
	result = GEOSisSimple(g1);
	GEOSGeom_destroy(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS issimple() threw an error!");
		PG_RETURN_NULL(); /*never get here */
	}

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(isring);
Datum isring(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	GEOSGeometry *g1;
	int result;

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	if (lwgeom_getType(geom->type) != LINETYPE)
	{
		elog(ERROR,"isring() should only be called on a LINE");
	}

	if (lwgeom_getnumgeometries(SERIALIZED_FORM(geom)) == 0)
		PG_RETURN_BOOL(false);

	initGEOS(lwnotice, lwnotice);

	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom );
	result = GEOSisRing(g1);
	GEOSGeom_destroy(g1);

	if (result == 2)
	{
		elog(ERROR,"GEOS isring() threw an error!");
		PG_RETURN_NULL();
	}

	PG_FREE_IF_COPY(geom, 0);
	PG_RETURN_BOOL(result);
}



/*
**  GEOS <==> PostGIS conversion functions
**
** Default conversion creates a GEOS point array, then iterates through the
** PostGIS points, setting each value in the GEOS array one at a time.
**
*/

/* Return a POINTARRAY from a GEOSCoordSeq */
POINTARRAY *
ptarray_from_GEOSCoordSeq(const GEOSCoordSequence *cs, char want3d)
{
	unsigned int dims=2;
	unsigned int size, i, ptsize;
	uchar *points, *ptr;
	POINTARRAY *ret;

	POSTGIS_DEBUG(2, "ptarray_fromGEOSCoordSeq called");

	if ( ! GEOSCoordSeq_getSize(cs, &size) )
		lwerror("Exception thrown");

	POSTGIS_DEBUGF(4, " GEOSCoordSeq size: %d", size);

	if ( want3d )
	{
		if ( ! GEOSCoordSeq_getDimensions(cs, &dims) )
			lwerror("Exception thrown");

		POSTGIS_DEBUGF(4, " GEOSCoordSeq dimensions: %d", dims);

		/* forget higher dimensions (if any) */
		if ( dims > 3 ) dims = 3;
	}

	POSTGIS_DEBUGF(4, " output dimensions: %d", dims);

	ptsize = sizeof(double)*dims;

	ret = ptarray_construct((dims==3), 0, size);

	points = ret->serialized_pointlist;
	ptr = points;
	for (i=0; i<size; i++)
	{
		POINT3DZ point;
		GEOSCoordSeq_getX(cs, i, &(point.x));
		GEOSCoordSeq_getY(cs, i, &(point.y));
		if ( dims >= 3 ) GEOSCoordSeq_getZ(cs, i, &(point.z));
		memcpy(ptr, &point, ptsize);
		ptr += ptsize;
	}

	return ret;
}

/* Return an LWGEOM from a Geometry */
LWGEOM *
GEOS2LWGEOM(const GEOSGeometry *geom, char want3d)
{
	int type = GEOSGeomTypeId(geom) ;
	bool hasZ = GEOSHasZ(geom);
	int SRID = GEOSGetSRID(geom);

	/* GEOS's 0 is equivalent to our -1 as for SRID values */
	if ( SRID == 0 ) SRID = -1;

	if ( ! hasZ )
	{
		if ( want3d )
		{
			POSTGIS_DEBUG(3, "Geometry has no Z, won't provide one");

			want3d = 0;
		}
	}

	if ( GEOSisEmpty(geom) )
	{
		return (LWGEOM*)lwcollection_construct_empty(SRID, want3d, 0);
	}

	switch (type)
	{
		const GEOSCoordSequence *cs;
		POINTARRAY *pa, **ppaa;
		const GEOSGeometry *g;
		LWGEOM **geoms;
		unsigned int i, ngeoms;

	case GEOS_POINT:
		POSTGIS_DEBUG(4, "lwgeom_from_geometry: it's a Point");

		cs = GEOSGeom_getCoordSeq(geom);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (LWGEOM *)lwpoint_construct(SRID, NULL, pa);

	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		POSTGIS_DEBUG(4, "lwgeom_from_geometry: it's a LineString or LinearRing");

		cs = GEOSGeom_getCoordSeq(geom);
		pa = ptarray_from_GEOSCoordSeq(cs, want3d);
		return (LWGEOM *)lwline_construct(SRID, NULL, pa);

	case GEOS_POLYGON:
		POSTGIS_DEBUG(4, "lwgeom_from_geometry: it's a Polygon");

		ngeoms = GEOSGetNumInteriorRings(geom);
		ppaa = lwalloc(sizeof(POINTARRAY *)*(ngeoms+1));
		g = GEOSGetExteriorRing(geom);
		cs = GEOSGeom_getCoordSeq(g);
		ppaa[0] = ptarray_from_GEOSCoordSeq(cs, want3d);
		for (i=0; i<ngeoms; i++)
		{
			g = GEOSGetInteriorRingN(geom, i);
			cs = GEOSGeom_getCoordSeq(g);
			ppaa[i+1] = ptarray_from_GEOSCoordSeq(cs,
			                                      want3d);
		}
		return (LWGEOM *)lwpoly_construct(SRID, NULL,
		                                  ngeoms+1, ppaa);

	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_GEOMETRYCOLLECTION:
		POSTGIS_DEBUG(4, "lwgeom_from_geometry: it's a Collection or Multi");

		ngeoms = GEOSGetNumGeometries(geom);
		geoms = NULL;
		if ( ngeoms )
		{
			geoms = lwalloc(sizeof(LWGEOM *)*ngeoms);
			for (i=0; i<ngeoms; i++)
			{
				g = GEOSGetGeometryN(geom, i);
				geoms[i] = GEOS2LWGEOM(g, want3d);
			}
		}
		return (LWGEOM *)lwcollection_construct(type,
		                                        SRID, NULL, ngeoms, geoms);

	default:
		lwerror("GEOS2LWGEOM: unknown geometry type: %d", type);
		return NULL;

	}

}


PG_LWGEOM *
GEOS2POSTGIS(GEOSGeom geom, char want3d)
{
	LWGEOM *lwgeom;
	PG_LWGEOM *result;

	lwgeom = GEOS2LWGEOM(geom, want3d);
	if ( ! lwgeom )
	{
		lwerror("GEOS2POSTGIS: GEOS2LWGEOM returned NULL");
		return NULL;
	}

	POSTGIS_DEBUGF(4, "GEOS2POSTGIS: GEOS2LWGEOM returned a %s", lwgeom_summary(lwgeom, 0));

	if ( is_worth_caching_lwgeom_bbox(lwgeom) )
	{
		lwgeom_add_bbox(lwgeom);
	}

	result = pglwgeom_serialize(lwgeom);

	return result;
}

/*-----=POSTGIS2GEOS= */

GEOSCoordSeq ptarray_to_GEOSCoordSeq(POINTARRAY *);
GEOSGeom LWGEOM2GEOS(LWGEOM *lwgeom);

GEOSCoordSeq
ptarray_to_GEOSCoordSeq(POINTARRAY *pa)
{
	unsigned int dims = 2;
	unsigned int size, i;
	POINT3DZ p;
	GEOSCoordSeq sq;

	if ( TYPE_HASZ(pa->dims) ) dims = 3;
	size = pa->npoints;

	sq = GEOSCoordSeq_create(size, dims);
	if ( ! sq ) lwerror("Error creating GEOS Coordinate Sequence");

	for (i=0; i<size; i++)
	{
		getPoint3dz_p(pa, i, &p);

		POSTGIS_DEBUGF(4, "Point: %g,%g,%g", p.x, p.y, p.z);

#if POSTGIS_GEOS_VERSION < 33
    /* Make sure we don't pass any infinite values down into GEOS */
    /* GEOS 3.3+ is supposed to  handle this stuff OK */
    if ( isinf(p.x) || isinf(p.y) || (dims == 3 && isinf(p.z)) )
      lwerror("Infinite coordinate value found in geometry.");
#endif

		GEOSCoordSeq_setX(sq, i, p.x);
		GEOSCoordSeq_setY(sq, i, p.y);
		if ( dims == 3 ) GEOSCoordSeq_setZ(sq, i, p.z);
	}
	return sq;
}

GEOSGeometry *
LWGEOM2GEOS(LWGEOM *lwgeom)
{
	GEOSCoordSeq sq;
	GEOSGeom g, shell, *geoms;
	/*
	LWGEOM *tmp;
	*/
	unsigned int ngeoms, i;
	int type = 0;
	int geostype;
#if POSTGIS_DEBUG_LEVEL >= 4
	char *wkt;
#endif

	POSTGIS_DEBUGF(4, "LWGEOM2GEOS got a %s", lwgeom_typename(type));

	if (has_arc(lwgeom))
	{
		POSTGIS_DEBUG(3, "LWGEOM2GEOS_c: arced geometry found.");

		lwerror("Exception in LWGEOM2GEOS: curved geometry not supported.");
		/*
		tmp = lwgeom;
		lwgeom = lwgeom_segmentize(tmp, 32);
		POSTGIS_DEBUGF(3, "LWGEOM2GEOM_c: was %p, is %p", tmp, lwgeom);
		*/
	}
	type = TYPE_GETTYPE(lwgeom->type);
	switch (type)
	{
		LWPOINT *lwp;
		LWPOLY *lwpoly;
		LWLINE *lwl;
		LWCOLLECTION *lwc;

	case POINTTYPE:
		lwp = (LWPOINT *)lwgeom;
		sq = ptarray_to_GEOSCoordSeq(lwp->point);
		g = GEOSGeom_createPoint(sq);
		if ( ! g ) lwerror("Exception in LWGEOM2GEOS");
		break;
	case LINETYPE:
		lwl = (LWLINE *)lwgeom;
		sq = ptarray_to_GEOSCoordSeq(lwl->points);
		g = GEOSGeom_createLineString(sq);
		if ( ! g ) lwerror("Exception in LWGEOM2GEOS");
		break;

	case POLYGONTYPE:
		lwpoly = (LWPOLY *)lwgeom;
		sq = ptarray_to_GEOSCoordSeq(lwpoly->rings[0]);
		shell = GEOSGeom_createLinearRing(sq);
		if ( ! shell ) return NULL;
		/*lwerror("LWGEOM2GEOS: exception during polygon shell conversion"); */
		ngeoms = lwpoly->nrings-1;
		geoms = malloc(sizeof(GEOSGeom)*ngeoms);
		for (i=1; i<lwpoly->nrings; ++i)
		{
			sq = ptarray_to_GEOSCoordSeq(lwpoly->rings[i]);
			geoms[i-1] = GEOSGeom_createLinearRing(sq);
			if ( ! geoms[i-1] ) return NULL;
			/*lwerror("LWGEOM2GEOS: exception during polygon hole conversion"); */
		}
		g = GEOSGeom_createPolygon(shell, geoms, ngeoms);
		if ( ! g ) return NULL;
		free(geoms);
		break;
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		if ( type == MULTIPOINTTYPE )
			geostype = GEOS_MULTIPOINT;
		else if ( type == MULTILINETYPE )
			geostype = GEOS_MULTILINESTRING;
		else if ( type == MULTIPOLYGONTYPE )
			geostype = GEOS_MULTIPOLYGON;
		else
			geostype = GEOS_GEOMETRYCOLLECTION;

		lwc = (LWCOLLECTION *)lwgeom;
		ngeoms = lwc->ngeoms;
		geoms = malloc(sizeof(GEOSGeom)*ngeoms);

		for (i=0; i<ngeoms; ++i)
		{
			geoms[i] = LWGEOM2GEOS(lwc->geoms[i]);
			if ( ! geoms[i] ) return NULL;
		}
		g = GEOSGeom_createCollection(geostype, geoms, ngeoms);
		if ( ! g ) return NULL;
		free(geoms);
		break;

	default:
		lwerror("Unknown geometry type: %d", type);

		return NULL;
	}

	GEOSSetSRID(g, lwgeom->SRID);

#if POSTGIS_DEBUG_LEVEL >= 4
	wkt = GEOSGeomToWKT(g);
	POSTGIS_DEBUGF(4, "LWGEOM2GEOS: GEOSGeom: %s", wkt);
	/*
	if(tmp != NULL) lwgeom_release(tmp);
	*/
	free(wkt);
#endif

	return g;
}

const GEOSGeometry *
POSTGIS2GEOS(PG_LWGEOM *pglwgeom)
{
	const GEOSGeometry *ret;
	LWGEOM *lwgeom = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom));
	if ( ! lwgeom )
	{
		lwerror("POSTGIS2GEOS: unable to deserialize input");
		return NULL;
	}
	ret = LWGEOM2GEOS(lwgeom);
	lwgeom_release(lwgeom);
	if ( ! ret )
	{
		lwerror("POSTGIS2GEOS conversion failed");
		return NULL;
	}
	return ret;
}


PG_FUNCTION_INFO_V1(GEOSnoop);
Datum GEOSnoop(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *geom;
	GEOSGeometry *geosgeom;
	PG_LWGEOM *lwgeom_result;
#if POSTGIS_DEBUG_LEVEL > 0
	int result;
	LWGEOM_UNPARSER_RESULT lwg_unparser_result;
#endif

	initGEOS(lwnotice, lwnotice);

	geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

#if POSTGIS_DEBUG_LEVEL > 0
	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, SERIALIZED_FORM(geom), PARSER_CHECK_NONE);
	POSTGIS_DEBUGF(2, "GEOSnoop: IN: %s", lwg_unparser_result.wkoutput);
#endif

	geosgeom = (GEOSGeometry *)POSTGIS2GEOS(geom);
	if ( ! geosgeom ) PG_RETURN_NULL();

	PROFSTART(PROF_GRUN);
	PROFSTOP(PROF_GRUN);

	lwgeom_result = GEOS2POSTGIS(geosgeom, TYPE_HASZ(geom->type));
	GEOSGeom_destroy(geosgeom);

#if POSTGIS_DEBUG_LEVEL > 0
	result = serialized_lwgeom_to_ewkt(&lwg_unparser_result, SERIALIZED_FORM(lwgeom_result), PARSER_CHECK_NONE);
	POSTGIS_DEBUGF(4, "GEOSnoop: OUT: %s", lwg_unparser_result.wkoutput);
#endif

	PG_FREE_IF_COPY(geom, 0);

	PG_RETURN_POINTER(lwgeom_result);
}

PG_FUNCTION_INFO_V1(polygonize_garray);
Datum polygonize_garray(PG_FUNCTION_ARGS)
{
	Datum datum;
	ArrayType *array;
	int is3d = 0;
	unsigned int nelems, i;
	PG_LWGEOM *result;
	GEOSGeometry *geos_result;
	const GEOSGeometry **vgeoms;
	int SRID=-1;
	size_t offset;
#if POSTGIS_DEBUG_LEVEL >= 3
	static int call=1;
#endif

#if POSTGIS_DEBUG_LEVEL >= 3
	call++;
#endif

	datum = PG_GETARG_DATUM(0);

	/* Null array, null geometry (should be empty?) */
	if ( (Pointer *)datum == NULL ) PG_RETURN_NULL();

	array = DatumGetArrayTypeP(datum);

	nelems = ArrayGetNItems(ARR_NDIM(array), ARR_DIMS(array));

	POSTGIS_DEBUGF(3, "polygonize_garray: number of elements: %d", nelems);

	if ( nelems == 0 ) PG_RETURN_NULL();

	/* Ok, we really need geos now ;) */
	initGEOS(lwnotice, lwnotice);

	vgeoms = palloc(sizeof(GEOSGeometry *)*nelems);
	offset = 0;
	for (i=0; i<nelems; i++)
	{
		PG_LWGEOM *geom = (PG_LWGEOM *)(ARR_DATA_PTR(array)+offset);
		offset += INTALIGN(VARSIZE(geom));

		vgeoms[i] = (GEOSGeometry *)POSTGIS2GEOS(geom);
		if ( ! i )
		{
			SRID = pglwgeom_getSRID(geom);
		}
		else
		{
			if ( SRID != pglwgeom_getSRID(geom) )
			{
				elog(ERROR, "polygonize: operation on mixed SRID geometries");
				PG_RETURN_NULL();
			}
		}
	}

	POSTGIS_DEBUG(3, "polygonize_garray: invoking GEOSpolygonize");

	geos_result = GEOSPolygonize(vgeoms, nelems);

	POSTGIS_DEBUG(3, "polygonize_garray: GEOSpolygonize returned");

	for (i=0; i<nelems; ++i) GEOSGeom_destroy((GEOSGeometry *)vgeoms[i]);
	pfree(vgeoms);

	if ( ! geos_result ) PG_RETURN_NULL();

	GEOSSetSRID(geos_result, SRID);
	result = GEOS2POSTGIS(geos_result, is3d);
	GEOSGeom_destroy(geos_result);
	if ( result == NULL )
	{
		elog(ERROR, "GEOS2POSTGIS returned an error");
		PG_RETURN_NULL(); /*never get here */
	}

	/*compressType(result); */

	PG_RETURN_POINTER(result);

}

PG_FUNCTION_INFO_V1(linemerge);
Datum linemerge(PG_FUNCTION_ARGS)
{
	PG_LWGEOM	*geom1;
	GEOSGeometry *g1, *g3;
	PG_LWGEOM *result;

	PROFSTART(PROF_QRUN);

	geom1 = (PG_LWGEOM *)  PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	initGEOS(lwnotice, lwnotice);

	PROFSTART(PROF_P2G1);
	g1 = (GEOSGeometry *)POSTGIS2GEOS(geom1);
	PROFSTOP(PROF_P2G1);

	PROFSTART(PROF_GRUN);
	g3 = GEOSLineMerge(g1);
	PROFSTOP(PROF_GRUN);

	if (g3 == NULL)
	{
		elog(ERROR,"GEOS LineMerge() threw an error!");
		GEOSGeom_destroy(g1);
		PG_RETURN_NULL(); /*never get here */
	}


	POSTGIS_DEBUGF(3, "result: %s", GEOSGeomToWKT(g3) ) ;

	GEOSSetSRID(g3, pglwgeom_getSRID(geom1));

	PROFSTART(PROF_G2P);
	result = GEOS2POSTGIS(g3, TYPE_HASZ(geom1->type));
	PROFSTOP(PROF_G2P);

	if (result == NULL)
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g3);
		elog(ERROR,"GEOS LineMerge() threw an error (result postgis geometry formation)!");
		PG_RETURN_NULL(); /*never get here */
	}
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g3);


	/* compressType(result); */

	PROFSTOP(PROF_QRUN);
	PROFREPORT("geos",geom1, NULL, result);

	PG_FREE_IF_COPY(geom1, 0);

	PG_RETURN_POINTER(result);
}

/*
 * Take a geometry and return an areal geometry
 * (Polygon or MultiPolygon).
 * Actually a wrapper around GEOSpolygonize,
 * transforming the resulting collection into
 * a valid polygonzl Geometry.
 */
PG_FUNCTION_INFO_V1(LWGEOM_buildarea);
Datum LWGEOM_buildarea(PG_FUNCTION_ARGS)
{
	int is3d = 0;
	unsigned int i, ngeoms;
	PG_LWGEOM *result;
	LWGEOM *lwg;
	GEOSGeometry *geos_result, *shp;
	GEOSGeometry const *vgeoms[1];
	int SRID=-1;
#if POSTGIS_DEBUG_LEVEL >= 3
	static int call=1;
#endif

	PG_LWGEOM *geom = (PG_LWGEOM *)PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

#if POSTGIS_DEBUG_LEVEL >= 3
	call++;
	lwnotice("buildarea called (call %d)", call);
#endif

	SRID = pglwgeom_getSRID(geom);
	is3d = TYPE_HASZ(geom->type);

	POSTGIS_DEBUGF(3, "LWGEOM_buildarea got geom @ %p", geom);

	initGEOS(lwnotice, lwnotice);

	vgeoms[0] = (const GEOSGeometry *)POSTGIS2GEOS(geom);
	geos_result = GEOSPolygonize(vgeoms, 1);
	GEOSGeom_destroy((GEOSGeometry *)vgeoms[0]);

	POSTGIS_DEBUGF(3, "GEOSpolygonize returned @ %p", geos_result);

	/* Null return from GEOSpolygonize */
	if ( ! geos_result ) PG_RETURN_NULL();

	/*
	 * We should now have a collection
	 */
#if PARANOIA_LEVEL > 0
	if ( GEOSGeometryTypeId(geos_result) != COLLECTIONTYPE )
	{
		GEOSGeom_destroy(geos_result);
		lwerror("Unexpected return from GEOSpolygonize");
		PG_RETURN_NULL();
	}
#endif

	ngeoms = GEOSGetNumGeometries(geos_result);

	POSTGIS_DEBUGF(3, "GEOSpolygonize: ngeoms in polygonize output: %d", ngeoms);

	/*
	 * No geometries in collection, return NULL
	 */
	if ( ngeoms == 0 )
	{
		GEOSGeom_destroy(geos_result);
		PG_RETURN_NULL();
	}

	/*
	 * Return first geometry if we only have one in collection,
	 * to avoid the unnecessary Geometry clone below.
	 */
	if ( ngeoms == 1 )
	{
		shp = (GEOSGeometry *)GEOSGetGeometryN(geos_result, 0);
		lwg = GEOS2LWGEOM(shp, is3d);
		lwg->SRID = SRID;
		result = pglwgeom_serialize(lwg);
		lwgeom_release(lwg);
		GEOSGeom_destroy(geos_result);
		PG_RETURN_POINTER(result);
	}

	/*
	 * Iteratively invoke symdifference on outer rings
	 * as suggested by Carl Anderson:
	 * postgis-devel/2005-December/001805.html
	 */
	shp = NULL;
	for (i=0; i<ngeoms; ++i)
	{
		GEOSGeom extring, tmp;
		GEOSCoordSeq sq;

		/*
		 * Construct a Polygon from geometry i exterior ring
		 * We don't use GEOSGeom_clone on the ExteriorRing
		 * due to a bug in CAPI contained in GEOS 2.2 branch
		 * failing to properly return a LinearRing from
		 * a LinearRing clone.
		 */
		sq=GEOSCoordSeq_clone(GEOSGeom_getCoordSeq(
		                          GEOSGetExteriorRing(GEOSGetGeometryN( geos_result, i))
		                      ));
		extring = GEOSGeom_createPolygon(
		              GEOSGeom_createLinearRing(sq),
		              NULL, 0
		          );

		if ( extring == NULL ) /* exception */
		{
			lwerror("GEOSCreatePolygon threw an exception");
			PG_RETURN_NULL();
		}

		if ( shp == NULL )
		{
			shp = extring;
		}
		else
		{
			tmp = GEOSSymDifference(shp, extring);
			GEOSGeom_destroy(shp);
			GEOSGeom_destroy(extring);
			shp = tmp;
		}
	}

	GEOSGeom_destroy(geos_result);

	GEOSSetSRID(shp, SRID);
	result = GEOS2POSTGIS(shp, is3d);
	GEOSGeom_destroy(shp);

#if PARANOIA_LEVEL > 0
	if ( result == NULL )
	{
		lwerror("serialization error");
		PG_RETURN_NULL(); /*never get here */
	}

#endif

	PG_RETURN_POINTER(result);

}

