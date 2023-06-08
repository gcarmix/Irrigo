function toggleCheckbox(element)
 {
   console.log(element.id)
   console.log($('#'+element.id).is(':checked'))
   $.get('/toggle',{c: element.id,v: $('#'+element.id).is(':checked')},function(response){

  });
 }

 setTimeout(function(){
  window.location.reload(1);
}, 5000);