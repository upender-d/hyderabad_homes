
$(function() {
    $( "#datepick" ).datepicker({
        maxDate: '0',
        dateFormat: 'dd-mm-yy',
        yearRange: "-60:+0",
        showOn: "button",
        buttonImage: "/assets/calendar.gif",
        buttonImageOnly: true,
        changeMonth: true,
        autoSize: true ,
        changeYear: true
    });
});




