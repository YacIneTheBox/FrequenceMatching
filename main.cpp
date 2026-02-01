#include "raylib.h"

int main(){
    InitWindow(800,600,"Frequency Matching");
    InitAudioDevice();
    Music guitarTrack = LoadMusicStream("guitar.mp3");
    Music drumsTrack = LoadMusicStream("drums.mp3");

    PlayMusicStream(guitarTrack);
    PlayMusicStream(drumsTrack);

    float volume = 1.0f;
    float pitch = 1.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()){

        UpdateMusicStream(guitarTrack);
        UpdateMusicStream(drumsTrack);

        // Contrôles du joueur (Test pour voir si ça marche)
        if (IsKeyDown(KEY_UP)) volume += 0.01f;
        if (IsKeyDown(KEY_DOWN)) volume -= 0.01f;
        if (IsKeyDown(KEY_RIGHT)) pitch += 0.01f; // Accélère
        if (IsKeyDown(KEY_LEFT)) pitch -= 0.01f; // Ralentit

        SetMusicVolume(guitarTrack, volume);
        SetMusicPitch(guitarTrack, pitch);
        SetMusicPitch(drumsTrack, pitch);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Utilise les FLECHES pour Pitch/Volume", 10, 10, 20, DARKGRAY);
            DrawText(TextFormat("Volume: %.2f", volume), 10, 40, 20, BLUE);
            DrawText(TextFormat("Pitch: %.2f", pitch), 10, 70, 20, RED);
        EndDrawing();
    }

    // 3. Nettoyage (Très important en C)
        UnloadMusicStream(guitarTrack);
        UnloadMusicStream(drumsTrack);
        CloseAudioDevice(); // Ferme la carte son
        CloseWindow();

        return 0;
}
