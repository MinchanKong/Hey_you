#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmpx.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <memory.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <grp.h>

struct COMMAND { 
        char* name;
        char* desc;
        bool ( *func )( int argc, char* argv[] ); 
};

pid_t child=-1;
int status;
int list_CHLD[100];
int count=0;

bool cmd_cd( int argc, char* argv[] );          
bool cmd_exit( int argc, char* argv[] );        
bool cmd_help( int argc, char* argv[] );      
bool cmd_myfinger(int argc, char* argv[]);   
bool cmd_mkdir( int argc, char* argv[] );	
bool cmd_rmdir( int argc, char* argv[] );
bool cmd_ls(int argc, char* argv[]);	
bool cmd_cat( int argc, char* argv[] );
bool cmd_pwd(int argc, char* argv[]);

struct COMMAND  builtin_cmds[] =
{
    { "cd",    "change directory",                    cmd_cd   },
    { "exit",   "exit this shell",                        cmd_exit  },
    { "quit",   "quit this shell",                        cmd_exit  },
    { "help",  "show this help",                      cmd_help },
    { "?",      "show this help",                      cmd_help },
    { "myfinger", "show user information", cmd_myfinger },
    { "mkdir", "make directory", cmd_mkdir },
    { "rmdir", "delete directory", cmd_rmdir },
    { "ls", "show all files in this directory" ,cmd_ls },
    { "cat", "show contents of the file", cmd_cat },
    { "pwd", "show the location of the current directory" ,cmd_pwd }
};

void clearbuffer(void)
{
	while(getchar() != '\n');
}

void handler(int sig)
{
	if(child!=0)
	{
		switch(sig)
		{
			case SIGINT:
				printf("Ctrl + c SIGINT\n");
				break;
			case SIGTSTP:
				printf("Ctrl + z SIGTSTP\n");
				kill(0, SIGCHLD);
				break;
			case SIGCONT:
				printf("Restart rs SIGCONT\n");
				break;
		}
	}
}

bool cmd_pwd (int argc, char *argv[])
{
	char wd[BUFSIZ];
	
	getcwd(wd,BUFSIZ);
	printf("%s\n",wd);
	
	return true;
}

bool cmd_cd( int argc, char* argv[] ) //cd : change directory
{ 
        if( argc == 1 )
                chdir( getenv( "HOME" ) );
        else if( argc == 2 ){
            if( chdir(argv[1])!=0 )
                printf( "No directory\n" );
				}
        else
            printf( "USAGE: cd [dir]\n" );
		return true;
}

bool cmd_exit( int argc, char* argv[] )
{
       return false; 
}

bool cmd_cat(int argc, char *argv[])
{
	char ch;
	int fd;
	
	if(argc!=2)
	{
		printf("argument error\n");
		return true;
	}
	
	fd=open(argv[1], O_RDONLY);
	
	while(read(fd,&ch,1))
		write(1,&ch,1);
	close(fd);
	return true;
}

bool cmd_help( int argc, char* argv[] ) 
{ 
        int i;
        for( i = 0; i < sizeof( builtin_cmds ) / sizeof( struct COMMAND ); i++ )
        {
                if( argc == 1 || strcmp( builtin_cmds[i].name, argv[1] ) == 0 )
                        printf( "%-10s: %s\n", builtin_cmds[i].name, builtin_cmds[i].desc );
        }
		return true;
}

bool cmd_myfinger( int argc, char* argv[] ) {
	struct utmpx *utx;
	struct passwd *pw;
	struct tm *tminfo;
	char buf[100];
	int n;
	int flag_a=0, flag_b=0, flag_c=0;
	
	if(argc==2) {
		n=*(argv[1]+1);
		switch(n) {
			case 'a':
				flag_a=1;	
				break;
			case 'b':
				flag_b=1;	
				break;
			case 'c':
				flag_c=1;	
				break;
			default :
				printf( "USAGE: myfinger [option]\n" );
				printf( "Available option : a,b,c\n");
				printf( "USAGE SAMPLE : myfinger -a\n");
				break;
		}
	}

	if(argc==1 || flag_a) {
	printf("%-10s %-12s %-11s %-11s %-18s\n","Login", "Name", "TTY", "When", "Where"); 

	while((utx=getutxent()) != NULL ) {
		if (utx->ut_type != USER_PROCESS) 
			continue;

		pw=getpwnam(utx->ut_user);
		tminfo=localtime(&(utx->ut_tv.tv_sec));
		strftime(buf, sizeof(buf), " %a %H:%M " ,tminfo);

		printf("%-10s %-12s %-10s %-12s %-18s\n",utx->ut_user, pw->pw_comment, utx->ut_line, buf, utx->ut_host);
		}
	endutxent();
	}

	if(flag_b) {
	while((utx=getutxent()) != NULL ) {
		if (utx->ut_type != USER_PROCESS) 
			continue;

		pw=getpwnam(utx->ut_user);
		tminfo=localtime(&(utx->ut_tv.tv_sec));
		strftime(buf, sizeof(buf), "%G년 %m월 %d일 %H:%M:%S" ,tminfo);

		printf("Login name: %-20s In real life: %-12s\n",utx->ut_user, pw->pw_comment);
		printf("Directory: %-21s Shell: %-15s\n",pw->pw_dir,pw->pw_shell);
		printf("On since %s on %s from %s\n\n", buf, utx->ut_line, utx->ut_host);  
		}
	endutxent();
	}

	if(flag_c) {
		DIR *dir;
		struct dirent *dent;
		struct stat file_info;
		int n;
		FILE *wfp;		

		chdir(getenv("HOME"));
		if((dir=opendir("./"))==NULL) {
			perror("opendir");
			exit(1);
		}

		while(dent=readdir(dir)) {
			if(dent->d_name[0]=='.') continue;
			else {
				if((stat(dent->d_name,&file_info))==-1) {
					perror("Error : ");
					exit(1);
				}
			
				if(S_ISDIR(file_info.st_mode)) {
					n=strcmp(dent->d_name,"finger_result");
					if(n==0) {
						printf("finger_result 디렉토리가 존재합니다.\n");
						break;	
					}
				}
				mkdir("finger_result",0755);
			}
		}
		chdir("finger_result");

		while((utx=getutxent()) != NULL ) {
			if (utx->ut_type != USER_PROCESS) continue;

			pw=getpwnam(utx->ut_user);
			tminfo=localtime(&(utx->ut_tv.tv_sec));
			strftime(buf, sizeof(buf), "%g%m%d" ,tminfo);

			if((dir=opendir("./"))==NULL) {
				perror("opendir");
				exit(1);
			}

			while(dent=readdir(dir)) {
				if((stat(dent->d_name,&file_info))==-1) {
					perror("Error : ");
					exit(1);
				}
		
				if(S_ISDIR(file_info.st_mode)) {
					n=strcmp(dent->d_name,buf);
					if(n==0) {
						printf("%s 디렉토리가 존재합니다.\n",buf);
						break;	
					}
				}
			}
			if(n!=0) mkdir(buf,0755);
			
			chdir(buf);
			memset(buf,NULL,100);	
			strftime(buf, sizeof(buf), "%G년 %m월 %d일 %H:%M:%S" ,tminfo);
		
			if((dir=opendir("./"))==NULL) {
				perror("opendir");
				exit(1);
			}

			while(dent=readdir(dir)) {
				if((stat(dent->d_name,&file_info))==-1) {
					perror("Error : ");
					exit(1);
				}
		
				n=strcmp(dent->d_name,utx->ut_user);
				if(n==0) {
					if((wfp=fopen(utx->ut_user, "a")) == NULL) {
						perror("fopen: ");
						break;	
					}	
				}
			}
			
			if(n!=0) {
				if((wfp=fopen(utx->ut_user, "w")) == NULL) {
					perror("fopen: ");
					exit(1);	
				}
			}

			fprintf(wfp,"Login name : %s\n",utx->ut_user);
			fprintf(wfp,"In real life : %s\n",pw->pw_comment);
			fprintf(wfp,"Directory : %s\n",pw->pw_dir);
			fprintf(wfp,"Shell : %s\n",pw->pw_shell);
			fprintf(wfp,"Time : %s\n",buf);
			fprintf(wfp,"TTY : %s\n",utx->ut_line);
			fprintf(wfp,"IP : %s\n",utx->ut_host);
			fprintf(wfp,"-------------------------------------\n");
		 
			chdir("..");
		}
		endutxent();
		fclose(wfp);
	}
	return true;
}

bool cmd_mkdir( int argc, char* argv[] ) {
	DIR *dir;
	struct dirent *dent;
	struct stat file_info;
	int n;

	if(argc!=2) {
		printf( "USAGE: mkdir [dir]\n" );
	}

	char wd[BUFSIZ];

	getcwd(wd,BUFSIZ);
	chdir(wd);
	if((dir=opendir("./"))==NULL) {
		perror("opendir");
		exit(1);
	}

	while(dent=readdir(dir)) {
		if(dent->d_name[0]=='.') continue;
		else {
			if((stat(dent->d_name,&file_info))==-1) {
				perror("Error : ");
				exit(1);
			}
			
			if(S_ISDIR(file_info.st_mode)) {
				n=strcmp(dent->d_name,argv[1]);
				if(n==0) {
					printf("mkdir: '%s' 디렉토리를 만들 수 없습니다: 파일이 존재함\n",argv[1]);
					return true;
				}
			}
		}
	}
	mkdir(argv[1],0755);
	return true;
}

bool cmd_rmdir( int argc, char* argv[] ) {
	char wd[BUFSIZ];

	getcwd(wd,BUFSIZ);
	chdir(wd);

	if(rmdir(argv[1])==-1) {
		printf("rmdir: failed to remove '%s': 해당 파일이나 디렉토리가 없음\n",argv[1]);
	}
	return true;
}

void rwx(file_mode) {
	if(S_ISDIR(file_mode)) {
		printf("d");
	}
	
	else printf("-");
	
	if(file_mode&S_IRUSR) {
		printf("r");
	}
	else printf("-");
	
	if(file_mode&S_IWUSR) {
		printf("w");
	}
	else printf("-");
	
	if(file_mode&S_IXUSR) {
		printf("x");
	}
	else printf("-");
	
	if(file_mode&S_IRGRP) {
		printf("r");
	}
	else printf("-");
	
	if(file_mode&S_IWGRP) {
		printf("w");
	}
	else printf("-");
	
	if(file_mode&S_IXGRP) {
		printf("x");
	}
	else printf("-");
	
	if(file_mode&S_IROTH) {
		printf("r");
	}
	else printf("-");
	
	if(file_mode&S_IWOTH) {
		printf("w");
	}
	else printf("-");
	
	if(file_mode&S_IXOTH) {
		printf("x");
	}
	else printf("-");
	
	printf(" ");
}
		
bool cmd_ls( int argc, char* argv[] ) {
	int flag_l=0;
	int n;
	
	if(argc==2) {
		n=*(argv[1]+1);
		switch(n) {
			case 'l':
				flag_l=1;	
				break;
			default :
				printf( "USAGE: myfinger [option]\n" );
				printf( "Available option : l\n");
				printf( "USAGE SAMPLE : ls -l\n");
				break;
		}
	}

	if(argc==1) {
		DIR *dir;
		struct dirent *dent;

		if((dir=opendir("./"))==NULL) {
			perror("opendir");
			exit(1);
		}

		while(dent=readdir(dir)) {
			if(dent->d_name[0]=='.') continue;
			else {
				if(dent->d_ino !=0) printf("%s ", dent->d_name);
			}
		}
		printf("\n");
		closedir(dir);
	}
	
	if(flag_l) {
		struct passwd *pw;
		DIR *dirp;
		struct dirent *dentry;
		struct group *group;
		struct stat file_info;
		mode_t file_mode;
		struct tm *tminfo;
		int return_stat;
		char buf[100];
		
		if((dirp=opendir("./"))==NULL) {
			exit(1);
		}
		
		while(dentry=readdir(dirp)) {
			if((return_stat=stat(dentry->d_name, &file_info))==-1) {
				perror("Error : ");
				exit(0);
			}
			
			if(dentry->d_name[0]=='.') {
				continue;
			}
			
			else {
				file_mode=file_info.st_mode;
				rwx(file_mode);
				printf(" %d ", file_info.st_nlink);
				
				pw=getpwuid(file_info.st_uid);
				group=getgrgid(file_info.st_gid);
				printf("%s", pw->pw_name);
				printf(" %s ", group->gr_name);
				
				printf(" %d ",file_info.st_size);
				
				tminfo=localtime(&(file_info.st_mtime));
				strftime(buf,sizeof(buf)," %m월 %d일 %H:%M ",tminfo);
				printf(" %s ",buf);
				
				if(dentry->d_ino !=0) {
					printf(" %s\n",dentry->d_name);
				}
			}
			closedir(dirp);
		}
	} 
	return true;
}

int tokenize( char* buf, char* delims, char* tokens[], int maxTokens ){
		int token_count = 0;
		char* token;	
		token = strtok( buf, delims );	
		while( token != NULL && token_count < maxTokens ){
				tokens[token_count]=token;
				token_count++;
				token=strtok(NULL, delims);
		}
        tokens[token_count] = NULL;
        return token_count;
}

bool run( char* line ){
        char delims[] = " \r\n\t";
        char* tokens[128];
        int token_count;
        int i;

        for(i=0;i<strlen(line);i++){ 
                if(line[i] == '&'){
                        line[i]='\0';
                        break;
                }
        }
		
        token_count = tokenize( line, delims, tokens, sizeof( tokens ) / sizeof( char* ) );
		if( token_count == 0 )
                return true;
        for( i = 0; i < sizeof( builtin_cmds ) / sizeof( struct COMMAND ); i++ ){
                if( strcmp( builtin_cmds[i].name, tokens[0] ) == 0 )
                        return builtin_cmds[i].func( token_count, tokens );
        }
	
	child=fork();
	if(child=0){
		if(signal(SIGINT,handler)==SIG_ERR){
			printf("SIGINT Error\n");
			_exit(1);
		}
		execvp(tokens[0], tokens);
		printf("No such file\n");
		_exit(0);
	}
	
	else if(child<0){
		printf("Failed to fork()!");
		_exit(0);
	}
	
	else {
		waitpid(child,&status,WUNTRACED);
	}
	return true;
}

int main(){
	char line[1024];
	char wd[BUFSIZ];
	char c;
	int n=0;
	
	signal(SIGINT, handler);
	signal(SIGTSTP, handler);

    while( 1 )
    {
		n=0;
		memset(line, '\0', sizeof(line));
		getcwd(wd,BUFSIZ);
       	printf( "[%s] $ ", wd);
		
		while(c=fgetc(stdin))
		{
			if((c<=26)&&(c!=8))
				break;
			switch(c)
			{
				case 8:
					if(n==0) break;
					
					fputc('\b', stdout);
					fputc(' ', stdout);
					fputc('\b', stdout);
					line[--n]=(char)0;
					break;
					
				default :
					line[n++]=(char)c;
					break;
			}
		}
		line[n]='\0';
		
	   	if( run( line ) == false ) break;

		fflush(stdin);
		memset(wd,NULL,BUFSIZ);	
	}
	return 0;
}
