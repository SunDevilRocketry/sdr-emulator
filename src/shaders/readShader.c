#include "shaders/readShader.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>

const char* readShaderSource
    (
    const char* path
    ) 
{

int shaderfd = open(path, O_RDONLY);

if (shaderfd == -1) {
    fprintf(stderr, "Failed to read shader source %s with errno %d\n", path, errno);
    return NULL;
}

struct stat stat;
int fstatStatus = fstat(shaderfd, &stat);

if (fstatStatus == -1) {
    fprintf(stderr, "Failed to get stat of shader file %s with errno %d\n", path, errno);
    return NULL;
}


char* data = malloc(sizeof(char) * stat.st_size + 1);
if (data == NULL) {
    fprintf(stderr, "Failed to allocate buffer for shader source %s\n", path);
}

ssize_t bytesRead = read(shaderfd, data, stat.st_size);
data[stat.st_size] = '\0'; // null term

if (bytesRead != stat.st_size || bytesRead == -1) {
    fprintf(stderr, "Bad read of shader file %s with errno %d\n", path, errno);
}

return data;
}

