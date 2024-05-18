void* InitializeTokenizer(const char* file_name);
void FreeTokenizer(void* tokenizer);
int Encode(void* tokenizer, const char* text, unsigned int* encoded_text, unsigned int* encoded_text_len);
int Decode(void* tokenizer, unsigned int* encoded_text, unsigned int encoded_text_len, wchar_t* decoded_text, unsigned int* decoded_text_len);
unsigned int GetVocabularySize(void* tokenizer);