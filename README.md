# Windows-Background-image-Style-Changer
Windows os allows for the user to rotate their background images for different styles was lackluster. This program can also play a sound, and change the system colors too if the user wants.

Uses Windows api and the cJSON library. Windows api to change registry keys to certain styles based on the folder it is in, and cJSON to easily read and write to a file.

To easily and quickly change the registry keys, below is a small look up table:

    typedef struct {
    const char* styleStr;
    const DWORD styleSize;
    const char* tileStr;
    } styleStr_and_tileStr;
    //A list to easily get the registry modes
    static const styleStr_and_tileStr REG_MODES[] ={
        { "0",  2, "0" }, // 0: CENTER
        { "2",  2, "0" }, // 1: STRETCH
        { "6",  2, "0" }, // 2: FIT
        { "10", 3, "0" }, // 3: FILL
        { "0",  2, "1" }, // 4: TILE
        { "22", 3, "0" }  // 5: SPAN
    };
