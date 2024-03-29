var deviceIp="";
var wsState = 0;


function wsConnect(wsIP) {
	let wsAdress = 'ws://'+wsIP+':81/'+data['page']+'.htm';
	console.log("wsAdress=", wsAdress);
	ws = new WebSocket(wsAdress, ['arduino']);
	
	ws.onopen = function(e) {
  		console.log("WS onConnected");
  		wsState = 1;
  	};

	ws.onclose = function(e) {
		console.log('WS is closed.', e.reason);
		wsState = 0;
	};

	ws.onerror = function (error) {
		wsState = 0;
	};

	ws.onmessage = function (e) {
		console.log('WS FROM Server: ', e.data);
		receivedDataProcessing (e.data);
	};
}



function startConControl(){
	console.log('deviceIp=', deviceIp);
	if (deviceIp!="")   wsConnect(deviceIp);
}



function startSendData(command) {
	switch (command) {
		case "JSON":
			sendFinishData(JSON.stringify(dataSend));
			break;
		case "RESET":
			sendFinishData('RESET');
			break;
	}

	function sendFinishData(dat){
		if (wsState==1){
			console.log('WS TO Server: ', dat);
			ws.send(dat);
		}
	};

}




function receivedDataProcessing(strJson){
	try {
		let obj=JSON.parse(strJson);
		for (x in obj) {
			if (data[x]!=null) {
				data[x]=obj[x];
			} else if (dataSend[x]!=null) {
				dataSend[x]=obj[x];
			}
		}

		updateAllPage();
	} catch (e) {
		console.log(e.message); 
	}
}




function setDeviceIp(setIp){
	deviceIp = setIp;
	console.log('deviceIp=', deviceIp);
}

if (location.host) {
	setDeviceIp(location.host);
	startConControl();
}

