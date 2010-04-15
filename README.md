## Meguro 

Meguro is a map/reduce engine built on Spidermonkey inspired by the internal
map/reduce engine inside of CouchDB, but meant to be database agnostic, running
with multiple input types. It currently supports line by line input and tokyo
cabinet hash databases as input types, with an interface for more in the future.

It is called Meguro after a neighborhood in Tokyo, since the core datastore
that is used as the map and reduce storage is Tokyo Cabinet.  The Tokyo Cabinet
key value store was also the first supported input type.

Please check out the [Meguro home page](http://www.sevenforge.com/meguro 
"Meguro Home Page") for more details.
