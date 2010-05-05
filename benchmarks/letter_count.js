function map(key,value) {
  var chars = []
  for(var i=0; i < value.length; i++) {
    var kar = value.charAt(i);
    if (chars[kar]) {
      chars[kar]++;
    } else {
      chars[kar] = 1;
    }
  }

  for(var kar in chars) {
    //Meguro.log("There are " + chars[kar] + " " + kar + " in " + key);
    Meguro.emit(kar,chars[kar]);
  }
}

function reduce(key,values) {
  sum = 0;
  for(var i=0; i < values.length; i++) {
    sum += parseInt(values[i]);
  }
  Meguro.save(key,sum);
}
