#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int countBits(char b) {
  // Count the number of 1's in the binary representation of b - group
  // into a hierarchy of pairs and propagate up the tree:
  b = (b & 0x55) + ((b >> 1) & 0x55);
  b = (b & 0x33) + ((b >> 2) & 0x33);
  b = (b & 0x0f) + ((b >> 4) & 0x0f);
  return b;
}

void findIndex(const unsigned char *word, unsigned char *indexEntry) {
  for(int i = 0; i < strlen(word); ++i) {
    unsigned char c = word[i];
    if((c >= 'a') && (c <= 'z')) {
      int offset = c - 'a';
      if(indexEntry[offset] < 255) {
	indexEntry[offset] <<= 1;
	indexEntry[offset] += 1;
      }
      else {
	fprintf(stderr, "Invalid entry %s: too many repeated characters\n", word);
      }
    }
    else {
      fprintf(stderr, "Invlid entry %s: contains characters other than lower case a-z\n", word);
    }
  }
}

int check(const unsigned char *wordPattern, const unsigned char *boardPattern) {
  for(int i = 0; i < 26; ++i) {
    if((wordPattern[i] & boardPattern[i]) != wordPattern[i]) {
      return 0;
    }
  }
  return 1;
}

int score(const unsigned char *wordPattern, const unsigned char *onePointPattern, const unsigned char *twoPointPattern) {
  int s = 0;
  for(int i = 0; i < 26; ++i) {
    s += countBits(wordPattern[i] & onePointPattern[i]);
    s += countBits(wordPattern[i] & twoPointPattern[i]);
  }
  return s;
}

int pickMove(const unsigned char *index, const int dictionarySize, const unsigned char *board, const unsigned char *onePoint, const unsigned char *twoPoint) {
  unsigned char *boardPattern = calloc(26, 1);
  findIndex(board, boardPattern);

  unsigned char *onePointPattern = calloc(26, 1);
  findIndex(onePoint, onePointPattern);

  unsigned char *twoPointPattern = calloc(26, 1);
  findIndex(twoPoint, twoPointPattern);

  int maxScore = -1;
  int maxIndex = -1;

  for(int i = 0; i < dictionarySize; ++i) {
    if(check(index + 26 * i, boardPattern)) {
      int s = score(index + 26 * i, onePointPattern, twoPointPattern);
      if(s > maxScore) {
	maxScore = s;
	maxIndex = i;
      }
    }
  }

  free(boardPattern);
  free(onePointPattern);
  free(twoPointPattern);

  return maxIndex;
}

int main(int argc, char **argv) {
  const char *host = "of1-dev-dan";
  int port = 5123;

  const int dictionarySize = 173529;
  unsigned char *index = calloc(dictionarySize * 26, 1);
  unsigned char *words = calloc(dictionarySize * 26, 1);

  FILE *f = fopen("dictionary", "r");

  for(int i = 0; i < dictionarySize; ++i) {
    fgets(words + 26 * i, 26, f);
    words[strlen(words + 26 * i) + 26 * i - 1] = 0;
    findIndex(words + 26 * i, index + 26 * i);
  }

  fclose(f);  

  int fd = socket(AF_INET, SOCK_STREAM, 0);

  struct sockaddr_in myaddr;
  memset((char *)&myaddr, 0, sizeof(myaddr));
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(0);

  bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));

  struct hostent *hp;
  struct sockaddr_in servaddr;
  memset((char*)&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);

  hp = gethostbyname(host);
  memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
  connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  char buffer[1024];

  while(read(fd, buffer, 1024)) {
    unsigned char *board = "abcdefghijklmnopqrstuvwxy";
    unsigned char *onePoint = "abcde"; // Letters which we can claim (including those owned by the opponent, but not those locked in)
    unsigned char *twoPoint = "abc"; // Letters which we can steal from the opponent
    
    int move = pickMove(index, dictionarySize, board, onePoint, twoPoint);
    
    if(move == -1) {
      printf("No suitable move: pass");
    }
    else {
      printf("Best word is %s\n", words + move * 26);
    }
  }

  shutdown(fd, SHUT_RDWR);

  free(words);
  free(index);
};
