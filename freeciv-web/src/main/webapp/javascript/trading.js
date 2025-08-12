document.addEventListener('DOMContentLoaded', () => {
    const goodsListDiv = document.getElementById('goods-list');
    const orderBookDiv = document.getElementById('order-book');
    const userOrdersDiv = document.getElementById('user-orders');
    const orderForm = document.getElementById('create-order-form');
    const orderBookTitle = document.getElementById('order-book-title');
    const selectedGoodIdInput = document.getElementById('selected-good-id');

    let selectedGoodId = null;

    // --- Authentication ---
    // In a real app, these would be securely stored after login.
    // For now, we'll rely on the user being logged into the main site,
    // and assume the credentials can be retrieved.
    // We will use placeholders for now. The user will need to input them.
    let tempUsername = localStorage.getItem('freeciv_username') || '';
    let tempAuthToken = localStorage.getItem('freeciv_authToken') || '';

    function getAuthParams() {
        // Prompt for credentials if not available. This is for testing only.
        if (!tempUsername || !tempAuthToken) {
            tempUsername = prompt("Please enter your username:");
            tempAuthToken = prompt("Please enter your password/token:");
            localStorage.setItem('freeciv_username', tempUsername);
            localStorage.setItem('freeciv_authToken', tempAuthToken);
        }
        return `&username=${encodeURIComponent(tempUsername)}&authToken=${encodeURIComponent(tempAuthToken)}`;
    }

    /**
     * Fetches the list of all tradable goods from the API.
     */
    async function loadGoods() {
        try {
            const response = await fetch('/freeciv-web/api/trading?action=getGoods');
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            const goods = await response.json();
            renderGoods(goods);
        } catch (error) {
            console.error('Failed to load goods:', error);
            goodsListDiv.innerHTML = '<p>Error loading goods.</p>';
        }
    }

    /**
     * Renders the goods into the #goods-list container.
     */
    function renderGoods(goods) {
        if (!goods || goods.length === 0) {
            goodsListDiv.innerHTML = '<p>No goods available.</p>';
            return;
        }
        goodsListDiv.innerHTML = '';
        goods.forEach(good => {
            const goodDiv = document.createElement('div');
            goodDiv.className = 'good-item';
            goodDiv.textContent = good.name;
            goodDiv.dataset.goodId = good.id;
            goodDiv.addEventListener('click', () => handleGoodSelection(goodDiv, good.id, good.name));
            goodsListDiv.appendChild(goodDiv);
        });
    }

    /**
     * Handles the click event on a good item.
     */
    function handleGoodSelection(selectedDiv, goodId, goodName) {
        document.querySelectorAll('.good-item').forEach(div => div.classList.remove('selected'));
        selectedDiv.classList.add('selected');

        selectedGoodId = goodId;
        selectedGoodIdInput.value = goodId;
        orderBookTitle.textContent = `Order Book for ${goodName}`;

        loadOrderBook(goodId);
    }

    /**
     * Loads and displays the order book for a given good.
     */
    async function loadOrderBook(goodId) {
        orderBookDiv.innerHTML = `<p>Loading order book...</p>`;
        try {
            const response = await fetch(`/freeciv-web/api/trading?action=getOrders&good_id=${goodId}`);
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            const data = await response.json();
            renderOrderBook(data.buy_orders, data.sell_orders);
        } catch (error) {
            console.error(`Failed to load order book for good ${goodId}:`, error);
            orderBookDiv.innerHTML = `<p>Error loading order book.</p>`;
        }
    }

    /**
     * Renders the buy and sell orders into tables.
     */
    function renderOrderBook(buyOrders, sellOrders) {
        const buysHtml = `<h4>Buy Orders</h4><table><tr><th>Price</th><th>Quantity</th></tr>` +
            buyOrders.map(o => `<tr><td>${o.price.toFixed(2)}</td><td>${o.quantity}</td></tr>`).join('') + `</table>`;

        const sellsHtml = `<h4>Sell Orders</h4><table><tr><th>Price</th><th>Quantity</th></tr>` +
            sellOrders.map(o => `<tr><td>${o.price.toFixed(2)}</td><td>${o.quantity}</td></tr>`).join('') + `</table>`;

        orderBookDiv.innerHTML = buysHtml + sellsHtml;
    }

    /**
     * Loads and displays the current user's open orders.
     */
    async function loadUserOrders() {
        userOrdersDiv.innerHTML = '<p>Loading my orders...</p>';
        try {
            const authParams = getAuthParams();
            if (!tempUsername) { // User cancelled prompt
                 userOrdersDiv.innerHTML = '<p>Enter credentials to see your orders.</p>';
                 return;
            }
            const response = await fetch(`/freeciv-web/api/trading?action=getUserOrders${authParams}`);
            if (response.status === 401) {
                alert('Authentication failed. Please check your username/password and try again.');
                localStorage.removeItem('freeciv_username');
                localStorage.removeItem('freeciv_authToken');
                tempUsername = '';
                tempAuthToken = '';
                userOrdersDiv.innerHTML = '<p>Authentication failed.</p>';
                return;
            }
            if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
            const orders = await response.json();
            renderUserOrders(orders);
        } catch (error) {
            console.error('Failed to load user orders:', error);
            userOrdersDiv.innerHTML = '<p>Error loading your orders.</p>';
        }
    }

    /**
     * Renders the user's orders into a table.
     */
    function renderUserOrders(orders) {
        if (!orders || orders.length === 0) {
            userOrdersDiv.innerHTML = '<p>You have no open orders.</p>';
            return;
        }
        let html = `<table><tr><th>Good</th><th>Type</th><th>Quantity</th><th>Price</th><th>Date</th></tr>`;
        html += orders.map(o => `<tr><td>${o.good_name}</td><td>${o.type}</td><td>${o.quantity}</td><td>${o.price.toFixed(2)}</td><td>${new Date(o.created_at).toLocaleString()}</td></tr>`).join('');
        html += `</table>`;
        userOrdersDiv.innerHTML = html;
    }

    /**
     * Handles the submission of the create order form.
     */
    orderForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        if (!selectedGoodId) {
            alert('Please select a good before placing an order.');
            return;
        }

        const formData = new FormData(orderForm);
        const authParams = getAuthParams();
        if (!tempUsername) return; // User cancelled prompt

        const body = new URLSearchParams(formData);
        body.append('action', 'createOrder');
        body.append('username', tempUsername);
        body.append('authToken', tempAuthToken);

        try {
            const response = await fetch('/freeciv-web/api/trading', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: body.toString(),
            });
            const result = await response.json();
            if (result.success) {
                alert('Order placed successfully!');
                orderForm.reset();
                // Refresh data
                loadOrderBook(selectedGoodId);
                loadUserOrders();
            } else {
                alert(`Error: ${result.message}`);
            }
        } catch (error) {
            console.error('Failed to create order:', error);
            alert('An unexpected error occurred while placing the order.');
        }
    });

    // Initial data loads
    loadGoods();
    loadUserOrders();
});
