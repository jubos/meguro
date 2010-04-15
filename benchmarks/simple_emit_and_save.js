function map(key,value) {
  Meguro.emit("" + key.length,key);
}

function reduce(key,values) {
  Meguro.save(key,values.length);
}
