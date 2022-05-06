#include <iostream>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <signal.h>

void worker();