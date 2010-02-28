-- MySQL dump 9.11
--
-- ------------------------------------------------------
-- Server version	4.0.27-standard

--
-- Table structure for table `clients`
--

CREATE TABLE clients (
  id int(11) NOT NULL auto_increment,
  date date NOT NULL default '0000-00-00',
  version varchar(60) NOT NULL default '',
  gui varchar(50) NOT NULL default '',
  os varchar(50) NOT NULL default '',
  osrev varchar(50) NOT NULL default '',
  arch varchar(50) NOT NULL default '',
  count int(11) NOT NULL default '0',
  tileset varchar(50) default 'unknown',
  PRIMARY KEY  (id),
  KEY date (date),
  KEY version (version),
  KEY gui (gui),
  KEY os (os),
  KEY arch (arch),
  KEY tileset (tileset)
) TYPE=MyISAM;

--
-- Table structure for table `poll_answers`
--

CREATE TABLE poll_answers (
  id int(11) NOT NULL auto_increment,
  poll_id int(11) default '0',
  answer text,
  votes int(11) default NULL,
  PRIMARY KEY  (id),
  KEY poll_id (poll_id)
) TYPE=MyISAM PACK_KEYS=1;

--
-- Table structure for table `poll_ips`
--

CREATE TABLE poll_ips (
  id int(11) NOT NULL auto_increment,
  ip varchar(15) default NULL,
  answer_id int(11) default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM PACK_KEYS=1;

--
-- Table structure for table `poll_queue`
--

CREATE TABLE poll_queue (
  id int(11) NOT NULL auto_increment,
  queue_num int(11) NOT NULL default '0',
  title text NOT NULL,
  approved tinyint(1) NOT NULL default '0',
  new tinyint(1) NOT NULL default '1',
  PRIMARY KEY  (id),
  UNIQUE KEY queue_num (queue_num)
) TYPE=MyISAM;

--
-- Table structure for table `poll_queue_answer`
--

CREATE TABLE poll_queue_answer (
  poll_queue_id int(11) default NULL,
  answer text NOT NULL,
  id int(10) unsigned NOT NULL auto_increment,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

--
-- Table structure for table `polls`
--

CREATE TABLE polls (
  id int(11) NOT NULL auto_increment,
  datetime datetime default NULL,
  title text,
  active enum('y','n') default NULL,
  PRIMARY KEY  (id),
  KEY active (active)
) TYPE=MyISAM PACK_KEYS=1;

