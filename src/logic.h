#ifndef LOGIC_H
#define LOGIC_H

struct parser_ctx
{
	char *argv[256];
	int argc;
	int pipeline;
	int dynamically_allocated;
};

void expand_wildcards(struct parser_ctx *);
void execute_line(char *, struct parser_ctx *);
char *get_prompt();
void expand_variables(struct parser_ctx *);
void init_signals();
void reset_context(struct parser_ctx *);
void pipeline(struct parser_ctx *);
void myexit(struct parser_ctx *);
void update_cwd_env(struct parser_ctx *);
void cd(struct parser_ctx *);
void pwd(struct parser_ctx *);
void echo(struct parser_ctx *);
void export(struct parser_ctx *);
void help();

int main(void);

int call();

#endif
