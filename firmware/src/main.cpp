#define LV_CONF_INCLUDE_SIMPLE
#include <lvgl.h>
#include <TFT_eSPI.h>

//if adding something that needs to be installed add it to platformio.ini
#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> 
#include <PubSubClient.h>


#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define MAX_SLOTS 6

//network
const char* mqttServer = "192.168.0.135";
const int mqttPort = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

//WiFi
String oldWiFiName = "";

String wifi_ssid = "";
String wifi_password = "";


//--------------------------- PINS ---------------------------
//WiFi LED
const int wifiRedLed = 32;
const int wifiGrnLed = 26;

const int blueLed = 13;
const int redLed = 12;


//pump pin
const int pumpPin = 27;


//Backlight pin
const int tftBackLight = 4;

const int arc_height_width = 50; //for the arc widget 

//Dimentions
//const int screenWidth = 0;
//const int screenHeight = 0;

//Terminal LCD screen
LiquidCrystal_I2C lcd(0x27, 16, 2);

//DHT
#define DHTPIN 33
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//Preferences
Preferences prefs;
Preferences wifiPrefs;
Preferences petPrefs;

//for the temp to switch between fahrenheit and cel
bool useFahrenheit = false;

//writeConfig
const int tftBackLightBrightness = 0;

//TFT + LVGL
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * 5];

//Screens
lv_obj_t *main_scrn;
lv_obj_t *settings_scrn;
lv_obj_t *edit_scrn;
lv_obj_t *wifi_scrn;
lv_obj_t *stat_scrn;
lv_obj_t *edit_pet_scrn;
lv_obj_t *add_pet_scrn;


//Buttons (global for theme)
//need better way later to add buttons
lv_obj_t *stnBtn;
lv_obj_t *statBtn;
lv_obj_t *home_btn;

//WiFi icon
LV_IMG_DECLARE(wifiON);
LV_IMG_DECLARE(wifiOFF);
lv_obj_t *wifi_img;

//Colors
//BUG: color not consistent with border, with lsit and item, check code?
lv_color_t wifi_box_color = lv_color_hex(0x52525c);
lv_color_t border_color = lv_color_hex(0x3cb371);
lv_color_t font_color= lv_color_hex(0xC9C7C7);
lv_color_t red_color = lv_color_hex(0xFF0000);
lv_color_t btn_color = lv_color_hex(0x212496);
lv_color_t btn_color_new = lv_color_hex(0x000000);
lv_color_t btn_border_color = lv_color_hex(0xFFFFFF);

//Styles
lv_style_t def_button_style;


lv_obj_t *temp_graph;
lv_chart_series_t *temp_series; //for the stats screen temp graph
bool stat_screen_initialized = false;

//uint32_t wifi_box_color = 0xff0000;

//Theme
lv_color_t current_theme_color;

//build screen declarations
void build_wifi_screen();
void build_edit_screen();
void build_stat_screen();
void build_main_screen();
void build_settings_screen();
void build_edit_pets_screen();
void build_add_pet_screen();

void load_pets();
int get_pet_count();
int get_first_free_pet_slot();

void build_home_button(lv_obj_t *screen);
void build_scrn_title(lv_obj_t *screen, const char *title_text);
void remove_shadow(lv_obj_t *obj);

void pre_add_menu(int index);
void pre_add_menu_btns(lv_obj_t *menu, const char *name, int j, int destination, int slot);
void open_add_menu_humidity(int slot_index);
void open_add_menu_temp(int slot_index);

void close_menu_cb(lv_event_t *e);

void open_wifi_password_popup(char *ssid);


//MYTESTS
struct Node {
    int data;
    Node* next;
    Node(int data = 0, Node* next = nullptr) : data(data), next(next) {}
};
int counter_off = 0;
int counter_off_limit = 5;
int counter_on = 0;
Node num_log;
int mynum = 50;
lv_obj_t *test_graph;
lv_chart_series_t *test_series;
bool is_on = false;



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

const int first_column_x = SCREEN_WIDTH/4;
const int second_column_x = 3*SCREEN_WIDTH/4;

const int first_row_y = 65;
const int second_row_y = 145;
const int third_row_y = 225;

const int max_pets = 25;
struct Pet{
    String name;
    String species;
    int age;
    String sex;
};
Pet pets[max_pets];

//Slot positions for the widgets
lv_point_t slot_pos[MAX_SLOTS] = {
    {first_column_x, first_row_y}, {first_column_x, second_row_y}, {first_column_x, third_row_y},
    {second_column_x, first_row_y}, {second_column_x, second_row_y}, {second_column_x, third_row_y}
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
};

struct WifiConnectData {
    char* ssid;
    lv_obj_t* ta;
};

struct AddMenuRouteData {
    int slot;
    int destination;
};

struct AddMenuChoiceData {
    int slot;
    WidgetType widgetType;
};

void load_wifi_credentials(){
    wifiPrefs.begin("wifi", false);
    wifi_ssid = wifiPrefs.getString("ssid", wifi_ssid);
    wifi_password = wifiPrefs.getString("password", wifi_password);
    wifiPrefs.end();
}

void save_wifi_credentials(){
    wifiPrefs.begin("wifi", false);
    wifiPrefs.putString("ssid", wifi_ssid);
    wifiPrefs.putString("password", wifi_password);
    wifiPrefs.end();
}

int get_pet_count(){
    int count = 0;
    for(int i = 0; i < max_pets; i++){
        if(pets[i].name.length() == 0) break;
        count++;
    }
    return count;
}

int get_first_free_pet_slot(){
    for(int i = 0; i < max_pets; i++){
        if(pets[i].name.length() == 0) return i;
    }
    return max_pets;
}

void load_pets(){
    petPrefs.begin("pets", false);
    int count = petPrefs.getUInt("count", 0);
    if(count > max_pets) count = max_pets;
    for(int i = 0; i < max_pets; i++){
        if(i < count){
            pets[i].name = petPrefs.getString(("pet" + String(i)).c_str(), "");
        } else {
            pets[i].name = "";
        }
    }
    petPrefs.end();
}

void save_pets(){
    petPrefs.begin("pets", false);
    int count = get_pet_count();
    petPrefs.putUInt("count", count);
    for(int i = 0; i < max_pets; i++){
        petPrefs.putString(("pet" + String(i)).c_str(), pets[i].name);
    }
    petPrefs.end();
}

//--------------------------- WIFI Screen UI ---------------------------
void build_wifi_screen(){

    wifi_scrn = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifi_scrn, lv_color_black(), 0);


    build_scrn_title(wifi_scrn, "Available WiFi");


    //list container (where they go in)
    lv_obj_t *list = lv_list_create(wifi_scrn);  
    lv_obj_set_size(list, 220, 200);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 40);
    //lv_obj_get_style_bg_color(list, wifi_box_color); 
    lv_obj_set_style_bg_color(list, lv_color_white(), 0); //only out bar not the items
    lv_obj_set_style_border_color(list, lv_color_white(), 0);

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
                lv_obj_set_style_bg_color(btn, wifi_box_color, 0);
                //lv_obj_set_style_border_color(btn, wifi_box_color,0);
                lv_obj_set_style_arc_color(btn, wifi_box_color,0);
                
                static String ssid_store[20];
                char* ssid_copy = strdup(ssid.c_str());

                lv_obj_add_event_cb(btn, [](lv_event_t * e){
                    char* ssid = (char*)lv_event_get_user_data(e);
                    open_wifi_password_popup(ssid);
                }, LV_EVENT_CLICKED, ssid_copy);
            }

        }
    }
    
    build_home_button(wifi_scrn);

}


//--------------------------- Wifi Password UI---------------------------
void open_wifi_password_popup(char *ssid){

    //dark overlay
    lv_obj_t *bg = lv_obj_create(lv_scr_act());
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(bg, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_70, 0);

    //popup container
    lv_obj_t *box = lv_obj_create(bg);
    lv_obj_set_size(box, 200, 120);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 0); 
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_get_style_bg_color(box, wifi_box_color);
    
    //lv_obj_align(box,0,0,0);
    
    //title
    lv_obj_t *label = lv_label_create(box);
    lv_label_set_text_fmt(label, "Enter Password:", ssid);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);
    

    //text input
    lv_obj_t *ta = lv_textarea_create(box);
    lv_obj_clear_flag(ta, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ta, 180, 40);
    lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 25);
    lv_textarea_set_password_mode(ta, true);

    //keyboard 
    lv_obj_t *kb = lv_keyboard_create(bg);
    lv_keyboard_set_textarea(kb, ta);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 14); //ngative to bring more up 
    lv_obj_set_size(kb, SCREEN_WIDTH - 6, SCREEN_HEIGHT/2); 
    lv_obj_set_style_opa(kb, LV_OPA_COVER, 0); 
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_SCROLLABLE); 

    //create data after ta exists
    WifiConnectData* data = new WifiConnectData;
    data->ssid = ssid;
    data->ta = ta;

    //connect button
    lv_obj_t *btn = lv_btn_create(box);
    lv_obj_set_size(btn, 80, 20);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);
    remove_shadow(btn);

    lv_obj_t *btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "Connect");
    lv_obj_center(btn_lbl);

    //Correct callback
    //event for wifi connect buttton
    lv_obj_add_event_cb(btn, [](lv_event_t * e){

        WifiConnectData* data = (WifiConnectData*)lv_event_get_user_data(e);

        wifi_ssid = data->ssid;
        wifi_password = lv_textarea_get_text(data->ta);
        save_wifi_credentials();

        //Serial.print("Connecting to: ");
        //Serial.println(data->ssid);

        WiFi.mode(WIFI_STA);
        WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
        
        lcd.clear();
        lcd.print(wifi_password);

        free(data->ssid);
        delete data;

        //after return to main screen
        lv_scr_load_anim(main_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);

    }, LV_EVENT_CLICKED, data);
}



//--------------------------- Widget Creation ---------------------------
void create_widget_for_slot(int i){
    switch(slots[i]){
        case WIDGET_HUMIDITY_ARC:{
            lv_obj_t *arc = lv_arc_create(main_scrn);
            lv_obj_set_size(arc, arc_height_width, arc_height_width);
            lv_obj_set_pos(arc, slot_pos[i].x - arc_height_width/2, slot_pos[i].y - arc_height_width/2);
                
            lv_arc_set_range(arc, 0, 100);
            lv_arc_set_value(arc, 50);
                
            lv_obj_set_style_arc_width(arc, 6, LV_PART_MAIN);
            lv_obj_set_style_arc_width(arc, 6, LV_PART_INDICATOR);
            lv_obj_remove_style(arc, NULL, LV_PART_KNOB);

            lv_obj_t *lbl = lv_label_create(main_scrn);
            lv_label_set_text(lbl, "--%");
            lv_obj_set_style_text_font(lbl, &lv_font_montserrat_10, 0);
            lv_obj_set_pos(lbl, slot_pos[i].x - 15, slot_pos[i].y - 5);
            lv_obj_set_style_text_color(lbl, lv_color_white(), 0);

                
            slot_obj[i] = arc;
            slot_label[i] = lbl;
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

//--------------------------- EDIT PET SCREEN ---------------------------
void build_edit_pets_screen(){

    edit_pet_scrn = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(edit_pet_scrn, lv_color_black(), 0);


    build_scrn_title(edit_pet_scrn, "Pets");


    build_home_button(edit_pet_scrn); //should be a back to settings button instead(?)

    lv_obj_t *add_pet_btn = lv_btn_create(edit_pet_scrn);
    lv_obj_set_size(add_pet_btn, 80, 20);
    lv_obj_align(add_pet_btn, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_style(add_pet_btn, &def_button_style, 0);
    

    lv_obj_t *lbl = lv_label_create(add_pet_btn);
    lv_label_set_text(lbl, "+");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    //lv_obj_set_pos(lbl, slot_pos[i].x - 15, slot_pos[i].y - 5);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(lbl, font_color, 0);

    lv_obj_add_event_cb(add_pet_btn, [](lv_event_t * e){
        build_add_pet_screen();
        lv_scr_load_anim(add_pet_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);



    int petCount = get_pet_count();
    if(petCount == 0){
        lv_obj_t *lbl = lv_label_create(edit_pet_scrn);
        lv_label_set_text(lbl, "No pets added");
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_text_color(lbl, font_color, 0);
    } else {
        lv_obj_t *pet_list = lv_list_create(edit_pet_scrn);  
        lv_obj_set_size(pet_list, 200, 200);
        lv_obj_align(pet_list, LV_ALIGN_TOP_MID, 0, 70);
        lv_obj_set_style_bg_color(pet_list, lv_color_black(), 0); //only out bar not the items
        lv_obj_set_style_border_color(pet_list, lv_color_white(), 0);

        for(int i = 0; i < petCount; i++){
            lv_obj_t *btn = lv_list_add_btn(pet_list, LV_SYMBOL_WIFI, pets[i].name.c_str());
            lv_obj_set_style_text_color(btn, lv_color_black(), 0);
            lv_obj_set_style_arc_rounded(btn, true, 0);
            lv_obj_set_style_arc_width(btn, 30, 0);
            lv_obj_set_style_bg_color(btn, wifi_box_color, 0);
            lv_obj_set_style_arc_color(btn, wifi_box_color, 0);
        }
    }

    lv_scr_load(edit_pet_scrn);

}

static void add_pet_save_cb(lv_event_t * e){
    lv_obj_t *ta = (lv_obj_t*)lv_event_get_user_data(e);
    String name = lv_textarea_get_text(ta);
    if(name.length() == 0) return;

    int slot = get_first_free_pet_slot();
    if(slot >= max_pets) return;

    pets[slot].name = name;
    save_pets();

    build_edit_pets_screen();
    lv_scr_load_anim(edit_pet_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
}

static void pet_name_keyboard_event_cb(lv_event_t * e){
    lv_obj_t *kb = (lv_obj_t*)lv_event_get_target(e);

    if(lv_event_get_code(e) == LV_EVENT_CANCEL){
        lv_obj_del(kb);
    }
}

static void show_pet_name_keyboard_cb(lv_event_t * e){
    lv_obj_t *name_ta = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_t *scrn = lv_obj_get_parent(name_ta);
    lv_obj_t *kb = lv_keyboard_create(scrn);
    lv_keyboard_set_textarea(kb, name_ta);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 15);
    lv_obj_set_size(kb, SCREEN_WIDTH - 6, SCREEN_HEIGHT/2);
    lv_obj_set_style_opa(kb, LV_OPA_COVER, 0);
    lv_obj_clear_flag(kb, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(kb, pet_name_keyboard_event_cb, LV_EVENT_CANCEL, NULL);
}

//--------------------------- ADD PET SCREEN ---------------------------
void build_add_pet_screen(){

    add_pet_scrn = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(add_pet_scrn, lv_color_black(), 0);

    build_scrn_title(add_pet_scrn, "Add Pet");

    lv_obj_t *name_label = lv_label_create(add_pet_scrn);
    lv_label_set_text(name_label, "Pet Name:");
    lv_obj_align(name_label, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_text_color(name_label, font_color, 0);

    lv_obj_t *name_ta = lv_textarea_create(add_pet_scrn);
    lv_obj_set_size(name_ta, 200, 40);
    lv_obj_align(name_ta, LV_ALIGN_TOP_MID, 0, 70);
    lv_textarea_set_one_line(name_ta, true);
    lv_textarea_set_text(name_ta, "");
    lv_obj_set_style_text_color(name_ta, font_color, 0);
    lv_obj_set_style_bg_color(name_ta, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);


    lv_obj_add_event_cb(name_ta, show_pet_name_keyboard_cb, LV_EVENT_CLICKED, name_ta);


    lv_obj_t *save_btn = lv_btn_create(add_pet_scrn);
    lv_obj_set_size(save_btn, 120, 40);
    lv_obj_align(save_btn, LV_ALIGN_TOP_MID, 0, 125);
    lv_obj_add_style(save_btn, &def_button_style, 0);

    lv_obj_t *save_lbl = lv_label_create(save_btn);
    lv_label_set_text(save_lbl, "Save Pet");
    lv_obj_center(save_lbl);

    lv_obj_add_event_cb(save_btn, add_pet_save_cb, LV_EVENT_CLICKED, name_ta);

    build_home_button(add_pet_scrn);
    lv_scr_load(add_pet_scrn);
}

//--------------------------- STAT SCREEN UI ---------------------------
void build_stat_screen(){
    if(stat_screen_initialized) return;

    lv_obj_set_style_bg_color(stat_scrn, lv_color_black(), 0);

    build_scrn_title(stat_scrn, "Stats");


    lv_obj_t *panel = lv_obj_create(stat_scrn);
    lv_obj_set_size(panel, SCREEN_WIDTH, SCREEN_HEIGHT - 80);
    lv_obj_align(panel, LV_ALIGN_CENTER, 0 , -15);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);
    lv_obj_set_style_bg_color(panel, lv_color_black(), 0);
    lv_obj_set_style_border_width(panel, 0, 0);

    temp_graph = lv_chart_create(panel);
    lv_chart_set_type(temp_graph, LV_CHART_TYPE_LINE);
    lv_obj_set_size(temp_graph, SCREEN_WIDTH - 10, 110);
    lv_obj_align(temp_graph, LV_ALIGN_TOP_MID, 0, 0);
    lv_chart_set_point_count(temp_graph, 100);
    lv_chart_set_range(temp_graph, LV_CHART_AXIS_PRIMARY_X, 0, 100);
    lv_chart_set_range(temp_graph, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_update_mode(temp_graph, LV_CHART_UPDATE_MODE_SHIFT);
    lv_obj_set_style_bg_color(temp_graph, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_border_color(temp_graph, lv_color_hex(0x242424), LV_PART_MAIN);
    lv_obj_set_style_line_color(temp_graph, lv_color_hex(0x242424), LV_PART_MAIN);
    temp_series = lv_chart_add_series(temp_graph, btn_color, LV_CHART_AXIS_PRIMARY_Y);


    test_graph = lv_chart_create(panel);
    lv_chart_set_type(test_graph, LV_CHART_TYPE_LINE);
    lv_obj_set_size(test_graph, SCREEN_WIDTH - 10, 110);
    lv_obj_align(test_graph, LV_ALIGN_TOP_MID, 0, 120);
    lv_chart_set_point_count(test_graph, 100);
    lv_chart_set_range(test_graph, LV_CHART_AXIS_PRIMARY_X, 0, 100);
    lv_chart_set_range(test_graph, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_update_mode(test_graph, LV_CHART_UPDATE_MODE_SHIFT);
    lv_obj_set_style_bg_color(test_graph, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_border_color(test_graph, lv_color_hex(0x242424), LV_PART_MAIN);
    lv_obj_set_style_line_color(test_graph, lv_color_hex(0x242424), LV_PART_MAIN);
    test_series = lv_chart_add_series(test_graph, btn_color, LV_CHART_AXIS_PRIMARY_Y);


    build_home_button(stat_scrn);

    stat_screen_initialized = true;

}

//--------------------------- MAIN SCREEN ---------------------------
void build_main_screen(){
    lv_obj_clean(main_scrn); //CHECK: maybe remove (problem)?

    lv_obj_set_style_bg_color(main_scrn, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(main_scrn, LV_OPA_COVER, 0);

    for(int i = 0; i < MAX_SLOTS; i++){
        create_widget_for_slot(i);
    }

    build_scrn_title(main_scrn, "Home");

    //WiFi icon
    wifi_img = lv_img_create(main_scrn);
    lv_obj_align(wifi_img, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_img_set_src(wifi_img, &wifiOFF);


    //WiFi button
    lv_obj_t *btn_wifi = lv_btn_create(main_scrn);
    lv_obj_set_size(btn_wifi, 140, 40);
    lv_obj_align(btn_wifi, LV_ALIGN_BOTTOM_MID, 0, -10); //-10 instead?
    lv_obj_set_style_bg_color(btn_wifi, btn_color, 0);
    remove_shadow(btn_wifi);

    lv_obj_t *lbl_wifi = lv_label_create(btn_wifi);
    lv_label_set_text(lbl_wifi, LV_SYMBOL_WIFI);
    lv_obj_center(lbl_wifi);

    lv_obj_add_event_cb(btn_wifi, [](lv_event_t * e){
        build_wifi_screen();
        lv_scr_load_anim(wifi_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);

    //Stats button
    lv_obj_t *btn_stat = lv_btn_create(main_scrn);
    lv_obj_set_size(btn_stat, 40, 40);
    lv_obj_align(btn_stat, LV_ALIGN_BOTTOM_RIGHT, 0, -10);
    lv_obj_set_style_bg_color(btn_stat, btn_color, 0);
    remove_shadow(btn_stat);
    lv_obj_t *stat_lbl = lv_label_create(btn_stat);
    lv_label_set_text(stat_lbl, "Stats");
    lv_obj_center(stat_lbl);

    lv_obj_add_event_cb(btn_stat, [](lv_event_t * e){
        build_stat_screen();
        lv_scr_load_anim(stat_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);


    //Settings button
    stnBtn = lv_btn_create(main_scrn);
    lv_obj_set_size(stnBtn, 40, 40);
    lv_obj_align(stnBtn, LV_ALIGN_BOTTOM_LEFT, 0, -10);
    lv_obj_set_style_bg_color(stnBtn, btn_color, 0);
    remove_shadow(stnBtn);

    lv_obj_t *lbl = lv_label_create(stnBtn);
    lv_label_set_text(lbl, LV_SYMBOL_SETTINGS);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(stnBtn, [](lv_event_t * e){
        build_settings_screen();
        lv_scr_load_anim(settings_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);

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
        //build_main_screen();
        
    }
}

static void open_add_menu_route_cb(lv_event_t * e){
    AddMenuRouteData* data = (AddMenuRouteData*)lv_event_get_user_data(e);

    if(data == nullptr) return;

    int slot = data->slot;
    int destination = data->destination;
    delete data;

    if(destination == 0){
        open_add_menu_humidity(slot);
    } else if(destination == 1){
        open_add_menu_temp(slot);
    }
}

static void add_widget_choice_cb(lv_event_t * e){
    AddMenuChoiceData* data = (AddMenuChoiceData*)lv_event_get_user_data(e);

    if(data == nullptr) return;

    slots[data->slot] = data->widgetType;
    save_slots();
    delete data;
    build_edit_screen();
}

//--------------------------- Pre menu for adding widgets ---------------------------
void pre_add_menu(int slot_index){

    //add selector pop up menu
    lv_obj_t *menu = lv_obj_create(edit_scrn);
    lv_obj_set_size(menu, 200, 220);
    lv_obj_center(menu);
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x383838),LV_PART_MAIN | LV_STATE_DEFAULT); //bg color of pop up menu
    lv_obj_set_style_border_color(menu, lv_color_hex(0xFFFFFF),LV_PART_MAIN | LV_STATE_DEFAULT); //border color

    const char *names[] = {"Humidity", "Temperature"};

    pre_add_menu_btns(menu, names[0], 0, 0, slot_index);
    pre_add_menu_btns(menu, names[1], 1, 1, slot_index);


    lv_obj_t *back_btn = lv_btn_create(menu);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_set_style_bg_color(back_btn, btn_color, 0);
    remove_shadow(back_btn);

    lv_obj_t *lbl = lv_label_create(back_btn);
    lv_label_set_text(lbl, "Back");
    lv_obj_center(lbl);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e){
        build_edit_screen();
        lv_scr_load_anim(settings_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);
}


//--------------------------- buttons for pre menu ---------------------------
void pre_add_menu_btns(lv_obj_t *menu, const char *name, int j, int destination, int slot){

    lv_obj_t *btn = lv_btn_create(menu);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 10 + j * 45);
    lv_obj_set_style_bg_color(btn, btn_color, 0);
    remove_shadow(btn);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, name);
    lv_obj_center(lbl);

    AddMenuRouteData* data = new AddMenuRouteData{slot, destination};
    lv_obj_add_event_cb(btn, open_add_menu_route_cb, LV_EVENT_CLICKED, data);
}

//--------------------------- Humidity widget selection ---------------------------
void open_add_menu_humidity(int slot_index){

    //add selector pop up menu
    lv_obj_t *menu = lv_obj_create(edit_scrn);
    lv_obj_set_size(menu, 200, 220);
    lv_obj_center(menu);
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x383838),LV_PART_MAIN | LV_STATE_DEFAULT); //bg color of pop up menu
    lv_obj_set_style_border_color(menu, lv_color_hex(0xFFFFFF),LV_PART_MAIN | LV_STATE_DEFAULT); //border color

    const char *names[] = {"Hum Arc", "Hum Text"};
    const WidgetType widgetTypes[] = {WIDGET_HUMIDITY_ARC, WIDGET_HUMIDITY_TEXT};

    for(int j = 0; j < 2; j++){
        lv_obj_t *btn = lv_btn_create(menu);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 10 + j * 45);
        lv_obj_set_style_bg_color(btn, btn_color, 0);
        remove_shadow(btn);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, names[j]);
        lv_obj_center(lbl);

        AddMenuChoiceData* data = new AddMenuChoiceData{slot_index, widgetTypes[j]};
        lv_obj_add_event_cb(btn, add_widget_choice_cb, LV_EVENT_CLICKED, data);
    }

    lv_obj_t *back_btn = lv_btn_create(menu);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_set_style_bg_color(back_btn, btn_color, 0);
    remove_shadow(back_btn);

    lv_obj_t *lbl = lv_label_create(back_btn);
    lv_label_set_text(lbl, "Back");
    lv_obj_center(lbl);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e){
        int slot_index = (int)(intptr_t)lv_event_get_user_data(e);
        pre_add_menu(slot_index);
    }, LV_EVENT_CLICKED, (void*)(intptr_t)slot_index);
}

//--------------------------- Temp widget selection ---------------------------
void open_add_menu_temp(int slot_index){

    //add selector pop up menu
    lv_obj_t *menu = lv_obj_create(edit_scrn);
    lv_obj_set_size(menu, 200, 220);
    lv_obj_center(menu);
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x383838),LV_PART_MAIN | LV_STATE_DEFAULT); //bg color of pop up menu
    lv_obj_set_style_border_color(menu, lv_color_hex(0xFFFFFF),LV_PART_MAIN | LV_STATE_DEFAULT); //border color

    const char *names[] = {"Temp Bar", "Temp Text"};
    const WidgetType widgetTypes[] = {WIDGET_TEMP_BAR, WIDGET_TEMP_TEXT};

    for(int j = 0; j < 2; j++){
        lv_obj_t *btn = lv_btn_create(menu);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 10 + j * 45);
        lv_obj_set_style_bg_color(btn, btn_color, 0);
        remove_shadow(btn);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, names[j]);
        lv_obj_center(lbl);

        AddMenuChoiceData* data = new AddMenuChoiceData{slot_index, widgetTypes[j]};
        lv_obj_add_event_cb(btn, add_widget_choice_cb, LV_EVENT_CLICKED, data);
    }

    lv_obj_t *back_btn = lv_btn_create(menu);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_set_style_bg_color(back_btn, btn_color, 0);
    remove_shadow(back_btn);

    lv_obj_t *lbl = lv_label_create(back_btn);
    lv_label_set_text(lbl, "Back");
    lv_obj_center(lbl);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e){
        int slot_index = (int)(intptr_t)lv_event_get_user_data(e);
        pre_add_menu(slot_index);
    }, LV_EVENT_CLICKED, (void*)(intptr_t)slot_index);


}


//--------------------------- Settings Screen ---------------------------
void build_settings_screen(){
    settings_scrn = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(settings_scrn, lv_color_black(), 0);

    build_scrn_title(settings_scrn, "Settings");

    //convert hard code
    //Edit Layout button (below theme)
    lv_obj_t *edit_btn = lv_btn_create(settings_scrn);
    lv_obj_set_size(edit_btn, 140, 40);
    lv_obj_align(edit_btn, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_style(edit_btn, &def_button_style, 0);
    //lv_obj_set_style_bg_color(editBtn, btn_color, 0);
    //remove_shadow(editBtn);

    lv_obj_t *lbl = lv_label_create(edit_btn);
    lv_label_set_text(lbl, "Edit Layout");
    lv_obj_set_style_text_color(lbl, font_color, 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(edit_btn, [](lv_event_t * e){
        build_edit_screen();
        lv_scr_load_anim(edit_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);


    build_home_button(settings_scrn);

    //Temp Unit Label
    lv_obj_t *temp_label = lv_label_create(settings_scrn);
    lv_label_set_text(temp_label, "Fahrenheit");
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 10, 125);
    lv_obj_set_style_text_color(temp_label, font_color, 0);

    //switch for temp
    lv_obj_t *temp_switch = lv_switch_create(settings_scrn);
    lv_obj_align(temp_switch, LV_ALIGN_TOP_RIGHT, -20, 120);
    lv_obj_set_style_bg_color(temp_switch, btn_color, LV_PART_INDICATOR | LV_STATE_CHECKED);

    //set initial state
    if(useFahrenheit){
        lv_obj_add_state(temp_switch, LV_STATE_CHECKED);
    }

    //Edit Pets button (below theme)
    lv_obj_t *pets_btn = lv_btn_create(settings_scrn);
    lv_obj_set_size(pets_btn, 140, 40);
    lv_obj_align(pets_btn, LV_ALIGN_TOP_MID, 0, 165);
    lv_obj_add_style(pets_btn, &def_button_style, 0);


    lv_obj_t *pet_lbl = lv_label_create(pets_btn);
    lv_label_set_text(pet_lbl, "Edit Pets");
    lv_obj_set_style_text_color(pet_lbl, font_color, 0);
    lv_obj_center(pet_lbl);

    lv_obj_add_event_cb(pets_btn, [](lv_event_t * e){
        build_edit_pets_screen();
        lv_scr_load_anim(edit_pet_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);


    lv_scr_load(settings_scrn);

    //temp switch event
    lv_obj_add_event_cb(temp_switch, [](lv_event_t * e){
        lv_obj_t *sw = lv_event_get_target(e);

        useFahrenheit = lv_obj_has_state(sw, LV_STATE_CHECKED);

        prefs.putBool("useF", useFahrenheit);

    }, LV_EVENT_VALUE_CHANGED, NULL);


}

//--------------------------- Edit Screen UI ---------------------------
//edit screen plus and x signs (for now)
void build_edit_screen(){

    edit_scrn = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(edit_scrn, lv_color_black(), 0);

    for(int i = 0; i < MAX_SLOTS; i++){


        build_scrn_title(edit_scrn, "Modify Widgets");

        //creating the buttons
        lv_obj_t *btn = lv_btn_create(edit_scrn);
        lv_obj_set_size(btn, 60, 60);
        lv_obj_set_pos(btn, slot_pos[i].x - 30, slot_pos[i].y - 30);
        remove_shadow(btn);
        
        //colors and border looks bad but 
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        //Ensure all shadow properties are disabled to remove any visible shadow
        remove_shadow(btn);

        lv_obj_t *label = lv_label_create(btn);
        //lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
        //checking if theres a widget or not
        if(slots[i] == WIDGET_NONE){
            //add widget labe
            lv_label_set_text(label, "+");
            
            lv_obj_align(label, LV_ALIGN_CENTER, 20, 20);
            lv_obj_set_style_text_color(label, font_color, 0);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
            lv_obj_add_event_cb(btn, [](lv_event_t * e){
                int index = (int)lv_event_get_user_data(e);
                //open_add_menu(index);
                pre_add_menu(index); 
            }, LV_EVENT_CLICKED, (void*)i);
        } else {
            //remove widget label
            lv_label_set_text(label, "X");
            lv_obj_set_style_border_color(btn, red_color, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_align(label, LV_ALIGN_CENTER, 20, 20);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(label, red_color, 0);
            lv_obj_add_event_cb(btn, slot_click_event, LV_EVENT_CLICKED, (void*)i);
        }

        lv_obj_center(label);
    }

    build_home_button(edit_scrn);
}


void remove_shadow(lv_obj_t *obj) {
    // Ensure all shadow properties are disabled to remove any visible shadow
    lv_obj_set_style_shadow_spread(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
}


//home button for all screens
void build_home_button(lv_obj_t *screen){
    //Back
    lv_obj_t *back = lv_btn_create(screen);
    lv_obj_align(back, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_size(back, 180, 40);
    lv_obj_add_style(back, &def_button_style, 0);


    lv_obj_t *lbl = lv_label_create(back);
    lv_label_set_text(lbl, LV_SYMBOL_HOME);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(back, [](lv_event_t * e){
        build_main_screen();
        lv_scr_load_anim(main_scrn, LV_SCR_LOAD_ANIM_NONE, 300, 0, false);
    }, LV_EVENT_CLICKED, NULL);
}


//--------------------------- Title Builder ---------------------------
//screen title builder for all screens
void build_scrn_title(lv_obj_t *screen, const char *title_text){
    lv_obj_t *title_rect = lv_obj_create(screen);
    lv_obj_set_size(title_rect, SCREEN_WIDTH, 25); // was -60 width
    lv_obj_align(title_rect, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(title_rect, &def_button_style, 0);
    
    lv_obj_clear_flag(title_rect, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *scrn_title = lv_label_create(screen);
    lv_label_set_text(scrn_title, title_text);
    lv_obj_set_style_text_color(scrn_title, font_color, 0);
    lv_obj_set_style_text_font(scrn_title, &lv_font_montserrat_14, 0);
    lv_obj_align(scrn_title, LV_ALIGN_TOP_MID, 0, 5);

}

//--------------------------- SETUP ---------------------------
void setup(){

    //setting up debug terminal LCD ports
    Wire.begin(21,22); //SDA,SCL

    lcd.init();
    lcd.backlight();
    lcd.print("hello");

    Serial.println(esp_reset_reason());

    Serial.begin(115200);

    dht.begin();

    pinMode(wifiRedLed, OUTPUT);
    pinMode(wifiGrnLed, OUTPUT);

    pinMode(redLed, OUTPUT);
    pinMode(blueLed, OUTPUT);

    pinMode(pumpPin, OUTPUT);

    digitalWrite(pumpPin, LOW);

    load_wifi_credentials();

    if(wifi_ssid.length() > 0 && wifi_password.length() > 0){
        WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
    } else {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }

    ledcSetup(tftBackLightBrightness, 5000, 8); //CHANGE: hard code value later channel 0, 5kHz, 8-bit resolution
    ledcAttachPin(tftBackLight, tftBackLightBrightness);

    ledcWrite(tftBackLightBrightness, 255);
    
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

    prefs.begin("calibration", false); //CHECK

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
    stat_scrn = lv_obj_create(NULL);


    lv_obj_set_style_bg_color(main_scrn, lv_color_black(), 0);
    lv_obj_set_style_bg_color(settings_scrn, lv_color_black(), 0);
    lv_obj_set_style_bg_color(stat_scrn, lv_color_black(), 0);

    prefs.begin("ui", false); //*

    for(int i = 0; i < MAX_SLOTS; i++){
        slots[i] = (WidgetType)prefs.getUInt(("slot" + String(i)).c_str(), WIDGET_NONE);
    }

    current_theme_color = lv_color_hex(prefs.getUInt("btn_color", 0x2196F3));

    useFahrenheit = prefs.getBool("useF", false);
    load_pets();

    build_stat_screen();

    Serial.println(esp_reset_reason());

    build_main_screen();
    lv_scr_load(main_scrn);

    Serial.println(esp_reset_reason());
    
    lcd.clear();

    lcd.print(WiFi.SSID());
    lcd.setCursor(0,1);
    lcd.print(WiFi.status());

    //style
    lv_style_init(&def_button_style);
    lv_style_set_bg_color(&def_button_style, btn_color_new);
    lv_style_set_border_color(&def_button_style, btn_border_color);
    lv_style_set_border_width(&def_button_style, 2);
    lv_style_set_shadow_color(&def_button_style, btn_border_color);
    lv_style_set_shadow_width(&def_button_style, 15); //TEST: was 25 lowered shadow

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

        mqttClient.setServer("192.168.0.135", 1883);

        mqttClient.connect("ESP32");


        digitalWrite(wifiGrnLed, HIGH);
        digitalWrite(wifiRedLed, LOW);

        //need to find a way for it to be cleared only when value changing 
        if(oldWiFiName != (WiFi.SSID())){
            lcd.clear();
            lcd.print(WiFi.SSID());
            lcd.setCursor(0,1);
            lcd.print(WiFi.status());
        }
        lv_img_set_src(wifi_img, &wifiON);
    }
}

void connect_mqtt(){
    if(WiFi.status() == WL_CONNECTED){
        mqttClient.setServer("192.168.0.135", 1883);
        mqttClient.connect("ESP32");
    }
}




void handle__hum_mqtt(int h){

    if(!mqttClient.connected()){
        connect_mqtt();
    }
    mqttClient.publish("enclosure/humidity", String(h).c_str());
}


void handle_temp_mqtt(float t){

    if(!mqttClient.connected()){
        connect_mqtt();
    }
    mqttClient.publish("enclosure/temperature", String(t).c_str());
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
    sprintf(buf, "%.1f%%", h);
    handle__hum_mqtt((int)(h + 0.5)); // rounded

    for(int i = 0; i < MAX_SLOTS; i++){
        
        //update arc widgets
        if(slots[i] == WIDGET_HUMIDITY_ARC && slot_obj[i]){
            lv_arc_set_value(slot_obj[i], (int)(h + 0.5)); // rounded
            lv_label_set_text(slot_label[i], buf);
        }

        //update text widgets
        if(slots[i] == WIDGET_HUMIDITY_TEXT && slot_label[i]){
            lv_label_set_text(slot_label[i], buf);
        }
    }
}


float CheckTemp(){

    static uint32_t lastUpdate = 0;

    //update every 4 seconds
    if(millis() - lastUpdate < 3000) return NAN;
    lastUpdate = millis();

    float h = dht.readHumidity();
    if(isnan(h)) return NAN;

    float t = dht.readTemperature();

    return t;
}

//--------------------------- Temp Update ---------------------------
void updateTemperature(float t){
    static uint32_t lastUpdate = 0;

    if(millis() - lastUpdate < 4000) return;
    lastUpdate = millis();

    //float t = dht.readTemperature();
    if(isnan(t)) return;

    // convert if needed
    if(useFahrenheit){
        t = t * 9.0 / 5.0 + 32.0;
    }



    char buf[32];
    sprintf(buf, "Temp: %.1f%s", t, useFahrenheit ? "F" : "C");
    handle_temp_mqtt(t);

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
            lv_obj_set_style_text_color(slot_label[i], font_color, 0);
        }
    }
}

//so we dont have to calibrate every time, we save the calibration data in preferences
void save_calibration_data(uint16_t *calData){
    prefs.putBytes("calData", calData, sizeof(uint16_t) * 5);
    prefs.putBool("calibrated", true);
}


//maybe just for temp maybe for more
void chart_handler(float t){
    static uint32_t lastUpdate = 0;

    //if(!stat_scrn || lv_scr_act() != stat_scrn) return;

    if(millis() - lastUpdate < 4000) return;
    lastUpdate = millis();

    if(isnan(t) || !temp_graph || !temp_series) return;

    //lcd.clear();
    //lcd.setCursor(1, 0);
    //lcd.print(t);

    lv_chart_set_next_value(temp_graph, temp_series, (lv_coord_t)t);
    lv_chart_refresh(temp_graph);

}


//--------------------------- MY TESTS ---------------------------

//void handle_test_int(){
//    static uint32_t lastUpdate = 0;
//
//    if(millis() - lastUpdate < 300000) return;
//    lastUpdate = millis();
//
//    mynum --;
//    
//}

void log_int(){
    static uint32_t lastUpdate = 0;

    if(millis() - lastUpdate < 60000) return;
    lastUpdate = millis();

    if(num_log.data == 0){
        num_log.data = mynum;
    }else{
        int size = 1;

        Node* q = &num_log;
        while(q->next != nullptr){
            q = q->next;
            size++;
        }
        if(size < 60){
            q->next = new Node(mynum, nullptr);
        }else{
            Node* oldHead = num_log.next;
            num_log.data = oldHead->data;
            num_log.next = oldHead->next;
            delete oldHead;

            q = &num_log;
            while(q->next != nullptr){
                q = q->next;
            }
            q->next = new Node(mynum, nullptr);
        }
    }
}


void put_num_in_graph(){
    static uint32_t lastUpdate = 0;
    static bool seeded = false;

    if(!test_graph || !test_series) return;

    if(!seeded){
        int graph_value = constrain(mynum, 0, 100);
        for(int i = 0; i < 6; i++){
            lv_chart_set_next_value(test_graph, test_series, (lv_coord_t)graph_value);
        }
        lv_chart_refresh(test_graph);
        seeded = true;
        lastUpdate = millis();
        return;
    }

    if(millis() - lastUpdate < 60000) return;
    lastUpdate = millis();

    int graph_value = constrain(mynum, 0, 100);
    lv_chart_set_next_value(test_graph, test_series, (lv_coord_t)graph_value);
    lv_chart_refresh(test_graph);
}


bool turn_on(){
    //static uint32_t lastUpdate = 0;
    //if(millis() - lastUpdate < 60000) return;
    //lastUpdate = millis();
    counter_off++;
    if(counter_off > counter_off_limit){
        //turned_on = true;
        counter_on++;
        
        if(counter_on > 2){
            counter_on = 0;
            counter_off = 0;
            is_on = false;
            return false;
        }
        is_on = true;
        return true;
    }else{
        is_on = false;
        return false;
    }
    
    //if(turned_on){
    //    mynum ++;
    //    counter_on++;
    //    if(counter_on > 2){
    //        turned_on = false;
    //        counter_off = 0;
    //        counter_on = 0;
    //    }
    //}

}


void evalute_int(){
    static uint32_t lastUpdate = 0;

    if(millis() - lastUpdate < 300000) return;
    lastUpdate = millis();

    int counter = 0;

    Node *q = &num_log;
    while(q->next != nullptr){
        q = q->next;
        if(counter == 5){
            break;
        }
        counter++;
    }

    if(q == nullptr) return;

    int prev_value = q->data;



    if(mynum > 60){//too high
        if(mynum > q->data){
            counter_off_limit++;
        }
    } else if(mynum < 60){ //is too low
        if(mynum < q->data){
            counter_off_limit--; //go up faster
        }
    }

    if(counter_off_limit < 1){
        counter_off_limit = 1;
    }
}




void num_handler(){

    static uint32_t lastUpdate = 0;
    if(millis() - lastUpdate < 60000) return;
    lastUpdate = millis();

    if(turn_on()){
        mynum = mynum + 4;
        digitalWrite(pumpPin, HIGH);
    }else{
        mynum--;
        digitalWrite(pumpPin, LOW);
    }
}

//--------------------------- LOOP ---------------------------
void loop(){

    static bool isDimmed = false;
    float currentTemp = CheckTemp();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(mynum);
    lcd.setCursor(0,1);
    lcd.print(counter_off_limit);
    //lcd.setCursor(0,2);
    //lcd.print(counter_off);
    //lcd.autoscroll();


    lv_timer_handler();

    oldWiFiName = WiFi.SSID();

    handleWiFi();
    updateHumidity();

    //handle_test_int();
    
    num_handler();
    evalute_int();
    put_num_in_graph();
    log_int();

    if(is_on){
        digitalWrite(blueLed, HIGH);
        digitalWrite(redLed, LOW);
    }else{
        digitalWrite(blueLed, LOW);
        digitalWrite(redLed, HIGH);
    }

    if(!isnan(currentTemp)){
        updateTemperature(currentTemp);
        chart_handler(currentTemp);
    }
    
    if(lv_disp_get_inactive_time(NULL) < 100000){

        if(isDimmed){
            ledcWrite(tftBackLightBrightness, 255);
            isDimmed = false;
        }
    }
    else{
        if(!isDimmed){
            ledcWrite(tftBackLightBrightness, 20); //dims the backlight after certain time of inactivity
            isDimmed = true;
        }
    }
    delay(5);

}