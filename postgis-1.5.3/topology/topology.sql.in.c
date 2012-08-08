-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
-- 
-- $Id: topology.sql.in.c 3687 2009-02-11 18:28:53Z pramsey $
--
-- PostGIS - Spatial Types for PostgreSQL
-- http://postgis.refractions.net
-- Copyright 2005 Refractions Research Inc.
--
-- This is free software; you can redistribute and/or modify it under
-- the terms of the GNU General Public Licence. See the COPYING file.
--
-- Author: Sandro Santilli <strk@refractions.net>
--  
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- STATUS:
--
--	All objects are created in the 'topology' schema.
--
--	We have PostGIS-specific objects and SQL/MM objects.
--	PostGIS-specific objects have no prefix, SQL/MM ones
--	have the ``ST_'' prefix.
--	
-- [PostGIS-specific]
--
--   TABLE topology
--	Table storing topology info (name, srid, precision)
--
--   TYPE TopoGeometry
--	Complex type storing topology_id, layer_id, geometry type
--	and topogeometry id.
--
--   DOMAIN TopoElement
--	An array of two elements: element_id and element_type.
--	In fact, an array of integers.
--
--   DOMAIN TopoElementArray
--	An array of element_id,element_type values.
--	In fact, a bidimensional array of integers:
--	'{{id,type}, {id,type}, ...}'
--
--   FUNCTION CreateTopology(name, [srid], [precision])
--	Initialize a new topology (creating schema with
--	edge,face,node,relation) and add a record into
--	the topology.topology table.
--	TODO: add triggers (or rules, or whatever) enforcing
--	      precision to the edge and node tables.
--  
--   FUNCTION DropTopology(name)
--  	Delete a topology removing reference from the
--  	topology.topology table
--
--   FUNCTION GetTopologyId(name)
--   FUNCTION GetTopologyName(id)
--	Return info about a Topology
--
--   FUNCTION AddTopoGeometryColumn(toponame, schema, table, column, geomtype)
--      Add a TopoGeometry column to a table, making it a topology layer.
--      Returns created layer id.
--
--   FUNCTION DropTopoGeometryColumn(schema, table, column)
--  	Drop a TopoGeometry column, unregister the associated layer,
-- 	cleanup the relation table.
--
--   FUNCTION CreateTopoGeom(toponame, geomtype, layer_id, topo_objects)
--  	Create a TopoGeometry object from existing Topology elements.
--	The "topo_objects" parameter is of TopoElementArray type.
--
--   FUNCTION GetTopoGeomElementArray(toponame, layer_id, topogeom_id)
--   FUNCTION GetTopoGeomElementArray(TopoGeometry)
--  	Returns a TopoElementArray object containing the topological
--	elements of the given TopoGeometry.
--
--   FUNCTION GetTopoGeomElements(toponame, layer_id, topogeom_id)
--   FUNCTION GetTopoGeomElements(TopoGeometry)
--  	Returns a set of TopoElement objects containing the
--	topological elements of the given TopoGeometry (primitive
--	elements)
--
--   FUNCTION ValidateTopology(toponame)
--  	Run validity checks on the topology, returning, for each
--	detected error, a 3-columns row containing error string
--	and references to involved topo elements: error, id1, id2
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--  Overloaded functions for TopoGeometry inputs
--
--   FUNCTION intersects(TopoGeometry, TopoGeometry)
--   FUNCTION equals(TopoGeometry, TopoGeometry)
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
--   FUNCTION TopoGeo_AddPoint(toponame, point)
--	Add a Point geometry to the topology
--	TODO: accept a topology/layer id
--	      rework to use existing node if existent
--
--   FUNCTION TopoGeo_AddLinestring(toponame, line)
--	Add a LineString geometry to the topology
--	TODO: accept a topology/layer id
--	      rework to use existing nodes/edges
--	      splitting them if required
--
--   FUNCTION TopoGeo_AddPolygon(toponame, polygon)
--	Add a Polygon geometry to the topology
--	TODO: implement
--
--   TYPE GetFaceEdges_ReturnType
--	Complex type used to return tuples from ST_GetFaceEdges
--
-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--
-- [SQL/MM]
--
--   ST_InitTopoGeo
--  	Done, can be modified to include explicit sequences or
--  	more constraints. Very noisy due to implicit index creations
--  	for primary keys and sequences for serial fields...
--  
--   ST_CreateTopoGeo
--  	Being working on. TODO: continue implementation
--  
--   ST_AddIsoNode
--  	Complete
--  
--   ST_RemoveIsoNode
--  	Complete
--  
--   ST_MoveIsoNode
--  	Complete
--  
--   ST_AddIsoEdge
--  	Complete
--
--   ST_RemoveIsoEdge
--	Complete, exceptions untested
--  
--   ST_ChangeEdgeGeom
--  	Complete
--
--   ST_NewEdgesSplit
--	Complete
--	this also updates the Relation table
--	TODO: add entries to the History table ?
--
--   ST_ModEdgesSplit
--	Complete
--	this also updates the Relation table
--	TODO: add entries to the History table ?
--
--   ST_AddEdgeNewFaces
--	Being working on. TODO: continue
--  
--   ST_GetFaceEdges
--  	Unimplemented (C seems appropriate)
--  
--   ST_GetFaceGeometry
--  	Implemented using polygonize()
--  
--  

#include "../postgis/sqldefines.h"

DROP SCHEMA topology CASCADE;

BEGIN;

CREATE SCHEMA topology;

--={ ----------------------------------------------------------------
--  POSTGIS-SPECIFIC block
--
--  This part contains function NOT in the SQL/MM specification
--
---------------------------------------------------------------------

--
-- Topology table.
-- Stores id,name,precision and SRID of topologies.
--
CREATE TABLE topology.topology (
	id SERIAL NOT NULL PRIMARY KEY,
	name VARCHAR NOT NULL UNIQUE,
	SRID INTEGER NOT NULL,
	precision FLOAT8 NOT NULL
);

--{ LayerTrigger()
--
-- Layer integrity trigger
--
CREATEFUNCTION topology.LayerTrigger()
	RETURNS trigger
AS
'
DECLARE
	rec RECORD;
	ok BOOL;
	toponame varchar;
	query TEXT;
BEGIN

	--RAISE NOTICE ''LayerTrigger called % % at % level'', TG_WHEN, TG_OP, TG_LEVEL;


	IF TG_OP = ''INSERT'' THEN
		RAISE EXCEPTION ''LayerTrigger not meant to be called on INSERT'';
	ELSIF TG_OP = ''UPDATE'' THEN
		RAISE EXCEPTION ''The topology.layer table cannot be updated'';
	END IF;


	-- Check for existance of any feature column referencing
	-- this layer
	FOR rec IN SELECT * FROM pg_namespace n, pg_class c, pg_attribute a
		WHERE text(n.nspname) = OLD.schema_name
		AND c.relnamespace = n.oid
		AND text(c.relname) = OLD.table_name
		AND a.attrelid = c.oid
		AND text(a.attname) = OLD.feature_column
	LOOP
		query = ''SELECT * ''
			|| '' FROM '' || quote_ident(OLD.schema_name)
			|| ''.'' || quote_ident(OLD.table_name)
			|| '' WHERE layer_id(''
			|| quote_ident(OLD.feature_column)||'') ''
			|| ''='' || OLD.layer_id
			|| '' LIMIT 1'';
		--RAISE NOTICE ''%'', query;
		FOR rec IN EXECUTE query
		LOOP
			RAISE NOTICE ''A feature referencing layer % of topology % still exists in %.%.%'', OLD.layer_id, OLD.topology_id, OLD.schema_name, OLD.table_name, OLD.feature_column;
			RETURN NULL;
		END LOOP;
	END LOOP;


	-- Get topology name
	SELECT name FROM topology.topology INTO toponame
		WHERE id = OLD.topology_id;

	IF toponame IS NULL THEN
		RAISE NOTICE ''Could not find name of topology with id %'',
			OLD.layer_id;
	END IF;

	-- Check if any record in the relation table references this layer
	FOR rec IN SELECT * FROM pg_namespace
		WHERE text(nspname) = toponame
	LOOP
		query = ''SELECT * ''
			|| '' FROM '' || quote_ident(toponame)
			|| ''.relation ''
			|| '' WHERE topogeo_id = '' || OLD.topology_id 
			|| '' AND layer_id = ''|| OLD.layer_id
			|| '' LIMIT 1'';
		--RAISE NOTICE ''%'', query;
		FOR rec IN EXECUTE query
		LOOP
			RAISE NOTICE ''A record in %.relation still references layer %'', toponame, OLD.layer_id;
			RETURN NULL;
		END LOOP;
	END LOOP;

	RETURN OLD;
END;
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
--} LayerTrigger()


--{
-- Layer table.
-- Stores topology layer informations
--
CREATE TABLE topology.layer (
	topology_id INTEGER NOT NULL
		REFERENCES topology.topology(id),
	layer_id integer NOT NULL,
	schema_name VARCHAR NOT NULL,
	table_name VARCHAR NOT NULL,
	feature_column VARCHAR NOT NULL,
	feature_type VARCHAR NOT NULL,
	level INTEGER NOT NULL DEFAULT 0,
	child_id INTEGER DEFAULT NULL,
	UNIQUE(schema_name, table_name, feature_column),
	PRIMARY KEY(topology_id, layer_id)
);

CREATE TRIGGER layer_integrity_checks BEFORE UPDATE OR DELETE
ON topology.layer FOR EACH ROW EXECUTE PROCEDURE topology.LayerTrigger();

--} Layer table.

--
-- Type returned by ST_GetFaceEdges
--
CREATE TYPE topology.GetFaceEdges_ReturnType AS (
	sequence integer,
	edge integer
);

--
-- Type returned by ValidateTopology 
--
CREATE TYPE topology.ValidateTopology_ReturnType AS (
	error varchar,
	id1 integer,
	id2 integer
);

--
-- TopoGeometry type
--
CREATE TYPE topology.TopoGeometry AS (
	topology_id integer,
	layer_id integer,
	id integer,
	type integer -- 1: [multi]point, 2: [multi]line,
	             -- 3: [multi]polygon, 4: collection
);

--
-- TopoElement domain
--
CREATE DOMAIN topology.TopoElement AS integer[]
	CONSTRAINT DIMENSIONS CHECK (
		array_upper(VALUE, 2) IS NULL
		AND array_upper(VALUE, 1) = 2
	);
--
-- TopoElementArray domain
--
CREATE DOMAIN topology.TopoElementArray AS integer[][]
	CONSTRAINT DIMENSIONS CHECK (
		array_upper(VALUE, 2) IS NOT NULL
		AND array_upper(VALUE, 2) = 2
		AND array_upper(VALUE, 3) IS NULL
	);

--
-- TopoGeomElementArray domain
--
CREATE DOMAIN topology.TopoGeomElementArray AS integer[][]
	CONSTRAINT DIMENSIONS CHECK (
		array_upper(VALUE, 2) IS NOT NULL
		AND array_upper(VALUE, 2) = 2
		AND array_upper(VALUE, 3) IS NULL
	);

--{ RelationTrigger()
--
-- Relation integrity trigger
--
CREATEFUNCTION topology.RelationTrigger()
	RETURNS trigger
AS
'
DECLARE
	toponame varchar;
	topoid integer;
	plyr RECORD; -- parent layer
	rec RECORD;
	ok BOOL;

BEGIN
	IF TG_NARGS != 2 THEN
		RAISE EXCEPTION ''RelationTrigger called with wrong number of arguments'';
	END IF;

	topoid = TG_ARGV[0];
	toponame = TG_ARGV[1];

	--RAISE NOTICE ''RelationTrigger called % % on %.relation for a %'', TG_WHEN, TG_OP, toponame, TG_LEVEL;


	IF TG_OP = ''DELETE'' THEN
		RAISE EXCEPTION ''RelationTrigger not meant to be called on DELETE'';
	END IF;

	-- Get layer info (and verify it exists)
	ok = false;
	FOR plyr IN EXECUTE ''SELECT * FROM topology.layer ''
		|| ''WHERE ''
		|| '' topology_id = '' || topoid
		|| '' AND''
		|| '' layer_id = '' || NEW.layer_id
	LOOP
		ok = true;
		EXIT;
	END LOOP;
	IF NOT ok THEN
		RAISE EXCEPTION ''Layer % does not exist in topology %'',
			NEW.layer_id, topoid;
		RETURN NULL;
	END IF;

	IF plyr.level > 0 THEN -- this is hierarchical layer

		-- ElementType must be the layer child id
		IF NEW.element_type != plyr.child_id THEN
			RAISE EXCEPTION ''Type of elements in layer % must be set to its child layer id %'', plyr.layer_id, plyr.child_id;
			RETURN NULL;
		END IF;

		-- ElementId must be an existent TopoGeometry in child layer
		ok = false;
		FOR rec IN EXECUTE ''SELECT topogeo_id FROM ''
			|| quote_ident(toponame) || ''.relation ''
			|| '' WHERE layer_id = '' || plyr.child_id 
			|| '' AND topogeo_id = '' || NEW.element_id
		LOOP
			ok = true;
			EXIT;
		END LOOP;
		IF NOT ok THEN
			RAISE EXCEPTION ''TopoGeometry % does not exist in the child layer %'', NEW.element_id, plyr.child_id;
			RETURN NULL;
		END IF;

	ELSE -- this is a basic layer

		-- ElementType must be compatible with layer type
		IF plyr.feature_type != 4
			AND plyr.feature_type != NEW.element_type
		THEN
			RAISE EXCEPTION ''Element of type % is not compatible with layer of type %'', NEW.element_type, plyr.feature_type;
			RETURN NULL;
		END IF;

		--
		-- Now lets see if the element is consistent, which
		-- is it exists in the topology tables.
		--

		--
		-- Element is a Node
		--
		IF NEW.element_type = 1 
		THEN
			ok = false;
			FOR rec IN EXECUTE ''SELECT node_id FROM ''
				|| quote_ident(toponame) || ''.node ''
				|| '' WHERE node_id = '' || NEW.element_id
			LOOP
				ok = true;
				EXIT;
			END LOOP;
			IF NOT ok THEN
				RAISE EXCEPTION ''Node % does not exist in topology %'', NEW.element_id, toponame;
				RETURN NULL;
			END IF;

		--
		-- Element is an Edge
		--
		ELSIF NEW.element_type = 2 
		THEN
			ok = false;
			FOR rec IN EXECUTE ''SELECT edge_id FROM ''
				|| quote_ident(toponame) || ''.edge_data ''
				|| '' WHERE edge_id = '' || abs(NEW.element_id)
			LOOP
				ok = true;
				EXIT;
			END LOOP;
			IF NOT ok THEN
				RAISE EXCEPTION ''Edge % does not exist in topology %'', NEW.element_id, toponame;
				RETURN NULL;
			END IF;

		--
		-- Element is a Face
		--
		ELSIF NEW.element_type = 3 
		THEN
			IF NEW.element_id = 0 THEN
				RAISE EXCEPTION ''Face % cannot be associated with any feature'', NEW.element_id;
				RETURN NULL;
			END IF;
			ok = false;
			FOR rec IN EXECUTE ''SELECT face_id FROM ''
				|| quote_ident(toponame) || ''.face ''
				|| '' WHERE face_id = '' || NEW.element_id
			LOOP
				ok = true;
				EXIT;
			END LOOP;
			IF NOT ok THEN
				RAISE EXCEPTION ''Face % does not exist in topology %'', NEW.element_id, toponame;
				RETURN NULL;
			END IF;
		END IF;

	END IF;
	
	RETURN NEW;
END;
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
--} RelationTrigger()

--{
--  AddTopoGeometryColumn(toponame, schema, table, colum, type, [child])
--
--  Add a TopoGeometry column to a table, making it a topology layer.
--  Returns created layer id.
--
--
CREATEFUNCTION topology.AddTopoGeometryColumn(varchar, varchar, varchar, varchar, varchar, integer)
	RETURNS integer
AS '
DECLARE
	toponame alias for $1;
	schema alias for $2;
	table alias for $3;
	col alias for $4;
	ltype alias for $5;
	child alias for $6;
	intltype integer;
	level integer;
	topoid integer;
	rec RECORD;
	layer_id integer;
	query text;
BEGIN

        -- Get topology id
        SELECT id FROM topology.topology into topoid
                WHERE name = toponame;

	IF topoid IS NULL THEN
		RAISE EXCEPTION ''Topology % does not exist'', toponame;
	END IF;

	--
	-- Get new layer id from sequence
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		quote_ident(toponame) || ''.layer_id_seq'''')''
	LOOP
		layer_id = rec.nextval;
	END LOOP;

	IF ltype = ''POINT'' THEN
		intltype = 1;
	ELSIF ltype = ''LINE'' THEN
		intltype = 2;
	ELSIF ltype = ''POLYGON'' THEN
		intltype = 3;
	ELSIF ltype = ''COLLECTION'' THEN
		intltype = 4;
	ELSE
		RAISE EXCEPTION ''Layer type must be one of POINT,LINE,POLYGON,COLLECTION'';
	END IF;

	--
	-- See if child id exists and extract its level
	--
	IF child IS NULL THEN
		EXECUTE ''INSERT INTO ''
			|| ''topology.layer(topology_id, ''
			|| ''layer_id, schema_name, ''
			|| ''table_name, feature_column, feature_type) ''
			|| ''VALUES (''
			|| topoid || '',''
			|| layer_id || '','' 
			|| quote_literal(schema) || '',''
			|| quote_literal(table) || '',''
			|| quote_literal(col) || '',''
			|| intltype || '');'';
	ELSE
		FOR rec IN EXECUTE ''SELECT level FROM topology.layer''
			|| '' WHERE layer_id = '' || child
		LOOP
			level = rec.level + 1;
		END LOOP;

		EXECUTE ''INSERT INTO '' 
			|| ''topology.layer(topology_id, ''
			|| ''layer_id, level, child_id, schema_name, ''
			|| ''table_name, feature_column, feature_type) ''
			|| ''VALUES (''
			|| topoid || '',''
			|| layer_id || '','' || level || '',''
			|| child || '',''
			|| quote_literal(schema) || '',''
			|| quote_literal(table) || '',''
			|| quote_literal(col) || '',''
			|| intltype || '');'';
	END IF;


	--
	-- Create a sequence for TopoGeometries in this new layer
	--
	EXECUTE ''CREATE SEQUENCE '' || quote_ident(toponame)
		|| ''.topogeo_s_'' || layer_id;

	--
	-- Add new TopoGeometry column in schema.table
	--
	EXECUTE ''ALTER TABLE '' || quote_ident(schema)
		|| ''.'' || quote_ident(table) 
		|| '' ADD COLUMN '' || quote_ident(col)
		|| '' topology.TopoGeometry;'';

	--
	-- Add constraints on TopoGeom column
	--
	EXECUTE ''ALTER TABLE '' || quote_ident(schema)
		|| ''.'' || quote_ident(table) 
		|| '' ADD CONSTRAINT check_topogeom CHECK (''
		|| ''topology_id('' || quote_ident(col) || '') = '' || topoid
		|| '' AND ''
		|| ''layer_id('' || quote_ident(col) || '') = '' || layer_id
		|| '' AND ''
		|| ''type('' || quote_ident(col) || '') = '' || intltype
		|| '');'';

	--
	-- Add dependency of the feature column on the topology schema
	--
	query = ''INSERT INTO pg_catalog.pg_depend SELECT ''
		|| ''fcat.oid, fobj.oid, fsub.attnum, tcat.oid, ''
		|| ''tobj.oid, 0, ''''n'''' ''
		|| ''FROM pg_class fcat, pg_namespace fnsp, ''
		|| '' pg_class fobj, pg_attribute fsub, ''
		|| '' pg_class tcat, pg_namespace tobj ''
		|| '' WHERE fcat.relname = ''''pg_class'''' ''
		|| '' AND fnsp.nspname = '' || quote_literal(schema)
		|| '' AND fobj.relnamespace = fnsp.oid ''
		|| '' AND fobj.relname = '' || quote_literal(table)
		|| '' AND fsub.attrelid = fobj.oid ''
		|| '' AND fsub.attname = '' || quote_literal(col)
		|| '' AND tcat.relname = ''''pg_namespace'''' ''
		|| '' AND tobj.nspname = '' || quote_literal(toponame);

--
-- The only reason to add this dependency is to avoid
-- simple drop of a feature column. Still, drop cascade
-- will remove both the feature column and the sequence
-- corrupting the topology anyway ...
--
#if 0
	--
	-- Add dependency of the topogeom sequence on the feature column 
	-- This is a dirty hack ...
	--
	query = ''INSERT INTO pg_catalog.pg_depend SELECT ''
		|| ''scat.oid, sobj.oid, 0, fcat.oid, ''
		|| ''fobj.oid, fsub.attnum, ''''n'''' ''
		|| ''FROM pg_class fcat, pg_namespace fnsp, ''
		|| '' pg_class fobj, pg_attribute fsub, ''
		|| '' pg_class scat, pg_class sobj, ''
		|| '' pg_namespace snsp ''
		|| '' WHERE fcat.relname = ''''pg_class'''' ''
		|| '' AND fnsp.nspname = '' || quote_literal(schema)
		|| '' AND fobj.relnamespace = fnsp.oid ''
		|| '' AND fobj.relname = '' || quote_literal(table)
		|| '' AND fsub.attrelid = fobj.oid ''
		|| '' AND fsub.attname = '' || quote_literal(col)
		|| '' AND scat.relname = ''''pg_class'''' ''
		|| '' AND snsp.nspname = '' || quote_literal(toponame)
		|| '' AND sobj.relnamespace = snsp.oid '' 
		|| '' AND sobj.relname = ''
		|| '' ''''topogeo_s_'' || layer_id || '''''' '';

	RAISE NOTICE ''%'', query;
	EXECUTE query;
#endif

	RETURN layer_id;
END;
'
LANGUAGE 'plpgsql' _VOLATILE;

CREATEFUNCTION topology.AddTopoGeometryColumn(varchar, varchar, varchar, varchar, varchar)
	RETURNS integer
AS '
	SELECT topology.AddTopoGeometryColumn($1, $2, $3, $4, $5, NULL);
'
LANGUAGE 'sql' _VOLATILE;

--
--} AddTopoGeometryColumn

--{
--  DropTopoGeometryColumn(schema, table, colum)
--
--  Drop a TopoGeometry column, unregister the associated layer,
--  cleanup the relation table.
--
--
CREATEFUNCTION topology.DropTopoGeometryColumn(varchar, varchar, varchar)
	RETURNS text
AS '
DECLARE
	schema alias for $1;
	table alias for $2;
	col alias for $3;
	rec RECORD;
	lyrinfo RECORD;
	ok BOOL;
	result text;
BEGIN

        -- Get layer and topology info
	ok = false;
	FOR rec IN EXECUTE ''SELECT t.name as toponame, l.* FROM ''
		|| ''topology.topology t, topology.layer l ''
		|| '' WHERE l.schema_name = '' || quote_literal(schema)
		|| '' AND l.table_name = '' || quote_literal(table)
		|| '' AND l.feature_column = '' || quote_literal(col)
	LOOP
		ok = true;
		lyrinfo = rec;
	END LOOP;

	-- Layer not found
	IF NOT ok THEN
		RAISE EXCEPTION ''No layer registered on %.%.%'',
			schema,table,col;
	END IF;
		
	-- Clean up the topology schema
	FOR rec IN SELECT * FROM pg_namespace
		WHERE text(nspname) = lyrinfo.toponame
	LOOP
		-- Cleanup the relation table
		EXECUTE ''DELETE FROM '' || quote_ident(lyrinfo.toponame)
			|| ''.relation ''
			|| '' WHERE ''
			|| ''layer_id = '' || lyrinfo.layer_id;

		-- Drop the sequence for topogeoms in this layer
		EXECUTE ''DROP SEQUENCE '' || quote_ident(lyrinfo.toponame)
			|| ''.topogeo_s_'' || lyrinfo.layer_id;

	END LOOP;

	ok = false;
	FOR rec IN SELECT * FROM pg_namespace n, pg_class c, pg_attribute a
		WHERE text(n.nspname) = schema
		AND c.relnamespace = n.oid
		AND text(c.relname) = table
		AND a.attrelid = c.oid
		AND text(a.attname) = col
	LOOP
		ok = true;
		EXIT;
	END LOOP;


	IF ok THEN
		-- Set feature column to NULL to bypass referential integrity
		-- checks
		EXECUTE ''UPDATE '' || quote_ident(schema) || ''.''
			|| quote_ident(table)
			|| '' SET '' || quote_ident(col)
			|| '' = NULL'';
	END IF;

	-- Delete the layer record
	EXECUTE ''DELETE FROM topology.layer ''
		|| '' WHERE topology_id = '' || lyrinfo.topology_id
		|| '' AND layer_id = '' || lyrinfo.layer_id;

	IF ok THEN
		-- Drop the layer column
		EXECUTE ''ALTER TABLE '' || quote_ident(schema) || ''.''
			|| quote_ident(table)
			|| '' DROP '' || quote_ident(col)
			|| '' cascade'';
	END IF;

	result = ''Layer '' || lyrinfo.layer_id || '' (''
		|| schema || ''.'' || table || ''.'' || col
		|| '') dropped'';

	RETURN result;
END;
'
LANGUAGE 'plpgsql' _VOLATILE;
--
--} DropTopoGeometryColumn


--{
-- CreateTopoGeom(topology_name, topogeom_type, layer_id, elements)
--
-- Create a TopoGeometry object from Topology elements.
-- The elements parameter is a two-dimensional array. 
-- Every element of the array is either a Topology element represented by
-- (id, type) or a TopoGeometry element represented by (id, layer).
-- The actual semantic depends on the TopoGeometry layer, either at
-- level 0 (elements are topological primitives) or higer (elements
-- are TopoGeoms from child layer).
-- 
-- Return a topology.TopoGeometry object.
--
CREATEFUNCTION topology.CreateTopoGeom(varchar, integer, integer, topology.TopoElementArray)
	RETURNS topology.TopoGeometry
AS '
DECLARE
	toponame alias for $1;
	tg_type alias for $2; -- 1:[multi]point
	                      -- 2:[multi]line
	                      -- 3:[multi]poly
	                      -- 4:collection
	layer_id alias for $3;
	tg_objs alias for $4;
	i integer;
	dims varchar;
	outerdims varchar;
	innerdims varchar;
	obj_type integer;
	obj_id integer;
	ret topology.TopoGeometry;
	rec RECORD;
	layertype integer;
	layerlevel integer;
	layerchild integer;
BEGIN

	IF tg_type < 1 OR tg_type > 4 THEN
		RAISE EXCEPTION ''Invalid TopoGeometry type (must be in the range 1..4'';
	END IF;

	-- Get topology id into return TopoGeometry
	SELECT id FROM topology.topology into ret.topology_id
		WHERE name = toponame;

	--
	-- Get layer info
	--
	layertype := NULL;
	FOR rec IN EXECUTE ''SELECT * FROM topology.layer''
		|| '' WHERE topology_id = '' || ret.topology_id
		|| '' AND layer_id = '' || layer_id
	LOOP
		layertype = rec.feature_type;
		layerlevel = rec.level;
		layerchild = rec.child_id;
	END LOOP;

	-- Check for existence of given layer id
	IF layertype IS NULL THEN
		RAISE EXCEPTION ''No layer with id % is registered with topology %'', layer_id, toponame;
	END IF;

	-- Verify compatibility between layer geometry type and
	-- TopoGeom requested geometry type
	IF layertype != 4 and layertype != tg_type THEN
		RAISE EXCEPTION ''A Layer of type % cannot contain a TopoGeometry of type %'', layertype, tg_type;
	END IF;

	-- Set layer id and type in return object
	ret.layer_id = layer_id;
	ret.type = tg_type;

	--
	-- Get new TopoGeo id from sequence
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		toponame || ''.topogeo_s_'' || layer_id || '''''')''
	LOOP
		ret.id = rec.nextval;
	END LOOP;

	-- Loop over outer dimension
	i = array_lower(tg_objs, 1);
	LOOP
		obj_id = tg_objs[i][1];
		obj_type = tg_objs[i][2];
		IF layerlevel = 0 THEN -- array specifies lower-level objects
			IF tg_type != 4 and tg_type != obj_type THEN
				RAISE EXCEPTION ''A TopoGeometry of type % cannot contain topology elements of type %'', tg_type, obj_type;
			END IF;
		ELSE -- array specifies lower-level topogeometries
			IF obj_type != layerchild THEN
				RAISE EXCEPTION ''TopoGeom element layer do not match TopoGeom child layer'';
			END IF;
			-- TODO: verify that the referred TopoGeometry really
			-- exists in the relation table ?
		END IF;

		--RAISE NOTICE ''obj:% type:% id:%'', i, obj_type, obj_id;

		--
		-- Insert record into the Relation table
		--
		EXECUTE ''INSERT INTO ''||quote_ident(toponame)
			|| ''.relation(topogeo_id, layer_id, ''
			|| ''element_id,element_type) ''
			|| '' VALUES (''||ret.id
			||'',''||ret.layer_id
			|| '','' || obj_id || '','' || obj_type || '');'';

		i = i+1;
		IF i > array_upper(tg_objs, 1) THEN
			EXIT;
		END IF;
	END LOOP;

	RETURN ret;

END
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
--} CreateTopoGeom(toponame,topogeom_type, TopoObject[])

--{
-- GetTopologyName(topology_id)
--
CREATEFUNCTION topology.GetTopologyName(integer)
	RETURNS varchar
AS
'
DECLARE
	topoid alias for $1;
	ret varchar;
BEGIN
        SELECT name FROM topology.topology into ret
                WHERE id = topoid;
	RETURN ret;
END
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
--} GetTopologyName(topoid)

--{
-- GetTopologyId(toponame)
--
CREATEFUNCTION topology.GetTopologyId(varchar)
	RETURNS integer
AS
'
DECLARE
	toponame alias for $1;
	ret integer;
BEGIN
        SELECT id FROM topology.topology into ret
                WHERE name = toponame;
	RETURN ret;
END
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
--} GetTopologyId(toponame)
	


--{
-- GetTopoGeomElementArray(toponame, layer_id, topogeom_id)
-- GetTopoGeomElementArray(TopoGeometry)
--
-- Returns a set of element_id,element_type
--
CREATEFUNCTION topology.GetTopoGeomElementArray(varchar, integer, integer)
	RETURNS topology.TopoElementArray
AS
'
DECLARE
	toponame alias for $1;
	layerid alias for $2;
	tgid alias for $3;
	rec RECORD;
	tg_objs varchar := ''{'';
	i integer;
	query text;
BEGIN

	query = ''SELECT * FROM topology.GetTopoGeomElements(''
		|| quote_literal(toponame) || '',''
		|| quote_literal(layerid) || '',''
		|| quote_literal(tgid)
		|| '') as obj ORDER BY obj'';

	RAISE NOTICE ''Query: %'', query;

	i = 1;
	FOR rec IN EXECUTE query
	LOOP
		IF i > 1 THEN
			tg_objs = tg_objs || '','';
		END IF;
		tg_objs = tg_objs || ''{''
			|| rec.obj[1] || '','' || rec.obj[2]
			|| ''}'';
		i = i+1;
	END LOOP;

	tg_objs = tg_objs || ''}'';

	RETURN tg_objs;
END;
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;

CREATEFUNCTION topology.GetTopoGeomElementArray(topology.TopoGeometry)
	RETURNS topology.TopoElementArray
AS
'
DECLARE
	tg alias for $1;
	toponame varchar;
	ret topology.TopoElementArray;
BEGIN
	toponame = topology.GetTopologyName(tg.topology_id);
	ret = topology.GetTopoGeomElementArray(toponame,tg.layer_id,tg.id);
	RETURN ret;
END;
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;

--} GetTopoGeomElementArray()

--{
-- GetTopoGeomElements(toponame, layer_id, topogeom_id)
-- GetTopoGeomElements(TopoGeometry)
--
-- Returns a set of element_id,element_type
--
CREATEFUNCTION topology.GetTopoGeomElements(varchar, integer, integer)
	RETURNS SETOF topology.TopoElement
AS
'
DECLARE
	toponame alias for $1;
	layerid alias for $2;
	tgid alias for $3;
	ret topology.TopoElement;
	rec RECORD;
	rec2 RECORD;
	query text;
	query2 text;
	lyr RECORD;
	ok bool;
BEGIN

	-- Get layer info
	ok = false;
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| '' topology.layer ''
		|| '' WHERE layer_id = '' || layerid
	LOOP
		lyr = rec;
		ok = true;
	END LOOP;

	IF NOT ok THEN
		RAISE EXCEPTION ''Layer % does not exist'', layerid;
	END IF;


	query = ''SELECT abs(element_id) as element_id, element_type FROM ''
		|| quote_ident(toponame) || ''.relation WHERE ''
		|| '' layer_id = '' || layerid
		|| '' AND topogeo_id = '' || quote_literal(tgid)
		|| '' ORDER BY element_type, element_id'';

	--RAISE NOTICE ''Query: %'', query;

	FOR rec IN EXECUTE query
	LOOP
		IF lyr.level > 0 THEN
			query2 = ''SELECT * from topology.GetTopoGeomElements(''
				|| quote_literal(toponame) || '',''
				|| rec.element_type
				|| '',''
				|| rec.element_id
				|| '') as ret;'';
			--RAISE NOTICE ''Query2: %'', query2;
			FOR rec2 IN EXECUTE query2
			LOOP
				RETURN NEXT rec2.ret;
			END LOOP;
		ELSE
			ret = ''{'' || rec.element_id || '','' || rec.element_type || ''}'';
			RETURN NEXT ret;
		END IF;
	
	END LOOP;

	RETURN;
END;
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;

CREATEFUNCTION topology.GetTopoGeomElements(topology.TopoGeometry)
	RETURNS SETOF topology.TopoElement
AS
'
DECLARE
	tg alias for $1;
	toponame varchar;
	rec RECORD;
BEGIN
	toponame = topology.GetTopologyName(tg.topology_id);
	FOR rec IN SELECT * FROM topology.GetTopoGeomElements(toponame,
		tg.layer_id,tg.id) as ret
	LOOP
		RETURN NEXT rec.ret;
	END LOOP;
	RETURN;
END;
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;

--} GetTopoGeomElements()

--{
-- Geometry(TopoGeometry)
--
-- Construct a Geometry from a TopoGeometry.
-- 
--
CREATE OR REPLACE FUNCTION topology.Geometry(topology.TopoGeometry)
	RETURNS Geometry
AS '
DECLARE
	topogeom alias for $1;
	toponame varchar;
	geom geometry;
	rec RECORD;
	plyr RECORD;
	clyr RECORD;
	query text;
	ok BOOL;
BEGIN
        -- Get topology name
        SELECT name FROM topology.topology into toponame
                WHERE id = topogeom.topology_id;

	-- Get layer info
	ok = false;
	FOR rec IN EXECUTE ''SELECT * FROM topology.layer ''
		|| '' WHERE topology_id = '' || topogeom.topology_id
		|| '' AND layer_id = '' || topogeom.layer_id
	LOOP
		ok = true;
		plyr = rec;
	END LOOP;

	IF NOT ok THEN
		RAISE EXCEPTION ''Could not find TopoGeometry layer % in topology %'', topogeom.layer_id, topogeom.topology_id;
	END IF;

	--
	-- If this feature layer is on any level > 0 we will
	-- compute the topological union of all child features
	-- in fact recursing.
	--
	IF plyr.level > 0 THEN
		-- Get child layer info
		FOR rec IN EXECUTE ''SELECT * FROM topology.layer''
			|| '' WHERE layer_id = '' || plyr.child_id
		LOOP
			clyr = rec;
		END LOOP;

		query = ''SELECT geomunion(topology.Geometry(''
			|| quote_ident(clyr.feature_column)
			|| '')) as geom FROM ''
			|| quote_ident(clyr.schema_name) || ''.''
			|| quote_ident(clyr.table_name)
			|| '', '' || quote_ident(toponame) || ''.relation pr''
			|| '' WHERE ''
			|| '' pr.topogeo_id = '' || topogeom.id
			|| '' AND ''
			|| '' pr.layer_id = '' || topogeom.layer_id
			|| '' AND ''
			|| '' id(''||quote_ident(clyr.feature_column)
			|| '') = pr.element_id ''
			|| '' AND ''
			|| ''layer_id(''||quote_ident(clyr.feature_column)
			|| '') = pr.element_type '';
		--RAISE NOTICE ''%'', query;
		FOR rec IN EXECUTE query
		LOOP
			RETURN rec.geom;
		END LOOP;
			
	END IF;
	

	IF topogeom.type = 3 THEN -- [multi]polygon
		FOR rec IN EXECUTE ''SELECT geomunion(''
			|| ''topology.ST_GetFaceGeometry(''
			|| quote_literal(toponame) || '',''
			|| ''element_id)) as g FROM '' 
			|| quote_ident(toponame)
			|| ''.relation WHERE topogeo_id = ''
			|| topogeom.id || '' AND layer_id = ''
			|| topogeom.layer_id || '' AND element_type = 3 ''
		LOOP
			geom := rec.g;
		END LOOP;

	ELSIF topogeom.type = 2 THEN -- [multi]line
		FOR rec IN EXECUTE ''SELECT linemerge(collect(e.geom)) as g FROM ''
			|| quote_ident(toponame) || ''.edge e, ''
			|| quote_ident(toponame) || ''.relation r ''
			|| '' WHERE r.topogeo_id = '' || topogeom.id
			|| '' AND r.layer_id = '' || topogeom.layer_id
			|| '' AND r.element_type = 2 ''
			|| '' AND abs(r.element_id) = e.edge_id''
		LOOP
			geom := rec.g;
		END LOOP;
	
	ELSIF topogeom.type = 1 THEN -- [multi]point
		FOR rec IN EXECUTE ''SELECT geomunion(n.geom) as g FROM ''
			|| quote_ident(toponame) || ''.node n, ''
			|| quote_ident(toponame) || ''.relation r ''
			|| '' WHERE r.topogeo_id = '' || topogeom.id
			|| '' AND r.layer_id = '' || topogeom.layer_id
			|| '' AND r.element_type = 1 ''
			|| '' AND r.element_id = n.node_id''
		LOOP
			geom := rec.g;
		END LOOP;

	ELSE
		RAISE NOTICE ''Geometry from TopoGeometry does not support TopoGeometries of type % so far'', topogeom.type;
		geom := ''GEOMETRYCOLLECTION EMPTY'';
	END IF;

	RETURN geom;
END
'
LANGUAGE 'plpgsql' VOLATILE STRICT;
--} Geometry(TopoGeometry)

--{
--  ValidateTopology(toponame)
--
--  Return a Set of ValidateTopology_ReturnType containing
--  informations on all topology inconsistencies
--
CREATEFUNCTION topology.ValidateTopology(varchar)
	RETURNS setof topology.ValidateTopology_ReturnType
AS
'
DECLARE
	toponame alias for $1;
	retrec topology.ValidateTopology_ReturnType;
	rec RECORD;
	i integer;
BEGIN

	-- Check for coincident nodes
	FOR rec IN EXECUTE ''SELECT a.node_id as id1, b.node_id as id2 FROM ''
		|| quote_ident(toponame) || ''.node a, ''
		|| quote_ident(toponame) || ''.node b ''
		|| ''WHERE a.node_id < b.node_id AND a.geom && b.geom''
	LOOP
		retrec.error = ''coincident nodes'';
		retrec.id1 = rec.id1;
		retrec.id2 = rec.id2;
		RETURN NEXT retrec;
	END LOOP;

	-- Check for edge crossed nodes
	FOR rec IN EXECUTE ''SELECT n.node_id as id1, e.edge_id as id2 FROM ''
		|| quote_ident(toponame) || ''.node n, ''
		|| quote_ident(toponame) || ''.edge e ''
		|| ''WHERE e.start_node != n.node_id ''
		|| ''AND e.end_node != n.node_id ''
		|| ''AND n.geom && e.geom ''
		|| ''AND intersects(n.geom, e.geom)''
	LOOP
		retrec.error = ''edge crosses node'';
		retrec.id1 = rec.id1;
		retrec.id2 = rec.id2;
		RETURN NEXT retrec;
	END LOOP;

	-- Check for non-simple edges
	FOR rec IN EXECUTE ''SELECT e.edge_id as id1 FROM ''
		|| quote_ident(toponame) || ''.edge e ''
		|| ''WHERE not issimple(e.geom)''
	LOOP
		retrec.error = ''edge not simple'';
		retrec.id1 = rec.id1;
		retrec.id2 = NULL;
		RETURN NEXT retrec;
	END LOOP;

	-- Check for edge crossing
	FOR rec IN EXECUTE ''SELECT e1.edge_id as id1, e2.edge_id as id2 FROM ''
		|| quote_ident(toponame) || ''.edge e1, ''
		|| quote_ident(toponame) || ''.edge e2 ''
		|| ''WHERE e1.edge_id < e2.edge_id ''
		|| ''AND e1.geom && e2.geom ''
		|| ''AND crosses(e1.geom, e2.geom)''
	LOOP
		retrec.error = ''edge crosses edge'';
		retrec.id1 = rec.id1;
		retrec.id2 = rec.id2;
		RETURN NEXT retrec;
	END LOOP;

	-- Check for edge start_node geometry mis-match
	FOR rec IN EXECUTE ''SELECT e.edge_id as id1, n.node_id as id2 FROM ''
		|| quote_ident(toponame) || ''.edge e, ''
		|| quote_ident(toponame) || ''.node n ''
		|| ''WHERE e.start_node = n.node_id ''
		|| ''AND NOT Equals(StartPoint(e.geom), n.geom)''
	LOOP
		retrec.error = ''edge start node geometry mis-match'';
		retrec.id1 = rec.id1;
		retrec.id2 = rec.id2;
		RETURN NEXT retrec;
	END LOOP;

	-- Check for edge end_node geometry mis-match
	FOR rec IN EXECUTE ''SELECT e.edge_id as id1, n.node_id as id2 FROM ''
		|| quote_ident(toponame) || ''.edge e, ''
		|| quote_ident(toponame) || ''.node n ''
		|| ''WHERE e.end_node = n.node_id ''
		|| ''AND NOT Equals(EndPoint(e.geom), n.geom)''
	LOOP
		retrec.error = ''edge end node geometry mis-match'';
		retrec.id1 = rec.id1;
		retrec.id2 = rec.id2;
		RETURN NEXT retrec;
	END LOOP;

	-- Check for faces w/out edges
	FOR rec IN EXECUTE ''SELECT face_id as id1 FROM ''
		|| quote_ident(toponame) || ''.face ''
		|| ''EXCEPT ( SELECT left_face FROM ''
		|| quote_ident(toponame) || ''.edge ''
		|| '' UNION SELECT right_face FROM ''
		|| quote_ident(toponame) || ''.edge ''
		|| '')''
	LOOP
		retrec.error = ''face without edges'';
		retrec.id1 = rec.id1;
		retrec.id2 = NULL;
		RETURN NEXT retrec;
	END LOOP;

	-- Check for overlapping faces 
	-- TODO: this check requires some thinking, do we really
	--       have to construct face geometry to detect overlap with
	--       another face ??
	FOR rec IN EXECUTE ''SELECT f1.face_id as id1, f2.face_id as id2 FROM ''
		|| quote_ident(toponame) || ''.face f1, ''
		|| quote_ident(toponame) || ''.face f2 ''
		|| ''WHERE f1.face_id > 0 AND f1.face_id < f2.face_id AND ''
		|| '' Overlaps(topology.ST_GetFaceGeometry(''
		|| quote_literal(toponame) || '', f1.face_id), ''
		|| '' topology.ST_GetFaceGeometry(''
		|| quote_literal(toponame) || '', f2.face_id))''
	LOOP
		retrec.error = ''face overlaps face'';
		retrec.id1 = rec.id1;
		retrec.id2 = rec.id2;
		RETURN NEXT retrec;
	END LOOP;

	-- Check for face within face
	-- TODO: this check requires some thinking, do we really
	--       have to construct face geometry to detect within condition
	--       another face ??
	FOR rec IN EXECUTE ''SELECT f1.face_id as id1, f2.face_id as id2 FROM ''
		|| quote_ident(toponame) || ''.face f1, ''
		|| quote_ident(toponame) || ''.face f2 ''
		|| ''WHERE f1.face_id != 0 AND f2.face_id != 0 ''
		|| ''AND f1.face_id != f2.face_id ''
		|| ''AND Within(topology.ST_GetFaceGeometry(''
		|| quote_literal(toponame) || '', f1.face_id), ''
		|| '' topology.ST_GetFaceGeometry(''
		|| quote_literal(toponame) || '', f2.face_id))''
	LOOP
		retrec.error = ''face within face'';
		retrec.id1 = rec.id1;
		retrec.id2 = rec.id2;
		RETURN NEXT retrec;
	END LOOP;

#if 0
	-- Check SRID consistency
	FOR rec in EXECUTE
		''SELECT count(*) FROM ( getSRID(geom) FROM ''
		|| quote_ident(toponame) || ''.edge ''
		|| '' UNION ''
		''SELECT getSRID(geom) FROM ''
		|| quote_ident(toponame) || ''.node )''
	LOOP
		IF rec.count > 1 THEN
			retrec.error = ''mixed SRIDs'';
			retrec.id1 = NULL;
			retrec.id2 = NULL;
			RETURN NEXT retrec;
		END IF;
	END LOOP;
#endif

	RETURN;
END
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
-- } ValidateTopology(toponame)

--{
--  TopoGeo_AddPoint(toponame, pointgeom, layer_id, topogeom_id)
--
--  Add a Point (node) into a topology 
--
CREATEFUNCTION topology.TopoGeo_AddPoint(varchar, geometry, integer, integer)
	RETURNS int AS
'
DECLARE
	atopology alias for $1;
	apoint alias for $2;
	ret int;
BEGIN

	-- Add nodes (contained in NO face)
	SELECT topology.ST_AddIsoNode(atopology, NULL, apoint) INTO ret;

	RAISE NOTICE ''TopoGeo_AddPoint: node: %'', ret;

	RETURN ret;

END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} TopoGeo_AddPoint

--{
--  TopoGeo_addLinestring
--
--  Add a Point (node) into a topology 
--
CREATEFUNCTION topology.TopoGeo_addLinestring(varchar, geometry)
	RETURNS int AS
'
DECLARE
	atopology alias for $1;
	aline alias for $2;
	rec RECORD;
	query text;
	firstpoint geometry;
	lastpoint geometry;
	firstnode int;
	lastnode int;
	edgeid int;
BEGIN
	firstpoint = StartPoint(aline);
	lastpoint = EndPoint(aline);

	-- Add first and last point nodes (contained in NO face)
	SELECT topology.ST_AddIsoNode(atopology, NULL, firstpoint) INTO firstnode;
	SELECT topology.ST_AddIsoNode(atopology, NULL, lastpoint) INTO lastnode;

	RAISE NOTICE ''First node: %, Last node: %'', firstnode, lastnode;

	-- Add edge 
	SELECT topology.ST_AddIsoEdge(atopology, firstnode, lastnode, aline)
		INTO edgeid;

	RAISE NOTICE ''Edge: %'', edgeid;

	RETURN edgeid;
END
'
LANGUAGE 'plpgsql';
--} TopoGeo_addLinestring

--{
--  TopoGeo_AddPolygon
--
--  Add a Polygon into a topology 
--
CREATEFUNCTION topology.TopoGeo_AddPolygon(varchar, geometry)
	RETURNS int AS
'
DECLARE
	rec RECORD;
	query text;
BEGIN

	RAISE EXCEPTION ''TopoGeo_AddPolygon not implemented yet'';
END
'
LANGUAGE 'plpgsql';
--} TopoGeo_AddPolygon

--{
--  CreateTopology(name, SRID, precision)
--
-- Create a topology schema, add a topology info record
-- in the topology.topology relation, return it's numeric
-- id. 
--
CREATEFUNCTION topology.CreateTopology(varchar, integer, float8)
RETURNS integer
AS '
DECLARE
	atopology alias for $1;
	srid alias for $2;
	precision alias for $3;
	rec RECORD;
	topology_id integer;
BEGIN

--	FOR rec IN SELECT * FROM pg_namespace WHERE text(nspname) = atopology
--	LOOP
--		RAISE EXCEPTION ''SQL/MM Spatial exception - schema already exists'';
--	END LOOP;

	------ Fetch next id for the new topology
	FOR rec IN SELECT nextval(''topology.topology_id_seq'')
	LOOP
		topology_id = rec.nextval;
	END LOOP;


	EXECUTE ''
CREATE SCHEMA '' || quote_ident(atopology) || '';
	'';

	-------------{ face CREATION
	EXECUTE 
	''CREATE TABLE '' || quote_ident(atopology) || ''.face (''
	|| ''face_id SERIAL,''
	|| ''mbr BOX2D,''
	|| '' CONSTRAINT face_primary_key PRIMARY KEY(face_id)''
	|| '');'';
	-------------} END OF face CREATION


	--------------{ node CREATION

	EXECUTE 
	''CREATE TABLE '' || quote_ident(atopology) || ''.node (''
	|| ''node_id SERIAL,''
	--|| ''geom GEOMETRY,''
	|| ''containing_face INTEGER,''

	|| ''CONSTRAINT node_primary_key PRIMARY KEY(node_id),''

	--|| ''CONSTRAINT node_geometry_type CHECK ''
	--|| ''( GeometryType(geom) = ''''POINT'''' ),''

	|| ''CONSTRAINT face_exists FOREIGN KEY(containing_face) ''
	|| ''REFERENCES '' || quote_ident(atopology) || ''.face(face_id)''

	|| '');'';

	-- Add geometry column to the node table 
	EXECUTE
	''SELECT AddGeometryColumn(''||quote_literal(atopology)
	||'',''''node'''',''''geom'''',''||quote_literal(srid)
	||'',''''POINT'''',''''2'''')'';

	--------------} END OF node CREATION

	--------------{ edge CREATION

	-- edge_data table
	EXECUTE 
	''CREATE TABLE '' || quote_ident(atopology) || ''.edge_data (''
	|| ''edge_id SERIAL NOT NULL PRIMARY KEY,''
	|| ''start_node INTEGER NOT NULL,''
	|| ''end_node INTEGER NOT NULL,''
	|| ''next_left_edge INTEGER NOT NULL,''
	|| ''abs_next_left_edge INTEGER NOT NULL,''
	|| ''next_right_edge INTEGER NOT NULL,''
	|| ''abs_next_right_edge INTEGER NOT NULL,''
	|| ''left_face INTEGER NOT NULL,''
	|| ''right_face INTEGER NOT NULL,''
	--|| ''geom GEOMETRY NOT NULL,''

	--|| ''CONSTRAINT edge_geometry_type CHECK ''
	--|| ''( GeometryType(geom) = ''''LINESTRING'''' ),''

	|| ''CONSTRAINT start_node_exists FOREIGN KEY(start_node)''
	|| '' REFERENCES '' || quote_ident(atopology) || ''.node(node_id),''

	|| ''CONSTRAINT end_node_exists FOREIGN KEY(end_node) ''
	|| '' REFERENCES '' || quote_ident(atopology) || ''.node(node_id),''

	|| ''CONSTRAINT left_face_exists FOREIGN KEY(left_face) ''
	|| ''REFERENCES '' || quote_ident(atopology) || ''.face(face_id),''

	|| ''CONSTRAINT right_face_exists FOREIGN KEY(right_face) ''
	|| ''REFERENCES '' || quote_ident(atopology) || ''.face(face_id),''

	|| ''CONSTRAINT next_left_edge_exists FOREIGN KEY(abs_next_left_edge)''
	|| '' REFERENCES '' || quote_ident(atopology)
	|| ''.edge_data(edge_id)''
	|| '' DEFERRABLE INITIALLY DEFERRED,''

	|| ''CONSTRAINT next_right_edge_exists ''
	|| ''FOREIGN KEY(abs_next_right_edge)''
	|| '' REFERENCES '' || quote_ident(atopology)
	|| ''.edge_data(edge_id) ''
	|| '' DEFERRABLE INITIALLY DEFERRED''
	|| '');'';

	-- Add geometry column to the edge_data table 
	EXECUTE
	''SELECT AddGeometryColumn(''||quote_literal(atopology)
	||'',''''edge_data'''',''''geom'''',''||quote_literal(srid)
	||'',''''LINESTRING'''',''''2'''')'';


	-- edge standard view (select rule)
	EXECUTE ''CREATE VIEW '' || quote_ident(atopology)
		|| ''.edge AS SELECT ''
		|| '' edge_id,start_node, end_node, next_left_edge, ''
		|| '' next_right_edge, ''
		|| '' left_face, right_face, geom FROM ''
		|| quote_ident(atopology) || ''.edge_data'';

	-- edge standard view (insert rule)
	EXECUTE ''CREATE RULE edge_insert_rule AS ON INSERT ''
        	|| ''TO '' || quote_ident(atopology)
		|| ''.edge DO INSTEAD ''
                || '' INSERT into '' || quote_ident(atopology)
		|| ''.edge_data ''
                || '' VALUES (NEW.edge_id, NEW.start_node, NEW.end_node, ''
		|| '' NEW.next_left_edge, abs(NEW.next_left_edge), ''
		|| '' NEW.next_right_edge, abs(NEW.next_right_edge), ''
		|| '' NEW.left_face, NEW.right_face, NEW.geom);'';

	--------------} END OF edge CREATION

	--------------{ layer sequence 
	EXECUTE ''CREATE SEQUENCE ''
		|| quote_ident(atopology) || ''.layer_id_seq;'';
	--------------} layer sequence

	--------------{ relation CREATION
	--
	EXECUTE 
	''CREATE TABLE '' || quote_ident(atopology) || ''.relation (''
	|| '' topogeo_id integer NOT NULL, ''
	|| '' layer_id integer NOT NULL, '' 
	|| '' element_id integer NOT NULL, ''
	|| '' element_type integer NOT NULL, ''
	|| '' UNIQUE(layer_id,topogeo_id,element_id,element_type));'';

	EXECUTE 
	''CREATE TRIGGER relation_integrity_checks ''
	||''BEFORE UPDATE OR INSERT ON ''
	|| quote_ident(atopology) || ''.relation FOR EACH ROW ''
	|| '' EXECUTE PROCEDURE topology.RelationTrigger(''
	||topology_id||'',''||quote_literal(atopology)||'')'';
	--------------} END OF relation CREATION

	
	------- Default (world) face
	EXECUTE ''INSERT INTO '' || quote_ident(atopology) || ''.face(face_id) VALUES(0);'';

	------- GiST index on node
	EXECUTE ''CREATE INDEX node_gist ON ''
		|| quote_ident(atopology)
		|| ''.node using gist (geom gist_geometry_ops);'';

	------- GiST index on edge
	EXECUTE ''CREATE INDEX edge_gist ON ''
		|| quote_ident(atopology)
		|| ''.edge_data using gist (geom gist_geometry_ops);'';

	------- Add record to the "topology" metadata table
	EXECUTE ''INSERT INTO topology.topology (id, name, srid, precision) ''
		|| '' VALUES ('' || quote_literal(topology_id) || '',''
		|| quote_literal(atopology) || '',''
		|| quote_literal(srid) || '','' || quote_literal(precision)
		|| '')'';

	RETURN topology_id;
END
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;

-- wrappers for unspecified srid or precision

CREATEFUNCTION topology.CreateTopology(varchar, integer)
RETURNS integer AS
' SELECT topology.CreateTopology($1, $2, -1); '
LANGUAGE 'SQL' _VOLATILE_STRICT;

CREATEFUNCTION topology.CreateTopology(varchar)
RETURNS integer AS
' SELECT topology.CreateTopology($1, -1, -1); '
LANGUAGE 'SQL' _VOLATILE_STRICT;

--} CreateTopology

--{
--  DropTopology(name)
--
-- Drops a topology schema getting rid of every dependent object.
--
CREATEFUNCTION topology.DropTopology(varchar)
RETURNS text
AS '
DECLARE
	atopology alias for $1;
	topoid integer;
	rec RECORD;
BEGIN

	-- Get topology id
        SELECT id FROM topology.topology into topoid
                WHERE name = atopology;


	IF topoid IS NOT NULL THEN

		RAISE NOTICE ''Dropping all layers from topology % (%)'',
			atopology, topoid;

		-- Drop all layers in the topology
		FOR rec IN EXECUTE ''SELECT * FROM topology.layer WHERE ''
			|| '' topology_id = '' || topoid
		LOOP

			EXECUTE ''SELECT topology.DropTopoGeometryColumn(''
				|| quote_literal(rec.schema_name)
				|| '',''
				|| quote_literal(rec.table_name)
				|| '',''
				|| quote_literal(rec.feature_column)
				|| '')'';
		END LOOP;

		-- Delete record from topology.topology
		EXECUTE ''DELETE FROM topology.topology WHERE id = ''
			|| topoid;

	END IF;


	-- Drop the schema (if it exists)
	FOR rec IN SELECT * FROM pg_namespace WHERE text(nspname) = atopology
	LOOP
		EXECUTE ''DROP SCHEMA ''||quote_ident(atopology)||'' CASCADE'';
	END LOOP;


	RETURN ''Topology '' || quote_literal(atopology) || '' dropped'';
END
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
--} DropTopology

--={ ----------------------------------------------------------------
--  POSTGIS-SPECIFIC topology predicates
--
--  This part contains function NOT in the SQL/MM specification
--
---------------------------------------------------------------------

--{
-- Intersects(TopoGeometry, TopoGeometry)
--
CREATEFUNCTION topology.intersects(topology.TopoGeometry, topology.TopoGeometry)
	RETURNS bool
AS
'
DECLARE
	tg1 alias for $1;
	tg2 alias for $2;
	tgbuf topology.TopoGeometry;
	rec RECORD;
	toponame varchar;
	query text;
BEGIN
	IF tg1.topology_id != tg2.topology_id THEN
		RAISE EXCEPTION ''Cannot compute intersection between TopoGeometries from different topologies'';
	END IF;

	-- Order TopoGeometries so that tg1 has less-or-same
	-- dimensionality of tg1 (point,line,polygon,collection)
	IF tg1.type > tg2.type THEN
		tgbuf := tg2;
		tg2 := tg1;
		tg1 := tgbuf;
	END IF;

	--RAISE NOTICE ''tg1.id:% tg2.id:%'', tg1.id, tg2.id;
	-- Geometry collection are not currently supported
	IF tg2.type = 4 THEN
		RAISE EXCEPTION ''GeometryCollection are not supported by intersects()'';
	END IF;

        -- Get topology name
        SELECT name FROM topology.topology into toponame
                WHERE id = tg1.topology_id;

	-- Hierarchical TopoGeometries are not currently supported
	query = ''SELECT level FROM topology.layer''
		|| '' WHERE ''
		|| '' topology_id = '' || tg1.topology_id
		|| '' AND ''
		|| ''( layer_id = '' || tg1.layer_id
		|| '' OR layer_id = '' || tg2.layer_id
		|| '' ) ''
		|| '' AND level > 0 '';

	--RAISE NOTICE ''%'', query;

	FOR rec IN EXECUTE query
	LOOP
		RAISE EXCEPTION ''Hierarchical TopoGeometries are not currently supported by intersects()'';
	END LOOP;

	IF tg1.type = 1 THEN -- [multi]point


		IF tg2.type = 1 THEN -- point/point
	---------------------------------------------------------
	-- 
	--  Two [multi]point features intersect if they share
	--  any Node 
	--
	--
	--
			query =
				''SELECT a.topogeo_id FROM ''
				|| quote_ident(toponame) ||
				''.relation a, ''
				|| quote_ident(toponame) ||
				''.relation b ''
				|| ''WHERE a.layer_id = '' || tg1.layer_id
				|| '' AND b.layer_id = '' || tg2.layer_id
				|| '' AND a.topogeo_id = '' || tg1.id
				|| '' AND b.topogeo_id = '' || tg2.id
				|| '' AND a.element_id = b.element_id ''
				|| '' LIMIT 1'';
			--RAISE NOTICE ''%'', query;
			FOR rec IN EXECUTE query
			LOOP
				RETURN TRUE; -- they share an element
			END LOOP;
			RETURN FALSE; -- no elements shared
	--
	---------------------------------------------------------
			

		ELSIF tg2.type = 2 THEN -- point/line
	---------------------------------------------------------
	-- 
	--  A [multi]point intersects a [multi]line if they share
	--  any Node. 
	--
	--
	--
			query =
				''SELECT a.topogeo_id FROM ''
				|| quote_ident(toponame) ||
				''.relation a, ''
				|| quote_ident(toponame) ||
				''.relation b, ''
				|| quote_ident(toponame) ||
				''.edge_data e ''
				|| ''WHERE a.layer_id = '' || tg1.layer_id
				|| '' AND b.layer_id = '' || tg2.layer_id
				|| '' AND a.topogeo_id = '' || tg1.id
				|| '' AND b.topogeo_id = '' || tg2.id
				|| '' AND abs(b.element_id) = e.edge_id ''
				|| '' AND ( ''
					|| '' e.start_node = a.element_id ''
					|| '' OR ''
					|| '' e.end_node = a.element_id ''
				|| '' )''
				|| '' LIMIT 1'';
			--RAISE NOTICE ''%'', query;
			FOR rec IN EXECUTE query
			LOOP
				RETURN TRUE; -- they share an element
			END LOOP;
			RETURN FALSE; -- no elements shared
	--
	---------------------------------------------------------

		ELSIF tg2.type = 3 THEN -- point/polygon
	---------------------------------------------------------
	-- 
	--  A [multi]point intersects a [multi]polygon if any
	--  Node of the point is contained in any face of the
	--  polygon OR ( is end_node or start_node of any edge
	--  of any polygon face ).
	--
	--  We assume the Node-in-Face check is faster becasue
	--  there will be less Faces then Edges in any polygon.
	--
	--
	--
	--
			-- Check if any node is contained in a face
			query =
				''SELECT n.node_id as id FROM ''
				|| quote_ident(toponame) ||
				''.relation r1, ''
				|| quote_ident(toponame) ||
				''.relation r2, ''
				|| quote_ident(toponame) ||
				''.node n ''
				|| ''WHERE r1.layer_id = '' || tg1.layer_id
				|| '' AND r2.layer_id = '' || tg2.layer_id
				|| '' AND r1.topogeo_id = '' || tg1.id
				|| '' AND r2.topogeo_id = '' || tg2.id
				|| '' AND n.node_id = r1.element_id ''
				|| '' AND r2.element_id = n.containing_face ''
				|| '' LIMIT 1'';
			--RAISE NOTICE ''%'', query;
			FOR rec IN EXECUTE query
			LOOP
				--RAISE NOTICE ''Node % in polygon face'', rec.id;
				RETURN TRUE; -- one (or more) nodes are
				             -- contained in a polygon face
			END LOOP;

			-- Check if any node is start or end of any polygon
			-- face edge
			query =
				''SELECT n.node_id as nid, e.edge_id as eid ''
				|| '' FROM ''
				|| quote_ident(toponame) ||
				''.relation r1, ''
				|| quote_ident(toponame) ||
				''.relation r2, ''
				|| quote_ident(toponame) ||
				''.edge_data e, ''
				|| quote_ident(toponame) ||
				''.node n ''
				|| ''WHERE r1.layer_id = '' || tg1.layer_id
				|| '' AND r2.layer_id = '' || tg2.layer_id
				|| '' AND r1.topogeo_id = '' || tg1.id
				|| '' AND r2.topogeo_id = '' || tg2.id
				|| '' AND n.node_id = r1.element_id ''
				|| '' AND ( ''
				|| '' e.left_face = r2.element_id ''
				|| '' OR ''
				|| '' e.right_face = r2.element_id ''
				|| '' ) ''
				|| '' AND ( ''
				|| '' e.start_node = r1.element_id ''
				|| '' OR ''
				|| '' e.end_node = r1.element_id ''
				|| '' ) ''
				|| '' LIMIT 1'';
			--RAISE NOTICE ''%'', query;
			FOR rec IN EXECUTE query
			LOOP
				--RAISE NOTICE ''Node % on edge % bound'', rec.nid, rec.eid;
				RETURN TRUE; -- one node is start or end
				             -- of a face edge
			END LOOP;

			RETURN FALSE; -- no intersection
	--
	---------------------------------------------------------

		ELSIF tg2.type = 4 THEN -- point/collection
			RAISE EXCEPTION ''Intersection point/collection not implemented yet'';

		ELSE
			RAISE EXCEPTION ''Invalid TopoGeometry type'', tg2.type;
		END IF;

	ELSIF tg1.type = 2 THEN -- [multi]line
		IF tg2.type = 2 THEN -- line/line
	---------------------------------------------------------
	-- 
	--  A [multi]line intersects a [multi]line if they share
	--  any Node. 
	--
	--
	--
			query =
				''SELECT e1.start_node FROM ''
				|| quote_ident(toponame) ||
				''.relation r1, ''
				|| quote_ident(toponame) ||
				''.relation r2, ''
				|| quote_ident(toponame) ||
				''.edge_data e1, ''
				|| quote_ident(toponame) ||
				''.edge_data e2 ''
				|| ''WHERE r1.layer_id = '' || tg1.layer_id
				|| '' AND r2.layer_id = '' || tg2.layer_id
				|| '' AND r1.topogeo_id = '' || tg1.id
				|| '' AND r2.topogeo_id = '' || tg2.id
				|| '' AND abs(r1.element_id) = e1.edge_id ''
				|| '' AND abs(r2.element_id) = e2.edge_id ''
				|| '' AND ( ''
					|| '' e1.start_node = e2.start_node ''
					|| '' OR ''
					|| '' e1.start_node = e2.end_node ''
					|| '' OR ''
					|| '' e1.end_node = e2.start_node ''
					|| '' OR ''
					|| '' e1.end_node = e2.end_node ''
				|| '' )''
				|| '' LIMIT 1'';
			--RAISE NOTICE ''%'', query;
			FOR rec IN EXECUTE query
			LOOP
				RETURN TRUE; -- they share an element
			END LOOP;
			RETURN FALSE; -- no elements shared
	--
	---------------------------------------------------------

		ELSIF tg2.type = 3 THEN -- line/polygon
	---------------------------------------------------------
	-- 
	-- A [multi]line intersects a [multi]polygon if they share
	-- any Node (touch-only case), or if any line edge has any
	-- polygon face on the left or right (full-containment case
	-- + edge crossing case).
	--
	--
			-- E1 are line edges, E2 are polygon edges
			-- R1 are line relations.
			-- R2 are polygon relations.
			-- R2.element_id are FACE ids
			query =
				''SELECT e1.edge_id''
				|| '' FROM ''
				|| quote_ident(toponame) ||
				''.relation r1, ''
				|| quote_ident(toponame) ||
				''.relation r2, ''
				|| quote_ident(toponame) ||
				''.edge_data e1, ''
				|| quote_ident(toponame) ||
				''.edge_data e2 ''
				|| ''WHERE r1.layer_id = '' || tg1.layer_id
				|| '' AND r2.layer_id = '' || tg2.layer_id
				|| '' AND r1.topogeo_id = '' || tg1.id
				|| '' AND r2.topogeo_id = '' || tg2.id

				-- E1 are line edges
				|| '' AND e1.edge_id = abs(r1.element_id) ''

				-- E2 are face edges
				|| '' AND ( e2.left_face = r2.element_id ''
				|| ''   OR e2.right_face = r2.element_id ) ''

				|| '' AND ( ''

				-- Check if E1 have left-or-right face 
				-- being part of R2.element_id
				|| '' e1.left_face = r2.element_id ''
				|| '' OR ''
				|| '' e1.right_face = r2.element_id ''

				-- Check if E1 share start-or-end node
				-- with any E2.
				|| '' OR ''
				|| '' e1.start_node = e2.start_node ''
				|| '' OR ''
				|| '' e1.start_node = e2.end_node ''
				|| '' OR ''
				|| '' e1.end_node = e2.start_node ''
				|| '' OR ''
				|| '' e1.end_node = e2.end_node ''

				|| '' ) ''

				|| '' LIMIT 1'';
			--RAISE NOTICE ''%'', query;
			FOR rec IN EXECUTE query
			LOOP
				RETURN TRUE; -- either common node
				             -- or edge-in-face
			END LOOP;

			RETURN FALSE; -- no intersection
	--
	---------------------------------------------------------

		ELSIF tg2.type = 4 THEN -- line/collection
			RAISE EXCEPTION ''Intersection line/collection not implemented yet'', tg1.type, tg2.type;

		ELSE
			RAISE EXCEPTION ''Invalid TopoGeometry type'', tg2.type;
		END IF;


	ELSIF tg1.type = 3 THEN -- [multi]polygon

		IF tg2.type = 3 THEN -- polygon/polygon
	---------------------------------------------------------
	-- 
	-- A [multi]polygon intersects a [multi]polygon if they share
	-- any Node (touch-only case), or if any face edge has any of the
	-- other polygon face on the left or right (full-containment case
	-- + edge crossing case).
	--
	--
			-- E1 are poly1 edges.
			-- E2 are poly2 edges
			-- R1 are poly1 relations.
			-- R2 are poly2 relations.
			-- R1.element_id are poly1 FACE ids
			-- R2.element_id are poly2 FACE ids
			query =
				''SELECT e1.edge_id''
				|| '' FROM ''
				|| quote_ident(toponame) ||
				''.relation r1, ''
				|| quote_ident(toponame) ||
				''.relation r2, ''
				|| quote_ident(toponame) ||
				''.edge_data e1, ''
				|| quote_ident(toponame) ||
				''.edge_data e2 ''
				|| ''WHERE r1.layer_id = '' || tg1.layer_id
				|| '' AND r2.layer_id = '' || tg2.layer_id
				|| '' AND r1.topogeo_id = '' || tg1.id
				|| '' AND r2.topogeo_id = '' || tg2.id

				-- E1 are poly1 edges
				|| '' AND ( e1.left_face = r1.element_id ''
				|| ''   OR e1.right_face = r1.element_id ) ''

				-- E2 are poly2 edges
				|| '' AND ( e2.left_face = r2.element_id ''
				|| ''   OR e2.right_face = r2.element_id ) ''

				|| '' AND ( ''

				-- Check if any edge from a polygon face
				-- has any of the other polygon face
				-- on the left or right 
				|| '' e1.left_face = r2.element_id ''
				|| '' OR ''
				|| '' e1.right_face = r2.element_id ''
				|| '' OR ''
				|| '' e2.left_face = r1.element_id ''
				|| '' OR ''
				|| '' e2.right_face = r1.element_id ''

				-- Check if E1 share start-or-end node
				-- with any E2.
				|| '' OR ''
				|| '' e1.start_node = e2.start_node ''
				|| '' OR ''
				|| '' e1.start_node = e2.end_node ''
				|| '' OR ''
				|| '' e1.end_node = e2.start_node ''
				|| '' OR ''
				|| '' e1.end_node = e2.end_node ''

				|| '' ) ''

				|| '' LIMIT 1'';
			--RAISE NOTICE ''%'', query;
			FOR rec IN EXECUTE query
			LOOP
				RETURN TRUE; -- either common node
				             -- or edge-in-face
			END LOOP;

			RETURN FALSE; -- no intersection
	--
	---------------------------------------------------------

		ELSIF tg2.type = 4 THEN -- polygon/collection
			RAISE EXCEPTION ''Intersection poly/collection not implemented yet'', tg1.type, tg2.type;

		ELSE
			RAISE EXCEPTION ''Invalid TopoGeometry type'', tg2.type;
		END IF;

	ELSIF tg1.type = 4 THEN -- collection
		IF tg2.type = 4 THEN -- collection/collection
			RAISE EXCEPTION ''Intersection collection/collection not implemented yet'', tg1.type, tg2.type;
		ELSE
			RAISE EXCEPTION ''Invalid TopoGeometry type'', tg2.type;
		END IF;

	ELSE
		RAISE EXCEPTION ''Invalid TopoGeometry type %'', tg1.type;
	END IF;
END
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
--} intersects(TopoGeometry, TopoGeometry)

--{
--  equals(TopoGeometry, TopoGeometry)
--
CREATEFUNCTION topology.equals(topology.TopoGeometry, topology.TopoGeometry)
	RETURNS bool
AS
'
DECLARE
	tg1 alias for $1;
	tg2 alias for $2;
	rec RECORD;
	toponame varchar;
	query text;
BEGIN

	IF tg1.topology_id != tg2.topology_id THEN
		RAISE EXCEPTION ''Cannot compare TopoGeometries from different topologies'';
	END IF;

	-- Not the same type, not equal
	IF tg1.type != tg2.type THEN
		RETURN FALSE;
	END IF;

	-- Geometry collection are not currently supported
	IF tg2.type = 4 THEN
		RAISE EXCEPTION ''GeometryCollection are not supported by equals()'';
	END IF;

        -- Get topology name
        SELECT name FROM topology.topology into toponame
                WHERE id = tg1.topology_id;

	-- Two geometries are equal if they are composed by 
	-- the same TopoElements
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| '' topology.GetTopoGeomElements(''
		|| quote_literal(toponame) || '', ''
		|| tg1.layer_id || '','' || tg1.id || '') ''
		|| '' EXCEPT SELECT * FROM ''
		|| '' topology.GetTopogeomElements(''
		|| quote_literal(toponame) || '', ''
		|| tg2.layer_id || '','' || tg2.id || '');''
	LOOP
		RETURN FALSE;
	END LOOP;

	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| '' topology.GetTopoGeomElements(''
		|| quote_literal(toponame) || '', ''
		|| tg2.layer_id || '','' || tg2.id || '')''
		|| '' EXCEPT SELECT * FROM ''
		|| '' topology.GetTopogeomElements(''
		|| quote_literal(toponame) || '', ''
		|| tg1.layer_id || '','' || tg1.id || ''); ''
	LOOP
		RETURN FALSE;
	END LOOP;
	RETURN TRUE;
END
'
LANGUAGE 'plpgsql' _VOLATILE_STRICT;
--} equals(TopoGeometry, TopoGeometry)

--=} POSTGIS-SPECIFIC topology predicates


--=}  POSTGIS-SPECIFIC block

--={ ----------------------------------------------------------------
--  SQL/MM block
--
--  This part contains function in the SQL/MM specification
--
---------------------------------------------------------------------


--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.5
--
--  ST_GetFaceEdges(atopology, aface)
--
--
-- 
CREATEFUNCTION topology.ST_GetFaceEdges(varchar, integer)
	RETURNS setof topology.GetFaceEdges_ReturnType
AS
'
DECLARE
	atopology ALIAS FOR $1;
	aface ALIAS FOR $2;
	rec RECORD;
BEGIN
	--
	-- Atopology and aface are required
	-- 
	IF atopology IS NULL OR aface IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	RAISE EXCEPTION
		 ''ST_GetFaceEdges: not implemented yet'';


END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_GetFaceEdges


--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.16
--
--  ST_GetFaceGeometry(atopology, aface)
-- 
CREATEFUNCTION topology.ST_GetFaceGeometry(varchar, integer)
	RETURNS GEOMETRY AS
'
DECLARE
	atopology ALIAS FOR $1;
	aface ALIAS FOR $2;
	rec RECORD;
BEGIN
	--
	-- Atopology and aface are required
	-- 
	IF atopology IS NULL OR aface IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Construct face 
	-- 
	FOR rec IN EXECUTE ''SELECT polygonize(geom) FROM ( SELECT geom FROM ''
		|| quote_ident(atopology)
		|| ''.edge WHERE left_face = '' || aface || 
		'' OR right_face = '' || aface || '') as foo''
	LOOP
		RETURN geometryN(rec.polygonize, 1);
		--RETURN rec.polygonize;
	END LOOP;


	--
	-- No face found
	-- 
	RAISE EXCEPTION
	''SQL/MM Spatial exception - non-existent face.'';
END
' 
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_GetFaceGeometry


--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.1 
--
--  ST_AddIsoNode(atopology, aface, apoint)
--
CREATEFUNCTION topology.ST_AddIsoNode(varchar, integer, geometry)
	RETURNS INTEGER AS
'
DECLARE
	atopology ALIAS FOR $1;
	aface ALIAS FOR $2;
	apoint ALIAS FOR $3;
	rec RECORD;
	nodeid integer;
BEGIN

	--
	-- Atopology and apoint are required
	-- 
	IF atopology IS NULL OR apoint IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Apoint must be a point
	--
	IF substring(geometrytype(apoint), 1, 5) != ''POINT''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid point'';
	END IF;

	--
	-- Check if a coincident node already exists
	-- 
	-- We use index AND x/y equality
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node '' ||
		''WHERE geom && '' || quote_literal(apoint) || ''::geometry''
		||'' AND x(geom) = x(''||quote_literal(apoint)||''::geometry)''
		||'' AND y(geom) = y(''||quote_literal(apoint)||''::geometry)''
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - coincident node'';
	END LOOP;

	--
	-- Check if any edge crosses (intersects) this node
	-- I used _intersects_ here to include boundaries (endpoints)
	--
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge '' 
		|| ''WHERE geom && '' || quote_literal(apoint) 
		|| '' AND intersects(geom, '' || quote_literal(apoint)
		|| '')''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - edge crosses node.'';
	END LOOP;


	--
	-- Verify that aface contains apoint
	-- if aface is null no check is done
	--
	IF aface IS NOT NULL THEN

	FOR rec IN EXECUTE ''SELECT within(''
		|| quote_literal(apoint) || ''::geometry, 
		topology.ST_GetFaceGeometry(''
		|| quote_literal(atopology) || '', '' || aface ||
		''))''
	LOOP
		IF rec.within = ''f'' THEN
			RAISE EXCEPTION
			''SQL/MM Spatial exception - not within face'';
		ELSIF rec.within IS NULL THEN
			RAISE EXCEPTION
			''SQL/MM Spatial exception - non-existent face'';
		END IF;
	END LOOP;

	END IF;

	--
	-- Get new node id from sequence
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.node_node_id_seq'''')''
	LOOP
		nodeid = rec.nextval;
	END LOOP;

	--
	-- Insert the new row
	--
	IF aface IS NOT NULL THEN
		EXECUTE ''INSERT INTO '' || quote_ident(atopology)
			|| ''.node(node_id, geom, containing_face) 
			VALUES(''||nodeid||'',''||quote_literal(apoint)||
			'',''||aface||'')'';
	ELSE
		EXECUTE ''INSERT INTO '' || quote_ident(atopology)
			|| ''.node(node_id, geom) 
			VALUES(''||nodeid||'',''||quote_literal(apoint)||
			'')'';
	END IF;

	RETURN nodeid;
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_AddIsoNode

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.2 
--
--  ST_MoveIsoNode(atopology, anode, apoint)
--
CREATEFUNCTION topology.ST_MoveIsoNode(varchar, integer, geometry)
	RETURNS TEXT AS
'
DECLARE
	atopology ALIAS FOR $1;
	anode ALIAS FOR $2;
	apoint ALIAS FOR $3;
	rec RECORD;
BEGIN

	--
	-- All arguments are required
	-- 
	IF atopology IS NULL OR anode IS NULL OR apoint IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Apoint must be a point
	--
	IF substring(geometrytype(apoint), 1, 5) != ''POINT''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid point'';
	END IF;

	--
	-- Check node isolation.
	-- 
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge '' ||
		'' WHERE start_node =  '' || anode ||
		'' OR end_node = '' || anode 
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - not isolated node'';
	END LOOP;

	--
	-- Check if a coincident node already exists
	-- 
	-- We use index AND x/y equality
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node '' ||
		''WHERE geom && '' || quote_literal(apoint) || ''::geometry''
		||'' AND x(geom) = x(''||quote_literal(apoint)||''::geometry)''
		||'' AND y(geom) = y(''||quote_literal(apoint)||''::geometry)''
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - coincident node'';
	END LOOP;

	--
	-- Check if any edge crosses (intersects) this node
	-- I used _intersects_ here to include boundaries (endpoints)
	--
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge '' 
		|| ''WHERE geom && '' || quote_literal(apoint) 
		|| '' AND intersects(geom, '' || quote_literal(apoint)
		|| '')''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - edge crosses node.'';
	END LOOP;

	--
	-- Update node point
	--
	EXECUTE ''UPDATE '' || quote_ident(atopology) || ''.node ''
		|| '' SET geom = '' || quote_literal(apoint) 
		|| '' WHERE node_id = '' || anode;

	RETURN ''Isolated Node '' || anode || '' moved to location ''
		|| x(apoint) || '','' || y(apoint);
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_MoveIsoNode

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.3 
--
--  ST_RemoveIsoNode(atopology, anode)
--
CREATEFUNCTION topology.ST_RemoveIsoNode(varchar, integer)
	RETURNS TEXT AS
'
DECLARE
	atopology ALIAS FOR $1;
	anode ALIAS FOR $2;
	rec RECORD;
BEGIN

	--
	-- Atopology and apoint are required
	-- 
	IF atopology IS NULL OR anode IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Check node isolation.
	-- 
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE start_node =  '' || anode ||
		'' OR end_node = '' || anode 
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - not isolated node'';
	END LOOP;

	EXECUTE ''DELETE FROM '' || quote_ident(atopology) || ''.node ''
		|| '' WHERE node_id = '' || anode;

	RETURN ''Isolated node '' || anode || '' removed'';
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_RemoveIsoNode

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.7 
--
--  ST_RemoveIsoEdge(atopology, anedge)
--
CREATEFUNCTION topology.ST_RemoveIsoEdge(varchar, integer)
	RETURNS TEXT AS
'
DECLARE
	atopology ALIAS FOR $1;
	anedge ALIAS FOR $2;
	edge RECORD;
	rec RECORD;
	ok BOOL;
BEGIN

	--
	-- Atopology and anedge are required
	-- 
	IF atopology IS NULL OR anedge IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Check node existance
	-- 
	ok = false;
	FOR edge IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE edge_id =  '' || anedge
	LOOP
		ok = true;
	END LOOP;
	IF NOT ok THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - non-existent edge'';
	END IF;

	--
	-- Check node isolation
	-- 
	IF edge.left_face != edge.right_face THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - not isolated edge'';
	END IF;

	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data '' 
		|| '' WHERE edge_id !=  '' || anedge
		|| '' AND ( start_node = '' || edge.start_node
		|| '' OR start_node = '' || edge.end_node
		|| '' OR end_node = '' || edge.start_node
		|| '' OR end_node = '' || edge.end_node
		|| '' ) ''
	LOOP
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - not isolated edge'';
	END LOOP;

	--
	-- Delete the edge
	--
	EXECUTE ''DELETE FROM '' || quote_ident(atopology) || ''.edge_data ''
		|| '' WHERE edge_id = '' || anedge;

	RETURN ''Isolated edge '' || anedge || '' removed'';
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_RemoveIsoEdge

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.8 
--
--  ST_NewEdgesSplit(atopology, anedge, apoint)
--
CREATEFUNCTION topology.ST_NewEdgesSplit(varchar, integer, geometry)
	RETURNS INTEGER AS
'
DECLARE
	atopology ALIAS FOR $1;
	anedge ALIAS FOR $2;
	apoint ALIAS FOR $3;
	oldedge RECORD;
	rec RECORD;
	tmp integer;
	topoid integer;
	nodeid integer;
	nodepos float8;
	edgeid1 integer;
	edgeid2 integer;
	edge1 geometry;
	edge2 geometry;
	ok BOOL;
BEGIN

	--
	-- All args required
	-- 
	IF atopology IS NULL OR anedge IS NULL OR apoint IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Check node existance
	-- 
	ok = false;
	FOR oldedge IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE edge_id =  '' || anedge
	LOOP
		ok = true;
	END LOOP;
	IF NOT ok THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - non-existent edge'';
	END IF;

	--
	-- Check that given point is Within(anedge.geom)
	-- 
	IF NOT within(apoint, oldedge.geom) THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - point not on edge'';
	END IF;

	--
	-- Check if a coincident node already exists
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node '' ||
		''WHERE geom && '' || quote_literal(apoint) || ''::geometry''
		||'' AND x(geom) = x(''||quote_literal(apoint)||''::geometry)''
		||'' AND y(geom) = y(''||quote_literal(apoint)||''::geometry)''
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - coincident node'';
	END LOOP;

	--
	-- Get new node id
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.node_node_id_seq'''')''
	LOOP
		nodeid = rec.nextval;
	END LOOP;

	--RAISE NOTICE ''Next node id = % '', nodeid;

	--
	-- Add the new node 
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.node(node_id, geom) 
		VALUES(''||nodeid||'',''||quote_literal(apoint)||
		'')'';

	--
	-- Delete the old edge
	--
	EXECUTE ''DELETE FROM '' || quote_ident(atopology) || ''.edge_data ''
		|| '' WHERE edge_id = '' || anedge;

	--
	-- Compute new edges
	--
	nodepos = line_locate_point(oldedge.geom, apoint);
	edge1 = line_substring(oldedge.geom, 0, nodepos);
	edge2 = line_substring(oldedge.geom, nodepos, 1);

	--
	-- Get ids for the new edges 
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.edge_data_edge_id_seq'''')''
	LOOP
		edgeid1 = rec.nextval;
	END LOOP;
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.edge_data_edge_id_seq'''')''
	LOOP
		edgeid2 = rec.nextval;
	END LOOP;

	--RAISE NOTICE ''EdgeId1 % EdgeId2 %'', edgeid1, edgeid2;

	--
	-- Insert the two new edges
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.edge VALUES(''
		||edgeid1||'',''||oldedge.start_node
		||'',''||nodeid
		||'',''||edgeid2
		||'',''||oldedge.next_right_edge
		||'',''||oldedge.left_face
		||'',''||oldedge.right_face
		||'',''||quote_literal(edge1)
		||'')'';

	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.edge VALUES(''
		||edgeid2||'',''||nodeid
		||'',''||oldedge.end_node
		||'',''||oldedge.next_left_edge
		||'',-''||edgeid1
		||'',''||oldedge.left_face
		||'',''||oldedge.right_face
		||'',''||quote_literal(edge2)
		||'')'';

	--
	-- Update all next edge references to match new layout
	--

	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET next_right_edge = ''
		|| edgeid2
		|| '',''
		|| '' abs_next_right_edge = '' || edgeid2
		|| '' WHERE next_right_edge = '' || anedge;
	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET next_right_edge = ''
		|| -edgeid1
		|| '',''
		|| '' abs_next_right_edge = '' || edgeid1
		|| '' WHERE next_right_edge = '' || -anedge;

	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET next_left_edge = ''
		|| edgeid1
		|| '',''
		|| '' abs_next_left_edge = '' || edgeid1
		|| '' WHERE next_left_edge = '' || anedge;
	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET ''
		|| '' next_left_edge = '' || -edgeid2
		|| '',''
		|| '' abs_next_left_edge = '' || edgeid2
		|| '' WHERE next_left_edge = '' || -anedge;

	-- Get topology id
        SELECT id FROM topology.topology into topoid
                WHERE name = atopology;

	--
	-- Update references in the Relation table.
	-- We only take into considerations non-hierarchical
	-- TopoGeometry here, for obvious reasons.
	--
	FOR rec IN EXECUTE ''SELECT r.* FROM ''
		|| quote_ident(atopology)
		|| ''.relation r, topology.layer l ''
		|| '' WHERE ''
		|| '' l.topology_id = '' || topoid
		|| '' AND l.level = 0 ''
		|| '' AND l.layer_id = r.layer_id ''
		|| '' AND abs(r.element_id) = '' || anedge
		|| '' AND r.element_type = 2''
	LOOP
		--RAISE NOTICE ''TopoGeometry % in layer % contains the edge being split'', rec.topogeo_id, rec.layer_id;

		-- Delete old reference
		EXECUTE ''DELETE FROM '' || quote_ident(atopology)
			|| ''.relation ''
			|| '' WHERE ''
			|| ''layer_id = '' || rec.layer_id
			|| '' AND ''
			|| ''topogeo_id = '' || rec.topogeo_id
			|| '' AND ''
			|| ''element_type = '' || rec.element_type
			|| '' AND ''
			|| ''abs(element_id) = '' || anedge;

		-- Add new reference to edge1
		IF rec.element_id < 0 THEN
			tmp = -edgeid1;
		ELSE
			tmp = edgeid1;
		END IF;
		EXECUTE ''INSERT INTO '' || quote_ident(atopology)
			|| ''.relation ''
			|| '' VALUES( ''
			|| rec.topogeo_id
			|| '',''
			|| rec.layer_id
			|| '',''
			|| tmp
			|| '',''
			|| rec.element_type
			|| '')'';

		-- Add new reference to edge2
		IF rec.element_id < 0 THEN
			tmp = -edgeid2;
		ELSE
			tmp = edgeid2;
		END IF;
		EXECUTE ''INSERT INTO '' || quote_ident(atopology)
			|| ''.relation ''
			|| '' VALUES( ''
			|| rec.topogeo_id
			|| '',''
			|| rec.layer_id
			|| '',''
			|| tmp
			|| '',''
			|| rec.element_type
			|| '')'';
			
	END LOOP;

	--RAISE NOTICE ''Edge % split in edges % and % by node %'',
	--	anedge, edgeid1, edgeid2, nodeid;

	RETURN nodeid; 
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_NewEdgesSplit

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.9 
--
--  ST_ModEdgesSplit(atopology, anedge, apoint)
--
CREATEFUNCTION topology.ST_ModEdgesSplit(varchar, integer, geometry)
	RETURNS INTEGER AS
'
DECLARE
	atopology ALIAS FOR $1;
	anedge ALIAS FOR $2;
	apoint ALIAS FOR $3;
	oldedge RECORD;
	rec RECORD;
	tmp integer;
	topoid integer;
	nodeid integer;
	nodepos float8;
	newedgeid integer;
	newedge1 geometry;
	newedge2 geometry;
	query text;
	ok BOOL;
BEGIN

	--
	-- All args required
	-- 
	IF atopology IS NULL OR anedge IS NULL OR apoint IS NULL THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Check node existance
	-- 
	ok = false;
	FOR oldedge IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE edge_id =  '' || anedge
	LOOP
		ok = true;
	END LOOP;
	IF NOT ok THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - non-existent edge'';
	END IF;

	--
	-- Check that given point is Within(anedge.geom)
	-- 
	IF NOT within(apoint, oldedge.geom) THEN
		RAISE EXCEPTION
		  ''SQL/MM Spatial exception - point not on edge'';
	END IF;

	--
	-- Check if a coincident node already exists
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node '' ||
		''WHERE geom && '' || quote_literal(apoint) || ''::geometry''
		||'' AND x(geom) = x(''||quote_literal(apoint)||''::geometry)''
		||'' AND y(geom) = y(''||quote_literal(apoint)||''::geometry)''
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - coincident node'';
	END LOOP;

	--
	-- Get new node id
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.node_node_id_seq'''')''
	LOOP
		nodeid = rec.nextval;
	END LOOP;

	--RAISE NOTICE ''Next node id = % '', nodeid;

	--
	-- Add the new node 
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.node(node_id, geom) 
		VALUES(''||nodeid||'',''||quote_literal(apoint)||
		'')'';

	--
	-- Compute new edge
	--
	nodepos = line_locate_point(oldedge.geom, apoint);
	newedge1 = line_substring(oldedge.geom, 0, nodepos);
	newedge2 = line_substring(oldedge.geom, nodepos, 1);


	--
	-- Get ids for the new edge
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.edge_data_edge_id_seq'''')''
	LOOP
		newedgeid = rec.nextval;
	END LOOP;

	--
	-- Insert the new edge
	--
	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.edge ''
		|| ''(edge_id, start_node, end_node,''
		|| ''next_left_edge, next_right_edge,''
		|| ''left_face, right_face, geom) ''
		|| ''VALUES(''
		||newedgeid||'',''||nodeid
		||'',''||oldedge.end_node
		||'',''||oldedge.next_left_edge
		||'',-''||anedge
		||'',''||oldedge.left_face
		||'',''||oldedge.right_face
		||'',''||quote_literal(newedge2)
		||'')'';

	--
	-- Update the old edge
	--
	EXECUTE ''UPDATE '' || quote_ident(atopology) || ''.edge_data ''
		|| '' SET geom = '' || quote_literal(newedge1)
		|| '',''
		|| '' next_left_edge = '' || newedgeid
		|| '',''
		|| '' end_node = '' || nodeid
		|| '' WHERE edge_id = '' || anedge;


	--
	-- Update all next edge references to match new layout
	--

	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET next_right_edge = ''
		|| -newedgeid 
		|| '',''
		|| '' abs_next_right_edge = '' || newedgeid
		|| '' WHERE next_right_edge = '' || -anedge;

	EXECUTE ''UPDATE '' || quote_ident(atopology)
		|| ''.edge_data SET ''
		|| '' next_left_edge = '' || -newedgeid
		|| '',''
		|| '' abs_next_left_edge = '' || newedgeid
		|| '' WHERE next_left_edge = '' || -anedge;

	-- Get topology id
        SELECT id FROM topology.topology into topoid
                WHERE name = atopology;

	--
	-- Update references in the Relation table.
	-- We only take into considerations non-hierarchical
	-- TopoGeometry here, for obvious reasons.
	--
	FOR rec IN EXECUTE ''SELECT r.* FROM ''
		|| quote_ident(atopology)
		|| ''.relation r, topology.layer l ''
		|| '' WHERE ''
		|| '' l.topology_id = '' || topoid
		|| '' AND l.level = 0 ''
		|| '' AND l.layer_id = r.layer_id ''
		|| '' AND abs(r.element_id) = '' || anedge
		|| '' AND r.element_type = 2''
	LOOP
		--RAISE NOTICE ''TopoGeometry % in layer % contains the edge being split (%) - updating to add new edge %'', rec.topogeo_id, rec.layer_id, anedge, newedgeid;

		-- Add new reference to edge1
		IF rec.element_id < 0 THEN
			tmp = -newedgeid;
		ELSE
			tmp = newedgeid;
		END IF;
		query = ''INSERT INTO '' || quote_ident(atopology)
			|| ''.relation ''
			|| '' VALUES( ''
			|| rec.topogeo_id
			|| '',''
			|| rec.layer_id
			|| '',''
			|| tmp
			|| '',''
			|| rec.element_type
			|| '')'';

		--RAISE NOTICE ''%'', query;
		EXECUTE query;
	END LOOP;

	--RAISE NOTICE ''Edge % split in edges % and % by node %'',
	--	anedge, anedge, newedgeid, nodeid;

	RETURN nodeid; 
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_ModEdgesSplit

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.4 
--
--  ST_AddIsoEdge(atopology, anode, anothernode, acurve)
-- 
CREATEFUNCTION topology.ST_AddIsoEdge(varchar, integer, integer, geometry)
	RETURNS INTEGER AS
'
DECLARE
	atopology ALIAS FOR $1;
	anode ALIAS FOR $2;
	anothernode ALIAS FOR $3;
	acurve ALIAS FOR $4;
	aface INTEGER;
	face GEOMETRY;
	snodegeom GEOMETRY;
	enodegeom GEOMETRY;
	count INTEGER;
	rec RECORD;
	edgeid INTEGER;
BEGIN

	--
	-- All arguments required
	-- 
	IF atopology IS NULL
	   OR anode IS NULL
	   OR anothernode IS NULL
	   OR acurve IS NULL
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Acurve must be a LINESTRING
	--
	IF substring(geometrytype(acurve), 1, 4) != ''LINE''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid curve'';
	END IF;

	--
	-- Acurve must be a simple
	--
	IF NOT issimple(acurve)
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - curve not simple'';
	END IF;

	--
	-- Check for:
	--    existence of nodes
	--    nodes faces match
	-- Extract:
	--    nodes face id
	--    nodes geoms
	--
	aface := NULL;
	count := 0;
	FOR rec IN EXECUTE ''SELECT geom, containing_face, node_id FROM ''
		|| quote_ident(atopology) || ''.node
		WHERE node_id = '' || anode ||
		'' OR node_id = '' || anothernode
	LOOP 
		IF count > 0 AND aface != rec.containing_face THEN
			RAISE EXCEPTION
			''SQL/MM Spatial exception - nodes in different faces'';
		ELSE
			aface := rec.containing_face;
		END IF;

		-- Get nodes geom
		IF rec.node_id = anode THEN
			snodegeom = rec.geom;
		ELSE
			enodegeom = rec.geom;
		END IF;

		count = count+1;

	END LOOP;
	IF count < 2 THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - non-existent node'';
	END IF;


	--
	-- Check nodes isolation.
	-- 
	FOR rec IN EXECUTE ''SELECT edge_id FROM ''
		|| quote_ident(atopology) || ''.edge_data '' ||
		'' WHERE start_node =  '' || anode ||
		'' OR end_node = '' || anode ||
		'' OR start_node = '' || anothernode ||
		'' OR end_node = '' || anothernode
	LOOP
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - not isolated node'';
	END LOOP;

	--
	-- Check acurve to be within endpoints containing face 
	-- (unless it is the world face, I suppose)
	-- 
	IF aface IS NOT NULL THEN

		--
		-- Extract endpoints face geometry
		--
		FOR rec IN EXECUTE ''SELECT topology.ST_GetFaceGeometry(''
			|| quote_literal(atopology) ||
			'','' || aface || '') as face''
		LOOP
			face := rec.face;
		END LOOP;

		--
		-- Check acurve to be within face
		--
		IF ! within(acurve, face) THEN
	RAISE EXCEPTION
	''SQL/MM Spatial exception - geometry not within face.'';
		END IF;

	END IF;

	--
	-- l) Check that start point of acurve match start node
	-- geoms.
	-- 
	IF x(snodegeom) != x(StartPoint(acurve)) OR
	   y(snodegeom) != y(StartPoint(acurve)) THEN
  RAISE EXCEPTION
  ''SQL/MM Spatial exception - start node not geometry start point.'';
	END IF;

	--
	-- m) Check that end point of acurve match end node
	-- geoms.
	-- 
	IF x(enodegeom) != x(EndPoint(acurve)) OR
	   y(enodegeom) != y(EndPoint(acurve)) THEN
  RAISE EXCEPTION
  ''SQL/MM Spatial exception - end node not geometry end point.'';
	END IF;

	--
	-- n) Check if curve crosses (contains) any node
	-- I used _contains_ here to leave endpoints out
	-- 
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node ''
		|| '' WHERE geom && '' || quote_literal(acurve) 
		|| '' AND contains('' || quote_literal(acurve)
		|| '',geom)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry crosses a node'';
	END LOOP;

	--
	-- o) Check if curve intersects any other edge
	-- 
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data
		WHERE geom && '' || quote_literal(acurve) || ''::geometry
		AND intersects(geom, '' || quote_literal(acurve) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry intersects an edge'';
	END LOOP;

	--
	-- Get new edge id from sequence
	--
	FOR rec IN EXECUTE ''SELECT nextval('''''' ||
		atopology || ''.edge_data_edge_id_seq'''')''
	LOOP
		edgeid = rec.nextval;
	END LOOP;

	--
	-- Insert the new row
	--
	IF aface IS NULL THEN aface := 0; END IF;

	EXECUTE ''INSERT INTO '' || quote_ident(atopology)
		|| ''.edge VALUES(''||edgeid||'',''||anode||
			'',''||anothernode||'',''
			||(-edgeid)||'',''||edgeid||'',''
			||aface||'',''||aface||'',''
			||quote_literal(acurve)||'')'';

	RETURN edgeid;

END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_AddIsoEdge

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.6
--
--  ST_ChangeEdgeGeom(atopology, anedge, acurve)
-- 
CREATEFUNCTION topology.ST_ChangeEdgeGeom(varchar, integer, geometry)
	RETURNS TEXT AS
'
DECLARE
	atopology ALIAS FOR $1;
	anedge ALIAS FOR $2;
	acurve ALIAS FOR $3;
	aface INTEGER;
	face GEOMETRY;
	snodegeom GEOMETRY;
	enodegeom GEOMETRY;
	count INTEGER;
	rec RECORD;
	edgeid INTEGER;
	oldedge RECORD;
BEGIN

	--
	-- All arguments required
	-- 
	IF atopology IS NULL
	   OR anedge IS NULL
	   OR acurve IS NULL
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Acurve must be a LINESTRING
	--
	IF substring(geometrytype(acurve), 1, 4) != ''LINE''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid curve'';
	END IF;

	--
	-- Acurve must be a simple
	--
	IF NOT issimple(acurve)
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - curve not simple'';
	END IF;

	--
	-- e) Check StartPoint consistency
	--
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data e, ''
		|| quote_ident(atopology) || ''.node n ''
		|| '' WHERE e.edge_id = '' || anedge
		|| '' AND n.node_id = e.start_node ''
		|| '' AND ( x(n.geom) != ''
		|| x(StartPoint(acurve))
		|| '' OR y(n.geom) != ''
		|| y(StartPoint(acurve))
		|| '')''
	LOOP 
		RAISE EXCEPTION
		''SQL/MM Spatial exception - start node not geometry start point.'';
	END LOOP;

	--
	-- f) Check EndPoint consistency
	--
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data e, ''
		|| quote_ident(atopology) || ''.node n ''
		|| '' WHERE e.edge_id = '' || anedge
		|| '' AND n.node_id = e.end_node ''
		|| '' AND ( x(n.geom) != ''
		|| x(EndPoint(acurve))
		|| '' OR y(n.geom) != ''
		|| y(EndPoint(acurve))
		|| '')''
	LOOP 
		RAISE EXCEPTION
		''SQL/MM Spatial exception - end node not geometry end point.'';
	END LOOP;

	--
	-- g) Check if curve crosses any node
	-- _within_ used to let endpoints out
	-- 
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node
		WHERE geom && '' || quote_literal(acurve) || ''::geometry
		AND within(geom, '' || quote_literal(acurve) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry crosses a node'';
	END LOOP;

	--
	-- h) Check if curve intersects any other edge
	-- 
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data ''
		|| '' WHERE edge_id != '' || anedge
		|| '' AND geom && ''
		|| quote_literal(acurve) || ''::geometry ''
		|| '' AND intersects(geom, ''
		|| quote_literal(acurve) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry intersects an edge'';
	END LOOP;

	--
	-- Update edge geometry
	--
	EXECUTE ''UPDATE '' || quote_ident(atopology) || ''.edge_data ''
		|| '' SET geom = '' || quote_literal(acurve) 
		|| '' WHERE edge_id = '' || anedge;

	RETURN ''Edge '' || anedge || '' changed'';

END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_ChangeEdgeGeom

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.12
--
--  ST_AddEdgeNewFaces(atopology, anode, anothernode, acurve)
--
CREATEFUNCTION topology.ST_AddEdgeNewFaces(varchar, integer, integer, geometry)
	RETURNS INTEGER AS
'
DECLARE
	atopology ALIAS FOR $1;
	anode ALIAS FOR $2;
	anothernode ALIAS FOR $3;
	acurve ALIAS FOR $4;
	rec RECORD;
	i INTEGER;
	az FLOAT8;
	azdif FLOAT8;
	myaz FLOAT8;
	minazimuth FLOAT8;
	maxazimuth FLOAT8;
	p2 GEOMETRY;
BEGIN

	--
	-- All args required
	-- 
	IF atopology IS NULL
		OR anode IS NULL
		OR anothernode IS NULL
		OR acurve IS NULL
	THEN
		RAISE EXCEPTION ''SQL/MM Spatial exception - null argument'';
	END IF;

	--
	-- Acurve must be a LINESTRING
	--
	IF substring(geometrytype(acurve), 1, 4) != ''LINE''
	THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - invalid curve'';
	END IF;
	
	--
	-- Curve must be simple
	--
	IF NOT issimple(acurve) THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - curve not simple'';
	END IF;

	-- 
	-- Check endpoints existance and match with Curve geometry
	--
	i=0;
	FOR rec IN EXECUTE ''SELECT ''
		|| '' CASE WHEN node_id = '' || anode
		|| '' THEN 1 WHEN node_id = '' || anothernode
		|| '' THEN 0 END AS start, geom FROM ''
		|| quote_ident(atopology)
		|| ''.node ''
		|| '' WHERE node_id IN ( ''
		|| anode || '','' || anothernode
		|| '')''
	LOOP
		IF rec.start THEN
			IF NOT Equals(rec.geom, StartPoint(acurve))
			THEN
	RAISE EXCEPTION
	''SQL/MM Spatial exception - start node not geometry start point.'';
			END IF;
		ELSE
			IF NOT Equals(rec.geom, EndPoint(acurve))
			THEN
	RAISE EXCEPTION
	''SQL/MM Spatial exception - end node not geometry end point.'';
			END IF;
		END IF;

		i=i+1;
	END LOOP;

	IF i < 2 THEN
		RAISE EXCEPTION
		 ''SQL/MM Spatial exception - non-existent node'';
	END IF;

	--
	-- Check if this geometry crosses any node
	--
	FOR rec IN EXECUTE ''SELECT node_id FROM ''
		|| quote_ident(atopology) || ''.node
		WHERE geom && '' || quote_literal(acurve) || ''::geometry
		AND within(geom, '' || quote_literal(acurve) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry crosses a node'';
	END LOOP;

	--
	-- Check if this geometry crosses any existing edge
	--
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data
		WHERE geom && '' || quote_literal(acurve) || ''::geometry
		AND crosses(geom, '' || quote_literal(acurve) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - geometry crosses an edge'';
	END LOOP;

	--
	-- Check if another edge share this edge endpoints
	--
	FOR rec IN EXECUTE ''SELECT * FROM ''
		|| quote_ident(atopology) || ''.edge_data ''
		|| '' WHERE ''
		|| '' geom && '' || quote_literal(acurve) || ''::geometry ''
		|| '' AND ''
		|| '' ( (''
		|| '' start_node = '' || anode
		|| '' AND ''
		|| '' end_node = '' || anothernode
		|| '' ) OR ( ''
		|| '' end_node = '' || anode
		|| '' AND ''
		|| '' start_node = '' || anothernode
		|| '' ) )''
		|| '' AND ''
		|| ''equals(geom,'' || quote_literal(acurve) || ''::geometry)''
	LOOP
		RAISE EXCEPTION
		''SQL/MM Spatial exception - coincident edge'';
	END LOOP;

	---------------------------------------------------------------
	--
	-- All checks passed, time to extract informations about
	-- endpoints:
	--
	--      next_left_edge
	--      next_right_edge
	--	left_face
	--	right_face
	--
	---------------------------------------------------------------

	--
	--
	-- Compute next_left_edge 
	--
	-- We fetch all edges with an endnode equal to
	-- this edge end_node (anothernode).
	-- For each edge we compute azimuth of the segment(s).
	-- Of interest are the edges with closest (smaller
	-- and bigger) azimuths then the azimuth of
	-- this edge last segment.
	--

	myaz = azimuth(EndPoint(acurve), PointN(acurve, NumPoints(acurve)-1));
	RAISE NOTICE ''My end-segment azimuth: %'', myaz;
	FOR rec IN EXECUTE ''SELECT ''
		|| ''edge_id, end_node, start_node, geom''
		|| '' FROM ''
		|| quote_ident(atopology)
		|| ''.edge_data ''
		|| '' WHERE ''
		|| '' end_node = '' || anothernode
		|| '' OR ''
		|| '' start_node = '' || anothernode
	LOOP

		IF rec.start_node = anothernode THEN
			--
			-- Edge starts at our node, we compute
			-- azimuth from node to its second point
			--
			az = azimuth(EndPoint(acurve),
				PointN(rec.geom, 2));

			RAISE NOTICE ''Edge % starts at node % - azimuth %'',
				rec.edge_id, rec.start_node, az;
		END IF;

		IF rec.end_node = anothernode THEN
			--
			-- Edge ends at our node, we compute
			-- azimuth from node to its second-last point
			--
			az = azimuth(EndPoint(acurve),
				PointN(rec.geom, NumPoints(rec.geom)-1));

			RAISE NOTICE ''Edge % ends at node % - azimuth %'',
				rec.edge_id, rec.end_node, az;
		END IF;
	END LOOP;


	RAISE EXCEPTION ''Not implemented yet'';
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_AddEdgeNewFaces

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.17
--
--  ST_InitTopoGeo(atopology)
--
CREATEFUNCTION topology.ST_InitTopoGeo(varchar)
RETURNS text
AS '
DECLARE
	atopology alias for $1;
	rec RECORD;
	topology_id numeric;
BEGIN
	IF atopology IS NULL THEN
		RAISE EXCEPTION ''SQL/MM Spatial exception - null argument'';
	END IF;

	FOR rec IN SELECT * FROM pg_namespace WHERE text(nspname) = atopology
	LOOP
		RAISE EXCEPTION ''SQL/MM Spatial exception - schema already exists'';
	END LOOP;

	FOR rec IN EXECUTE ''SELECT topology.CreateTopology(''
		||quote_literal(atopology)|| '') as id''
	LOOP
		topology_id := rec.id;
	END LOOP;

	RETURN ''Topology-Geometry '' || quote_literal(atopology)
		|| '' (id:'' || topology_id || '') created. '';
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_InitTopoGeo

--{
-- Topo-Geo and Topo-Net 3: Routine Details
-- X.3.18
--
--  ST_CreateTopoGeo(atopology, acollection)
--
CREATEFUNCTION topology.ST_CreateTopoGeo(varchar, geometry)
RETURNS text
AS '
DECLARE
	atopology alias for $1;
	acollection alias for $2;
	typ char(4);
	rec RECORD;
	ret int;
	schemaoid oid;
BEGIN
	IF atopology IS NULL OR acollection IS NULL THEN
		RAISE EXCEPTION ''SQL/MM Spatial exception - null argument'';
	END IF;

	-- Verify existance of the topology schema 
	FOR rec in EXECUTE ''SELECT oid,count(*) FROM pg_namespace WHERE ''
		|| '' nspname = '' || quote_literal(atopology)
		|| '' GROUP BY oid''
		
	LOOP
		IF rec.count < 1 THEN
	RAISE EXCEPTION ''SQL/MM Spatial exception - non-existent schema'';
		END IF;
		schemaoid := rec.oid;
	END LOOP;

	-- Verify existance of the topology views in the topology schema 
	FOR rec in EXECUTE ''SELECT count(*) FROM pg_class WHERE ''
		|| '' relnamespace = '' || schemaoid 
		|| '' and relname = ''''node''''''
		|| '' OR relname = ''''edge''''''
		|| '' OR relname = ''''face''''''
	LOOP
		IF rec.count < 3 THEN
	RAISE EXCEPTION ''SQL/MM Spatial exception - non-existent view'';
		END IF;
	END LOOP;

	-- Verify the topology views in the topology schema to be empty
	FOR rec in EXECUTE
		''SELECT count(*) FROM ''
		|| quote_ident(atopology) || ''.edge_data ''
		|| '' UNION '' ||
		''SELECT count(*) FROM ''
		|| quote_ident(atopology) || ''.node ''
	LOOP
		IF rec.count > 0 THEN
	RAISE EXCEPTION ''SQL/MM Spatial exception - non-empty view'';
		END IF;
	END LOOP;

	-- face check is separated as it will contain a single (world)
	-- face record
	FOR rec in EXECUTE
		''SELECT count(*) FROM ''
		|| quote_ident(atopology) || ''.face ''
	LOOP
		IF rec.count != 1 THEN
	RAISE EXCEPTION ''SQL/MM Spatial exception - non-empty face view'';
		END IF;
	END LOOP;

	-- 
	-- LOOP through the elements invoking the specific function
	-- 
	FOR rec IN SELECT geom(dump(acollection))
	LOOP
		typ := substring(geometrytype(rec.geom), 1, 3);

		IF typ = ''LIN'' THEN
	SELECT topology.TopoGeo_addLinestring(atopology, rec.geom) INTO ret;
		ELSIF typ = ''POI'' THEN
	SELECT topology.TopoGeo_AddPoint(atopology, rec.geom) INTO ret;
		ELSIF typ = ''POL'' THEN
	SELECT topology.TopoGeo_AddPolygon(atopology, rec.geom) INTO ret;
		ELSE
	RAISE EXCEPTION ''ST_CreateTopoGeo got unknown geometry type: %'', typ;
		END IF;

	END LOOP;

	RETURN ''Topology '' || atopology || '' populated'';

	RAISE EXCEPTION ''ST_CreateTopoGeo not implemente yet'';
END
'
LANGUAGE 'plpgsql' _VOLATILE;
--} ST_CreateTopoGeo

--=}  SQL/MM block

COMMIT;

