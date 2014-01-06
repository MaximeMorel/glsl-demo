////////////////////////////////////////////////////////////////////////////////
// Simple C clone of shadertoy
//
// How to use :
// run ./glsl_demo
// It will automatically scan for *.fs file in the same folder
// Use n for next shader, b for previous shader
// p to pause time
// +- to increase, decrease timestep
// w to toggle fullscreen
//
// If the active shader .fs file is modified, it will recompile the shader.
//
// You can almost copy/paste shadertoy shaders to test (mostly the one wihtout
// attachements, others needs some tweaking)
//
// How to compile :
// gcc glsl.c -lGL -lglut -lGLEW -o glsl-demo && ./glsl-demo
//
// Author : Maxime Morel <maxime.morel69@gmail.com>
////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <GL/glew.h>
//#include <GL/wglew.h>
#include <GL/gl.h>
#include <GL/freeglut.h>
////////////////////////////////////////////////////////////////////////////////
typedef struct _DemoContext
{
  int resx, resy;
  int mousex, mousey;
  int pause;
  float time, timeStep;
  float pos[3];
  char* basePath;
  char** shaderFiles;
  time_t* shaderLastModificationTime;
  int nbShaders;
  int currentShaderId;
  GLuint vbo;
  GLuint ibo;
  GLuint vao;
  GLuint prog;
} DemoContext;
////////////////////////////////////////////////////////////////////////////////
DemoContext gCtx;
////////////////////////////////////////////////////////////////////////////////
char* defaultVertexShader = "#version 430 core \nlayout(location = 0) in vec3 in_position;\nvoid main(){gl_Position = vec4(in_position, 1.0);}";
////////////////////////////////////////////////////////////////////////////////
char** findShaderFiles(const char* dir, int* nb, time_t** shaderLastModificationTime)
{
  DIR* dp = opendir(dir);
  if(dp)
  {
    struct dirent *dirp;
    int nbFiles = 0;
    while((dirp = readdir(dp)) != NULL) // parse directory
    {
      int l = strlen(dirp->d_name);
      if(l >= 4 && dirp->d_name[l-3] == '.' && dirp->d_name[l-2] == 'f' && dirp->d_name[l-1] == 's')
      {
        ++nbFiles;
      }
    }

    int id = 0;
    *nb = nbFiles;
    char** res = (char**)malloc(nbFiles*sizeof(char*));
    *shaderLastModificationTime = (time_t*)malloc(nbFiles*sizeof(time_t));

    rewinddir(dp);
    while((dirp = readdir(dp)) != NULL) // parse directory
    {
      int l = strlen(dirp->d_name);
      if(l >= 4 && dirp->d_name[l-3] == '.' && dirp->d_name[l-2] == 'f' && dirp->d_name[l-1] == 's')
      {
        int len = strlen(dir)+1+strlen(dirp->d_name)+1;
        char* file = (char*)malloc(len*sizeof(char));
        memcpy(file, dir, strlen(dir)+1);
        strcat(file, "/");
        strcat(file, dirp->d_name);

        // modif time
        struct stat buf;
        stat(file, &buf);
        (*shaderLastModificationTime)[id] = buf.st_mtime;

        file[len-4] = '\0';

        res[id++] = file;

        printf("file : %s ,last modif : %d\n", file, buf.st_mtime);
      }
    }
    closedir(dp);

    if(id==0)
    {
      printf("No shader files found !\n");
    }

    return res;
  }

  return NULL;
}
////////////////////////////////////////////////////////////////////////////////
int isShaderFileModified(const char* basepath)
{
  int modif = 0;
  char* file = (char*)malloc(strlen(basepath)+1+3);
  memcpy(file, basepath, strlen(basepath)+1);
  strcat(file, ".fs");

  //modif time
  struct stat buf;
  stat(file, &buf);
  if(buf.st_mtime > gCtx.shaderLastModificationTime[gCtx.currentShaderId])
  {
    gCtx.shaderLastModificationTime[gCtx.currentShaderId] = buf.st_mtime;
    modif = 1;
  }

  free(file);
  return modif;
}
////////////////////////////////////////////////////////////////////////////////
char* textFileRead(char* filename)
{
  FILE* fp;
  char* text = NULL;
  int count = 0;

  if(filename != NULL)
  {
    fp = fopen(filename,"rt");

    if(fp != NULL)
    {
      fseek(fp, 0, SEEK_END);
      count = ftell(fp);
      rewind(fp);

      if(count > 0)
      {
        text = (char*)malloc(sizeof(char) * (count+1));
        count = fread(text,sizeof(char),count,fp);
        text[count] = '\0';
      }
      fclose(fp);
    }
  }
  return text;
}
////////////////////////////////////////////////////////////////////////////////
void checkShaderStatus(GLuint shader)
{
  GLint status = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  if(status == GL_FALSE)
  {
    printf("shader compile failed\n");
    GLsizei logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if(logLength > 0)
    {
      GLsizei charsWritten = 0;
      GLchar* infoLog = (char*)malloc(logLength);
      glGetShaderInfoLog(shader, logLength, &charsWritten, infoLog);
      printf("%s\n", infoLog);
      free(infoLog);
    }
  }
}
////////////////////////////////////////////////////////////////////////////////
GLuint createShader(const char* filename, const char* ext, GLenum type)
{
  char* path = (char*)malloc(strlen(filename)+strlen(ext)+1);
  memcpy(path, filename, strlen(filename)+1);
  strcat(path, ext);
  char* source = textFileRead(path);
  printf("shader compiling : %s\n", path);
  free(path);

  int dontfree = 0;
  if(source == NULL && type == GL_VERTEX_SHADER)
  {
    source = defaultVertexShader;
    dontfree = 1;
  }

  if(source != NULL)
  {
    GLuint shader = glCreateShader(type);
    const char* psource = source;
    glShaderSource(shader, 1, &psource, NULL);
    if(!dontfree) free(source);
    glCompileShader(shader);
    checkShaderStatus(shader);
    return shader;
  }
  return 0;
}
////////////////////////////////////////////////////////////////////////////////
GLuint createProgram(const char* filename)
{
  GLuint prog = glCreateProgram();

  GLuint vs = createShader(filename, ".vs", GL_VERTEX_SHADER);
  GLuint fs = createShader(filename, ".fs", GL_FRAGMENT_SHADER);

  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glDeleteShader(vs);
  glDeleteShader(fs);

  glLinkProgram(prog);

  GLint status = GL_FALSE;
  glGetProgramiv(prog, GL_LINK_STATUS, &status);
  if(status == GL_FALSE)
  {
    printf("shader program link failed\n");
    GLsizei logLength = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if(logLength > 0)
    {
      GLsizei charsWritten = 0;
      GLchar* infoLog = (char *)malloc(logLength);
      glGetProgramInfoLog(prog, logLength, &charsWritten, infoLog);
      printf("%s\n",infoLog);
      free(infoLog);
    }
  }
  //glValidateProgram(prog);
  //glGetProgramiv(prog, GL_VALIDATE_STATUS,&status);
  //printf("program valid : %d \n", status);

  // dump binary
  GLint binLength;
  GLsizei len;
  GLenum binFmt;
  glGetProgramiv(prog, GL_PROGRAM_BINARY_LENGTH, &binLength);
  GLsizei bufsize = binLength;
  void* binary = (char*)malloc(binLength);
  glGetProgramBinary(prog, bufsize, &len, &binFmt, binary);

  FILE* f = fopen("shader.bin", "wb");
  fwrite(binary, binLength, 1, f);
  free(binary);
  fclose(f);

  printf("Shader program binary dump : len:%d - len wrote:%d\n", binLength, len);

  return prog;
}
////////////////////////////////////////////////////////////////////////////////
void resetDemo()
{
  gCtx.time = 0.0f;
}
////////////////////////////////////////////////////////////////////////////////
void reloadShader()
{
  //gCtx.prog = createProgram(gCtx.basePath);
  glDeleteProgram(gCtx.prog);
  gCtx.prog = createProgram(gCtx.shaderFiles[gCtx.currentShaderId]);
}
////////////////////////////////////////////////////////////////////////////////
void initRender()
{
  //gCtx.prog = createProgram(gCtx.basePath);
  gCtx.prog = createProgram(gCtx.shaderFiles[gCtx.currentShaderId]);

  //glEnable(GL_DEPTH_TEST);
  //glEnable(GL_CULL_FACE);
  //glEnable(GL_CULL_FACE);
  //glEnable(GL_MULTISAMPLE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_CULL_FACE);
  glDisable(GL_MULTISAMPLE);
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  glGenVertexArrays(1, &gCtx.vao);
  glBindVertexArray(gCtx.vao);

  glGenBuffers(1, &gCtx.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, gCtx.vbo);
  GLfloat vertices[] = {-1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
  glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

  glGenBuffers(1, &gCtx.ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gCtx.ibo);
  GLubyte indices[] = {0, 1, 2, 0, 2, 3};
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(GLubyte), indices, GL_STATIC_DRAW);

  //glDisableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glBindVertexArray(0);

  printf("glerror init: %d\n", glGetError());
}
////////////////////////////////////////////////////////////////////////////////
int count = 0;
void renderScene()
{
  if(gCtx.pause)
  {
    const struct timespec ts = {0, 15000000};
    nanosleep(&ts, NULL);
    //Sleep(15);
    return;
  }

  //printf("%d \n", glutGet(GLUT_ELAPSED_TIME));
  //struct timeval timeBegin;
  //gettimeofday(&timeBegin, NULL);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(gCtx.prog);
  glUniform1f(0, gCtx.time);
  glUniform2f(1, gCtx.resx, gCtx.resy);
  glUniform4f(2, gCtx.mousex*3.0f, gCtx.mousey*3.0f, gCtx.mousex*3.0f, gCtx.mousey*3.0f);
  glUniform3f(3, gCtx.pos[0], gCtx.pos[1], gCtx.pos[2]);

  glBindVertexArray(gCtx.vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, NULL);
  glBindVertexArray(0);

  glUseProgram(0);

  glutSwapBuffers();

  //struct timeval timeEnd;
  //gettimeofday(&timeEnd, NULL);

  //printf("%d %d \n", timeEnd.tv_sec, timeEnd.tv_usec);

  //const struct timespec ts = {0, };
  //nanosleep(ts, NULL);

  gCtx.time += gCtx.timeStep;

  GLenum glerr = glGetError();
  if(glerr) printf("glerror render: %d\n", glerr);

  if(++count % 50 == 0 && isShaderFileModified(gCtx.shaderFiles[gCtx.currentShaderId]))
    reloadShader();
}
////////////////////////////////////////////////////////////////////////////////
void changeSize(int width, int height)
{
  if(height == 0)
    height = 1;

  gCtx.resx = width;
  gCtx.resy = height;

  glViewport(0, 0, gCtx.resx, gCtx.resy);
}
////////////////////////////////////////////////////////////////////////////////
void processKeys(unsigned char c, int x, int y)
{
  switch(c)
  {
    case 27:
      glutLeaveMainLoop();
      break;
    case 'z':
    case 'Z':
      gCtx.pos[0] += 0.5f;
      break;
    case 's':
    case 'S':
      gCtx.pos[0] -= 0.5f;
      break;
    case 'q':
    case 'Q':
      gCtx.pos[2] += 0.5f;
      break;
    case 'd':
    case 'D':
      gCtx.pos[2] -= 0.5f;
      break;
    case 'r':
    case 'R':
      gCtx.pos[1] += 0.5f;
      break;
    case 'f':
    case 'F':
      gCtx.pos[1] -= 0.5f;
      break;
    case 'i':
    case 'I':
      resetDemo();
      break;
    case 'l':
    case 'L':
      reloadShader();
      break;
    case 'p':
    case 'P':
      gCtx.pause = 1 - gCtx.pause;
      break;
    case '+':
      gCtx.timeStep += 0.01;
      break;
    case '-':
      gCtx.timeStep -= 0.01;
      break;
    case 'n':
    case 'N':
      gCtx.currentShaderId = (gCtx.currentShaderId+1 < gCtx.nbShaders ? ++gCtx.currentShaderId : 0); printf("id : %d \n", gCtx.currentShaderId);
      reloadShader();
    break;
    case 'b':
    case 'B':
      gCtx.currentShaderId = (gCtx.currentShaderId-1 < 0 ? gCtx.nbShaders-1 : --gCtx.currentShaderId); printf("id : %d \n", gCtx.currentShaderId);
      reloadShader();
    break;
    case 'w':
    case 'W':
      glutFullScreenToggle();
      break;
    default:
      break;
  }
}
////////////////////////////////////////////////////////////////////////////////
void processMouseButtons(int button, int state, int x, int y)
{
//printf("mouse buttons : %d %d %d %d\n", button, state, x, y);
}
////////////////////////////////////////////////////////////////////////////////
void processMouseMotion(int x, int y)
{
//printf("mouse move : %d %d\n", x, y);
  gCtx.mousex = x;
  gCtx.mousey = y;
}
////////////////////////////////////////////////////////////////////////////////
void mouseWheel(int button, int state, int x, int y)
{
//printf("mouse wheel : %d %d %d %d\n", button, state, x, y);
}
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
  char* basePath = ".";
  if(argc >= 2)
  {
    basePath = argv[1];
  }
  gCtx.basePath = basePath;
  gCtx.resx = 800;
  gCtx.resy = 600;

  gCtx.pause = 0;
  gCtx.time = 0.0f;
  gCtx.timeStep = 0.01f;

  gCtx.pos[0] = 0;
  gCtx.pos[1] = 0;
  gCtx.pos[2] = 0;

  gCtx.currentShaderId = 0;
  gCtx.shaderFiles = findShaderFiles(gCtx.basePath, &gCtx.nbShaders, &gCtx.shaderLastModificationTime);

  glutInit(&argc, argv);
  //glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

  //glutInitContextVersion(4, 3);
  //glutInitContextProfile(GLUT_CORE_PROFILE);
  //glutInitContextFlags(GLUT_DEBUG);

  glutInitWindowPosition(100, 100);
  glutInitWindowSize(gCtx.resx, gCtx.resy);
  glutCreateWindow("GLSL Demo");

  glutDisplayFunc(renderScene);
  glutReshapeFunc(changeSize);
  glutIdleFunc(renderScene);

  glutKeyboardFunc(processKeys);
  glutMouseFunc(processMouseButtons);
  glutMotionFunc(processMouseMotion);

  glutMouseWheelFunc(mouseWheel);

  glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

  //glewExperimental = GL_TRUE;
  glewInit();

  printf("Vendor: %s\n", glGetString (GL_VENDOR));
  printf("Renderer: %s\n", glGetString (GL_RENDERER));
  printf("Version: %s\n", glGetString (GL_VERSION));
  printf("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));

  initRender();

  glutMainLoop();

  return 0;
}
////////////////////////////////////////////////////////////////////////////////
