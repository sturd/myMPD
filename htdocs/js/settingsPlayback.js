"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

/** @module settingsPlayback_js */

/**
 * Initialization function for the playback settings elements
 */
function initSettingsPlayback() {
    initElements(document.getElementById('modalQueueSettings'));

    document.getElementById('modalQueueSettings').addEventListener('shown.bs.modal', function () {
        cleanupModalId('modalQueueSettings');
        getSettings();
    });

    document.getElementById('btnJukeboxModeGroup').addEventListener('mouseup', function () {
        setTimeout(function() {
            toggleJukeboxSettings();
            checkConsume();
        }, 100);
    });

    document.getElementById('btnConsumeGroup').addEventListener('mouseup', function() {
        setTimeout(function() {
            checkConsume();
        }, 100);
    });

    setDataId('selectJukeboxPlaylist', 'cb-filter', 'filterPlaylistsSelect');
    setDataId('selectJukeboxPlaylist', 'cb-filter-options', [0, 'selectJukeboxPlaylist']);
}

/**
 * Toggles the mpd playback mode
 * @param {string} option playback option to toggle
 */
//eslint-disable-next-line no-unused-vars
function togglePlaymode(option) {
    let value;
    let title;
    switch(option) {
        case 'random':
        case 'repeat':
            if (settings.partition[option] === true) {
                value = false;
                title = 'Disable ' + option;
            }
            else {
                value = true;
                title = 'Enable ' + option;
            }
            break;
        case 'consume':
            if (settings.partition.consume === '0') {
                value = 'oneshot';
                title = 'Enable consume oneshot';
            }
            else if (settings.partition.consume === 'oneshot') {
                value = '1';
                title = 'Enable consume mode';
            }
            else if (settings.partition.consume === '1') {
                value = '0';
                title = 'Disable consume mode';
            }
            break;
        case 'single':
            if (settings.partition.single === '0') {
                value = 'oneshot';
                title = 'Enable single oneshot';
            }
            else if (settings.partition.single === 'oneshot') {
                value = '1';
                title = 'Enable single mode';
            }
            else if (settings.partition.single === '1') {
                value = '0';
                title = 'Disable single mode';
            }
            break;
    }
    const params = {};
    params[option] = value;
    sendAPI('MYMPD_API_PLAYER_OPTIONS_SET', params, null, false);
    showNotification(tn(title), '', 'queue', 'info');
}

/**
 * Checks the state of the consume mode
 */
function checkConsume() {
    const stateConsume = getBtnGroupValueId('btnConsumeGroup');
    const stateJukeboxMode = getBtnGroupValueId('btnJukeboxModeGroup');
    if (stateJukeboxMode !== 'off' &&
        stateConsume !== '1')
    {
        elShowId('warnConsume');
    }
    else {
        elHideId('warnConsume');
    }
}

/**
 * Toggle jukebox setting elements
 */
function toggleJukeboxSettings() {
    const value = getBtnGroupValueId('btnJukeboxModeGroup');
    if (value === 'off') {
        elDisableId('inputJukeboxQueueLength');
        elDisableId('selectJukeboxPlaylist');
        elDisableId('btnJukeboxIgnoreHated');
    }
    else if (value === 'album') {
        elDisableId('inputJukeboxQueueLength');
        elDisableId('selectJukeboxPlaylist');
        elDisableId('btnJukeboxIgnoreHated');
        elDisable(document.getElementById('selectJukeboxPlaylist').nextElementSibling);
        document.getElementById('selectJukeboxPlaylist').value = 'Database';
        setDataId('selectJukeboxPlaylist', 'value', 'Database');
    }
    else if (value === 'song') {
        elEnableId('inputJukeboxQueueLength');
        elEnableId('selectJukeboxPlaylist');
        elEnableId('btnJukeboxIgnoreHated');
        elEnable(document.getElementById('selectJukeboxPlaylist').nextElementSibling);
    }
    if (value !== 'off') {
        toggleBtnGroupValueId('btnConsumeGroup', '1');
        toggleBtnGroupValueId('btnSingleGroup', '0');
    }
}

/**
 * Populates the playback settings modal
 */
function populateQueueSettingsFrm() {
    toggleBtnGroupValueCollapse(document.getElementById('btnJukeboxModeGroup'), 'collapseJukeboxMode', settings.partition.jukeboxMode);
    addTagListSelect('selectJukeboxUniqueTag', 'tagListBrowse');

    document.getElementById('selectJukeboxUniqueTag').value = settings.partition.jukeboxUniqueTag;
    document.getElementById('inputJukeboxQueueLength').value = settings.partition.jukeboxQueueLength;
    document.getElementById('inputJukeboxLastPlayed').value = settings.partition.jukeboxLastPlayed;
    toggleJukeboxSettings();
    document.getElementById('selectJukeboxPlaylist').filterInput.value = '';
    toggleBtnChkId('btnJukeboxIgnoreHated', settings.partition.jukeboxIgnoreHated);

    if (settings.partition.mpdConnected === true) {
        if (features.featPlaylists === true) {
            filterPlaylistsSelect(0, 'selectJukeboxPlaylist', '', settings.partition.jukeboxPlaylist);
            setDataId('selectJukeboxPlaylist', 'value', settings.partition.jukeboxPlaylist);
        }
        else {
            document.getElementById('selectJukeboxPlaylist').value = tn('Database');
            setDataId('selectJukeboxPlaylist', 'value', 'Database');
        }
        toggleBtnChkId('btnRandom', settings.partition.random);
        toggleBtnChkId('btnRepeat', settings.partition.repeat);
        toggleBtnChkId('btnAutoPlay', settings.partition.autoPlay);
        toggleBtnGroupValueId('btnConsumeGroup', settings.partition.consume);
        toggleBtnGroupValueId('btnSingleGroup', settings.partition.single);
        toggleBtnGroupValueId('btnReplaygainGroup', settings.partition.replaygain);
        document.getElementById('inputCrossfade').value = settings.partition.crossfade;
        document.getElementById('inputMixrampDb').value = settings.partition.mixrampDb;
        document.getElementById('inputMixrampDelay').value = settings.partition.mixrampDelay;
        if (features.featStickers === false) {
            elShowId('warnPlaybackStatistics');
            elDisableId('inputJukeboxLastPlayed');
        }
        else {
            elHideId('warnPlaybackStatistics');
            elEnableId('inputJukeboxLastPlayed');
        }
    }
    checkConsume();
}

/**
 * Saves the playback settings
 */
//eslint-disable-next-line no-unused-vars
function saveQueueSettings() {
    cleanupModalId('modalQueueSettings');
    let formOK = true;

    for (const inputId of ['inputCrossfade', 'inputJukeboxQueueLength', 'inputJukeboxLastPlayed']) {
        const inputEl = document.getElementById(inputId);
        if (validateIntEl(inputEl) === false) {
            formOK = false;
        }
    }

    const mixrampDbEl = document.getElementById('inputMixrampDb');
    if (validateFloatRangeEl(mixrampDbEl, -100, 0) === false) {
        formOK = false;
    }
    const mixrampDelayEl = document.getElementById('inputMixrampDelay');
    if (validateFloatRangeEl(mixrampDelayEl, -1, 100) === false) {
        formOK = false;
    }

    const jukeboxMode = getBtnGroupValueId('btnJukeboxModeGroup');
    let jukeboxUniqueTag = jukeboxMode === 'album'
        ? 'Album'
        : getSelectValueId('selectJukeboxUniqueTag');

    if (formOK === true) {
        btnWaitingId('btnSaveQueueSettings', true);
        sendAPI("MYMPD_API_PLAYER_OPTIONS_SET", {
            "random": getBtnChkValueId('btnRandom'),
            "single": getBtnGroupValueId('btnSingleGroup'),
            "consume": getBtnGroupValueId('btnConsumeGroup'),
            "repeat": getBtnChkValueId('btnRepeat'),
            "replaygain": getBtnGroupValueId('btnReplaygainGroup'),
            "crossfade": Number(document.getElementById('inputCrossfade').value),
            "mixrampDb": Number(mixrampDbEl.value),
            "mixrampDelay": Number(mixrampDelayEl.value),
            "jukeboxMode": jukeboxMode,
            "jukeboxPlaylist": getDataId('selectJukeboxPlaylist', 'value'),
            "jukeboxQueueLength": Number(document.getElementById('inputJukeboxQueueLength').value),
            "jukeboxLastPlayed": Number(document.getElementById('inputJukeboxLastPlayed').value),
            "jukeboxUniqueTag": jukeboxUniqueTag,
            "jukeboxIgnoreHated": getBtnChkValueId('btnJukeboxIgnoreHated'),
            "autoPlay": getBtnChkValueId('btnAutoPlay')
        }, saveQueueSettingsClose, true);
    }
}

/**
 * Handler for the MYMPD_API_PLAYER_OPTIONS_SET jsonrpc response
 * @param {object} obj jsonrpc response
 */
function saveQueueSettingsClose(obj) {
    btnWaitingId('btnSaveQueueSettings', false);
    if (obj.error) {
        showModalAlert(obj);
    }
    else {
        getSettings();
        uiElements.modalQueueSettings.hide();
    }
}
