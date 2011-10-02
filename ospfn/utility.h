



int compute_hash(char *data);
char * strToLower(char *str);
char * align_data(char *data);
unsigned int number_width(unsigned int number);
char * substring(const char* str, size_t begin, size_t len);

char * lpad( unsigned int num, unsigned int padNumber);
char * getLocalTimeStamp(void);
char * startLogging(char *loggingDir);
void writeLogg(const char  *file, const char *format, ...);
