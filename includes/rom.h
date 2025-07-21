#ifndef ROM_H
#define ROM_H

/**
 * load_rom - Loads a Game Boy ROM into memory.
 * 
 * Parameters: 
 * @path: Path to the ROM file.
 *
 * Return: 1 on success, 0 on failure.
 */
int load_rom(const char* path);

#endif