/*******************************************************************************
 * Freeciv-web - the web version of Freeciv. https://www.freeciv.org/
 * Copyright (C) 2024 The Freeciv-web project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *******************************************************************************/
package org.freeciv.servlet;

import java.io.IOException;
import java.io.PrintWriter;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;

import javax.naming.Context;
import javax.naming.InitialContext;
import javax.sql.DataSource;

import jakarta.servlet.ServletException;
import jakarta.servlet.http.HttpServlet;
import jakarta.servlet.http.HttpServletRequest;
import jakarta.servlet.http.HttpServletResponse;

import org.apache.commons.codec.digest.DigestUtils;
import org.json.JSONArray;
import org.json.JSONObject;
import org.freeciv.util.Constants;

/**
 * Handles all API requests for the trading platform.
 * Actions are specified by the 'action' parameter.
 *
 * GET actions:
 *  - getGoods: Returns a list of all tradable goods.
 *  - getOrders: Returns buy/sell orders for a specific good. (Requires 'good_id')
 *  - getUserOrders: Returns all open orders for the current user.
 *
 * POST actions:
 *  - createOrder: Creates a new buy or sell order.
 */
public class TradingApiServlet extends HttpServlet {

    private static final long serialVersionUID = 1L;

    @Override
    protected void doGet(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        handleRequest(req, resp);
    }

    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
        String action = request.getParameter("action");
        if ("createOrder".equals(action)) {
            createOrder(request, response);
        } else {
            sendError(response, "Invalid POST action: " + action, HttpServletResponse.SC_BAD_REQUEST);
        }
    }

    private void handleRequest(HttpServletRequest request, HttpServletResponse response) throws IOException {
        // GET requests are now handled here
        String action = request.getParameter("action");

        if (action == null) {
            sendError(response, "Action parameter is missing for GET request.", HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        switch (action) {
            case "getGoods":
                getGoods(request, response);
                break;
            case "getOrders":
                getOrders(request, response);
                break;
            case "getUserOrders":
                getUserOrders(request, response);
                break;
            default:
                sendError(response, "Invalid GET action: " + action, HttpServletResponse.SC_BAD_REQUEST);
                break;
        }
    }

    private void getGoods(HttpServletRequest request, HttpServletResponse response) throws IOException {
        JSONArray goodsList = new JSONArray();
        String query = "SELECT id, name, description, image_url FROM Goods ORDER BY id";

        try (Connection conn = getDbConnection();
             PreparedStatement ps = conn.prepareStatement(query);
             ResultSet rs = ps.executeQuery()) {

            while (rs.next()) {
                JSONObject good = new JSONObject();
                good.put("id", rs.getInt("id"));
                good.put("name", rs.getString("name"));
                good.put("description", rs.getString("description"));
                good.put("image_url", rs.getString("image_url"));
                goodsList.put(good);
            }

            sendJsonResponse(response, goodsList.toString());

        } catch (Exception e) {
            e.printStackTrace();
            sendError(response, "Failed to retrieve goods from database.", HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
        }
    }

    private void getOrders(HttpServletRequest request, HttpServletResponse response) throws IOException {
        String goodIdStr = request.getParameter("good_id");
        if (goodIdStr == null) {
            sendError(response, "good_id parameter is missing.", HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        try {
            int goodId = Integer.parseInt(goodIdStr);
            JSONObject orderBook = new JSONObject();
            JSONArray buyOrders = new JSONArray();
            JSONArray sellOrders = new JSONArray();

            String buyQuery = "SELECT quantity, price FROM Orders WHERE good_id = ? AND type = 'BUY' AND status = 'OPEN' ORDER BY price DESC";
            String sellQuery = "SELECT quantity, price FROM Orders WHERE good_id = ? AND type = 'SELL' AND status = 'OPEN' ORDER BY price ASC";

            try (Connection conn = getDbConnection()) {
                // Get BUY orders
                try (PreparedStatement ps = conn.prepareStatement(buyQuery)) {
                    ps.setInt(1, goodId);
                    ResultSet rs = ps.executeQuery();
                    while (rs.next()) {
                        JSONObject order = new JSONObject();
                        order.put("quantity", rs.getInt("quantity"));
                        order.put("price", rs.getDouble("price"));
                        buyOrders.put(order);
                    }
                }
                // Get SELL orders
                try (PreparedStatement ps = conn.prepareStatement(sellQuery)) {
                    ps.setInt(1, goodId);
                    ResultSet rs = ps.executeQuery();
                    while (rs.next()) {
                        JSONObject order = new JSONObject();
                        order.put("quantity", rs.getInt("quantity"));
                        order.put("price", rs.getDouble("price"));
                        sellOrders.put(order);
                    }
                }
            }

            orderBook.put("buy_orders", buyOrders);
            orderBook.put("sell_orders", sellOrders);
            sendJsonResponse(response, orderBook.toString());

        } catch (NumberFormatException e) {
            sendError(response, "Invalid good_id parameter.", HttpServletResponse.SC_BAD_REQUEST);
        } catch (Exception e) {
            e.printStackTrace();
            sendError(response, "Failed to retrieve order book from database.", HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
        }
    }

    private void createOrder(HttpServletRequest request, HttpServletResponse response) throws IOException {
        try {
            // Authenticate user
            String username = request.getParameter("username");
            String authToken = request.getParameter("authToken");
            int userId = getAuthenticatedUserId(username, authToken);

            if (userId == -1) {
                sendError(response, "Authentication failed.", HttpServletResponse.SC_UNAUTHORIZED);
                return;
            }

            // Parse parameters
            int goodId = Integer.parseInt(request.getParameter("good_id"));
            String type = request.getParameter("type");
            int quantity = Integer.parseInt(request.getParameter("quantity"));
            double price = Double.parseDouble(request.getParameter("price"));

            // Call the service
            TradingService tradingService = new TradingService();
            JSONObject result = tradingService.createOrderAndAttemptMatch(userId, goodId, type, quantity, price);

            sendJsonResponse(response, result.toString());

        } catch (NumberFormatException e) {
            sendError(response, "Invalid parameter format.", HttpServletResponse.SC_BAD_REQUEST);
        } catch (Exception e) {
            e.printStackTrace();
            sendError(response, "Failed to create order.", HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
        }
    }

    private void getUserOrders(HttpServletRequest request, HttpServletResponse response) throws IOException {
        try {
            // Authenticate user
            String username = request.getParameter("username");
            String authToken = request.getParameter("authToken");
            int userId = getAuthenticatedUserId(username, authToken);

            if (userId == -1) {
                sendError(response, "Authentication failed.", HttpServletResponse.SC_UNAUTHORIZED);
                return;
            }

            JSONArray ordersList = new JSONArray();
            String query = "SELECT o.id, g.name as good_name, o.type, o.quantity, o.price, o.created_at " +
                           "FROM Orders o JOIN Goods g ON o.good_id = g.id " +
                           "WHERE o.user_id = ? AND o.status = 'OPEN' ORDER BY o.created_at DESC";

            try (Connection conn = getDbConnection();
                 PreparedStatement ps = conn.prepareStatement(query)) {
                ps.setInt(1, userId);
                ResultSet rs = ps.executeQuery();
                while (rs.next()) {
                    JSONObject order = new JSONObject();
                    order.put("id", rs.getInt("id"));
                    order.put("good_name", rs.getString("good_name"));
                    order.put("type", rs.getString("type"));
                    order.put("quantity", rs.getInt("quantity"));
                    order.put("price", rs.getDouble("price"));
                    order.put("created_at", rs.getTimestamp("created_at").toString());
                    ordersList.put(order);
                }
            }
            sendJsonResponse(response, ordersList.toString());

        } catch (Exception e) {
            e.printStackTrace();
            sendError(response, "Failed to get user orders.", HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
        }
    }


    private int getAuthenticatedUserId(String username, String authToken) throws Exception {
        if (username == null || authToken == null) {
            return -1;
        }
        String hashedPassword = DigestUtils.sha256Hex(authToken);
        String query = "SELECT id FROM auth WHERE LOWER(username) = LOWER(?) AND secure_hashed_password = ? AND activated = '1'";
        try (Connection conn = getDbConnection();
             PreparedStatement ps = conn.prepareStatement(query)) {
            ps.setString(1, username);
            ps.setString(2, hashedPassword);
            ResultSet rs = ps.executeQuery();
            if (rs.next()) {
                return rs.getInt("id");
            }
        }
        return -1; // Auth failed
    }


    /* --- Helper Methods --- */

    private Connection getDbConnection() throws Exception {
        Context env = (Context) new InitialContext().lookup(Constants.JNDI_CONNECTION);
        DataSource ds = (DataSource) env.lookup(Constants.JNDI_DDBBCON_MYSQL);
        return ds.getConnection();
    }

    private void sendJsonResponse(HttpServletResponse response, String json) throws IOException {
        response.setContentType("application/json");
        response.setCharacterEncoding("UTF-8");
        PrintWriter out = response.getWriter();
        out.print(json);
        out.flush();
    }

    private void sendError(HttpServletResponse response, String message, int statusCode) throws IOException {
        response.sendError(statusCode, message);
    }
}
