"use strict";

/** @module Snapcast_js */

const SnapCastEndpoint = "";
let nextCommand = 0;

/*
 * Initialisation function for SnapCast Client
 */
function initSnapCast() {
    getStatus();
}

function getStatus() {
    sendSnapCast("Server.GetStatus",
        "",
        parseStatus,
        true
    );
}

function parseStatus(obj) {
    const selectStreamEl = document.getElementById('inputSettingsnapcastStream');
    const streams = obj.server.streams;
    for(var i = 0; i < streams.length; ++i) {
        console.log(streams[i].id);
        selectStreamEl.options[selectStreamEl.options.length] =
            new Option(streams[i].id, streams[i].id);
    }
}


/**
 * Sends a JSON-RPC SnapCast request to the selected partition and handles the response.
 * @param {string} method jsonrpc api method
 * @param {object} params jsonrpc parameters
 * @param {Function} callback callback function
 * @param {boolean} onerror true = execute callback also on error
 * @returns {boolean} true on success, else false
 */
 function sendSnapCast(method, params, callback, onerror) {
    return sendSnapCastpartition(localSettings.partition, method, params, callback, onerror);
 }

/**
 * Sends a JSON-RPC SnapCast request and handles the response.
 * @param {string} partition partition endpoint
 * @param {string} method jsonrpc api method
 * @param {object} params jsonrpc parameters
 * @param {Function} callback callback function
 * @param {boolean} onerror true = execute callback also on error
 * @returns {boolean} true on success, else false
 */
function sendSnapCastpartition(partition, method, params, callback, onerror) {
    if (settings.pin === true &&
        session.token === '' &&
        session.timeout < getTimestamp())
    {
        logDebug('Request must be authorized but we have no session');
        enterPin(method, params, callback, onerror);
        return false;
    }

    const snapcastHostEl = document.getElementById('inputSettingsnapcastHost');
    const snapcastPortEl = document.getElementById('inputSettingsnapcastPort');

    //we do not use the jsonrpc id field because we get the response directly.
    const request = {"jsonrpc": "2.0", "id": 0, "method": method, "host": snapcastHostEl.value, "port": snapcastPortEl.value};
    if(params != null && params != "")
        request.params = params;

    const ajaxRequest = new XMLHttpRequest();
    ajaxRequest.open('POST', subdir + '/snapcast/' + partition, true);
    ajaxRequest.setRequestHeader('Content-type', 'application/json');
    if (session.token !== '') {
        ajaxRequest.setRequestHeader('X-myMPD-Session', session.token);
    }
    ajaxRequest.onreadystatechange = function() {
        if (ajaxRequest.readyState !== 4) {
            return;
        }
        if (ajaxRequest.status !== 200 ||
            ajaxRequest.responseText === '' ||
            ajaxRequest.responseText.length > 1000000)
        {
            logError('Invalid response for request: ' + JSON.stringify(request));
            logError('Response code: ' + ajaxRequest.status);
            logError('Response length: ' + ajaxRequest.responseText.length);
            if (onerror === true) {
                if (callback !== undefined && typeof(callback) === 'function') {
                    logDebug('Got empty SnapCast response calling ' + callback.name);
                    callback({"error": {"message": "Invalid response"}});
                }
            }
            return;
        }

        if (settings.pin === true &&
            session.token !== '')
        {
            //session was extended through request
            session.timeout = getTimestamp() + sessionLifetime;
            resetSessionTimer();
        }
        let obj;
        try {
            obj = JSON.parse(ajaxRequest.responseText);
        }
        catch(error) {
            showNotification(tn('Can not parse response from %{uri} to json object', {"uri": subdir + '/api/' + partition}), '', 'general', 'error');
            logError('Can not parse response to json object:' + ajaxRequest.responseText);
        }
        if (obj.error &&
            typeof obj.error.message === 'string')
        {
            //show and log message
            showNotification(tn(obj.error.message, obj.error.data), '', obj.error.facility, obj.error.severity);
            logSeverity(obj.error.severity, ajaxRequest.responseText);
        }
        else if (obj.result)
        {
            //show no message
            logDebug('Got SnapCast response: ' + ajaxRequest.responseText);
        }
        else {
            //remaining results are invalid
            logError('Got invalid SnapCast response: ' + ajaxRequest.responseText);
            if (onerror !== true) {
                return;
            }
        }
        if (callback !== undefined &&
            typeof(callback) === 'function')
        {
            if (obj.result !== undefined ||
                onerror === true)
            {
                logDebug('Calling ' + callback.name);
                callback(obj.result);
            }
            else {
                logDebug('Undefined resultset, skip calling ' + callback.name);
            }
        }
    };
    ajaxRequest.send(JSON.stringify(request));
    logDebug('Send SnapCast request: ' + method);
    return true;
}
