/**********************************************************************
 * $Id: cu_libgeom.c 5181 2010-02-01 17:35:55Z pramsey $
 *
 * PostGIS - Spatial Types for PostgreSQL
 * http://postgis.refractions.net
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "cu_libgeom.h"

/*
** Called from test harness to register the tests in this file.
*/
CU_pSuite register_libgeom_suite(void)
{
	CU_pSuite pSuite;
	pSuite = CU_add_suite("LibGeom Suite", init_libgeom_suite, clean_libgeom_suite);
	if (NULL == pSuite)
	{
		CU_cleanup_registry();
		return NULL;
	}

	if (
	    (NULL == CU_add_test(pSuite, "test_typmod_macros()", test_typmod_macros)) ||
	    (NULL == CU_add_test(pSuite, "test_flags_macros()", test_flags_macros)) ||
	    (NULL == CU_add_test(pSuite, "test_serialized_srid()", test_serialized_srid)) ||
	    (NULL == CU_add_test(pSuite, "test_gserialized_from_lwgeom_size()", test_gserialized_from_lwgeom_size)) ||
	    (NULL == CU_add_test(pSuite, "test_gbox_serialized_size()", test_gbox_serialized_size)) ||
	    (NULL == CU_add_test(pSuite, "test_lwgeom_from_gserialized()", test_lwgeom_from_gserialized))  ||
	    (NULL == CU_add_test(pSuite, "test_lwgeom_count_vertices()", test_lwgeom_count_vertices))  ||
	    (NULL == CU_add_test(pSuite, "test_on_gser_lwgeom_count_vertices()", test_on_gser_lwgeom_count_vertices))  ||
	    (NULL == CU_add_test(pSuite, "test_geometry_type_from_string()", test_geometry_type_from_string)) ||
	    (NULL == CU_add_test(pSuite, "test_lwcollection_extract()", test_lwcollection_extract)) ||
	    (NULL == CU_add_test(pSuite, "test_lwgeom_free()", test_lwgeom_free))

	)
	{
		CU_cleanup_registry();
		return NULL;
	}
	return pSuite;
}

/*
** The suite initialization function.
** Create any re-used objects.
*/
int init_libgeom_suite(void)
{
	return 0;
}

/*
** The suite cleanup function.
** Frees any global objects.
*/
int clean_libgeom_suite(void)
{
	return 0;
}

void test_typmod_macros(void)
{
	uint32 typmod = 0;
	int srid = 4326;
	int type = 6;
	int z = 1;
	int rv;

	TYPMOD_SET_SRID(typmod,srid);
	rv = TYPMOD_GET_SRID(typmod);
	CU_ASSERT_EQUAL(rv, srid);

	TYPMOD_SET_TYPE(typmod,type);
	rv = TYPMOD_GET_TYPE(typmod);
	CU_ASSERT_EQUAL(rv,type);

	TYPMOD_SET_Z(typmod);
	rv = TYPMOD_GET_Z(typmod);
	CU_ASSERT_EQUAL(rv,z);

	rv = TYPMOD_GET_M(typmod);
	CU_ASSERT_EQUAL(rv,0);

}

void test_flags_macros(void)
{
	uchar flags = 0;

	CU_ASSERT_EQUAL(0, FLAGS_GET_Z(flags));
	FLAGS_SET_Z(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_Z(flags));
	FLAGS_SET_Z(flags, 0);
	CU_ASSERT_EQUAL(0, FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_M(flags));
	FLAGS_SET_M(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_M(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_BBOX(flags));
	FLAGS_SET_BBOX(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_BBOX(flags));

	CU_ASSERT_EQUAL(0, FLAGS_GET_GEODETIC(flags));
	FLAGS_SET_GEODETIC(flags, 1);
	CU_ASSERT_EQUAL(1, FLAGS_GET_GEODETIC(flags));

	flags = 0;
	flags = gflags(1, 0, 1); /* z=1, m=0, geodetic=1 */

	CU_ASSERT_EQUAL(1, FLAGS_GET_GEODETIC(flags));
	CU_ASSERT_EQUAL(1, FLAGS_GET_Z(flags));
	CU_ASSERT_EQUAL(0, FLAGS_GET_M(flags));
}

void test_serialized_srid(void)
{
	GSERIALIZED s;
	uint32 srid = 4326;
	uint32 rv;

	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 0;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);

	srid = 1000000;
	gserialized_set_srid(&s, srid);
	rv = gserialized_get_srid(&s);
	CU_ASSERT_EQUAL(rv, srid);
}

void test_gserialized_from_lwgeom_size(void)
{
	LWGEOM *g;
	size_t size = 0;

	g = lwgeom_from_ewkt("POINT(0 0)", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 32 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("POINT(0 0 0)", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 40 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("MULTIPOINT(0 0 0, 1 1 1)", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 80 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("LINESTRING(0 0, 1 1)", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 48 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("MULTILINESTRING((0 0, 1 1),(0 0, 1 1))", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 96 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("POLYGON((0 0, 0 1, 1 1, 1 0, 0 0))", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 104 );
	lwgeom_free(g);

	g = lwgeom_from_ewkt("POLYGON((-1 -1, -1 2, 2 2, 2 -1, -1 -1), (0 0, 0 1, 1 1, 1 0, 0 0))", PARSER_CHECK_NONE);
	size = gserialized_from_lwgeom_size(g);
	CU_ASSERT_EQUAL( size, 184 );
	lwgeom_free(g);

}

void test_gbox_serialized_size(void)
{
	uchar flags = gflags(0, 0, 0);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),0);
	FLAGS_SET_BBOX(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),16);
	FLAGS_SET_Z(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),24);
	FLAGS_SET_M(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),32);
	FLAGS_SET_GEODETIC(flags, 1);
	CU_ASSERT_EQUAL(gbox_serialized_size(flags),24);

}




void test_lwgeom_from_gserialized(void)
{
	LWGEOM *geom;
	GSERIALIZED *g;
	char *in_ewkt;
	char *out_ewkt;
	int i = 0;

	char ewkt[][512] =
	{
		"POINT(0 0.2)",
		"LINESTRING(-1 -1,-1 2.5,2 2,2 -1)",
		"MULTIPOINT(0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9,0.9 0.9)",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"SRID=1;MULTILINESTRING((-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))",
		"POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0))",
		"SRID=4326;POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))",
		"SRID=100000;POLYGON((-1 -1 3,-1 2.5 3,2 2 3,2 -1 3,-1 -1 3),(0 0 3,0 1 3,1 1 3,1 0 3,0 0 3),(-0.5 -0.5 3,-0.5 -0.4 3,-0.4 -0.4 3,-0.4 -0.5 3,-0.5 -0.5 3))",
		"SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))",
		"SRID=4326;GEOMETRYCOLLECTION(POINT(0 1),POLYGON((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0)),MULTIPOLYGON(((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5))))",
		"MULTICURVE((5 5 1 3,3 5 2 2,3 3 3 1,0 3 1 1),CIRCULARSTRING(0 0 0 0,0.26794 1 3 -2,0.5857864 1.414213 1 2))",
		"MULTISURFACE(CURVEPOLYGON(CIRCULARSTRING(-2 0,-1 -1,0 0,1 -1,2 0,0 2,-2 0),(-1 0,0 0.5,1 0,0 1,-1 0)),((7 8,10 10,6 14,4 11,7 8)))",
	};

	for ( i = 0; i < 13; i++ )
	{
		in_ewkt = ewkt[i];
		geom = lwgeom_from_ewkt(in_ewkt, PARSER_CHECK_NONE);
		g = gserialized_from_lwgeom(geom, 0, 0);
		lwgeom_free(geom);
		geom = lwgeom_from_gserialized(g);
		out_ewkt = lwgeom_to_ewkt(geom, PARSER_CHECK_NONE);
		//printf("\n in = %s\nout = %s\n", in_ewkt, out_ewkt);
		CU_ASSERT_STRING_EQUAL(in_ewkt, out_ewkt);
		lwgeom_release(geom);
		lwfree(g);
		lwfree(out_ewkt);
	}

}

void test_geometry_type_from_string(void)
{
	int rv;
	int type = 0, z = 0, m = 0;
	char *str;

	str = "  POINTZ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_SUCCESS);
	CU_ASSERT_EQUAL(type, POINTTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 0);

	str = "LINESTRINGM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_SUCCESS);
	CU_ASSERT_EQUAL(type, LINETYPE);
	CU_ASSERT_EQUAL(z, 0);
	CU_ASSERT_EQUAL(m, 1);

	str = "MULTIPOLYGONZM";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_SUCCESS);
	CU_ASSERT_EQUAL(type, MULTIPOLYGONTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 1);

	str = "  GEOMETRYCOLLECTIONZM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_SUCCESS);
	CU_ASSERT_EQUAL(type, COLLECTIONTYPE);
	CU_ASSERT_EQUAL(z, 1);
	CU_ASSERT_EQUAL(m, 1);

	str = "  GEOMERYCOLLECTIONZM ";
	rv = geometry_type_from_string(str, &type, &z, &m);
	//printf("\n in type: %s\nout type: %d\n out z: %d\n out m: %d", str, type, z, m);
	CU_ASSERT_EQUAL(rv, G_FAILURE);

}

void test_lwgeom_count_vertices(void)
{
	LWGEOM *geom;

	geom = lwgeom_from_ewkt("MULTIPOINT(-1 -1,-1 2.5,2 2,2 -1)", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),4);
	lwgeom_free(geom);

	geom = lwgeom_from_ewkt("SRID=1;MULTILINESTRING((-1 -131,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1),(-1 -1,-1 2.5,2 2,2 -1))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),16);
	lwgeom_free(geom);

	geom = lwgeom_from_ewkt("SRID=4326;MULTIPOLYGON(((-1 -1,-1 2.5,211 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)),((-1 -1,-1 2.5,2 2,2 -1,-1 -1),(0 0,0 1,1 1,1 0,0 0),(-0.5 -0.5,-0.5 -0.4,-0.4 -0.4,-0.4 -0.5,-0.5 -0.5)))", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(geom),30);
	lwgeom_free(geom);

}

void test_on_gser_lwgeom_count_vertices(void)
{
	LWGEOM *lwgeom;
	GSERIALIZED *g_ser1;
	size_t ret_size;

	lwgeom = lwgeom_from_ewkt("MULTIPOINT(-1 -1,-1 2.5,2 2,2 -1,1 1,2 2,4 5)", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	g_ser1 = gserialized_from_lwgeom(lwgeom, 1, &ret_size);
	FLAGS_SET_GEODETIC(g_ser1->flags, 1);
	lwgeom_free(lwgeom);

	lwgeom = lwgeom_from_gserialized(g_ser1);
	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	lwgeom_release(lwgeom);

	lwgeom = lwgeom_from_gserialized(g_ser1);

	CU_ASSERT_EQUAL(lwgeom_count_vertices(lwgeom),7);
	lwgeom_release(lwgeom);

	lwfree(g_ser1);

}

void test_lwcollection_extract(void)
{

	LWGEOM *geom;
	LWCOLLECTION *col;

	geom = lwgeom_from_ewkt("GEOMETRYCOLLECTION(POINT(0 0))", PARSER_CHECK_NONE);
	col = lwcollection_extract((LWCOLLECTION*)geom, 1);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(col->type), MULTIPOINTTYPE);

	lwfree(col);
	lwgeom_free(geom);

}

void test_lwgeom_free(void)
{
	LWGEOM *geom;

	/* Empty geometries don't seem to free properly (#370) */
	geom = lwgeom_from_ewkt("GEOMETRYCOLLECTION EMPTY", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), COLLECTIONTYPE);
	lwgeom_free(geom);

	/* Empty geometries don't seem to free properly (#370) */
	geom = lwgeom_from_ewkt("POLYGON EMPTY", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), COLLECTIONTYPE);
	lwgeom_free(geom);

	/* Empty geometries don't seem to free properly (#370) */
	geom = lwgeom_from_ewkt("LINESTRING EMPTY", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), COLLECTIONTYPE);
	lwgeom_free(geom);

	/* Empty geometries don't seem to free properly (#370) */
	geom = lwgeom_from_ewkt("POINT EMPTY", PARSER_CHECK_NONE);
	CU_ASSERT_EQUAL(TYPE_GETTYPE(geom->type), COLLECTIONTYPE);
	lwgeom_free(geom);

}
