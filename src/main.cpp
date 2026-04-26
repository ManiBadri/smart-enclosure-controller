#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include <TFT_eSPI.h>

#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <Preferences.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define MAX_SLOTS 6

//WiFi
const char* ssid = "money2.4";
const char* password = "money123";

//WiFi LED
const int wifiRedLed = 32;
const int wifiGrnLed = 26;

//DHT
#define DHTPIN 33
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Preferences
Preferences prefs;

//for the temp to switch between fahrenheit and cel
bool useFahrenheit = false;

//TFT + LVGL
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 5];

//Screens
lv_obj_t *main_scrn;
lv_obj_t *settings_scrn;
lv_obj_t *edit_scrn;
lv_obj_t *wifi_scrn;

//Buttons (global for theme)
lv_obj_t *stnBtn;
lv_obj_t *statBtn;
lv_obj_t *home_btn;

//WiFi icon
LV_IMG_DECLARE(wifiON);
LV_IMG_DECLARE(wifiOFF);
lv_obj_t *wifi_img;

//Theme
lv_color_t current_theme_color;

//build screen declarations
void build_wifi_screen();
void build_edit_screen();

void open_wifi_password_popup(char *ssid);

//for the widget slots
lv_obj_t* slot_obj[MAX_SLOTS];   // main widget (arc, bar, etc.)
lv_obj_t* slot_label[MAX_SLOTS]; // for text widgets (optional)

//Slots
enum WidgetType {
    WIDGET_NONE,
    WIDGET_HUMIDITY_ARC,
    WIDGET_HUMIDITY_TEXT,
    WIDGET_TEMP_BAR,
    WIDGET_TEMP_TEXT
};

WidgetType slots[MAX_SLOTS];

//Slot positions for the widgets
lv_point_t slot_pos[MAX_SLOTS] = {
    {50, 60}, {50, 140}, {50, 220},
    {160, 60}, {160, 140}, {160, 220}
};

//--------------------------- DISPLAY ---------------------------
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p){
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

//--------------------------- TOUCH ---------------------------
void my_touch_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data){
    uint16_t x, y;

    if(tft.getTouch(&x, &y)){
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

//--------------------------- SAVE ---------------------------
void save_slots(){
    for(int i = 0; i < MAX_SLOTS; i++){
        prefs.putUInt(("slot" + String(i)).c_str(), slots[i]);
    }
}

//--------------------------- THEME ---------------------------
void apply_theme_color(lv_color_t color){
    current_theme_color = color;

    if(stnBtn) lv_obj_set_style_bg_color(stnBtn, color, 0);
    if(statBtn) lv_obj_set_style_bg_color(statBtn, color, 0);
    if(home_btn) lv_obj_set_style_bg_color(home_btn, color, 0);
}

struct WifiConnectData {
    char* ssid;
    lv_obj_t* ta;
};


//--------------------------- WIFI Screen UI ---------------------------
void build_wifi_screen(){

    wifi_scrn = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifi_scrn, lv_color_black(), 0);

    //title
    lv_obj_t *title = lv_label_create(wifi_scrn);
    lv_label_set_text(title, "Available WiFi");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    //list container (where they go in)
    lv_obj_t *list = lv_list_create(wifi_scrn);
    lv_obj_set_size(list, 220, 200);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 40);

    //show scanning text
    lv_obj_t *loading = lv_label_create(wifi_scrn);
    lv_label_set_text(loading, "Scanning...");
    lv_obj_align(loading, LV_ALIGN_CENTER, 0, 0);

    lv_scr_load(wifi_scrn);

    //scan networks
    int n = WiFi.scanNetworks(); //what does true do?

    lv_obj_del(loading); // remove "Scanning..."

    if(n == 0){
        lv_list_add_text(list, "No networks found");
    } else {
        String seen[20]; // to make string array and hold all the unique SSID
        int seenCount = 0;
        for(int i = 0; i < n; i++){

            String ssid = WiFi.SSID(i);

            bool duplicate = false;

            for(int j = 0; j < seenCount; j++){
                if(seen[j] == ssid){
                    duplicate = true;
                    break;
                }
            }

            if(duplicate == false){
                seen[seenCount++] = ssid;

                lv_obj_t *btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, ssid.c_str());

                static String ssid_store[20];
                char* ssid_copy = strdup(ssid.c_str());

                lv_obj_add_event_cb(btn, [](lv_event_t * e){
                    char* ssid = (char*)lv_event_get_user_data(e);
                    open_wifi_password_popup(ssid);
                }, LV_EVENT_CLICKED, ssid_copy);
            }


            //delay(500); //testing

            //click event

        }
    }
    

    //back button
    lv_obj_t *back = lv_btn_create(wifi_scrn);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_size(back, 180, 40);

    lv_obj_t *lbl = lv_label_create(back);
    lv_label_set_text(lbl, LV_SYMBOL_HOME);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(back, [](lv_event_t * e){
        lv_scr_load_anim(main_scrn, LV_SCR_LOAD_ANIM_MOVE_BOTTOM, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);

}



//--------------------------- Wifi Password ---------------------------
void open_wifi_password_popup(char *ssid){

    //dark overlay
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bg, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_70, 0);

    //popup container
    lv_obj_t *box = lv_obj_create(bg);
    lv_obj_set_size(box, 200, 180);
    //lv_obj_center(box);
    lv_obj_align(box,0,0,0);
    

    //title
    lv_obj_t *label = lv_label_create(box);
    lv_label_set_text_fmt(label, "Connect to:\n%s", ssid);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);

    //text input
    lv_obj_t *ta = lv_textarea_create(box);
    lv_obj_set_size(ta, 180, 40);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 50);
    lv_textarea_set_password_mode(ta, true);

    //keyboard
    lv_obj_t *kb = lv_keyboard_create(bg);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 10);

    //create data AFTER ta exists
    WifiConnectData* data = new WifiConnectData;
    data->ssid = ssid;
    data->ta = ta;

    //connect button
    lv_obj_t *btn = lv_btn_create(box);
    lv_obj_set_size(btn, 80, 30);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Connect");
    lv_obj_center(btn_lbl);

    //Correct callback
    lv_obj_add_event_cb(btn, [](lv_event_t * e){

        WifiConnectData* data = (WifiConnectData*)lv_event_get_user_data(e);

        const char *password = lv_textarea_get_text(data->ta);

        Serial.print("Connecting to: ");
        Serial.println(data->ssid);

        WiFi.begin(data->ssid, password);

        free(data->ssid);
        delete data;

    }, LV_EVENT_CLICKED, data);
}

//--------------------------- Widget Creation ---------------------------
void create_widget_for_slot(int i){
    switch(slots[i]){
        case WIDGET_HUMIDITY_ARC:{
            lv_obj_t *arc = lv_arc_create(main_scrn);
            lv_obj_set_size(arc, 50, 50);
            lv_obj_set_pos(arc, slot_pos[i].x - 25, slot_pos[i].y - 25);
                
            lv_arc_set_range(arc, 0, 100);
            lv_arc_set_value(arc, 50);
                
            lv_obj_set_style_arc_width(arc, 6, LV_PART_MAIN);
            lv_obj_set_style_arc_width(arc, 6, LV_PART_INDICATOR);
            lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
                
            slot_obj[i] = arc;
            slot_label[i] = NULL;
        } break;

        case WIDGET_HUMIDITY_TEXT:{
            lv_obj_t *lbl = lv_label_create(main_scrn);
            lv_label_set_text(lbl, "Hum: --%");
            lv_obj_set_pos(lbl, slot_pos[i].x - 30, slot_pos[i].y);
            lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

            slot_obj[i] = NULL;
            slot_label[i] = lbl;
        } break;

        case WIDGET_TEMP_BAR:{
            lv_obj_t *bar = lv_bar_create(main_scrn);
            lv_obj_set_size(bar, 10, 80);
            lv_obj_set_pos(bar, slot_pos[i].x, slot_pos[i].y - 40);

            //dif range for temp bard depending on C or F
            if(slots[i] == WIDGET_TEMP_BAR && slot_obj[i]){

                if(useFahrenheit){
                    lv_bar_set_range(slot_obj[i], 10, 40);
                } else {
                    lv_bar_set_range(slot_obj[i], 50, 104);
                }
            }
            lv_bar_set_value(bar, 20, LV_ANIM_OFF);

            slot_obj[i] = bar;
            slot_label[i] = NULL;
        } break;

        case WIDGET_TEMP_TEXT:{
            lv_obj_t *lbl = lv_label_create(main_scrn);
            lv_label_set_text(lbl, "Temp: --C");
            lv_obj_set_pos(lbl, slot_pos[i].x - 30, slot_pos[i].y);
            lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

            slot_obj[i] = NULL;
            slot_label[i] = lbl;
        } break;

        default: break;
    }
}

//--------------------------- MAIN SCREEN ---------------------------
void build_main_screen(){
    lv_obj_clean(main_scrn);

    lv_obj_set_style_bg_color(main_scrn, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(main_scrn, LV_OPA_COVER, 0);

    for(int i = 0; i < MAX_SLOTS; i++){
        create_widget_for_slot(i);
    }

    //WiFi icon
    wifi_img = lv_img_create(main_scrn);
    lv_obj_align(wifi_img, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_img_set_src(wifi_img, &wifiOFF);

    //WiFi button
    lv_obj_t *btn_wifi = lv_btn_create(main_scrn);
    lv_obj_set_size(btn_wifi, 80, 40);
    lv_obj_align(btn_wifi, LV_ALIGN_BOTTOM_MID, 0, -10); //-10 instead?
    lv_obj_set_style_bg_color(btn_wifi, lv_color_hex(0x0000FF), 0);

    lv_obj_t *lbl_wifi = lv_label_create(btn_wifi);
    lv_label_set_text(lbl_wifi, LV_SYMBOL_WIFI);
    lv_obj_center(lbl_wifi);

    lv_obj_add_event_cb(btn_wifi, [](lv_event_t * e){
        build_wifi_screen();
        lv_scr_load_anim(wifi_scrn, LV_SCR_LOAD_ANIM_MOVE_TOP, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    //Stats button
    lv_obj_t *btn_stat = lv_btn_create(main_scrn);
    lv_obj_set_size(btn_stat, 40, 40);
    lv_obj_align(btn_stat, LV_ALIGN_BOTTOM_RIGHT, 0, -10);
    lv_obj_set_style_bg_color(btn_stat, lv_color_hex(0x0000FF), 0);

    lv_obj_t *stat_lbl = lv_label_create(btn_stat);
    lv_label_set_text(stat_lbl, "Stats");
    lv_obj_center(stat_lbl);


    //Settings button
    stnBtn = lv_btn_create(main_scrn);
    lv_obj_set_size(stnBtn, 40, 40);
    lv_obj_align(stnBtn, LV_ALIGN_BOTTOM_LEFT, 0, -10);

    lv_obj_t *lbl = lv_label_create(stnBtn);
    lv_label_set_text(lbl, LV_SYMBOL_SETTINGS);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(stnBtn, [](lv_event_t * e){
        lv_scr_load_anim(settings_scrn, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    apply_theme_color(current_theme_color);
}

//--------------------------- Event For Clicking Slots ---------------------------
void slot_click_event(lv_event_t * e){
    int index = (int)lv_event_get_user_data(e);

    if(slots[index] == WIDGET_NONE){
        build_edit_screen();
    } else {
        slots[index] = WIDGET_NONE;
        save_slots();
        build_edit_screen();
    }
}

//--------------------------- Add and Delete Widget Screen ---------------------------
void open_add_menu(int slot_index){

    //add selector pop up menu
    lv_obj_t *menu = lv_obj_create(edit_scrn);
    lv_obj_set_size(menu, 200, 220);
    lv_obj_center(menu);
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x383838),LV_PART_MAIN | LV_STATE_DEFAULT); //bg color of pop up menu
    lv_obj_set_style_border_color(menu, lv_color_hex(0xFFFFFF),LV_PART_MAIN | LV_STATE_DEFAULT); //border color

    const char *names[] = {"Hum Arc", "Hum Text", "Temp Bar", "Temp Text"};

    for(int j = 0; j < 4; j++){
        lv_obj_t *btn = lv_btn_create(menu);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 10 + j * 45);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, names[j]);
        lv_obj_center(lbl);

        //PACK slot + type into one int
        int packed = (slot_index << 8) | j;

        lv_obj_add_event_cb(btn, [](lv_event_t * e){

            int packed = (int)lv_event_get_user_data(e);

            int slot = packed >> 8;
            int type = packed & 0xFF;

            slots[slot] = (WidgetType)(type + 1);

            save_slots();
            build_edit_screen();

        }, LV_EVENT_CLICKED, (void*)packed);
    }
}

//--------------------------- Edit Screen UI ---------------------------
//edit screen plus and x signs (for now)
void build_edit_screen(){

    edit_scrn = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(edit_scrn, lv_color_black(), 0);

    for(int i = 0; i < MAX_SLOTS; i++){

        //creating the buttons
        lv_obj_t *btn = lv_btn_create(edit_scrn);
        lv_obj_set_size(btn, 60, 60);
        lv_obj_set_pos(btn, slot_pos[i].x - 30, slot_pos[i].y - 30);

        //colors and border looks bad but 
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000),LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *label = lv_label_create(btn);
        //checking if theres a widget or not
        if(slots[i] == WIDGET_NONE){
            //add widget labe
            lv_label_set_text(label, "+");
            lv_obj_add_event_cb(btn, [](lv_event_t * e){
                int index = (int)lv_event_get_user_data(e);
                open_add_menu(index);
            }, LV_EVENT_CLICKED, (void*)i);
        } else {
            //remove widget label
            lv_label_set_text(label, "X");
            lv_obj_add_event_cb(btn, slot_click_event, LV_EVENT_CLICKED, (void*)i);
        }

        lv_obj_center(label);
    }

    //Back
    lv_obj_t *back = lv_btn_create(edit_scrn);
    lv_obj_align(back, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    lv_obj_t *lbl = lv_label_create(back);
    lv_label_set_text(lbl, LV_SYMBOL_HOME);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(back, [](lv_event_t * e){
        build_main_screen();
        lv_scr_load_anim(main_scrn, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);
}

//--------------------------- SETUP ---------------------------
void setup(){
    Serial.println(esp_reset_reason());

    Serial.begin(115200);

    dht.begin();

    pinMode(wifiRedLed, OUTPUT);
    pinMode(wifiGrnLed, OUTPUT);


    WiFi.begin(ssid, password);

    pinMode(4, OUTPUT);
    digitalWrite(4, HIGH);   //turn ON backlight
    
    
    tft.begin();
    tft.setRotation(0);

    lv_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 5);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);

    uint16_t calData[5];
    //tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    if(prefs.getBool("calibrated", false)){
        prefs.getBytes("calData", calData, sizeof(calData));
        tft.setTouch(calData);
    } else {
        tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
        prefs.putBytes("calData", calData, sizeof(calData));
        prefs.putBool("calibrated", true);
    }
    tft.setTouch(calData);

    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);

    main_scrn = lv_scr_act();
    settings_scrn = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(main_scrn, lv_color_black(), 0);
    lv_obj_set_style_bg_color(settings_scrn, lv_color_black(), 0);

    prefs.begin("ui", false); //*

    for(int i = 0; i < MAX_SLOTS; i++){
        slots[i] = (WidgetType)prefs.getUInt(("slot" + String(i)).c_str(), WIDGET_NONE);
    }

    current_theme_color = lv_color_hex(prefs.getUInt("btn_color", 0x2196F3));

    useFahrenheit = prefs.getBool("useF", false);

    //SETTINGS SCREEN BUTTON

    Serial.println(esp_reset_reason());


    //Title
    lv_obj_t *title = lv_label_create(settings_scrn);
    lv_label_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    //Theme label
    lv_obj_t *theme_label = lv_label_create(settings_scrn);
    lv_label_set_text(theme_label, "Theme:");
    lv_obj_align(theme_label, LV_ALIGN_TOP_LEFT, 10, 50);
    lv_obj_set_style_text_color(theme_label, lv_color_white(), 0);

    //Theme buttons
    lv_obj_t *btn_red = lv_btn_create(settings_scrn);
    lv_obj_set_size(btn_red, 30, 30);
    lv_obj_align(btn_red, LV_ALIGN_TOP_LEFT, 80, 45);
    lv_obj_set_style_bg_color(btn_red, lv_color_hex(0xFF0000), 0);

    lv_obj_t *btn_green = lv_btn_create(settings_scrn);
    lv_obj_set_size(btn_green, 30, 30);
    lv_obj_align(btn_green, LV_ALIGN_TOP_LEFT, 120, 45);
    lv_obj_set_style_bg_color(btn_green, lv_color_hex(0x00FF00), 0);

    lv_obj_t *btn_blue = lv_btn_create(settings_scrn);
    lv_obj_set_size(btn_blue, 30, 30);
    lv_obj_align(btn_blue, LV_ALIGN_TOP_LEFT, 160, 45);
    lv_obj_set_style_bg_color(btn_blue, lv_color_hex(0x0000FF), 0);  //my blue

    //Theme click handler
    auto theme_event = [](lv_event_t * e){
        lv_obj_t *btn = lv_event_get_target(e);
        lv_color_t color = lv_obj_get_style_bg_color(btn, 0);

        apply_theme_color(color);
        prefs.putUInt("btn_color", lv_color_to32(color));
    };

    lv_obj_add_event_cb(btn_red, theme_event, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_green, theme_event, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn_blue, theme_event, LV_EVENT_CLICKED, NULL);

    //Edit Layout button (below theme)
    lv_obj_t *editBtn = lv_btn_create(settings_scrn);
    lv_obj_set_size(editBtn, 140, 40);
    lv_obj_align(editBtn, LV_ALIGN_TOP_MID, 0, 110);

    lv_obj_t *lbl = lv_label_create(editBtn);
    lv_label_set_text(lbl, "Edit Layout");
    lv_obj_center(lbl);

    lv_obj_add_event_cb(editBtn, [](lv_event_t * e){
        build_edit_screen();
        lv_scr_load_anim(edit_scrn, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    //Back button
    home_btn = lv_btn_create(settings_scrn);
    lv_obj_set_size(home_btn, 60, 30);
    lv_obj_align(home_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);

    lv_obj_t *home_lbl = lv_label_create(home_btn);
    lv_label_set_text(home_lbl, LV_SYMBOL_HOME);
    lv_obj_center(home_lbl);

    lv_obj_add_event_cb(home_btn, [](lv_event_t * e){
        lv_scr_load_anim(main_scrn, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    //Temp Unit Label
    lv_obj_t *temp_label = lv_label_create(settings_scrn);
    lv_label_set_text(temp_label, "Fahrenheit");
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 10, 170);
    lv_obj_set_style_text_color(temp_label, lv_color_white(), 0);

    //switch for temp
    lv_obj_t *temp_switch = lv_switch_create(settings_scrn);
    lv_obj_align(temp_switch, LV_ALIGN_TOP_RIGHT, -20, 165);
    lv_obj_set_style_bg_color(temp_switch, lv_color_hex(0x0000FF), LV_PART_INDICATOR | LV_STATE_CHECKED);


    //set initial state
    if(useFahrenheit){
        lv_obj_add_state(temp_switch, LV_STATE_CHECKED);
    }

    //temp switch event
    lv_obj_add_event_cb(temp_switch, [](lv_event_t * e){
        lv_obj_t *sw = lv_event_get_target(e);

        useFahrenheit = lv_obj_has_state(sw, LV_STATE_CHECKED);

        prefs.putBool("useF", useFahrenheit);

    }, LV_EVENT_VALUE_CHANGED, NULL);

    build_main_screen();
    lv_scr_load(main_scrn);

    Serial.println(esp_reset_reason());


    //force first draw
    lv_timer_handler();   
    delay(50);

}


//--------------------------- WiFi ---------------------------
void handleWiFi(){
  if(WiFi.status() != WL_CONNECTED){

      digitalWrite(wifiRedLed, HIGH);
      digitalWrite(wifiGrnLed, LOW);

      lv_img_set_src(wifi_img, &wifiOFF);
  }
  else{
      digitalWrite(wifiGrnLed, HIGH);
      digitalWrite(wifiRedLed, LOW);

      lv_img_set_src(wifi_img, &wifiON);
  }
}

//--------------------------- Humidity Update ---------------------------
void updateHumidity(){
    static uint32_t lastUpdate = 0;

    //update every 4 seconds
    if(millis() - lastUpdate < 4000) return;
    lastUpdate = millis();


    float h = dht.readHumidity();
    if(isnan(h)) return;


    if(h < 0) h = 0;
    if(h > 100) h = 100;


    char buf[32];
    sprintf(buf, "Hum: %.1f%%", h);

    for(int i = 0; i < MAX_SLOTS; i++){

        //update arc widgets
        if(slots[i] == WIDGET_HUMIDITY_ARC && slot_obj[i]){
            lv_arc_set_value(slot_obj[i], (int)(h + 0.5)); // rounded
        }

        //update text widgets
        if(slots[i] == WIDGET_HUMIDITY_TEXT && slot_label[i]){
            lv_label_set_text(slot_label[i], buf);
        }
    }
}

//--------------------------- Temp Update ---------------------------
void updateTemperature(){
    static uint32_t lastUpdate = 0;

    if(millis() - lastUpdate < 4000) return;
    lastUpdate = millis();

    float t = dht.readTemperature();
    if(isnan(t)) return;

    // convert if needed
    if(useFahrenheit){
        t = t * 9.0 / 5.0 + 32.0;
    }



    char buf[32];
    sprintf(buf, "Temp: %.1f%s", t, useFahrenheit ? "F" : "C");

    for(int i = 0; i < MAX_SLOTS; i++){

        //so it depends depending on C and F changing 
        if(slots[i] == WIDGET_TEMP_BAR && slot_obj[i]){
            if(useFahrenheit){
                lv_bar_set_range(slot_obj[i], 50, 104);
            } else {
                lv_bar_set_range(slot_obj[i], 10, 40);
            }

            lv_bar_set_value(slot_obj[i], (int)(t + 0.5), LV_ANIM_OFF);
        }

        if(slots[i] == WIDGET_TEMP_TEXT && slot_label[i]){
            lv_label_set_text(slot_label[i], buf);
        }
    }
}

//--------------------------- LOOP ---------------------------
void loop(){
    lv_timer_handler();
    handleWiFi();

    updateHumidity();
    updateTemperature();


    delay(5);
}