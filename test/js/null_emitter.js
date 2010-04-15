function map(key,value) {
  Meguro.emit(null,null);
}

function reduce(key,values) {
  Meguro.save(key, values.join('|'));
}
