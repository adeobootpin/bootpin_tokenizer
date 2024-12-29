
class CSVReader
{
public:
	CSVReader() {}
	~CSVReader() {}
	bool Init(const char* file_name, const char* field_name)
	{
		int ret;
		bool bool_ret;
		char buffer[1024];

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
		errno_t err;
		err = fopen_s(&stream_, file_name, "rb");
		if (err)
		{
			bool_ret = false;
			goto Exit;
		}
#else
		stream = fopen(file_name, "rb");
		if (!stream_)
		{
			ret = -1;
			goto Exit;
		}
#endif

		bool_ret = fgets(buffer, sizeof(buffer), stream_);
		if (bool_ret)
		{
			field_index_ = GetFieldIndex(buffer, field_name);
			if (field_index_ < 0)
			{
				return false;
			}
		}
		else
		{
			goto Exit;
		}

		count_ = 0;
		bool_ret = true;
	Exit:
		return bool_ret;
	}

	uint32_t CountQuotes(const char* str) 
	{
		uint32_t count = 0;
		char ch;

		while (true) 
		{
			ch = *str;
			if (ch == '\0')
			{
				break;
			}

			if (ch == '"') 
			{
				++count;
			}
			str++;
		}
		return count;
	}

//#define CONV_CR_LF_TO_LF
	int32_t ReadNextRecord(char* buffer, uint32_t buffer_size, char* scratch, uint32_t scratch_size, char* scratch2, uint32_t scratch2_size)
	{
		int index;
		uint32_t len;
		int ret;
		char* currentRecord;
		uint32_t quote_count;
		uint32_t num_fields;
		char* field;
		int currentRecord_len;
		currentRecord = scratch2;
		currentRecord[0] = '\0';
		quote_count = 0;

		currentRecord_len = 0;

		while (fgets(scratch, scratch_size, stream_))
		{
			if (count_ == 30000)
			{
				fclose(stream_);
				stream_ = nullptr;
				return 0;
			}

			len = (uint32_t)strlen(scratch);

			if (len >= scratch_size - 1)
			{
				printf("Buffer may be too small, increase buffer size to be safe\n");
				ret = -1;
				goto Exit;
			}

			if (len > 0)
			{
#ifdef CONV_CR_LF_TO_LF
				char* ch = &scratch[len - 1];
				if (*ch == '\n' || *ch == '\r')
				{
					while (*ch == '\n' || *ch == '\r')
					{
						ch--;
					}
					ch++;
					*ch = '\0';
				}
				len = ch - scratch;
#endif
				memcpy(&currentRecord[currentRecord_len], scratch, len + 1); //strcat_s(currentRecord, scratch2_size, scratch);
				currentRecord_len += len;

				quote_count += CountQuotes(scratch);
			}

			if (quote_count % 2 == 0)
			{
				len = buffer_size;
				ret = ParseCSVLine(currentRecord, buffer, &len);
				if (!ret)
				{
					count_++;
					ret = (int32_t)len;
					goto Exit;
				}
				else
				{
					assert(0);
					ret = -1;
					goto Exit;
				}
			}
			else
			{
#ifdef CONV_CR_LF_TO_LF
				currentRecord[currentRecord_len++] = '\n';  //strcat_s(currentRecord, scratch2_size, "\n");
				currentRecord[currentRecord_len] = '\0';
#endif
			}
		}

		fclose(stream_);
		stream_ = nullptr;
		ret = 0;

	Exit:
		return ret;
	}

	int ParseCSVLine(char* currentRecord, char* field, uint32_t* field_buffer_len)
	{
		size_t len;
		size_t i;
		bool insideQuotes = false;
		int field_idx;
		int idx;
		int ret;

		field_idx = 0;
		idx = 0;
		len = strlen(currentRecord);

		for (i = 0; i < len; i++)
		{
			char c = currentRecord[i];
			if (c == '"')
			{
				if (insideQuotes && i + 1 < len && currentRecord[i + 1] == '"')
				{
					if (field_idx == field_index_)
					{
						field[idx++] = '"';
					}
					
					++i; // Skip the second quote
				}
				else 
				{
					insideQuotes = !insideQuotes;
				}
			}
			else
			{
				if (c == ',' && !insideQuotes)
				{
					field_idx++;
					if (field_idx > field_index_)
					{
						field[idx] = '\0';
						*field_buffer_len = idx;
						ret = 0;
						goto Exit;
					}
				}
				else
				{
					if (field_idx == field_index_)
					{
						field[idx++] = c;
					}
				}
			}
		}

		ret = -1;
	Exit:
		return ret;
	}


	int GetFieldIndex(char* fields, const char* field_name) 
	{
		char* context = nullptr;
		char* ch;
		int index;
		char tokens[] = ",";

		index = 0;
		ch = strtok_s(fields, tokens, &context);

		while (ch) 
		{
			if (!strcmp(ch, field_name)) 
			{
				return index;
			}
			++index;
			ch = strtok_s(nullptr, tokens, &context);  // Get the next token
		}

		return -1; 
	}
	

private:
	FILE* stream_;
	int field_index_;
	int count_;
};

