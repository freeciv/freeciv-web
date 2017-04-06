define([
        '../shared/EventSource',
        './ResourceVersion'
    ], function (
        EventSource,
        ResourceVersion
    ) {

    // Incrmeents with each resource allocated
    var uniqueId = 0;

    var Resource = function (gl, frameNumber, stack, target) {
        this.id = uniqueId++;
        this.status = Resource.ALIVE;

        this.defaultName = "res " + this.id;

        this.target = target;
        target.trackedObject = this;

        this.mirror = {
            gl: null,
            target: null,
            version: null
        };
        this.mirrorSets = {};
        this.mirrorSets["default"] = this.mirror;

        this.creationFrameNumber = frameNumber;
        this.creationStack = stack;
        this.deletionStack = null;

        // previousVersion is the previous version that was captured
        // currentVersion is the version as it is at the current point in time
        this.previousVersion = null;
        this.currentVersion = new ResourceVersion();
        this.versionNumber = 0;
        this.dirty = true;

        this.modified = new EventSource("modified");
        this.deleted = new EventSource("deleted");
    };

    Resource.ALIVE = 0;
    Resource.DEAD = 1;

    Resource.prototype.getName = function () {
        if (this.target.displayName) {
            return this.target.displayName;
        } else {
            return this.defaultName;
        }
    };

    Resource.prototype.setName = function (name, ifNeeded) {
        if (!ifNeeded && this.target.displayName && this.target.displayName !== name) {
            this.target.displayName = name;
            this.modified.fireDeferred(this);
        }
    };

    Resource.prototype.captureVersion = function () {
        this.dirty = false;
        return this.currentVersion;
    };

    Resource.prototype.markDirty = function (reset) {
        if (!this.dirty) {
            this.previousVersion = this.currentVersion;
            this.currentVersion = reset ? new ResourceVersion() : this.previousVersion.clone();
            this.versionNumber++;
            this.currentVersion.versionNumber = this.versionNumber;
            this.dirty = true;
            this.cachedPreview = null; // clear a preview if we have one
            this.modified.fireDeferred(this);
        } else {
            if (reset) {
                this.currentVersion = new ResourceVersion();
            }
            this.modified.fireDeferred(this);
        }
    };

    Resource.prototype.markDeleted = function (stack) {
        this.status = Resource.DEAD;
        this.deletionStack = stack;

        // TODO: hang on to object?
        //this.target = null;

        this.deleted.fireDeferred(this);
    };

    Resource.prototype.restoreVersion = function (gl, version, force, options) {
        if (force || (this.mirror.version != version)) {
            this.disposeMirror();

            this.mirror.gl = gl;
            this.mirror.version = version;
            this.mirror.target = this.createTarget(gl, version, options);
            this.mirror.target.trackedObject = this;
        } else {
            // Already at the current version
        }
    };

    Resource.prototype.switchMirror = function (setName) {
        setName = setName || "default";
        var oldMirror = this.mirror;
        var newMirror = this.mirrorSets[setName];
        if (oldMirror == newMirror) {
            return;
        }
        if (!newMirror) {
            newMirror = {
                gl: null,
                target: null,
                version: null
            };
            this.mirrorSets[setName] = newMirror;
        }
        this.mirror = newMirror;
    };

    Resource.prototype.disposeMirror = function () {
        if (this.mirror.target) {
            this.deleteTarget(this.mirror.gl, this.mirror.target);
            this.mirror.gl = null;
            this.mirror.target = null;
            this.mirror.version = null;
        }
    };

    Resource.prototype.disposeAllMirrors = function () {
        for (var setName in this.mirrorSets) {
            var mirror = this.mirrorSets[setName];
            if (mirror && mirror.target) {
                this.deleteTarget(mirror.gl, mirror.target);
                mirror.gl = null;
                mirror.target = null;
                mirror.version = null;
            }
        }
    };

    Resource.prototype.createTarget = function (gl, version, options) {
        console.log("unimplemented createTarget");
        return null;
    };

    Resource.prototype.deleteTarget = function (gl, target) {
        console.log("unimplemented deleteTarget");
    };

    Resource.prototype.replayCalls = function (gl, version, target, filter) {
        for (var n = 0; n < version.calls.length; n++) {
            var call = version.calls[n];

            var args = [];
            for (var m = 0; m < call.args.length; m++) {
                // TODO: unpack refs?
                args[m] = call.args[m];
                if (args[m] == this) {
                    args[m] = target;
                } else if (args[m] && args[m].mirror) {
                    args[m] = args[m].mirror.target;
                }
            }

            if (filter) {
                if (filter(call, args) == false) {
                    continue;
                }
            }

            gl[call.name].apply(gl, args);
        }
    }

    return Resource;

});

