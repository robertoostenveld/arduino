jQuery( document ).ready(function( $ ) {
  $(document).ready( updateDiv );
});

// update the content of div elements
function updateDiv() {
$.getJSON( "json", function() {
  //alert( "success" );
})
  .done(function(data) {
    console.log(data);
    $.each(data, function(key, value) {
      elem = document.getElementById(key);
      if (elem)
        elem.innerHTML = value;
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

