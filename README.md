#MiniHTTP

A simple webserver that implements only basic HTTP. Processes are forked to handle requests GET or POST request. Also has not-fully-implemented CGI support.

### Arguments

__-p [PORT]__ port number    
__-d [DOCROOT]__ directory where files will be served from    
__-b [BACKLOG]__ backlog for accept()    
__-l [LOGFILE]__ a path to a FILE that minihttp can write logs to
 - logs are of the form "<method>,<requested file>,<GMT time>"
__-a__ run as a daemon     


Thanks to:    
[Beej's Guide to Network Programming](http://beej.us/guide/bgnet/output/print/bgnet_USLetter_2.pdf)    
[HTTP Parser](https://github.com/nodejs/http-parser)