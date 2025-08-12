/*******************************************************************************
 * Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
 * Copyright (C) 2024 The Freeciv-web project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *******************************************************************************/
package org.freeciv.services;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.sql.DataSource;

import org.freeciv.util.Constants;
import org.json.JSONObject;

/**
 * Provides business logic for the trading platform, including
 * order creation and matching.
 */
public class TradingService {

    /**
     * Creates a new order and immediately tries to match it with existing orders.
     * All database operations are performed in a single transaction.
     *
     * @param userId The ID of the user placing the order.
     * @param goodId The ID of the good being traded.
     * @param type The type of order ('BUY' or 'SELL').
     * @param quantity The amount of the good to trade.
     * @param price The price per unit.
     * @return A JSONObject indicating success or failure.
     */
    public JSONObject createOrderAndAttemptMatch(int userId, int goodId, String type, int quantity, double price) {
        JSONObject result = new JSONObject();
        Connection conn = null;

        try {
            conn = getDbConnection();
            conn.setAutoCommit(false); // Start transaction

            // Step 1: Validate the order
            if ("BUY".equals(type)) {
                if (!hasSufficientFunds(conn, userId, quantity * price)) {
                    result.put("success", false).put("message", "Insufficient funds.");
                    conn.rollback();
                    return result;
                }
            } else if ("SELL".equals(type)) {
                if (!hasSufficientInventory(conn, userId, goodId, quantity)) {
                    result.put("success", false).put("message", "Insufficient inventory.");
                    conn.rollback();
                    return result;
                }
            } else {
                result.put("success", false).put("message", "Invalid order type.");
                conn.rollback();
                return result;
            }

            // Step 2: Insert the new order and get its ID
            long newOrderId = -1;
            String sql = "INSERT INTO Orders (user_id, good_id, type, quantity, price) VALUES (?, ?, ?, ?, ?)";
            try (PreparedStatement ps = conn.prepareStatement(sql, Statement.RETURN_GENERATED_KEYS)) {
                ps.setInt(1, userId);
                ps.setInt(2, goodId);
                ps.setString(3, type);
                ps.setInt(4, quantity);
                ps.setDouble(5, price);
                ps.executeUpdate();
                ResultSet generatedKeys = ps.getGeneratedKeys();
                if (generatedKeys.next()) {
                    newOrderId = generatedKeys.getLong(1);
                } else {
                    throw new SQLException("Creating order failed, no ID obtained.");
                }
            }

            // Step 3: Attempt to match the order
            // This is still a simplified engine that processes one match. A loop would be needed for full matching.
            String matchSql;
            if ("BUY".equals(type)) {
                matchSql = "SELECT * FROM Orders WHERE good_id = ? AND type = 'SELL' AND status = 'OPEN' AND price <= ? ORDER BY price ASC, created_at ASC LIMIT 1 FOR UPDATE";
            } else { // SELL
                matchSql = "SELECT * FROM Orders WHERE good_id = ? AND type = 'BUY' AND status = 'OPEN' AND price >= ? ORDER BY price DESC, created_at ASC LIMIT 1 FOR UPDATE";
            }

            try (PreparedStatement ps = conn.prepareStatement(matchSql)) {
                ps.setInt(1, goodId);
                ps.setDouble(2, price);

                ResultSet rs = ps.executeQuery();
                if (rs.next()) {
                    // Match found!
                    int matchedOrderId = rs.getInt("id");
                    int matchedOrderUserId = rs.getInt("user_id");
                    int matchedOrderQuantity = rs.getInt("quantity");
                    double matchedOrderPrice = rs.getDouble("price");

                    // For simplicity, we'll assume a full match. Partial fills are more complex.
                    int tradeQuantity = Math.min(quantity, matchedOrderQuantity);
                    double tradePrice = matchedOrderPrice; // The price of the existing order on the book is used

                    int buyerId, sellerId, buyOrderId, sellOrderId;
                    if ("BUY".equals(type)) {
                        buyerId = userId;
                        sellerId = matchedOrderUserId;
                        buyOrderId = (int) newOrderId;
                        sellOrderId = matchedOrderId;
                    } else {
                        buyerId = matchedOrderUserId;
                        sellerId = userId;
                        buyOrderId = matchedOrderId;
                        sellOrderId = (int) newOrderId;
                    }

                    // Perform the transaction
                    updateBalance(conn, buyerId, -(tradeQuantity * tradePrice));
                    updateBalance(conn, sellerId, (tradeQuantity * tradePrice));
                    updateUserInventory(conn, sellerId, goodId, -tradeQuantity);
                    updateUserInventory(conn, buyerId, goodId, tradeQuantity);

                    // TODO: Handle partial fills properly. For now, assume full fills and close both orders.
                    updateOrderStatus(conn, matchedOrderId, "CLOSED");
                    updateOrderStatus(conn, (int) newOrderId, "CLOSED");
                    createTransaction(conn, buyOrderId, sellOrderId, tradeQuantity, tradePrice);
                }
            }

            conn.commit(); // Commit transaction
            result.put("success", true).put("message", "Order created successfully.");

        } catch (Exception e) {
            e.printStackTrace();
            if (conn != null) { try { conn.rollback(); } catch (SQLException ex) { ex.printStackTrace(); } }
            result.put("success", false).put("message", "An error occurred: " + e.getMessage());
        } finally {
            if (conn != null) { try { conn.close(); } catch (SQLException e) { e.printStackTrace(); } }
        }
        return result;
    }

    private boolean hasSufficientInventory(Connection conn, int userId, int goodId, int quantity) throws SQLException {
        String sql = "SELECT quantity FROM User_Inventories WHERE user_id = ? AND good_id = ?";
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setInt(1, userId);
            ps.setInt(2, goodId);
            ResultSet rs = ps.executeQuery();
            if (rs.next()) {
                return rs.getInt("quantity") >= quantity;
            }
        }
        return false; // No inventory record means they have 0
    }

    private void updateUserInventory(Connection conn, int userId, int goodId, int quantityDelta) throws SQLException {
        // This query will insert a new record if it doesn't exist, or update the existing one.
        String sql = "INSERT INTO User_Inventories (user_id, good_id, quantity) VALUES (?, ?, ?) " +
                     "ON DUPLICATE KEY UPDATE quantity = quantity + ?";
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setInt(1, userId);
            ps.setInt(2, goodId);
            ps.setInt(3, quantityDelta);
            ps.setInt(4, quantityDelta); // For the UPDATE part of ON DUPLICATE KEY
            ps.executeUpdate();
        }
    }

    private void createTransaction(Connection conn, int buyOrderId, int sellOrderId, int quantity, double price) throws SQLException {
        String sql = "INSERT INTO Transactions (buy_order_id, sell_order_id, quantity, price) VALUES (?, ?, ?, ?)";
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setInt(1, buyOrderId);
            ps.setInt(2, sellOrderId);
            ps.setInt(3, quantity);
            ps.setDouble(4, price);
            ps.executeUpdate();
        }
    }

    private boolean hasSufficientFunds(Connection conn, int userId, double amount) throws SQLException {
        String sql = "SELECT account_balance FROM auth WHERE id = ?";
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setInt(1, userId);
            ResultSet rs = ps.executeQuery();
            if (rs.next()) {
                return rs.getDouble("account_balance") >= amount;
            }
        }
        return false;
    }

    private void updateBalance(Connection conn, int userId, double amountDelta) throws SQLException {
        String sql = "UPDATE auth SET account_balance = account_balance + ? WHERE id = ?";
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setDouble(1, amountDelta);
            ps.setInt(2, userId);
            ps.executeUpdate();
        }
    }

    private void updateOrderStatus(Connection conn, int orderId, String status) throws SQLException {
        String sql = "UPDATE Orders SET status = ? WHERE id = ?";
        try (PreparedStatement ps = conn.prepareStatement(sql)) {
            ps.setString(1, status);
            ps.setInt(2, orderId);
            ps.executeUpdate();
        }
    }

    private Connection getDbConnection() throws Exception {
        Context env = (Context) new InitialContext().lookup(Constants.JNDI_CONNECTION);
        DataSource ds = (DataSource) env.lookup(Constants.JNDI_DDBBCON_MYSQL);
        return ds.getConnection();
    }
}
