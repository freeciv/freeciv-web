create table hall_of_fame (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `username` varchar(32) NOT NULL,
  `nation` varchar(64) NOT NULL,
  `score` int(10),
  `end_turn` varchar(4),
  `end_date` DATE,
  `ip` varchar(16),
  PRIMARY KEY (`id`)
);

