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
    protected void doPost(HttpServletRequest req, HttpServletResponse resp) throws ServletException, IOException {
        handleRequest(req, resp);
    }

    private void handleRequest(HttpServletRequest request, HttpServletResponse response) throws IOException {
        String action = request.getParameter("action");

        if (action == null) {
            sendError(response, "Action parameter is missing.", HttpServletResponse.SC_BAD_REQUEST);
            return;
        }

        switch (action) {
            case "getGoods":
                getGoods(request, response);
                break;
            // TODO: Implement other actions
            // case "getOrders":
            //     getOrders(request, response);
            //     break;
            // case "createOrder":
            //     createOrder(request, response);
            //     break;
            // case "getUserOrders":
            //     getUserOrders(request, response);
            //     break;
            default:
                sendError(response, "Invalid action: " + action, HttpServletResponse.SC_BAD_REQUEST);
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
