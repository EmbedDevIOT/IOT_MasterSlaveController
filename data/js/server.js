
// Start "INDEX"
function StartIndex() {
    getConfig("index");   	   // Load configuration or  config.json 	
}

// Start "Network_Config"
function StartNetConfig() {
    getConfig("network");
}

// Start "System"
function StartSystem() {
    getConfig("system");
    getUpdate();
}

// Start "System"
function StartInfo() {
    getConfig("shortcut");
}


function getConfig(information) {
    // 1. Создаём новый XMLHttpRequest-объект с именем o
    let o = new XMLHttpRequest(); // Функция конструктор. При помощи встроенного в браузер обьекта XMLHttpRequest делаем HTTP запрос к серверу без перезагрузки страницы. 
    // 2. Настраиваем параметры запроса ( HTTP метод "GET" /куда запрос: URL или файл/)
    // o.open("GET", "config.json", 1); // поставить "/config.json"
    o.open("GET", "config.json", 1); // поставить "/config.json"
    // 3. Отправка инициализированного запроса 
    o.send();
    // 4. Этот код сработает после того, как мы получим ответ сервера
    o.onload = function () {
        // 5. Декодируем полученный текст ответа от сервера и запишем его в переменную ConfigFilePars
        jsonBuf = JSON.parse(o.responseText);

        // 6. Цикл для перебора всех свойств в строке ConfigFilePars
        for (let key in jsonBuf) {
            // Выполняется код внутри try, и если в нем нет ошибок - catch игнорируется. 
            try {
                let { type } = input = document.getElementById(key);
                if (type == "checkbox") input.checked = jsonBuf[key] > 0;
                else input.value = jsonBuf[key];
            }
            catch (e) { };

            if (information == "index") {
                document.getElementById("sn").value = jsonBuf["sn"];
                document.getElementById("carname").value = jsonBuf["carname"];
                document.getElementById("carnum").value = jsonBuf["carnum"];
                // document.getElementById("infotext").value = jsonBuf["infotext"];
                // document.getElementById("speed").value = jsonBuf["speed"];
                // document.getElementById("c_infotext").value = jsonBuf["c_infotext"];
                document.getElementById("c_carnum").value = jsonBuf["c_carnum"];
                document.getElementById("c_time").value = jsonBuf["c_time"];
                document.getElementById("c_date").value = jsonBuf["c_date"];
                document.getElementById("c_tempin").value = jsonBuf["c_tempin"];
                document.getElementById("c_tempout").value = jsonBuf["c_tempout"];


                // if (jsonBuf["runtext"] == 1) {
                //     const element1 = document.getElementById("sw_runtext");
                //     const attribute = document.createAttribute('checked');
                //     element1.setAttributeNode(attribute);
                // }
                // else {
                //     document.getElementById("sw_runtext").removeAttribute('checked');
                // }

                if (jsonBuf["hide"] == 1) {
                    const element1 = document.getElementById("sw_hide");
                    const attribute = document.createAttribute('checked');
                    element1.setAttributeNode(attribute);
                }
                else {
                    document.getElementById("sw_hide").removeAttribute('checked');
                }
            }
            else if (information == "network") {
                document.getElementById("wifiip").value = jsonBuf["ip1"] + '.' + jsonBuf["ip2"] + '.' + jsonBuf["ip3"] + '.' + jsonBuf["ip4"];
                document.getElementById("ssid_name").value = jsonBuf["ssid"];
            }
            else if (information == "system") {
                document.getElementById("t1_offset").value = jsonBuf["t1_offset"];
                document.getElementById("t2_offset").value = jsonBuf["t2_offset"];
                document.getElementById("brigh").value = jsonBuf["br"];
                document.getElementById("volume").value = jsonBuf["vol"];
            }
            else if (information == "shortcut") {
                document.getElementById("sn").value = jsonBuf["sn"];
                document.getElementById("firmware").value = jsonBuf["firmware"];
            }
        }

        // if (jsonBuf["sn"] == 0) {
        //     info_msg = "Внимание! Первый запуск устройства. Необходимо установить серийный номер и параметры конфигурации.";
        //     alert(info_msg);
        // }
    }
}

function getUpdate() {
    let xml = new XMLHttpRequest();
    xml.open("GET", "update.json", 1);
    xml.send();
    xml.onload = function () {
        let Pars = JSON.parse(xml.responseText);
        document.getElementById("date").value = Pars["d"];
        document.getElementById("time").value = Pars["t"];
    }
    // setTimeout("getUpdate()", 1000);
}


function AllDataUPD() {
    let xml = new XMLHttpRequest();
    let buf = "?";

    // var sw = document.getElementById("sw_runtext")
    // var isCheck = sw.checked;


    buf += "TN=" + document.getElementById("carname").value + "&";
    buf += "TNU=" + document.getElementById("carnum").value + "&";
    // buf += "TI=" + document.getElementById("infotext").value + "&";

    // if (isCheck) {
    //     buf += "SW=" + 1 + "&";
    // } else buf += "SW=" + 0 + "&";

    var sw = document.getElementById("sw_hide");
    var isCheck = sw.checked;

    if (isCheck) {
        buf += "SWH=" + 1;
    } else buf += "SWH=" + 0;

    // buf += "SP=" + document.getElementById("speed").value;

    xml.open("GET", "TextUPD" + buf, 1);
    xml.send();
    setTimeout("BColUPD()", 200);
    // console.log(xml);
}

function BColUPD() {
    let xml = new XMLHttpRequest();
    let buf = "?";

    buf += "CC=" + document.getElementById("c_carnum").value + "&";
    // buf += "CI=" + document.getElementById("c_infotext").value + "&";
    buf += "CT=" + document.getElementById("c_time").value + "&";
    buf += "CD=" + document.getElementById("c_date").value + "&";
    buf += "CTI=" + document.getElementById("c_tempin").value + "&";
    buf += "CTO=" + document.getElementById("c_tempout").value;

    xml.open("GET", "ColUPD" + buf, 1);
    xml.send();
    alert("Настройки сохранены. Обновление займет до 10 сек.");
    console.log(xml);
}

function BSysUPD() {
    let xml = new XMLHttpRequest();
    let buf = "?";
    buf += "T=" + document.getElementById("time").value + "&";
    buf += "D=" + document.getElementById("date").value + "&";
    buf += "T1O=" + document.getElementById("t1_offset").value + "&";
    buf += "T2O=" + document.getElementById("t2_offset").value + "&";
    buf += "BR=" + document.getElementById("brigh").value + "&";
    buf += "VOL=" + document.getElementById("volume").value;

    xml.open("GET", "SysUPD" + buf, 1);
    console.log(xml);
    xml.send();
    alert("Настройки сохранены.");

}

function BTellmeTime()
{
    let xml = new XMLHttpRequest();
    xml.open("GET", "BTTS", 1);
    xml.send();
}

function BDoorState()
{
    let xml = new XMLHttpRequest();
    xml.open("GET", "BDS", 1);
    xml.send();
}

function BWiFiUPD() {

    let xml = new XMLHttpRequest();

    result = confirm("Вы уверены, что хотите изменить имя точки доступа / пароль табло?");

    if (result) {

        let buf = "?";
        buf += "ssid=" + document.getElementById("ssid_name").value + "&";
        buf += "pass=" + document.getElementById("pass").value;

        xml.open("GET", "WiFiUPD" + buf, 1);
        xml.send();
        mcu_restart();
        // console.log(xml);
        let msg = "Изменения сохранены! Табло уже перезагружается (займёт 15сек).\r\n";
        msg += "Запишите новые настройки:\r\n";
        msg += "Имя сети: ";
        msg += document.getElementById("ssid_name").value + "\r\n";
        msg += "Пароль: ";
        msg += document.getElementById("pass").value + "\r\n";
        msg += "\r\n";
        msg += "Если пароль будет утерян, можно сбросить все настройки до";
        msg += " заводских (удерживать кнопки №1 + №5 (две крайние одновременно) в течение 20сек)";

        alert(msg);
    }
}

function reload() {
    window.location.reload();
}

function mcu_restart() {
    let xml = new XMLHttpRequest();
    xml.open("GET", "BRBT", 1);
    console.log(xml);
    xml.send();

}

function FactoryReset() {
    result = confirm("Вы уверены, что хотите сбросить все установленные настройки?");
    if (result) {

        let xml = new XMLHttpRequest();
        xml.open("GET", "BFRST", 1);
        console.log(xml);
        xml.send();
        mcu_restart();

        let msg = "Изменения сохранены! Табло уже перезагружается (займёт 15сек).\r\n";
        msg += "Настройки установлены по умолчанию:\r\n";
        msg += "Имя сети: ";
        msg += "retra0846 \r\n";
        msg += "Пароль: ";
        msg += "retra0846zxc\r\n";
        alert(msg);

    }
}