#include "myshell.h"
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <termios.h>

jmp_buf jmpBuf; // 存放堆栈环境的变量

//验证账户
int verify_password(uid_t uid){
    seteuid(0);  //以root权限运行

    char ch;
    int len = 0; //循环变量
    int flag = 0;//计数三次的$

    char salt[20] = {0};
    char password[COMMAND_MAX] = {0};

    struct termios oldt,newt;
    struct spwd *pwd = getspnam(getpwuid(uid)->pw_name);

    for (int i = 0; flag != 3; i++){ //获得spwd中的salt
        salt[i] = pwd->sp_pwdp[i];
        if (salt[i] == '$'){
            flag++;
        }
        
    }

    while (1) //输入部分
    {
        tcgetattr(STDIN_FILENO,&oldt);
        newt = oldt;
        newt.c_lflag &=~(ECHO | ICANON); //标识相应标识位不会显
        tcsetattr(STDIN_FILENO,TCSANOW,&newt); //设置标识位

        ch = getchar();
        if (ch == '\n')
        {
            password[len] = '\0';
            tcsetattr(STDIN_FILENO,TCSANOW,&oldt);//恢复标识位
            break; 
        }

        if (ch == 8)
        {
            continue;
        }

        password[len] = ch;
        len++;
        tcsetattr(STDIN_FILENO,TCSANOW,&oldt);    
    }
    
    if(strcmp(pwd->sp_pwdp,crypt(password,salt)) == 0){ //密码正确
        seteuid(geteuid());
        return 1;
    }else //密码错误
    {
        seteuid(geteuid());
        return 0;
    }
}

//初始化控制节点
void initNode(CMD_NODE *cmdNode){

    if(cmdNode->ssu == 0){
        seteuid(getuid());   //获得实际用户的uid运行
        memset(cmdNode,0,sizeof(CMD_NODE));//内存空间初始化
    }else
    {
        memset(cmdNode,0,sizeof(CMD_NODE));
        cmdNode->ssu = 1;
    }   

    cmdNode->type = NORMAL; //类型为一般命令
} 

//得到输入的命令
void get_input(char cmd[COMMAND_MAX],char inCommand[IN_COMMAND],FHISNODE fhisNode,int su){
    char *str = NULL;

    strcat(inCommand,":"); //调用连接字符串函数

    do
    {
        if(su){
            printf("root@");
        }   
        str = readline(inCommand);

    }while(strcmp(str,"") == 0);
    

    if(strlen(str) == COMMAND_MAX){
       printf("error:Command is too long!\n");
       exit(-1); //命令过长退出程序
    }else{
        add_history(str);
        strcpy(cmd,str);
    }

    strcpy(fhisNode->history[fhisNode->hisSubs],str);
    fhisNode-> hisSubs = (fhisNode->hisSubs+1) % HIS_MAX;
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
        cmdNode->type = IN_COMMAND; 
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
            longjmp(jmpBuf,0); //非局部跳转
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
       cmdNode->type = IN_COMMAND; 
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
    get_type(cmdNode->arg,&(cmdNode->type));//指针类型

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
    int argflag = 0;
    int count[6] = {0};

    while(arg[argflag][0] != '\0'){

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

        argflag++;
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

    if (cmdNode->type == COMMAND_ERR) { 
        printf("你输入的命令有误，请检查后重试\n");
        return ;
    }

    if (cmdNode->type == IN_COMMAND){
        return;
    }
    
    pid = fork(); //创建子进程

    if (pid < 0){
        printf("创建子进程失败");
        exit(-1);
    }
    
    if (pid == 0){  //子进程

        if (cmdNode->type & IN_REDIRECT){
            int fid;
            if(cmdNode->type & IN_REDIRECT_APP){
                fid = open(cmdNode->infile,O_RDONLY|O_CREAT|O_APPEND,0644); 
                //只读|文件不存在自动创建|写入的数据以附加方式加入到文件后边
            }else
            {
               fid =  open(cmdNode->infile,O_RDONLY|O_CREAT,0644); 
            }
            dup2(fid,STDIN); //进行输入重定向
        }


        if (cmdNode->type & OUT_REDIRECT){ //如果有>>> 参数
            int fod;
            if (cmdNode->type & OUT_REDIRECT_APP){
                fod = open(cmdNode->outfile,O_WRONLY|O_CREAT|O_APPEND,0644);//???
            }else
            {
                fod = open(cmdNode->outfile,O_WRONLY|O_CREAT,0644);
            }
            dup2(fod,STDOUT); //进行输出重定向
        }


        if (cmdNode->type & HAVE_PIPE){
            pid_t pid2 = 0;
            int fd = 0;

            pid2 = fork(); //再创建一个进程，子进程的子进程运行管道前命令，子进程运行管道后命令
            if (pid2 < 0){
                printf("管道命令运行失败\n");
                exit(-1);
            }

            if (pid2 == 0){
                fd = open("/tmp/shellTemp",O_WRONLY|O_CREAT|O_TRUNC,0644);//???
                dup2(fd,STDOUT);
                execvp(cmdNode->argList[0],cmdNode->argList);
                printf("你输入的命令有误，请检查后重试\n");
                exit(1); // 防止子子进程的exec函数没有运行导致错误
            }

            if (waitpid(pid2,0,0) == -1){
               printf("管道命令运行失败\n");
               exit(-1);
            }

            fd = open("/tmp/shellTemp",O_RDONLY);
            dup2(fd,STDIN);
            execvp(cmdNode->argNext[0],cmdNode->argNext);
            printf("你输入的命令有误，请检查后重试\n");
            exit(1); // 防止子进程的exec函数没有运行导致错误
        }
   }   

   if (cmdNode->type & BACKSTAGE){ //???
       printf("子进程的pid为%d \n",pid);
       return ; 
   }
   
   if (waitpid(pid,0,0) == -1){
        printf("等待子进程失败\n");
        exit(-1);
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

    if (fscanf(fhisNode->fthis,"%d",&(fhisNode->hisSubs)) == EOF){
        fhisNode -> hisSubs = 0;
    }
    
    int subs = fhisNode -> hisSubs;

    for (int i = 0; i < HIS_MAX; i++){
        fscanf(fhisNode->fthis,"%s",fhisNode->history[i]);
    }

    for (int i = (fhisNode->hisSubs + 1)%HIS_MAX; i != fhisNode->hisSubs; i = (i+1)% HIS_MAX){
        if(fhisNode->history[i][0] == '\0'){
            continue ;
        }
        add_history(fhisNode->history[i]);
    }
    add_history(fhisNode->history[fhisNode->hisSubs]);
    
    fclose(fhisNode->fthis);
    fhisNode->fthis = fopen(temp,"w"); //???
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

// 处理一些命令特有事情
void other_work(CMD_NODE *cmdNode){
    int argIndex = 0;
    int argListIndex = 0;
    int nextIndex = 0;
    int isNext = 0;
    int type = cmdNode -> type; 

    if (type & COMMAND_ERR || type & IN_COMMAND){
        return ;
    }
    
    while(cmdNode->arg[argIndex][0] != '\0')
    {
        if (type & BACKSTAGE){  //有后台运行命令
            cmdNode->argList[argListIndex] = NULL; //屏蔽后台运行
            break ; //后台运行符后边的参数不加入到artList中
        }

        if (strncmp(cmdNode->argList[argIndex],"<",1) == 0 || strncmp(cmdNode->argList[argIndex],">",1)){ //判断是否为重定向输入或输出
            
            argIndex++; //跳过> >> < <<

            if (cmdNode->argList[argIndex-1][0] == '>'){
                strcpy(cmdNode->outfile,cmdNode->argList[argIndex]);
            }else
            {
                strcpy(cmdNode->infile,cmdNode->argList[argIndex]);
            }
            
            argIndex++; //跳过路径
            continue;
        }
        
        if (strcmp(cmdNode->arg[argIndex],"|") == 0){ //判断是否为管道命令
            argIndex++;
            isNext = 1;
            continue;//？to
        }
        
        if (isNext){ //是管道命令,开始给Next数组中存放数据
            cmdNode -> argNext[nextIndex++] = cmdNode -> arg[argIndex++];
        }else{
            cmdNode -> argNext[argListIndex++] = cmdNode -> arg[argIndex++];
        }      
    }
}

void sigint(int signo) {
    printf("\n");
    longjmp(jmpBuf, 0);
}

//综合所有函数的最终体
void shell(void){
    CMD_NODE cmdNote = {0};
    HISNODE hisNode = {0};

    get_his(&hisNode); //得到历史命令
    chdir(getenv("HOME")); //初始工作目录在用户的HOME目录

    struct sigaction act;
    act.sa_handler = sigint;
    act.sa_flags = SA_NOMASK;
    sigaction(SIGINT,&act,NULL);//安装信号屏蔽函数

    while (1)
    {
        setjmp(jmpBuf); //跳转到初始化目录
        initNode(&cmdNote);
        get_input(cmdNote.cmd,getcwd(cmdNote.workpath,FILE_PATH_MAX),&hisNode,cmdNote.ssu);
        explain_input(&cmdNote,hisNode);
        other_work(&cmdNote);
        run(&cmdNote);
    }
}
