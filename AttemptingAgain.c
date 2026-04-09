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
    Fit,
    Fill,
    Stretch,
    Center,
    Span,
    Tile
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
void add_to_list(const char* path, const char* base_path) {
    size_t new_len = strlen(base_path) + strlen(path) + 2; // +2 for '\\' and '\0'
    char* temp = malloc(new_len);
    if (!temp) {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strcpy(temp, base_path);
    strcat(temp, "\\");
    strcat(temp, path);
     // If we need more space
    if (image_Count >= image_Capacity) {
        // Double the size when growing (more efficient than just +1)
        image_Capacity = image_Capacity == 0 ? INITIAL_CAPACITY : image_Capacity * 2;
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
    listOfImages[image_Count].current_Style = Tile; // Default style
    // listOfImages[image_Count].used_Yet = false;
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
        cJSON_AddItemToArray(json_Array, temp_Json_Storage);
    }

    cJSON_AddItemToObject(json_Obj_root, "Arrays_To_Path_Of_Images", json_Array);

    if (cJSON_Print(json_Obj_root))
        fputs(cJSON_Print(json_Obj_root), openJson);

    fclose(openJson);
    cJSON_Delete(json_Obj_root);
}
void search_File(const char* base_path) {
    DIR *dir = opendir (base_path);
    struct dirent *ent;
    if (dir != NULL) {
        /* print all the files and directories within directory and adds them to list*/
        while ((ent = readdir (dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
                continue;
            if (is_image(ent->d_name)) {
                add_to_list(ent->d_name, base_path);
            }
            else {
                printf("This does not have a valid extension: %s", ent->d_name);
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        perror ("Could not open directory!");
    }
}
void randomizer() {
    srand(time(NULL));
    for (int i = image_Count - 1; i > 0; i--) {
        const int position = rand() % (i + 1);
        char* temp = listOfImages[i].filePaths;
        listOfImages[i].filePaths = listOfImages[position].filePaths;
        listOfImages[position].filePaths = temp;  // Simple pointer swap
        size_t tempPos = listOfImages[i].position;
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
void add_To_List_After(char* path) {
    if (image_Count >= image_Capacity) {
        // Double the size when growing (more efficient than just +1)
        image_Capacity = image_Capacity == 0 ? INITIAL_CAPACITY : image_Capacity * 2;
        struct ImageHolder* newList = realloc(listOfImages, image_Capacity * sizeof(struct ImageHolder));
        if (!newList) {
            fprintf(stderr, "Memory allocation failed!\n");
            exit(1);
        }
        listOfImages = newList;
    }
    listOfImages[image_Count].filePaths = path;
    listOfImages[image_Count].position = image_Count;
    listOfImages[image_Count].current_Style = Tile; // Default style
    image_Count++;
}
void get_From_Json(const cJSON *root) {
    const cJSON *images_array = cJSON_GetObjectItemCaseSensitive(root, "Arrays_To_Path_Of_Images");
    if (!cJSON_IsArray(images_array)) return;
    const cJSON *item = NULL;
    int index = 0;
    cJSON_ArrayForEach(item, images_array) {
        /*const cJSON *path_node = cJSON_GetObjectItemCaseSensitive(item, "Absolute_Path_Of_Images");

        if (cJSON_IsString(path_node) && path_node->valuestring != NULL) {
            // Reusing your add_to_list function ensures
            // filePaths is properly malloc'd and the struct grows.
            add_To_List_After(path_node->valuestring);
        }*/
        // if (index > 0) { // Skips the first image (index 0)
            const cJSON *path_node = cJSON_GetObjectItemCaseSensitive(item, "Absolute_Path_Of_Images");
            if (cJSON_IsString(path_node) && path_node->valuestring != NULL) {
                add_To_List_After(path_node->valuestring);
            }
        // }
        // index++;
    }
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
        for (int i = 0; i < image_Count; i++)
            printf("This image is here %d: %s\n", i, listOfImages[i].filePaths);
        if (SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, listOfImages[0].filePaths, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE))
            printf("Wallpaper set successfully to: %s\n", listOfImages[0].filePaths);
        else
            printf("Failed to set wallpaper\n");
    }
    else
        printf("No images found in the specified directory 1!\n");
    free_ImageHolder();
}
void get_Path() {
    FILE* openJson = fopen("Caching.json", "r");
    if (openJson == NULL) {
        printf("Problem with file openning :))\n");
        return;
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
        get_From_Json(root);
        const cJSON *base_path = cJSON_GetObjectItemCaseSensitive(root, "Folder_Path");
        // search_File(base_path->valuestring);
        if (image_Count > 0) {
            // randomizer();
            set_Up_Json(base_path->valuestring);
            for (int i = 0; i < image_Count; i++)
                printf("This image is here %d: %s\n", i, listOfImages[i].filePaths);
            if (SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, listOfImages[0].filePaths, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE))
                printf("Wallpaper set successfully to: %s\n", listOfImages[0].filePaths);
            else
                printf("Failed to set wallpaper\n");
        }
        else {
            // printf("No images found in the specified directory 3!\n");
            free_ImageHolder();
            printf("No images are left to cycle through! :(\nFinding new images to find.\n");
            search_File(base_path->valuestring);
            if (image_Count > 0) {
                randomizer();
                set_Up_Json(base_path->valuestring);
                for (int i = 0; i < image_Count; i++)
                    printf("This image is here %d: %s\n", i, listOfImages[i].filePaths);
                if (SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, listOfImages[0].filePaths, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE))
                    printf("Wallpaper set successfully to: %s\n", listOfImages[0].filePaths);
                else
                    printf("Failed to set wallpaper\n");
            }
            else
                printf("No images found in the specified directory 1!\n");
            free_ImageHolder();
        }
        free_ImageHolder();
    }
}
int main() {
    get_Path();
    return 0;
}