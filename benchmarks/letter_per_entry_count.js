function map(key,value) {
  var chars = new Object;
  for(var i=0; i < key.length; i++) {
    var kar = "" + (key.charAt(i));
    if (chars[kar]) {
      chars[kar]++;
    } else {
      chars[kar] = 1;
    }
  }

  for(var kar in chars) {
    Meguro.emit(kar,chars[kar]);
  }
}

function reduce(key,values) {
  Meguro.save(key,"" + values.length)
}
