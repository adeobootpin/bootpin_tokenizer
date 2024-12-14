#include <stdio.h>
#include <string.h>

#include "bootpin_tokenizer.h"

int main()
{
	void* tokenizer;

	tokenizer = InitializeTokenizer("f:\\src\\bootpin_tokenizer\\data\\tokenizer.bin");

	unsigned int encoded_text[500];
	unsigned int encoded_text_len = sizeof(encoded_text)/sizeof(encoded_text[0]);
	const char* text = "Once upon a time in the West.";

	Encode(tokenizer, text, encoded_text, &encoded_text_len);
	size_t len = strlen(text);


	unsigned int w_buffer_len = 10000;
	wchar_t* w_buffer = new wchar_t[w_buffer_len];
	
	Decode(tokenizer, encoded_text, encoded_text_len, w_buffer, &w_buffer_len);


	return 0;
}