// Called when a message is passed.  We assume that the content script wants to show the page action.
function onRequest(request, sender, sendResponse) {
    if (request.present) {
        // Show the page action for the tab that the sender (content script) was on.
        chrome.pageAction.show(sender.tab.id);
    }

    // Return nothing to let the connection be cleaned up.
    sendResponse({});
};

// Listen for the content script to send a message to the background page.
chrome.extension.onRequest.addListener(onRequest);

chrome.pageAction.onClicked.addListener(function (tab) {
    chrome.tabs.sendRequest(tab.id, {
        reload: true
    }, function (response) {
    });
});
