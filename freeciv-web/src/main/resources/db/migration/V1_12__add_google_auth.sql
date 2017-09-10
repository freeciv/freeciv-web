create table google_auth(
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(32) DEFAULT NULL,
  `subject` varchar(128) DEFAULT NULL,
  `email` varchar(128) DEFAULT NULL,
  `activated` int(1) DEFAULT 1,
  PRIMARY KEY (`id`),
  UNIQUE KEY `username` (`username`),
  UNIQUE KEY `subject` (`subject`),
  UNIQUE KEY `email` (`email`)
);

