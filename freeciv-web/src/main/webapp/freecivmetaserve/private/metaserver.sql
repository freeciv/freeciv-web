-- MySQL dump 9.11
--
-- ------------------------------------------------------
-- Server version	4.0.27-standard

--
-- Table structure for table `players`
--

CREATE TABLE players (
  hostport varchar(255) NOT NULL default 'unknown:5556',
  name varchar(64) NOT NULL default 'name',
  user varchar(64) default NULL,
  nation varchar(64) default NULL,
  type varchar(10) default NULL,
  host varchar(255) default 'unknown',
  PRIMARY KEY  (hostport,name)
) TYPE=HEAP MAX_ROWS=8192;

--
-- Table structure for table `servers`
--

CREATE TABLE servers (
  host varchar(249) NOT NULL default 'unknown',
  port int(11) NOT NULL default '5556',
  version varchar(64) default 'unknown',
  patches varchar(255) default 'none',
  capability varchar(255) default NULL,
  state varchar(20) default 'Pregame',
  topic varchar(255) default 'none',
  message varchar(255) default 'none',
  stamp timestamp(14) NOT NULL,
  available int(11) default '0',
  serverid varchar(255) default NULL,
  PRIMARY KEY  (host,port)
) TYPE=HEAP MAX_ROWS=256;

--
-- Table structure for table `variables`
--

CREATE TABLE variables (
  hostport varchar(64) NOT NULL default '',
  name varchar(64) NOT NULL default '',
  value varchar(64) default NULL,
  PRIMARY KEY  (hostport,name)
) TYPE=MyISAM;

