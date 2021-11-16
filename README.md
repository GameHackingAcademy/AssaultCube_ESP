# Assault Cube ESP
Referenced in https://gamehacking.academy/lesson/5/9.

An ESP for Assault Cube 1.2.0.2 that displays information about enemy players above their heads.

It works by iterating over the enemy list and calculating the yaw and pitch required to aim at that enemy using arctangents. This part of the code is taken from the aimbot code.

The difference between the calculated yaw and pitch and our player's yaw and pitch is then used to derive the screen coordinates of the enemy. This is done by adding the difference multiplied by a scaling factor to the middle of the screen.

This must be injected into the Assault Cube process to work. One way to do this is to use a DLL injector. Another way is to enable AppInit_DLLs in the registry.
