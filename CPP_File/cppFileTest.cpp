#include <stdio.h>
#include <iostream>
#include <cstring>

using namespace std;

int main() {
  
  FILE *fptr;

  // Create a file on your computer (filename.txt)
  // w = write, a = append, r = read
  fptr = fopen("filename.txt", "w");

  fprintf(fptr, "Some text");

  // Close the file
  fclose(fptr);

  //+++++++++To Append+++++++++++

  // Open a file in append mode
  fptr = fopen("filename.txt", "a");

  // Append some text to the file
  fprintf(fptr, "\nHi everybody!");
  fprintf(fptr, "\nHi everybody!1");
  fprintf(fptr, "\nHi everybody!2");
  fprintf(fptr, "\nHi everybody!3");

  // Close the file
  fclose(fptr);

  //+++++++++to read+++++++++++

  fptr = fopen("filename.txt", "r");

  // Store the content of the file
  char myString[100];

  // Read the content and store it inside myString
  fgets(myString, 100, fptr);

  // Print first line of file
  printf("%s", myString);

  //prints all lines
  while(fgets(myString, 100, fptr)) {
    printf("%s", myString);
  }

  // Close the file
  fclose(fptr);


  //++++++++++++Test Parse++++++++++++++
  FILE *fileTest;

  fileTest = fopen("10bytetxt.txt", "r");

  char myString1[100];

  fgets(myString1, 100, fileTest);

  for(int i = 0; i <= strlen(myString1) - 1; i++) {
    cout << "\n" << myString1[i] << endl;
  }

  while()

  fclose(fileTest);

  return 0;
}
