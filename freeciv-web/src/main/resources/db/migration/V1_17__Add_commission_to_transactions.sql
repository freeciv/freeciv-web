--
-- Add a commission column to the Transactions table to record fees.
--

ALTER TABLE `Transactions`
ADD COLUMN `commission` DECIMAL(15, 2) NOT NULL DEFAULT 0.00 COMMENT 'Commission fee taken from the transaction';
