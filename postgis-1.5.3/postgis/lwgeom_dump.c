/**********************************************************************
 * $Id: lwgeom_dump.c 4635 2009-10-09 19:23:05Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2001-2009 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/
#include "postgres.h"
#include "fmgr.h"
#include "utils/elog.h"
#include "utils/array.h"
#include "utils/geo_decls.h"
#include "funcapi.h"

#include "liblwgeom.h"
#include "lwgeom_pg.h"
#include "profile.h"

#include "../postgis_config.h"

#include <math.h>
#include <float.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>


Datum LWGEOM_dump(PG_FUNCTION_ARGS);
Datum LWGEOM_dump_rings(PG_FUNCTION_ARGS);

typedef struct GEOMDUMPNODE_T
{
	int idx;
	LWCOLLECTION *geom;
}
GEOMDUMPNODE;

#define MAXDEPTH 32
typedef struct GEOMDUMPSTATE
{
	int stacklen;
	GEOMDUMPNODE *stack[MAXDEPTH];
	LWGEOM *root;
	LWGEOM *geom;
}
GEOMDUMPSTATE;

#define PUSH(x,y) ((x)->stack[(x)->stacklen++]=(y))
#define LAST(x) ((x)->stack[(x)->stacklen-1])
#define POP(x) (--((x)->stacklen))


PG_FUNCTION_INFO_V1(LWGEOM_dump);
Datum LWGEOM_dump(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwgeom;
	LWCOLLECTION *lwcoll;
	LWGEOM *lwgeom;
	FuncCallContext *funcctx;
	GEOMDUMPSTATE *state;
	GEOMDUMPNODE *node;
	TupleDesc tupdesc;
	HeapTuple tuple;
	AttInMetadata *attinmeta;
	MemoryContext oldcontext, newcontext;
	Datum result;
	char address[256];
	char *ptr;
	unsigned int i;
	char *values[2];

	if (SRF_IS_FIRSTCALL())
	{
		funcctx = SRF_FIRSTCALL_INIT();
		newcontext = funcctx->multi_call_memory_ctx;

		oldcontext = MemoryContextSwitchTo(newcontext);

		pglwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
		lwgeom = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom));

		/* Create function state */
		state = lwalloc(sizeof(GEOMDUMPSTATE));
		state->root = lwgeom;
		state->stacklen=0;

		if ( lwgeom_is_collection(TYPE_GETTYPE(lwgeom->type)) )
		{
			/*
			 * Push a GEOMDUMPNODE on the state stack
			 */
			node = lwalloc(sizeof(GEOMDUMPNODE));
			node->idx=0;
			node->geom = (LWCOLLECTION *)lwgeom;
			PUSH(state, node);
		}

		funcctx->user_fctx = state;

		/*
		 * Build a tuple description for an
		 * geometry_dump tuple
		 */
		tupdesc = RelationNameGetTupleDesc("geometry_dump");

		/*
		 * generate attribute metadata needed later to produce
		 * tuples from raw C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	newcontext = funcctx->multi_call_memory_ctx;

	/* get state */
	state = funcctx->user_fctx;

	/* Handled simple geometries */
	if ( ! state->root ) SRF_RETURN_DONE(funcctx);
	if ( ! lwgeom_is_collection(TYPE_GETTYPE(state->root->type)) )
	{
		values[0] = "{}";
		values[1] = lwgeom_to_hexwkb(state->root, PARSER_CHECK_NONE, -1);
		tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);
		result = HeapTupleGetDatum(tuple);

		state->root = NULL;
		SRF_RETURN_NEXT(funcctx, result);
	}

	while (1)
	{
		node = LAST(state);
		lwcoll = node->geom;

		if ( node->idx < lwcoll->ngeoms )
		{
			lwgeom = lwcoll->geoms[node->idx];
			if ( ! lwgeom_is_collection(TYPE_GETTYPE(lwgeom->type)) )
			{
				/* write address of current geom */
				ptr=address;
				*ptr++='{';
				for (i=0; i<state->stacklen; i++)
				{
					if ( i ) ptr += sprintf(ptr, ",");
					ptr += sprintf(ptr, "%d", state->stack[i]->idx+1);
				}
				*ptr++='}';
				*ptr='\0';

				break;
			}

			/*
			 * It's a collection, increment index
			 * of current node, push a new one on the
			 * stack
			 */

			oldcontext = MemoryContextSwitchTo(newcontext);

			node = lwalloc(sizeof(GEOMDUMPNODE));
			node->idx=0;
			node->geom = (LWCOLLECTION *)lwgeom;
			PUSH(state, node);

			MemoryContextSwitchTo(oldcontext);

			continue;
		}

		if ( ! POP(state) ) SRF_RETURN_DONE(funcctx);
		LAST(state)->idx++;
	}

	lwgeom->SRID = state->root->SRID;

	values[0] = address;
	values[1] = lwgeom_to_hexwkb(lwgeom, PARSER_CHECK_NONE, -1);
	tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);
	result = TupleGetDatum(funcctx->slot, tuple);
	node->idx++;
	SRF_RETURN_NEXT(funcctx, result);
}

struct POLYDUMPSTATE
{
	int ringnum;
	LWPOLY *poly;
};

PG_FUNCTION_INFO_V1(LWGEOM_dump_rings);
Datum LWGEOM_dump_rings(PG_FUNCTION_ARGS)
{
	PG_LWGEOM *pglwgeom;
	LWGEOM *lwgeom;
	FuncCallContext *funcctx;
	struct POLYDUMPSTATE *state;
	TupleDesc tupdesc;
	HeapTuple tuple;
	AttInMetadata *attinmeta;
	MemoryContext oldcontext, newcontext;
	Datum result;
	char address[256];
	char *values[2];

	if (SRF_IS_FIRSTCALL())
	{
		funcctx = SRF_FIRSTCALL_INIT();
		newcontext = funcctx->multi_call_memory_ctx;

		oldcontext = MemoryContextSwitchTo(newcontext);

		pglwgeom = (PG_LWGEOM *)PG_DETOAST_DATUM_COPY(PG_GETARG_DATUM(0));
		if ( TYPE_GETTYPE(pglwgeom->type) != POLYGONTYPE )
		{
			lwerror("Input is not a polygon");
		}

		lwgeom = lwgeom_deserialize(SERIALIZED_FORM(pglwgeom));

		/* Create function state */
		state = lwalloc(sizeof(struct POLYDUMPSTATE));
		state->poly = lwgeom_as_lwpoly(lwgeom);
		assert (state->poly);
		state->ringnum=0;

		funcctx->user_fctx = state;

		/*
		 * Build a tuple description for an
		 * geometry_dump tuple
		 */
		tupdesc = RelationNameGetTupleDesc("geometry_dump");

		/*
		 * generate attribute metadata needed later to produce
		 * tuples from raw C strings
		 */
		attinmeta = TupleDescGetAttInMetadata(tupdesc);
		funcctx->attinmeta = attinmeta;

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	newcontext = funcctx->multi_call_memory_ctx;

	/* get state */
	state = funcctx->user_fctx;

	/* Loop trough polygon rings */
	while (state->ringnum < state->poly->nrings )
	{
		LWPOLY* poly = state->poly;
		POINTARRAY *ring;
		LWGEOM* ringgeom;

		/* Switch to an appropriate memory context for POINTARRAY
		 * cloning and hexwkb allocation */
		oldcontext = MemoryContextSwitchTo(newcontext);

		/* We need a copy of input ring here */
		ring = ptarray_clone(poly->rings[state->ringnum]);

		/* Construct another polygon with shell only */
		ringgeom = (LWGEOM*)lwpoly_construct(
		               poly->SRID,
		               NULL, /* TODO: could use input bounding box here */
		               1, /* one ring */
		               &ring);

		/* Write path as ``{ <ringnum> }'' */
		sprintf(address, "{%d}", state->ringnum);

		values[0] = address;
		values[1] = lwgeom_to_hexwkb(ringgeom, PARSER_CHECK_NONE, -1);

		MemoryContextSwitchTo(oldcontext);

		tuple = BuildTupleFromCStrings(funcctx->attinmeta, values);
		result = HeapTupleGetDatum(tuple);
		++state->ringnum;
		SRF_RETURN_NEXT(funcctx, result);
	}

	SRF_RETURN_DONE(funcctx);

}

