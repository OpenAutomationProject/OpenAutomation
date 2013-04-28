<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8" />
<title>jQuery Google Kalender Termine</title>
 <META NAME="Author" CONTENT="by NetFritz">
<!-- <link rel="stylesheet" href="http://code.jquery.com/ui/1.10.2/themes/smoothness/jquery-ui.css" /> -->
<link rel="stylesheet" href="http://code.jquery.com/ui/1.10.2/themes/ui-darkness/jquery-ui.css" />
<link rel="stylesheet" media="all" type="text/css" href="css/jquery-ui-timepicker-addon.css" />
<link type="text/css" rel="stylesheet" href="css/jquery.flexbox.css" />
<link rel="stylesheet" type="text/css" href="css/g_kalender.css">

<script src="http://code.jquery.com/jquery-1.9.1.js"></script>
<script src="http://code.jquery.com/ui/1.10.2/jquery-ui.js"></script>

<script type="text/javascript" src="js/jquery-ui-timepicker-addon.js"></script>
<script type="text/javascript" src="js/jquery.flexbox.min.js"></script></p>
<script type="text/javascript" src="js/jquery.flexbox.js"></script></p>  
<script type="text/javascript" src="eibga.js"></script>   <!-- wird von schaltliste erstellen erzeugt -->
<script>

  var datetimepick_start = "";
  var datetimepick_end = "";
  var NameGrAdrDtp_value = "";
  var wertvalue = "";

function checkForm1(wert) {
  $.ajax( {
    type: "GET",
    url: "event_to_gCal.php",
    data: {time_start:datetimepick_start,time_end:datetimepick_end,title:NameGrAdrDtp_value + wertvalue},
    success: function(antwort){
	window.location.href = "http://wiregate544/gCal/index.php";
	// alert( "Senden erfolgreich: " + antwort );
  }
});
};

function checkForm2(wert) { 
  $.ajax( {
    type: "POST",
    url: "eibga_to_schaltliste.php",
    // data: "time=" + datetimepick + "&title=" + NameGrAdrDtp_value + wertvalue,
    success: function(antwort){
	window.location.href = "http://wiregate544/gCal/index.php";
	// alert( "Senden erfolgreich: " + antwort );
  }
});
};

$(document).ready(function(){
$(function() {
   $( "#dialog" ).dialog({
      position: [20,40],
      height:400,
      width:800,
      modal: true,
      autoOpen: false,
      show: {
        effect: "blind",
        duration: 1000
      },
      hide: {
        effect: "explode",
        duration: 1000
      }   
   });
   $( "#opener" ).click(function() {
      $( "#dialog" ).dialog( "open" );
   });
});
    $.timepicker.regional['de'] = {
      timeOnlyTitle: 'Uhrzeit auswaehlen',
      timeText: 'Zeit',
      hourText: 'Stunde',
      minuteText: 'Minute',
      secondText: 'Sekunde',
      currentText: 'Jetzt',
      closeText: 'Auswaehlen',
      ampm: false
    };
   $.timepicker.setDefaults($.timepicker.regional['de']);
	
    var ex13 = $('#event_date_start');
	$('#event_date_start').datetimepicker({ 
	  controlType: 'select',
	  showSecond: false,
	  //timeFormat: 'HH:mm',	  
	  dateFormat: 'yy-mm-dd',
      onClose: function(dateText, inst) {
		         datetimepick_start = ex13.datetimepicker('getDate');
	  }
	});	
	
	    var ex14 = $('#event_date_end');
	$('#event_date_end').datetimepicker({ 
	  controlType: 'select',
	  showSecond: false,
	  //timeFormat: 'HH:mm',	  
	  dateFormat: 'yy-mm-dd',
      onClose: function(dateText, inst) {
		         datetimepick_end = ex14.datetimepicker('getDate');
	  }
	});






	$('#NameGrAdrDtp').flexbox(eibga,{
        watermark: 'Name ; GrpAdr ; Dpt ; auswaehlen',
        width: 550,
	    paging: false ,
	    maxVisibleRows: 10, 
        onSelect: function()
		          {
                     NameGrAdrDtp_value = this.value;
		          }
        });
	
	var data = {results: [{id:1,name:'0'},{id:2,name:'1'},{id:3,name:'0;1'},{id:4,name:'1;0'},{id:5,name:'0;0'},{id:6,name:'1;1'},], total:6};
	$('#wert').flexbox(data,{
        watermark: 'Wert auswaehlen',
        width: 150,
	    paging: false ,
	    maxVisibleRows: 10, 
        onSelect: function()
                  {
				     wertvalue = this.value;
                  }
    });





});
</script>
</head>
<body>
<button id="opener">Schalttermine eintragen</button>
<div id="dialog" title="Schalttermin eintragen">

 <table style="width:100%; border: solid 3px gray; border-spacing: 9px; border-radius: 6px">
	   <colgroup>
       <col width="10%">
       <col width="60%">
       <col width="20%">
	   <col width="10%">
       </colgroup>
       <tr>
          <th colspan="4" style="text-align:center; border: solid 3px gray; padding: 10px; border-radius: 6px">
             <div>Start: <input name="event_date_start" type="text" size="23" id="event_date_start"/>
			      End: <input name="event_date_end" type="text" size="23" id="event_date_end"/></div>
		  </th>
       </tr>
	   <tr>
	   	  <th colspan="4" style="border: solid 3px gray; padding: 10px; border-radius: 6px">
	        <div id="NameGrAdrDtp" class="post"></div>
		  </th>
	   </tr>
	   <tr>
	   	  <td colspan="1" style="text-align:center; border: solid 3px gray; padding: 10px; border-radius: 6px">
            <button class="reply" value="id1" onclick="checkForm1(this);">SAVE Termin</button>
		  </td>	
		  </td>
	   	   	  <td colspan="2"  style="border: solid 3px gray; padding: 10px; border-radius: 6px"> 
	        <div id="wert"></div>
		  <td colspan="1" style="text-align:center; border: solid 3px gray; padding: 10px; border-radius: 6px">
            <button class="reply" value="id2" onclick="checkForm2(this);"> lade eibga</button>
		  </td>
	   </tr>
</table>
</div>

      <!-- Google Kalender einbinden	 -->
     <div style="position:absolute; top:35px; left:8px">
       <iframe src="https://www.google.com/calendar/embed?src=myhouse544%40gmail.com&ctz=Europe/Berlin" style="border: 0" width="925" height="600" frameborder="0" scrolling="no"></iframe>
     </div> 
</body>
</html>