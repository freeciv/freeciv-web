alter table players  add column flag VARCHAR(128);


alter table auth add column `openid_user` varchar(512) default NULL;



alter table auth add column `fb_uid` int(11) default NULL;
alter table auth add column `email_hash` varchar(64) default NULL;

alter table auth drop index name;

alter table auth modify fb_uid bigint;

ALTER TABLE auth ADD UNIQUE (username);
ALTER TABLE auth ADD UNIQUE (email);

CREATE INDEX id_fb_uid ON auth(fb_uid);
CREATE INDEX id_email_hask ON auth(email_hash);

DROP INDEX id_email_hask ON auth;


DROP INDEX email ON auth;


CREATE TABLE js_breakpad (
	id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
	msg VARCHAR(1000),
	url VARCHAR(100), 
	linenumber INT,
	stacktrace VARCHAR(4000),
	timepoint TIMESTAMP, 
	ip VARCHAR(20),
	useragent VARCHAR(100) 
       );


ALTER TABLE js_breakpad MODIFY useragent varchar(200);

CREATE TABLE player_game_list (
	username VARCHAR(32) NOT NULL PRIMARY KEY,
	host varchar(249), 
	port int(11),
	timepoint TIMESTAMP
       );

