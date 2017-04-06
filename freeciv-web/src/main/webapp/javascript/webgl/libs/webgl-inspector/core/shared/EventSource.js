define(function () {

    var setTimeout = window.setTimeout.bind(window);

    var EventSource = function (name) {
        this.name = name;
        this.listeners = [];
    };

    EventSource.prototype.addListener = function (target, callback) {
        this.listeners.push({
            target: target,
            callback: callback
        });
    };

    EventSource.prototype.removeListener = function (target, callback) {
        for (var n = 0; n < this.listeners.length; n++) {
            var listener = this.listeners[n];
            if (listener.target === target) {
                if (callback) {
                    if (listener.callback === callback) {
                        this.listeners.splice(n, 1);
                        break;
                    }
                } else {
                    this.listeners.splice(n, 1);
                }
            }
        }
    };

    EventSource.prototype.fire = function () {
        for (var n = 0; n < this.listeners.length; n++) {
            var listener = this.listeners[n];
            //try {
                listener.callback.apply(listener.target, arguments);
            //} catch (e) {
            //    console.log("exception thrown in target of event " + this.name + ": " + e);
            //}
        }
    };

    EventSource.prototype.fireDeferred = function () {
        var self = this;
        var args = arguments;
        setTimeout(function () {
            self.fire.apply(self, args);
        }, 0);
    };

    EventSource.setSetTimeoutFn = function(fn) {
        setTimeout = fn;
    };

    return EventSource;
});
