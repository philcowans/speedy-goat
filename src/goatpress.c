#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const char* positionStrings[] = {
  "00", "01", "02", "03", "04",
  "10", "11", "12", "13", "14",
  "20", "21", "22", "23", "24",
  "30", "31", "32", "33", "34",
  "40", "41", "42", "43", "44"
};

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

int entrenched(const int i, const unsigned char *ownerships) {
  int row = i / 5;
  int col = i % 5;
  if((row > 0) && (ownerships[i - 5] != ownerships[i])) {
    return 0;
  }
  if((row < 4) && (ownerships[i + 5] != ownerships[i])) {
    return 0;
  }
  if((col > 0) && (ownerships[i - 1] != ownerships[i])) {
    return 0;
  }
  if((col < 4) && (ownerships[i + 1] != ownerships[i])) {
    return 0;
  }
  return 1;
}

int pickMove(const unsigned char *index, const int dictionarySize, const unsigned char *board, const unsigned char *onePoint, const unsigned char *twoPoint, int *played) {
  unsigned char *boardPattern = calloc(26, 1);
  findIndex(board, boardPattern);

  unsigned char *onePointPattern = calloc(26, 1);
  findIndex(onePoint, onePointPattern);

  unsigned char *twoPointPattern = calloc(26, 1);
  findIndex(twoPoint, twoPointPattern);

  int maxScore = -1;
  int maxIndex = -1;

  for(int i = 0; i < dictionarySize; ++i) {
    if(!played[i] && check(index + 26 * i, boardPattern)) {
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
  int *played = calloc(dictionarySize, sizeof(int));

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
  int charsRead;
  char *moveOffset;

  int started = 0;
  char ourSymbol;
  char theirSymbol;
  
  while(charsRead = read(fd, buffer, 1023)) {
    buffer[charsRead - 1] = 0; // Assume that response has a newline at the end

    if(!strncmp(buffer, "goatpress<VERSION=1> ;", charsRead)) {
      // Do nothing
    }
    else if(!strncmp(buffer, "new game ;", charsRead)) {
      started = 0;
    }
    else if(!strncmp(buffer, "; name ?", charsRead)) {
      write(fd, "speedy-goat\n", 12);
    }
    else if(!strncmp(buffer, "; ping ?", charsRead)) {
      write(fd, "pong\n", 5);
    }
    else if(moveOffset = strstr(buffer, "; move ")) {
      char *originalMoveOffset = moveOffset; // Store this so we can go back and parse the opponent's move once we know the board.

      moveOffset += 7; // Step over prefix;

      unsigned char board[26];
      for(int i = 0; i < 5; ++i) {
	memcpy(board + i * 5, moveOffset, 5);
	moveOffset += 6;
      }
      board[25] = 0;

      unsigned char ownershipString[26];
      for(int i = 0; i < 5; ++i) {
	memcpy(ownershipString + i * 5, moveOffset, 5);
	moveOffset += 6;	
      }
      ownershipString[25] = 0;

      if(originalMoveOffset == buffer) { // No previous move reported, so we are player 1
	if(!started) {
	  ourSymbol = '1'; theirSymbol = '2';
	}
      }
      else {
	if(!started) {
	  ourSymbol = '2'; theirSymbol = '1';
	  // Parse what the opponent played, format is: opponent: move:01,40,02,12,31,13,04,14,20,03,41,32,00
	  // TODO: There must be a better way of doing this!
	  char *currentPosition = buffer + 15;
	  char opponentsWord[26];
	  char *opponentsWordPtr = opponentsWord;
	  while(currentPosition < originalMoveOffset) {
	    int row = *currentPosition - '0'; // ASCII arithmetic FTW
	    int col = *(currentPosition + 1) - '0';
	    currentPosition += 3;
	    *opponentsWordPtr = board[row * 5 + col];
	    ++opponentsWordPtr;
	  }
	  *opponentsWordPtr = 0;
	  for(int i = 0; i < dictionarySize; ++i) {
	    if(!strcmp(opponentsWord, words + i * 26)) {
	      played[i] = 1;
	      break;
	    }
	  }
	}
      }

      started = 1;

      unsigned char onePoint[26];
      unsigned char twoPoint[26];
      int onePointOffset = 0;
      int twoPointOffset = 0;
      int tileValues[25];
      for(int i = 0; i < 25; ++i) {
	if(ownershipString[i] == ourSymbol) {
	  tileValues[i] = 0;
	}
	else if(ownershipString[i] == theirSymbol) {
	  if(entrenched(i, ownershipString)) {
	    tileValues[i] = 0;
	  }
	  else {
	    onePoint[onePointOffset] = board[i];
	    ++onePointOffset;
	    twoPoint[twoPointOffset] = board[i];
	    ++twoPointOffset;
	    tileValues[i] = 2;
	  }
	}
	else {
	  onePoint[onePointOffset] = board[i];
	  ++onePointOffset;
          tileValues[i] = 1;
	}
      }
      onePoint[onePointOffset] = 0;
      twoPoint[twoPointOffset] = 0;
      
      int move = pickMove(index, dictionarySize, board, onePoint, twoPoint, played);
      
      if(move == -1) {
	write(fd, "pass\n", 5);
      }
      else {
	played[move] = 1;
	write(fd, "move:", 5);
	int usedTiles[25];
	memset(usedTiles, 0, sizeof(usedTiles));
	for(int i = 0; i < strlen(words + move * 26); ++i) {
	  char c = words[move * 26 + i];
	  int maxPos = -1;
	  int maxScore = -1;
	  for(int j = 0; j < 25; ++j) {
	    if((board[j] == c) && !usedTiles[j] && (tileValues[j] > maxScore)) {
	      maxPos = j;
	      maxScore = tileValues[j];
	    }
	  }
	  usedTiles[maxPos] = 1;
	  write(fd, positionStrings[maxPos], 2);
	  if(i < (strlen(words + move * 26) - 1)) {
	    write(fd, ",", 1);
	  }
	}
	write(fd, "\n", 1);
      }
    }
    else {
      fprintf(stderr, "Received unexpected request");
      exit(1);
    }
  }

  shutdown(fd, SHUT_RDWR);

  free(words);
  free(index);
  free(played);

  return 0;
};
