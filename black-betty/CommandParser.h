#pragma once

#include <stddef.h>

class CommandParser
{
public:
    CommandParser();
    CommandParser(const CommandParser&) = delete;
    CommandParser& operator=(const CommandParser&) = delete;

    void update();

    int get_security_token() const;
    bool validate_security_token(int token) const;

    bool execute(const char* command, bool requires_security_token, char* output, size_t output_size);

private:
    char serialbuffer[96];
    int security_token[2];
    unsigned long security_token_timeout;

    bool main(char** token, const int token_count, char* output, size_t output_size);
    bool get(char** token, const int token_count, char* output, size_t output_size);
    bool set(char** token, const int token_count, char* output, size_t output_size);

    void restart();
};

CommandParser& get_command_parser();
