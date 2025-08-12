--
-- Schema and initial data for the Trading Platform feature
--

-- Add account balance to the users table (auth)
ALTER TABLE `auth` ADD COLUMN `account_balance` DECIMAL(15, 2) NOT NULL DEFAULT 0.00;

-- Create the Goods table for tradable items
CREATE TABLE `Goods` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `name` VARCHAR(255) NOT NULL UNIQUE,
    `description` TEXT,
    `image_url` VARCHAR(255)
);

-- Create the Orders table for buy and sell orders
CREATE TABLE `Orders` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `user_id` INT NOT NULL,
    `good_id` INT NOT NULL,
    `type` ENUM('BUY', 'SELL') NOT NULL,
    `quantity` INT NOT NULL,
    `price` DECIMAL(15, 2) NOT NULL,
    `status` ENUM('OPEN', 'CLOSED', 'CANCELLED') NOT NULL DEFAULT 'OPEN',
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (`user_id`) REFERENCES `auth`(`id`) ON DELETE CASCADE,
    FOREIGN KEY (`good_id`) REFERENCES `Goods`(`id`) ON DELETE CASCADE
);

-- Create the Transactions table to record completed trades
CREATE TABLE `Transactions` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `buy_order_id` INT NOT NULL,
    `sell_order_id` INT NOT NULL,
    `quantity` INT NOT NULL,
    `price` DECIMAL(15, 2) NOT NULL,
    `created_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (`buy_order_id`) REFERENCES `Orders`(`id`),
    FOREIGN KEY (`sell_order_id`) REFERENCES `Orders`(`id`)
);

-- Insert some initial goods for the marketplace to be usable
INSERT INTO `Goods` (`name`, `description`, `image_url`) VALUES
('Gold', 'The primary currency for trade and diplomacy.', 'images/icons/gold.png'),
('Iron', 'A fundamental resource for building advanced units and wonders.', 'images/icons/iron.png'),
('Wood', 'A basic resource used for construction and early units.', 'images/icons/wood.png'),
('Research', 'Represents scientific knowledge, driving technological advancement.', 'images/icons/research.png');
