// This is where scripts' loading functions should be declared:
void AddSC_administration_commandscript();
void AddSC_freedom_commandscript();
// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddFreedomScripts()
{
    AddSC_administration_commandscript();
    AddSC_freedom_commandscript();
}
