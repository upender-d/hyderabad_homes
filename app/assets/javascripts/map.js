/**
 * @author: ideawu
 * @link: http://www.ideawu.com/prj/address-picker/
 */

var MyMap = {};

// callback(LatLng)
MyMap.auto_location = function(callback, fallback_addr){
	if(navigator.geolocation) {
		navigator.geolocation.getCurrentPosition(
			function(position){
				var pos = new google.maps.LatLng(position.coords.latitude, position.coords.longitude);
				callback(pos);
			},
			function(){
				if(default_addr){
					MyMap.geoencode(fallback_addr, function(pos){
						callback(pos);
					});
				}
			}
		);
	} else {
		if(fallback_addr){
			MyMap.geoencode(fallback_addr, function(pos){
				callback(pos);
			});
		}
	}
}


/**
 * option{addr, pos, callback(pos, addr, addr_ext), width, height}
 * if addr and pos are both provided, only pos will be used.
 */
function AddressPicker(option){
	var self = this;

	var default_opt = {
		callback : function(){},
		width: 640,
		height: 420
	};

	if(option == undefined){
		option = default_opt;
	}else{
		for(var k in default_opt){
			if(option[k] == undefined){
				option[k] = default_opt[k];
			}
		}
	}

	var map = null;
	var marker;
	var geocoder;

	var html = ''
		+ '<div style="display: none; background: #fff;">'
		+ '		<p style="margin: 0; padding: 4px 0;">'
		+ '			Address: <input type="text" name="input_addr" value="" style="width:240px;" />'
		+ '			<button class="locate">Locate</button>'
		+ '			Click on the map to select the exact position.'
		+ '		</p>'
		+ '		<div class="map_canvas" style="border: 1px solid #999;"></div>'
		+ '		<div style="text-align: center; margin: 8px 0;"><button class="cancel">Cancel</button></div>'
		+ '</div>';
	self.node = $(html);
	self.node.appendTo($('body'));

	self.node.find('button.cancel').click(function(){
		self.close();
	});
	self.node.find('button.locate').click(function(){
		lacate_at_addr(self.node.find('input[name=input_addr]').val());
	});
	self.node.find('input[name=input_addr]').keydown(function (e){
		var key = e.which;
		if (key == 13) {
			lacate_at_addr(self.node.find('input[name=input_addr]').val());
		}
	});

	if(option.addr){
		self.node.find('input[name=input_addr]').val(option.addr);
	}

	self.close = function(){
		self.node.slideUp('normal', function(){
			infowindow.close();
		});
	}

	self.open = function(){
		var top = ($(window).height() - option.height) /2;
		var left = ($(window).width() - option.width) /2;
        /*var top = 680;
		var left = 820;*/
		top = Math.max(0, top);
		left = Math.max(0, left);
		
		self.node.css({
			padding: '10px',
			border: '3px solid #3cf',
			position: 'absolute',
			top:  top + 'px', 
			left: left + 'px',
			width: option.width + 'px'
		});

		self.node.slideDown('normal', function(){
			if(map == null){
				var map_canvas = self.node.find('.map_canvas');
				map_canvas.css('width', option.width).css('height', option.height);
				map_init(map_canvas[0]);
			}
		});
	}

	function set_position(pos){
		marker.setPosition(pos);
		map.setZoom(12);
		map.setCenter(pos);
	}

	function lacate_at_addr(addr){
		infowindow.close();
		geocoder.geocode({'address': addr}, function(results, status){
			if (status == google.maps.GeocoderStatus.OK) {
				var pos = results[0].geometry.location;
				set_position(pos);
			}
		});
	}

	function map_init(map_canvas){
		var myOptions = {
			zoom: 12,
			mapTypeId: google.maps.MapTypeId.ROADMAP
		};
		map = new google.maps.Map(map_canvas, myOptions);
		marker = new google.maps.Marker({map: map});
		geocoder = new google.maps.Geocoder();
		infowindow = new google.maps.InfoWindow({});

		set_position(new google.maps.LatLng(21.1256, 78.3106))

		if(option.pos){
			set_position(option.pos);
		}else if(option.addr){
			lacate_at_addr(option.addr);
		}else{
			MyMap.auto_location(function(pos){
				set_position(pos);
			});
		}

		function geodecode(latLng){
			marker.setPosition(latLng);
			
			var str = '<div style="text-align: center;">';
			str += '<b>Loading...</b>';
			str += '</div>';
			infowindow.setContent(str);
			infowindow.open(map, marker);

			geocoder.geocode({'latLng': latLng}, function(results, status){
				if (status != google.maps.GeocoderStatus.OK) {
					return;
				}
				var addr = results[0].formatted_address;

				var str = '<div>';
				str += '<div style="text-align: center; font-size: 110%;">';
				str += '<p><span class="addr">' + addr + '</span>';
				str += ' <input type="text" name="addr_ext" value="Addition..." style="width: 140px;color:#999;"';
				str += 'onclick="if(this.value==\'Addition...\')this.value=\'\';" />';
				str += '</p>'
				str += '<button class="ok">Done</button>';
				str += '</div>';
				str += '</div>';

				var node = $(str);
				node.find('button.ok').click(function(){
					//infowindow.close();
					
					var addr_ext = node.find('input[name=addr_ext]').val();
					if(addr_ext == 'Addition...'){
						addr_ext = '';
					}

					var ret = option.callback(latLng, addr, addr_ext);
					if(ret !== false){
						self.close();
					}

					return false;
				});

				infowindow.setContent(node[0]);
			});
		}

		google.maps.event.addListener(map, 'click', function(event){
			geodecode(event.latLng);
		});
		google.maps.event.addListener(marker, 'click', function(event){
			geodecode(event.latLng);
		});
	}
}
