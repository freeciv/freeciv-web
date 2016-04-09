create table game_results (
  id int(11) NOT NULL AUTO_INCREMENT, 
  playerOne varchar(32), 
  playerTwo varchar(32), 
  winner varchar(32), 
  endDate DATE, 
PRIMARY KEY (`id`));
