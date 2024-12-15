#include "bn_core.h"
#include "bn_fixed_point.h"
#include "bn_keypad.h"
#include "bn_bg_palettes.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_vector.h"
#include "bn_string.h"
#include "bn_random.h"
#include "bn_math.h"  // For bn::abs()
#include "bn_sprite_text_generator.h"            // For text rendering
#include "common_variable_8x8_sprite_font.h"     // Your custom font header
#include "bn_sprite_items_player_ship.h"
#include "bn_sprite_items_enemy_ship.h"
#include "bn_sprite_items_energy_orb.h"
#include "bn_regular_bg_items_space_bg.h"
#include "bn_regular_bg_items_start_bg.h"

enum class GameState
{
    START_SCREEN,
    PLAYING,
    GAME_OVER
};

// Function to detect collision between two sprites
bool sprites_collide(const bn::sprite_ptr& a, const bn::sprite_ptr& b)
{
    bn::fixed dx = bn::abs(a.x() - b.x());
    bn::fixed dy = bn::abs(a.y() - b.y());
    bn::fixed collision_threshold = 16; // Adjust based on sprite sizes
    return (dx < collision_threshold) && (dy < collision_threshold);
}

int main()
{
    bn::core::init();

    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));
    bn::bg_palettes::set_brightness(0);

    GameState game_state = GameState::START_SCREEN;

    // Optional backgrounds
    bn::optional<bn::regular_bg_ptr> start_bg;
    bn::optional<bn::regular_bg_ptr> main_bg;

    // Optional player sprite
    bn::optional<bn::sprite_ptr> player_ship;

    // Vectors to hold enemy ships and energy orbs
    bn::vector<bn::sprite_ptr, 4> enemy_ships;
    bn::vector<bn::sprite_ptr, 4> orbs;

    // Initialize score and spawn variables
    int score = 0;
    int spawn_counter = 0;
    const int spawn_interval = 60;
    static bn::random rng;

    // Create the START SCREEN background
    start_bg = bn::regular_bg_items::start_bg.create_bg(0, 0);

    // Initialize the text generator with your custom font sprite sheet
    // Ensure that 'common::variable_8x8_sprite_font' is correctly defined in the 'common' namespace
    bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);

    // Declare vectors to store text sprites
    bn::vector<bn::sprite_ptr, 32> score_text_sprites;         // Displays the current score
    bn::vector<bn::sprite_ptr, 32> game_over_text_sprites;     // Displays "GAME OVER"
    bn::vector<bn::sprite_ptr, 32> final_score_text_sprites;   // Displays the final score

    while(true)
    {
        if(game_state == GameState::START_SCREEN)
        {
            // Press START to proceed
            if(bn::keypad::start_pressed())
            {
                game_state = GameState::PLAYING;

                // Hide and free the start screen background
                if(start_bg.has_value())
                {
                    start_bg->set_visible(false);
                    start_bg.reset();
                }

                // Create main gameplay background
                main_bg = bn::regular_bg_items::space_bg.create_bg(0, 0);

                // Create player ship sprite at initial position
                player_ship = bn::sprite_items::player_ship.create_sprite(0, 50);
                player_ship->set_z_order(1);

                // Initialize the score display near the top-left corner
                bn::string<16> initial_score_str = "Score: 0";
                text_generator.generate(-100, 60, initial_score_str, score_text_sprites); // x = -100, y = +60
            }
        }
        else if(game_state == GameState::PLAYING)
        {
            // Handle player movement
            if(player_ship.has_value())
            {
                bn::fixed_point pos = player_ship->position();
                if(bn::keypad::left_held() && pos.x() > -112)
                {
                    player_ship->set_x(pos.x() - 1);
                }
                if(bn::keypad::right_held() && pos.x() < 112)
                {
                    player_ship->set_x(pos.x() + 1);
                }
                if(bn::keypad::up_held() && pos.y() > -72)
                {
                    player_ship->set_y(pos.y() - 1);
                }
                if(bn::keypad::down_held() && pos.y() < 72)
                {
                    player_ship->set_y(pos.y() + 1);
                }
            }

            // Handle spawning of enemies and orbs
            spawn_counter++;
            if(spawn_counter > spawn_interval)
            {
                spawn_counter = 0;
                bool spawn_enemy = rng.get_int(2); // 0 or 1
                int random_x = rng.get_int(200) - 100; // Range: -100 to +99
                int top_y = -80;

                if(spawn_enemy)
                {
                    if(enemy_ships.size() < enemy_ships.max_size())
                    {
                        auto new_enemy = bn::sprite_items::enemy_ship.create_sprite(random_x, top_y);
                        new_enemy.set_z_order(1);
                        enemy_ships.push_back(new_enemy);
                    }
                }
                else
                {
                    if(orbs.size() < orbs.max_size())
                    {
                        auto new_orb = bn::sprite_items::energy_orb.create_sprite(random_x, top_y);
                        new_orb.set_z_order(1);
                        orbs.push_back(new_orb);
                    }
                }
            }

            // Move enemies and orbs downward
            for(auto& enemy : enemy_ships)
            {
                enemy.set_y(enemy.y() + 1);
            }
            for(auto& orb : orbs)
            {
                orb.set_y(orb.y() + 1);
            }

            if(player_ship.has_value())
            {
                // Check collisions with enemies
                for(int i = 0; i < enemy_ships.size(); i++)
                {
                    if(sprites_collide(*player_ship, enemy_ships[i]))
                    {
                        game_state = GameState::GAME_OVER;
                        break;
                    }
                }

                // Check collisions with orbs
                for(int i = 0; i < orbs.size(); i++)
                {
                    if(sprites_collide(*player_ship, orbs[i]))
                    {
                        score += 10;

                        // Update the score display
                        // Clear existing score sprites
                        score_text_sprites.clear();

                        // Create updated score string with specified MaxSize
                        bn::string<16> updated_score_str = "Score: ";
                        updated_score_str += bn::to_string<16>(score); // Specify MaxSize

                        // Generate new score sprites
                        text_generator.generate(-100, 60, updated_score_str, score_text_sprites); // x = -100, y = +60

                        // Remove the collected orb by swapping with the last and popping
                        orbs[i] = orbs.back();
                        orbs.pop_back();
                        i--; // Adjust index after removal
                    }
                }
            }

            // Remove enemies that have moved off-screen
            for(int i = 0; i < enemy_ships.size(); i++)
            {
                if(enemy_ships[i].y() > 90)
                {
                    enemy_ships[i] = enemy_ships.back();
                    enemy_ships.pop_back();
                    i--;
                }
            }

            // Remove orbs that have moved off-screen
            for(int i = 0; i < orbs.size(); i++)
            {
                if(orbs[i].y() > 90)
                {
                    orbs[i] = orbs.back();
                    orbs.pop_back();
                    i--;
                }
            }
        }
        else if(game_state == GameState::GAME_OVER)
        {
            // Hide main background and player sprite
            if(main_bg.has_value())
            {
                main_bg->set_visible(false);
            }
            if(player_ship.has_value())
            {
                player_ship->set_visible(false);
            }

            // Hide and clear all enemy ships
            for(auto& e : enemy_ships)
            {
                e.set_visible(false);
            }
            enemy_ships.clear();

            // Hide and clear all orbs
            for(auto& o : orbs)
            {
                o.set_visible(false);
            }
            orbs.clear();

            // Hide the score display
            score_text_sprites.clear();

            // Create "GAME OVER" text slightly below center
            bn::string<16> game_over_str = "GAME OVER";
            text_generator.generate(0, 20, game_over_str, game_over_text_sprites); // y = +20

            // Create final score display directly below "GAME OVER"
            bn::string<16> final_score_str = "Final Score: ";
            final_score_str += bn::to_string<16>(score); // Specify MaxSize
            text_generator.generate(0, 0, final_score_str, final_score_text_sprites); // y = 0

            // Wait for START to reset the game
            while(true)
            {
                bn::core::update();

                if(bn::keypad::start_pressed())
                {
                    // Reset score and game state
                    score = 0;
                    game_state = GameState::START_SCREEN;

                    // Hide game over texts
                    game_over_text_sprites.clear();
                    final_score_text_sprites.clear();

                    // Re-display the start screen background
                    start_bg = bn::regular_bg_items::start_bg.create_bg(0, 0);
                    break; // Exit the GAME_OVER state loop
                }
            }
        }

        bn::core::update();
    }
}
