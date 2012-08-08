/**********************************************************************
 * $Id: lwmline.c 5181 2010-02-01 17:35:55Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "liblwgeom.h"

void
lwmline_release(LWMLINE *lwmline)
{
	lwgeom_release(lwmline_as_lwgeom(lwmline));
}

LWMLINE *
lwmline_deserialize(uchar *srl)
{
	LWMLINE *result;
	LWGEOM_INSPECTED *insp;
	int type = lwgeom_getType(srl[0]);
	int i;

	if ( type != MULTILINETYPE )
	{
		lwerror("lwmline_deserialize called on NON multiline: %d",
		        type);
		return NULL;
	}

	insp = lwgeom_inspect(srl);

	result = lwalloc(sizeof(LWMLINE));
	result->type = insp->type;
	result->SRID = insp->SRID;
	result->ngeoms = insp->ngeometries;

	if ( insp->ngeometries )
	{
		result->geoms = lwalloc(sizeof(LWLINE *)*insp->ngeometries);
	}
	else
	{
		result->geoms = NULL;
	}

	if (lwgeom_hasBBOX(srl[0]))
	{
		result->bbox = lwalloc(sizeof(BOX2DFLOAT4));
		memcpy(result->bbox, srl+1, sizeof(BOX2DFLOAT4));
	}
	else result->bbox = NULL;


	for (i=0; i<insp->ngeometries; i++)
	{
		result->geoms[i] = lwline_deserialize(insp->sub_geoms[i]);
		if ( TYPE_NDIMS(result->geoms[i]->type) != TYPE_NDIMS(result->type) )
		{
			lwerror("Mixed dimensions (multiline:%d, line%d:%d)",
			        TYPE_NDIMS(result->type), i,
			        TYPE_NDIMS(result->geoms[i]->type)
			       );
			return NULL;
		}
	}

	return result;
}

/*
 * Add 'what' to this multiline at position 'where'.
 * where=0 == prepend
 * where=-1 == append
 * Returns a MULTILINE or a COLLECTION
 */
LWGEOM *
lwmline_add(const LWMLINE *to, uint32 where, const LWGEOM *what)
{
	LWCOLLECTION *col;
	LWGEOM **geoms;
	int newtype;
	uint32 i;

	if ( where == -1 ) where = to->ngeoms;
	else if ( where < -1 || where > to->ngeoms )
	{
		lwerror("lwmline_add: add position out of range %d..%d",
		        -1, to->ngeoms);
		return NULL;
	}

	/* dimensions compatibility are checked by caller */

	/* Construct geoms array */
	geoms = lwalloc(sizeof(LWGEOM *)*(to->ngeoms+1));
	for (i=0; i<where; i++)
	{
		geoms[i] = lwgeom_clone((LWGEOM *)to->geoms[i]);
	}
	geoms[where] = lwgeom_clone(what);
	for (i=where; i<to->ngeoms; i++)
	{
		geoms[i+1] = lwgeom_clone((LWGEOM *)to->geoms[i]);
	}

	if ( TYPE_GETTYPE(what->type) == LINETYPE ) newtype = MULTILINETYPE;
	else newtype = COLLECTIONTYPE;

	col = lwcollection_construct(newtype,
	                             to->SRID, NULL,
	                             to->ngeoms+1, geoms);

	return (LWGEOM *)col;

}

/**
* Re-write the measure ordinate (or add one, if it isn't already there) interpolating
* the measure between the supplied start and end values.
*/
LWMLINE*
lwmline_measured_from_lwmline(const LWMLINE *lwmline, double m_start, double m_end)
{
	int i = 0;
	int hasm = 0, hasz = 0;
	double length = 0.0, length_so_far = 0.0;
	double m_range = m_end - m_start;
	LWGEOM **geoms = NULL;

	if ( TYPE_GETTYPE(lwmline->type) != MULTILINETYPE )
	{
		lwerror("lwmline_measured_from_lmwline: only multiline types supported");
		return NULL;
	}

	hasz = TYPE_HASZ(lwmline->type);
	hasm = 1;

	/* Calculate the total length of the mline */
	for ( i = 0; i < lwmline->ngeoms; i++ )
	{
		LWLINE *lwline = (LWLINE*)lwmline->geoms[i];
		if ( lwline->points && lwline->points->npoints > 1 )
		{
			length += lwgeom_pointarray_length2d(lwline->points);
		}
	}

	if ( lwgeom_is_empty((LWGEOM*)lwmline) )
	{
		return (LWMLINE*)lwcollection_construct_empty(lwmline->SRID, hasz, hasm);
	}

	geoms = lwalloc(sizeof(LWGEOM*) * lwmline->ngeoms);

	for ( i = 0; i < lwmline->ngeoms; i++ )
	{
		double sub_m_start, sub_m_end;
		double sub_length = 0.0;
		LWLINE *lwline = (LWLINE*)lwmline->geoms[i];

		if ( lwline->points && lwline->points->npoints > 1 )
		{
			sub_length = lwgeom_pointarray_length2d(lwline->points);
		}

		sub_m_start = (m_start + m_range * length_so_far / length);
		sub_m_end = (m_start + m_range * (length_so_far + sub_length) / length);

		geoms[i] = (LWGEOM*)lwline_measured_from_lwline(lwline, sub_m_start, sub_m_end);

		length_so_far += sub_length;
	}

	return (LWMLINE*)lwcollection_construct(lwmline->type, lwmline->SRID, NULL, lwmline->ngeoms, geoms);
}

void lwmline_free(LWMLINE *mline)
{
	int i;
	if ( mline->bbox )
	{
		lwfree(mline->bbox);
	}
	for ( i = 0; i < mline->ngeoms; i++ )
	{
		if ( mline->geoms[i] )
		{
			lwline_free(mline->geoms[i]);
		}
	}
	if ( mline->geoms )
	{
		lwfree(mline->geoms);
	}
	lwfree(mline);

}
