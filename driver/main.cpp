#include <stdio.h>
#include <string.h>

#include "bootpin_tokenizer.h"

int main()
{
	return Train();

	void* tokenizer;

	tokenizer = InitializeTokenizer("c:\\src\\bootpin_tokenizer\\data\\smollm_tokenizer.bin");



	unsigned int encoded_text[500];
	unsigned int encoded_text_len = 500;
	//const char* text = "Once upon a time in the West.";

	const char* text = "One day, a little girl named Lily found a needle in her room. She knew it was difficult to play with it because it was sharp. Lily wanted to share the needle with her mom, so she could sew a button on her shirt. Lily went to her mom and said, \"Mom, I found this needle. Can you share it with me and sew my shirt?\" Her mom smiled and said, \"Yes, Lily, we can share the needle and fix your shirt. \"Together, they shared the needle and sewed the button on Lily's shirt. It was not difficult for them because they were sharing and helping each other. After they finished, Lily thanked her mom for sharing the needle and fixing her shirt. They both felt happy because they had shared and worked together.\"";

	Encode(tokenizer, text, encoded_text, &encoded_text_len);
	size_t len = strlen(text);

	unsigned int w_buffer_len = 10000;
	wchar_t* w_buffer = new wchar_t[w_buffer_len];
	
	Decode(tokenizer, encoded_text, encoded_text_len, w_buffer, &w_buffer_len);


	return 0;
}

