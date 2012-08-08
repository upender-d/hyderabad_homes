SELECT 'ndims01', ndims(geomfromewkt('CIRCULARSTRING(
                0 0 0 0, 
                0.26794919243112270647255365849413 1 3 -2, 
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2)'));
SELECT 'geometrytype01', geometrytype(geomfromewkt('CIRCULARSTRING(
                0 0 0 0, 
                0.26794919243112270647255365849413 1 3 -2, 
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2)'));
SELECT 'ndims02', ndims(geomfromewkt('CIRCULARSTRING(
                0 0 0, 
                0.26794919243112270647255365849413 1 3, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 1)'));
SELECT 'geometrytype02', geometrytype(geomfromewkt('CIRCULARSTRING(
                0 0 0, 
                0.26794919243112270647255365849413 1 3, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 1)'));
SELECT 'ndims03', ndims(geomfromewkt('CIRCULARSTRINGM(
                0 0 0, 
                0.26794919243112270647255365849413 1 -2, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 2)'));
SELECT 'geometrytype03', geometrytype(geomfromewkt('CIRCULARSTRINGM(
                0 0 0, 
                0.26794919243112270647255365849413 1 -2, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 2)'));
SELECT 'ndims04', ndims(geomfromewkt('CIRCULARSTRING(
                0 0, 
                0.26794919243112270647255365849413 1, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097)'));
SELECT 'geometrytype04', geometrytype(geomfromewkt('CIRCULARSTRING(
                0 0, 
                0.26794919243112270647255365849413 1, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097)'));

SELECT 'isClosed01', isClosed(geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                2 0,
                0 -2)'));
SELECT 'isSimple01', isSimple(geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                2 0,
                0 -2)'));
SELECT 'isRing01', isRing(geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                2 0,
                0 -2)'));
SELECT 'isClosed02', isClosed(geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                -2 0,
                2 -2,
                -2 0,
                -2 -2,
                -2 0,
                0 -2)'));
SELECT 'isSimple02', isSimple(geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                -2 0,
                2 -2,
                -2 0,
                -2 -2,
                -2 0,
                0 -2)'));
SELECT 'isRing02', isRing(geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                -2 0,
                2 -2,
                -2 0,
                -2 -2,
                -2 0,
                0 -2)'));

-- Repeat tests with new function names.
SELECT 'ndims01', ST_ndims(ST_geomfromewkt('CIRCULARSTRING(
                0 0 0 0, 
                0.26794919243112270647255365849413 1 3 -2, 
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2)'));
SELECT 'geometrytype01', geometrytype(ST_geomfromewkt('CIRCULARSTRING(
                0 0 0 0, 
                0.26794919243112270647255365849413 1 3 -2, 
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2)'));
SELECT 'ndims02', ST_ndims(ST_geomfromewkt('CIRCULARSTRING(
                0 0 0, 
                0.26794919243112270647255365849413 1 3, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 1)'));
SELECT 'geometrytype02', geometrytype(ST_geomfromewkt('CIRCULARSTRING(
                0 0 0, 
                0.26794919243112270647255365849413 1 3, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 1)'));
SELECT 'ndims03', ST_ndims(ST_geomfromewkt('CIRCULARSTRINGM(
                0 0 0, 
                0.26794919243112270647255365849413 1 -2, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 2)'));
SELECT 'geometrytype03', geometrytype(ST_geomfromewkt('CIRCULARSTRINGM(
                0 0 0, 
                0.26794919243112270647255365849413 1 -2, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 2)'));
SELECT 'ndims04', ST_ndims(ST_geomfromewkt('CIRCULARSTRING(
                0 0, 
                0.26794919243112270647255365849413 1, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097)'));
SELECT 'geometrytype04', geometrytype(ST_geomfromewkt('CIRCULARSTRING(
                0 0, 
                0.26794919243112270647255365849413 1, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097)'));

SELECT 'isClosed01', ST_isClosed(ST_geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                2 0,
                0 -2)'));
SELECT 'isSimple01', ST_isSimple(ST_geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                2 0,
                0 -2)'));
SELECT 'isRing01', ST_isRing(ST_geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                2 0,
                0 -2)'));
SELECT 'isClosed02', ST_isClosed(ST_geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                -2 0,
                2 -2,
                -2 0,
                -2 -2,
                -2 0,
                0 -2)'));
SELECT 'isSimple02', ST_isSimple(ST_geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                -2 0,
                2 -2,
                -2 0,
                -2 -2,
                -2 0,
                0 -2)'));
SELECT 'isRing02', ST_isRing(ST_geomfromewkt('CIRCULARSTRING(
                0 -2,
                -2 0,
                0 2,
                -2 0,
                2 -2,
                -2 0,
                -2 -2,
                -2 0,
                0 -2)'));

CREATE TABLE public.circularstring (id INTEGER, description VARCHAR);
SELECT AddGeometryColumn('public', 'circularstring', 'the_geom_2d', -1, 'CIRCULARSTRING', 2);
SELECT AddGeometryColumn('public', 'circularstring', 'the_geom_3dm', -1, 'CIRCULARSTRINGM', 3);
SELECT AddGeometryColumn('public', 'circularstring', 'the_geom_3dz', -1, 'CIRCULARSTRING', 3);
SELECT AddGeometryColumn('public', 'circularstring', 'the_geom_4d', -1, 'CIRCULARSTRING', 4);

INSERT INTO public.circularstring (
        id, 
        description
      ) VALUES (
        1, 
        '180-135 degrees');
UPDATE public.circularstring
        SET the_geom_4d = geomfromewkt('CIRCULARSTRING(
                0 0 0 0, 
                0.26794919243112270647255365849413 1 3 -2, 
                0.5857864376269049511983112757903 1.4142135623730950488016887242097 1 2)')
        WHERE id = 1;
UPDATE public.circularstring
        SET the_geom_3dz = geomfromewkt('CIRCULARSTRING(
                0 0 0, 
                0.26794919243112270647255365849413 1 3, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 1)')
        WHERE id = 1;
UPDATE public.circularstring
        SET the_geom_3dm = geomfromewkt('CIRCULARSTRINGM(
                0 0 0, 
                0.26794919243112270647255365849413 1 -2, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097 2)') 
        WHERE id = 1;
UPDATE public.circularstring       
        SET the_geom_2d = geomfromewkt('CIRCULARSTRING(
                0 0, 
                0.26794919243112270647255365849413 1, 
                0.5857864376269049511981127579 1.4142135623730950488016887242097)')
        WHERE id = 1;

INSERT INTO public.circularstring (
        id,
        description
      ) VALUES (
        2,
        '2-segment string');
UPDATE public.circularstring
        SET the_geom_4d = geomfromewkt('CIRCULARSTRING(
                -5 0 0 4, 
                0 5 1 3, 
                5 0 2 2, 
                10 -5 3 1, 
                15 0 4 0)')
        WHERE id = 2;
UPDATE public.circularstring
        SET the_geom_3dz = geomfromewkt('CIRCULARSTRING(
                -5 0 0, 
                0 5 1, 
                5 0 2, 
                10 -5 3, 
                15 0 4)')
        WHERE id = 2;
UPDATE public.circularstring
        SET the_geom_3dm = geomfromewkt('CIRCULARSTRINGM(
                -5 0 4, 
                0 5 3, 
                5 0 2, 
                10 -5 1, 
                15 0 0)')
        WHERE id = 2;
UPDATE public.circularstring
        SET the_geom_2d = geomfromewkt('CIRCULARSTRING(
                -5 0, 
                0 5, 
                5 0, 
                10 -5, 
                15 0)')
        WHERE id = 2;

SELECT 'astext01', astext(the_geom_2d) FROM public.circularstring;        
SELECT 'astext02', astext(the_geom_3dm) FROM public.circularstring;        
SELECT 'astext03', astext(the_geom_3dz) FROM public.circularstring;        
SELECT 'astext04', astext(the_geom_4d) FROM public.circularstring;        

SELECT 'asewkt01', asewkt(the_geom_2d) FROM public.circularstring;        
SELECT 'asewkt02', asewkt(the_geom_3dm) FROM public.circularstring;        
SELECT 'asewkt03', asewkt(the_geom_3dz) FROM public.circularstring;        
SELECT 'asewkt04', asewkt(the_geom_4d) FROM public.circularstring;        

-- These tests will fail on different architectures
-- We need a way to handle multiple byte orderings
--SELECT 'asbinary01', encode(asbinary(the_geom_2d), 'hex') FROM public.circularstring;
--SELECT 'asbinary02', encode(asbinary(the_geom_3dm), 'hex') FROM public.circularstring;
--SELECT 'asbinary03', encode(asbinary(the_geom_3dz), 'hex') FROM public.circularstring;
--SELECT 'asbinary04', encode(asbinary(the_geom_4d), 'hex') FROM public.circularstring;
--
--SELECT 'asewkb01', encode(asewkb(the_geom_2d), 'hex') FROM public.circularstring;
--SELECT 'asewkb02', encode(asewkb(the_geom_3dm), 'hex') FROM public.circularstring;
--SELECT 'asewkb03', encode(asewkb(the_geom_3dz), 'hex') FROM public.circularstring;
--SELECT 'asewkb04', encode(asewkb(the_geom_4d), 'hex') FROM public.circularstring;

SELECT 'ST_CurveToLine-201', asewkt(snapToGrid(ST_CurveToLine(the_geom_2d, 2), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine-202', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dm, 2), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine-203', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dz, 2), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine-204', asewkt(snapToGrid(ST_CurveToLine(the_geom_4d, 2), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;

SELECT 'ST_CurveToLine-401', asewkt(snapToGrid(ST_CurveToLine(the_geom_2d, 4), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine-402', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dm, 4), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine-403', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dz, 4), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine-404', asewkt(snapToGrid(ST_CurveToLine(the_geom_4d, 4), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;

SELECT 'ST_CurveToLine01', asewkt(snapToGrid(ST_CurveToLine(the_geom_2d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine02', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dm), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine03', asewkt(snapToGrid(ST_CurveToLine(the_geom_3dz), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'ST_CurveToLine04', asewkt(snapToGrid(ST_CurveToLine(the_geom_4d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;

--Removed due to discrepencies between hardware
--SELECT 'box2d01', box2d(the_geom_2d) FROM public.circularstring;
--SELECT 'box2d02', box2d(the_geom_3dm) FROM public.circularstring;
--SELECT 'box2d03', box2d(the_geom_3dz) FROM public.circularstring;
--SELECT 'box2d04', box2d(the_geom_4d) FROM public.circularstring;

--SELECT 'box3d01', box3d(the_geom_2d) FROM public.circularstring;
--SELECT 'box3d02', box3d(the_geom_3dm) FROM public.circularstring;
--SELECT 'box3d03', box3d(the_geom_3dz) FROM public.circularstring;
--SELECT 'box3d04', box3d(the_geom_4d) FROM public.circularstring;

SELECT 'isValid01', isValid(the_geom_2d) FROM public.circularstring;
SELECT 'isValid02', isValid(the_geom_3dm) FROM public.circularstring;
SELECT 'isValid03', isValid(the_geom_3dz) FROM public.circularstring;
SELECT 'isValid04', isValid(the_geom_4d) FROM public.circularstring;

SELECT 'dimension01', dimension(the_geom_2d) FROM public.circularstring;
SELECT 'dimension02', dimension(the_geom_3dm) FROM public.circularstring;
SELECT 'dimension03', dimension(the_geom_3dz) FROM public.circularstring;
SELECT 'dimension04', dimension(the_geom_4d) FROM public.circularstring;

SELECT 'SRID01', SRID(the_geom_2d) FROM public.circularstring;
SELECT 'SRID02', SRID(the_geom_3dm) FROM public.circularstring;
SELECT 'SRID03', SRID(the_geom_3dz) FROM public.circularstring;
SELECT 'SRID04', SRID(the_geom_4d) FROM public.circularstring;

SELECT 'accessors01', isEmpty(the_geom_2d), isSimple(the_geom_2d), isClosed(the_geom_2d), isRing(the_geom_2d) FROM public.circularstring;
SELECT 'accessors02', isEmpty(the_geom_3dm), isSimple(the_geom_3dm), isClosed(the_geom_3dm), isRing(the_geom_3dm) FROM public.circularstring;
SELECT 'accessors03', isEmpty(the_geom_3dz), isSimple(the_geom_3dz), isClosed(the_geom_3dz), isRing(the_geom_3dz) FROM public.circularstring;
SELECT 'accessors04', isEmpty(the_geom_4d), isSimple(the_geom_4d), isClosed(the_geom_4d), isRing(the_geom_4d) FROM public.circularstring;

SELECT 'envelope01', asText(snapToGrid(envelope(the_geom_2d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'envelope02', asText(snapToGrid(envelope(the_geom_3dm), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'envelope03', asText(snapToGrid(envelope(the_geom_3dz), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'envelope04', asText(snapToGrid(envelope(the_geom_4d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;

-- TODO: ST_SnapToGrid is required to remove platform dependent precision
-- issues.  Until ST_SnapToGrid is updated to work against curves, these
-- tests cannot be run.
--SELECT 'ST_LineToCurve01', asewkt(ST_LineToCurve(ST_CurveToLine(the_geom_2d))) FROM public.circularstring;
--SELECT 'ST_LineToCurve02', asewkt(ST_LineToCurve(ST_CurveToLine(the_geom_3dm))) FROM public.circularstring;
--SELECT 'ST_LineToCurve03', asewkt(ST_LineToCurve(ST_CurveToLine(the_geom_3dz))) FROM public.circularstring;
--SELECT 'ST_LineToCurve04', asewkt(ST_LineToCurve(ST_CurveToLine(the_geom_4d))) FROM public.circularstring;

-- Repeat tests with new function names.
SELECT 'astext01', ST_astext(the_geom_2d) FROM public.circularstring;        
SELECT 'astext02', ST_astext(the_geom_3dm) FROM public.circularstring;        
SELECT 'astext03', ST_astext(the_geom_3dz) FROM public.circularstring;        
SELECT 'astext04', ST_astext(the_geom_4d) FROM public.circularstring;        

SELECT 'asewkt01', ST_asewkt(the_geom_2d) FROM public.circularstring;        
SELECT 'asewkt02', ST_asewkt(the_geom_3dm) FROM public.circularstring;        
SELECT 'asewkt03', ST_asewkt(the_geom_3dz) FROM public.circularstring;        
SELECT 'asewkt04', ST_asewkt(the_geom_4d) FROM public.circularstring;        

-- These tests will fail on different architectures
-- We need a way to handle multiple byte orderings
--SELECT 'asbinary01', encode(ST_asbinary(the_geom_2d), 'hex') FROM public.circularstring;
--SELECT 'asbinary02', encode(ST_asbinary(the_geom_3dm), 'hex') FROM public.circularstring;
--SELECT 'asbinary03', encode(ST_asbinary(the_geom_3dz), 'hex') FROM public.circularstring;
--SELECT 'asbinary04', encode(ST_asbinary(the_geom_4d), 'hex') FROM public.circularstring;
--
--SELECT 'asewkb01', encode(ST_asewkb(the_geom_2d), 'hex') FROM public.circularstring;
--SELECT 'asewkb02', encode(ST_asewkb(the_geom_3dm), 'hex') FROM public.circularstring;
--SELECT 'asewkb03', encode(ST_asewkb(the_geom_3dz), 'hex') FROM public.circularstring;
--SELECT 'asewkb04', encode(ST_asewkb(the_geom_4d), 'hex') FROM public.circularstring;

--Removed due to discrepencies between hardware
--SELECT 'box2d01', ST_box2d(the_geom_2d) FROM public.circularstring;
--SELECT 'box2d02', ST_box2d(the_geom_3dm) FROM public.circularstring;
--SELECT 'box2d03', ST_box2d(the_geom_3dz) FROM public.circularstring;
--SELECT 'box2d04', ST_box2d(the_geom_4d) FROM public.circularstring;

--SELECT 'box3d01', ST_box3d(the_geom_2d) FROM public.circularstring;
--SELECT 'box3d02', ST_box3d(the_geom_3dm) FROM public.circularstring;
--SELECT 'box3d03', ST_box3d(the_geom_3dz) FROM public.circularstring;
--SELECT 'box3d04', ST_box3d(the_geom_4d) FROM public.circularstring;

SELECT 'isValid01', ST_isValid(the_geom_2d) FROM public.circularstring;
SELECT 'isValid02', ST_isValid(the_geom_3dm) FROM public.circularstring;
SELECT 'isValid03', ST_isValid(the_geom_3dz) FROM public.circularstring;
SELECT 'isValid04', ST_isValid(the_geom_4d) FROM public.circularstring;

SELECT 'dimension01', ST_dimension(the_geom_2d) FROM public.circularstring;
SELECT 'dimension02', ST_dimension(the_geom_3dm) FROM public.circularstring;
SELECT 'dimension03', ST_dimension(the_geom_3dz) FROM public.circularstring;
SELECT 'dimension04', ST_dimension(the_geom_4d) FROM public.circularstring;

SELECT 'SRID01', ST_SRID(the_geom_2d) FROM public.circularstring;
SELECT 'SRID02', ST_SRID(the_geom_3dm) FROM public.circularstring;
SELECT 'SRID03', ST_SRID(the_geom_3dz) FROM public.circularstring;
SELECT 'SRID04', ST_SRID(the_geom_4d) FROM public.circularstring;

SELECT 'accessors01', ST_isEmpty(the_geom_2d), ST_isSimple(the_geom_2d), ST_isClosed(the_geom_2d), ST_isRing(the_geom_2d) FROM public.circularstring;
SELECT 'accessors02', ST_isEmpty(the_geom_3dm), ST_isSimple(the_geom_3dm), ST_isClosed(the_geom_3dm), ST_isRing(the_geom_3dm) FROM public.circularstring;
SELECT 'accessors03', ST_isEmpty(the_geom_3dz), ST_isSimple(the_geom_3dz), ST_isClosed(the_geom_3dz), ST_isRing(the_geom_3dz) FROM public.circularstring;
SELECT 'accessors04', ST_isEmpty(the_geom_4d), ST_isSimple(the_geom_4d), ST_isClosed(the_geom_4d), ST_isRing(the_geom_4d) FROM public.circularstring;

SELECT 'envelope01', ST_asText(ST_snapToGrid(envelope(the_geom_2d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'envelope02', ST_asText(ST_snapToGrid(envelope(the_geom_3dm), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'envelope03', ST_asText(ST_snapToGrid(envelope(the_geom_3dz), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;
SELECT 'envelope04', ST_asText(ST_snapToGrid(envelope(the_geom_4d), 'POINT(0 0 0 0)'::geometry, 1e-8, 1e-8, 1e-8, 1e-8)) FROM public.circularstring;

SELECT DropGeometryColumn('public', 'circularstring', 'the_geom_4d');
SELECT DropGeometryColumn('public', 'circularstring', 'the_geom_3dz');
SELECT DropGeometryColumn('public', 'circularstring', 'the_geom_3dm');
SELECT DropGeometryColumn('public', 'circularstring', 'the_geom_2d');
DROP TABLE public.circularstring;
SELECT ST_asText(ST_box2d('CIRCULARSTRING(220268.439465645 150415.359530563,220227.333322076 150505.561285879,220227.353105332 150406.434743975)'::geometry));
SELECT ST_NumPoints(ST_GeomFromEWKT('CIRCULARSTRING(0 0,2 0, 2 1, 2 3, 4 3)'));

SELECT ST_BOX2D('CIRCULARSTRING(10 2,12 2,14 2)'::geometry);
