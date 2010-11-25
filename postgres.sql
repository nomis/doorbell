SET client_encoding = 'UTF8';
SET standard_conforming_strings = off;
SET check_function_bodies = false;
SET client_min_messages = warning;
SET escape_string_warning = off;

CREATE TABLE doorbells (
	id serial NOT NULL,
	name text NOT NULL
);

CREATE TABLE dingdong (
	doorbell integer NOT NULL,
	start timestamp with time zone NOT NULL,
	stop timestamp with time zone,
	notify bool NOT NULL default 'f',
	CONSTRAINT valid_press CHECK ((stop >= start))
);

ALTER TABLE ONLY doorbells
	ADD CONSTRAINT doorbells_name_key UNIQUE (name);

ALTER TABLE ONLY doorbells
	ADD CONSTRAINT doorbells_pkey PRIMARY KEY (id);

ALTER TABLE ONLY dingdong
	ADD CONSTRAINT dingdong_pkey PRIMARY KEY (doorbell, start);

ALTER TABLE ONLY dingdong
	ADD CONSTRAINT dingdong_doorbell_fkey FOREIGN KEY (doorbell) REFERENCES doorbells(id);

CREATE RULE notify_delete AS ON DELETE TO dingdong DO NOTIFY changed;

CREATE RULE notify_insert AS ON INSERT TO dingdong DO NOTIFY changed;

CREATE RULE notify_update AS ON UPDATE TO dingdong DO NOTIFY changed;

CREATE TABLE pachube (
	id bigint NOT NULL,
	key text NOT NULL
);

ALTER TABLE ONLY pachube
	ADD CONSTRAINT pachube_pkey PRIMARY KEY (id);

