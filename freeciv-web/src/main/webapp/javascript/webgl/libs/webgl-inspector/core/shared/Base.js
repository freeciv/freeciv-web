define(function() {

    function subclass(parent, child, args) {
        parent.apply(child, args);

        // TODO: this sucks - do it right

        for (var propertyName in parent.prototype) {
            if (propertyName == "constructor") {
                continue;
            }
            if (!child.__proto__[propertyName]) {
                child.__proto__[propertyName] = parent.prototype[propertyName];
            }
        }

        for (var propertyName in parent) {
            child[propertyName] = parent[propertyName];
        }
    };

    function typename(value) {
        function stripConstructor(value) {
            if (value) {
                return value.replace("Constructor", "");
            } else {
                return value;
            }
        };
        if (value) {
            var mangled = value.constructor.toString();
            if (mangled) {
                var matches = mangled.match(/function (.+)\(/);
                if (matches) {
                    // ...function Foo()...
                    if (matches[1] == "Object") {
                        // Hrm that's likely not right...
                        // constructor may be fubar
                        mangled = value.toString();
                    } else {
                        return stripConstructor(matches[1]);
                    }
                }

                // [object Foo]
                matches = mangled.match(/\[object (.+)\]/);
                if (matches) {
                    return stripConstructor(matches[1]);
                }
            }
        }
        return null;
    };

    function scrollIntoViewIfNeeded(el) {
        if (el.scrollIntoViewIfNeeded) {
            el.scrollIntoViewIfNeeded();
        } else if (el.offsetParent) {
            // TODO: determine if el is in the current view of the parent
            var scrollTop = el.offsetParent.scrollTop;
            var scrollBottom = el.offsetParent.scrollTop + el.offsetParent.clientHeight;
            var elTop = el.offsetTop;
            var elBottom = el.offsetTop + el.offsetHeight;
            if ((elTop < scrollTop) || (elTop > scrollBottom)) {
                el.scrollIntoView();
            }
        }
    };

    return {
        subclass: subclass,
        typename: typename,
        scrollIntoViewIfNeeded: scrollIntoViewIfNeeded,
    };

});
