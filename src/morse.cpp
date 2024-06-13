#include <Arduino.h>
#include <string.h>

const char* morseAlphabet[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", 
  "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.",
  "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-",
  "-.--", "--.."
};

void morseCode(const char* text){
  for(size_t i = 0; i < strlen(text); i++){
    char c = text[i];
    if(c == ' ')
    {
      delay(3000);
    }
    else
    {
      int index = c - 'A';
      const char* morse = morseAlphabet[index];
      for(size_t j = 0; j < strlen(morse); j++){
        char m = morse[j];
        if(m == '.')
        {
          digitalWrite(LED_BUILTIN, HIGH);
          delay(300);
          digitalWrite(LED_BUILTIN, LOW);
          delay(300);
        }
        else if(m == '-')
        {
          digitalWrite(LED_BUILTIN, HIGH);
          delay(900);
          digitalWrite(LED_BUILTIN, LOW);
          delay(300);
        }
      }
      delay(900);
    }
  }
}