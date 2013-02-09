#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char **argv) {
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

  unsigned char *board = "abcdeabcdeabcdeabcdeabcde";
  unsigned char *boardPattern = calloc(26, 1);
  findIndex(board, boardPattern);

  unsigned char *onePoint = "abcde"; // Letters which we can claim (including those owned by the opponent, but not those locked in)
  unsigned char *onePointPattern = calloc(26, 1);
  findIndex(onePoint, onePointPattern);

  unsigned char *twoPoint = "abc"; // Letters which we can steal from the opponent
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

  printf("Best word is %s - score %d\n", words + maxIndex * 26, maxScore);

  free(boardPattern);
  free(onePointPattern);
  free(twoPointPattern);
  free(index);
};
