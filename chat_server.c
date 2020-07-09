#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>

#define MAXLINE 1024
#define MAX_SOCK 512
#define MAX_ROUND 11

int readline(int fd, char* ptr, int maxline);
struct client {
	int client_fd;
	char name[10];
	int score;
};

int main(int argc, char* argv[])
{
	char rline[MAXLINE], my_msg[MAXLINE];
	int i, j, n;
	int s, client_fd, clilen;
	int nfds;
	fd_set read_fds;
	//연결된 클라이언트의 수
	int num_chat = 0;

	struct client client_s[MAX_SOCK];
	struct sockaddr_in client_addr, server_addr;

	if (argc < 2)
	{
		printf("포트 번호를 입력하세요\n");
		return -1;
	}

	printf("server initializing...\n");

	//소켓 주소구조체 생성
	memset((char*)& server_addr, '\0', sizeof(server_addr));
	//주소체계 저장
	server_addr.sin_family = AF_INET;
	//자신의 IP할당 INADDR_ANY를 사용하여 자동으로 할당
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//사용할 포트 저장 (명령행 인자로 전달 받음)
	server_addr.sin_port = htons(atoi(argv[1]));

	//socket함수를 이용해 소켓파일 기술자 생성
	//소켓 종류로 IPv4인터넷 프로토콜 체계인 PF_INET을 지정하고, 통신 방식으로 연결형 서비스를 위해 SOCK_STREAM을 지정
	if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Server:Can't open stream socket.\n");
		return -1;
	}
	//소켓에 포트를 지정
	//bind 함수를 이용해서 소켓기술자 s와 소켓 주소구조체 server_addr을 연결
	if (bind(s, (struct sockaddr*) & server_addr, sizeof(server_addr)) < 0) {
		printf("Server:Can't bind local address.\n");
		return -1;
	}

	//클라이언트 연결을 기다린다.
	//최대 허용 클라이언트 수는 5이다
	listen(s, 5);

	//최대 소켓번호+1
	nfds = s + 1;
	FD_ZERO(&read_fds);
	int round = 1;
	int turn = 0;

	//초기 단어의 끝 글자를 랜덤으로 지정 
	srand(time(NULL));
	char pre = rand() % 26 + 'a';

	//점수 정보를 저장할 파일 
	FILE* fp;
	fp = fopen("score.txt", "w");

	while (1)
	{
		//nfds값을 갱신
		if ((num_chat - 1) >= 0)
			nfds = client_s[num_chat - 1].client_fd + 1;

		//읽기 변화를 감지할 소켓번호를 fd_set구조체 read_fds에 지정
		//FD_SET은 read_fds 중 소켓 s에 해당하는 비트를 1로 함
		FD_SET(s, &read_fds);
		for (i = 0; i < num_chat; i++)
			FD_SET(client_s[i].client_fd, &read_fds);
		//select 함수를 호출하여 읽기 변화 감지
		if (select(nfds, &read_fds, (fd_set*)0, (fd_set*)0, (struct timeval*)0) < 0) {
			printf("select error in server\n");
			return -1;
		}
		char chmsg[50];
		char msg[50];
		//클라이언트 연결요청 처리
		//소켓 s가 read_fds에 1로 설정되어 있는지 확인
		if (FD_ISSET(s, &read_fds)) {
			clilen = sizeof(client_addr);
			//클라이언트 접속
			//accept함수를 이용해 연결 요청을 수락
			client_fd = accept(s, (struct sockaddr*) & client_addr, &clilen);
			if (client_fd != -1) {
				//클라이언트를 연결한 순서대로 배열에 저장
				client_s[num_chat].client_fd = client_fd;
				char name[50];
				n = recv(client_s[i].client_fd, name, MAXLINE, 0);
				name[n] = 0;
				strcpy(client_s[num_chat].name, name);
				client_s[num_chat].score = 0;
				//클라이언트의 수 증가
				num_chat++;
				//접속한 클라이언트 정보 출력
				printf("plus %d user %s\n", num_chat, name);
				//새롭게 접속한 클라이언트에게 순서 정보와 마지막 글자 정보 전송

				sprintf(msg, "%s %d %s", "당신은", num_chat, "번째 순서 입니다.\n턴이 10번 반복되면 게임을 종료합니다.\nexit을 입력하면 종료됩니다.\n");
				sprintf(chmsg, "시작할 알파벳은 '%c' 입니다.\n", pre);
				send(client_s[num_chat - 1].client_fd, msg, strlen(msg), 0);
				send(client_s[num_chat - 1].client_fd, chmsg, strlen(chmsg), 0);
				//첫번째로 접속한 사용자에게 차례 정보 전달
				if (num_chat == 1)
				{
					char turn_msg[] = "[System] 당신 차례입니다 (1 번째 턴)\n";
					send(client_s[0].client_fd, turn_msg, strlen(turn_msg), 0);
				}


			}
		}
		for (i = 0; i < num_chat; i++) {
			//소켓 s가 read_fds에 1로 설정되어 있는지 확인
			if (FD_ISSET(client_s[i].client_fd, &read_fds)) {
				//i번째 클라이언트로 부터 받은 메세지를 읽어서 rline에 저장
				if ((n = recv(client_s[i].client_fd, rline, MAXLINE, 0)) > 0) {
					rline[n] = '\0';
					//turn과 클라이언트 번호(순서)가 일치할 때만 확인 및 다른 클라이언트에게 메세지 전송
					if (turn == i) {
						//턴 증가
						turn++;
						turn %= num_chat;
						//한 턴이 끝나면 round 증가
						if (!turn)
							round++;
						
						char is_correct[20];
						//이전 단어의 마지막 글자와 전달받은 메세지의 첫 글자가 맞아서 정답인 경우
						if (pre == rline[0]) {
							client_s[i].score += 10;
							strcpy(is_correct, "맞았습니다.");
							printf("O\n");
						}
						//이전 단어의 마지막 글자와 전달받은 메세지의 첫 글자가 달라서 오답인 경우
						else {
							client_s[i].score -= 5;
							strcpy(is_correct, "틀렸습니다.");
							printf("X\n");
						}
						//pre값 갱신
						pre = rline[n - 2];
						char sendline[50];
						sprintf(sendline, "[%s] %s", client_s[i].name, rline);
						int sendlen = strlen(sendline);
						//접속한 다른 클라이언트들에게 메세지 전송
						for (j = 0; j < num_chat; j++) {
							send(client_s[j].client_fd, sendline, sendlen, 0);
							//다음 차례(turn번째)인 클라이언트에게 턴 정보 전송
							if (turn == j) {
								char turn_msg[50];
								sprintf(turn_msg, "\n[System] 당신 차례입니다 (%d 번째 턴)", round);
								send(client_s[turn].client_fd, turn_msg, strlen(turn_msg), 0);

							}
						}
						//현재 턴의 클라이언트에게 끝말을 제대로 이었는지 정답 여부와 현재 점수 정보 전송
						char cur_msg[100];
						sprintf(cur_msg, "[System]%s 현재 점수 : %d\n", is_correct, client_s[i].score);
						send(client_s[i].client_fd, cur_msg, strlen(cur_msg), 0);

					}

					//전달받은 메세지 출력
					printf("[%s]%s",client_s[i].name, rline);
					//턴이 종료되는 조건
					if (round == MAX_ROUND) {
							//클라이언트들에게 점수 정보 전송
							for (int i = 0; i < num_chat; i++) {
								char score_msg[100];
								sprintf(score_msg, "\n당신의 점수는 %d점 입니다.\n종료하려면 exit를 입력하세요.\n", client_s[i].score);
								send(client_s[i].client_fd, score_msg, strlen(score_msg), 0);
							}
							//점수의 내림차순으로 정렬 
							struct client temp;
							for (int i = 0; i < num_chat - 1; i++) {
								for (int j = 0; j < num_chat - 1 - i; j++) {
									if (client_s[j].score < client_s[j + 1].score) {
										temp = client_s[j];
										client_s[j] = client_s[j + 1];
										client_s[j + 1] = temp;
									}
								}
							}
							//순위와 점수 정보를 파일에 출력 
							fprintf(fp, "순위  사용자명 점수\n");
							for (int i = 0; i < num_chat; i++) {
								fprintf(fp, "%2d %10s %3d\n", i + 1, client_s[i].name, client_s[i].score);
								shutdown(client_s[i].client_fd, 2);
							}
							return 0;
						}
				}
			}
		}
	}
}

