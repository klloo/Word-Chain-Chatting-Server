#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//fd로 지정한 스트림으로 입력받은 문자열을 ptr에 저장하고, 입력받은 문자열의 길이를 리턴한다.
int readline(int fd, char *ptr, int maxlen){
   int n, rc;
   char c;

   for(n=1; n<maxlen; n++){
      if((rc = read(fd, &c, 1)) ==1){
         *ptr++ = c;
         if(c=='\n') break;
      }
      else if(rc == 0){
         if(n == 1) return 0;
         else break;
      }

   }

   *ptr = 0;

   return n;
}
