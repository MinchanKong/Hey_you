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

struct COMMAND{ // Ŀ�ǵ� ����ü
        char* name;
        char* desc;
        bool ( *func )( int argc, char* argv[] ); // �Լ�������. ����� �Լ����� �Ű������� ������
};

bool cmd_cd( int argc, char* argv[] );          //cd ��ɾ�
bool cmd_exit( int argc, char* argv[] );        //exit, quit ��ɾ�
bool cmd_help( int argc, char* argv[] );       //help ��ɾ�
bool cmd_myfinger(int argc, char* argv[]);     //myfinger ��ɾ� 
bool cmd_mkdir( int argc, char* argv[] );	// mkdir ��ɾ�
bool cmd_rmdir( int argc, char* argv[] );	// rmdir ��ɾ�
bool cmd_ls(int argc, char* argv[]);	// ls ��ɾ�

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
    { "ls", "show all files in this directory" ,cmd_ls }
};

bool cmd_cd( int argc, char* argv[] ){ //cd : change directory
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

bool cmd_exit( int argc, char* argv[] ){
       return false; 
}

bool cmd_help( int argc, char* argv[] ){ // ��ɾ� ���
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
		strftime(buf, sizeof(buf), " %G�� %m�� %d��  %H:%M:%S" ,tminfo);

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
						printf("finger_result ���丮�� �����մϴ�.\n");
						break;	
					}
				}
				mkdir("finger_result",0755);
			}
		}
		chdir("finger_result");

		while((utx=getutxent()) != NULL ) {
			if (utx->ut_type != USER_PROCESS) {
				continue;
			}

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
						printf("%s ���丮�� �����մϴ�.\n",buf);
						break;	
					}
				}
			}
			if(n!=0){
				mkdir(buf,0755);
			}
	//	}
		chdir(buf);
		memset(buf,NULL,100);	
		strftime(buf, sizeof(buf), " %G�� %m�� %d��  %H:%M:%S" ,tminfo);
		
	//	if((wfp=fopen(utx->ut_user, "w")) == NULL) {
		if((wfp=fopen("a", "w")) == NULL) {
			perror("fopen: ");
			exit(1);
		}

		fprintf(wfp,"Login name : %s\n",utx->ut_user);
		fprintf(wfp,"In real life : %s\n",pw->pw_comment);
		fprintf(wfp,"Directory : %s\n",pw->pw_dir);
		fprintf(wfp,"Shell : %s\n",pw->pw_shell);
		fprintf(wfp,"Time : %s\n",buf);
		fprintf(wfp,"TTY : %s\n",utx->ut_line);
		fprintf(wfp,"IP : %s\n",utx->ut_host);
		//	printf("Login name: %-20s In real life: %-12s\n",utx->ut_user, pw->pw_comment);
		//	printf("Directory: %-21s Shell: %-15s\n",pw->pw_dir,pw->pw_shell);
		//	printf("On since %s on %s from %s\n\n", buf, utx->ut_line, utx->ut_host);  
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
					printf("mkdir: '%s' ���丮�� ���� �� �����ϴ�: ������ ������\n",argv[1]);
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
		printf("rmdir: failed to remove '%s': �ش� �����̳� ���丮�� ����\n",argv[1]); 
	}
	return true;
}

bool cmd_ls( int argc, char* argv[] ) {
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
}

int main(){
        char line[1024];
		char wd[BUFSIZ];

        while( 1 )
        {
			getcwd(wd,BUFSIZ);
            printf( "[%s] $ ", wd);
			fgets(line, sizeof(line), stdin); 
            if( run( line ) == false )
				break;
			memset(wd,NULL,BUFSIZ);	
		}
		return 0;
}
