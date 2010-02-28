-- MySQL dump 10.11
--
-- Host: localhost    Database: freecivmetaserver
-- ------------------------------------------------------
-- Server version	5.0.67

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `players`
--

DROP TABLE IF EXISTS `players`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `players` (
  `hostport` varchar(255) NOT NULL default 'unknown:5556',
  `name` varchar(64) NOT NULL default 'name',
  `user` varchar(64) default NULL,
  `nation` varchar(64) default NULL,
  `type` varchar(10) default NULL,
  `host` varchar(255) default 'unknown',
  PRIMARY KEY  (`hostport`,`name`)
) ENGINE=MEMORY DEFAULT CHARSET=latin1 MAX_ROWS=8192;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `players`
--

LOCK TABLES `players` WRITE;
/*!40000 ALTER TABLE `players` DISABLE KEYS */;
/*!40000 ALTER TABLE `players` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `servers`
--

DROP TABLE IF EXISTS `servers`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `servers` (
  `host` varchar(249) NOT NULL default 'unknown',
  `port` int(11) NOT NULL default '5556',
  `version` varchar(64) default 'unknown',
  `patches` varchar(255) default 'none',
  `capability` varchar(255) default NULL,
  `state` varchar(20) default 'Pregame',
  `topic` varchar(255) default 'none',
  `message` varchar(255) default 'none',
  `stamp` timestamp NOT NULL default CURRENT_TIMESTAMP on update CURRENT_TIMESTAMP,
  `available` int(11) default '0',
  `serverid` varchar(255) default NULL,
  PRIMARY KEY  (`host`,`port`)
) ENGINE=MEMORY DEFAULT CHARSET=latin1 MAX_ROWS=256;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `servers`
--

--
-- Table structure for table `variables`
--

DROP TABLE IF EXISTS `variables`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `variables` (
  `hostport` varchar(64) NOT NULL default '',
  `name` varchar(64) NOT NULL default '',
  `value` varchar(64) default NULL,
  PRIMARY KEY  (`hostport`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
SET character_set_client = @saved_cs_client;

--
-- Dumping data for table `variables`
--

LOCK TABLES `variables` WRITE;
/*!40000 ALTER TABLE `variables` DISABLE KEYS */;
INSERT INTO `variables` VALUES (':5558','minplayers','1'),(':5558','year','-4000'),(':5558','turn','0'),(':5558','endturn','5000'),(':5559','size','4'),(':5558','timeout','0'),(':5559','timeout','0'),(':5559','year','-4000'),(':5559','turn','0'),(':5559','endturn','5000'),(':5559','minplayers','1'),(':5559','maxplayers','30'),(':5559','allowtake','HAhadOo'),(':5559','generator','1'),(':5567','endturn','5000'),(':5556','timeout','0'),(':5556','year','-4000'),(':5556','generator','1'),(':5567','year','-4000'),(':5567','turn','0'),(':5557','maxplayers','30'),(':5557','timeout','0'),(':5557','year','-4000'),(':5557','turn','0'),(':5557','endturn','5000'),(':5557','minplayers','1'),(':5567','timeout','0'),(':5556','turn','0'),(':5556','endturn','5000'),(':5556','minplayers','1'),(':5556','maxplayers','30'),(':5556','allowtake','HAhadOo'),(':5562','minplayers','1'),(':5556','size','4'),(':5562','timeout','0'),(':5562','year','-4000'),(':5562','turn','0'),(':5562','endturn','5000'),(':5565','maxplayers','30'),(':5565','timeout','0'),(':5565','year','-4000'),(':5565','turn','0'),(':5565','endturn','5000'),(':5565','minplayers','1'),(':5560','maxplayers','30'),(':5560','timeout','0'),(':5560','year','-4000'),(':5560','turn','0'),(':5560','endturn','5000'),(':5560','minplayers','1'),(':5566','maxplayers','30'),(':5566','timeout','0'),(':5564','timeout','0'),(':5566','year','-4000'),(':5564','year','-4000'),(':5566','turn','0'),(':5564','turn','0'),(':5566','endturn','5000'),(':5564','endturn','5000'),(':5566','minplayers','1'),(':5564','minplayers','1'),(':5561','maxplayers','30'),(':5561','timeout','0'),(':5561','year','-4000'),(':5561','turn','0'),(':5561','endturn','5000'),(':5561','minplayers','1'),(':5563','maxplayers','30'),(':5563','timeout','0'),(':5563','year','-4000'),(':5563','turn','0'),(':5563','endturn','5000'),(':5563','minplayers','1'),(':5569','timeout','0'),(':5570','timeout','0'),(':5570','year','-4000'),(':5570','turn','0'),(':5570','endturn','5000'),(':5570','size','4'),(':5570','maxplayers','30'),(':5570','minplayers','1'),(':5570','allowtake','HAhadOo'),(':5570','generator','1'),(':5568','maxplayers','30'),(':5568','timeout','0'),(':5568','year','-4000'),(':5568','turn','0'),(':5568','endturn','5000'),(':5568','minplayers','1'),(':5558','maxplayers','30'),(':5558','allowtake','HAhadOo'),(':5558','generator','1'),(':5558','size','4'),(':5557','allowtake','HAhadOo'),(':5557','generator','1'),(':5557','size','4'),(':5567','minplayers','1'),(':5567','maxplayers','30'),(':5567','allowtake','HAhadOo'),(':5567','generator','1'),(':5567','size','4'),(':5562','maxplayers','30'),(':5562','allowtake','HAhadOo'),(':5562','generator','1'),(':5562','size','4'),(':5565','allowtake','HAhadOo'),(':5565','generator','1'),(':5565','size','4'),(':5563','allowtake','HAhadOo'),(':5563','generator','1'),(':5563','size','4'),(':5568','allowtake','HAhadOo'),(':5568','generator','1'),(':5568','size','4'),(':5560','allowtake','HAhadOo'),(':5560','generator','1'),(':5560','size','4'),(':5561','allowtake','HAhadOo'),(':5561','generator','1'),(':5561','size','4'),(':5564','maxplayers','30'),(':5566','allowtake','HAhadOo'),(':5564','allowtake','HAhadOo'),(':5566','generator','1'),(':5564','generator','1'),(':5566','size','4'),(':5564','size','4'),(':5569','year','-4000'),(':5569','turn','0'),(':5569','endturn','5000'),(':5569','minplayers','1'),(':5569','maxplayers','30'),(':5569','allowtake','HAhadOo'),(':5569','generator','1'),(':5569','size','4');
/*!40000 ALTER TABLE `variables` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2009-10-23 22:30:08


CREATE TABLE auth ( 
   id int(11) NOT NULL auto_increment, 
   username varchar(32) default NULL, 
   name varchar(32) default NULL, 
   password varchar(32) default NULL, 
   email varchar(128) default NULL, 
   createtime int(11) default NULL,
   accesstime int(11) default NULL, 
   address varchar(255) default NULL, 
   createaddress varchar(15) default NULL, 
   logincount int(11) default '0', 
   PRIMARY KEY  (id), 
   UNIQUE KEY name (name) 
 ) TYPE=MyISAM;
