drop table turncount;
create table time_played_stats (statsDate DATE, timePlayed int(32), 
PRIMARY KEY (statsDate));
create table games_played_stats (statsDate DATE, gameType int(2), gameCount int(32), 
PRIMARY KEY (statsDate, gameType));
