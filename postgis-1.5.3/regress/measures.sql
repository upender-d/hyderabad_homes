select '113', area2d('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '114', perimeter2d('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '115', perimeter3d('MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7  0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;


select '116', length2d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select '117', length3d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select '118', length3d('MULTILINESTRING((0 0 0, 1 1 1),(0 0 0, 1 1 1, 2 2 2) )'::GEOMETRY) as value;

select '134', distance('POINT(1 2)', 'POINT(1 2)');
select '135', distance('POINT(5 0)', 'POINT(10 12)');

select '136', distance('POINT(0 0)', translate('POINT(0 0)', 5, 12, 0));

-- postgis-users/2006-May/012174.html
select 'dist', distance(a,b), ST_distance(b,a) from (
	select 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))'::geometry as a,
		'POLYGON((11 0, 11 10, 20 10, 20 0, 11 0),
			(15 5, 15 8, 17 8, 17 5, 15 5))'::geometry as b
	) as foo;

-- Apply the same tests using the new function names.
select '113', ST_area2d('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '114', ST_perimeter2d('MULTIPOLYGON( ((0 0, 10 0, 10 10, 0 10, 0 0)),( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7 , 5 7, 5 5) ) ,( (0 0, 10 0, 10 10, 0 10, 0 0),(5 5, 7 5, 7 7, 5 7, 5 5),(1 1,2 1, 2 2, 1 2, 1 1) ) )'::GEOMETRY) as value;

select '115', ST_perimeter3d('MULTIPOLYGON( ((0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0)),( (0 0 0, 10 0 0, 10 10 0, 0 10 0, 0 0 0),(5 5 0, 7 5 0, 7 7  0, 5 7 0, 5 5 0) ) ,( (0 0 1, 10 0 1, 10 10 1, 0 10 1, 0 0 1),(5 5 1, 7 5 1, 7 7 1, 5 7 1, 5 5 1),(1 1 1,2 1 1, 2 2 1, 1 2 1, 1 1 1) ) )'::GEOMETRY) as value;


select '116', ST_length2d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select '117', ST_length3d('MULTILINESTRING((0 0, 1 1),(0 0, 1 1, 2 2) )'::GEOMETRY) as value;
select '118', ST_length3d('MULTILINESTRING((0 0 0, 1 1 1),(0 0 0, 1 1 1, 2 2 2) )'::GEOMETRY) as value;

select '134', ST_distance('POINT(1 2)', 'POINT(1 2)');
select '135', ST_distance('POINT(5 0)', 'POINT(10 12)');

select '136', ST_distance('POINT(0 0)', ST_translate('POINT(0 0)', 5, 12, 0));


-- postgis-users/2006-May/012174.html
select 'dist', ST_distance(a,b), ST_distance(b,a) from (
	select 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))'::geometry as a,
		'POLYGON((11 0, 11 10, 20 10, 20 0, 11 0),
			(15 5, 15 8, 17 8, 17 5, 15 5))'::geometry as b
	) as foo;


--st_shortestline

select 'st_shortestline_134', st_astext(st_shortestline('POINT(1 2)', 'POINT(1 2)'));
select 'st_shortestline_135', st_astext(st_shortestline('POINT(5 0)', 'POINT(10 12)'));

select 'st_shortestline_136', st_astext(st_shortestline('POINT(0 0)', ST_translate('POINT(0 0)', 5, 12, 0)));

-- postgis-users/2006-May/012174.html
select 'st_shortestline_dist', st_astext(st_shortestline(a,b)), st_astext(st_shortestline(b,a)) from (
	select 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))'::geometry as a,
		'POLYGON((11 0, 12 10, 20 10, 20 0, 11 0),
			(15 5, 15 8, 17 8, 17 5, 15 5))'::geometry as b
	) as foo;


--st_maxdistance

select 'st_maxdistance_134', st_maxdistance('POINT(1 2)', 'POINT(1 2)');
select 'st_maxdistance_135', st_maxdistance('POINT(5 0)', 'POINT(10 12)');

select 'st_maxdistance_136', st_maxdistance('POINT(0 0)', ST_translate('POINT(0 0)', 5, 12, 0));

-- postgis-users/2006-May/012174.html
select 'st_maxdistance_dist', st_maxdistance(a,b), st_maxdistance(b,a) from (
	select 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))'::geometry as a,
		'POLYGON((11 0, 11 10, 20 10, 20 0, 11 0),
			(15 5, 15 8, 17 8, 17 5, 15 5))'::geometry as b
	) as foo;



--st_longestline

select 'st_longestline_134', st_astext(st_longestline('POINT(1 2)', 'POINT(1 2)'));
select 'st_longestline_135', st_astext(st_longestline('POINT(5 0)', 'POINT(10 12)'));

select 'st_longestline_136', st_astext(st_longestline('POINT(0 0)', ST_translate('POINT(0 0)', 5, 12, 0)));

-- postgis-users/2006-May/012174.html
select 'st_longestline_dist', st_astext(st_longestline(a,b)), st_astext(st_longestline(b,a)) from (
	select 'POLYGON((0 0, 0 10, 10 10, 10 0, 0 0))'::geometry as a,
		'POLYGON((11 0, 11 10, 20 10, 20 0, 11 0),
			(15 5, 15 8, 17 8, 17 5, 15 5))'::geometry as b
	) as foo;

	




select 'distancetest1',		
		st_distance(a, b),
			st_maxdistance(a, b),
				st_astext(st_shortestline(a,b)),
					st_astext(st_shortestline(b,a)),
						st_astext(st_longestline(a,b)),
							st_astext(st_longestline(b,a)) from (
	select geomfromtext('MULTILINESTRING((17 16, 16 17, 17 18, 17 17, 17 16), (28 35,29 39, 30 35))') as a,
			geomfromtext('MULTIPOLYGON(((-1 -1, -1 25, 25 25, 25 -1, -1 -1), (14 14,14 19,19 19,19 14,14 14)),
			((33 35,33 40, 35 40, 35 35, 33 35)))') as b
	) as foo;


select  'distancetest2',
	ST_Distance(a, b),
	st_maxdistance(a, b),
	round(st_x(st_startpoint(st_shortestline(a,b)))::numeric, 10),
	round(st_y(st_startpoint(st_shortestline(a,b)))::numeric, 10),
	round(st_x(st_endpoint(st_shortestline(a,b)))::numeric, 10),
	round(st_y(st_endpoint(st_shortestline(a,b)))::numeric, 10),	
	st_astext(st_longestline(a,b)),
	st_astext(st_longestline(b,a)) from (
select 
	geomfromtext('LINESTRING(-40 -20 , 4 2)') as a,
	geomfromtext('LINESTRING(-10 20, 1 -2)') as b
) as foo;
	

select 'distancepoly1',		
		st_distance(a, b),
			st_maxdistance(a, b),
				st_astext(st_shortestline(a,b)),
					st_astext(st_shortestline(b,a)),
						st_astext(st_longestline(a,b)),
							st_astext(st_longestline(b,a)) from (
	select geomfromtext('MULTIPOLYGON(((17 16, 16 17, 17 18, 17 17, 17 16)), ((28 35,29 39, 30 35, 28 35)))') as a,
			geomfromtext('MULTIPOLYGON(((-1 -1, -1 25, 25 25, 25 -1, -1 -1), (14 14,14 19,19 19,19 14,14 14)),
			((33 35,33 40, 35 40, 35 35, 33 35)))') as b
	) as foo;



select 'distancepoly2',		
		st_distance(a, b),
			st_maxdistance(a, b),
				st_astext(st_shortestline(a,b)),
					st_astext(st_shortestline(b,a)),
						st_astext(st_longestline(a,b)),
							st_astext(st_longestline(b,a)) from (
	select geomfromtext('POLYGON((17 14, 16 17, 17 18, 17 17, 17 14))') as a,
			geomfromtext('POLYGON((-1 -1, -1 25, 25 25, 25 -1, -1 -1), (14 14,14 19,19 19,19 14,14 14))') as b
	) as foo;



select 'distancepoly3',		
		st_distance(a, b),
			st_maxdistance(a, b),
				st_astext(st_shortestline(a,b)),
					st_astext(st_shortestline(b,a)),
						st_astext(st_longestline(a,b)),
							st_astext(st_longestline(b,a)) from (
	select geomfromtext('POLYGON((17 16, 16 17, 17 19, 17 17, 17 16))') as a,
			geomfromtext('POLYGON((-1 -1, -1 25, 25 25, 25 -1, -1 -1), (14 14,14 19,19 19,19 14,14 14))') as b
	) as foo;


select 'distancepoly4',		
		st_distance(a, b),
			st_maxdistance(a, b),
				st_astext(st_shortestline(a,b)),
					st_astext(st_shortestline(b,a)),
						st_astext(st_longestline(a,b)),
							st_astext(st_longestline(b,a)) from (
	select geomfromtext('POLYGON((17 16, 16 17, 16 20, 18 20, 18 17, 17 16))') as a,
			geomfromtext('POLYGON((-1 -1, -1 25, 25 25, 25 -1, -1 -1), (14 14,14 19,19 19,19 14,14 14))') as b
	) as foo;



select 'distancepoly5',		
		st_distance(a, b),
			st_maxdistance(a, b),
				st_astext(st_shortestline(a,b)),
					st_astext(st_shortestline(b,a)),
						st_astext(st_longestline(a,b)),
							st_astext(st_longestline(b,a)) from (
	select geomfromtext('POLYGON((17 12, 16 17, 17 18, 17 17, 17 12))') as a,
			geomfromtext('POLYGON((-1 -1, -1 25, 25 25, 25 -1, -1 -1), (14 14,14 19,19 19,19 14,14 14))') as b
	) as foo;




select 'distancepoly6',		
		st_distance(a, b),
			st_maxdistance(a, b),
				st_astext(st_shortestline(a,b)),
					st_astext(st_shortestline(b,a)),
						st_astext(st_longestline(a,b)),
							st_astext(st_longestline(b,a)) from (
	select geomfromtext('POLYGON((2 2, 2 3, 3 3, 3 2, 2 2))') as a,
			geomfromtext('POLYGON((-1 -1, -1 25, 25 25, 25 -1, -1 -1), (14 14,14 19,19 19,19 14,14 14))') as b
	) as foo;


	

-- Area of an empty polygon
select 'emptyPolyArea', st_area('POLYGON EMPTY');

-- Area of an empty linestring
select 'emptyLineArea', st_area('LINESTRING EMPTY');

-- Area of an empty point
select 'emptyPointArea', st_area('POINT EMPTY');

-- Area of an empty multipolygon
select 'emptyMultiPolyArea', st_area('MULTIPOLYGON EMPTY');

-- Area of an empty multilinestring
select 'emptyMultiLineArea', st_area('MULTILINESTRING EMPTY');

-- Area of an empty multilipoint
select 'emptyMultiPointArea', st_area('MULTIPOINT EMPTY');

-- Area of an empty collection
select 'emptyCollectionArea', st_area('GEOMETRYCOLLECTION EMPTY');
