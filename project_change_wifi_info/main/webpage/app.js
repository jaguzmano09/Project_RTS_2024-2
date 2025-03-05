/**
 * Add gobals here
 */
var seconds 	= null;
var otaTimerVar =  null;
var wifiConnectInterval = null;

//MARK: INIT
/**
 * Initialize functions here.
 */
$(document).ready(function(){
	//getUpdateStatus();
	startNTCSensorInterval();// Start the interval for getting the updated NTC sensor values
	fetchSavedWiFi();// Fetch the saved WiFi SSID
	startWifiConnectStatusInterval();// Start the interval for checking the connection status
	setGETtime();// Set the interval for getting the updated time
	$("#connect_wifi").on("click", function(){
		checkCredentials();
	});
	$("#OFF_Send_UART").on("click", function(){
		turn_off_uart();
	});
	$("#ON_Send_UART").on("click", function(){
		turn_on_uart();
	});
	$("#forget_wifi").on("click", function(){
		forgetWiFi();
	});
});   

//MARK:FILE_INFO
/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() 
{
    var x = document.getElementById("selected_file");
    var file = x.files[0];

    document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
}

//MARK: MANUAL_MODE
function open_window()
{	
	$.ajax({
		url: '/OPEN_WINDOW.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}

function close_window()
{	
	$.ajax({
		url: '/CLOSE_WINDOW.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}



//MARK: Update_firware
/**
 * Handles the firmware update.
 */
function updateFirmware() 
{
    // Form Data
    var formData = new FormData();
    var fileSelect = document.getElementById("selected_file");
    
    if (fileSelect.files && fileSelect.files.length == 1) 
	{
        var file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Http Request
        var request = new XMLHttpRequest();

        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");
        request.responseType = "blob";
        request.send(formData);
    } 
	else 
	{
        window.alert('Select A File First')
    }
}

//MARK:Update_Progres
/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent) 
{
    if (oEvent.lengthComputable) 
	{
        getUpdateStatus();
    } 
	else 
	{
        window.alert('total size is unknown')
    }
}

//MARK: GET_STATUS
/**
 * Posts the firmware udpate status.
 */
function getUpdateStatus() 
{
    var xhr = new XMLHttpRequest();
    var requestURL = "/OTAstatus";
    xhr.open('POST', requestURL, false);
    xhr.send('ota_update_status');

    if (xhr.readyState == 4 && xhr.status == 200) 
	{		
        var response = JSON.parse(xhr.responseText);
						
	 	document.getElementById("latest_firmware").innerHTML = response.compile_date + " - " + response.compile_time

		// If flashing was complete it will return a 1, else -1
		// A return of 0 is just for information on the Latest Firmware request
        if (response.ota_update_status == 1) 
		{
    		// Set the countdown timer time
            seconds = 10;
            // Start the countdown timer
            otaRebootTimer();
        } 
        else if (response.ota_update_status == -1)
		{
            document.getElementById("ota_update_status").innerHTML = "!!! Upload Error !!!";
        }
    }
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() 
{	
    document.getElementById("ota_update_status").innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds == 0) 
	{
        clearTimeout(otaTimerVar);
        window.location.reload();
    } 
	else 
	{
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}


//MARK: NTC_READING
/**
 * Gets NTC sensor values of temperature for display on the web page.
 */
function getNTCSensorValues()
{
	$.getJSON('/NTCSensor.json', function(data) {
		$("#temperature_reading").text(data["temp"]);
	});
}
/**
 * Sets the interval for getting the updated NTC sensor values of temperature.
 */
function startNTCSensorInterval()
{
	setInterval(getNTCSensorValues, 1000);    
}

//MARK: UART_OFF
/**
 * Turn off uart function.
 */
function turn_off_uart() {
	var errorList = "";
	// Muestra mensaje de éxito en la página
	errorList += "<h4 class='rd'>UART send OFF!</h4>";
	$("#message_uart").html(errorList);
    // Enviar petición AJAX para apagar UART
    $.ajax({
        url: '/toogle_uart_off.json',
        dataType: 'json',
        method: 'POST',
        cache: false,
    });
}

//MARK: UART_ON
/**
 * turn on uart function.
 */
function turn_on_uart()
{
	var errorList = "";
	// Muestra mensaje de éxito en la página
	errorList += "<h4 class='rd'>UART send ON!</h4>";
	$("#message_uart").html(errorList);
	$.ajax({
		url: '/toogle_uart_on.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});
}

// MARK: WIFI_INFO
function fetchSavedWiFi() {
	fetch("/get_saved_wifi")
		.then(response => response.json())
		.then(data => {
			document.getElementById("saved_ssid").innerText = data.ssid;
		})
		.catch(error => console.error("Error obteniendo redes guardadas:", error));
}
window.onload = fetchSavedWiFi;
//MARK: WIFI_FORGET
/**
 * Forget WiFi function.
 */
function forgetWiFi() {
    fetch("/forget_wifi", { method: "POST" })
    .then(response => response.text())
    .then(data => {
        alert("WiFi eliminado con éxito!");
        setTimeout(() => {
            location.reload();
        }, 3000); // Espera 2 segundos antes de recargar
    })
    .catch(error => console.error("Error eliminando WiFi:", error));
}



//MARK: WIFI_CONNECT
/**
 * Clears the connection status interval.
 */
function stopWifiConnectStatusInterval()
{
	if (wifiConnectInterval != null)
	{
		clearInterval(wifiConnectInterval);
		wifiConnectInterval = null;
	}
}

/**
 * Gets the WiFi connection status.
 */
function getWifiConnectStatus() {
    fetch("/wifiConnectStatus", {
        method: "POST",
        headers: {
            "Content-Type": "application/json"
        },
        body: JSON.stringify({ wifi_connect_status: true })
    })
    .then(response => response.json())
    .then(data => {
        document.getElementById("wifi_connect_status").innerHTML = "Conectando...";

        if (data.wifi_connect_status === 2) {
            document.getElementById("wifi_connect_status").innerHTML = 
                "<h4 class='rd'>Error: No se pudo conectar. Verifica las credenciales y la compatibilidad del AP.</h4>";
            stopWifiConnectStatusInterval();
        } 
        else if (data.wifi_connect_status === 3) {
            document.getElementById("wifi_connect_status").innerHTML = "<h4 class='gr'>¡Conexión exitosa!</h4>";
            
            alert("WiFi conectado con éxito!");
            stopWifiConnectStatusInterval();
        }
    })
    .catch(error => {
        console.error("Error al obtener el estado de conexión:", error);
    });
}


/**
 * Starts the interval for checking the connection status.
 */
function startWifiConnectStatusInterval()
{
	wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

/**
 * Connect WiFi function called using the SSID and password entered into the text fields.
 */
function connectWifi()
{
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	// Create an object to hold the data to be sent in the request body
	var requestData = {
	  'selectedSSID': selectedSSID,
	  'pwd': pwd,
	  'timestamp': Date.now()
	};
	
	// Serialize the data object to JSON
	var requestDataJSON = JSON.stringify(requestData);
	
	$.ajax({
	  url: '/wifiConnect.json',
	  dataType: 'json',
	  method: 'POST',
	  cache: false,
	  data: requestDataJSON, // Send the JSON data in the request body
	  contentType: 'application/json', // Set the content type to JSON
	  success: function(response) {
		console.log(response);
		setTimeout(() => location.reload(), 2000); //  
	},
	error: function(xhr, status, error) {
		console.error(xhr.responseText);
		$("#wifi_status").text("No se pudo conectar.");
	}
});
}

/**
 * Checks credentials on connect_wifi button click.
 */
function checkCredentials()
{
	errorList = "";
	credsOk = true;
	
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	if (selectedSSID == "")
	{
		errorList += "<h4 class='rd'>SSID cannot be empty!</h4>";
		credsOk = false;
	}
	if (pwd == "")
	{
		errorList += "<h4 class='rd'>Password cannot be empty!</h4>";
		credsOk = false;
	}
	
	if (credsOk == false)
	{
		$("#wifi_connect_credentials_errors").html(errorList);
	}
	else
	{
		$("#wifi_connect_credentials_errors").html("");
        connectWifi().then(() => {
            setTimeout(() => {
                location.reload();
            }, 2000); // Espera 2 segundos antes de recargar la página
        }).catch(error => {
            console.error("Error al conectar:", error);
        });
    }
	
}

/**
 * Shows the WiFi password if the box is checked.
 */
function showPassword()
{
	var x = document.getElementById("connect_pass");
	if (x.type === "password")
	{
		x.type = "text";
	}
	else
	{
		x.type = "password";
	}
}


//MARK: LED
function toogle_led() 
{	
	$.ajax({
		url: '/toogle_led.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}

function brigthness_up() 
{	
	$.ajax({
		url: '/toogle_led.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}

//MARK: TIME
/**
 * Updates the time.
 */
function updateTime() {
		$.getJSON('/get_time.json', function(data) {
			$("#esp32_time").text(data["time"]);
		});
}

/**
 * Sets the interval for getting the updated time.
 * @returns {number} The interval for updating the time.
 */
function setGETtime() {
	setInterval(updateTime, 1000);
}

//MARK: Send_register
/**
 * Send register function.
 */
function send_register()
{
    // Assuming you have selectedNumber, hours, minutes variables populated from your form
    selectedNumber = $("#selectNumber").val();
    hours = $("#hours").val();
    minutes = $("#minutes").val();
    
    // Create an array for selected days
    var selectedDays = [];
    if ($("#day_mon").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_tue").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_wed").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_thu").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_fri").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_sat").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");
    if ($("#day_sun").prop("checked")) selectedDays.push("1");
	else selectedDays.push("0");

    // Create an object to hold the data to be sent in the request body
    var requestData = {
        'selectedNumber': selectedNumber,
        'hours': hours,
        'minutes': minutes,
        'selectedDays': selectedDays,
        'timestamp': Date.now()
    };

    // Serialize the data object to JSON
    var requestDataJSON = JSON.stringify(requestData);

	$.ajax({
		url: '/regchange.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		data: requestDataJSON, // Send the JSON data in the request body
		contentType: 'application/json', // Set the content type to JSON
		success: function(response) {
		  // Handle the success response from the server
		  console.log(response);
		},
		error: function(xhr, status, error) {
		  // Handle errors
		  console.error(xhr.responseText);
		}
	  });

}


//MARK:Read_register
/**
 * Read register function.
 */	
function read_reg()
{
	$.ajax({
		url: '/readreg.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
	});

}

//MARK: Get_Reg
/**
 * Gets the register values.
 */
function getregValues()
{
	$.getJSON('/read_regs.json', function(data) {
		$("#reg_1").text(data["reg1"]);
		$("#reg_2").text(data["reg2"]);
		$("#reg_3").text(data["reg3"]);
		$("#reg_4").text(data["reg4"]);
		$("#reg_5").text(data["reg5"]);
		$("#reg_6").text(data["reg6"]);
		$("#reg_7").text(data["reg7"]);
		$("#reg_8").text(data["reg8"]);
		$("#reg_9").text(data["reg9"]);
		$("#reg_10").text(data["reg10"]);
	});
}

function setgetregValues(){
	setInterval(getregValues, 1000);
}


//MARK: Erase_Reg
/**
 * Erase register function.
 */
function erase_reg()
{
    // Assuming you have selectedNumber, hours, minutes variables populated from your form
    selectedNumber = $("#selectNumber").val();
    // Create an object to hold the data to be sent in the request body
    var requestData = {
        'selectedNumber': selectedNumber,
        'timestamp': Date.now()
    };

    // Serialize the data object to JSON
    var requestDataJSON = JSON.stringify(requestData);

	$.ajax({
		url: '/regerase.json',
		dataType: 'json',
		method: 'POST',
		cache: false,
		data: requestDataJSON, // Send the JSON data in the request body
		contentType: 'application/json', // Set the content type to JSON
		success: function(response) {
		  // Handle the success response from the server
		  console.log(response);
		},
		error: function(xhr, status, error) {
		  // Handle errors
		  console.error(xhr.responseText);
		}
	  });
}











    










    


