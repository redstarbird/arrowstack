#include "BundleFiles.h"

struct ImportedESM
{
    char *name;
    char *alias;
    bool IsDefault;
    bool IsArrayEnd;
} ImportedESM;

int IsEndOfJSName(const char character)
{
    const char disallowed_chars[] = {' ', '!', '@', '#', '%', '^', '&', '*', '(', ')', '-', '+', '=', '{', '}', '[', ']', ':', ';', '"', '\'', '<', '>', ',', '.', '?', '/', '+', '-', '*', '/', '%', '+', '-', '\n'};
    const int array_size = sizeof(disallowed_chars) / sizeof(disallowed_chars[0]);
    for (int i = 0; i < array_size; i++)
    {
        if (disallowed_chars[i] == character)
        {
            return true;
        }
    }
    return false;
}

void BundleFile(struct Node *GraphNode)
{
    int ShiftLocationsLength = 1; // Includes end element to signal the end of the array
    struct ShiftLocation *ShiftLocations = malloc(sizeof(ShiftLocation));
    ShiftLocations[0].location = -1; // Indicates end of array although probably not needed because the length of the array is being stored

    int FileTypeID = GetFileTypeID(GraphNode->path);

    char *FileContents = ReadDataFromFile(GraphNode->path);
    if (FileContents == NULL)
    {
        return;
    }
    struct Edge *CurrentEdge = GraphNode->edge;
    while (CurrentEdge != NULL)
    {
        struct Node *CurrentDependency = CurrentEdge->vertex;
        ColorGreen();
        printf("Building file: %s\n", CurrentDependency->path);
        ColorNormal();
        char *DependencyExitPath = EntryToExitPath(CurrentDependency->path);
        char *InsertText = ReadDataFromFile(DependencyExitPath);
        int DependencyFileType = GetFileTypeID(DependencyExitPath);

        if (FileTypeID == HTMLFILETYPE_ID)
        {
            if (DependencyFileType == HTMLFILETYPE_ID)
            {
                int InsertEnd = CurrentEdge->EndRefPos + 1;
                FileContents = ReplaceSectionOfString(
                    FileContents,
                    GetShiftedAmount(CurrentEdge->StartRefPos, ShiftLocations),
                    GetShiftedAmount(CurrentEdge->EndRefPos + 1, ShiftLocations), InsertText);
                // totalAmountShifted += strlen(InsertText) - (InsertEnd - GraphNode->Dependencies[i].StartRefPos);
                AddShiftNum(CurrentEdge->StartRefPos, strlen(InsertText) - (InsertEnd - CurrentEdge->StartRefPos), &ShiftLocations, &ShiftLocationsLength);
            }
            else if (DependencyFileType == CSSFILETYPE_ID) // Bundle CSS into HTML file
            {
                if (Settings.bundleCSSInHTML == true)
                {
                    char *InsertString;
                    struct RegexMatch *StyleResults = GetAllRegexMatches(FileContents, "<style[^>]*>", 0, 0);
                    if (StyleResults[0].IsArrayEnd)
                    {
                        // Style tag doesn't already exist
                        InsertString = malloc(strlen(InsertText) + 16); // allocates space for start and end of <style> tag
                        strcpy(InsertString, "<style>");
                        strcpy(InsertString + 7, InsertText);
                        strcat(InsertString, "</style>");
                        struct RegexMatch *HeadTagResults = GetAllRegexMatches(FileContents, "< ?head[^>]*>", 0, 0);
                        if (HeadTagResults[0].IsArrayEnd == false) // If <head> tag is found
                        {

                            RemoveSectionOfString(
                                FileContents,
                                GetShiftedAmount(CurrentEdge->StartRefPos, ShiftLocations),
                                GetShiftedAmount(CurrentEdge->EndRefPos, ShiftLocations) + 1);
                            // totalAmountShifted -= (GraphNode->Dependencies[i].EndRefPos - GraphNode->Dependencies[i].StartRefPos + 1);
                            AddShiftNum(CurrentEdge->StartRefPos, (CurrentEdge->EndRefPos - CurrentEdge->StartRefPos + 1) * -1, &ShiftLocations, &ShiftLocationsLength);
                            FileContents = InsertStringAtPosition(FileContents, InsertString, HeadTagResults[0].EndIndex);
                            AddShiftNum(GetInverseShiftedAmount(HeadTagResults[0].EndIndex, ShiftLocations), strlen(InsertString), &ShiftLocations, &ShiftLocationsLength);
                            // totalAmountShifted += strlen(InsertString);
                        }
                        else
                        {
                            ColorYellow();
                            printf("No <head> tag found for file: %s, unable to bundle CSS into file, file will still work\n", GraphNode->path);
                            ColorReset();
                        }
                    }
                    else
                    {
                        FileContents = InsertStringAtPosition(FileContents, InsertText, StyleResults[0].EndIndex);
                    }
                }
            }
            else if (DependencyFileType == JSFILETYPE_ID)
            {
                char *ReferencedString = getSubstring(FileContents, GetShiftedAmount(CurrentEdge->StartRefPos, ShiftLocations), GetShiftedAmount(CurrentEdge->EndRefPos, ShiftLocations));
                if (!StringContainsSubstring(ReferencedString, " defer") && !StringContainsSubstring(ReferencedString, "async"))
                {
                    int startlocation = -1;
                    int endlocation = -1;
                    int TempShiftedAmount = GetShiftedAmount(CurrentEdge->EndRefPos, ShiftLocations);
                    for (int v = GetShiftedAmount(CurrentEdge->StartRefPos, ShiftLocations); v < strlen(FileContents); v++)
                    {

                        if (strncasecmp(FileContents + v, "src", 3) == 0)
                        {
                            startlocation = v;
                            break;
                        }
                    }
                    if (startlocation != -1)
                    {

                        endlocation = startlocation;
                        bool pastEquals = false;
                        bool pastText = false;
                        for (int v = startlocation; v < strlen(FileContents); v++)
                        {
                            if (!pastEquals)
                            {
                                if (FileContents[v] == '=')
                                {
                                    pastEquals = true;
                                }
                            }
                            else if (!pastText)
                            {
                                if (FileContents[v] != '\'' && FileContents[v] != '\"' && FileContents[v] != ' ')
                                {
                                    pastText = true;
                                }
                            }
                            else
                            {
                                if (FileContents[v] == '\'' || FileContents[v] == '\"' || FileContents[v] == ' ')
                                {
                                    endlocation = v + 1;
                                    break;
                                }
                                else if (FileContents[v] == '>' || FileContents[v] == '\0')
                                {
                                    endlocation = v - 1;
                                    break;
                                }
                            }
                        }
                        RemoveSectionOfString(FileContents, startlocation, endlocation);
                        AddShiftNum(CurrentEdge->StartRefPos, (endlocation - startlocation) * -1, &ShiftLocations, &ShiftLocationsLength);
                        FileContents = InsertStringAtPosition(FileContents, InsertText, GetShiftedAmount(CurrentEdge->EndRefPos + 1, ShiftLocations));
                        AddShiftNum(CurrentEdge->EndRefPos + 1, strlen(InsertText), &ShiftLocations, &ShiftLocationsLength);
                    }
                }
            }
            else // Will hopefully work for most custom dependencies
            {
                char *InsertText = ReadDataFromFile(DependencyExitPath);
                int InsertEnd2 = CurrentEdge->EndRefPos + 1;
                FileContents = ReplaceSectionOfString(FileContents, GetShiftedAmount(CurrentEdge->StartRefPos, ShiftLocations), GetShiftedAmount(InsertEnd2, ShiftLocations), InsertText);
                AddShiftNum(CurrentEdge->StartRefPos, strlen(InsertText) - (InsertEnd2 - CurrentEdge->StartRefPos), &ShiftLocations, &ShiftLocationsLength);
            }
        }
        else if (FileTypeID == CSSFILETYPE_ID)
        {
            if (DependencyFileType == CSSFILETYPE_ID)
            {
                char *InsertText = ReadDataFromFile(DependencyExitPath);
                int InsertEnd2 = CurrentEdge->EndRefPos + 1;
                FileContents = ReplaceSectionOfString(FileContents, GetShiftedAmount(CurrentEdge->StartRefPos, ShiftLocations), GetShiftedAmount(InsertEnd2, ShiftLocations), InsertText);

                AddShiftNum(CurrentEdge->StartRefPos, strlen(InsertText) - (InsertEnd2 - CurrentEdge->StartRefPos), &ShiftLocations, &ShiftLocationsLength);
            }
        }
        else if (FileTypeID == JSFILETYPE_ID)
        {
            if (DependencyFileType == JSFILETYPE_ID)
            {
                char *ReferenceText = getSubstring(FileContents, GetShiftedAmount(CurrentEdge->StartRefPos, ShiftLocations), GetShiftedAmount(CurrentEdge->EndRefPos, ShiftLocations));
                bool ISESModule = StringStartsWith(ReferenceText, "import "); //  Checks if the current dependency is an ESModule
                struct RegexMatch *IteratePointer;                            // Defines Iterate Pointer
                char *InsertText = ReadDataFromFile(DependencyExitPath);      // Reads the data for the current dependency
                struct RegexMatch *ExtraExportMatches;                        // Defines Extra Export Matches for commonJS modules to use
                struct RegexMatch *UsableExtraImports;                        // Defines usable extra imports needed for commonJS modules
                struct RegexMatch *FinalElement;

                int FullExportsArrayLength;
                char *NewModuleExportsName;
                int ImportedFunctionNameLength = 0;
                struct ImportedESM *ImportedFunctionNames = malloc(sizeof(struct ImportedESM));
                ImportedFunctionNames[0].IsArrayEnd = true;

                struct RegexMatch *FunctionNames = GetAllRegexMatches(InsertText, "function [^(]*", 9, 1);
                IteratePointer = &FunctionNames[0];
                for (int i = 0; i < strlen(IteratePointer->Text); i++)
                {
                    if (IsEndOfJSName(IteratePointer->Text[i]))
                    {
                        IteratePointer->Text[i] = '\0';
                        break;
                    }
                }

                if (ISESModule) // If the current dependency is an ESModule then this code is run to find the exports
                {
                    bool ImportDefault, ImportNamed, ImportAll, ImportAllAlias = false; // Tracks how/what needs to be imported
                    if (HasRegexMatch(ReferenceText, "import\\s+*"))                    // Detects if the import is importing all exports
                    {
                        ImportAll = true;
                        if (HasRegexMatch(ReferenceText, "import\\s+*\\s+as")) // Checks if the import is using an alias
                        {
                            ImportAllAlias = true;
                        }
                    }
                    else if (LastOccurenceOfChar(ReferenceText, '{') != -1) // Checks if the import uses named exports
                    {
                        int EndLocation = LastOccurenceOfChar(ReferenceText, '}');
                        bool InVariable = false;
                        int CurrentVariableStart = 0;
                        for (int i = LastOccurenceOfChar(ReferenceText, '{'); i <= EndLocation; i++)
                        {
                            if (!InVariable)
                            {
                                if (!IsEndOfJSName(ReferenceText[i]))
                                {
                                    InVariable = true;
                                    CurrentVariableStart = i;
                                    ImportedFunctionNameLength++;
                                    ImportedFunctionNames = realloc(ImportedFunctionNames, sizeof(struct ImportedESM) * (ImportedFunctionNameLength + 1));
                                }
                            }
                            else
                            {
                                if (IsEndOfJSName(ReferenceText[i]))
                                {
                                    InVariable = false;
                                    // ImportedFunctionNames[ImportedFunctionNameLength] = malloc(sizeof(char) * (2 + i - CurrentVariableStart));
                                    if (0) {

                                    }
                                        ImportedFunctionNames[ImportedFunctionNameLength - 1].name = getSubstring(ReferenceText, CurrentVariableStart, i);
                                }
                            }
                        }
                        ImportNamed = true;
                        ImportDefault = HasRegexMatch(ReferenceText, "import[^,;]*,[^;]*{[^;]*}[^;]*;");
                    }
                    else
                    {
                        ImportDefault = true;
                    }
                    struct RegexMatch *DefaultExport;
                    struct RegexMatch *Exports;
                    if (ImportDefault || ImportAll)
                    {
                        DefaultExport = GetRegexMatch(InsertText, "export\\s+default");
                    }
                    if (ImportNamed || ImportAll)
                    {
                        Exports = GetAllRegexMatches(InsertText, "export[^;]+;", 7, 0);
                    }

                    IteratePointer = &Exports[0];

                    while (IteratePointer->IsArrayEnd == false) // Removes export if it is a default export
                    {
                        struct RegexMatch *IteratePointer2 = &FunctionNames[0];

                        while (IteratePointer2->IsArrayEnd != true)
                        {
                            if (IteratePointer2->StartIndex > IteratePointer->StartIndex && IteratePointer2->StartIndex < IteratePointer->EndIndex)
                            {
                                RemoveRegexMatch(IteratePointer2);
                            }
                        }

                        if (HasRegexMatch(IteratePointer->Text, "export\\s+default"))
                        {
                            RemoveRegexMatch(IteratePointer); // Doesn't break in the case of the JS being invalid and having 2 default exports
                        }

                        IteratePointer++;
                    }

                    IteratePointer = &FunctionNames[0];
                }
                else // If the current dependency is a CommonJS module then this code is run to find the exports
                {
                    struct RegexMatch *FullExportMatches = GetAllRegexMatches(InsertText, "module\\.exports\\s*=\\s*[^;]*", 0, 0);
                    FullExportsArrayLength = RegexMatchArrayLength(FullExportMatches);

                    if (FullExportsArrayLength != 0)
                    {
                        FinalElement = &FullExportMatches[FullExportsArrayLength - 1];
                    }
                    ExtraExportMatches = GetAllRegexMatches(InsertText, "[^.]exports.[^;]*", 0, 0);
                    UsableExtraImports = &ExtraExportMatches[0];
                    if (FullExportsArrayLength > 0)
                    {
                        while (UsableExtraImports->IsArrayEnd != true)
                        {
                            if (UsableExtraImports->StartIndex < FinalElement->EndIndex)
                            {
                                UsableExtraImports++;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                    NewModuleExportsName = malloc(CurrentEdge->EndRefPos - CurrentEdge->StartRefPos + 11);
                    strcpy(NewModuleExportsName, getSubstring(FileContents, GetShiftedAmount(CurrentEdge->StartRefPos + 9, ShiftLocations), GetShiftedAmount(CurrentEdge->EndRefPos - 2, ShiftLocations)));
                    RemoveCharFromString(NewModuleExportsName, '/');
                    strcat(NewModuleExportsName, "_ARROWPACK");

                    RemoveCharFromString(NewModuleExportsName, '.');
                    while (StringContainsSubstring(InsertText, NewModuleExportsName) || StringContainsSubstring(FileContents, NewModuleExportsName))
                    {
                        char *NewUniqueName = CreateUnusedName();
                        NewModuleExportsName = realloc(NewModuleExportsName, (strlen(NewUniqueName) + strlen(NewModuleExportsName) + 1) * sizeof(char));
                        strcat(NewModuleExportsName, NewUniqueName);
                    }
                }
                // This code is run once the exports have been found
                struct ShiftLocation *JSFileShiftLocations = malloc(sizeof(ShiftLocation));
                JSFileShiftLocations[0].location = -1;
                int JSShiftLocationsLength = 1;

                while (IteratePointer->IsArrayEnd != true)
                {
                    if (strlen(IteratePointer->Text) > 1) // Ignores unamed functions
                    {

                        bool InString, StringStartDoubleQuotes, FunctionDuplicateFound = false;
                        for (int i = 0; i < strlen(FileContents) - strlen(IteratePointer->Text); i++)
                        {
                            if (FileContents[i] == '\'' || FileContents[i] == '\"')
                            {
                                if (InString)
                                {
                                    if (StringStartDoubleQuotes)
                                    {
                                        if (FileContents[i] == '\"')
                                        {
                                            InString = false;
                                        }
                                    }
                                    else
                                    {
                                        if (FileContents[i] == '\'')
                                        {
                                            InString = false;
                                        }
                                    }
                                }
                                else
                                {
                                    InString = true;
                                    StringStartDoubleQuotes = FileContents[i] == '\"';
                                }
                            }
                            else if (strncmp(FileContents + i, IteratePointer->Text, strlen(IteratePointer->Text)) == 0 && !InString)
                            {
                                ColorGreen();
                                printf("Resolving name collision: %s\n", FileContents + i);
                                ColorNormal();
                                FunctionDuplicateFound = true;
                                break;
                            }
                        }
                        if (FunctionDuplicateFound)
                        {
                            char *NewUnusedName = CreateUnusedName();
                            InString = false;
                            StringStartDoubleQuotes = false;
                            FunctionDuplicateFound = false;
                            int LoopLength = strlen(InsertText) - strlen(IteratePointer->Text) + 2;
                            for (int i = 0; i < LoopLength; i++)
                            {
                                if (InsertText[i] == '\'' || InsertText[i] == '\"')
                                {
                                    if (InString)
                                    {
                                        if (StringStartDoubleQuotes)
                                        {
                                            if (InsertText[i] == '\"')
                                            {
                                                InString = false;
                                            }
                                        }
                                        else
                                        {
                                            if (InsertText[i] == '\'')
                                            {
                                                InString = false;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        InString = true;
                                        StringStartDoubleQuotes = InsertText[i] == '\"';
                                    }
                                }
                                else if (strncmp(InsertText + i, IteratePointer->Text, strlen(IteratePointer->Text)) == 0 && !InString)
                                {
                                    InsertText = InsertStringAtPosition(InsertText, NewUnusedName, i + strlen(IteratePointer->Text));
                                    int InverseShiftAmount = GetInverseShiftedAmount(i + strlen(IteratePointer->Text), JSFileShiftLocations);
                                    AddShiftNum(InverseShiftAmount, strlen(NewUnusedName), &JSFileShiftLocations, &JSShiftLocationsLength);
                                }
                            }
                        }
                    }
                    IteratePointer++;
                }
                if (ISESModule)
                {
                }
                else
                {

                    IteratePointer = &ExtraExportMatches[0];
                    while (IteratePointer != UsableExtraImports && IteratePointer->IsArrayEnd == false)
                    {
                        RemoveSectionOfString(InsertText, IteratePointer->StartIndex, IteratePointer->EndIndex);
                        AddShiftNum(IteratePointer->StartIndex, (IteratePointer->EndIndex - IteratePointer->StartIndex) * -1, &JSFileShiftLocations, &JSShiftLocationsLength);
                        IteratePointer++;
                    }

                    char *ModuleObjectDefinition = malloc(strlen(NewModuleExportsName) + 13);
                    strcpy(ModuleObjectDefinition, "let ");
                    strcat(ModuleObjectDefinition, NewModuleExportsName);
                    strcat(ModuleObjectDefinition, " = {};");
                    if (FullExportsArrayLength > 0)
                    {
                        InsertText = InsertStringAtPosition(InsertText, ModuleObjectDefinition, GetShiftedAmount(FinalElement->StartIndex, JSFileShiftLocations));
                        AddShiftNum(FinalElement->StartIndex, strlen(ModuleObjectDefinition), &JSFileShiftLocations, &JSShiftLocationsLength);
                        InsertText = ReplaceSectionOfString(InsertText, GetShiftedAmount(FinalElement->StartIndex, JSFileShiftLocations), GetShiftedAmount(FinalElement->StartIndex, JSFileShiftLocations) + 14, NewModuleExportsName);
                        AddShiftNum(FinalElement->StartIndex, strlen(NewModuleExportsName) - 14, &JSFileShiftLocations, &ShiftLocationsLength);
                    }
                    else
                    {
                        InsertText = InsertStringAtPosition(InsertText, ModuleObjectDefinition, UsableExtraImports->StartIndex);
                        AddShiftNum(UsableExtraImports->StartIndex, strlen(ModuleObjectDefinition), &JSFileShiftLocations, &JSShiftLocationsLength);
                    }

                    while (!UsableExtraImports->IsArrayEnd)
                    {
                        InsertText = ReplaceSectionOfString(InsertText, GetShiftedAmount(UsableExtraImports->StartIndex, JSFileShiftLocations), GetShiftedAmount(UsableExtraImports->StartIndex + 8, JSFileShiftLocations), NewModuleExportsName);
                        AddShiftNum(UsableExtraImports->StartIndex, strlen(NewModuleExportsName) - 8, &JSFileShiftLocations, &JSShiftLocationsLength);
                        UsableExtraImports++;
                    }
                }
                if (Settings.productionMode == false) // Keeps line numbers the same by turning new import into one line
                {
                    RemoveSingleLineComments(InsertText);
                    RemoveCharFromString(InsertText, '\n');
                }

                FileContents = ReplaceSectionOfString(FileContents, GetShiftedAmount(CurrentEdge->StartRefPos, ShiftLocations), GetShiftedAmount(CurrentEdge->EndRefPos, ShiftLocations) + 1, NewModuleExportsName);
                AddShiftNum(CurrentEdge->StartRefPos, strlen(NewModuleExportsName) - ((CurrentEdge->EndRefPos + 1) - CurrentEdge->StartRefPos), &ShiftLocations, &ShiftLocationsLength);
                FileContents = InsertStringAtPosition(FileContents, InsertText, 0);
                AddShiftNum(0, strlen(InsertText), &ShiftLocations, &ShiftLocationsLength);
                /*free(JSFileShiftLocations);
                JSFileShiftLocations = NULL;*/
            }
        }
        CurrentEdge = CurrentEdge->next;
    }
    free(ShiftLocations);
    ShiftLocations = NULL;
    RemoveSubstring(FileContents, "</include>");
    CreateFileWrite(EntryToExitPath(GraphNode->path), FileContents);
    ColorGreen();
    printf("Finished bundling file:%s\n", GraphNode->path);
    ColorNormal();
}

void PostProcessFile(struct Node *node, struct Graph *graph)
{
    if (node->path == NULL)
    {
        return;
    }
    struct ShiftLocation *shiftLocations = malloc(sizeof(struct ShiftLocation));
    int ShiftlocationLength = 0;
    char *ExitPath = EntryToExitPath(node->path);
    if (node->FileType == JSFILETYPE_ID)
    {
        char *FileContents = ReadDataFromFile(ExitPath);
        if (FileContents == NULL)
        {
            return;
        }
        struct RegexMatch *FullExportMatches = GetAllRegexMatches(FileContents, "module\\.exports\\s*=\\s*[^;]*", 0, 0);
        struct RegexMatch *IteratePointer = &FullExportMatches[0];
        while (IteratePointer->IsArrayEnd == false)
        {
            RemoveSectionOfString(FileContents, GetShiftedAmount(IteratePointer->StartIndex, shiftLocations), GetShiftedAmount(IteratePointer->EndIndex, shiftLocations) + 1);
            AddShiftNum(IteratePointer->StartIndex, IteratePointer->EndIndex - IteratePointer->StartIndex - 1, &shiftLocations, &ShiftlocationLength);
            IteratePointer++;
        }
        struct RegexMatch *SmallExportMatches = GetAllRegexMatches(FileContents, "[^.]exports.[^;]*", 0, 0);
        IteratePointer = &SmallExportMatches[0];
        while (IteratePointer->IsArrayEnd == false)
        {
            RemoveSectionOfString(FileContents, GetShiftedAmount(IteratePointer->StartIndex, shiftLocations), GetShiftedAmount(IteratePointer->EndIndex, shiftLocations) + 1);
            AddShiftNum(IteratePointer->StartIndex, IteratePointer->EndIndex - IteratePointer->StartIndex - 1, &shiftLocations, &ShiftlocationLength);
            IteratePointer++;
        }
        ColorGreen();
        printf("Post processed: %s", FileContents);
        ColorNormal();
        CreateFileWrite(ExitPath, FileContents);
    }
    free(shiftLocations);
    shiftLocations = NULL;
}

bool EMSCRIPTEN_KEEPALIVE BundleFiles(struct Graph *graph)
{
    bool Success = true;
    int FilesBundled;

    for (FilesBundled = 0; FilesBundled < graph->VerticesNum; FilesBundled++)
    {
        struct Node *FileNode = graph->SortedArray[FilesBundled];
        if (count_edges(FileNode) == 0)
        {
            char *ExitPath = strdup(FileNode->path);
            ExitPath = EntryToExitPath(ExitPath);

            CopyFile(FileNode->path, ExitPath);
            free(ExitPath);
        }
        else
        {
            FilesBundled--;
            break;
        }
    }

    for (int i = FilesBundled; i < graph->VerticesNum; i++)
    {
        struct Node *FileNode = graph->SortedArray[i];
        ColorMagenta();
        printf("\nBundling file: %s\n", FileNode->path);
        ColorReset();
        // print_progress_bar(i, graph->VerticesNum);
        BundleFile(FileNode);
    }

    // PostProcess functions that run after main bundling process
    for (int i = 0; i < graph->VerticesNum; i++)
    {
        PostProcessFile(graph->SortedArray[i], graph);
    }
    ColorNormal();  // Need to find out where everything is being turned green
    return Success; // This is always true currently
}