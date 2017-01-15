jQuery( document ).ready(function( $ ) {
  $(document).ready( updateDiv );
});

// update the content of div elements
function updateDiv() {
$.getJSON( "json", function() {
  //alert( "success" );
})
  .done(function(data) {
    $.each(data, function(key, value) {
      document.getElementById(key).innerHTML = value;
    });
    //alert( "done" );
  })
  .fail(function() {
    //alert( "error" );
  })
  .always(function() {
    //alert( "finished" );
  });
}

