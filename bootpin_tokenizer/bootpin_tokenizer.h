void* InitializeTokenizer(const char* file_name);
void FreeTokenizer(void* tokenizer);
int Encode(void* tokenizer, const char* text, unsigned int* encoded_text, unsigned int* encoded_text_len);
int Encode(void* tokenizer, const char* text, unsigned int* encoded_text, unsigned int* encoded_text_len, wchar_t* w_scratch_buffer, unsigned int w_scratch_buffer_len); // faster version with scratch buffer (i.e. no dynamic allocation)
int Decode(void* tokenizer, unsigned int* encoded_text, unsigned int encoded_text_len, wchar_t* decoded_text, unsigned int* decoded_text_len);
unsigned int GetVocabularySize(void* tokenizer);
int Train();