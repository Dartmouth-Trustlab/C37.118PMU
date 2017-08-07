#ifndef PROCESS_H_
#define PROCESS_H_
size_t process(uint8_t *input, size_t inputsize);
#endif

#ifndef CONFIG_H_
#define CONFIG_H_
int config(uint8_t input[], size_t inputsize);
#endif

#ifndef DATA_H_
#define DATA_H_
int data(uint8_t input[], size_t inputsize);
#endif

#ifndef COMMAND_H_
#define COMMAND_H_
int command(uint8_t input[], size_t inputsize);
#endif

#ifndef HEADER_H_
#define HEADER_H_
int header(uint8_t input[], size_t inputsize);
#endif

#ifndef CHOOSE_H_
#define CHOOSE_H_
int choose(uint8_t input[], size_t inputsize);
#endif