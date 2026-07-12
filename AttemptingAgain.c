#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <windows.h>
#include "cJSON-1.7.19/cJSON.h"
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <errno.h>
#include "miniaudio-0.11.25/miniaudio.h"
#include <wincodec.h> //Windows Imaging Component API
// #include <Sound_Player.h>
#define MAXSIZE 260
#define INITIAL_CAPACITY 10
//Supported windows background image support
const char AllowableExtensions[8][5] = {"avif", "jpg", "jpeg", "png", "webp", "bmp", "tiff", "tif"};
const char Themes[6][9] = {"Center", "Stretch", "Fit", "Fill", "Title", "Span"};
//Styles (most) that windows allows
typedef enum {
    CENTER,
    STRETCH,
    FIT,
    FILL,
    TILE,
    SPAN
}Style;
//Stores each images
struct ImageHolder{
    //Parent folder and specific image name (combination of file_type && path)
    char* filePaths;
    //Last character of the folder
    int Top_filepath;
    //Image name within the folder
    char* Specific_Image;
    //Style
    Style current_Style;
    //Color
    COLORREF color;
};
//For quick access for the styles and tile
typedef struct {
    const char* styleStr;
    const DWORD styleSize;
    const char* tileStr;
}styleStr_and_tileStr;
//A list to easily get the registry modes
static const styleStr_and_tileStr REG_MODES[] ={
    { "0",  2, "0" }, // 0: CENTER
    { "2",  2, "0" }, // 1: STRETCH
    { "6",  2, "0" }, // 2: FIT
    { "10", 3, "0" }, // 3: FILL
    { "0",  2, "1" }, // 4: TILE
    { "22", 3, "0" }  // 5: SPAN
};
//Customizations/initializations
bool PLAY_SOUND = false;
bool CHANGE_COLOR = false;
char* FOLDER_PATH = NULL;
struct ImageHolder* listOfImages = NULL;
size_t image_Count = 0;
size_t image_Capacity = 0;
//Checks if the extension of the image is valid
bool is_image(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return false;
    const char *ext = dot + 1;
    for (int i = 0; i < sizeof(AllowableExtensions) / sizeof(AllowableExtensions[0]); i++)
        if (strcasecmp(ext, AllowableExtensions[i]) == 0)
            return true;
    return false;
}
//Checks what to set the style to
void check_Style(const char second_last) {
    //Using the last letters of each name was that was easies to using mass if statements
    switch (second_last) {
        case 't':
            listOfImages[image_Count].current_Style = FIT;
            break;
        case 'l':
            listOfImages[image_Count].current_Style = FILL;
            break;
        case 'e':
            listOfImages[image_Count].current_Style = TILE;
            break;
        case 'n':
            listOfImages[image_Count].current_Style = SPAN;
            break;
        case 'h':
            listOfImages[image_Count].current_Style = STRETCH;
            break;
        case 'r':
            listOfImages[image_Count].current_Style = CENTER;
            break;
        default:
            printf("There was an error in finding what this would correspond to the enum: %c", second_last);
    }
}
//Algorithm that divides up the image to find the average color of the image done on startup or if invalid color
COLORREF GetAverageImageColor(const WCHAR* imagePath) {
    IWICImagingFactory *pFactory = NULL;
    IWICBitmapDecoder *pDecoder = NULL;
    IWICBitmapFrameDecode *pFrame = NULL;
    IWICFormatConverter *pConverter = NULL;
    COLORREF dominantColor = RGB(50, 50, 50);
    CoInitialize(NULL);
    HRESULT hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (LPVOID *)&pFactory);
    if (SUCCEEDED(hr)) hr = pFactory->lpVtbl->CreateDecoderFromFilename(pFactory, imagePath, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pDecoder);
    if (SUCCEEDED(hr)) hr = pDecoder->lpVtbl->GetFrame(pDecoder, 0, &pFrame);
    if (SUCCEEDED(hr)) hr = pFactory->lpVtbl->CreateFormatConverter(pFactory, &pConverter);
    if (SUCCEEDED(hr)) hr = pConverter->lpVtbl->Initialize(pConverter, (IWICBitmapSource*)pFrame, &GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
    UINT width = 0, height = 0;
    if (SUCCEEDED(hr)) hr = pConverter->lpVtbl->GetSize(pConverter, &width, &height);
    if (SUCCEEDED(hr) && width > 0 && height > 0) {
        BYTE *pixels = malloc(width * 4 * height);
        if (pixels && SUCCEEDED(pConverter->lpVtbl->CopyPixels(pConverter, NULL, width * 4, width * 4 * height, pixels))) {
            #define BUCKETS 8
            #define CHUNK 32
            int counts[BUCKETS][BUCKETS][BUCKETS] = {0};
            unsigned long long sumR[BUCKETS][BUCKETS][BUCKETS] = {0};
            unsigned long long sumG[BUCKETS][BUCKETS][BUCKETS] = {0};
            unsigned long long sumB[BUCKETS][BUCKETS][BUCKETS] = {0};
            for (int i = 0; i < (int) width * (int) height * 4; i += 4) {
                if (pixels[i + 2] > 230 && pixels[i + 1] > 230 && pixels[i] > 230) continue;
                if (pixels[i + 2] < 25 && pixels[i + 1] < 25 && pixels[i] < 25) continue;
                counts[pixels[i + 2] / CHUNK][pixels[i + 1] / CHUNK][pixels[i] / CHUNK]++;
                sumR[pixels[i + 2] / CHUNK][pixels[i + 1] / CHUNK][pixels[i] / CHUNK] += pixels[i + 2];
                sumG[pixels[i + 2] / CHUNK][pixels[i + 1] / CHUNK][pixels[i] / CHUNK] += pixels[i + 1];
                sumB[pixels[i + 2] / CHUNK][pixels[i + 1] / CHUNK][pixels[i] / CHUNK] += pixels[i];
            }
            int maxCount = 0;
            for (int rIdx = 0; rIdx < BUCKETS; rIdx++)
                for (int gIdx = 0; gIdx < BUCKETS; gIdx++)
                    for (int bIdx = 0; bIdx < BUCKETS; bIdx++)
                        if (counts[rIdx][gIdx][bIdx] > maxCount) {
                            maxCount = counts[rIdx][gIdx][bIdx];
                            // 0.75f is the darkening factor
                            dominantColor = RGB((BYTE)(sumR[rIdx][gIdx][bIdx] / maxCount) * 0.75f,
                                                (BYTE)(sumG[rIdx][gIdx][bIdx] / maxCount) * 0.75f,
                                                (BYTE)(sumB[rIdx][gIdx][bIdx] / maxCount) * 0.75f);
                        }
            free(pixels);
        }
    }
    if (pConverter) pConverter->lpVtbl->Release(pConverter);
    if (pFrame) pFrame->lpVtbl->Release(pFrame);
    if (pDecoder) pDecoder->lpVtbl->Release(pDecoder);
    if (pFactory) pFactory->lpVtbl->Release(pFactory);
    CoUninitialize();
    return dominantColor;
}
//WCHAR needed for average image color
WCHAR* create_L_string(const char* to_convert) {
    if (FOLDER_PATH == NULL) {
        fprintf(stderr, "Error: FOLDER_PATH is NULL. Did the JSON configuration fail to load?\n");
        return NULL;
    }
    if (to_convert == NULL) {
        fprintf(stderr, "Error: to_convert string is NULL.\n");
        return NULL;
    }
    char* combinedAnsiStr = malloc(strlen(FOLDER_PATH) + strlen(to_convert) + 2);
    if (combinedAnsiStr == NULL) return NULL;
    strcat(strcat(strcpy(combinedAnsiStr, FOLDER_PATH), "\\"), to_convert);
    WCHAR* widePath = NULL;
    const int requiredSize = MultiByteToWideChar(CP_ACP, 0, combinedAnsiStr, -1, NULL, 0);
    if (requiredSize > 0) {
        widePath = malloc(requiredSize * sizeof(WCHAR));
        if (widePath != NULL)
            MultiByteToWideChar(CP_ACP, 0, combinedAnsiStr, -1, widePath, requiredSize);
    }
    free(combinedAnsiStr);
    return widePath;
}
//Adding the contents to the list struct
void add_to_list(const char* path, const int file_type, const int type, const int color) {
    if (image_Count >= image_Capacity) {
        image_Capacity = image_Capacity == 0 ? INITIAL_CAPACITY : image_Capacity + 3;
        struct ImageHolder* newList = realloc(listOfImages, image_Capacity * sizeof(struct ImageHolder));
        if (!newList) {
            fprintf(stderr, "Memory allocation failed!\n");
            exit(1);
        }
        listOfImages = newList;
    }
    if (type < 0)
        check_Style((char) (file_type + 'a'));
    else
        listOfImages[image_Count].current_Style = (Style) type;
    char* temp = malloc(strlen(Themes[listOfImages[image_Count].current_Style]) + strlen(path) + 2); // "\\" counts as 1 character
    if (!temp) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strcat(strcat(strcpy(temp, Themes[listOfImages[image_Count].current_Style]), "\\"), path);
    //Initialization
    listOfImages[image_Count].filePaths = temp;
    listOfImages[image_Count].Top_filepath = file_type;
    listOfImages[image_Count].Specific_Image = strdup(path);
    //Checks if certain values need fixing
    if (color < 0) {
        WCHAR* myWideStr = create_L_string(listOfImages[image_Count].filePaths);
        if (myWideStr != NULL) {
            listOfImages[image_Count].color = GetAverageImageColor(myWideStr);
            free(myWideStr);
        }
        else
            perror("ERROR");
    }
    else
        listOfImages[image_Count].color = (COLORREF) color;
    image_Count++;
}
//Dumping the remining data back into the json
void set_Up_Json(const char* folder_path, const int position) {
    FILE* openJson = fopen("Current_New_Caching.json", "w");
    if (openJson == NULL) {
        perror("There was an error in creating/opening the json");
        return;
    }
    //Set up the root file
    cJSON* json_Obj_root = cJSON_CreateObject();
    cJSON_AddStringToObject(json_Obj_root, "Folder_Path", folder_path);
    cJSON_AddBoolToObject(json_Obj_root, "PLAY_SOUND", PLAY_SOUND);
    cJSON_AddBoolToObject(json_Obj_root, "CHANGE_COLOR", CHANGE_COLOR);
    //Create a json array
    cJSON* json_Array = cJSON_CreateArray();
    //Parsing each filename in the struct into a json array
    // printf("%d", position);
    for (int i = position + 1; i < image_Count; i++) {
        cJSON *temp_Json_Storage = cJSON_CreateObject();
        cJSON_AddNumberToObject(temp_Json_Storage, "Folder", listOfImages[i].Top_filepath);
        cJSON_AddStringToObject(temp_Json_Storage, "Image", listOfImages[i].Specific_Image);
        cJSON_AddNumberToObject(temp_Json_Storage, "Enum", listOfImages[i].current_Style);
        cJSON_AddNumberToObject(temp_Json_Storage, "ARGB", listOfImages[i].color);
        cJSON_AddItemToArray(json_Array, temp_Json_Storage);
    }
    cJSON_AddItemToObject(json_Obj_root, "Arrays_Of_Images", json_Array);
    char *json_string = cJSON_Print(json_Obj_root);
    if (json_string) {
        fputs(json_string, openJson);
        free(json_string);
    }
    fclose(openJson);
    cJSON_Delete(json_Obj_root);
}
//Getting each individual file in each subfolder
void process_files_in_specific_folder(const char* path, const char* file_type) {
    DIR *dir = opendir(path);
    struct dirent *ent;
    if (!dir) return;
    while ((ent = readdir(dir)) != NULL) {
        //Skips non file types
        if (ent->d_name[0] == '.') continue;
        if (is_image(ent->d_name))
            add_to_list(ent->d_name, file_type[strlen(file_type) - 1] - 'a', -1, -1);
    }
    closedir(dir);
}
//Searches the parent folder for valid subfolders to then find their contents
void search_File(const char* base_path) {
    struct dirent *ent;
    struct stat st;
    DIR *dir = opendir(base_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
        //Temp string to check for valid folder
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, ent->d_name);
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode) && (strcmp(ent->d_name, "Center") == 0 ||
                strcmp(ent->d_name, "Tile") == 0 ||
                strcmp(ent->d_name, "Stretch") == 0 ||
                strcmp(ent->d_name, "Fit") == 0 ||
                strcmp(ent->d_name, "Fill") == 0 ||
                strcmp(ent->d_name, "Span") == 0))
                    process_files_in_specific_folder(full_path, ent->d_name);
        else
            search_File(full_path);
    }
    closedir(dir);
}
void randomizer() {
    srand(time(NULL));
    for (int i = (int) image_Count - 1; i > 0; i--) {
        const int position = rand() % (i + 1);
        char* temp = listOfImages[i].filePaths;
        listOfImages[i].filePaths = listOfImages[position].filePaths;
        listOfImages[position].filePaths = temp;
        const int tempInt = listOfImages[i].Top_filepath;
        listOfImages[i].Top_filepath = listOfImages[position].Top_filepath;
        listOfImages[position].Top_filepath = tempInt;
        char* temp3 = listOfImages[i].Specific_Image;
        listOfImages[i].Specific_Image = listOfImages[position].Specific_Image;
        listOfImages[position].Specific_Image = temp3;
        const Style tempStyle = listOfImages[i].current_Style;
        listOfImages[i].current_Style = listOfImages[position].current_Style;
        listOfImages[position].current_Style = tempStyle;
        const COLORREF tempColor = listOfImages[i].color;
        listOfImages[i].color = listOfImages[position].color;
        listOfImages[position].color = tempColor;
    }
}
//Cleaning everything
void free_ImageHolder() {
    for (size_t i = 0; i < image_Count; i++) {
        free(listOfImages[i].filePaths);
        free(listOfImages[i].Specific_Image);
    }
    free(listOfImages);
    listOfImages = NULL;
    image_Count = 0;
    image_Capacity = 0;
}
//Retrieving all the information from the JSON file
void get_From_Json(const cJSON *root) {
    const cJSON *images_array = cJSON_GetObjectItemCaseSensitive(root, "Arrays_Of_Images");
    if (!cJSON_IsArray(images_array)) return;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, images_array) {
        const cJSON *Parent_folder_node = cJSON_GetObjectItemCaseSensitive(item, "Folder");
        const cJSON *path_node = cJSON_GetObjectItemCaseSensitive(item, "Image");
        const cJSON *enum_node = cJSON_GetObjectItemCaseSensitive(item, "Enum");
        const cJSON *ARGB_node = cJSON_GetObjectItemCaseSensitive(item, "ARGB");
        if (cJSON_IsString(path_node) && cJSON_IsNumber(Parent_folder_node) && cJSON_IsNumber(enum_node) && cJSON_IsNumber(ARGB_node))
            add_to_list(path_node->valuestring, Parent_folder_node->valueint, enum_node->valueint, ARGB_node->valueint);
    }
}
//Temp string that is the absolute path of an image used in first_file_to_exist
char* create_Temp_String(const char* path, const int i) {
    char* temp = malloc(strlen(path) + strlen(listOfImages[i].filePaths) + 2);
    if (!temp) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strcat(strcat(strcpy(temp, path), "\\"), listOfImages[i].filePaths);
    // printf("%s\n", temp);
    return temp;
}
//Finding the most recent valid file from the list that currently exist in case of invalid name or not existing files
const char* first_file_to_exist(const char* path, int* pos) {
    const char* temp = create_Temp_String(path, *pos);
    while (*pos < image_Count) {
        // printf("%s, %zu, %d\n", listOfImages[i].filePaths, image_Count, i);
        temp = create_Temp_String(path, *pos);
        if (access(temp, F_OK) == 0)
            break;
        free((void*) temp);
        temp = NULL;
        if (errno == ENOENT)
            (*pos)++;
        else {
            ShowWindow(GetConsoleWindow(), SW_SHOW);
            printf("%s", listOfImages[*pos].filePaths);
            perror("File access failed");
            getchar();
            break;
        }
    }
    return temp;
}
//Sets up the config for the regKeys that gets sets in the "finish" function below
void Change_Reg_hKey(const DWORD color, const Style type) {
    //Sets the regKey if the user wants to change the color
    if (CHANGE_COLOR) {
        HKEY hKey_DWM = NULL, hKey_Accent = NULL;
        do {
            if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\DWM", 0, KEY_SET_VALUE, &hKey_DWM) != ERROR_SUCCESS ||
                RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent", 0, KEY_SET_VALUE, &hKey_Accent) != ERROR_SUCCESS) {
                if (hKey_DWM) RegCloseKey(hKey_DWM);
                if (hKey_Accent) RegCloseKey(hKey_Accent);
                break;
            }
            const DWORD abgrColor = color | 0xFF000000;
            const DWORD r = color & 0x000000FF;
            const DWORD g_shifted_to_start = (color & 0x0000FF00) >> 8;
            const DWORD b = (color & 0x00FF0000) >> 16;
            const DWORD argbColor = 0xFF000000 | r << 16 | g_shifted_to_start << 8 | b;
            RegSetValueExA(hKey_DWM, "ColorizationColor", 0, REG_DWORD, (const BYTE*)&argbColor, 4);
            RegSetValueExA(hKey_DWM, "ColorizationAfterglow", 0, REG_DWORD, (const BYTE*)&argbColor, 4);
            RegSetValueExA(hKey_DWM, "AccentColor", 0, REG_DWORD, (const BYTE*)&abgrColor, 4);
            RegCloseKey(hKey_DWM);
            DWORD palette[8];
            static const int scales[8] = { 358, 307, 281, 256, 230, 204, 153, 102 };
            for (int i = 0; i < 8; ++i) {
                const int sr = ((int) r * scales[i]) >> 8;
                const int sg = ((int) g_shifted_to_start * scales[i]) >> 8;
                const int sb = ((int) b * scales[i]) >> 8;
                const BYTE final_r = sr > 255 ? 255 : (BYTE)sr;
                const BYTE final_g = sg > 255 ? 255 : (BYTE)sg;
                const BYTE final_b = sb > 255 ? 255 : (BYTE)sb;
                palette[i] = 0xFF000000 | final_b << 16 | final_g << 8 | final_r;
            }
            RegSetValueExA(hKey_Accent, "AccentPalette", 0, REG_BINARY, (const BYTE*)palette, 32);
            RegSetValueExA(hKey_Accent, "AccentColorMenu", 0, REG_DWORD, (const BYTE*)&abgrColor, 4);
            RegSetValueExA(hKey_Accent, "StartColorMenu", 0, REG_DWORD, (const BYTE*)&abgrColor, 4);
            RegCloseKey(hKey_Accent);
        } while (0);
    }
    //Sets the regKey for the wallpaper
    HKEY hKey_Desktop = NULL;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_SET_VALUE, &hKey_Desktop) == ERROR_SUCCESS) {
        RegSetValueExA(hKey_Desktop, "WallpaperStyle", 0, REG_SZ, (const BYTE*)REG_MODES[type].styleStr, REG_MODES[type].styleSize);
        RegSetValueExA(hKey_Desktop, "TileWallpaper", 0, REG_SZ, (const BYTE*)REG_MODES[type].tileStr, 2);
        RegCloseKey(hKey_Desktop);
    }
}
//Finds the position of the first valid image in the list and executes setting up the regKeys
int finish(const char* path) {
    //Get the filename to change the background and then the position to pass to the json
    int pos = 0;
    const char* temp = first_file_to_exist(path, &pos);
    if (pos == image_Count)
        search_File(path);
    Change_Reg_hKey(listOfImages[pos].color, listOfImages[pos].current_Style);
    //Statement that sets the regKeys
    if (SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID) temp, SPIF_UPDATEINIFILE)) {
        PostMessageA(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETDESKWALLPAPER, 0);
        if (CHANGE_COLOR)
            PostMessageA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"ImmersiveColorSet");
        free((void*) temp);
        return pos;
    }
    ShowWindow(GetConsoleWindow(), SW_SHOW);
    printf("Failed to set wallpaper\n");
    free((void*) temp);
    return pos;
}
//Lazy and did not want to repeat the same thing 4 times
char prompt_Question(const char* special_prompt) {
    printf("Do you want %s? (y/n): ", special_prompt);
    return (char) getchar();
}
//Same with this function
void call_switch(const char ans, const char type) {
    switch (ans) {
        case 'y':
            printf("Setting to true!\n");
            if (type == 's')
                PLAY_SOUND = true;
            else
                CHANGE_COLOR = true;
            break;
        case 'n':
            printf("Setting to false!\n");
            if (type == 's')
                PLAY_SOUND = false;
            else
                CHANGE_COLOR = false;
            break;
        default:
            printf("Setting to a default of false!\n");
            PLAY_SOUND = false;
    }
}
//Checks if the current "Folder_Path" in the JSON exists, if not calls the user to set the path
bool check_if_path_exists(const char* path) {
    DIR* dir = opendir(path);
    if (dir) {
        closedir(dir);
        return true;
    }
    if (ENOENT == errno || ENOTDIR == errno)
        printf("Directory does not exist.\n"
               "The path that was there before does not exist no more :((!\n"
               "Set the path again!\n");
    else
        perror("opendir failed either:\nPermission Denied\n"
               "Too Many Open Files\nOut of Memory\nNot a valid directory 1!\n");
    return false;
}
//If there is no path in the JSON or the current path is invalid
void set_Path(const char* text) {
    char base_path[MAXSIZE] = "";
    printf("%s\n", text);
    fgets(base_path, MAXSIZE, stdin);
    base_path[strcspn(base_path, "\n")] = 0;
    while (strcmp(base_path, ".") == 0 || strcmp(base_path, "/") == 0 || strcmp(base_path, "\\") == 0 || strcmp(base_path, ",") == 0 || !check_if_path_exists(base_path)) {
        printf("A \"%s\" will not suffice\nEnter the path for the folders: ", base_path);
        fgets(base_path, MAXSIZE, stdin);
        base_path[strcspn(base_path, "\n")] = 0;
    }
    free(FOLDER_PATH);
    FOLDER_PATH = strdup(base_path);
    if (FOLDER_PATH == NULL) {
        perror("Unable to allocate memory for folder path");
        return;
    }
    search_File(base_path);
    if (image_Count > 0) {
        const char play = prompt_Question("sound to play");
        call_switch(play, 's');
        getchar();
        const char change = prompt_Question("change color");
        call_switch(change, 'c');
        getchar();
        randomizer();
        set_Up_Json(base_path, 0);
        finish(base_path);
    }
    else
        set_Path("No images found in the specified directory try a different directory1!");
    free_ImageHolder();
}
//Sound effect if wanted must be in .wav format
void on_sound_end(void* pUserData, ma_sound* pSound) {
    ma_engine* engine = pUserData;
    ma_sound_uninit(pSound);
    ma_engine_uninit(engine);
    free(pSound);
    free(engine);
}
//Sets up playing the sound
void playsound(const char* path_to_audio, const float volume) {
    ma_engine* engine = malloc(sizeof(ma_engine));
    ma_sound* sound = malloc(sizeof(ma_sound));
    if (engine == NULL || sound == NULL) {
        free(engine);
        free(sound);
        return;
    }
    if (ma_engine_init(NULL, engine) != MA_SUCCESS) {
        free(engine);
        free(sound);
        return;
    }
    if (ma_sound_init_from_file(engine, path_to_audio, 0, NULL, NULL, sound) != MA_SUCCESS) {
        ma_engine_uninit(engine);
        free(engine);
        free(sound);
        return;
    }
    ma_sound_set_volume(sound, volume);
    ma_sound_set_end_callback(sound, on_sound_end, engine); //The function above
    ma_sound_start(sound);
}
//Used to just get the user preferences quickly at the beggining
bool scan_BOOL_sound_at_boot(const char *filename, const int offset) {
    FILE *file = fopen(filename, "r");
    if (!file) return false;
    fseek(file, offset, SEEK_SET);
    const char c = (char) fgetc(file);
    fclose(file);
    // printf("%d -> %c\n", c, c);
    return c == 't';
}
int main() {
    //To close terminal as it is not needed, and will be oppened when needed
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    /*//Testing the speed of the program
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    // 1. Initialize the high-resolution hardware clock frequency
    if (!QueryPerformanceFrequency(&frequency)) {
        printf("Error: High-resolution timer not supported by CPU.\n");
        return -1;
    }
    // 2. Take the start snapshot
    QueryPerformanceCounter(&start);*/
    PLAY_SOUND = scan_BOOL_sound_at_boot("Current_New_Caching.json", 87);
    CHANGE_COLOR = scan_BOOL_sound_at_boot("Current_New_Caching.json", 112);
    //Up here as the sond used is quite lengthy at 1-2 seconds and the actual code runs faster so seemed like the best option
    if (PLAY_SOUND)
        playsound("../Sounds/Something-Finished-Fast.wav", 0.4f);
    //Setting up the program and getting the path
    FILE* openJson = fopen("Current_New_Caching.json", "r");
    if (openJson == NULL) {
        openJson = fopen("Current_New_Caching.json", "w");
        if (openJson == NULL) {
            ShowWindow(GetConsoleWindow(), SW_SHOW);
            printf("Problem with file opening :))\n");
            return -2;
        }
    }
    //Finds the size of the json file to allocate the amount of characters into a buffer
    fseek(openJson, 0, SEEK_END);
    const long fileSize = ftell(openJson);
    rewind(openJson);
    char* string_Read_From_Json = malloc(fileSize + 1);
    if (string_Read_From_Json == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(openJson);
        return -3;
    }
    const size_t bytesRead = fread(string_Read_From_Json, 1, fileSize, openJson);
    string_Read_From_Json[bytesRead] = '\0';
    fclose(openJson);
    //JSON parsing and start of the main program
    const cJSON* root = cJSON_Parse(string_Read_From_Json);
    if (root != NULL && check_if_path_exists(cJSON_GetObjectItemCaseSensitive(root, "Folder_Path")->valuestring)) {
        //Needed for creation of the L string
        FOLDER_PATH = strdup(cJSON_GetObjectItemCaseSensitive(root, "Folder_Path")->valuestring);
        get_From_Json(root);
        const cJSON *base_path = cJSON_GetObjectItemCaseSensitive(root, "Folder_Path");
        // printf("%s\n", FOLDER_PATH);
        //Checks for if there is no images left in the JSON array
        if (image_Count == 0) {
            // printf("No images are left to cycle through! :(\nFinding new images to find.\n");
            //Checks the path for images
            search_File(base_path->valuestring);
            //If there is images continues to set everything up
            if (image_Count > 0) {
                randomizer();
                set_Up_Json(base_path->valuestring, finish(base_path->valuestring));
            }
            else
                printf("No images found in the specified directory 0!\n");
            free_ImageHolder();
        }
        //If the list still has images then the program can resume
        else
            set_Up_Json(base_path->valuestring, finish(base_path->valuestring));
    }
    else {
        ShowWindow(GetConsoleWindow(), SW_SHOW);
        // printf("There is no path inputted from previous entries or the current folder does not exist\n");
        set_Path("There is no path inputted from previous entries or the current folder does not exist\nEnter the path for the folders: ");
    }
    free(string_Read_From_Json);
    free(FOLDER_PATH);
    //Since the program exits faster than the main logic this is here to let the sound fully play out
    if (PLAY_SOUND)
        Sleep(1040);
    /*//Speed testing
    QueryPerformanceCounter(&end);
    // 5. Calculate the performance metrics
    LONGLONG elapsed_ticks = end.QuadPart - start.QuadPart;
    double milliseconds = (double)(elapsed_ticks * 1000) / frequency.QuadPart;
    double microseconds = (double)(elapsed_ticks * 1000000) / frequency.QuadPart;
    // 6. Output the results
    printf("\n--- Benchmark Results ---\n");
    printf("Execution time: %.2f ms (%.0f microseconds)\n", milliseconds, microseconds);
    printf("Windows accent colors updated successfully.\n");*/
    return 0;
}