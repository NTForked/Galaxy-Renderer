#include "VertexBuffer.hpp"
#include <stdexcept>

VertexBuffer::VertexBuffer(int lineWidth)
	: _vbo(0)
	, _ibo(0)
	, _vao(0)
	, _vert()
	, _idx()
	, _vertexShader(0)
	, _fragmentShader(0)
	, _shaderProgram(0)
	, _lineWidth(lineWidth)
{}

VertexBuffer::~VertexBuffer()
{}

GLuint VertexBuffer::CreateShader(GLenum shaderType, const char **shaderSource)
{
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, shaderSource, nullptr);
	glCompileShader(shader);
	
	GLint isCompiled = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (isCompiled == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetShaderInfoLog(shader, maxLength, &maxLength, &infoLog[0]);

		// We don't need the shader anymore.
		glDeleteShader(shader);

		throw std::runtime_error("VertecBuffer: Shader compilation failed!");
	}

	return shader;
}

void VertexBuffer::Initialize()
{
	glGenBuffers(1, &_vbo);
	glGenBuffers(1, &_ibo);
	glGenVertexArrays(1, &_vao);

	const char *srcVertex =
		"#version 330 core\n"
		"layout(location = 0) in vec3 position;\n"
		"layout(location = 1) in vec3 color;\n"
		"out vec3 vertexColor;\n"
		"void main()\n"
		"{\n"
		"	gl_Position =  vec4(position, 1);\n"
//		"   gl_Position = gl_ModelViewProjectionMatrix * (vec4(1.0, 0.1, 1.0, 1.0) * gl_Vertex);\n"
		"	vertexColor = color;\n"
		"}\n";
	_vertexShader = CreateShader(GL_VERTEX_SHADER, &srcVertex);

	const char* srcFragment =
		"#version 330 core\n"
//		"out vec4 FragColor;\n"
//		"in vec3 vertexColor;\n"
		"void main()\n"
		"{\n"
//		"	gl_FragColor = vec4(vertexColor, 1.0);\n"
		"	gl_FragColor = vec4(0.0, 1.0,+ 0.0, 1.0);\n"
		"}\n";
	_fragmentShader = CreateShader(GL_FRAGMENT_SHADER, &srcFragment);

	_shaderProgram = glCreateProgram();
	glAttachShader(_shaderProgram, _vertexShader);
	glAttachShader(_shaderProgram, _fragmentShader);
	glLinkProgram(_shaderProgram);

	GLint isLinked = 0;
	glGetProgramiv(_shaderProgram, GL_LINK_STATUS, (int*)&isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint maxLength = 0;
		glGetProgramiv(_shaderProgram, GL_INFO_LOG_LENGTH, &maxLength);

		// The maxLength includes the NULL character
		std::vector<GLchar> infoLog(maxLength);
		glGetProgramInfoLog(_shaderProgram, maxLength, &maxLength, &infoLog[0]);

		// clean up
		glDeleteProgram(_shaderProgram);
		glDeleteShader(_vertexShader);
		glDeleteShader(_fragmentShader);

		throw std::runtime_error("VertexBuffer: shader program linking failed!");
	}

	// Always detach shaders after a successful link.
	glDetachShader(_shaderProgram, _vertexShader);
	glDetachShader(_shaderProgram, _fragmentShader);
}

void VertexBuffer::Release()
{
	glDisableVertexAttribArray(attPosition);
	glDisableVertexAttribArray(attColor);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	if (_vbo != 0)
		glDeleteBuffers(1, &_vbo);

	if (_ibo != 0)
		glDeleteBuffers(1, &_ibo);

	if (_vao != 0)
		glDeleteVertexArrays(1, &_vao);
}


void VertexBuffer::Update(const std::vector<VertexColor>& vert, const std::vector<int>& idx) 
{
	_vert = vert;
	_idx = idx;

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, _vert.size() * sizeof(VertexColor), _vert.data(), GL_STATIC_DRAW);

	glBindVertexArray(_vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);

	// Set up vertex buffer array
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);

	glEnableVertexAttribArray(attPosition);
	glVertexAttribPointer(attPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), 0); 

	glEnableVertexAttribArray(attColor);
	int rgbOffset = offsetof(VertexColor, red);
	glVertexAttribPointer(attColor, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), (GLvoid*)(rgbOffset));


	// Set up index buffer array
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _idx.size() * sizeof(int), _idx.data(), GL_STATIC_DRAW);

	auto error = glGetError();
	if (error != GL_NO_ERROR)
		throw std::runtime_error("VertexBuffer: Cannot create vbo!");

	glBindVertexArray(0);
}

void VertexBuffer::Draw()
{
	glUseProgram(_shaderProgram);
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(0xFFFF);

#ifdef USE_VAO
	glBindVertexArray(_vao);
	glDrawElements(GL_LINE_STRIP, _idx.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
#else
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
	glBindBuffer(GL_ARRAY_BUFFER, _vbo);

	// assign the vertex shader attributes:   Attribute 0: Position
	glEnableVertexAttribArray(attPosition);
	glVertexAttribPointer(attPosition, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), 0);

	// assign the vertex shader attributes:   Attribute 1: Color
	glEnableVertexAttribArray(attColor);
	glVertexAttribPointer(attColor, 3, GL_FLOAT, GL_FALSE, sizeof(VertexColor), (GLvoid*)(offsetof(VertexColor, red)));

	glDrawElements(GL_LINE_STRIP, _idx.size(), GL_UNSIGNED_INT, nullptr);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
#endif

	glDisable(GL_PRIMITIVE_RESTART);
	glUseProgram(0);
}
