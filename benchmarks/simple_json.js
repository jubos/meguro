function map(key,value) {
  var json = JSON.parse(value);
  Meguro.emit(key,json.length);
}
