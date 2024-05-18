#include <stdio.h>
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
#include <thread>

#include "utils.h"

using namespace bootpin_tokenizer;

namespace bootpin_tokenizer {

	static void* BlockRealloc(void* current_block_ptr, uint64_t current_size, uint64_t new_size)
	{
		unsigned char* reallocated_block_ptr;

		reallocated_block_ptr = new unsigned char[new_size];

		memcpy(reallocated_block_ptr, current_block_ptr, current_size);

		delete current_block_ptr;

		return reallocated_block_ptr;
	}

	int64_t GetFileSize(FILE* stream)
	{
		int64_t size;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		_fseeki64(stream, 0, SEEK_END);
		size = _ftelli64(stream);
#else
		fseek(stream, 0, SEEK_END);
		size = ftell(stream);
#endif

		fseek(stream, 0, SEEK_SET);

		return size;
	}


	int ReadDataFromUTF8File(const char* file_name, void** pp_data, size_t* data_size)
	{
		int ret;
		FILE* stream;
		size_t bytes;
		size_t bytes_read;
		size_t file_size;
		unsigned char* char_data;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		errno_t err;
		err = fopen_s(&stream, file_name, "rb");
		if (err)
		{
			ret = -1;
			goto Exit;
		}
#else
		stream = fopen(file_name, "rb");
		if (!stream)
		{
			ret = -1;
			goto Exit;
		}
#endif
		file_size = GetFileSize(stream);
		char_data = new unsigned char[file_size + 2]; // add space for double null terminators
		char_data[file_size] = '\0';
		char_data[file_size + 1] = '\0';

		if (!char_data)
		{
			assert(0);
			ret = -1;
			goto Exit;
		}

		*pp_data = static_cast<void*>(char_data);

		bytes_read = 0;

		while (bytes_read < file_size)
		{
			bytes = fread(char_data, 1, file_size, stream);
			bytes_read += bytes;
			char_data += bytes;
		}

		fclose(stream);

		char_data = static_cast<unsigned char*>(*pp_data);

		if (char_data[0] == 0xef && // strip out byte-order mark if it exists
			char_data[1] == 0xbb &&
			char_data[2] == 0xbf)
		{
			bytes_read -= 3;
			memmove(char_data, &char_data[3], bytes_read + 2); // +2 to copy the double null terminators
		}

		*data_size = bytes_read;


		ret = 0;

	Exit:
		return ret;

	}

	int ReadDataFromFile(const char* file_name, void** pp_data, size_t* data_size)
	{
		int ret;
		FILE* stream;
		size_t bytes;
		size_t bytes_read;
		size_t file_size;
		char* char_data;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		errno_t err;
		err = fopen_s(&stream, file_name, "rb");
		if (err)
		{
			ret = -1;
			goto Exit;
		}
#else
		stream = fopen(file_name, "rb");
		if (!stream)
		{
			ret = -1;
			goto Exit;
		}
#endif
		file_size = GetFileSize(stream);
		char_data = new char[file_size];
		if (!char_data)
		{
			assert(0);
			ret = -1;
			goto Exit;
		}

		*pp_data = static_cast<void*>(char_data);

		bytes_read = 0;

		while (bytes_read < file_size)
		{
			bytes = fread(char_data, 1, file_size, stream);
			bytes_read += bytes;
			char_data += bytes;
		}

		*data_size = bytes_read;

		fclose(stream);

		ret = 0;

	Exit:
		return ret;

	}

	int WriteDataToFile(const char* file_name, void* data, size_t data_size)
	{
		int ret;
		FILE* stream;
		size_t bytes;
		size_t bytes_written;
		char* char_data;

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		errno_t err;
		err = fopen_s(&stream, file_name, "wb");
		if (err)
		{
			ret = -1;
			goto Exit;
		}
#else
		stream = fopen(file_name, "wb");
		if (!stream)
		{
			ret = -1;
			goto Exit;
		}
#endif
		bytes_written = 0;
		char_data = (char*)data;

		while (bytes_written < data_size)
		{
			bytes = fwrite(char_data, sizeof(char), data_size - bytes_written, stream);
			bytes_written += bytes;
			char_data += bytes;
		}

		fclose(stream);

		ret = 0;
	Exit:

		return ret;

	}

	void SpinForEver(const char* pszMessage)
	{
		while (true)
		{
			printf("\r\n%s", pszMessage);
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}

}

int StepsBackToBeforeLastDelimiter(char* buffer, size_t bytes_read, int16_t delimeter)
{
	int steps_back = 0;
	char delimeter_char;
	delimeter_char = (char)delimeter;
	char* ch = &buffer[bytes_read - 1];

	while (*ch != delimeter_char)
	{
		ch--;
		steps_back++;
		if (ch == buffer)
		{
			goto Exit;
		}
	}


	while (*ch == delimeter_char) // move back to first occurence of delimiter
	{
		ch--;
		steps_back++;
		if (ch == buffer)
		{
			goto Exit;
		}
	}


Exit:
	return steps_back;
}


// read up to buffer_size bytes into the buffer
// stop before first instance of delimiter a the end of buffer
// e.g. read up till just before the last ' ' ( single space) or just befor the first of a sequence of spaces '   '
// 'Hello Beryl!' 
// 'Hello   Beryl!' 
// stop at 'o' in both cases
// use this to split text into sentences
FILE* ReadFirstFileBuffer(const char* file_name, char* buffer, uint32_t* buffer_size, int16_t delimeter)
{
	FILE* stream;
	size_t bytes;
	size_t bytes_read;
	size_t read_size;
	char* char_data;


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
	errno_t err;
	err = fopen_s(&stream, file_name, "rb");
	if (err)
	{
		goto Exit;
	}
#else
	stream = fopen(file_name, "rb");
	if (!stream)
	{
		goto Exit;
	}
#endif

	char_data = buffer;
	read_size = *buffer_size;
	bytes_read = 0;

	while (bytes_read < *buffer_size)
	{
		bytes = fread(char_data, 1, read_size, stream);
		if (bytes == 0)
		{
			if (!feof(stream))
			{
				assert(0);
			}
			goto Exit;
		}

		bytes_read += bytes;
		char_data += bytes;
		read_size -= bytes;
	}

	if (delimeter >= 0)
	{
		int steps_back;
		steps_back = StepsBackToBeforeLastDelimiter(buffer, bytes_read, delimeter);
		fseek(stream, -steps_back, SEEK_CUR);
		bytes_read -= steps_back;
	}


Exit:
	*buffer_size = (uint32_t)bytes_read;
	return stream;

}

// read up to buffer_size bytes into the buffer
// stop before the last instance of the delimiter
// e.g. read up till just before the last ' ' (space) 
// use this to split text into sentences
int ReadNextBuffer(FILE* stream, char* buffer, uint32_t* buffer_size, int16_t delimeter)
{
	//strcpy_s(buffer, *buffer_size, "F1 viewing figures drop .BBC Sports. 26 February 2002. Archived from the original on 7 April 2008. Retrieved 10 March 2007. The cumulative figure, which exceeds the total population of the planet by many times, counts all viewers who watch F1 on any programme at any time during the year.");
	//*buffer_size = strlen(buffer);
	//return 0;

	int ret;
	size_t bytes;
	size_t bytes_read;
	size_t read_size;
	char* char_data;

	char_data = buffer;
	read_size = *buffer_size;
	bytes_read = 0;

	while (bytes_read < *buffer_size)
	{
		bytes = fread(char_data, 1, read_size, stream);
		if (bytes == 0)
		{
			if (!feof(stream))
			{
				assert(0);
				ret = -1;
			}
			else
			{
				ret = 1;
			}
			goto Exit;
		}
		bytes_read += bytes;
		char_data += bytes;
		read_size -= bytes;
	}

	if (delimeter >= 0)
	{
		int steps_back;
		steps_back = StepsBackToBeforeLastDelimiter(buffer, bytes_read, delimeter);
		fseek(stream, -steps_back, SEEK_CUR);
		bytes_read -= steps_back;
	}

	ret = 0;

Exit:
	*buffer_size = (uint32_t)bytes_read;
	return ret;
}




template uint16_t** GetRegexMatches(char** file_names, uint32_t num_files, uint32_t* num_matches);
template<typename Dtype>
Dtype** GetRegexMatches(char** file_names, uint32_t num_files, uint32_t* num_matches)
{
	int ret;
	uint32_t i;
	char* buffer;
	wchar_t* w_buffer;
	FILE* fp;
	size_t len;
	uint32_t total_matches;
	uint32_t buffer_size;
	Dtype** regex_matches = nullptr;
	char regex_chunk[MAX_REGEX_MATCH_LEN];
	unsigned int merged_len;


	const int block_allocate_stride = 1000000;

	std::wregex word_pattern(LR"(( ?)(\w+|[`~!@#$%^&*\(\)-_=+,<.>/?;:'\"\[{\]}\\|]|[^\u0000-\u007F]))"); //https://stackoverflow.com/questions/150033/regular-expression-to-match-non-ascii-characters

	buffer = new char[MAX_TO_FILE_CHUNK_LEN + 2];
	w_buffer = new wchar_t[MAX_TO_FILE_CHUNK_LEN + 2];

	printf("Running regex on training set...\n");

	total_matches = 0;
	for (i = 0; i < num_files; i++)
	{
		buffer_size = MAX_TO_FILE_CHUNK_LEN - 2;
		fp = ReadFirstFileBuffer(file_names[i], buffer, &buffer_size, ' ');


		while (true)
		{
			buffer[buffer_size] = '\0';
			buffer[buffer_size + 1] = '\0';
			buffer_size += 2;


			len = strlen(buffer);

			len = mbstowcs(w_buffer, buffer, len + 1);
			if (static_cast<std::size_t> (-1) == len)
			{
				SpinForEver("Error converting string:\n");
			}
			else
			{
				std::wstring text(w_buffer);

				std::wsregex_iterator end;
				std::wsregex_iterator it(text.begin(), text.end(), word_pattern);


				while (it != end)
				{
					if (!(total_matches % block_allocate_stride))
					{
						regex_matches = (Dtype**)BlockRealloc(regex_matches, sizeof(Dtype*) * (total_matches), sizeof(Dtype*) * (total_matches + block_allocate_stride));
					}

					wcstombs((char*)regex_chunk, it->str().c_str(), sizeof(regex_chunk));

					merged_len = MAX_REGEX_MATCH_LEN;
					len = strlen(regex_chunk);

					len = strlen(regex_chunk);
					if (len > sizeof(regex_chunk))
					{
						SpinForEver("Something went wrong with wcstombs!\n");
					}

					regex_matches[total_matches] = new Dtype[len + 1];

					regex_matches[total_matches][0] = static_cast<Dtype>(len); // since these are no longer C strings, need to save the length

					for (int j = 0; j < len; j++)
					{
						regex_matches[total_matches][j + 1] = (Dtype)(unsigned char)regex_chunk[j];
					}

					total_matches++;
					it++;
				}
			}


			buffer_size = MAX_TO_FILE_CHUNK_LEN - 2;
			ret = ReadNextBuffer(fp, buffer, &buffer_size, ' ');
			if (ret)
			{
				if (!buffer_size)
				{
					fclose(fp);
					break;
				}
			}
		}
	}

	printf("Total matches: %d\n", total_matches);
	delete w_buffer;
	delete buffer;

	*num_matches = total_matches;
	return regex_matches;
}

void PrintETA(double nseconds_latest_iteration, uint32_t remaining_iterations)
{
	uint32_t day = 86400;
	uint32_t hour = 3600;
	uint32_t minute = 60;

	uint32_t totalseconds = (uint32_t)(nseconds_latest_iteration * remaining_iterations);

	uint32_t daysout = totalseconds / day;
	uint32_t hoursout = ((totalseconds - daysout * day) / hour);
	uint32_t minutesout = ((totalseconds - daysout * day - hoursout * hour) / minute);
	uint32_t secondsout = totalseconds - daysout * day - hoursout * hour - minutesout * minute;

	printf("duration: %f sec | eta: %02d:%02d:%02d:%02d\n", nseconds_latest_iteration, daysout, hoursout, minutesout, secondsout);

}


template uint16_t getMaxValue(uint16_t** ptr);
template uint32_t getMaxValue(uint32_t** ptr);
template<typename T>T getMaxValue(T** ptr)
{
	return (std::numeric_limits<typename std::remove_pointer<typename std::remove_pointer<T>::type>::type>::max)();
}


template int SaveTokenizer(const char* tokenizer_file_name, std::unordered_map<std::pair<uint32_t, uint32_t>, uint32_t, pair_hash>& merges);
template int SaveTokenizer(const char* tokenizer_file_name, std::unordered_map<std::pair<uint16_t, uint16_t>, uint16_t, pair_hash>& merges);
template<typename Dtype>
int SaveTokenizer(const char* tokenizer_file_name, std::unordered_map<std::pair<Dtype, Dtype>, Dtype, pair_hash>& merges)
{
	int ret;
	size_t num_records;
	int idx;
	unsigned int* buffer;
	num_records = merges.size();

	buffer = new unsigned int[num_records * 3];
	idx = 0;
	for (const auto& entry : merges)
	{
		buffer[idx++] = entry.second;
		buffer[idx++] = entry.first.first;
		buffer[idx++] = entry.first.second;
	}

	ret = WriteDataToFile(tokenizer_file_name, buffer, sizeof(unsigned int) * idx);

	if (ret)
	{
		SpinForEver("Unable to save tokenizer!\n");
	}

	delete buffer;

	return ret;
}


void TestFileStreaming()
{
	FILE* fp_in;
	FILE* fp_out;
	char buff[10000] = { 0 };
	uint32_t buffer_sz = sizeof(buff) - 1;
	uint32_t bytes_written = 0;
	fopen_s(&fp_out, "d:\\xfer\\tiny2\\TinyStories_500k.csv", "wb");

	fp_in = ReadFirstFileBuffer("d:\\xfer\\tiny2\\TinyStories.csv", buff, &buffer_sz, ' ');
	while (true)
	{
		/*
		buff[buffer_sz] = '\0';
		for (int j = 0; j < buffer_sz; j++)
		{
			printf("%c", buff[j]);
		}
		printf("\n");
		*/

		fwrite(buff, sizeof(char), buffer_sz, fp_out);

		bytes_written += buffer_sz;

		if (bytes_written >= 1024 * 1024 * 500)
		{
			//fclose(fp_out);
		}

		int xx;
		buffer_sz = sizeof(buff) - 1;
		memset(buff, 0, sizeof(buff));
		xx = ReadNextBuffer(fp_in, buff, &buffer_sz, ' ');

		if (xx)
		{
			fwrite(buff, sizeof(char), buffer_sz, fp_out);
			fclose(fp_out);
			fclose(fp_in);
			break;
		}
	}

}


// Note: at training time, data type can be uint16_t (to fit in memory) or uint32_t (to support larger vocabulary sizes)
// at run time data type is uint32_t
void TestTokenizer()
{
	unsigned int* buffer;
	size_t size;
	size_t i;
	int idx;

	std::unordered_map<std::pair<unsigned int, unsigned int>, unsigned int, pair_hash> merges;
	std::unordered_map<unsigned int, std::pair<unsigned int, unsigned int>> vocabulary;

	ReadDataFromFile("f:\\src\\bootpin_tokenizer\\data\\tokenizer.bin", (void**)&buffer, &size);

	idx = 0;
	size /= (sizeof(unsigned int) * 3);

	for (i = 0; i < size; i++)
	{
		merges[std::make_pair(buffer[idx + 1], buffer[idx + 2])] = buffer[idx];
		vocabulary[buffer[idx]] = std::make_pair(buffer[idx + 1], buffer[idx + 2]);
		idx += 3;
	}


	unsigned int encoded_text[MAX_REGEX_MATCH_LEN];
	unsigned int encoded_text_len = MAX_REGEX_MATCH_LEN;
	//const char* text = u8"Hi Beryl! aabcaadde this 越 is the way to Abinci";
	//const char* text = "ASCII stands for American Standard Code for Information Interchange";
	//const char* text = "This is the way home.";
	const char* text = "Once upon a time in the West.";

	Encode(text, encoded_text, &encoded_text_len, merges);
	size_t len = strlen(text);

	unsigned int w_buffer_len = 10000;
	wchar_t* w_buffer = new wchar_t[w_buffer_len];
	Decode(encoded_text, encoded_text_len, w_buffer, &w_buffer_len, vocabulary);

}

int SetLocale_threadproc(void* params, int block_index, int total_blocks)
{
	if (!std::setlocale(LC_ALL, "en_US.UTF-8"))
	{
		std::cerr << "Failed to set UTF-8 locale." << std::endl;
		assert(0);
		return -1;
	}

	return 0;
}



namespace fs = std::filesystem;
char** GetTrainingFileNames(const char* training_set_folder, uint32_t* num_files)
{
	size_t len;
	uint32_t file_count;
	char** file_names = nullptr;

	file_count = 0;
	for (const auto& entry : fs::directory_iterator(training_set_folder))
	{
		len = strlen(training_set_folder) + strlen(entry.path().filename().string().c_str());
		file_names = (char**)BlockRealloc(file_names, sizeof(char*) * (file_count), sizeof(char*) * (file_count + 1));
		file_names[file_count] = new char[len + 1];
		sprintf_s(file_names[file_count], len + 1, "%s%s", training_set_folder, entry.path().filename().string().c_str());
		file_count++;
	}

	*num_files = file_count;
	return file_names;
}

