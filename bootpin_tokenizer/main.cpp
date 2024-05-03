#include <stdio.h>
#include <unordered_map>
#include <fstream>
#include <string>
#include <locale>
#include <codecvt>
#include <regex>
#include <set>
#include <iostream>
#include <assert.h>
#include <filesystem>
#include <stdlib.h>
#include <clocale>
#include <limits>

#include "threadpool2.h"

#include "bootpin_tokenizer.h"


#include <windows.h>
CRITICAL_SECTION cs_itr;
CRITICAL_SECTION cs_tally;

ThreadPool* threadpool;



int UpdateMerges(std::unordered_map<std::pair<unsigned int, unsigned int>, unsigned int, pair_hash>& merges, std::unordered_map<std::pair<unsigned int, unsigned int>, unsigned int, pair_hash>& tally)
{
	uint32_t max;
	unsigned int new_merge_pair[2];
	unsigned int new_token_id;

	if (!tally.size())
	{
		return 2;
	}

	max = 0;
	for (const auto& entry : tally)
	{
		char ch1 = static_cast<char>(entry.first.first);
		char ch2 = static_cast<char>(entry.first.second);

		unsigned int count = static_cast<int>(entry.second);

		//printf("Tally: [%c, %c] %d\n", ch1, ch2, count);

		if (count > max)
		{
			max = count;
			new_merge_pair[0] = entry.first.first;
			new_merge_pair[1] = entry.first.second;
		}
	}

	
	new_token_id = (unsigned int)merges.size() + 256;
	merges[std::make_pair(new_merge_pair[0], new_merge_pair[1])] = new_token_id;

	if (new_merge_pair[0] < 128 && new_merge_pair[1] < 128)
	{
		printf("new token: [%c, %c] -> %d (%d occurences)\n", (char)new_merge_pair[0], (char)new_merge_pair[1], new_token_id, max);
	}
	else
	{
		if (new_merge_pair[0] < 128 && new_merge_pair[1] >= 128)
		{
			printf("new token: [%c, %d] -> %d (%d occurences)\n", (char)new_merge_pair[0], new_merge_pair[1], new_token_id, max);
		}
		else
		{
			if (new_merge_pair[0] >= 128 && new_merge_pair[1] < 128)
			{
				printf("new token: [%d, %c] -> %d (%d occurences)\n", new_merge_pair[0], (char)new_merge_pair[1], new_token_id, max);
			}
			else
			{
				if (new_merge_pair[0] >= 128 && new_merge_pair[1] >= 128)
				{
					printf("new token: [%d, %d] -> %d (%d occurences)\n", new_merge_pair[0], new_merge_pair[1], new_token_id, max);
				}
			}
		}
	}
	


	return 0;
}



int UpdateTally(unsigned int* merged_chunk, unsigned int len, std::unordered_map<std::pair<unsigned int, unsigned int>, unsigned int, pair_hash>& tally)
{
	unsigned int i;

	for (i = 0; i < len - 1; i++)
	{
		std::pair<unsigned int, unsigned int> keyToSearch = std::make_pair(merged_chunk[i], merged_chunk[i + 1]);
		auto it = tally.find(keyToSearch);
		if (it != tally.end())
		{
			tally[std::make_pair(merged_chunk[i], merged_chunk[i + 1])] = it->second + 1;
		}
		else
		{
			tally[std::make_pair(merged_chunk[i], merged_chunk[i + 1])] = 1;
		}
	}

	//printf("Tally: %d\n", tally.size());
	return 0;
}


// apply merges to regex chunk, len of merged_chunk must be at least strlen(regex_chunk)
// merged_chunk_len in: size of merged_chunk buffer
// merged_chunk_len out: merged_chunk length
int ApplyMerges(const char* regex_chunk, unsigned int* merged_chunk, unsigned int* merged_chunk_len, std::unordered_map<std::pair<unsigned int, unsigned int>, unsigned int, pair_hash>& merges)
{
	unsigned int temp[MAX_REGEX_MATCH_LEN];
	size_t merge_index;
	size_t len;
	size_t i;
	int idx;
	unsigned int min;
	unsigned int min_pair[2];

	wchar_t debug[MAX_REGEX_MATCH_LEN] = { 0 };
	int debug_count = 0;

	len = strlen(regex_chunk);

	if (*merged_chunk_len < len)
	{
		SpinForEver("*merged_chunk_len < len check failed\n");
		assert(0);
		return -1;
	}

	for (i = 0; i < len; i++)
	{
		merged_chunk[i] = (int)(unsigned char)regex_chunk[i];
		debug[i] = merged_chunk[i];
	}
	//std::wcout << L"Original: " <<  debug << L"\n";

	while (true)
	{
		// find the earliest merge (i.e. the one with the lowest encoding)
		min = UINT_MAX;
		for (i = 0; i < len - 1; i++)
		{
			std::pair<unsigned int, unsigned int> keyToSearch = std::make_pair(merged_chunk[i], merged_chunk[i + 1]);

			auto it = merges.find(keyToSearch);
			if (it != merges.end())
			{
				if (it->second < min)
				{
					min = it->second;
					min_pair[0] = merged_chunk[i];
					min_pair[1] = merged_chunk[i + 1];
					merge_index = i;
				}
			}
		}

		if (min == UINT_MAX)
		{
			break; // nothing to merge so bail
		}

		//
		// do the merge
		//
		memset(debug, 0, sizeof(debug));
		idx = 0;

		for (i = 0; i < len - 1; i++)
		{
			if (i == merge_index)
			{
				debug[idx] = min;
				temp[idx++] = min;
				i++;
				if (i == (len - 2)) // get the last one
				{
					debug[idx] = merged_chunk[i + 1];
					temp[idx++] = merged_chunk[i + 1];
				}
			}
			else
			{
				debug[idx] = merged_chunk[i];
				temp[idx++] = merged_chunk[i];

				if (i == (len - 2)) // get the last one
				{
					debug[idx] = merged_chunk[i + 1];
					temp[idx++] = merged_chunk[i + 1];
				}
			}
		}

		len = idx;
		memcpy(merged_chunk, temp, sizeof(unsigned int) * len);
		//std::wcout << L"Merge " << ++debug_count << L": " << debug << L"\n";
	}

	*merged_chunk_len = (unsigned int)len;
	return 0;
}



int Encode(const char* text, unsigned int* encoded_text, unsigned int* encoded_text_len, std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t, pair_hash>& merges)
{
	size_t len;
	unsigned int buffer_len;
	unsigned int* encode_index;
	unsigned int encode_buffer_len;
	char regex_chunk[MAX_REGEX_MATCH_LEN];


	std::wregex word_pattern(LR"(( ?)(\w+|[`~!@#$%^&*\(\)-_=+,<.>/?;:'\"\[{\]}\\|]|[^\u0000-\u007F]))"); //https://stackoverflow.com/questions/150033/regular-expression-to-match-non-ascii-characters

	len = strlen(text) + 1;
	
	wchar_t* w_buffer = new wchar_t[len];
	
	len = mbstowcs(w_buffer, text, len);

	std::wstring w_text(w_buffer);

	std::wsregex_iterator end;
	std::wsregex_iterator it(w_text.begin(), w_text.end(), word_pattern);

	encode_index = encoded_text;
	encode_buffer_len = *encoded_text_len;

	while (it != end)
	{
		auto dodo = it->str();
		const wchar_t* debug = it->str().c_str();
		std::wcout << it->str() << std::endl;
		wcstombs((char*)regex_chunk, it->str().c_str(), sizeof(regex_chunk));

		buffer_len = encode_buffer_len;
		ApplyMerges(regex_chunk, encode_index, &buffer_len, merges);
		encode_buffer_len -= buffer_len;
		encode_index += buffer_len;
		
		++it;
	}

	*encoded_text_len = *encoded_text_len - encode_buffer_len;
	
	delete w_buffer;
	
	return 0;
}



int Decode(unsigned int* encoded_text, unsigned int encoded_text_len, wchar_t* decoded_text, unsigned int* decoded_text_len, std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>&vocabulary)
{
	
	unsigned int i;
	wchar_t* decoded;
	bool token_decoded;
	unsigned int* temp = nullptr;
	unsigned int len;
	unsigned int idx;

	if (*decoded_text_len < encoded_text_len)
	{
		assert(0);
		return -1;
	}

	temp = new unsigned int[*decoded_text_len]; // allocate enough space for decoded text
	memcpy(temp, encoded_text, sizeof(unsigned int) * encoded_text_len);
	decoded = decoded_text;
	len = encoded_text_len;

	while (true)
	{
		token_decoded = false;

		idx = 0;
		for (i = 0; i < len; i++)
		{
			auto it = vocabulary.find(temp[i]);
			if (it != vocabulary.end())
			{
				decoded_text[idx++] = (wchar_t)it->second.first;
				decoded_text[idx++] = (wchar_t)it->second.second;
				token_decoded = true;
			}
			else
			{
				decoded_text[idx++] = (wchar_t)temp[i];
			}
		}

		if (!token_decoded)
		{
			break;
		}

		len = idx;
		
		//decoded_text[len] = L'\0';
		//std::wcout << L"Decoded: " << decoded_text << L"\n";
	
		for (i = 0; i < len; i++)
		{
			temp[i] = decoded_text[i];
		}

	}

	char* temp2 = new char[idx + 1];
	for (i = 0; i < idx; i++)
	{
		temp2[i] = (char)decoded_text[i];
	}
	temp2[idx] = '\0';

	mbstowcs(decoded_text, temp2, idx + 1);

	*decoded_text_len = idx;
	delete temp;
	delete temp2;
	
	return 0;
}



// update tally table, return mode token pair
template<typename Dtype>
int UpdateTally(Dtype* merged_chunk, std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>& tally, Dtype* mode_token1, Dtype* mode_token2, Dtype* frequency)
{
	unsigned int len;
	unsigned int i;
	Dtype max_count;
	Dtype new_count;

	if (!tally.size())
	{
		max_count = 0;
	}
	else
	{
		std::pair<Dtype, Dtype> keyToSearch = std::make_pair(*mode_token1, *mode_token2);
		auto it = tally.find(keyToSearch);

		if (it != tally.end())
		{
			max_count = it->second;
		}
		else
		{
			SpinForEver("Invalid max token pair!\n");
		}
	}


	len = merged_chunk[0]; // string length is in location 0

	for (i = 1; i < len; i++)
	{
		std::pair<Dtype, Dtype> keyToSearch = std::make_pair(merged_chunk[i], merged_chunk[i + 1]);
		auto it = tally.find(keyToSearch);
		if (it != tally.end())
		{
			new_count = it->second + 1;	
		}
		else
		{
			new_count = 1;
		}

		tally[std::make_pair(merged_chunk[i], merged_chunk[i + 1])] = new_count;

		if (new_count > max_count)
		{
			max_count = new_count;
			*mode_token1 = merged_chunk[i];
			*mode_token2 = merged_chunk[i + 1];
			*frequency = max_count;
		}
	}

	return 0;
}


template<typename Dtype>
int UpdateMerges(std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>& merges, Dtype mode_token1, Dtype mode_token2, Dtype* new_token, Dtype frequency)
{
	unsigned int new_token_id;

	new_token_id = (unsigned int)merges.size() + 256;
	merges[std::make_pair(mode_token1, mode_token2)] = new_token_id;

	*new_token = new_token_id;

	// note all lines below just for debugging and can be removed
	// frequency also only for debugging
	if (mode_token1 < 128 && mode_token2 < 128)
	{
		printf("new token: [%c, %c] -> %d (%d occurences)\n", (char)mode_token1, (char)mode_token2, new_token_id, frequency);
	}
	else
	{
		if (mode_token1 < 128 && mode_token2 >= 128)
		{
			printf("new token: [%c, %d] -> %d (%d occurences)\n", (char)mode_token1, mode_token2, new_token_id, frequency);
		}
		else
		{
			if (mode_token1 >= 128 && mode_token2 < 128)
			{
				printf("new token: [%d, %c] -> %d (%d occurences)\n", mode_token1, (char)mode_token2, new_token_id, frequency);
			}
			else
			{
				if (mode_token1 >= 128 && mode_token2 >= 128)
				{
					printf("new token: [%d, %d] -> %d (%d occurences)\n", mode_token1, mode_token2, new_token_id, frequency);
				}
			}
		}
	}



	return 0;
}

// apply single merge to regex chunk
template<typename Dtype>
int ApplyMerge(Dtype* regex_chunk, Dtype mode_token1, Dtype mode_token2, Dtype new_token)
{
	Dtype temp[MAX_REGEX_MATCH_LEN];
	unsigned int i;
	unsigned int len;
	unsigned int idx;

	len = regex_chunk[0]; // string length is in location 0

	if (len == 1)
	{
		return 0; // nothing to do
	}


	idx = 0;
	for (i = 1; i < len; i++)
	{
		if (regex_chunk[i] == mode_token1 && regex_chunk[i + 1] == mode_token2)
		{
			temp[idx++] = new_token;
			i++;
			if (i == (len - 1)) // get the last one
			{
				temp[idx++] = regex_chunk[i + 1];
			}
		}
		else
		{
			temp[idx++] = regex_chunk[i];
			if (i == (len - 1)) // get the last one
			{
				temp[idx++] = regex_chunk[i + 1];
			}
		}
	}

	regex_chunk[0] = idx;
	memcpy(&regex_chunk[1], temp, sizeof(Dtype) * idx);
	
	return 0;
}


template<typename Dtype>
struct BytePairEncodeParams
{
	Dtype** regex_matches;
	uint32_t num_regex_matches;
	std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>* merges;
	std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>* tally;
	Dtype mode_token1;
	Dtype mode_token2;
	Dtype new_token;
	Dtype frequency;
};


template<typename Dtype>
int UpdateTally_threadproc(void* params, int block_index, int total_blocks)
{
	int i;
	BytePairEncodeParams<Dtype>* bp;
	std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash> local_tally;
	bp = (BytePairEncodeParams<Dtype>*)params;
	Dtype mode_token1;
	Dtype mode_token2;
	Dtype frequency;

	for (i = block_index; i < (int)bp->num_regex_matches; i += total_blocks)
	{
		UpdateTally(bp->regex_matches[i], local_tally, &mode_token1, &mode_token2, &frequency); // out parameter not used here (only used in single threaded mode)
	}

	EnterCriticalSection(&cs_tally);
	for (const auto& entry : local_tally)
	{
		std::pair<unsigned int, unsigned int> keyToSearch = std::make_pair(entry.first.first, entry.first.second);
		auto it = bp->tally->find(keyToSearch);

		if (it != bp->tally->end())
		{
			frequency = it->second + entry.second;
		}
		else
		{
			frequency = entry.second;
		}

		(*(bp->tally))[std::make_pair(entry.first.first, entry.first.second)] = frequency;


		if (frequency > bp->frequency)
		{
			bp->frequency = frequency;
			bp->mode_token1 = entry.first.first;
			bp->mode_token2 = entry.first.second;
		}

	}

	LeaveCriticalSection(&cs_tally);


	return 0;
}



template<typename Dtype>
int ApplyMerge_threadproc(void* params, int block_index, int total_blocks)
{
	int i;
	BytePairEncodeParams<Dtype>* bp;
	bp = (BytePairEncodeParams<Dtype>*)params;

	for (i = block_index; i < (int)bp->num_regex_matches; i += total_blocks)
	{
		ApplyMerge(bp->regex_matches[i], bp->mode_token1, bp->mode_token2, bp->new_token);
	}

	return 0;
}

template<typename Dtype>
int BytePairEncode(Dtype** regex_matches, uint32_t num_regex_matches, std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>& merges, std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>& tally, ThreadPool* threadpool = nullptr)
{
	uint32_t i;
	int* threadpool_ret;
	BytePairEncodeParams<Dtype> bp;
	Dtype mode_token1;
	Dtype mode_token2;
	Dtype new_token;
	Dtype frequency;

	//threadpool = nullptr;

	if (!threadpool)
	{
		for (i = 0; i < num_regex_matches; i++)
		{
			/*
			int len;
			len = regex_matches[i][0];
			char strng[100];
			wchar_t w_buffer[100];
			memset(strng, 0, sizeof(strng));
			for (int j = 0; j < len; j++)
			{
				strng[j] = regex_matches[i][j + 1];
			}
			mbstowcs(w_buffer, strng, len + 1);
			*/

			UpdateTally(regex_matches[i], tally, &mode_token1, &mode_token2, &frequency);
		}

		UpdateMerges(merges, mode_token1, mode_token2, &new_token, frequency);

		for (i = 0; i < num_regex_matches; i++)
		{
			ApplyMerge(regex_matches[i], mode_token1, mode_token2, new_token);
		}

	}
	else
	{

		bp.regex_matches = regex_matches;
		bp.num_regex_matches = num_regex_matches;
		bp.merges = &merges;
		bp.tally = &tally;
		bp.frequency = 0;

		threadpool->Execute(UpdateTally_threadproc<Dtype>, &bp, threadpool->get_thread_count());
		threadpool->WaitForTaskCompletion(&threadpool_ret);

		UpdateMerges(merges, bp.mode_token1, bp.mode_token2, &bp.new_token, bp.frequency);

		threadpool->Execute(ApplyMerge_threadproc<Dtype>, &bp, threadpool->get_thread_count());
		threadpool->WaitForTaskCompletion(&threadpool_ret);
	}

	return 0;
}


template<typename Dtype>
int Train()
{
	Dtype** regex_matches = nullptr; // this means vocabaular size cannot be larger than 65535, use uint32_t** if larger vocabaular size is required (see getMaxValue function)
	char** file_names;
	uint32_t num_files;
	uint32_t target_vocab_size;
	uint32_t vocab_size;
	uint32_t num_matches;

	char file_path[250];
	const char* training_set_folder = "c:\\xfer\\tiny\\";


	std::chrono::steady_clock::time_point clock_begin;
	std::chrono::steady_clock::time_point clock_end;
	std::chrono::steady_clock::duration time_span;
	double nseconds;
	double nseconds_avg;


	if (!std::setlocale(LC_ALL, "en_US.UTF-8"))
	{
		std::cerr << "Failed to set UTF-8 locale." << std::endl;
		assert(0);
		return -1;
	}

	//TestFileStreaming();
	TestTokenizer(); return 0;



	std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash> merges;
	std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash> tally;
	std::unordered_map<Dtype, std::pair<Dtype, Dtype>> vocabulary;

	file_names = GetTrainingFileNames(training_set_folder, &num_files);
	regex_matches = GetRegexMatches<uint16_t>(file_names, num_files, &num_matches);

	tally.reserve(10000);

	InitializeCriticalSection(&cs_itr);
	InitializeCriticalSection(&cs_tally);
	threadpool = new ThreadPool;
	if (threadpool)
	{
		threadpool->Init(0);
		printf("Training tokenizer with %d threads\n", threadpool->get_thread_count());

		threadpool->Execute(SetLocale_threadproc, nullptr, threadpool->get_thread_count());
		
		int* threadpool_ret;
		threadpool->WaitForTaskCompletion(&threadpool_ret);
		for (int j = 0; j < threadpool->get_thread_count(); j++)
		{
			if (threadpool_ret[j])
			{
				SpinForEver("Unable to setlocale for thread %d\n");
				assert(0);
			}
		}
	}

	nseconds_avg = 0;
	target_vocab_size = 10000;

	if (target_vocab_size > getMaxValue(regex_matches))
	{
		SpinForEver("Data size too small for vocabulary size!\n");
	}


	for (vocab_size = 0; vocab_size < target_vocab_size; vocab_size++)
	{
		clock_begin = std::chrono::steady_clock::now();
		BytePairEncode<uint16_t>(regex_matches, num_matches, merges, tally, threadpool);

		printf("round: %d\n", vocab_size);
		tally.clear();

		if (!((vocab_size + 1) % 100))
		{
			sprintf_s(file_path, sizeof(file_path), "c:\\src\\bootpin_tokenizer\\data\\tokenizer_%d.bin", vocab_size + 1);
			SaveTokenizer(file_path, merges);
		}

		clock_end = std::chrono::steady_clock::now();
		time_span = clock_end - clock_begin;
		nseconds = double(time_span.count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;

		//nseconds_avg = ((nseconds_avg * vocab_size) + nseconds) / (vocab_size + 1);
		nseconds_avg += nseconds;
		PrintETA(nseconds_avg / (vocab_size + 1), target_vocab_size - (vocab_size + 1));
	}


	SaveTokenizer("f:\\src\\bootpin_tokenizer\\data\\tokenizer.bin", merges);

	return 0;
}



int main()
{
	Train<uint16_t>();
	return 0;
}




