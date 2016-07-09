#MiniHTTP

A simple webserver that implements a subset of HTTP/1.1. Processes are forked to handle GET or POST request. Also has not-fully-implemented CGI support.

### Arguments

__-p [PORT]__ Port number    
__-d [DOCROOT]__ Directory where files will be served from    
__-b [BACKLOG]__ Backlog for accept()    
__-l [LOGFILE]__ A path to a FILE that minihttp can write logs to, file will be created if none exists. Logs are of the form "\<method\>,\<requested file\>,\<GMT time\>"    
__-a__ run as a daemon     


Thanks to:    
[Beej's Guide to Network Programming](http://beej.us/guide/bgnet/output/print/bgnet_USLetter_2.pdf)    
[HTTP Parser](https://github.com/nodejs/http-parser)