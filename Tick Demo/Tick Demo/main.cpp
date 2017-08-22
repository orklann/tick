//
//  main.cpp
//  Tick Demo
//
//  Created by Aaron Elkins on 8/22/17.
//  Copyright © 2017 Aaron Elkins. All rights reserved.
//

//Using SDL, SDL OpenGL, standard IO, and, strings

#include <SDL2/SDL.h>
#include <OpenGL/gl3.h>

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <errno.h>

#include "GLUtil.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/constants.hpp> // glm::pi


#include <tick/2D/line.h>

// Vertice shader
const char * VERTEX_SHADER = R"SHADER(
#version 330 core

layout(location = 0) in vec4 vPosition;

uniform mat4 modelView;
uniform mat4 project;
uniform vec2 u_center;

void
main(){
    vec4 pos = project * modelView * vec4(vPosition.xy / 1.0, 0, 1);
    gl_Position = pos;
}
)SHADER";

// Fragment shader
const char * FRAGMENT_SHADER = R"SHADER(
#version 330 core

#define feather 0.5

uniform vec2 u_center;
uniform float u_radius;
out vec4 fColor;

uniform float u_lineWidth;
void
main(){
    vec2 p = vec2(gl_FragCoord.x, gl_FragCoord.y);
    float dist = distance(p, u_center);
    
    float halfLineWidth = u_lineWidth / 2.0 + 0.5;
    float d = abs(dist - u_radius);
    float alpha = 0.0;
    if (d < halfLineWidth){
        alpha = clamp(abs(d - halfLineWidth) / 1.0, 0.0, 1.0);
    } else {
        alpha = 0.0;
    }
    
    fColor = vec4(0.0, 0.0, 0.0, alpha);
}
)SHADER";

float radius = 50.0;
float lineWidth = 20.0;
glm::vec2 center = glm::vec2(100.0, 200.0);

//Screen dimension constants
const int SCREEN_WIDTH = 640;

const int SCREEN_HEIGHT = 480;

GLuint program;
GLuint vertexbuffer;

glm::vec2 perp(glm::vec2 p) {
    float ty = p[1];
    float y = p[0];
    float x = -1 * ty;
    return glm::vec2(x, y);
}

glm::vec2 unit(glm::vec2 p) {
    float x = p[0];
    float y = p[1];
    float mag = sqrt((x*x) + (y*y));
    return glm::vec2(x/mag, y/mag);
}

glm::vec2 mult(glm::vec2 p, float m) {
    return glm::vec2(p[0] * m, p[1] * m);
}

void initVertices(){
    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);
    
    // Create and compile our GLSL program from the shaders
    program = LoadShaders(VERTEX_SHADER, FRAGMENT_SHADER);
    
    // Arc points
    glm::vec2 p1 = center;
    glm::vec2 vP1P2 = glm::vec2(radius + lineWidth, 0);
    glm::vec2 vP1P3 = glm::vec2(0, -1 * (radius + lineWidth));
    
    glm::vec2 p2 = p1 + vP1P2;
    glm::vec2 p3 = p1 + vP1P3;
    glm::vec2 p4 = p2 + vP1P3;
    
    static const GLfloat g_vertex_buffer_data[] = {
        // triangle 1
        p1[0], p1[1],
        p2[0], p2[1],
        p4[0], p4[1],
        
        // triangle 2
        p1[0], p1[1],
        p3[0], p3[1],
        p4[0], p4[1],
    };
    
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    
}
// End Red book


//Starts up SDL, creates window, and initializes OpenGL
bool init();

//Initializes matrices and clear color
bool initGL();

//Input handler
void handleKeys( unsigned char key, int x, int y );

//Per frame update
void update();

//Renders quad to the screen
void render();

//Frees media and shuts down SDL
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

//Render flag
bool gRender= true;

bool init(){
    //Initialization flag
    bool success = true;
    
    //Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
        success = false;
    }else{
        //Use OpenGL 2.1
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        
        // Turn on AA in SDL
        // OpenGL side must also turn on AA
        // See glEnable(GL_POLYGON_SMOOTH);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,4);
        
        //Create window
        gWindow = SDL_CreateWindow("Stroke Arc", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        if(gWindow == NULL){
            printf( "Window could not be created! SDL Error: %s\n", SDL_GetError());
            success = false;
        }else{
            //Create context
            gContext = SDL_GL_CreateContext(gWindow);
            if(gContext == NULL){
                printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
                success = false;
            }else{
                //Use Vsync
                if(SDL_GL_SetSwapInterval( 1 ) < 0){
                    printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
                }
                
                //Initialize OpenGL
                if(!initGL()){
                    printf("Unable to initialize OpenGL!\n");
                    success = false;
                }
            }
            
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            // Antialiasing
            glEnable(GL_LINE_SMOOTH);
            glEnable(GL_POLYGON_SMOOTH);
            // MSAA
            //glDisable(GL_MULTISAMPLE);
        }
    }
    
    return success;
}

bool initGL()
{
    bool success = true;
    GLenum error = GL_NO_ERROR;
    
    //Initialize clear color
    glClearColor(1.0, 1.0, 1.0, 1);
    
    //Check for error
    error = glGetError();
    if(error != GL_NO_ERROR){
        printf( "Error initializing OpenGL! %d\n", error);
        success = false;
    }
    
    return success;
}

void handleKeys(unsigned char key, int x, int y){
    //Toggle quad
    if(key == 'q'){
        gRender = !gRender;
    }
}

void update(){
    //No per frame update needed
}

void render(){
    if (!gRender) {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);
        return ;
    }
    
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Use our shader
    glUseProgram(program);
    
    // Ortho matrix
    glm::mat4 modelView = glm::mat4(1.0f);
    modelView = glm::translate(modelView, glm::vec3(0.0, 0.0, -1.0f));
    GLint uniModelView = glGetUniformLocation(program, "modelView");
    glUniformMatrix4fv(uniModelView, 1, GL_FALSE, glm::value_ptr(modelView));
    
    
    glm::mat4 ortho = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f, 0.1f, 100.0f);
    GLint uniProj = glGetUniformLocation(program, "project");
    glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(ortho));
    
    GLint uniRadius = glGetUniformLocation(program, "u_radius");
    glUniform1f(uniRadius, radius);
    
    GLint uniLineWidth = glGetUniformLocation(program, "u_lineWidth");
    glUniform1f(uniLineWidth, lineWidth);
    
    GLint uniCenter = glGetUniformLocation(program, "u_center");
    // 400 = canvas height, need to translate origin to lower left
    // because gl_FragCoord is base on lower left origin
    glUniform2f(uniCenter, center[0], SCREEN_HEIGHT - center[1]);
    
    // 1st attribute buffer : vertices
    GLuint VertexPosition_location = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(VertexPosition_location);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glVertexAttribPointer(
                          VertexPosition_location,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
                          2,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          sizeof(GLfloat) * 2,                  // stride
                          (void*)0            // array buffer offset
                          );
    
    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, 6); // 3 indices starting at 0 -> 1 triangle
    
    glDisableVertexAttribArray(0);
}

void close(){
    //Destroy window
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
    
    //Quit SDL subsystems
    SDL_Quit();
}

int main(int argc, char* args[]){
    //Start up SDL and create window
    if(!init()){
        printf( "Failed to initialize!\n" );
    }else{
        //Main loop flag
        bool quit = false;
        
        //Event handler
        SDL_Event e;
        
        //Enable text input
        SDL_StartTextInput();
        
        // Vetext init
        initVertices();
        
        //While application is running
        while(!quit){
            //Handle events on queue
            while(SDL_PollEvent( &e ) != 0){
                //User requests quit
                if(e.type == SDL_QUIT){
                    quit = true;
                }
                //Handle keypress with current mouse position
                else if(e.type == SDL_TEXTINPUT){
                    int x = 0, y = 0;
                    SDL_GetMouseState(&x, &y);
                    handleKeys(e.text.text[ 0 ], x, y);
                }
            }
            
            //Render
            render();
            
            //Update screen
            SDL_GL_SwapWindow(gWindow);
        }
        
        //Disable text input
        SDL_StopTextInput();
    }
    
    //Free resources and close SDL
    close();
    
    
    return 0;
}
