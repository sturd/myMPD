"use strict";
// SPDX-License-Identifier: GPL-3.0-or-later
// myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
// https://github.com/jcorporation/mympd

function getBgImageList() {
    const list = document.getElementById('inputWebUIsettinguiBgImage');
    getImageList(list, bgImageValues, 'backgrounds');
}

//eslint-disable-next-line no-unused-vars
function getImageListId(selectId, addOptions, type) {
    getImageList(document.getElementById(selectId), addOptions, type)
}

function getImageList(sel, addOptions, type) {
    sendAPI("MYMPD_API_PICTURE_LIST", {
        "type": type
    }, function(obj) {
        elClear(sel.filterResult);
        for (const option of addOptions) {
            sel.addFilterResult(option.text, option.value);
        }
        for (let i = 0; i < obj.result.returnedEntities; i++) {
            sel.addFilterResult(obj.result.data[i], obj.result.data[i]);
        }
    }, false);
}

//eslint-disable-next-line no-unused-vars
function filterImageSelect(elId, searchstr) {
    const select = document.getElementById(elId).filterResult;
    searchstr = searchstr.toLowerCase();
    const items = select.querySelectorAll('li');
    for (const item of items) {
        const value = item.textContent.toLowerCase();
        if (value.indexOf(searchstr) >= 0) {
            elShow(item);
        }
        else {
            elHide(item);
        }
    }
}

/**
 * Checks if the uri is defined as an albumart file
 * @param {*} uri 
 * @returns {Boolean}
 */
 function isCoverfile(uri) {
    const filename = basename(uri, true);
    const fileparts = splitFilename(filename);

    const coverimageNames = [...settings.coverimageNames.split(','), ...settings.thumbnailNames.split(',')];
    for (let i = 0, j = coverimageNames.length; i < j; i++) {
        const name = coverimageNames[i].trim();
        if (filename === name) {
            return true;
        }
        if (name === fileparts.file &&
            imageExtensions.includes(fileparts.ext))
        {
            return true;
        }
    }
    return false;
}

/**
 * Checks if the uri is defined as an albumart thumbnail file
 * @param {*} uri 
 * @returns {Boolean}
 */
function isThumbnailfile(uri) {
    const filename = basename(uri, true);
    const fileparts = splitFilename(filename);

    const coverimageNames = settings.thumbnailNames.split(',');
    for (let i = 0, j = coverimageNames.length; i < j; i++) {
        const name = coverimageNames[i].trim();
        if (filename === name) {
            return true;
        }
        if (name === fileparts.file &&
            imageExtensions.includes(fileparts.ext))
        {
            return true;
        }
    }
    return false;
}

/**
 * Opens the picture modal
 * @param {HTMLElement} el 
 */
//eslint-disable-next-line no-unused-vars
function zoomPicture(el) {
    if (el.classList.contains('booklet')) {
        window.open(getData(el, 'href'));
        return;
    }

    if (el.classList.contains('carousel')) {
        let images;
        let embeddedImageCount;
        const dataImages = getData(el, 'images');
        if (dataImages !== undefined) {
            images = dataImages.slice();
            embeddedImageCount = getData(el, 'embeddedImageCount');
        }
        else if (currentSongObj.images) {
            images = currentSongObj.images.slice();
            embeddedImageCount = currentSongObj.embeddedImageCount;
        }
        else {
            return;
        }

        const uri = getData(el, 'uri');
        const imgEl = document.getElementById('modalPictureImg');
        imgEl.style.paddingTop = 0;
        createImgCarousel(imgEl, 'picsCarousel', uri, images, embeddedImageCount);
        elHideId('modalPictureZoom');
        uiElements.modalPicture.show();
        return;
    }

    if (el.style.backgroundImage !== '') {
        const imgEl = document.getElementById('modalPictureImg');
        elClear(imgEl);
        imgEl.style.paddingTop = '100%';
        imgEl.style.backgroundImage = el.style.backgroundImage;
        elShowId('modalPictureZoom');
        uiElements.modalPicture.show();
    }
}

/**
 * Opens the picture in a new window
 */
//eslint-disable-next-line no-unused-vars
function zoomZoomPicture() {
    window.open(document.getElementById('modalPictureImg').style.backgroundImage.match(/^url\(["']?([^"']*)["']?\)/)[1]);
}

/**
 * Creates the array of images and creates the image carousel
 * @param {HTMLElement} imgEl 
 * @param {String} name name to construct the image carousel id from
 * @param {*} uri uri of the image
 * @param {Object} images array of additional images
 * @param {Number} embeddedImageCount 
 */
function createImgCarousel(imgEl, name, uri, images, embeddedImageCount) {
    //embedded albumart
    if (embeddedImageCount === 0) {
        //enforce first coverimage
        embeddedImageCount++;
    }
    const aImages = [];
    for (let i = 0; i < embeddedImageCount; i++) {
        aImages.push(subdir + '/albumart?offset=' + i + '&uri=' + myEncodeURIComponent(uri));
    }
    //add all but coverfiles to image list
    for (let i = 0, j = images.length; i < j; i++) {
        if (isCoverfile(images[i]) === false) {
            aImages.push(subdir + myEncodeURI(images[i]));
        }
    }
    _createImgCarousel(imgEl, name, aImages);
}

/**
 * Creates the image carousel
 * @param {HTMLElement} imgEl 
 * @param {String} name 
 * @param {Object} images array of all images to display
 */
function _createImgCarousel(imgEl, name, images) {
    const nrImages = images.length;
    const carousel = elCreateEmpty('div', {"id": name, "class": ["carousel", "slide"], "data-bs-ride": "carousel"});
    if (nrImages > 1) {
        const carouselIndicators = elCreateEmpty('div', {"class": ["carousel-indicators"]});
        for (let i = 0; i < nrImages; i++) {
            carouselIndicators.appendChild(
                elCreateEmpty('button', {"type": "button", "data-bs-target": "#" + name, "data-bs-slide-to": i})
            );
            if (i === 0) {
                carouselIndicators.lastChild.classList.add('active');
            }
        }
        carousel.appendChild(carouselIndicators);
    }
    const carouselInner = elCreateEmpty('div', {"class": ["carousel-inner"]});
    for (let i = 0; i < nrImages; i++) {
        const carouselItem = elCreateNode('div', {"class": ["carousel-item"]},
            elCreateEmpty('div', {})
        );

        carouselItem.style.backgroundImage = 'url("' + images[i] + '")';
        carouselInner.appendChild(carouselItem);
        if (i === 0) {
            carouselItem.classList.add('active');
        }
    }
    carousel.appendChild(carouselInner);
    if (nrImages > 1) {
        carousel.appendChild(
            elCreateNode('a', {"href": "#" + name, "data-bs-slide": "prev", "class": ["carousel-control-prev"]},
                elCreateEmpty('span', {"class": ["carousel-control-prev-icon"]})
            )
        );
        carousel.appendChild(
            elCreateNode('a', {"href": "#" + name, "data-bs-slide": "next", "class": ["carousel-control-next"]},
                elCreateEmpty('span', {"class": ["carousel-control-next-icon"]})
            )
        );
    }

    elClear(imgEl);
    imgEl.appendChild(carousel);
    uiElements.albumartCarousel = new BSN.Carousel(carousel, {
        interval: false,
        pause: false
    });
}
