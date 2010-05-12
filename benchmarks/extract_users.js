/*
 * This extracts users from a twitter stream and makes them addressable by
 * username.
 */
function map(key,value) {
  var json = JSON.parse(value);
  if (json.user) {
    Meguro.set(json.user.screen_name,JSON.stringify(json.user));
  }
}
