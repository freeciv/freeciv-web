--
-- Create a table for global, adjustable settings for the application.
--

CREATE TABLE `Global_Settings` (
    `setting_key` VARCHAR(255) PRIMARY KEY,
    `setting_value` VARCHAR(255) NOT NULL,
    `description` TEXT,
    `updated_at` TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);

-- Insert the initial commission rate as requested (15%)
INSERT INTO `Global_Settings` (`setting_key`, `setting_value`, `description`)
VALUES ('commission_rate', '0.15', 'The commission rate for all trades in the marketplace (e.g., 0.15 means 15%).');
