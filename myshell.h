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
#include <setjmp.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <termios.h>

#define COMMAND_TYPE int  // 命令类别

/* 命令类别码 */
#define COMMAND_ERR         0    
#define NORMAL              1    //一般的命令
#define BACKSTAGE           2    //后台运行
#define HAVE_PIPE           4    //管道命令
#define IN_REDIRECT         8    //输入重定向
#define IN_REDIRECT_APP     24   //
#define OUT_REDIRECT        32   //输出重定向
#define OUT_REDIRECT_APP    96   //
#define IN_COMMAND          256  //    

/* 输入命令及文件名字数限制 */
#define ARGLIST_NUM_MAX 32  // 最多有32个命令
#define COMMAND_MAX 256     // 一个命令最多有256个字符
#define FILE_PATH_MAX 256   // 文件路径最多有256个字符
#define HIS_MAX 100  // 最多保存100个命令

/* 标准输入输出流 */
// 此处的 0 和 1 是在Linux文件描述符中规定的
#define STDIN   0   // 标准输入流
#define STDOUT  1   // 标准输出流

/* 创建命令控制节点 */
typedef struct CommandNode{
    COMMAND_TYPE type;     //用户输入的命令类型
    char  cmd[COMMAND_MAX]; //用户输入的命令
    char  arg[ARGLIST_NUM_MAX][COMMAND_MAX];//存放所有分解命令
    char *argList[ARGLIST_NUM_MAX+1]; // 对用户输入的命令进行解析之后的字符串数组 , +1 是为了execvp操作
    char *argNext[ARGLIST_NUM_MAX];//管道之后命令的字符串数组
    char infile[FILE_PATH_MAX]; //存放输入重定向文件路径
    char outfile[FILE_PATH_MAX];//存放输出重定向文件路径
    char workpath[FILE_PATH_MAX];//存放工作目录的数组
    char ssu;//超级用户
}CMD_NODE;

typedef struct HistoryNode
{
    char history[HIS_MAX][COMMAND_MAX];//存放历史命令
    int  hisSubs; //历史队列的下标
    FILE *fthis;
}HISNODE,*FHISNODE;

int verify_password(uid_t uid);  //验证账户
void initNode(CMD_NODE *cmdNode); //初始化控制节点
void get_input(char cmd[COMMAND_MAX],char inCommand[IN_COMMAND],FHISNODE fhisNode,int su);//得到输入的命令
void explain_input(CMD_NODE *cmdNode,HISNODE hisNode);//对输入命令进行解析
void in_arr(char arg[ARGLIST_NUM_MAX][COMMAND_MAX],char *cmd);//将命令放入arr中
void get_type(char arg[ARGLIST_NUM_MAX][COMMAND_MAX],COMMAND_TYPE *type);//将命令分类
void run(CMD_NODE *cmdNode);//按类别执行命令
void put(CMD_NODE *cmdNode,HISNODE hisNode);  // 输出控制节点的信息
void get_his(FHISNODE fhisNode); //得到历史命令文件
void save_his(HISNODE hisNode); //保存历史命令文件
void other_work(CMD_NODE *cmdNode); // 处理一些命令特有事情
void sigint(int signo);//SIGINT信号屏蔽函数
void shell(void); //综合所有函数的最终体

#endif  // !_SHELL_H_
