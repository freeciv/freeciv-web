-- MySQL dump 10.13  Distrib 5.5.31, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: freeciv_web
-- ------------------------------------------------------
-- Server version	5.5.31-0ubuntu0.13.04.1

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
-- Table structure for table `auth`
--

DROP TABLE IF EXISTS `auth`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `auth` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(32) DEFAULT NULL,
  `name` varchar(32) DEFAULT NULL,
  `password` varchar(32) DEFAULT NULL,
  `email` varchar(128) DEFAULT NULL,
  `createtime` int(11) DEFAULT NULL,
  `accesstime` int(11) DEFAULT NULL,
  `address` varchar(255) DEFAULT NULL,
  `createaddress` varchar(255) DEFAULT NULL,
  `logincount` int(11) DEFAULT '0',
  `fb_uid` bigint(20) DEFAULT NULL,
  `email_hash` varchar(64) DEFAULT NULL,
  `openid_user` varchar(512) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `username` (`username`),
  KEY `id_fb_uid` (`fb_uid`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;


--
-- Table structure for table `players`
--

DROP TABLE IF EXISTS `players`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `players` (
  `hostport` varchar(255) NOT NULL DEFAULT 'unknown:5556',
  `name` varchar(64) NOT NULL DEFAULT 'name',
  `user` varchar(64) DEFAULT NULL,
  `nation` varchar(64) DEFAULT NULL,
  `type` varchar(10) DEFAULT NULL,
  `host` varchar(255) DEFAULT 'unknown',
  `flag` varchar(128) DEFAULT NULL,
  PRIMARY KEY (`hostport`,`name`)
) ENGINE=MEMORY DEFAULT CHARSET=latin1 MAX_ROWS=8192;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `savegames`
--

DROP TABLE IF EXISTS `savegames`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `savegames` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `username` varchar(32) NOT NULL,
  `title` varchar(64) NOT NULL,
  `digest` varchar(256) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=30 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `servers`
--

DROP TABLE IF EXISTS `servers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `servers` (
  `host` varchar(249) NOT NULL DEFAULT 'unknown',
  `port` int(11) NOT NULL DEFAULT '5556',
  `version` varchar(64) DEFAULT 'unknown',
  `patches` varchar(255) DEFAULT 'none',
  `capability` varchar(255) DEFAULT NULL,
  `state` varchar(20) DEFAULT 'Pregame',
  `ruleset` varchar(255) DEFAULT 'Unknown',
  `topic` varchar(255) DEFAULT 'none',
  `type` varchar(255) DEFAULT 'none',
  `humans` varchar(64) DEFAULT '0',
  `message` varchar(255) DEFAULT 'none',
  `stamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  `available` int(11) DEFAULT '0',
  `serverid` varchar(255) DEFAULT NULL,
  PRIMARY KEY (`host`,`port`)
) ENGINE=MEMORY DEFAULT CHARSET=latin1 MAX_ROWS=256;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `variables`
--

DROP TABLE IF EXISTS `variables`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `variables` (
  `hostport` varchar(64) NOT NULL DEFAULT '',
  `name` varchar(64) NOT NULL DEFAULT '',
  `value` varchar(64) DEFAULT NULL,
  PRIMARY KEY (`hostport`,`name`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

DROP EVENT IF EXISTS `freeciv_web_server_cleanup`;
CREATE EVENT freeciv_web_server_cleanup
    ON SCHEDULE
      EVERY 30 MINUTE
    COMMENT 'Removes unused Freeciv-web servers from metaserver'
    DO
      DELETE FROM servers where stamp <= DATE_SUB(NOW(), INTERVAL 30 MINUTE);

-- Dump completed on 2013-05-25 11:32:48
