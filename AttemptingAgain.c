#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <windows.h>
#include "cJSON-1.7.19/cJSON.h"
#include <time.h>
#include <stdbool.h>
#include <sys/stat.h>
#define MAXSIZE 3024
#define INITIAL_CAPACITY 10
const char AllowableExtensions[7][5] = {"jpg", "jpeg", "png", "webp", "bmp", "tiff", "tif"};
typedef enum {
    CENTER,
    STRETCH,
    FIT,
    FILL,
    TILE,
    SPAN
}Style;
struct ImageHolder{
    char* filePaths;
    size_t position;
    Style current_Style;
};
struct ImageHolder* listOfImages = NULL;
size_t image_Count = 0;
size_t image_Capacity = 0;
bool is_image(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return false;
    const char *ext = dot + 1;
    for (int i = 0; i < sizeof(AllowableExtensions) / sizeof(AllowableExtensions[0]); i++)
        if (strcasecmp(ext, AllowableExtensions[i]) == 0) {
            return true;
        }
    return false;
}
void check_Style(const char* second_last) {
    if (strcmp(second_last, "Fit") == 0)
        listOfImages[image_Count].current_Style = FIT;
    else if (strcmp(second_last, "Tile") == 0)
        listOfImages[image_Count].current_Style = TILE;
    else if (strcmp(second_last, "Center") == 0)
        listOfImages[image_Count].current_Style = CENTER;
    else if (strcmp(second_last, "Stretch") == 0)
        listOfImages[image_Count].current_Style = STRETCH;
    else if (strcmp(second_last, "Fill") == 0)
        listOfImages[image_Count].current_Style = FILL;
    else if (strcmp(second_last, "Span") == 0)
        listOfImages[image_Count].current_Style = SPAN;
}
void add_to_list(const char* path, const char* base_path) {
    char* temp = malloc(strlen(base_path) + strlen(path) + 2); // +2 for '\\' and '\0'
    if (!temp) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strcpy(temp, base_path);
    strcat(temp, "\\");
    strcat(temp, path);
     // If we need more space
    if (image_Count >= image_Capacity) {
        image_Capacity = image_Capacity == 0 ? INITIAL_CAPACITY : image_Capacity + 3;
        struct ImageHolder* newList = realloc(listOfImages, image_Capacity * sizeof(struct ImageHolder));
        if (!newList) {
            fprintf(stderr, "Memory allocation failed!\n");
            exit(1);
        }
        listOfImages = newList;
    }
    // Add the new path
    listOfImages[image_Count].filePaths = temp;
    listOfImages[image_Count].position = image_Count;
    listOfImages[image_Count].current_Style = FIT;
    char *last = strrchr(base_path, '/');
    printf("%s", last);
    if (last == NULL) {
        last = strrchr(base_path, '\\');
        if (last == NULL) {
            printf("No slashes found.\n");
            return;
        }
    }
    // 2. Temporarily "cut" the string or search before that pointer
    // We create a search limit by looking only at the string before 'last'
     const char *second_last = last + 1;
    if (second_last != NULL) {
        printf("\nFolder name 1: %s\n", second_last);
        check_Style(second_last);
    }
    else {
        printf("Only one slash found. No parent directory segment.\n");
    }

    image_Count++;
}
void set_Up_Json(const char* folder_path) {
    FILE* openJson = fopen("Caching.json", "w");
    if (openJson == NULL) {
        perror("There was an error in creating/opening the json");
        return;
    }

    //Set up the root file
    cJSON* json_Obj_root = cJSON_CreateObject();
    cJSON_AddStringToObject(json_Obj_root, "Folder_Path", folder_path);

    //Create a json array
    cJSON* json_Array = cJSON_CreateArray();
    //Parsing each filename in the struct into a json array
    for (int i = 1; i < image_Count; i++) {
        cJSON *temp_Json_Storage = cJSON_CreateObject();
        cJSON_AddStringToObject(temp_Json_Storage, "Absolute_Path_Of_Images", listOfImages[i].filePaths);
        cJSON_AddNumberToObject(temp_Json_Storage, "Enum", listOfImages[i].current_Style);
        cJSON_AddItemToArray(json_Array, temp_Json_Storage);
    }

    cJSON_AddItemToObject(json_Obj_root, "Arrays_To_Path_Of_Images", json_Array);
    char *json_string = cJSON_Print(json_Obj_root);
    if (json_string) {
        fputs(json_string, openJson);
        free(json_string);
    }

    fclose(openJson);
    cJSON_Delete(json_Obj_root);
}
void process_files_in_target(const char* path) {
    DIR *dir = opendir(path);
    struct dirent *ent;
    if (!dir) return;
    while ((ent = readdir(dir)) != NULL) {
        // Skip hidden files and directories
        if (ent->d_name[0] == '.') continue;
        if (is_image(ent->d_name))
            add_to_list(ent->d_name, path);
    }
    closedir(dir);
}
void search_File(const char* base_path) {
    struct dirent *ent;
    struct stat st;
    DIR *dir = opendir(base_path);

    if (dir == NULL) {
        perror("opendir");
        return;
    }

    while ((ent = readdir(dir)) != NULL) {
        // 1. Skip "." and ".." to prevent infinite loops
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        // 2. Build the FULL path (stat needs the full path to work)
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, ent->d_name);

        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {

            // 4. Check if the folder name is one of your allowed names
            if (strcmp(ent->d_name, "Center") == 0 ||
                strcmp(ent->d_name, "Tile") == 0 ||
                strcmp(ent->d_name, "Stretch") == 0 ||
                strcmp(ent->d_name, "Fit") == 0 ||
                strcmp(ent->d_name, "Fill") == 0 ||
                strcmp(ent->d_name, "Span") == 0) {
                    process_files_in_target(full_path);
                }
        } else {
            search_File(full_path);
        }
    }
    closedir(dir);
}
void randomizer() {
    srand(time(NULL));
    for (int i = (int) image_Count - 1; i > 0; i--) {
        const int position = rand() % (i + 1);
        char* temp = listOfImages[i].filePaths;
        listOfImages[i].filePaths = listOfImages[position].filePaths;
        listOfImages[position].filePaths = temp;  // Simple pointer swap
        const Style tempStyle = listOfImages[i].current_Style;
        listOfImages[i].current_Style = listOfImages[position].current_Style;
        listOfImages[position].current_Style = tempStyle;
        const size_t tempPos = listOfImages[i].position;
        listOfImages[i].position = listOfImages[position].position;
        listOfImages[position].position = tempPos;
    }
}
void free_ImageHolder() {
    for (size_t i = 0; i < image_Count; i++) {
        free(listOfImages[i].filePaths); // Free each string
    }
    free(listOfImages); // Free the array itself
    listOfImages = NULL;
    image_Count = 0;
    image_Capacity = 0;
}
void add_To_List_After(char* path, Style fitting) {
    if (image_Count >= image_Capacity) {
        image_Capacity = image_Capacity == 0 ? INITIAL_CAPACITY : image_Capacity + 3;
        struct ImageHolder* newList = realloc(listOfImages, image_Capacity * sizeof(struct ImageHolder));
        if (!newList) {
            fprintf(stderr, "Memory allocation failed!\n");
            exit(1);
        }
        listOfImages = newList;
    }
    listOfImages[image_Count].filePaths = path;
    listOfImages[image_Count].position = image_Count;
    listOfImages[image_Count].current_Style = fitting; // Default style
    image_Count++;
}
void get_From_Json(const cJSON *root, const char* path) {
    const cJSON *images_array = cJSON_GetObjectItemCaseSensitive(root, "Arrays_To_Path_Of_Images");
    if (!cJSON_IsArray(images_array)) return;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, images_array) {
        const cJSON *path_node = cJSON_GetObjectItemCaseSensitive(item, "Absolute_Path_Of_Images");
        const cJSON *enum_node = cJSON_GetObjectItemCaseSensitive(item, "Enum");
        if (cJSON_IsString(path_node) && cJSON_IsNumber(enum_node))
            add_To_List_After(path_node->valuestring, (Style) enum_node->valueint);
    }
}
void finish() {
    HKEY hKey;
    const char* styleStr = "0";
    const char* tileStr = "0";

    // Assuming your struct has a member called 'style' (e.g., listOfImages[0].style)
    switch (listOfImages[0].current_Style) {
        case CENTER:  styleStr = "0";  tileStr = "0"; break;
        case TILE:    styleStr = "0";  tileStr = "1"; break;
        case STRETCH: styleStr = "2";  tileStr = "0"; break;
        case FIT:     styleStr = "6";  tileStr = "0"; break;
        case FILL:    styleStr = "10"; tileStr = "0"; break;
        case SPAN:    styleStr = "22"; tileStr = "0"; break;
    }
    for (int i = 0; i < image_Count; i++)
        printf("This image is here %d: %s\n", i, listOfImages[i].filePaths);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Control Panel\\Desktop", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, "WallpaperStyle", 0, REG_SZ, (const BYTE*)styleStr, lstrlenA(styleStr) + 1);
        RegSetValueExA(hKey, "TileWallpaper", 0, REG_SZ, (const BYTE*)tileStr, lstrlenA(tileStr) + 1);
        RegCloseKey(hKey);
    }
    if (SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, listOfImages[0].filePaths, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE))
        printf("Wallpaper set successfully to: %s\n", listOfImages[0].filePaths);
    else
        printf("Failed to set wallpaper\n");
}
void set_Path() {
    char base_path[MAXSIZE] = "";
    printf("Enter the path for the folders");
    fgets(base_path, MAXSIZE, stdin);
    base_path[strcspn(base_path, "\n")] = 0;
    search_File(base_path);
    if (image_Count > 0) {
        randomizer();
        set_Up_Json(base_path);
        finish();
    }
    else
        printf("No images found in the specified directory 1!\n");
    free_ImageHolder();
}
int main() {
    //Setting up the program and getting the path
    FILE* openJson = fopen("Caching.json", "r");
    if (openJson == NULL) {
        printf("Problem with file opening :))\n");
        return 1;
    }
    // Use this for when u write the get function, this function will only run for the set
    char path_Read_From_Json[MAXSIZE];
    const int bytesRead = (int)fread(path_Read_From_Json, 1, sizeof(path_Read_From_Json) - 1, openJson);
    path_Read_From_Json[bytesRead] = '\0';
    fclose(openJson);
    const cJSON* root = cJSON_Parse(path_Read_From_Json);
    if (root == NULL) {
        printf("There is no path inputted from previous entries or the current folder does not exist\n");
        set_Path();
    }
    else {
        get_From_Json(root, root->child->valuestring);
        const cJSON *base_path = cJSON_GetObjectItemCaseSensitive(root, "Folder_Path");
        if (image_Count > 0) {
        set_Up_Json(base_path->valuestring);
            finish();
        }
        else {
            printf("No images are left to cycle through! :(\nFinding new images to find.\n");
            search_File(base_path->valuestring);
            if (image_Count > 0) {
                randomizer();
                set_Up_Json(base_path->valuestring);
                finish();
            }
            else
                printf("No images found in the specified directory 0!\n");
            free_ImageHolder();
        }
    }
    return 0;
}