/**
* Author: [Antonio Baranes]
* Assignment: Final
* Date due: 2024-12-6, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 3

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <random>
#include <cstdlib>
#include<string>
#include "Entity.h"
#include<chrono>
#include <thread>
#include<random>


// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* player2;
    Entity* platforms;
    Entity* enemies;
    Entity* attack;
    Entity* attack2;
    Entity* background;
    Entity* cannons;
};

enum moment {INTRO, PLAYING, WON, LOST};
moment state = INTRO;

// ––––– CONSTANTS ––––– //
constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

constexpr float BG_RED = 0.1922f,
BG_BLUE = 0.549f,
BG_GREEN = 0.9059f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;
constexpr char SPRITESHEET_FILEPATH[] = "assets/spr_crosshair.png";
constexpr char CROSSHAIR_FILEPATH[] = "assets/spr_crosshair2.png";

constexpr char ENEMY_FILEPATH[] = "assets/spr_saibaman.png";
constexpr char CANNON_FILEPATH[] = "assets/spr_yamcha.png";

constexpr char PLATFORM_FILEPATH[] = "assets/spr_platform.png";
constexpr char FONTSHEET_FILEPATH[] = "assets/font1.png";
constexpr char SOKIDAN_FILEPATH[] = "assets/spr_sokidan.png";
constexpr char SOKIDAN2_FILEPATH[] = "assets/spr_attack2.png";
constexpr char BACKGROUND_FILEPATH[] = "assets/spr_background.png";

constexpr int NUMBER_OF_TEXTURES = 1;
constexpr GLint LEVEL_OF_DETAIL = 0;
constexpr GLint TEXTURE_BORDER = 0;

constexpr int CD_QUAL_FREQ = 44100,
AUDIO_CHAN_AMT = 2,     // stereo
AUDIO_BUFF_SIZE = 4096;

constexpr char BGM_FILEPATH[] = "assets/snd_bgm.mp3",
SFX_FILEPATH[] = "assets/snd_shot.wav",
SFX_FILEPATH2[] = "assets/snd_shot_p2.wav";


constexpr int PLAY_ONCE = 0,    // play once, loop never
NEXT_CHNL = -1,   // next available channel
ALL_SFX_CHNL = -1;

float ANGLE = 0.0F;

Mix_Music* g_music;
Mix_Chunk* g_jump_sfx;
Mix_Chunk* g_shot_sfx;
Mix_Chunk* g_shot_2_sfx;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

ShaderProgram g_shader_program = ShaderProgram();
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

glm::vec3 plat_pos;

GLuint g_font_texture_id;

GLuint g_background_texture_id;

bool pause = false;

int g_enemy_count = 3;
int g_score = 0;
int g_score2 = 0;

glm::vec3 g_player_position = glm::vec3(-2.0f, 0.0f, 0.0f);
glm::vec3 g_player2_position = glm::vec3(2.0f, 0.0f, 0.0f);


constexpr glm::vec3 init_player_position = glm::vec3(0.0f, 3.5f, 0.0f),
          init_attack_position = glm::vec3(-2.0f, -2.0f, 0.0f),
            init_attack2_position = glm::vec3(2.0f, -2.0f, 0.0f);

bool g_attack_active = false;

// Helper function for random float generation
float random_float(float min, float max) {
    static std::random_device rd;  // Seed for randomness
    static std::mt19937 gen(rd()); // Mersenne Twister engine
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

void delay(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);



    stbi_image_free(image);

    return textureID;
}

constexpr int FONTBANK_SIZE = 16;

void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text,
    float font_size, float spacing, glm::vec3 position)
{
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
        texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void won() {}

void lost() {}

void initialize_window() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow("First to 10!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    // ––––– VIDEO ––––– //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ––––– BGM ––––– //
    Mix_OpenAudio(CD_QUAL_FREQ, MIX_DEFAULT_FORMAT, AUDIO_CHAN_AMT, AUDIO_BUFF_SIZE);

    // STEP 1: Have openGL generate a pointer to your music file
    g_music = Mix_LoadMUS(BGM_FILEPATH); // works only with mp3 files

    // STEP 2: Play music
    Mix_PlayMusic(
        g_music,  // music file
        -1        // -1 means loop forever; 0 means play once, look never
    );

    // STEP 3: Set initial volume
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2.0);

    // ––––– SFX ––––– //
    g_jump_sfx = Mix_LoadWAV(SFX_FILEPATH);
    g_shot_sfx = Mix_LoadWAV(SFX_FILEPATH);
    g_shot_2_sfx = Mix_LoadWAV(SFX_FILEPATH2);


    GLuint player_texture_id = load_texture(SPRITESHEET_FILEPATH);
    /*
    int player_walking_animation[4][4] =
    {
        { 1, 5, 9, 13 },  // for George to move to the left,
        { 3, 7, 11, 15 }, // for George to move to the right,
        { 2, 6, 10, 14 }, // for George to move upwards,
        { 0, 4, 8, 12 }   // for George to move downwards
    };
    */
    //int player_walking_animation[1][1]{ {1} };
    //glm::vec3 acceleration = glm::vec3(0.0f, -18.905f, 0.0f);
    
    glm::vec3 acceleration = glm::vec3(0.0f, 0.0f, 0.0f);


    g_state.player = new Entity;
    glm::vec3 player_scaler = glm::vec3(0.7f, 1.0f, 0.0f);
    //g_state.player->set_scale(player_scaler);
    g_state.player->set_texture_id(player_texture_id);
    g_state.player->set_speed(0.05f);
    g_state.player->set_acceleration(acceleration);
    //g_state.player->set_jumping_power(3.0f);
    g_state.player->set_width(1.0f);
    g_state.player->set_height(1.0f);
    g_state.player->set_entity_type(PLAYER);
    g_state.player->set_position(g_player_position);


    GLuint p2_texture_id = load_texture(CROSSHAIR_FILEPATH);
    g_state.player2 = new Entity;
    //g_state.player->set_scale(player_scaler);
    g_state.player2->set_texture_id(p2_texture_id);
    g_state.player2->set_speed(0.05f);
    g_state.player2->set_acceleration(acceleration);
    //g_state.player->set_jumping_power(3.0f);
    g_state.player2->set_width(1.0f);
    g_state.player2->set_height(1.0f);
    g_state.player2->set_entity_type(PLAYER);
    g_state.player2->set_position(g_player2_position);

    GLuint attack_texture_id = load_texture(SOKIDAN_FILEPATH);


    //Sokidan!
    g_state.attack = new Entity;
    //glm::vec3 player_scaler = glm::vec3(0.7f, 1.0f, 0.0f);
    //g_state.player->set_scale(player_scaler);
    g_state.attack->set_texture_id(attack_texture_id);
    g_state.attack->set_speed(1.0f);
    g_state.attack->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
    //g_state.player->set_jumping_power(3.0f);
    g_state.attack->set_width(1.0f);
    g_state.attack->set_height(1.0f);
    g_state.attack->set_entity_type(ATTACK);
    g_state.attack->set_position(glm::vec3(init_attack_position));
    g_state.attack->deactivate();


    GLuint attack2_texture_id = load_texture(SOKIDAN2_FILEPATH);

    g_state.attack2 = new Entity;
    //glm::vec3 player_scaler = glm::vec3(0.7f, 1.0f, 0.0f);
    //g_state.player->set_scale(player_scaler);
    g_state.attack2->set_texture_id(attack2_texture_id);
    g_state.attack2->set_speed(1.0f);
    g_state.attack2->set_acceleration(glm::vec3(0.0f, 0.0f, 0.0f));
    //g_state.player->set_jumping_power(3.0f);
    g_state.attack2->set_width(1.0f);
    g_state.attack2->set_height(1.0f);
    g_state.attack2->set_entity_type(ATTACK);
    g_state.attack2->set_position(glm::vec3(init_attack2_position));
    g_state.attack2->deactivate();

    g_background_texture_id = load_texture(BACKGROUND_FILEPATH);

    g_state.background = new Entity;
    g_state.background->set_texture_id(g_background_texture_id);
    g_state.background->set_position(glm::vec3(0.0f, 0.0f, 0.0f));
    g_state.background->set_width(10.0f);
    g_state.background->set_height(7.5f);
    g_state.background->set_scale(glm::vec3(12.8f, 9.6f, 0.0f));
    g_state.background->update(0.0f, nullptr, nullptr, 0);

    GLuint cannon_texture_id = load_texture(CANNON_FILEPATH);

    g_state.cannons = new Entity[2];
    g_state.cannons[0].set_texture_id(cannon_texture_id);
    g_state.cannons[1].set_texture_id(cannon_texture_id);

    g_state.cannons[0].set_position(init_attack_position);
    g_state.cannons[1].set_position(init_attack2_position);

    g_state.cannons[0].set_entity_type(CANNON);
    g_state.cannons[1].set_entity_type(CANNON);



}

void initialise()
{

    // ––––– PLATFORMS ––––– //
    GLuint platform_texture_id = load_texture(PLATFORM_FILEPATH);

    g_state.platforms = new Entity[3];
    /*
    std::random_device rd;        
    std::mt19937 gen(rd());        
    std::uniform_real_distribution<float> x_dist(-5.0f, 5.0f); // X position range for random spawn

    float random_x = x_dist(gen);
    */
    /**/
    glm::vec3 scaler_platform = glm::vec3(2.2f, 1.0f, 0.0f);

    float platform_width = 0.8f;

    g_state.platforms[0].set_width(platform_width);
    g_state.platforms[0].set_height(0.3f);
    g_state.platforms[0].set_entity_type(PLATFORM);
    g_state.platforms[0].set_texture_id(platform_texture_id);
    //g_state.platforms[0].set_scale(scaler_platform);
    g_state.platforms[0].set_position(glm::vec3(-3.5f, -1.0f, 0.0f));
    
    g_state.platforms[1].set_width(platform_width);
    g_state.platforms[1].set_height(0.3f);
    g_state.platforms[1].set_entity_type(PLATFORM);
    g_state.platforms[1].set_texture_id(platform_texture_id);
    //g_state.platforms[1].set_scale(scaler_platform);
    g_state.platforms[1].set_position(glm::vec3(0.0f, -3.0f, 0.0f));
    
    g_state.platforms[2].set_width(platform_width);
    g_state.platforms[2].set_height(0.3f);
    g_state.platforms[2].set_entity_type(PLATFORM);
    g_state.platforms[2].set_texture_id(platform_texture_id);
    //g_state.platforms[2].set_scale(scaler_platform);
    g_state.platforms[2].set_position(glm::vec3(3.5f, -1.0f, 0.0f));
    

    g_state.platforms[0].update(0.0f, nullptr, nullptr, 0);
    g_state.platforms[1].update(0.0f, nullptr, nullptr, 0);
    g_state.platforms[2].update(0.0f, nullptr, nullptr, 0);
    g_state.platforms[0].deactivate();
    g_state.platforms[1].deactivate();
    g_state.platforms[2].deactivate();

    //Enemies
    GLuint enemy_texture_id = load_texture(ENEMY_FILEPATH);

    g_state.enemies = new Entity[3];
    g_state.enemies[0].set_texture_id(enemy_texture_id);
    int enemy_x = random_float(-5.0, 5.0);
    g_state.enemies->set_position(glm::vec3(enemy_x, 2.5f, 0.0f));
    g_state.enemies[0].set_speed(1.0f);
    g_state.enemies[0].set_entity_type(ENEMY_ROTATOR);
    g_state.enemies[0].update(0.0f, nullptr, nullptr, 0);

    //g_state.enemies[0].deactivate();
    g_state.enemies[1].deactivate();
    g_state.enemies[2].deactivate();

    glm::vec3 acceleration = glm::vec3(0.0f, -18.905f, 0.0f);

    

    // ––––– PLAYER (GEORGE) ––––– //
    


    
    /*
    g_state.player = new Entity(
        player_texture_id,         // texture id
        0.1f,                      // speed
        acceleration,              // acceleration
        3.0f,                      // jumping power
        player_walking_animation,  // animation index sets
        0.0f,                      // animation time
        4,                         // animation frame amount
        0,                         // current animation index
        4,                         // animation column amount
        4,                         // animation row amount
        1.0f,                      // width
        1.0f,                       // height
        PLAYER
    );
    */

    // Jumping
    //g_state.player->set_jumping_power(3.0f);
    //g_state.player->set_acceleration(acceleration);
    //g_state.player->set_position(glm::vec3(0.0, 2.0, 0.0));
    
    g_font_texture_id = load_texture(FONTSHEET_FILEPATH);
    

    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



}

void initalize_objects() {

}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        

        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_r:
                if (!g_attack_active) {
                    Mix_PlayChannel(NEXT_CHNL, g_shot_sfx, 0);

                    //----Bullet Calculations----//
                    g_state.attack->set_position(init_attack_position);

                    // Calculate direction toward the player's position
                    glm::vec3 player_position = g_state.player->get_position();
                    glm::vec3 direction = player_position - init_attack_position;

                    glm::vec3 normalized_direction = glm::normalize(direction);

                    g_state.attack->set_velocity(normalized_direction * 2.0f);

                    g_state.attack->activate();
                    //g_attack_active = true; Spamming is not good!
                }
                // Jump
                if (g_state.player->get_collided_bottom())
                {
                    //g_state.player->jump();
                    Mix_PlayChannel(NEXT_CHNL, g_jump_sfx, 0);
                }
                break;
            case SDLK_l:
                if (!g_attack_active) {
                    g_state.attack2->set_position(init_attack2_position);
                    Mix_PlayChannel(NEXT_CHNL, g_shot_2_sfx, 0);

                    //----Bullet Calculations----//
                    // Calculate direction toward the player's position
                    glm::vec3 player_position = g_state.player2->get_position();
                    glm::vec3 direction = player_position - init_attack2_position;

                    glm::vec3 normalized_direction = glm::normalize(direction);

                    g_state.attack2->set_velocity(normalized_direction * 2.0f);

                    g_state.attack2->activate();
                    //g_attack_active = true; Spamming is not good!
                }
                // Jump
                if (g_state.player->get_collided_bottom())
                {
                    //g_state.player->jump();
                    Mix_PlayChannel(NEXT_CHNL, g_jump_sfx, 0);
                }
                break;

            case SDLK_h:
                // Stop music
                Mix_HaltMusic();
                break;

            case SDLK_p:
                Mix_PlayMusic(g_music, -1);
                break;
            case SDLK_RETURN:
                if (state == INTRO) {
                    state = PLAYING;
                    
                }
                else if (state == WON) {
                    state = INTRO;
                }
                else if (state == LOST) {
                    state = INTRO;
                }
                break;
            default:
                break;
            }

        default:
            break;
        }
    }



    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    if (key_state[SDL_SCANCODE_RIGHT]) {
        g_state.player2->move_right();
    }
    else if (key_state[SDL_SCANCODE_LEFT]) {
        g_state.player2->move_left();
    }
    if (key_state[SDL_SCANCODE_UP]) {
        g_state.player2->move_up();
    }else if (key_state[SDL_SCANCODE_DOWN]) {
        g_state.player2->move_down();
    }
    
    if (key_state[SDL_SCANCODE_A])
    {
        g_state.player->move_left();
    }
    else if (key_state[SDL_SCANCODE_D])
    {
        g_state.player->move_right();
        
    }
    if (key_state[SDL_SCANCODE_W]) {
        g_state.player->move_up();
    }
    else if (key_state[SDL_SCANCODE_S]) {
        g_state.player->move_down();
    }
    

    if (glm::length(g_state.player->get_movement()) > 1.0f)
    {
        g_state.player->normalise_movement();
    }
    if (glm::length(g_state.attack->get_movement()) > 1.0f)
    {
        g_state.attack->normalise_movement();
    }
    if (glm::length(g_state.player2->get_movement()) > 1.0f)
    {
        g_state.player2->normalise_movement();
    }
    if (glm::length(g_state.attack2->get_movement()) > 1.0f)
    {
        g_state.attack2->normalise_movement();
    }
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    g_state.background->render(&g_shader_program);

    if (state == PLAYING) {
        g_state.player->render(&g_shader_program);
        g_state.player2->render(&g_shader_program);
        g_state.attack->render(&g_shader_program);
        g_state.attack2->render(&g_shader_program);

        for (int i = 0; i < PLATFORM_COUNT; i++) {
            if (g_state.platforms[i].get_entity_type() == EntityType::PLATFORM)
                g_state.platforms[i].render(&g_shader_program);
        }
        for (int i = 0; i < 3; i++) {
            g_state.enemies[i].render(&g_shader_program);
        }
        for (int i = 0; i < 2; i++) {
            g_state.cannons[i].render(&g_shader_program);
        }
    }
    
    std::string str_score = "P1 Score: " + std::to_string(g_score);
    std::string str_score2 = "P2 Score " + std::to_string(g_score2);
    if (state == moment::PLAYING) {
        draw_text(&g_shader_program, g_font_texture_id, str_score, 0.3f, 0.05f, (glm::vec3(-4.5f, 3.0f, 0.0f)));
        draw_text(&g_shader_program, g_font_texture_id, str_score2, 0.3f, 0.05f, (glm::vec3(1.0f, 3.0f, 0.0f)));

    }
    else if (state == moment::WON) {
        draw_text(&g_shader_program, g_font_texture_id, "Player1 Won!", 0.5f, 0.05f,
            glm::vec3(-3.0f, 2.0f, 0.0f));
        draw_text(&g_shader_program, g_font_texture_id, "Enter to play again!", 0.3f, 0.05f,
            glm::vec3(-3.5f, 1.0f, 0.0f));
    }
    else if (state == moment::LOST) {
        draw_text(&g_shader_program, g_font_texture_id, "Player2 Won!", 0.5f, 0.05f,
            glm::vec3(-3.0f, 2.0f, 0.0f));
        draw_text(&g_shader_program, g_font_texture_id, "Enter to play again!", 0.3f, 0.05f,
            glm::vec3(-3.5f, 1.0f, 0.0f));
    }
    else if (state == moment::INTRO) {
        draw_text(&g_shader_program, g_font_texture_id, "First to 10!", 0.5f, 0.05f,
            glm::vec3(-3.0f, 2.0f, 0.0f));
        draw_text(&g_shader_program, g_font_texture_id, "P1 WASD and R to Shoot", 0.3f, 0.05f,
            glm::vec3(-3.5f, 1.0f, 0.0f));
        draw_text(&g_shader_program, g_font_texture_id, "P2 ARROWS and L to Shoot", 0.3f, 0.05f,
            glm::vec3(-3.8f, 0.0f, 0.0f));
        draw_text(&g_shader_program, g_font_texture_id, "PRESS ENTER TO START", 0.3f, 0.05f,
            glm::vec3(-3.0f, -1.0f, 0.0f));
    }

    
    SDL_GL_SwapWindow(g_display_window);
}


void update()
{
    //temp bool to tell us to break block under

    if (state == PLAYING) {
        g_state.player->check_collision_y(g_state.platforms, PLATFORM_COUNT);
        g_state.player->check_collision_x(g_state.platforms, PLATFORM_COUNT);


        float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
        float delta_time = ticks - g_previous_ticks;
        g_previous_ticks = ticks;

        delta_time += g_accumulator;

        if (delta_time < FIXED_TIMESTEP)
        {
            g_accumulator = delta_time;
            return;
        }

        while (delta_time >= FIXED_TIMESTEP)
        {
            g_state.player->update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
            g_state.player2->update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
            g_state.attack->update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
            g_state.attack2->update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
            delta_time -= FIXED_TIMESTEP;
            for (int i = 0; i < 3; i++) {
                g_state.enemies[i].update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
            }
            for (int i = 0; i < 2; i++) {
                g_state.cannons[i].update(FIXED_TIMESTEP, NULL, g_state.platforms, PLATFORM_COUNT);
            }
        }

        g_accumulator = delta_time;
        if (ANGLE >= 360.0f) {
            ANGLE = 0.0f;
        }

        bool collided_with_player = g_state.enemies[0].check_collision(g_state.attack);
        bool collided_with_player2 = g_state.enemies[0].check_collision(g_state.attack2);
        bool collided_with_eachother = g_state.attack->check_collision(g_state.attack2) || g_state.attack2->check_collision(g_state.attack);

        float enemy_x = random_float(-5, 5);
        float enemy_y = random_float(-0, 3);
        if (collided_with_player) {
            g_score++;
            if (g_state.enemies[0].get_entity_type() == ENEMY_ROTATOR) {
                g_state.enemies[0].set_entity_type(ENEMY_JUMPER);
            }
            else if (g_state.enemies[0].get_entity_type() == ENEMY_JUMPER) {
                g_state.enemies[0].set_entity_type(ENEMY_FOLLOWER);
                g_state.enemies[0].set_speed(1.0f);
            }
            else if (g_state.enemies[0].get_entity_type() == ENEMY_FOLLOWER) {
                g_state.enemies[0].set_entity_type(ENEMY_ROTATOR);
                g_state.enemies[0].set_speed(1.0f);
            }
            g_state.enemies[0].set_position(glm::vec3(enemy_x, enemy_y, 0.0f));
            g_state.attack->set_position(glm::vec3(0.0f, 10.0f, 0.0f));//offscreen
            g_state.attack->deactivate();

        }
        else if (collided_with_player2) {
            g_score2++;
            if (g_state.enemies[0].get_entity_type() == ENEMY_ROTATOR) {
                g_state.enemies[0].set_entity_type(ENEMY_JUMPER);
            }
            else if (g_state.enemies[0].get_entity_type() == ENEMY_JUMPER) {
                g_state.enemies[0].set_entity_type(ENEMY_FOLLOWER);
                g_state.enemies[0].set_speed(1.0f);
            }
            else if (g_state.enemies[0].get_entity_type() == ENEMY_FOLLOWER) {
                g_state.enemies[0].set_entity_type(ENEMY_ROTATOR);
                g_state.enemies[0].set_speed(1.0f);
            }
            g_state.enemies[0].set_position(glm::vec3(enemy_x, enemy_y, 0.0f));
            g_state.attack2->set_position(glm::vec3(0.0f, 10.0f, 0.0f));//offscreen
            g_state.attack2->deactivate();
        }
        else if (collided_with_eachother) {
            g_state.attack->set_position(glm::vec3(0.0f, 10.0f, 0.0f));//offscreen
            g_state.attack2->set_position(glm::vec3(0.0f, 10.0f, 0.0f));//offscreen
            g_state.attack->deactivate();
            g_state.attack2->deactivate();
        }


        //----Rotation Calculations----//
        glm::vec3 direction = g_state.player->get_position() - g_state.cannons[0].get_position();

        float anglep1 = std::atan2(direction.y, direction.x);

        float anglep1_degrees = glm::degrees(anglep1);
        float smoothing_factor = 0.1f; 
        float rocketAngle = glm::mix(g_state.cannons[0].get_angle(), anglep1, smoothing_factor);
        g_state.cannons[0].set_angle(rocketAngle);


        glm::vec3 direction2 = g_state.player2->get_position() - g_state.cannons[1].get_position();

        float anglep2 = std::atan2(direction2.y, direction2.x);

        float anglep2_degrees = glm::degrees(anglep2);
        float rocketAngle2 = glm::mix(g_state.cannons[1].get_angle(), anglep2, smoothing_factor);
        g_state.cannons[1].set_angle(rocketAngle2);

        // g_state.player->set_acceleration(glm::vec3(0.0f, -18.905f, 0.0f)); //attempting to fix goofy bug

        float plat_pos_x = plat_pos.x;
        bool win_condition = plat_pos_x - 0.5f < g_state.player->get_position().x < plat_pos_x + 0.5f;


        if (g_score >= 10) {
            state = moment::WON;
            g_state.player->set_position(g_player_position);
            g_state.player2->set_position(g_player2_position);
            g_score = 0;
            g_score2 = 0;
        }
        if (g_score2 >= 10) {
            state = moment::LOST;
            g_state.player->set_position(g_player_position);
            g_state.player2->set_position(g_player2_position);
            g_score2 = 0;
            g_score = 0;
        }
        
        //glm::vec3 player_pos = g_state.player->get_position();

    }

}



void shutdown()
{
    SDL_Quit();

    delete[] g_state.platforms;
    delete g_state.player;
    delete g_state.player2;
    delete[] g_state.enemies;
    delete g_state.attack;
    delete g_state.attack2;
    delete g_state.background;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    std::cout << "program started\n";
    initialize_window();
    initialise();

    while (g_game_is_running)
    {
        if (pause == false) {
            process_input();
            update();
        }
        render();
    }

    shutdown();
    return 0;
}