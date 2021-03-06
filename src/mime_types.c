//
//  mime_types.c
//  MiniHTTP
//
//  Created by Colin Luoma on 2016-07-06.
//  Copyright (c) 2016 Colin Luoma. All rights reserved.
//

#include "mime_types.h"

#ifndef ARRAY_SIZE
# define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

typedef struct {
    char *ext; // File extension
    char *med; // Media type
} http_mime;

http_mime http_mime_types[] = {
    {"txt", "text/plain"},
    {"jpg", "image/jpg"},
    {"gif", "image/gif"},
    {"png", "image/png"},
    {"html", "text/html"},
    {"htm", "text/html"},
    {"css", "text/css"},
    {"js", "text/javascript"},
    {"zip", "application/zip"}
};

char *
mime_from_ext(char *ext)
{
    for (size_t i = 0; i < ARRAY_SIZE(http_mime_types); i++)
    {
        if (strcasecmp(ext, http_mime_types[i].ext) == 0)
        {
            return http_mime_types[i].med;
        }
    }
    
    return "text/plain";
}
