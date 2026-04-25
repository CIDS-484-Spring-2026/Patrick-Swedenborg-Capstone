default:
	clear
	flex scanner.l
	bison -d parser.y
	gcc logic.c parser.tab.c lex.yy.c -lfl -o shell
