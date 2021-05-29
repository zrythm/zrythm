%option c++ debug stack noyywrap yylineno

%{

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "Converter.hpp"
#include "BashConverter.hpp"
#include "ZshConverter.hpp"

static std::string buffer,
    command,
    description;
static std::vector<std::string> short_option,
    long_option,
    old_option;
static std::vector<Converter*> converters;

void log(std::string s) {
    //std::cout << s << std::endl;
}

%}

SPACE [ \t]+

%x X_line X_c X_d X_o X_s X_l X_s_quoted X_d_quoted

%%

complete {
    yy_push_state(X_line);
    log("begin line");
}

<X_line>(-c|--command){SPACE}      yy_push_state(X_c);
<X_line>(-d|--description){SPACE}  yy_push_state(X_d);
<X_line>(-o|--old-option){SPACE}   yy_push_state(X_o);
<X_line>(-s|--short-option){SPACE} yy_push_state(X_s);
<X_line>(-l|--long-option){SPACE}  yy_push_state(X_l);

<X_c,X_d,X_o,X_s,X_l>"\'" {
    buffer += yytext;
    yy_push_state(X_s_quoted);
}
<X_s_quoted>"\\\'" buffer += "\'\"\'\"\'";
<X_s_quoted>"\'"   { 
    buffer += yytext;
    yy_pop_state();
}

<X_c,X_d,X_o,X_s,X_l>"\"" {
    buffer += yytext;
    yy_push_state(X_d_quoted);
}
<X_d_quoted>"\\\"" buffer += "\"\'\"\'\"";
<X_d_quoted>"\""   { 
    buffer += yytext;
    yy_pop_state();
}

<X_s_quoted,X_d_quoted>"\["    buffer += "\\\\[";
<X_s_quoted,X_d_quoted>"\]"    buffer += "\\\\]";
<X_s_quoted,X_d_quoted>"\`"    buffer += "\'\"\'\"\'";
<X_s_quoted,X_d_quoted>{SPACE} buffer += " ";
<X_s_quoted,X_d_quoted>.       buffer += yytext;

<X_c>{SPACE}|\n {
    command = buffer;
    buffer = "";
    yy_pop_state();
    if (strcmp(yytext, "\n") == 0) {
        yyless(0);
    }
    log("command: " + command);
}
<X_d>{SPACE}|\n {
    description = buffer;
    buffer = "";
    yy_pop_state();
    if (strcmp(yytext, "\n") == 0) {
        yyless(0);
    }
    log("description: " + description);
}
<X_o>{SPACE}|\n {
    old_option.push_back(buffer);
    buffer = "";
    yy_pop_state();
    if (strcmp(yytext, "\n") == 0) {
        yyless(0);
    }
    log("old_option: " + old_option.back());
}
<X_s>{SPACE}|\n {
    short_option.push_back(buffer);
    buffer = "";
    yy_pop_state();
    if (strcmp(yytext, "\n") == 0) {
        yyless(0);
    }
    log("short_option: " + short_option.back());
}
<X_l>{SPACE}|\n {
    long_option.push_back(buffer);
    buffer = "";
    yy_pop_state();
    if (strcmp(yytext, "\n") == 0) {
        yyless(0);
    }
    log("long_option: " + long_option.back());
}
<X_c,X_d,X_o,X_s,X_l>. buffer += yytext;

<X_line>\n {
    yy_pop_state();

    for (auto &it : converters) {
        it->convert(
            command, 
            description,
            short_option,
            long_option,
            old_option);
    }

    buffer =
        command = 
        description = 
        "";
    short_option.clear();
    long_option.clear();
    old_option.clear();

    log("end line");
}

.|\n {}

%%

int main() { 
    BashConverter bash_converter;
    converters.push_back((Converter*)&bash_converter);

    ZshConverter zsh_converter;
    converters.push_back((Converter*)&zsh_converter);

    FlexLexer* lexer = new yyFlexLexer;
    //lexer->set_debug(1);
    while(lexer->yylex() != 0)
        ;

    return 0;
}
