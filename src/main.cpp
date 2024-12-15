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

// Include your auto-generated sprite/background headers:
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

bool sprites_collide(const bn::sprite_ptr& a, const bn::sprite_ptr& b)
{
    bn::fixed dx = bn::abs(a.x() - b.x());
    bn::fixed dy = bn::abs(a.y() - b.y());
    bn::fixed collision_threshold = 16; // adjust to sprite sizes
    return (dx < collision_threshold) && (dy < collision_threshold);
}

int main()
{
    bn::core::init();

    bn::bg_palettes::set_transparent_color(bn::color(0, 0, 0));
    bn::bg_palettes::set_brightness(0);

    GameState game_state = GameState::START_SCREEN;

    // We'll have two backgrounds:
    bn::optional<bn::regular_bg_ptr> start_bg;
    bn::optional<bn::regular_bg_ptr> main_bg;

    bn::optional<bn::sprite_ptr> player_ship;
    bn::vector<bn::sprite_ptr, 4> enemy_ships;
    bn::vector<bn::sprite_ptr, 4> orbs;

    int score = 0;
    int spawn_counter = 0;
    const int spawn_interval = 60;
    static bn::random rng;

    // Create the START SCREEN background:
    start_bg = bn::regular_bg_items::start_bg.create_bg(0, 0);

    while(true)
    {
        if(game_state == GameState::START_SCREEN)
        {
            // Press START to proceed
            if(bn::keypad::start_pressed())
            {
                game_state = GameState::PLAYING;

                // Hide the start screen BG
                if(start_bg.has_value())
                {
                    start_bg->set_visible(false);
                    start_bg.reset(); // not strictly necessary, but let's free it
                }

                // Create main background
                main_bg = bn::regular_bg_items::space_bg.create_bg(0, 0);

                // Create player sprite
                player_ship = bn::sprite_items::player_ship.create_sprite(0, 50);
                player_ship->set_z_order(1);
            }
        }
        else if(game_state == GameState::PLAYING)
        {
            // Movement
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

            // Spawning
            spawn_counter++;
            if(spawn_counter > spawn_interval)
            {
                spawn_counter = 0;
                bool spawn_enemy = rng.get_int(2);
                int random_x = rng.get_int(200) - 100;
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

            // Move / collisions
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
                // Collisions with enemies
                for(int i = 0; i < enemy_ships.size(); i++)
                {
                    if(sprites_collide(*player_ship, enemy_ships[i]))
                    {
                        game_state = GameState::GAME_OVER;
                        break;
                    }
                }
                // Collisions with orbs
                for(int i = 0; i < orbs.size(); i++)
                {
                    if(sprites_collide(*player_ship, orbs[i]))
                    {
                        score += 10;
                        orbs[i] = orbs.back();
                        orbs.pop_back();
                        i--;
                    }
                }
            }

            // Cleanup off-screen
            for(int i = 0; i < enemy_ships.size(); i++)
            {
                if(enemy_ships[i].y() > 90)
                {
                    enemy_ships[i] = enemy_ships.back();
                    enemy_ships.pop_back();
                    i--;
                }
            }
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
            if(main_bg.has_value())
            {
                main_bg->set_visible(false);
            }
            if(player_ship.has_value())
            {
                player_ship->set_visible(false);
            }
            for(auto& e : enemy_ships) { e.set_visible(false); }
            enemy_ships.clear();

            for(auto& o : orbs) { o.set_visible(false); }
            orbs.clear();

            // Wait for START to reset
            while(true)
            {
                bn::core::update();
                if(bn::keypad::start_pressed())
                {
                    score = 0;
                    game_state = GameState::START_SCREEN;

                    // Re-show start screen background
                    start_bg = bn::regular_bg_items::start_bg.create_bg(0, 0);
                    break;
                }
            }
        }

        bn::core::update();
    }
}