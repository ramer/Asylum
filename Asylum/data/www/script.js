/*/////////////// GLOBAL SCRIPTS ////////////////////*/
/*
 * Change mode of operation
 */
function changeMode(event) {
    var local = event.target.value == 0 ? true : false;
    var mqtt = event.target.value == 2 ? true : false;
    ["networks", "apssid"].forEach(function (id) {
        var el = document.getElementById(id);
        if (id != "networks") {
            el.required = !local;
            if (local) {
                el.removeAttribute('required');
            }
        }
        el.disabled = local;
    });
    ["mqttserver", "mqttlogin", "mqttpassword"].forEach(function (id) {
        var el = document.getElementById(id);
        if (id == "mqttserver") {
            el.required = mqtt;
            if (!mqtt) {
                el.removeAttribute('required');
            }
        }
        el.disabled = !mqtt;
    });
}


/*
 * Load JSON from URL
 */
function getJSON(url, callback) {
    var xhr = new XMLHttpRequest();
    xhr.open('get', url, true);
    xhr.responseType = 'json';
    xhr.onload = function () {
        var status = xhr.status;
        if (status == 200) {
            callback(null, xhr.response);
        } else {
            callback(status);
        }
    };
    xhr.send();
};


/*
 * Change mode of operation
 */
function submitForm($form) {
    event.preventDefault();

    var formData = [];
    for (var i = 0; i < $form.elements.length; i++) {
        var e = $form.elements[i];
        if (!e.disabled &&
            e.name != "" &&
            e.name !== "saveForm" &&
            (e.type == "text" || e.type == "password")) {
            formData.push(encodeURIComponent(e.name) + "=" + encodeURIComponent(e.value));
            continue;
        }
        if ((e.name == "mode") && e.checked) {
            formData.push(encodeURIComponent(e.name) + "=" + encodeURIComponent(e.value));
            continue;
        }
        if ((e.name == "onboot") && e.checked) {
            formData.push(encodeURIComponent(e.name) + "=" + encodeURIComponent(e.value));
            continue;
        }
        if ((e.name == "onboardled") && e.checked) {
            formData.push(encodeURIComponent(e.name) + "=" + encodeURIComponent(e.value));
            continue;
        }
    }
    formData = formData.join("&");


    var xhr = new XMLHttpRequest();
    xhr.open($form.method, $form.action, true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.send(formData);
    xhr.onreadystatechange = function () {
        if (xhr.readyState != XMLHttpRequest.DONE) return;

        // alert response
        if (xhr.status != 200) {
            alert(xhr.status + ': ' + xhr.statusText);
        } else {
            alert(xhr.responseText);
        }
    }
}

/*
 * Checkbox click event
 */
function onCheckboxClick(event) {
    var el = event.target;
    el.disabled = true;

    var formData = [];
    formData.push(encodeURIComponent('state') + '=' + encodeURIComponent(+el.checked));
    formData.push(encodeURIComponent('id') + '=' + encodeURIComponent(el.id));
    formData = formData.join("&");

    var xhr = new XMLHttpRequest();
    xhr.open('post', '/api_command', true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.send(formData);
    xhr.onreadystatechange = function () {
        if (xhr.readyState != XMLHttpRequest.DONE) return;

        // alert response
        if (xhr.status != 200) {
            el.disabled = false;
            alert(xhr.status + ': ' + xhr.statusText);
        } else {
            var state = Boolean(parseInt(xhr.responseText));
            el.disabled = false;
            el.checked = state;
            if (state) {
                el.labels[0].innerHTML = 'ON';
            } else {
                el.labels[0].innerHTML = 'OFF';
            }
        }
    }
}

/*
 * Color change event
 */
function onColorChange(event) {
    var el = event.target;
    el.disabled = true;

    var formData = [];
    formData.push(encodeURIComponent('state') + '=' + encodeURIComponent(parseInt(el.value.replace("#", ""), 16)));
    formData.push(encodeURIComponent('id') + '=' + encodeURIComponent(el.id));
    formData = formData.join("&");

    var xhr = new XMLHttpRequest();
    xhr.open('post', '/api_command', true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.send(formData);
    xhr.onreadystatechange = function () {
        if (xhr.readyState != XMLHttpRequest.DONE) return;

        // alert response
        if (xhr.status != 200) {
            el.disabled = false;
            alert(xhr.status + ': ' + xhr.statusText);
        } else {
            var state = parseInt(xhr.responseText);
            el.disabled = false;
            el.value = "#" + state.toString(16);
        }
    }
}

/*
 * Range change event
 */
function onRangeChange(event) {
    var el = event.target;
    el.disabled = true;

    var formData = [];
    formData.push(encodeURIComponent('state') + '=' + encodeURIComponent(+el.value));
    formData.push(encodeURIComponent('id') + '=' + encodeURIComponent(el.id));
    formData = formData.join("&");

    var xhr = new XMLHttpRequest();
    xhr.open('post', '/api_command', true);
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    xhr.send(formData);
    xhr.onreadystatechange = function () {
        if (xhr.readyState != XMLHttpRequest.DONE) return;

        // alert response
        if (xhr.status != 200) {
            el.disabled = false;
            alert(xhr.status + ': ' + xhr.statusText);
        } else {
            var state = parseInt(xhr.responseText);
            el.disabled = false;
            el.value = state;
        }
    }
}

/*
 * Run this on load
 */

// load settings
function fillForm(data) {
    document.getElementById("uid").textContent = data.uid || "";
    document.getElementById("description").value = data.description || "";
    document.getElementById("mode" + parseInt(data.mode || "0")).click();
    document.getElementById("apssid").value = data.apssid || "";
    document.getElementById("apkey").value = data.apkey || "";
    document.getElementById("locallogin").value = data.locallogin || "";
    document.getElementById("localpassword").value = data.localpassword || "";
    document.getElementById("mqttserver").value = data.mqttserver || "";
    document.getElementById("mqttlogin").value = data.mqttlogin || "";
    document.getElementById("mqttpassword").value = data.mqttpassword || "";
    document.getElementById("onboot" + parseInt(data.onboot || "0")).click();
    document.getElementById("onboardled" + parseInt(data.onboardled || "0")).click();
    document.getElementById("extension1").value = data.extension1 || "";
    document.getElementById("extension2").value = data.extension2 || "";
    document.getElementById("extension3").value = data.extension3 || "";

    var networks = data.networks || "[]";
    var ddlnetworks = document.getElementById("networks");
    for (var i = 0; i < networks.length; i++) {
        var option = document.createElement("OPTION");
        option.innerHTML = networks[i].name;
        option.value = networks[i].ssid;
        ddlnetworks.options.add(option);
    }

    // show form
    document.getElementById("loading-placeholer").style.display = "none";
    document.getElementById("mainForm").style.display = "block";
}

// load controls
function fillControls() {
    // get control panel
    var xhr = new XMLHttpRequest();
    xhr.open('get', '/api_controls', true);
    xhr.send();
    xhr.onreadystatechange = function () {
        if (xhr.readyState != XMLHttpRequest.DONE) return;
        if (xhr.status != 200) {
            alert(xhr.status + ': ' + xhr.statusText);
        } else {
            document.getElementById('controlpanel').innerHTML = xhr.responseText;
        }
    }
}

try {
    // get form
    getJSON('/api_config', function (err, data) {
        if (err != null) {
            alert('getJSON returned: ' + err);
            fillForm([]);
        } else {
            fillForm(data);
        }
    });

    // get controls
    fillControls();

} catch (err) {
    alert('Something went wrong: ' + err);
    fillForm([]);
}