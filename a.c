#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmpx.h>
#include <stdbool.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <siginfo.h>
#include <time.h>
#include <memory.h>

struct COMMAND{ // 커맨드 구조체
        char* name;
        char* desc;
        bool ( *func )( int argc, char* argv[] ); // 함수포인터. 사용할 함수들의 매개변수를 맞춰줌
};

pid_t child=-1;                                            // 자식프로세스 pid 저장 전역변수
int status;                                                  // 프로세스 status

bool cmd_cd( int argc, char* argv[] );          //cd 명령어
bool cmd_exit( int argc, char* argv[] );        //exit, quit 명령어
bool cmd_help( int argc, char* argv[] );       //help 명령어
bool cmd_myfinger(int argc, char* argv[]);     //myfinger 명령어 

struct COMMAND  builtin_cmds[] =
{
    { "cd",    "change directory",                    cmd_cd   },
    { "exit",   "exit this shell",                        cmd_exit  },
    { "quit",   "quit this shell",                        cmd_exit  },
    { "help",  "show this help",                      cmd_help },
    { "?",      "show this help",                      cmd_help },
    { "myfinger", "show user information", cmd_myfinger }
};

bool cmd_cd( int argc, char* argv[] ){ //cd : change directory
        if( argc == 1 )
                chdir( getenv( "HOME" ) );
        else if( argc == 2 ){
		//	chdir(argv[1]);
            if( chdir(argv[1])!=0 )
                printf( "No directory\n" );
				}
        else
            printf( "USAGE: cd [dir]\n" );
		return true;
}

bool cmd_exit( int argc, char* argv[] ){
       return false; 
}

bool cmd_help( int argc, char* argv[] ){ // 명령어 출력
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
	int n=0;
	char *optarg;
	int optind;
		
	while((n=getopt(argc, argv, "ab")) != -1) {
		switch(n) {
			case 'a':
				printf("Option : a\n");	
				break;
			case 'b':
				printf("Option : b\n");
				break;
			default :
				printf("Nothing : no\n");
				break;
		}
	}

	if(argc==1) {
	printf("%-10s %-12s %-11s %-11s %-18s\n","Login", "Name", "TTY", "When", "Where"); 

	while((utx=getutxent()) != NULL ) {
		if (utx->ut_type != USER_PROCESS) 
			continue;

		pw=getpwnam(utx->ut_user);
		tminfo=localtime(&(utx->ut_tv.tv_sec));
		strftime(buf, sizeof(buf), " %a %H:%M " ,tminfo);

		printf("%-10s %-12s %-10s %-12s %-18s\n",utx->ut_user, pw->pw_comment, utx->ut_line, buf, utx->ut_host);
		}
	//setutxent();
	endutxent();
	}
}

int tokenize( char* buf, char* delims, char* tokens[], int maxTokens ){
        int token_count = 0;
        char* token;
        token = strtok( buf, delims );
        while( token != NULL && token_count < maxTokens ){
                tokens[token_count] = token;
                token_count++;
                token = strtok( NULL, delims );
        }
        tokens[token_count] = NULL;
        return token_count;
}

bool run( char* line ){
        char delims[] = " \r\n\t";
        char* tokens[128];
        int token_count;
        int i;

        for(i=0;i<strlen(line);i++){ //background 실행은 wait하지 않는다.
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
		
		/*
        child = fork();
        if( child == 0 ){
                execvp( tokens[0], tokens );
                printf( "No such file\n" );
                _exit( 0 );
        }
        else if( child < 0 )
        {
            printf( "Failed to fork()!" );
            _exit( 0 );
        }
		else { 
            waitpid(child, &status, WUNTRACED );
        }
        return true;
		*/	
}

int main(){
        char line[1024];
		char wd[BUFSIZ];

        while( 1 )
        {
			getcwd(wd,BUFSIZ);
            printf( "[%s] $ ", wd);
            fgets( line, sizeof( line ) - 1, stdin );
            if( run( line ) == false )
				break;
			memset(wd,NULL,BUFSIZ);	
		}
		return 0;
}
