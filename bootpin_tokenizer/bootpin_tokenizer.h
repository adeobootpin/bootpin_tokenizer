
#define MAX_REGEX_MATCH_LEN 1500 // i.e. the length of longest 'word'
#define MAX_TO_FILE_CHUNK_LEN 10000 // some training files are too big to fit in memory so this is the streaming buffer size

struct pair_hash
{
	template <class T1, class T2>
	std::size_t operator() (const std::pair<T1, T2>& pair) const
	{
		auto hash1 = std::hash<T1>{}(pair.first);
		auto hash2 = std::hash<T2>{}(pair.second);
		return hash1 ^ (hash2 << 1); // Combine the two hash values
	}
};



int ReadDataFromUTF8File(const char* file_name, void** pp_data, size_t* data_size);
void* BlockRealloc(void* current_block_ptr, uint64_t current_size, uint64_t new_size);
int ReadDataFromFile(const char* file_name, void** pp_data, size_t* data_size);
int WriteDataToFile(const char* file_name, void* data, size_t data_size);
char** GetTrainingFileNames(const char* training_set_folder, uint32_t* num_files);
void SpinForEver(const char* pszMessage);
FILE* ReadFirstFileBuffer(const char* file_name, char* buffer, uint32_t* buffer_size, int16_t delimeter = -1);
int ReadNextBuffer(FILE* fp, char* buffer, uint32_t* buffer_size, int16_t delimeter = -1);
void PrintETA(double nseconds_latest_iteration, uint32_t remaining_iterations);

int SaveTokenizer(const char* tokenizer_file_name, std::unordered_map<std::pair<unsigned int, unsigned int>, unsigned int, pair_hash>& merges);
template<typename T>T getMaxValue(T** ptr);
void TestFileStreaming();
void TestTokenizer();
int SetLocale_threadproc(void* params, int block_index, int total_blocks);

template<typename Dtype> Dtype** GetRegexMatches(char** file_names, uint32_t num_files, uint32_t* num_matches);
template<typename Dtype> int SaveTokenizer(const char* tokenizer_file_name, std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>& merges);
template<typename T>T getMaxValue(T** ptr);


int Encode(const char* text, unsigned int* encoded_text, unsigned int* encoded_text_len, std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t, pair_hash>& merges);
int Decode(unsigned int* encoded_text, unsigned int encoded_text_len, wchar_t* decoded_text, unsigned int* decoded_text_len, std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>&vocabulary);
//template<typename Dtype>int Encode(const char* text, unsigned int* encoded_text, unsigned int* encoded_text_len, std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>& merges);
//template<typename Dtype>int Decode(unsigned int* encoded_text, unsigned int encoded_text_len, wchar_t* decoded_text, unsigned int* decoded_text_len, std::unordered_map<Dtype, std::pair<Dtype, Dtype>>&vocabulary);


//template<typename Dtype> void TestTokenizer();

//int Encode(const char* text, unsigned int* encoded_text, unsigned int* encoded_text_len, std::unordered_map<std::pair<unsigned int, unsigned int>, unsigned int, pair_hash>& merges);
//int Decode(unsigned int* encoded_text, unsigned int encoded_text_len, wchar_t* decoded_text, unsigned int* decoded_text_len, std::unordered_map<unsigned int, std::pair<unsigned int, unsigned int>>&vocabulary);


