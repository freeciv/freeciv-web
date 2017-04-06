define(function () {

    return {
        installAll: function (gl) {
            if (gl.__hasHacksInstalled) {
              return;
            }
            gl.__hasHacksInstalled = true;
        },
    };

});
