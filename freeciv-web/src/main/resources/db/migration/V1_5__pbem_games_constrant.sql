-- maximum of one game result for two players in one day.
alter table game_results ADD CONSTRAINT game_results_unique UNIQUE (playerOne, playerTwo, endDate);
