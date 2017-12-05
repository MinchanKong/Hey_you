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


struct COMMAND { // 커맨드 구조체

	char* name;
	char* desc;
	bool(*func)(int argc, char* argv[]); // 함수포인터. 사용할 함수들의 매개변수를 맞춰줌

};

typedef struct Hislist
{
	// 히스토리 구조체
	char data[BUFSIZ];
	struct Hislist *prev;
	struct Hislist *next;

}hlist;


pid_t child = -1;                                            // 자식프로세스 pid 저장 전역변수
int status;                                                  // 프로세스 status


hlist *head = NULL;
hlist *tail = NULL;
hlist *nowhis;


bool cmd_cd(int argc, char* argv[]);          //cd 명령어
bool cmd_exit(int argc, char* argv[]);        //exit, quit 명령어
bool cmd_help(int argc, char* argv[]);       //help 명령어
bool cmd_myfinger(int argc, char* argv[]);     //myfinger 명령어 
void InitList(hlist **h_head, hlist **h_tail);	// 히스토리 초기화
void Add_history(hlist **H_head, hlist **h_list,char *data); // 히스토리 추가
hlist *Find(hlist *serch, char data);				// 히스토리 검색


struct COMMAND  builtin_cmds[] =
{
	{ "cd",    "change directory",                    cmd_cd },
	{ "exit",   "exit this shell",                        cmd_exit },
	{ "quit",   "quit this shell",                        cmd_exit },
	{ "help",  "show this help",                      cmd_help },
	{ "?",      "show this help",                      cmd_help },
	{ "myfinger", "show user information", cmd_myfinger }
};

void InitList()
{
	head = CreatHistory(0);
	tail = CreatHistory(0);
	head->next = tail;
	head->prev = NULL;
	tail->next = NULL;
	tail->prev = head;
}


hlist *CreatHistory(char *data)
{
	hlist *newhis = (hlist*)malloc(sizeof(hlist));
	strcpy(newhis->data, data);
	newhis->prev = NULL;
	newhis->next = NULL;

	return newhis;
}

void insert_his(hlist *tmp, char *data)
{
	hlist *newhis = CreatHistory(data);

	newhis->next = tmp->next;
	newhis->prev = tmp;
	tmp->next->prev = newhis;
	tmp->next = newhis;
	// head 의 앞에 값을넣기 insert_his(head, data)
}

bool cmd_cd(int argc, char* argv[]) { //cd : change directory
	if (argc == 1)
		chdir(getenv("HOME"));
	else if (argc == 2) {
		if (chdir(argv[1]))
			printf("No directory\n");
	}
	else
		printf("USAGE: cd [dir]\n");
}

bool cmd_exit(int argc, char* argv[]) {
	return false;
}

bool cmd_help(int argc, char* argv[]) { // 명령어 출력
	int i;
	for (i = 0; i < sizeof(builtin_cmds) / sizeof(struct COMMAND); i++)
	{
		if (argc == 1 || strcmp(builtin_cmds[i].name, argv[1]) == 0)
			printf("%-10s: %s\n", builtin_cmds[i].name, builtin_cmds[i].desc);
	}
}

bool cmd_myfinger(int argc, char* argv[]) {
	struct utmpx *utx;
	struct passwd *pw;
	struct tm *tminfo;
	char buf[100];
	int n;
	char *optarg;
	int optind;

	if (argc == 1) {
		printf("%-10s %-12s %-11s %-11s %-18s\n", "Login", "Name", "TTY", "When", "Where");

		while ((utx = getutxent()) != NULL) {
			if (utx->ut_type != USER_PROCESS)
				continue;

			pw = getpwnam(utx->ut_user);
			tminfo = localtime(&(utx->ut_tv.tv_sec));
			strftime(buf, sizeof(buf), " %a %H:%M ", tminfo);

			printf("%-10s %-12s %-10s %-12s %-18s\n", utx->ut_user, pw->pw_comment, utx->ut_line, buf, utx->ut_host);
		}
	}
	if ((n = getopt(argc, argv, "ab")) != -1) {
		switch (n) {
		case 'a':
			printf("Option : a\n");
			break;
		case 'b':
			printf("Option : b\n");
			break;
		default:
			printf("Nothing : no\n");
			break;
		}
	}
}

int tokenize(char* buf, char* delims, char* tokens[], int maxTokens) {
	int token_count = 0;
	char* token;
	token = strtok(buf, delims);
	while (token != NULL && token_count < maxTokens) {
		tokens[token_count] = token;
		token_count++;
		token = strtok(NULL, delims);
	}
	tokens[token_count] = NULL;
	return token_count;
}

bool run(char* line, hlist *head, hlist *tail) 
{
	
	Add_history(&head, &tail, line);	// 실행한 명령어 히스토리 리스트에 추가

	char delims[] = " \r\n\t";
	char* tokens[128];
	int token_count;
	int i;
	bool back;
	for (i = 0; i<strlen(line); i++) { //background 실행은 wait하지 않는다.
		if (line[i] == '&') {
			back = true;
			line[i] = '\0';
			break;
		}
	}
	token_count = tokenize(line, delims, tokens, sizeof(tokens) / sizeof(char*));
	if (token_count == 0)
		return true;
	for (i = 0; i < sizeof(builtin_cmds) / sizeof(struct COMMAND); i++) {
		if (strcmp(builtin_cmds[i].name, tokens[0]) == 0)
			return builtin_cmds[i].func(token_count, tokens);
	}
	child = fork();
	if (child == 0) {
		execvp(tokens[0], tokens);
		printf("No such file\n");
		_exit(0);
	}
	else if (child < 0)
	{
		printf("Failed to fork()!");
		_exit(0);
	}
	else if (back == false) {
		waitpid(child, &status, WUNTRACED);
	}
	return true;
}

int main() {

	char line[1024];
	char wd[BUFSIZ];

	nowhis = tail;

	getcwd(wd, BUFSIZ);
	InitList();

	while (1)
	{
		fflush(NULL);
		printf("[%s] $ ", wd);
		while (fgets(line, sizeof(line) - 1, stdin))
		{
			if (line[0] == 27 && line[1] == 91 && line[2] == 64)
				//  ^[ , [, A 일경우 ( 방향키 위)
			{
				nowhis = nowhis->prev;
				if (nowhis != head)
				{
					fprintf(stdout, "\r%80s", " ");
					fprintf(stdout, "\r%s%s", wd, nowhis->data);
					strcpy(line, nowhis->data);
				}
			}

			else if (line[0] == '^]' && line[1] == ']' && line[2] == 'B')
				//  ^[ , [, B 일경우 ( 방향키 아래)
			{
				nowhis = nowhis->next;
				if (nowhis != tail)
				{
					fprintf(stdout, "\r%80s", " ");
					fprintf(stdout, "\r%s%s", wd, nowhis->data);
					strcpy(line, nowhis->data);
				}
			}
			
			else if (line[0] == '^]' && line[1] == ']' && line[2] == 'C')
				//  ^[ , [, C 일경우 ( 방향키 오른쪽)
			{
				// 아무동작 x
			}
			else if (line[0] == '^]' && line[1] == ']' && line[2] == 'D')
				//  ^[ , [, D 일경우 ( 방향키 왼쪽)
			{
				// 아무동작 x
			}

			if (run(line, &head, &tail) == false)
				break;
		}
	}
	return 0;
}