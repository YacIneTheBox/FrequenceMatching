#include "raylib.h"
#include <math.h>
#include <string.h>

#define FFT_SIZE 512
#define SAMPLES_COUNT 512

float guitarFftData[FFT_SIZE] = {0.0f};
float drumsFftData[FFT_SIZE] = {0.0f};

// --- DEBUT DE LA MAGIE MATHEMATIQUE ---
// Cette fonction transforme l'onde (time) en fréquences (frequency)
// in_raw = les données audio brutes
// out_fft = le tableau où on stockera la puissance des fréquences
void ComputeFFT(float *in_raw, float *out_fft)
{
    float real[FFT_SIZE];
    float imag[FFT_SIZE];

    // Initialisation
    for (int i = 0; i < FFT_SIZE; i++) {
        real[i] = in_raw[i];
        imag[i] = 0;
    }

    // Algorithme FFT simplifié (Danielson-Lanczos)
    // C'est complexe, mais ça trie les fréquences de 0Hz (Basses) à 22000Hz (Aigus)
    int i, j, k, n, m, mmax, step;
    float tempr, tempi, theta, wpr, wpi, wr, wi;

    // Bit-reversal permutation
    j = 0;
    for (i = 0; i < FFT_SIZE - 1; i++) {
        if (i < j) {
            tempr = real[j]; real[j] = real[i]; real[i] = tempr;
            tempr = imag[j]; imag[j] = imag[i]; imag[i] = tempr;
        }
        k = FFT_SIZE / 2;
        while (k <= j) { j -= k; k /= 2; }
        j += k;
    }

    // Danielson-Lanczos section
    mmax = 1;
    while (n = mmax * 2, n <= FFT_SIZE) {
        theta = -3.14159265f * 2.0f / n; // -2 * PI / n
        wpr = cosf(theta);
        wpi = sinf(theta);
        wr = 1.0f;
        wi = 0.0f;
        for (m = 0; m < mmax; m++) {
            for (i = m; i < FFT_SIZE; i += n) {
                j = i + mmax;
                tempr = wr * real[j] - wi * imag[j];
                tempi = wr * imag[j] + wi * real[j];
                real[j] = real[i] - tempr;
                imag[j] = imag[i] - tempi;
                real[i] += tempr;
                imag[i] += tempi;
            }
            tempr = wr;
            wr = wr * wpr - wi * wpi;
            wi = wi * wpr + tempr * wpi;
        }
        mmax = n;
    }

    // Calcul de la magnitude (Puissance) pour visualiser
    // On ne garde que la première moitié (car le spectre est miroir)
    for (int i = 0; i < FFT_SIZE / 2; i++) {
        out_fft[i] = sqrtf(real[i] * real[i] + imag[i] * imag[i]);
    }
}

void ProcessAudioData(void *bufferData,unsigned int frames,float *destinationFFT){
    float tempWave[SAMPLES_COUNT] = {0.0f};
    memset(tempWave, 0, sizeof(tempWave));
    float *samples = (float *)bufferData;

    for (unsigned int i = 0;i<frames;i++){
        if (i<SAMPLES_COUNT){
            tempWave[i] = samples[i];
        }
    }
    ComputeFFT(tempWave, destinationFFT);
}

void GuitarCallBack(void *bufferData,unsigned int frames){
    ProcessAudioData(bufferData,frames,guitarFftData);
}

void DrumsCallBack(void *bufferData,unsigned int frames){
    ProcessAudioData(bufferData,frames,drumsFftData);
}

int main(){
    InitWindow(800,600,"Frequency Matching");
    InitAudioDevice();
    Music guitarTrack = LoadMusicStream("guitar.mp3");
    Music drumsTrack = LoadMusicStream("drums.mp3");

    PlayMusicStream(guitarTrack);
    PlayMusicStream(drumsTrack);

    AttachAudioStreamProcessor(drumsTrack.stream, DrumsCallBack);
    AttachAudioStreamProcessor(guitarTrack.stream, GuitarCallBack);

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

            // Dessin du Spectre (Fréquences)
            // Note : On ne boucle que sur la MOITIE du tableau (i < FFT_SIZE/2)
            for (int i = 0; i < FFT_SIZE / 2; i++)
            {
                // On étale les points sur la largeur de l'écran
                // i * 800 / 256 pour couvrir tout l'écran
                int x = i * (800 / (FFT_SIZE/2));

                // La hauteur de la barre (Magnitude)
                // On multiplie par 400 ou plus car les valeurs brutes sont souvent petites
                float height = guitarFftData[i] * 400 * volume;

                // On dessine des rectangles (Barres) plutôt que des lignes, c'est plus lisible pour les fréquences
                // x, y, width, height
                // On dessine à "300 - height" pour que ça monte vers le haut
                DrawRectangle(x, 300 - height, (800 / (FFT_SIZE/2)) - 1, height, MAROON);

            }

            for (int i = 0; i < FFT_SIZE / 2; i++)
            {
                // On étale les points sur la largeur de l'écran
                // i * 800 / 256 pour couvrir tout l'écran
                int x = i * (800 / (FFT_SIZE/2));

                // La hauteur de la barre (Magnitude)
                // On multiplie par 400 ou plus car les valeurs brutes sont souvent petites
                float height = drumsFftData[i] * 400 * volume;

                // Effet miroir (optionnel) : on dessine aussi vers le bas pour faire joli
                DrawRectangle(x, 300, (800 / (FFT_SIZE/2)) - 1, height, Fade(BLUE, 0.5f));
            }
        EndDrawing();
    }

    // 3. Nettoyage (Très important en C)
        UnloadMusicStream(guitarTrack);
        UnloadMusicStream(drumsTrack);
        CloseAudioDevice(); // Ferme la carte son
        CloseWindow();

        return 0;
}
