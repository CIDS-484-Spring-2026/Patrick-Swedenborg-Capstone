default:
	clear
	flex -l scanner.l
	bison -dv parser.y
	gcc logic.c parser.tab.c lex.yy.c -lfl -o shell
