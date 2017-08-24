/**********************************************************************
    Copyright (C) 2017  The Freeciv-web project

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************/

/**********************************************************************
 * Aggregates events to call their handler at a reduced frequency.
 *
 * handler: The event handler to call, may receive a parameter depending
 *          on dataPolicy.
 * timeout: First timeout (ms) from event reception to handler call.
 *          Default value: 1000.
 * dataPolicy: What to call the handler with. Default: DP_NONE.
 * latency: How long to wait (ms) after calling the handler before
 *          the next call. Default: timeout.
 * maxDelays: How many times to delay calling the handler while in
 *          "burst mode". Default: 0 (do not delay).
 * delayTimeout: How long to delay (ms) calling the handler if more
 *          events are received in a waiting period. Default: timeout/2.
 *
 * The events are reported to the aggregator with the update method.
 * The handler is called with different data depending on dataPolicy:
 * - DP_NONE : nothing.
 * - DP_FIRST: first event received since last call.
 * - DP_LAST : last event received since last call.
 * - DP_COUNT: number of events received since last call.
 * - DP_ALL  : an array with all the events received since last call.
 **********************************************************************/
function EventAggregator(handler, timeout, dataPolicy, latency, maxDelays, delayTimeout) {
  this.handler = handler;
  this.timeout = timeout || 1000;
  if (latency === 0) {
    this.latency = 0;
  } else {
    this.latency = latency || this.timeout;
  }
  this.maxDelays = maxDelays || 0;
  if (delayTimeout === 0) {
    this.delayTimeout = 0;
  } else {
    this.delayTimeout = delayTimeout || (this.timeout/2);
  }
  this.dataPolicy = dataPolicy || EventAggregator.DP_NONE;
  this.timer = null;
  this.clear();
}

EventAggregator.DP_NONE  = 0; // Don't keep data
EventAggregator.DP_FIRST = 1; // Keep only first
EventAggregator.DP_LAST  = 2; // Keep only last
EventAggregator.DP_COUNT = 3; // Count aggregations
EventAggregator.DP_ALL   = 4; // Keep all in an array


// Helper to set the callback so that 'this' is the aggregator
EventAggregator.prototype.setTimeout = function (delay) {
  var owner = this;
  return setTimeout(function () { owner.fired(); }, delay);
}

// Calls the handler with no delay.
EventAggregator.prototype.fireNow = function () {
  if (this.count > 0) {
    var count = this.count;
    var data = this.data;
    this.cancel();
    this.clear();
    switch (this.dataPolicy) {
    case EventAggregator.DP_FIRST:
    case EventAggregator.DP_LAST:
    case EventAggregator.DP_ALL:
      this.handler(data);
      break;
    case EventAggregator.DP_COUNT:
      this.handler(count);
      break;
    default:
      this.handler();
    }
    if (this.latency > 0) {
      this.timer = this.setTimeout(this.latency);
    }
  }
}

// Notifies the EventAggregator of an event.
EventAggregator.prototype.update = function (data) {
  switch (this.dataPolicy) {
  case EventAggregator.DP_FIRST:
    if (this.count === 0) {
      this.data = data;
    }
    break;
  case EventAggregator.DP_LAST:
    this.data = data;
    break;
  case EventAggregator.DP_ALL:
    this.data.push(data);
    break;
  }
  this.count++;
  if (this.timer === null) {
    this.timer = this.setTimeout(this.timeout);
  } else if (this.count > 1) {
    this.burst = true;
  }
}

// Timeout handler
EventAggregator.prototype.fired = function () {
  if (this.burst && this.delays < this.maxDelays) {
    // Event received while waiting to fire after another event
    this.burst = false;
    this.delays++;
    this.timer = this.setTimeout(this.delayTimeout);
  } else {
    this.timer = null;
    this.fireNow();
  }
}

// Cancels a timeout if there's one. Current data is kept.
EventAggregator.prototype.cancel = function () {
  if (this.timer !== null) {
    clearTimeout(this.timer);
    this.timer = null;
  }
}

// Clears current data and repeating counters.
EventAggregator.prototype.clear = function () {
  if (this.dataPolicy === EventAggregator.DP_ALL) {
    this.data = [];
  } else {
    this.data = undefined;
  }
  this.burst = false;
  this.delays = 0;
  this.count = 0;
}

