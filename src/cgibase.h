//
//  cgibase.h
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-07.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#ifndef __MiniHTTP__cgibase__
#define __MiniHTTP__cgibase__

#include <stdio.h>

#include "request.h"

void exec_cgi(int sock, http_request *request, char *file_path);

#endif /* defined(__MiniHTTP__cgibase__) */
