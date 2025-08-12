<%@ include file="/WEB-INF/jsp/fragments/i18n.jsp"%>
<!DOCTYPE html>
<html lang="en">
<head>
	<%@include file="/WEB-INF/jsp/fragments/head.jsp"%>
    <title>Marketplace</title>
    <style>
        /* Basic styling for the market page */
        .market-container {
            display: flex;
            flex-wrap: wrap;
            gap: 20px;
            padding: 20px;
        }
        .goods-section, .order-book-section, .user-orders-section, .trade-form-section {
            border: 1px solid #ccc;
            padding: 15px;
            border-radius: 8px;
            background-color: #f9f9f9;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .goods-section { flex-basis: 100%; }
        .order-book-section { flex: 1; min-width: 300px; }
        .trade-form-section { flex: 1; min-width: 300px; }
        .user-orders-section { flex-basis: 100%; margin-top: 20px; }

        #goods-list { display: flex; gap: 15px; flex-wrap: wrap; }
        .good-item { cursor: pointer; padding: 10px 15px; border: 1px solid #ddd; border-radius: 5px; transition: all 0.2s; }
        .good-item:hover { background-color: #f0f0f0; }
        .good-item.selected { background-color: #d4edda; border-color: #c3e6cb; }

        #order-book table, #user-orders table { width: 100%; border-collapse: collapse; }
        #order-book th, #order-book td, #user-orders th, #user-orders td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }

        .order-form form { display: flex; flex-direction: column; gap: 10px; }
        .order-form label { font-weight: bold; }
        .order-form input, .order-form select, .order-form button { padding: 8px; border-radius: 4px; border: 1px solid #ccc; }
        .order-form button { background-color: #28a745; color: white; cursor: pointer; }
        .order-form button:hover { background-color: #218838; }

    </style>
</head>
<body>
	<div class="container">
		<%@include file="/WEB-INF/jsp/fragments/header.jsp"%>

		<div class="market-container">
            <div class="goods-section">
                <h2>Marketplace</h2>
                <p>Welcome to the Freeciv Marketplace. Select a good to see the order book and place an order.</p>
                <div id="goods-list">
                    <!-- Goods will be loaded here by JavaScript -->
                    <p>Loading goods...</p>
                </div>
            </div>

            <div class="order-book-section">
                <h3 id="order-book-title">Order Book</h3>
                <div id="order-book">
                    <p>Select a good to view its order book.</p>
                </div>
            </div>

            <div class="trade-form-section">
                <h3>Place an Order</h3>
                <div class="order-form">
                    <form id="create-order-form">
                        <input type="hidden" id="selected-good-id" name="good_id">
                        <div>
                            <label for="order-type">Type:</label>
                            <select id="order-type" name="type">
                                <option value="BUY">Buy</option>
                                <option value="SELL">Sell</option>
                            </select>
                        </div>
                        <div>
                            <label for="order-quantity">Quantity:</label>
                            <input type="number" id="order-quantity" name="quantity" min="1" required>
                        </div>
                        <div>
                            <label for="order-price">Price:</label>
                            <input type="number" id="order-price" name="price" step="0.01" min="0.01" required>
                        </div>
                        <button type="submit">Submit Order</button>
                    </form>
                </div>
            </div>

            <div class="user-orders-section">
                <h3>My Open Orders</h3>
                <div id="user-orders">
                    <!-- User's open orders will be loaded here -->
                </div>
            </div>
        </div>

		<%@include file="/WEB-INF/jsp/fragments/footer.jsp"%>
	</div>
    <script src="${pageContext.request.contextPath}/javascript/trading.js"></script>
</body>
</html>
