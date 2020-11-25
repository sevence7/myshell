#include "myshell.h"
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <termios.h>

jmp_buf jmpBuf; // 存放堆栈环境的变量

//验证账户
int verify_password(uid_t uid){
    seteuid(0);  //以root权限运行


}

//初始化控制节点
void initNode(CMD_NODE *cmdNode){

    if(cmdNode->ssu == 0){
        seteuid(getuid());   //获得实际用户的uid运行
        memset(cmdNode,0,sizeof(Cmd_Node));//内存空间初始化
    }else
    {
        memset(cmdNode,0,sizeof(Cmd_Node));
        cmdNode->ssu = 1;
    }   

    cmdNode->type = NORMAL； //类型为一般命令
} 

//得到输入的命令
void get_input(char cmd[COMMAND_MAX],char inCommand[IN_COMMAND],FHISNODE fhisNode,int su){
    char *str = NULL;

    strcat(inCommand,":"); //调用连接字符串函数

    do
    {
        if(ssu){
            printf("root@");
        }   // to do:？？
        str = readline(inCommand);

    }while(strcmp(str,"") == 0)
    

    if(strlen(str) == COMMAND_MAX){
       printf("error:Command is too long!\n")
       exit(-1); //命令过长退出程序
    }else{
        //add_history(str);
        strcpy(cmd,str);
    }

    strcpy(fhisNode->history[fhisNode->hisSubs],str);
    fhisNode->history = (fhisNode->hisSubs+1) % HIS_MAX;
}

//对输入命令进行解析,判断是否为内部命令，否则放入arg数组中
void explain_input(CMD_NODE *cmdNode,HISNODE hisNode){

    if (strcmp(cmdNode->cmd,"exit") == 0)
    {
        if(cmdNode->ssu){
            seteuid(getuid()); //以实际用户权限运行
            cmdNode->ssu = 0;
            return ;
        }
        save_his(hisNode);
        exit(0);
    }

    if (strcmp(cmdNode->cmd,"pause") == 0)
    {
        cmdNode->type = IN_COMMAND; //不懂？？
        char ch;
        while ((ch = getchar()) == '\r')
        {
            return;
        }
    }

    if (strcmp(cmdNode->cmd,"su") == 0)
    {
        if(verify_password(getuid())){
            cmdNode->ssu = 1;
            seteuid(0);
            longjmp(jmpBuf,0); //非局部跳转？？
        }else
        {
            printf("password error!\n");
            longjmp(jmpBuf,0);
        }
        
    }

    if (strncmp(cmdNode->cmd,"sudo",4) == 0)
    {
        if(strlen(cmdNode->cmd) == 4){
            printf("format error！\n"); //格式错误
            return;
        }else
        {
            if(verify_password(getuid())){

                if (seteuid(0) == -1)
                {
                    perror("sudo error");//将输出sudo error: No such file or directory
                }
                
                strcpy(cmdNode->cmd,cmdNode->cmd + 5);
            }else
            {
                printf("password error!\n");
                longjmp(jmpBuf,0);
            }           
        }
   }
    
   if (strncmp(cmdNode->cmd,"cd",2) == 0)
   {
       cmdNode->type = IN_COMMAND; //？？
       in_arr(cmdNode->arg,cmdNode->cmd);

       //命令为cd或者cd ~
       if(strcmp(cmdNode->arg[1],"~") == 0 || strlen(cmdNode->arg[0]) == 0){
           chdir(getenv("HOME")); //回到主目录
           return ;
       }

       if (chdir(cmdNode->arg[1]) == -1){  //执行失败
           printf("%s",cmdNode->arg[1]);
           perror(" ");
       }
       longjmp(jmpBuf,0);
       return ;
   }

    //放入arr，获取命令类型
    in_arr(cmdNode->arg,cmdNode->cmd);
    get_type(cmdNode->arg,&(cmdNode->type));//指针？

}

//将命令放入arr中
void in_arr(char arg[ARGLIST_NUM_MAX][COMMAND_MAX],char *cmd){

    int argFlag = 0;
    int flag = 0;
    while (*cmd != '\0')
    {
        if (*cmd == ' '){  //遇到空格换下一个数组
            argFlag++;
            flag = -1;
        }else
        {
            arg[argFlag][flag] = *cmd;
        }
        cmd++;
        flag++;
    }   
}

//将命令分类
void get_type(char arg[ARGLIST_NUM_MAX][COMMAND_MAX],COMMAND_TYPE *type){
    int argFlag = 0;
    int count[6] = {0};

    while(arg[argFlag][0] != '\0'){

        if (strcmp(arg[argflag],"|") == 0){ //将命令类型置为管道命令
            *type |= HAVE_PIPE; //???
            count[0]++;
        }
        
        if (strcmp(arg[argflag],">") == 0){ //将命令类型置为输出重定向
            *type |= OUT_REDIRECT;
            count[1]++;
        }

        if (strcmp(arg[argflag],"<") == 0){ //将命令类型置为输入重定向
            *type |= IN_REDIRECT;
            count[2]++;
        }

        if (strcmp(arg[argflag],">>") == 0){ //将命令类型置为输出重定向（尾加）
            *type |= OUT_REDIRECT_APP;
            count[3]++;
        }

        if (strcmp(arg[argflag],"<<") == 0){ //将命令类型置为输入重定向（尾加）
            *type |= IN_REDIRECT_APP;
            count[4]++;
        }

        if (strcmp(arg[argflag],"&") == 0){ //将命令类型置为后台运行
            *type |= BACKSTAGE;
            count[5]++;
        }

        argFlag++;
    }

    //检验类型是否重复定义
    for (int i = 0; i < 6; i++)
    {
        if (count[i] > 1)
        {
            printf("error:have too many args\n");
            exit(-1);
            *type =COMMAND_ERR;
        }
        
    }
    

}

//执行命令
void run(CMD_NODE *cmdNode){
    pid_t pid;
    pid = 0;

    if (cmdNode->type | COMMAND_ERR) //认为应该写或，不是&？？？
    {
        /* code */
    }
    
}

// 输出控制节点的信息
void put(CMD_NODE *cmdNode,HISNODE hisNode){

    int i;
    printf("类型：%d\n",cmdNode->type);
    printf("命令：%s\n",cmdNode->cmd);

    printf("argList:\n");
    for (i = 0;cmdNode->argList[i] != NULL; i++)
    {
        printf("  [%d]:%s\n",i,cmdNode->argList[i]);
    }

    printf("argNext:\n");
    for (i = 0;cmdNode->argNext[i] != NULL; i++)
    {
        printf("  [%d]:%s\n",i,cmdNode->argNext[i]);
    }

    printf("infile:%s\n",cmdNode->infile);
    printf("outfile:%s\n",cmdNode->outfile);
    
    printf("历史命令\n");
    for (i = 0;hisNode.history[i] != NULL; i++)
    {
        printf("%d  %s\n",hisNode.hisSubs,hisNode.history[i]);
    }

} 

//得到历史命令文件
void get_his(FHISNODE fhisNode){

    char temp[COMMAND_MAX];
    strcpy(temp,getenv("HOME"));
    strcat(temp,"/.history");
    fhisNode->fthis = fopen(temp,"a+");

}

//保存历史命令文件
void save_his(HISNODE hisNode){
    fprintf(hisNode.fthis,"%d\n",hisNode.hisSubs);

    for (int i = 0; i < HIS_MAX; i++)
    {
        fprintf(hisNode.fthis,"%s\n",hisNode.history[i]);
    }
    fclose(hisNode.fthis);
}