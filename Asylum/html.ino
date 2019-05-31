
const char setup_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Настройка устройства</title>
  <link rel="stylesheet" type="text/css" href="style.css">
</head>

<body>
    <div class="wrapper_full">
        <header>
            <h1 id="uid"></h1>
        </header>
        <div id="loading-placeholer">Загрузка...</div>
        <form id="mainForm" style="display: none;" action="/submit" method="POST" onsubmit="submitForm(this);">
            <div class="field-group"><a class="upload" href="/upload">Обновление ПО</a></div>
            <div class="field-group">
                <label for="description" class="label">Описание:</label>
                <div class="field">
                    <input id="description" type="text" name="description" maxlength="128" placeholder="Люстра в гостиной" maxlength="64" autofocus />
                </div>
            </div>
            <div class="field-group">
                <div class="label">Режим работы устройства:</div>
                <div class="field" onchange="changeMode(event);">
                    <div>
                        <label>
                            <input id="mode0" name="mode" type="radio" value="0" checked="checked" />Автономно
                        </label>
                    </div>
                    <div>
                        <label>
                            <input id="mode1" name="mode" type="radio" value="1" />В локальной сети
                        </label>
                    </div>
                    <div>
                        <label>
                            <input id="mode2" name="mode" type="radio" value="2" />С сервером MQTT
                        </label>
                    </div>
                </div>
            </div>
            <h3>Беспроводная сеть</h3>
            <div class="field-group">
                <label for="apssid" class="label">SSID точки доступа:</label>
                <div class="field">
                    <div class="field_absolute_wrapper">
                        <select class="field_absolute_back" id="networks" onchange="document.getElementById('apssid').value=this.options[this.selectedIndex].value;" disabled="true">
                            <option></option>
                        </select>
                        <input class="field_absolute_front" id="apssid" type="text" name="apssid" maxlength="32" placeholder="Мой Wi-Fi" value="" onfocus="this.select()" disabled="true"/>
                    </div>
                </div>
            </div>
            <div class="field-group">
                <label for="apkey" class="label">Пароль:</label>
                <div class="field">
                    <input id="apkey" type="password" name="apkey" maxlength="32" value="" />
                </div>
            </div>
            <h3>Авторизация на устройстве</h3>
            <div class="field-group">
                <label for="locallogin" class="label">Имя пользователя:</label>
                <div class="field">
                    <input id="locallogin" type="text" name="locallogin" maxlength="32" placeholder="admin" value="" />
                </div>
            </div>
            <div class="field-group">
                <label for="localpassword" class="label">Пароль:</label>
                <div class="field">
                    <input id="localpassword" type="password" name="localpassword" maxlength="32" value="" />
                </div>
            </div>
            <h3>MQTT-сервер</h3>
            <div class="field-group">
                <label for="mqttserver" class="label">Адрес сервера:</label>
                <div class="field">
                    <input id="mqttserver" type="text" name="mqttserver" maxlength="64" placeholder="192.168.1.123" value="" disabled="true" required/>
                </div>
            </div>
            <div class="field-group">
                <label for="mqttlogin" class="label">Имя пользователя:</label>
                <div class="field">
                    <input id="mqttlogin" type="text" name="mqttlogin" maxlength="32" placeholder="vasya" value="" disabled="true" />
                </div>
            </div>
            <div class="field-group">
                <label for="mqttpassword" class="label">Пароль:</label>
                <div class="field">
                    <input id="mqttpassword" type="password" name="mqttpassword" maxlength="32" disabled="true" />
                </div>
            </div>
            <h3>Дополнительные параметры</h3>
            <div class="field-group">
                <div class="label">Состояние при включении устройства:</div>
                <div class="field">
                    <div>
                        <label>
                            <input id="onboot0" name="onboot" type="radio" value="0" checked="checked" />Выключено
                        </label>
                    </div>
                    <div>
                        <label>
                            <input id="onboot1" name="onboot" type="radio" value="1" />Включено
                        </label>
                    </div>
                    <div>
                        <label>
                            <input id="onboot2" name="onboot" type="radio" value="2" />Последнее состояние
                        </label>
                    </div>
                    <div>
                        <label>
                            <input id="onboot3" name="onboot" type="radio" value="3" />Инвертированное состояние
                        </label>
                    </div>
                </div>
            </div>
            <div class="field-group">
                <div class="label">Световой индикатор:</div>
                <div class="field">
                    <div>
                        <label>
                            <input id="onboardled0" name="onboardled" type="radio" value="0" checked="checked" />Выключен
                        </label>
                    </div>
                    <div>
                        <label>
                            <input id="onboardled1" name="onboardled" type="radio" value="1" />Включен
                        </label>
                    </div>
                </div>
            </div>
            <div class="field-group">
                <label for="extension1" class="label">Параметр 1:</label>
                <div class="field">
                    <div class="first">
                        <input id="extension1" type="text" name="extension1" maxlength="32" value=""/>
                    </div>
                </div>
            </div>
            <div class="field-group">
                <label for="extension2" class="label">Параметр 2:</label>
                <div class="field">
                    <div class="first">
                        <input id="extension2" type="text" name="extension2" maxlength="32" value=""/>
                    </div>
                </div>
            </div>
            <div class="field-group">
                <label for="extension3" class="label">Параметр 3:</label>
                <div class="field">
                    <div class="first">
                        <input id="extension3" type="text" name="extension3" maxlength="32" value=""/>
                    </div>
                </div>
            </div>
            <div class="field-group">
                <div class="label"></div>
                <div class="field">
                    <input id="saveForm" type="submit" name="saveForm" value="Сохранить и применить">
                </div>
            </div>

        </form>
    </div>
    <script>
        // IE 9- polyfill
        (function () {
            setTimeout(function (arg1) {
                if (arg1 === 'test') {
                    // feature test is passed, no need for polyfill
                    return;
                }
                var __nativeST__ = window.setTimeout;
                window.setTimeout = function (vCallback, nDelay /*, argumentToPass1, argumentToPass2, etc. */) {
                    var aArgs = Array.prototype.slice.call(arguments, 2);
                    return __nativeST__(vCallback instanceof Function ? function () {
                        vCallback.apply(null, aArgs);
                    } : vCallback, nDelay);
                };
            }, 0, 'test');

            var interval = setInterval(function (arg1) {
                clearInterval(interval);
                if (arg1 === 'test') {
                    // feature test is passed, no need for polyfill
                    return;
                }
                var __nativeSI__ = window.setInterval;
                window.setInterval = function (vCallback, nDelay /*, argumentToPass1, argumentToPass2, etc. */) {
                    var aArgs = Array.prototype.slice.call(arguments, 2);
                    return __nativeSI__(vCallback instanceof Function ? function () {
                        vCallback.apply(null, aArgs);
                    } : vCallback, nDelay);
                };
            }, 0, 'test');
        }())
    </script>
    <script>
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

        try {
            getJSON('/api_config', function (err, data) {
                if (err != null) {
                    alert('getJSON returned: ' + err);
                    fillForm([]);
                } else {
                    fillForm(data);
                }
            });
        } catch (err) {
            alert('Something went wrong: ' + err);
            fillForm([]);
        }

    </script>
</body>

</html>)~";

const char style_html[] PROGMEM = R"~(/*/////////////// GLOBAL STYLES ////////////////////*/

body {
    padding: 24px;
    background: white;
}

.wrapper_full {
    margin: auto;
    max-width: 100%;
}

.wrapper_fixed {
    margin: auto;
    max-width: 600px;
}

header {
    padding: 12px;
    margin-bottom: 30px;
}

/*/////////////// FONT STYLES ////////////////////*/

html {
    font-size: 62.5%;
}

body, textarea {
    font-family: 'Verdana, Geneva, sans-serif';
}

h1 {
    font-size: 3rem;
}

h2 {
    font-size: 1.6rem;
}

/*/////////////// FORM STYLES ////////////////////*/

form {
    background-color: #fff;
    border-radius: 7px;
    border: 1px solid #c1c1c1;
    background: -webkit-gradient(linear, left top, left bottom, from(#fff), to(#f3f3f3));
    background: -moz-linear-gradient(top, #fff, #f3f3f3);
    font-size: 1.4rem;
    padding: 12px;
    color: #222;
}

form .field-group {
    display: flex;
    margin: 0 0 12px 0;
}

form .field-group .label {
    flex: 1;
    text-align: right;
    margin: 0 8px 0 0;
    padding: 4px 4px;
}

form .field-group .upload {
    flex: 1;
    text-align: right;
    font-size: 1rem;
    color: black;
    float: right;
    text-decoration: none;
    padding: 4px 4px;
}

form .field-group .field {
    flex: 3;
}

form .field-group .field .field_absolute_wrapper {
    width: 50%;
    position: relative;
    box-sizing: border-box;
    padding-right: 21px;
}

form .field-group .field .field_absolute_wrapper .field_absolute_back {
    width: 100%;
    position: absolute;
}

form .field-group .field .field_absolute_wrapper .field_absolute_front {
    width: 100%;
    position: relative;
    z-index: 100;
}

form .field-group:last-child {
    display: flex;
    justify-content: flex-end;
}

form .field-group:last-child .field {
    max-width: 75%;
}

input, select, textarea {
    padding: 4px 4px;
    font-size: 1.6rem;
    border: solid 1px #eee;
    background: #fff;
    box-sizing: border-box;
}

select {
    border: solid 1px #c1c1c1;
    height: 30px;
}

input:disabled {
    background: #eee;
}

input:invalid {
    border: 1px solid red;
}

input:valid {
    border: 1px solid #c1c1c1;
}

select:focus, input:focus {
    outline: none;
}

textarea {
    width: 100%;
}

input, select {
    width: 50%;
}

input[type="text"] {
    height: 30px;
}

input[type="radio"], input[type="checkbox"], input[type="submit"] {
    width: auto;
}

.flexy {
    display: flex;
    flex-direction: row;
}

.flexy .testbutton {
    width: 100px;
}

.flexy .first {
    width: 130px;
}

/*/////////////// RWD STYLES ////////////////////*/

@media (max-width: 720px) {
    form .field-group .label {
        text-align: left;
        margin: 0;
    }
    input, select {
        width: 85%;
    }
    form .field-group .field .field_absolute_wrapper {
        width: 85%;
    }
}

@media (max-width: 480px) {
    input, select {
        width: 100%;
    }
    input[type="radio"], input[type="checkbox"], input[type="submit"] {
        width: auto;
    }
    form .field-group {
        display: flex;
        flex-direction: column;
        margin: 0 0 6px 0;
    }
    form .field-group .label {
        padding: 0 0 5px 0;
        margin: 10px 0 0 0;
    }
    form .field-group .field .field_absolute_wrapper {
        width: 100%;
    }
})~";

const char upload_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Обновление устройства</title>
  <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
  <div class="wrapper_fixed">
  <header>
    <h2>Обновление ПО устройства</h2>
  </header>
  <form method="POST" action="/update" enctype="multipart/form-data">
    <div class="field-group"><a class="upload" href="/">Назад</a></div>
    <h3>Укажите файл для загрузки</h3>
    <div>
      <input type="checkbox" id="spiffs" name="spiffs" value="1">
      <label for="spiffs">Обновление файловой системы</label>
    </div>
    <br>
    <div>
      <input type="file" name="update">
    </div>
    <br>
    <div>
      <input type="submit" value="Обновить и перезагрузить">
    </div>
  </form>
  </div>
</body>
</html>)~";

const char updatesuccess_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Обновление устройства</title>
  <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
  <div class="wrapper_fixed">
    <header>
        <h2>Обновление завершено успешно</h2>
    </header>
    <form>
        <div class="field-group"><a class="upload" href="/">Назад</a></div>
    </form>
  </div>
</body>
</html>)~";

const char updatefailure_html[] PROGMEM = R"~(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Обновление устройства</title>
  <link rel="stylesheet" type="text/css" href="style.css">
</head>
<body>
  <div class="wrapper_fixed">
    <header>
        <h2>Ошибка обновления</h2>
    </header>
    <form>
        <div class="field-group"><a class="upload" href="/">Назад</a></div>
    </form>
  </div>
</body>
</html>)~";