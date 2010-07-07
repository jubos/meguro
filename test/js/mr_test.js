function map(key,value) {
  var words = value.split(/\s+/);
  for(var i=0; i < words.length; i++) {
    Meguro.emit(words[i],'1');
  }
}

function reduce(key,values) {
  Meguro.save(key,values.length);
}
