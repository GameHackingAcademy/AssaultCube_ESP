#include <Windows.h>
#include <math.h>

#define M_PI 3.14159265358979323846
#define MAX_PLAYERS 32

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

Player* player = NULL;

DWORD ret_address = 0x0040BE83;
DWORD text_address = 0x419880;

const char* text = "";
const char* empty_text = "";

DWORD x = 0;
DWORD y = 0;

DWORD x_values[MAX_PLAYERS] = { 0 };
DWORD y_values[MAX_PLAYERS] = { 0 };
char* names[MAX_PLAYERS] = { NULL };

int* current_players;

__declspec(naked) void codecave() {
	current_players = (int*)(0x50F500);

	__asm {
		mov ecx, empty_text
		call text_address
		pushad
	}

	for (int i = 1; i < *current_players; i++) {
		x = x_values[i];
		y = y_values[i];
		text = names[i];

		if (x > 2400 || x < 0 || y < 0 || y > 1800) {
			text = "";
		}

		__asm {
			mov ecx, text
			push y
			push x
			call text_address
			add esp, 8
		}
	}

	__asm {
		popad
		jmp ret_address
	}
}

void injected_thread() {
	while (true) {
		DWORD* player_offset = (DWORD*)(0x509B74);
		player = (Player*)(*player_offset);

		current_players = (int*)(0x50F500);

		for (int i = 1; i < *current_players; i++) {
			DWORD* enemy_list = (DWORD*)(0x50F4F8);
			DWORD* enemy_offset = (DWORD*)(*enemy_list + (i*4));
			Player* enemy = (Player*)(*enemy_offset);

			if (player != NULL && enemy != NULL) {
				float abspos_x = enemy->x - player->x;
				float abspos_y = enemy->y - player->y;
				float abspos_z = enemy->z - player->z;

				float azimuth_xy = atan2f(abspos_y, abspos_x);
				float yaw = (float)(azimuth_xy * (180.0 / M_PI));
				yaw += 90;

				float yaw_dif = player->yaw - yaw;

				if (yaw_dif > 180) {
					yaw_dif = yaw_dif - 360;
				}

				if (yaw_dif < -180) {
					yaw_dif = yaw_dif + 360;
				}

				x_values[i] = (DWORD)(1200 + (yaw_dif * -30));

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
				float pitch = (float)(azimuth_z * (180.0 / M_PI));
				float pitch_dif = player->pitch - pitch;

				DWORD center_y = 1800 / 2;
				y_values[i] = (DWORD)(center_y + ((pitch_dif) * 25));

				names[i] = enemy->name;
			}
		}

		Sleep(1);
	}
}

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
