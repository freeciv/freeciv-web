document.addEventListener('DOMContentLoaded', () => {
    const goodsListDiv = document.getElementById('goods-list');
    const orderBookDiv = document.getElementById('order-book');
    const userOrdersDiv = document.getElementById('user-orders');
    const orderForm = document.getElementById('create-order-form');
    const orderBookTitle = document.getElementById('order-book-title');
    const selectedGoodIdInput = document.getElementById('selected-good-id');

    let selectedGoodId = null;

    /**
     * Fetches the list of all tradable goods from the API.
     */
    async function loadGoods() {
        try {
            const response = await fetch('/freeciv-web/api/trading?action=getGoods');
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
            const goods = await response.json();
            renderGoods(goods);
        } catch (error) {
            console.error('Failed to load goods:', error);
            goodsListDiv.innerHTML = '<p>Error loading goods. Please try again later.</p>';
        }
    }

    /**
     * Renders the goods into the #goods-list container.
     * @param {Array} goods - The array of good objects from the API.
     */
    function renderGoods(goods) {
        if (!goods || goods.length === 0) {
            goodsListDiv.innerHTML = '<p>No goods available for trading.</p>';
            return;
        }
        goodsListDiv.innerHTML = ''; // Clear loading message
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
     * @param {HTMLElement} selectedDiv - The div element that was clicked.
     * @param {number} goodId - The ID of the selected good.
     * @param {string} goodName - The name of the selected good.
     */
    function handleGoodSelection(selectedDiv, goodId, goodName) {
        // Update selected state visuals
        const allGoods = document.querySelectorAll('.good-item');
        allGoods.forEach(div => div.classList.remove('selected'));
        selectedDiv.classList.add('selected');

        selectedGoodId = goodId;
        selectedGoodIdInput.value = goodId;
        orderBookTitle.textContent = `Order Book for ${goodName}`;

        // TODO: Load the order book for the selected good
        console.log(`Good selected: ${goodId}. Implement loadOrderBook(${goodId}).`);
        orderBookDiv.innerHTML = `<p>Loading order book for ${goodName}... (not implemented yet)</p>`;
    }

    // TODO: Implement function to load order book for a given goodId.
    // It should fetch from `/api/trading?action=getOrders&good_id=<goodId>`
    // and render two tables: one for buy orders and one for sell orders.
    // async function loadOrderBook(goodId) { ... }

    // TODO: Implement function to handle the order form submission.
    // It should send a POST request to `/api/trading` with form data.
    // On success, it should refresh the order book and user orders.
    // orderForm.addEventListener('submit', async (e) => { ... });

    // TODO: Implement function to load the current user's open orders.
    // It should fetch from `/api/trading?action=getUserOrders`.
    // This will require the user to be logged in.
    // async function loadUserOrders() { ... }

    // Initial data load when the page is ready.
    loadGoods();
});
