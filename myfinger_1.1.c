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


struct COMMAND { // Ŀ�ǵ� ����ü

	char* name;
	char* desc;
	bool(*func)(int argc, char* argv[]); // �Լ�������. ����� �Լ����� �Ű������� ������

};

typedef struct Hislist
{
	// �����丮 ����ü
	char data[BUFSIZ];
	struct Hislist *prev;
	struct Hislist *next;

}hlist;


pid_t child = -1;                                            // �ڽ����μ��� pid ���� ��������
int status;                                                  // ���μ��� status


hlist *head = NULL;
hlist *tail = NULL;
hlist *nowhis;


bool cmd_cd(int argc, char* argv[]);          //cd ��ɾ�
bool cmd_exit(int argc, char* argv[]);        //exit, quit ��ɾ�
bool cmd_help(int argc, char* argv[]);       //help ��ɾ�
bool cmd_myfinger(int argc, char* argv[]);     //myfinger ��ɾ� 
void InitList(hlist **h_head, hlist **h_tail);	// �����丮 �ʱ�ȭ
void Add_history(hlist **H_head, hlist **h_list,char *data); // �����丮 �߰�
hlist *Find(hlist *serch, char data);				// �����丮 �˻�


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
	// head �� �տ� �����ֱ� insert_his(head, data)
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

bool cmd_help(int argc, char* argv[]) { // ��ɾ� ���
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
	
	Add_history(&head, &tail, line);	// ������ ��ɾ� �����丮 ����Ʈ�� �߰�

	char delims[] = " \r\n\t";
	char* tokens[128];
	int token_count;
	int i;
	bool back;
	for (i = 0; i<strlen(line); i++) { //background ������ wait���� �ʴ´�.
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
				//  ^[ , [, A �ϰ�� ( ����Ű ��)
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
				//  ^[ , [, B �ϰ�� ( ����Ű �Ʒ�)
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
				//  ^[ , [, C �ϰ�� ( ����Ű ������)
			{
				// �ƹ����� x
			}
			else if (line[0] == '^]' && line[1] == ']' && line[2] == 'D')
				//  ^[ , [, D �ϰ�� ( ����Ű ����)
			{
				// �ƹ����� x
			}

			if (run(line, &head, &tail) == false)
				break;
		}
	}
	return 0;
}