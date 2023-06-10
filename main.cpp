/**
* Author: Tan Kim
* Assignment: Simple 2D Scene
* Date due: 2023-06-11, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'


// variables for main window
SDL_Window* display_window;
const int WINDOW_WIDTH = 640,
          WINDOW_HEIGHT = 480;

// background color settings
const float BG_RED = 0.1922f,
            BG_BLUE = 0.549f,
            BG_GREEN = 0.9059f,
            BG_OPACITY = 1.0f;

// viewport settings
const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

bool game_is_running = true;    // state of the game

// ------------------------important------------------------ \\
// the file paths used in this code are absolute, not relative
// please change the file paths into relative if there is a failure of loading files
// --------------------------------------------------------- \\
// required file paths
const char V_SHADER_PATH[] = "/Users/tankim/Desktop/SDLSimple/SDLSimple/shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "/Users/tankim/Desktop/SDLSimple/SDLSimple/shaders/fragment_textured.glsl",
CHROME_SPRITE_FILEPATH[] = "/Users/tankim/Desktop/SDLSimple/SDLSimple/sprites/chrome.png",
FIREFOX_SPRITE_FILEPATH[] = "/Users/tankim/Desktop/SDLSimple/SDLSimple/sprites/firefox.png";

// variables for generating texture
const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero

// variables for objects
ShaderProgram chrome, firefox;
glm::mat4 view_matrix, chrome_matrix, firefox_matrix, projection_matrix, trans_matrix;
GLuint chrome_texture_id, firefox_texture_id;

// variable for player controller
SDL_Joystick *player_one_controller;

GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    // Initialise video and joystick subsystems
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    
    // Open the first controller found. Returns null on error
    player_one_controller = SDL_JoystickOpen(0);
    
    display_window = SDL_CreateWindow("Project 1: Simple 2D Scene",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(display_window);
    SDL_GL_MakeCurrent(display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    chrome.Load(V_SHADER_PATH, F_SHADER_PATH);
    firefox.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    chrome_matrix = glm::mat4(1.0f);
    firefox_matrix = glm::mat4(1.0f);
    view_matrix = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    chrome.SetProjectionMatrix(projection_matrix);
    chrome.SetViewMatrix(view_matrix);
    firefox.SetProjectionMatrix(projection_matrix);
    firefox.SetViewMatrix(view_matrix);
    // Notice we haven't set our model matrix yet!
    
    glUseProgram(chrome.programID);
    glUseProgram(firefox.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    chrome_texture_id = load_texture(CHROME_SPRITE_FILEPATH);
    firefox_texture_id = load_texture(FIREFOX_SPRITE_FILEPATH);
    
    // enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    SDL_Event event;
    
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_WINDOWEVENT_CLOSE:
            case SDL_QUIT:
                game_is_running = false;
                break;
            default:
                break;
        }
    }
}

// variables for update()
const float MILLISECONDS_IN_SECOND = 1000.0;
const float CHROME_ROTATE_ANGLE = 30.0f;    // rotation angle per second is 30 degrees
const float CHROME_SCALE_FACTOR = 0.2f; // scale factor per second is 20%
bool is_growing = true; // check if the object has to grow or shrink
const float MAX_SCALE_TIME = 2.0f;  // growth or shrinkage proceeds for up to 2 seconds

const float FIREFOX_TRANSLATE_SPEED = 0.5f; // translation speed is 0.5 per second
const float FIREFOX_ROTATE_ANGLE = 30.0f;   // rotation angle per second is 30 degrees

float ticks = 0.0f; // get the current number of ticks
float previous_ticks = 0.0f;    // get the previous number of ticks
float delta_time = 0.0f;    // the delta time is the difference from the last frame
float time_stack = 0.0f;    // stack time passed since the game starts

void update()
{
    ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    delta_time = ticks - previous_ticks;
    previous_ticks = ticks;
    
    if (time_stack >= MAX_SCALE_TIME){
        time_stack -= MAX_SCALE_TIME;
        is_growing = !is_growing;
    }else{
        time_stack += delta_time;
    }
    
    chrome_matrix = glm::scale(chrome_matrix,
                               glm::vec3(is_growing ? 1.0f + CHROME_SCALE_FACTOR * delta_time :
                                         1.0f - CHROME_SCALE_FACTOR * delta_time,
                                         is_growing ? 1.0f + CHROME_SCALE_FACTOR * delta_time :
                                         1.0f - CHROME_SCALE_FACTOR * delta_time,
                                         0.0f)
                               );
    chrome_matrix = glm::rotate(chrome_matrix,
                                glm::radians(CHROME_ROTATE_ANGLE * delta_time),
                                glm::vec3(0.0f, 0.0f, -1.0f)
                                );
    
    firefox_matrix = glm::rotate(firefox_matrix,
                                 glm::radians(FIREFOX_ROTATE_ANGLE*delta_time),
                                 glm::vec3(0.0f, 0.0f, 1.0f)
                                 );
    firefox_matrix = glm::translate(firefox_matrix,
                                    glm::vec3(FIREFOX_TRANSLATE_SPEED * delta_time, 0.0f, 0.0f)
                                    );
}

void draw_object(ShaderProgram object, glm::mat4 &object_model_matrix, GLuint &object_texture_id)
{
    object.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Vertices
    float vertices[] = {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };
    
    glVertexAttribPointer(chrome.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(chrome.positionAttribute);
    
    glVertexAttribPointer(chrome.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(chrome.texCoordAttribute);
    
    glVertexAttribPointer(firefox.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(firefox.positionAttribute);
    
    glVertexAttribPointer(firefox.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(firefox.texCoordAttribute);
    
    // Bind texture
    draw_object(chrome, chrome_matrix, chrome_texture_id);
    draw_object(firefox, firefox_matrix, firefox_texture_id);
    
    // We disable two attribute arrays now
    glDisableVertexAttribArray(chrome.positionAttribute);
    glDisableVertexAttribArray(chrome.texCoordAttribute);
    glDisableVertexAttribArray(firefox.positionAttribute);
    glDisableVertexAttribArray(firefox.texCoordAttribute);
    
    SDL_GL_SwapWindow(display_window);
}

void shutdown()
{
    SDL_JoystickClose(player_one_controller);
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    initialise();
    
    while (game_is_running)
    {
        process_input();
        update();
        render();
    }
    
    shutdown();
    return 0;
}
