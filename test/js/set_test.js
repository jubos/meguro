function map(key,value) {
  var words = value.split(/\s+/);
  for(var i=0; i < words.length; i++) {
    Meguro.set(words[i],'100');
  }
}
