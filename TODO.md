TODO

Receive messages:
- error checking on malloc/realloc, specifically for running out of memory
- double size with realloc instead of increasing by buf_size
- better way to know the end of sockstream, currently will hang when total bytes receiving are a multiple of 512

Parse Requests:
- create struct for http requests (header field/values, body, etc.)
