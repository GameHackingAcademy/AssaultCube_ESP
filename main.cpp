/* An ESP for Assault Cube, a type of hack that displays information about enemy players above their heads.

It works by iterating over the enemy list and calculating the yaw and pitch required to aim at that enemy using arctangents.
This part of the code is taken from the aimbot code available at: 
https://github.com/GameHackingAcademy/AssaultCube_Aimbot/blob/master/main.cpp

The difference between the calculated pitch yaw and pitch and our player's yaw and pitch is then used to derive the screen
coordinates of the enemy. This is done by adding the difference times a scaling factor to the middle of the screen.

This must be injected into the Assault Cube process to work. One way to do this is to use a DLL injector. 
Another way is to enable AppInit_DLLs in the registry.

The offsets and method to discover them are discussed in the article at: https://gamehacking.academy/lesson/28
*/

#include <Windows.h>
#include <math.h>

// The atan2f function produces a radian. To convert it to degrees, we need the value of pi
#define M_PI 3.14159265358979323846
// The maximum amount of players in an Assault Cube
#define MAX_PLAYERS 32

// The player structure for every player in the game
struct Player {
	char unknown1[4];
	float x;
	float y;
	float z;
	char unknown2[0x30];
	float yaw;
	float pitch;
	char unknown3[0x1DD];
	char name[16];
};

// Our player
Player* player = NULL;

DWORD ret_address = 0x0040BE83;
DWORD text_address = 0x419880;

// Our temporary variables for our print text codecave
const char* text = "";
const char* empty_text = "";

DWORD x = 0;
DWORD y = 0;

// List of calculated ESP values
DWORD x_values[MAX_PLAYERS] = { 0 };
DWORD y_values[MAX_PLAYERS] = { 0 };
char* names[MAX_PLAYERS] = { NULL };

int* current_players;

// Our codecave responsible for printing text
__declspec(naked) void codecave() {
	current_players = (int*)(0x50F500);

	// First, recreate the original function we hooked but set the text to empty
	__asm {
		mov ecx, empty_text
		call text_address
		pushad
	}

	// Next, loop through all the current players in the game
	for (int i = 1; i < *current_players; i++) {
		// Store the calculated screen positions in temporary variables
		x = x_values[i];
		y = y_values[i];
		text = names[i];

		// Make sure our text is on screen
		if (x > 2400 || x < 0 || y < 0 || y > 1800) {
			text = "";
		}
		
		x += 200;

		// Invoke the print text function to display the text
		__asm {
			mov ecx, text
			push y
			push x
			call text_address
			add esp, 8
		}
	}

	// Restore the registers and jump back to the original code
	__asm {
		popad
		jmp ret_address
	}
}

// This thread contains all of the code for calculating our ESP screen positions
void injected_thread() {
	while (true) {
		// First, grab the current position and view angles of our player
		DWORD* player_offset = (DWORD*)(0x509B74);
		player = (Player*)(*player_offset);

		// Then, get the current number of players in the game
		current_players = (int*)(0x50F500);

		// Iterate through all active enemies
		for (int i = 1; i < *current_players; i++) {
			DWORD* enemy_list = (DWORD*)(0x50F4F8);
			DWORD* enemy_offset = (DWORD*)(*enemy_list + (i*4));
			Player* enemy = (Player*)(*enemy_offset);

			// Make sure the enemy is valid
			if (player != NULL && enemy != NULL) {
				// Calculate the absolute position of the enemy away from us to ensure that our future calculations are correct and based
				// around the origin
				float abspos_x = enemy->x - player->x;
				float abspos_y = enemy->y - player->y;
				float abspos_z = enemy->z - player->z;

				// Calculate the yaw
				float azimuth_xy = atan2f(abspos_y, abspos_x);
				// Convert to degrees
				float yaw = (float)(azimuth_xy * (180.0 / M_PI));
				// Add 90 since the game assumes direct north is 90 degrees
				yaw += 90;

				// Calculate the difference between our current yaw and the calculated yaw to the enemy
				float yaw_dif = player->yaw - yaw;

				// If we are near the 275 angle boundary, our yaw_dif will be too large, causing our text to appear incorrectly
				// To compensate for that, subtract the yaw_dif from 360 if it is over 180, since our viewport can never show 180 degrees
				if (yaw_dif > 180) {
					yaw_dif = yaw_dif - 360;
				}

				if (yaw_dif < -180) {
					yaw_dif = yaw_dif + 360;
				}

				// Calculate our X value by adding the yaw_dif times a scaling factor to the center of the screen horizontally (1200)
				x_values[i] = (DWORD)(1200 + (yaw_dif * -30));

				// Calculate the pitch
				// Since Z values are so limited, pick the larger between x and y to ensure that we 
				// don't look straight at the air when close to an enemy
				if (abspos_y < 0) {
					abspos_y *= -1;
				}
				if (abspos_y < 5) {
					if (abspos_x < 0) {
						abspos_x *= -1;
					}
					abspos_y = abspos_x;
				}
				float azimuth_z = atan2f(abspos_z, abspos_y);
				// Covert the value to degrees
				float pitch = (float)(azimuth_z * (180.0 / M_PI));
				
				// Same as above but for pitch
				float pitch_dif = player->pitch - pitch;

				// Calculate our Y value by adding the pitch_dif times a scaling factor to the center of the screen vertically (900)
				y_values[i] = (DWORD)(900 + ((pitch_dif) * 25));

				// Set the name to the enemy name
				names[i] = enemy->name;
			}
		}

		// So our thread doesn't constantly run, we have it pause execution for a millisecond.
		// This allows the processor to schedule other tasks.
		Sleep(1);
	}
}

// When our DLL is loaded, create a thread in the process that will handle the aimbot code
// Then, create a codecave for our print text function
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	DWORD old_protect;
	unsigned char* hook_location = (unsigned char*)0x0040BE7E;

	if (fdwReason == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)injected_thread, NULL, 0, NULL);

		VirtualProtect((void*)hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
		*hook_location = 0xE9;
		*(DWORD*)(hook_location + 1) = (DWORD)&codecave - ((DWORD)hook_location + 5);
	}

	return true;
}
