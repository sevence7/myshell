#ifndef _SHELL_H_

#define _SHELL_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>


#define  normal              0  //一般的命令
#define  out_redirect        1  //输出重定向
#define  in_redirect         2  //输入重定向
#define  have_pipe           3  //命令中有管道
#endif  // !_SHELL_H_