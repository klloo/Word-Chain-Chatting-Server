#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/signal.h>

#define MAXLINE 1024
#define MAX_SOCK 512
char* escapechar = "exit\n";
int readline(int, char*, int);

int s; //서버와 연결된 소켓번호
struct Name {
	char n[20]; //대화방에서 사용할 이름
	int len; //이름의 크기
}name;


int main(int argc, char* argv[]) {
	char line[MAXLINE], sendline[MAXLINE + 1];
	int n, pid, size;
	struct sockaddr_in server_addr;
	int nfds;
	fd_set read_fds;

	if (argc < 4) {
		printf("실행방법 :%s 호스트 IP 주소 포트번호 사용자 이름\n", argv[0]);
		return -1;
	}

	//채팅 참가자 이름 구조체 초기화
	sprintf(name.n, "%s", argv[3]);
	name.len = strlen(name.n);


	//채팅 서버의 소켓주소 구조체 server_addr 초기화
	memset((char*)& server_addr, '\0', sizeof(server_addr));
	//주소체계 저장
	server_addr.sin_family = AF_INET;
	//자신의 IP할당 (명령행 인자로 전달 받음)
	server_addr.sin_addr.s_addr = inet_addr(argv[1]);
	//사용할 포트 저장 (명령행 인자로 전달 받음)
	server_addr.sin_port = htons(atoi(argv[2]));


	//socket함수를 이용해 소켓파일 기술자 생성
	//소켓 종류로 IPv4인터넷 프로토콜 체계인 PF_INET을 지정하고, 통신 방식으로 연결형 서비스를 위해 SOCK_STREAM을 지정
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Client : Can't open stream socket\n");
		return -1;
	}

	//서버에 연결 요청
	if (connect(s, (struct sockaddr*) & server_addr, sizeof(server_addr)) < 0) {
		printf("Client : Can't connect to server.\n");
		return -1;
	}
	else {
		printf("접속에 성공했습니다.\n");
		//접속에 성공한 경우 서버에 대화방에서 사용하는 자신의 이름 전달
		send(s, name.n, name.len, 0);
	}

	nfds = s + 1;
	FD_ZERO(&read_fds);
	
	//글자색을 파란색으로 변경
	fprintf(stderr, "\033[1;34m");

	while (1) {

		FD_SET(0, &read_fds);
		FD_SET(s, &read_fds);

		//select 함수를 호출하여 읽기 변화 감지
		if (select(nfds, &read_fds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0) {
			printf("select error in client\n");
			return -1;
		}
		//서버로부터 수신한 메시지 처리
		//소켓 s가 read_fds에 1로 설정되어 있는지 확인
		if (FD_ISSET(s, &read_fds)) {
			char recvline[MAXLINE];
			int size;
			//recv함수를 이용해 소켓 s로부터 받은 데이터를 recvline에 저장
			if ((size = recv(s, recvline, MAXLINE, 0)) > 0) {
				recvline[size] = '\0';
				printf("%s \n", recvline);
			}
			//글자색을 파란색으로 변경
			fprintf(stderr, "\033[1;34m");
		}
		if (FD_ISSET(0, &read_fds)) {
			//글자색을 노란색으로 변경
			fprintf(stderr, "\033[1;33m"); 
			//readline함수를 통해 표준 입력(0)으로 입력을 받는다.
			if (readline(0, sendline, MAXLINE) > 0) {
				size = strlen(sendline);
				//입력받은 문자열을 서버에 전송한다.
				if (send(s, sendline, size, 0) != (size))
					printf("Error: Written error on socket.\n");
				//입력받은 문자열 exit이라면 서버에서 파일에 입력한 순위정보를 출력하고 종료한다.
				if (size == 5 && strncmp(sendline, escapechar, 5) == 0) {
					FILE* fp = fopen("score.txt", "r");
					int c;
					while ((c = fgetc(fp)) != EOF)
						printf("%c", c);
					printf("\nGood bye.\n");
					close(s);
					return -1;
				}
			}

		}

	}

}

