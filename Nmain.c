#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <strings.h> // For strcasecmp (case-insensitive string comparison)
#include <windows.h>

// Structure to hold our dynamic array of image paths
typedef struct {
    char **filepaths;
    size_t count;
    size_t capacity;
} ImageList;

// 1. Helper function: Check if a file extension matches common image formats
int is_image(const char *filename) {
    const char *dot = strrchr(filename, '.');
    // If there's no extension, or the file starts with a dot (hidden file)
    if (!dot || dot == filename) return 0;
    
    const char *ext = dot + 1; // Get the string after the dot
    
    // Check against common image extensions (case-insensitive)
    if (strcasecmp(ext, "jpg") == 0 ||
        strcasecmp(ext, "jpeg") == 0 ||
        strcasecmp(ext, "png") == 0 ||
        strcasecmp(ext, "gif") == 0 ||
        strcasecmp(ext, "webp") == 0 ||
        strcasecmp(ext, "bmp") == 0) {
        return 1;
    }
    return 0;
}

// 2. Helper function: Add a file path to our dynamic array
void add_to_list(ImageList *list, const char *filepath) {
    // If the array is full, double its capacity
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->filepaths = realloc(list->filepaths, list->capacity * sizeof(char *));
        if (!list->filepaths) {
            fprintf(stderr, "Memory allocation failed!\n");
            free(list->filepaths);
            exit(1);
        }
    }
    // Duplicate the string and add it to the array
    list->filepaths[list->count++] = strdup(filepath);
}

// 3. The Recursive Search Function
void search_images_recursive(const char *base_path, ImageList *list) {
    struct dirent *entry;
    
    // Try to open the directory. If it fails (e.g., permission denied), skip it.
    if (!(/*dir = */opendir(base_path))) {
        return; 
    }
    DIR *dir = opendir(base_path);
    // Read directory entries one by one
    while ((entry = readdir(dir)) != NULL) {
        // Skip the current directory "." and parent directory ".." to prevent infinite loops
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char path[1024];
        // Construct the full path
        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);
        
        struct stat statbuf;
        if (stat(path, &statbuf) == -1) continue;
        
        // If it's a directory, recurse into it
        if (S_ISDIR(statbuf.st_mode)) {
            search_images_recursive(path, list);
        } 
        // If it's a regular file, check the extension
        else if (S_ISREG(statbuf.st_mode)) {
            if (is_image(entry->d_name)) {
                add_to_list(list, path);
            }
        }
    }
    closedir(dir);
}

// 4. Main wrapper function to initialize the list and start the search
ImageList* get_all_images(const char *path) {
    ImageList *list = malloc(sizeof(ImageList));
    list->capacity = 16; // Start with space for 16 items
    list->count = 0;
    list->filepaths = malloc(list->capacity * sizeof(char *));
    search_images_recursive(path, list);
    return list;
}

// 5. Cleanup function to prevent memory leaks
void free_image_list(ImageList *list) {
    for (size_t i = 0; i < list->count; i++) {
        free(list->filepaths[i]); // Free each string
    }
    free(list->filepaths); // Free the array of pointers
    free(list);            // Free the struct itself
}

//Remove the extra ".."
void remove_all_but_last(char* str, char char_to_keep) {
    // 1. Find the index of the last occurrence of the character
    char* last_occurrence_ptr = strrchr(str, char_to_keep);
    int last_index;

    if (last_occurrence_ptr != NULL) {
        last_index = (int)(last_occurrence_ptr - str);
    } else {
        // If the character is not found, nothing to remove
        return;
    }

    // 2. Iterate through the string, keeping only the last occurrence and other characters
    for (int read_index = 0, write_index = 0; str[read_index] != '\0'; read_index++)
        // Keep the character if it's not the target character, or if it's the last occurrence
        if (str[read_index] != char_to_keep || read_index == last_index)
            str[write_index++] = str[read_index];
    // Null-terminate the modified string
    str[strlen(str)] = '\0';
}

// --- Example Usage ---
int main() {
    char path[1024];
    // 1. Get user input for the directory to start listing
    printf("Enter directory path (e.g., / or /home): ");
    if (fgets(path, sizeof(path), stdin) == NULL) return 1;
    path[strcspn(path, "\n")] = 0; // Remove newline character
    // 2. Open the directory
    DIR *dp = opendir(path);
    if (dp == NULL) {
        perror("opendir() error");
        return 1;
    }

    printf("\nDirectories in %s:\n", path);
    ImageList *images = get_all_images(path);


    printf("Found %zu images:\n", images->count);
    for (size_t i = 0; i < images->count; i++) {
        printf("%zu: %s\n", i + 1, images->filepaths[i]);
    }
    for (int i = 0; i < images->count; i++) {
        remove_all_but_last(images->filepaths[i], '.');
        printf("This image is now in rotation: %s\n", images->filepaths[i]);
        Sleep(2000);
        // SPI_SETDESKWALLPAPER updates the desktop background
        SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void*)images->filepaths[i]/*(void*)full_path*/, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
    }

    // Always clean up!
    free_image_list(images);
    closedir(dp);

    return 0;
}