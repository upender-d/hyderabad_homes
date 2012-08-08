window.log = function(){
  log.history = log.history || [];  
  log.history.push(arguments);
  arguments.callee = arguments.callee.caller;  
  if(this.console) console.log( Array.prototype.slice.call(arguments) );
};
(function(b){function c(){}for(var d="assert,count,debug,dir,dirxml,error,exception,group,groupCollapsed,groupEnd,info,log,markTimeline,profile,profileEnd,time,timeEnd,trace,warn".split(","),a;a=d.pop();)b[a]=b[a]||c})(window.console=window.console||{});


/* SOME STUFF FOR THE DOWNLOAD */
$(document).ready(function() {
	/* set the static url */
	if($('.download_static_link').length > 0) {
		$('.download_static_link').attr('href', staticDownloadURL);
	}
	
	/* set the dynamic url */
	if($('.download_dynamic_link').length > 0) {
		$('.download_dynamic_link').attr('href', dynamicDownloadURL);
	}
	
	/* set the version number */
	$('h3').each(function() {
		if($(this).text() === 'THE_VERSION') {
			$(this).text('Version ' + theTMPAndEspressoVersion.substr(1));	
		}
	});
	
	/* init the countdown */
	if($('.download_countdown').length > 0) {
		var downloadCountdown = window.setInterval(function() {
			var seconds = parseInt($('.download_countdown').html()) - 1;
			$('.download_countdown').html(seconds);
			if(seconds === 0) {
				window.clearInterval(downloadCountdown);
			}
		}, 1000);
	}
});