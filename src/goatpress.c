#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  for(int i = 0; i < dictionarySize; ++i) {
    if(check(index + 26 * i, boardPattern)) {
      printf("Found candidate word: %s\n", words + i * 26);
    }
  }

  free(boardPattern);
  free(index);
};
