#include "../include/kasm.h"
#include "../include/libstd.h"
#include "../include/kc.h"
#include "../include/kernel.h"
#include "../include/string.h"

int speed = 1;


static const struct
{
   const char letter, *morse;
} Code[] =
{
   { 'A', ". - " },{ 'B', "- . . . " },{ 'C', "- . - . " },{ 'D', "- . . "  },
   { 'E', ". " },{ 'F', ". . - . " },{ 'G', "- - . "  },{ 'H', ". . . . " },
   { 'I', ". . " },{ 'J', ". - - - " },{ 'K', ". - . - " },{ 'L', ". - . . " },
   { 'M', "- - "   },{ 'N', "- .  " },{ 'O', "- - - "  },{ 'P', ". - - . " },
   { 'Q', "- - . - " },{ 'R', ". - . "  },{ 'S', ". . . "  },{ 'T', "- "    },
   { 'U', ". . - "  },{ 'V', ". . . - " },{ 'W', ". - - "  },{ 'X', "- . . - " },
   { 'Y', "- . - - " },{ 'Z', "- - . . " },{ ' ', "  "  } };

void makeNoise(int freq, int duration) {
  _speaker(freq, duration);
}

void putInSpeaker(char sound, int freq) {
  if ( sound == '.' ) {
    makeNoise(0x1234DD/freq,speed*100);
    return;
  }
  if ( sound == '-' ) {
    makeNoise(0x1234DD/freq,speed*300);
    return;
  }
  if ( sound == ' ' ) {
    waiti(100*speed);
  }
  
}

void getMorseCode(char character, char * str) {
  if ( !((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') )) {
    str[0] = '\0';
    return;
  }
  int i;
   for ( i = 0; i < sizeof Code / sizeof *Code; i++ ) {
         if ( toupper(character) == Code[i].letter ) {
	   int j = 0;
	   while ( Code[i].morse[j] != '\0' ) {
	     str[j] = Code[i].morse[j];
	     j++;
	   }
	   str[j] = '\0';
	   break;
         }
   } 
}
  

void changeSpeed(int newSpeed) {
  if ( newSpeed < 1 || newSpeed > 3 ) {
    return; 
  }
  speed = newSpeed;
  return;
}

void putStringInSpeaker(const char * str) {
  int i = 0;
  while ( *(str+i) != '\0' ) {
    putInSpeaker(*(str+i), 440);
    i++;
  }
}

void encodeChar(int c) {
  int i;
  for ( i = 0; i < sizeof Code / sizeof *Code; i++ ) {
         if ( toupper(c) == Code[i].letter ) {
	    putStringInSpeaker(Code[i].morse);
            break;
         }
   }
}

void encode(const char *s) {
   int i;
   for ( i = 0; s[i]; i++ ) {
     putc(s[i], SPEAKER);
   }
  
}
