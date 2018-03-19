CREATE TABLE auth (
  id int IDENTITY(1,1) PRIMARY KEY,
  username varchar(32) UNIQUE DEFAULT NULL,
  email varchar(128) UNIQUE DEFAULT NULL,
  activated bit DEFAULT '1',
  secure_hashed_password varchar(128) DEFAULT NULL,
  ip varchar(16) DEFAULT NULL,
);

CREATE TABLE game_results (
  id int IDENTITY(1,1) PRIMARY KEY,
  playerOne varchar(32) DEFAULT NULL,
  playerTwo varchar(32) DEFAULT NULL,
  winner varchar(32) DEFAULT NULL,
  endDate date DEFAULT NULL,
  CONSTRAINT game_results_unique UNIQUE NONCLUSTERED (playerOne,playerTwo,endDate)
);

CREATE TABLE games_played_stats (
  statsDate date NOT NULL,
  gameType tinyint NOT NULL,
  gameCount int DEFAULT NULL,
  CONSTRAINT games_played_stats_pkey PRIMARY KEY CLUSTERED (statsDate,gameType)
);

CREATE TABLE google_auth (
  id int IDENTITY(1,1) PRIMARY KEY,
  username varchar(32) UNIQUE,
  subject varchar(128) UNIQUE,
  email varchar(128) UNIQUE,
  activated bit DEFAULT '1',
  ip varchar(16) DEFAULT NULL,
);

CREATE TABLE hall_of_fame (
  id int IDENTITY(1,1) PRIMARY KEY,
  username varchar(32) NOT NULL,
  nation varchar(64) NOT NULL,
  score smallint DEFAULT NULL,
  end_turn varchar(4) DEFAULT NULL,
  end_date date DEFAULT NULL,
  ip varchar(16) DEFAULT NULL,
);

CREATE TABLE players (
  hostport varchar(255) NOT NULL DEFAULT 'unknown:5556',
  name varchar(64) NOT NULL DEFAULT 'name',
  [user] varchar(64) DEFAULT NULL,
  nation varchar(64) DEFAULT NULL,
  type varchar(10) DEFAULT NULL,
  host varchar(255) DEFAULT 'unknown',
  flag varchar(128) DEFAULT NULL,
  CONSTRAINT players_pkey PRIMARY KEY (hostport,name)
);

CREATE TABLE servers (
  host varchar(249) NOT NULL DEFAULT 'unknown',
  port int NOT NULL DEFAULT '5556',
  version varchar(64) DEFAULT 'unknown',
  patches varchar(255) DEFAULT 'none',
  capability varchar(255) DEFAULT NULL,
  state varchar(20) DEFAULT 'Pregame',
  ruleset varchar(255) DEFAULT 'Unknown',
  topic varchar(255) DEFAULT 'none',
  type varchar(255) DEFAULT 'none',
  humans varchar(64) DEFAULT '0',
  message varchar(255) DEFAULT 'none',
  stamp datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  available int DEFAULT '0',
  serverid varchar(255) DEFAULT NULL,
  CONSTRAINT servers_pkey PRIMARY KEY (host,port)
);

CREATE TABLE time_played_stats (
  statsDate date PRIMARY KEY,
  timePlayed int DEFAULT NULL,
);

CREATE TABLE variables (
  hostport varchar(64) NOT NULL DEFAULT '',
  name varchar(64) NOT NULL DEFAULT '',
  value varchar(64) DEFAULT NULL,
  CONSTRAINT variables_pkey PRIMARY KEY (hostport,name)
);
