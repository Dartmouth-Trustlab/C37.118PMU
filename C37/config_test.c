/*
 * config_text.c
 * Copyright (C) 2018 Prashant A <prashant.barca@gmail.com> 2018-04-27
 *
 */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<netinet/in.h>
#include "C37_tools.h"
int main()
{
	int result = -1;
    uint8_t buf[128];
    ssize_t read_bytes = read(stdin, buf, 128); 
    result = config(buf, read_bytes);
    return 0;
}

