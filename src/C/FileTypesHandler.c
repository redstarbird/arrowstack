#include "FileTypesHandler.h"

struct FileType *FileTypes;

static void SetString(char *stringToSet, char *ReplaceString, size_t size)
{
    strncpy(stringToSet, ReplaceString, size);
}

static void SetFileExtension(char *stringToSet, char *ReplaceString)
{
    SetString(stringToSet, ReplaceString, 16);
}

static void SetShortName(char *stringToSet, char *ReplaceString)
{
    SetString(stringToSet, ReplaceString, 10);
}

void InitFileTypes() // Probably a better way to do this
{
    printf("Setting up file types struct\n");
    unsigned int FileTypesNum = 5;
    FileTypes = malloc(sizeof(struct FileType) * (FileTypesNum + 1));

    /*
        ╭━━━╮╭━━━╮╭━━━╮
        ┃╭━╮┃┃╭━╮┃┃╭━╮┃
        ┃┃╱╰╯┃╰━━╮┃╰━━╮
        ┃┃╱╭╮╰━━╮┃╰━━╮┃
        ┃╰━╯┃┃╰━╯┃┃╰━╯┃
        ╰━━━╯╰━━━╯╰━━━╯*/
    SetFileExtension(FileTypes[0].FileExtensions[0], "css");
    SetShortName(FileTypes[0].ShortName, "CSS");
    FileTypes[0].id = CSSFILETYPE_ID;

    /*
          ╭╮╭━━━╮
          ┃┃┃╭━╮┃
          ┃┃┃╰━━╮
        ╭╮┃┃╰━━╮┃
        ┃╰╯┃┃╰━╯┃
        ╰━━╯╰━━━╯*/
    SetFileExtension(FileTypes[1].FileExtensions[0], "js");
    SetShortName(FileTypes[1].ShortName, "CSS");
    FileTypes[1].id = JSFILETYPE_ID;

    /*
        ╭╮ ╭╮╭━━━━╮╭━╮╭━╮╭╮
        ┃┃ ┃┃┃╭╮╭╮┃┃┃╰╯┃┃┃┃
        ┃╰━╯┃╰╯┃┃╰╯┃╭╮╭╮┃┃┃
        ┃╭━╮┃  ┃┃  ┃┃┃┃┃┃┃┃ ╭╮
        ┃┃ ┃┃  ┃┃  ┃┃┃┃┃┃┃╰━╯┃
        ╰╯ ╰╯  ╰╯  ╰╯╰╯╰╯╰━━━╯*/
    SetFileExtension(FileTypes[2].FileExtensions[0], "html");
    SetFileExtension(FileTypes[2].FileExtensions[1], "htm");
    SetShortName(FileTypes[2].ShortName, "HTML");
    FileTypes[2].id = HTMLFILETYPE_ID;
}