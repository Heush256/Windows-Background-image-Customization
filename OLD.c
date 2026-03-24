/*This block of code works but now it is time to make it rotate within the folder and multiple images
 *#include <windows.h>

int main() {
    const char* path = "C:/Users/1toas/OneDrive/Pictures/Background wallpaper/Yus.jpeg";
    // SPI_SETDESKWALLPAPER updates the desktop background
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void*)path, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
    return 0;
}*/

/*#include <windows.h>
int main() {
    //Main path that leads to the file, now must concatenate the image to it and then randomize it.
    char* path = "C:/Users/1toas/OneDrive/Pictures/Background wallpaper/";
    char* adding;

    // SPI_SETDESKWALLPAPER updates the desktop background
    SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (void*)path, SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);
    return 0;
}*/

/*
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <dirent.h>

void listFiles(const char* dirname, int depth) {
    // Prevent going too deep (avoid infinite recursion)
    if (depth > 10) {
        return;
    }

    DIR* dir = opendir(dirname);
    if (dir == NULL) {
        printf("Could not open directory: %s\n", dirname);
        return;
    }

    struct dirent* entity;
    while ((entity = readdir(dir)) != NULL) {
        // Skip current and parent directory entries
        if (strcmp(entity->d_name, ".") == 0 || strcmp(entity->d_name, "..") == 0) {
            continue;
        }

        // Build full path
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, entity->d_name);

        // Check if it's a directory using Windows API
        DWORD attributes = GetFileAttributesA(fullpath);
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            // Only recurse into visible directories (not hidden/system)
            if (!(attributes & FILE_ATTRIBUTE_HIDDEN) && !(attributes & FILE_ATTRIBUTE_SYSTEM)) {
                printf("[DIR]  %s\n", entity->d_name);
                listFiles(fullpath, depth + 1);
            } else {
                printf("[DIR]  %s (skipped - hidden/system)\n", entity->d_name);
            }
        } else {
            // Only show common image files to reduce output
            const char* ext = strrchr(entity->d_name, '.');
            if (ext && (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0 ||
                       strcmp(ext, ".png") == 0 || strcmp(ext, ".bmp") == 0)) {
                printf("[FILE] %s\n", entity->d_name);
                       }
        }
    }

    closedir(dir);
}

int main(int argc, char* argv[]) {
    const char* path = argc > 1 ? argv[1] : "./Pics";
    printf("Listing image files in: %s\n\n", path);
    listFiles(path, 0);
    return 0;
}*/