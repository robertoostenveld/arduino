jQuery( document ).ready(function( $ ) {
  $(document).ready( updateForm );
});

// update the value of form elements
function updateForm() {
$.getJSON( "json", function() {
  //alert( "success" );
})
  .done(function(data) {
    //alert( "done" );
    $.each(data, function(key, value) {
      $( document.getElementById("settings-form").elements[key] ).val(value)
    });
  })
  .fail(function() {
    //alert( "fail" );
  })
  .always(function() {
    //alert( "always" );
  });
}
