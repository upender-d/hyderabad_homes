/* DO DEVICE STUFF */
//css def.
//tablet @media handheld and (max-width: 480px), screen and (max-device-width: 480px), screen and (max-width: 767px)  {  {
//mobile @media handheld and (max-width: 479px), screen and (max-device-width: 479px), screen and (max-width: 479px)  {

var deviceDetection = (function(){

	//you can use this to check for an option - is set when a new device is detected
	var currentDevice = {};
	//constants
	var MOBILEDEVICE = 1024;
	var TABLETDEVICE = 800;
	var COMPUTERDEVICE = 1025;
	
	var getDeviceProps = function(size){
		if(size === MOBILEDEVICE) 	return {width: MOBILEDEVICE, 	type: 'mobilephone'	};
		if(size === TABLETDEVICE) 	return {width: TABLETDEVICE, 	type: 'tablet'		};
		if(size === COMPUTERDEVICE) return {width: COMPUTERDEVICE,	type: 'computer'	};
		return {};
	};
	
	// detectDevice function needs this var to check if there was an device change
	var  deviceCheckLastDevicedInLoop = '';
	
	// triggers newDeviceSize  - if it is a new one
	// returns a device object
	// callback with the device object as the first parameter
	var detectDevice = function(callback){
		var ww = window.innerWidth;
		//var wh = window.innerHeight;
		if(ww <= MOBILEDEVICE && (deviceCheckLastDevicedInLoop === '' || deviceCheckLastDevicedInLoop !== MOBILEDEVICE)){
			$(document).trigger('newDeviceSize', [getDeviceProps(MOBILEDEVICE)]);
			deviceCheckLastDevicedInLoop = MOBILEDEVICE;
		}else if(ww > MOBILEDEVICE && ww < COMPUTERDEVICE && (deviceCheckLastDevicedInLoop === '' || ww > MOBILEDEVICE && deviceCheckLastDevicedInLoop !== TABLETDEVICE)){
			$(document).trigger('newDeviceSize', [getDeviceProps(TABLETDEVICE)]);
			deviceCheckLastDevicedInLoop = TABLETDEVICE;
		}else if(ww >= COMPUTERDEVICE && (deviceCheckLastDevicedInLoop === '' || deviceCheckLastDevicedInLoop !== COMPUTERDEVICE)){
			$(document).trigger('newDeviceSize', [getDeviceProps(COMPUTERDEVICE)]);
			deviceCheckLastDevicedInLoop = COMPUTERDEVICE;
		}
		if(callback){
			callback(getDeviceProps(deviceCheckLastDevicedInLoop));
		}
		return getDeviceProps(deviceCheckLastDevicedInLoop);
	};
	
	/*var reRender = function(settings){
		
		Object.keys(settings).forEach(function(key, index){
			console.log(index, key, settings[key]);
			parent = $(key);
			var inner = settings[key];
			if(inner && inner.length > 0){
				Object.keys(inner).forEach(function(key, index){
					var from = inner[key].from
					var to = inner[key].to
					console.log(from, to);
				});
			}
		});
	};*/
	
	$(document).ready(function(){
	
		/*bind resize*/
		$(window).resize(function(){
			detectDevice();
		});
		
		// sets the currentDevice when a new device is detected
		$(document).bind('newDeviceSize', function(obj, device){
			currentDevice = getDeviceProps(device);
		});
		
		//detect current device
		detectDevice();
		
	});
	
	return {
		/* return the current device*/
		getCurrentDevice: function(){
			return currentDevice;
		},
		
		/* initializes again */
		reset: function(callback){
			return detectDevice(callback);
		}
		
		/*adjust: function(settings){
			throw 'deviceDetection.adjust is not implemented yet';
			//return reRender(settings);
		}*/
	}
	
})();