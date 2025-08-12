--
-- Schema and initial data for the User Inventories feature
--

-- Create the User_Inventories table to track player assets.
-- This table will store how much of each good a user owns.
CREATE TABLE `User_Inventories` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `user_id` INT NOT NULL,
    `good_id` INT NOT NULL,
    `quantity` INT NOT NULL DEFAULT 0,
    FOREIGN KEY (`user_id`) REFERENCES `auth`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`good_id`) REFERENCES `Goods`(`id`) ON DELETE CASCADE,
    UNIQUE KEY `user_good_unique` (`user_id`, `good_id`)
);

-- Grant some initial resources to a user for testing purposes.
-- This assumes a user with id=1 exists (typically the first registered user or an admin)
-- and that goods with ids 1, 2, 3 have been created by the V1_15 migration.
INSERT INTO `User_Inventories` (`user_id`, `good_id`, `quantity`) VALUES
(1, 1, 1000), -- 1000 Gold
(1, 2, 50),   -- 50 Iron
(1, 3, 100);  -- 100 Wood
