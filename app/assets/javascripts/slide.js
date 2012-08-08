$(document).ready(function() {

    // Expand Panel
    $(".open").click(function(){

        $("div#panel").slideDown("slow");
        $(".open").hide();
        $(".close").show();

    });

    // Collapse Panel
    $(".close").click(function(){
        $("div#panel").slideUp("slow");
            $(".close").hide();
            $(".open").show();

    });



});
