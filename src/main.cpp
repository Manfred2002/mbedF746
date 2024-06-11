#include <mbed.h>
#include <threadLvgl.h>
#include "demos/lv_demos.h"
#include <cstdio>
#include <cstdlib> 
#include <ctime>   

ThreadLvgl threadLvgl(30);

lv_obj_t *circle;
lv_timer_t *spawn_timer;

const int screen_width = 480;
const int screen_height = 272;
const int line_offset = 10;
const int num_buttons = 6;

lv_obj_t *score_label = NULL;
int score = 0;
int vitesse = 4000;


DigitalIn buttons[num_buttons] = {DigitalIn(D2, PullUp), DigitalIn(D4, PullUp), DigitalIn(D6, PullUp), DigitalIn(D5, PullUp), DigitalIn(D3, PullUp), DigitalIn(D1, PullUp)};



DigitalOut leds[num_buttons] = {DigitalOut(D8), DigitalOut(D10), DigitalOut(D12), DigitalOut(D11), DigitalOut(D9), DigitalOut(D7)};


void update_score() {
    char score_text[30];
    lv_label_set_recolor(score_label, true);
    sprintf(score_text, "#ffffff Score: %d#", score);
    lv_label_set_text(score_label, score_text);
}

void animate_circle() {
    lv_obj_t *parent = lv_scr_act();

    
    circle = lv_obj_create(parent);
    lv_obj_set_size(circle, 15, 15);  // Taille du cercle
    lv_obj_set_style_bg_color(circle, lv_color_hex(0xFF0000), LV_PART_MAIN);  // rouge
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);  

    lv_obj_set_pos(circle, screen_width / 2, 0);

    int target_position = rand() % num_buttons;

    int target_x = target_position * (screen_width / num_buttons) + (screen_width / (num_buttons * 2));

    lv_obj_set_user_data(circle, (void *)target_x);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, circle);
    lv_anim_set_exec_cb(&a, [](void * var, int32_t v) {
        threadLvgl.lock();
        lv_obj_t *obj = static_cast<lv_obj_t *>(var);
        int32_t target = reinterpret_cast<int32_t>(lv_obj_get_user_data(obj));
        int32_t x = (v / (float)screen_height) * (target - (screen_width/2)) + (screen_width/2);
        lv_obj_set_x(obj, x);
        lv_obj_set_y(obj, v);

        if (v >= screen_height - line_offset - 20 && v <= screen_height - line_offset) {
            int circle_x = lv_obj_get_x(obj);
            int segment = circle_x / (screen_width / num_buttons);
            if (!buttons[segment].read()) {
                lv_obj_t *plus_one = lv_label_create(lv_scr_act());
                lv_label_set_text(plus_one, "+1");
                lv_obj_set_pos(plus_one, circle_x, v - 20);  
                lv_obj_set_style_text_color(plus_one, lv_color_hex(0xFF0FFF), LV_PART_MAIN);
                lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
                    threadLvgl.lock();
                    lv_obj_del(static_cast<lv_obj_t *>(t->user_data));
                    lv_timer_del(t);
                    threadLvgl.unlock();
                }, 500, plus_one);  // Supprimer après 0.5 seconde

                // Mettre à jour le score
                score++;
                lv_obj_del(obj);

                threadLvgl.unlock();
                return;
            }
         
            
        }

        

        // Supprimer le cercle s'il dépasse la ligne en dessous de l'écran
        if (v > screen_height) {
            lv_obj_del(obj);
            score--;

        }
        threadLvgl.unlock();
    });
    lv_anim_set_values(&a, 0, screen_height + line_offset);
    lv_anim_set_time(&a, vitesse);  // Durée de l'animation en millisecondes
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);

    // Animer le cercle pour qu'il se déplace horizontalement vers la position cible
    // lv_anim_t a_x;
    // lv_anim_init(&a_x);
    // lv_anim_set_var(&a_x, circle);
    // lv_anim_set_exec_cb(&a_x, [](void * var, int32_t v) {
    //     lv_obj_t *obj = static_cast<lv_obj_t *>(var);
    //     lv_obj_set_x(obj, v);
    // });
    // lv_anim_set_values(&a_x, screen_width / 2, target_x);
    // lv_anim_set_time(&a_x, 2000);  // Durée de l'animation en millisecondes
    // lv_anim_set_path_cb(&a_x, lv_anim_path_linear);
    // lv_anim_start(&a_x);
}

// Callback pour le timer de spawn
void spawn_timer_cb(lv_timer_t *timer) {
    threadLvgl.lock();
    animate_circle();
    threadLvgl.unlock();
}

// Fonction de callback pour l'événement de clic du bouton
void btn_event_cb(lv_event_t *e) {
    threadLvgl.lock();

    lv_obj_t *btn = lv_event_get_target(e);
    lv_obj_t *screen = lv_scr_act();

    // Masquer le bouton
    lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);

    // Remplacer le fond par une image
    lv_obj_t *img = lv_img_create(screen);
    LV_IMG_DECLARE(image);
    lv_img_set_src(img, &image);
    lv_obj_center(img);

    // Créez le label pour le score
    score_label = lv_label_create(screen);
    lv_obj_align(score_label, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(score_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN); 

    // Démarrer le timer pour animer les cercles à des intervalles réguliers
    spawn_timer = lv_timer_create(spawn_timer_cb, 500, NULL);  // Intervalle de 0.5 secondes

    threadLvgl.unlock();
}

int main() {
    threadLvgl.lock();

    // Initialiser le générateur de nombres aléatoires
    srand(time(NULL));

    // Définir les dimensions de l'écran
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_size(screen, screen_width, screen_height);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(screen);

    // Créez le bouton
    lv_obj_t *btn = lv_btn_create(screen);
    lv_obj_center(btn);

    // Ajouter un label "Start" au bouton
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Start");
    lv_obj_center(label);

    // Ajoutez l'événement de clic au bouton
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);

    // Créez les lignes
    lv_obj_t *line1 = lv_line_create(screen);
    static lv_point_t line1_points[] = {{0, screen_height - line_offset}, {screen_width, screen_height - line_offset}};
    lv_line_set_points(line1, line1_points, 2);
    lv_obj_set_style_line_width(line1, 2, LV_PART_MAIN);
    lv_obj_set_style_line_color(line1, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(line1);

    lv_obj_t *line2 = lv_line_create(screen);
    static lv_point_t line2_points[] = {{0, screen_height + line_offset}, {screen_width, screen_height + line_offset}};
    lv_line_set_points(line2, line2_points, 2);
    lv_obj_set_style_line_width(line2, 2, LV_PART_MAIN);
    lv_obj_set_style_line_color(line2, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_center(line2);
    
    threadLvgl.unlock();

    while (1) {
        // put your main code here, to run repeatedly:
        int compteur=0;

        for (int i = 0; i < num_buttons; i++) {
            if (!buttons[i].read()) {
                leds[i] = 1;
                compteur++;  // Allume la LED correspondante
            } else {
                leds[i] = 0;  // Éteint la LED correspondante
            }
        
        }
        if(compteur >1){
            score--;
        }
        threadLvgl.lock();
        if (score_label) update_score();
        threadLvgl.unlock();
        ThisThread::sleep_for(10ms);
        if(score>=10){
            vitesse= 2000;
        }
        if(score>=50){
            vitesse= 1000;
        }
        if(score>=100){
            vitesse= 500;
        }
        if(score <=0){
            score = 0;
        }
    }
}
