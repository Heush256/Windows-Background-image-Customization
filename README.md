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

Stores information of the images into an array of stucts:

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

This will be stored into the json file.

    "Folder_Path":	"C:\\Some\\random\\path\\to parent folder",
	"PLAY_SOUND":	false,
	"CHANGE_COLOR":	false,
	"Arrays_Of_Images":	[{
			"Folder":	19,
			"Image":	"r-291.avif",
			"Enum":	2,
			"ARGB":	10595514
		}, {
			"Folder":	19,
			"Image":	"r-68.avif",
			"Enum":	2,
			"ARGB":	2104093
		}, {
			"Folder":	19,
			"Image":	"r-432.avif",
			"Enum":	2,
			"ARGB":	11117733
		}
The "Folder" being the integer value of the last letter for the style (stored in Top_filepath variable), "Image" being the name and extension of the individual image (stored in Specific_Image variable), "Enum" being the style to set the image (stored in current_Style variable), "ARGB" being the color value of the image (stored in color variable).
