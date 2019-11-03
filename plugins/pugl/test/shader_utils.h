/*
  Copyright 2019 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "glad/glad.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint program;
} Program;

static GLuint
compileShader(const char* source, const GLenum type)
{
	GLuint    shader       = glCreateShader(type);
	const int sourceLength = (int)strlen(source);
	glShaderSource(shader, 1, &source, &sourceLength);
	glCompileShader(shader);

	int status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

		char* log = (char*)calloc(1, (size_t)length);
		glGetShaderInfoLog(shader, length, &length, log);
		fprintf(stderr, "error: Failed to compile shader (%s)\n", log);
		free(log);

		return 0;
	}

	return shader;
}

static void
deleteProgram(Program program)
{
	glDeleteShader(program.vertexShader);
	glDeleteShader(program.fragmentShader);
	glDeleteProgram(program.program);
}

static Program
compileProgram(const char* vertexSource, const char* fragmentSource)
{
	static const Program nullProgram = {0, 0, 0};

	Program program = {compileShader(vertexSource, GL_VERTEX_SHADER),
	                   compileShader(fragmentSource, GL_FRAGMENT_SHADER),
	                   glCreateProgram()};

	if (!program.vertexShader || !program.fragmentShader || !program.program) {
		deleteProgram(program);
		return nullProgram;
	}

	glAttachShader(program.program, program.vertexShader);
	glAttachShader(program.program, program.fragmentShader);
	glLinkProgram(program.program);

	GLint status;
	glGetProgramiv(program.program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		glGetProgramiv(program.program, GL_INFO_LOG_LENGTH, &length);

		char* log = (char*)calloc(1, (size_t)length);
		glGetProgramInfoLog(program.program, length, &length, &log[0]);
		fprintf(stderr, "error: Failed to link program (%s)\n", log);
		free(log);

		deleteProgram(program);
		return nullProgram;
	}

	return program;
}
