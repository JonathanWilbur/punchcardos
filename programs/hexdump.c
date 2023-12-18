/*
Hexdump utility.

Source: https://github.com/Acedev003/hexdump/tree/9485208205262e52f358e6d01e7cb937fd0cfa5f

MIT License

Copyright (c) 2022 Acedev003

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include<ctype.h>
#include<stdio.h>
#include<stdint.h>
#include<sys/stat.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include<errno.h>

#define HEXDUMP_VERSION "0.1.0"
/* Poor man's partial <stdbool.h> for pre-C99 standard libraries. */
#ifndef __bool_true_false_are_defined
#define false 0
#define true 1
#endif

int print_hex(
    char *input_file_name,
    uint64_t start_byte,
    uint64_t no_of_bytes,
    char *output_file_name
);

#define LINE_COUNTER_WIDTH   8
#define BYTES_PER_LINE       16
#define PRINTF_BUFFER_SIZE   3200

/* Prints out the ASCII-view sidebar in canonical mode
 * e.g. |hello..| for 'h','e','l','l','o','\r','\n'.
 * Does not include a trailing newline. */
void print_sidebar(FILE *output_stream, char *line, int line_len)
{
    int i;
    fputc('|', output_stream);
    for (i = 0; i < line_len; ++i) {
        fputc(isprint(line[i]) ? line[i] : '.', output_stream);
    }
    fputc('|', output_stream);
}
    
int print_hex(char *input_file_name, uint64_t start_byte, uint64_t no_of_bytes, char *output_file_name)
{
    FILE *input_stream;
    FILE *output_stream;
    struct stat file_data;  
    input_stream = fopen(input_file_name,"rb");
    if (input_stream == NULL)
    {
        fprintf(stderr, "Error: Failed to open file [%s]\n", input_file_name);
        return -1;
    }

    if (output_file_name == NULL)
    {
        output_stream = stdout;
    }
    else
    {
        output_stream = fopen(output_file_name,"wb");
        if(output_stream == NULL)
        {
            fprintf(stderr, "Error: Failed to open file [%s]\n", output_file_name);
            return -1;
        }
    }

    fprintf(output_stream,"Hexdump:v%s \n\n",HEXDUMP_VERSION);
    fprintf(output_stream,"File : %s\n",input_file_name);
    if (stat(input_file_name, &file_data) == 0)
    {
        fprintf(output_stream, "Size : %ld bytes\n\n",file_data.st_size);
    }
    else
    {
        fprintf(stderr, "Error: Failed to parse file size");
        return -1;
    }
    
    if (file_data.st_size >= UINT64_MAX-1000)
    {
        fprintf(stderr, "\nWARN: Too large file. Exiting....\n");
        return 0;
    }

    if ((fseek(input_stream, start_byte, SEEK_SET) != 0) || (start_byte > file_data.st_size ))
    {
        fprintf(stderr,"Error: Failed to set start position at byte %ld\n", start_byte);
        return -1;
    }

    if (no_of_bytes <= 0)
    {
        no_of_bytes = file_data.st_size;
    }

    int int_byte;               // Refer NOTES (LL1)
    int line_cursor   = 0;

    uint64_t byte_cursor = ftell(input_stream); 
    uint64_t line_count = byte_cursor / BYTES_PER_LINE;

    char printf_buffer[PRINTF_BUFFER_SIZE];
    unsigned char line[BYTES_PER_LINE];                     // Refer NOTES (LL2)

    setvbuf(output_stream, printf_buffer, _IOFBF, sizeof(printf_buffer));

    if (byte_cursor % BYTES_PER_LINE != 0)
    {
        int stop_execution = false;

        for(uint64_t i = 0; i < BYTES_PER_LINE; i++)
        {
            line[i] = 0;   
        }

        for (uint64_t i = byte_cursor % BYTES_PER_LINE; i < BYTES_PER_LINE; i++)
        {
            line[i] = fgetc(input_stream);
            no_of_bytes--;
            if(line[i] == EOF || no_of_bytes == 0)
            {
                stop_execution = true;
                break;
            }       
        }

        fprintf(output_stream,
            "%.*lx  %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x  %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x  ",
            LINE_COUNTER_WIDTH, line_count,
            line[0],line[1],line[2],line[3],line[4],line[5],line[6],line[7],
            line[8],line[9],line[10],line[11],line[12],line[13],line[14],line[15]
        );

        print_sidebar(output_stream, line, 16);
        fputc('\n', output_stream);
        line_count++;
        if (stop_execution)
        {
            return 0;
        }
    }

    while ((int_byte = fgetc(input_stream)) != EOF && no_of_bytes != 0)
    {   
        if (line_cursor == BYTES_PER_LINE)
        {
            fprintf(output_stream,
                "%.*lx  %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x  %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x  ",
                LINE_COUNTER_WIDTH, line_count,
                line[0],line[1],line[2],line[3],line[4],line[5],line[6],line[7],
                line[8],line[9],line[10],line[11],line[12],line[13],line[14],line[15]
                );
            print_sidebar(output_stream, line, 16);
            fputc('\n', output_stream);
            line_count  += 16;
            line_cursor  =  0;
            line[line_cursor] = int_byte;
            no_of_bytes--;
            line_cursor++;
        }
        else
        {
            line[line_cursor] = int_byte;
            no_of_bytes--;
            line_cursor++;
        }
        byte_cursor++;
    }
    fflush(output_stream);
    for (int i = line_cursor; i < BYTES_PER_LINE; i++)
    {
        line[i] = 0;
    }
    fprintf(output_stream,
        "%.*lx  %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x  %.2x %.2x %.2x %.2x %.2x %.2x %.2x %.2x  ",
        LINE_COUNTER_WIDTH, line_count,
        line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7],
        line[8], line[9], line[10], line[11], line[12], line[13], line[14], line[15]);
    print_sidebar(output_stream, line, 16);
    fputc('\n', output_stream);
    fclose(input_stream);
    fclose(output_stream);
    return 0;
}

int main(int argc,char* argv[])
{

    uint64_t start_byte  = 0;
    uint64_t no_of_bytes = 0;

    int save_to_file = false;
    int file_name_index = 0;

    char *number_end_ptr;

    if(argc % 2 != 0)
    {
        fprintf(stderr,"\nIncomplete Parameters. Type [%s -h] for all available options.\n",argv[0]);
        return -1;
    }

    for(int i = 1; i < argc; i += 2)
    {
        if((argv[i][0] == '-') && (strlen(argv[i]) == 2))
        {
            switch(argv[i][1])
            {
                case 'h': printf("Usage:\n\n");
                          printf("   %s [options] <file_name>\n\n",argv[0]);
                          printf("Options:\n\n");
                          printf(" -s: skips specified amount of bytes (Default value: 0)\n");
                          printf(" -n: interpret only 'n' bytes of input (Default value: 0) [Prints entire file if value is 0 or below]\n");
                          printf(" -f: Saves output into the specified file\n");
                          printf(" -h: View help\n");
                          return 0;

                case 's': start_byte = strtoull(argv[i+1],&number_end_ptr,10);
                          if(number_end_ptr == argv[i+1] || *number_end_ptr != '\0')
                          { 
                              fprintf(stderr,"\nError: Failed to parse -s value");
                              return -1;
                          }
                          if((start_byte == UINT64_MAX) && (errno == ERANGE))
                          {
                              fprintf(stderr,"\nError: Input value (-s) out of range");
                              return -1;
                          }
                          break;

                case 'n': no_of_bytes = strtoull(argv[i+1],&number_end_ptr,10);
                          if(number_end_ptr == argv[i+1] || *number_end_ptr != '\0')
                          {
                              fprintf(stderr,"\nError: Failed to parse -n value");
                              return -1;
                          }
                          if((start_byte == UINT64_MAX) && (errno == ERANGE))
                          {
                              fprintf(stderr,"\nError: Input value (-n) out of range");
                              return -1;
                          }
                          break;
                          
                case 'f': save_to_file = true;
                          file_name_index = i + 1;
                          break;
            }
        }
    }

    if (!save_to_file)
    {
        print_hex(argv[argc-1], start_byte, no_of_bytes, NULL);
    }
    else
    {
        print_hex(argv[argc-1], start_byte, no_of_bytes, argv[file_name_index]);
    }

    return 0;
}