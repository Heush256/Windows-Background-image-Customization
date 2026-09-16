# Windows-Background-image-Style-Changer
Just cuse why not?

But in all seriousness, how windows os allows for the user to rotate their background images for different styles was lackluster. This program can also play a sound, and change the system colors too if the user wants.

Uses Windows api to change registry keys to certain styles based on the folder it is in. (ie. REG_MODES[] ={
    { "0",  2, "0" }, // 0: CENTER
    { "2",  2, "0" }, // 1: STRETCH
    { "6",  2, "0" }, // 2: FIT
    { "10", 3, "0" }, // 3: FILL
    { "0",  2, "1" }, // 4: TILE
    { "22", 3, "0" }  // 5: SPAN
};
the 1st value being the value style of the style as a str, the 2nd value being the size of the str in the 1st column, and the 3rd value being the title string)
