#include "raylib.h"
#include "raymath.h"
#include <math.h>
#include <string.h>

#define FFT_SIZE 8192
#define SAMPLES_COUNT 8192

#define VISUAL_BARS_COUNT 32 // On veut seulement 16 grosses barres pour le jeu (plus lisible)

// Ce tableau stockera la hauteur LISSÉE de nos 16 barres
float smoothGuitar[VISUAL_BARS_COUNT] = {0.0f};
float smoothDrums[VISUAL_BARS_COUNT] = {0.0f};

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

// Cette fonction transforme la FFT brute en barres visuelles agréables
void AnalyzeAndSmooth(float* fftData, float* smoothBuffer, float dt)
{
    int sampleRate = 44100;
    float minHz = 40.0f;
    float maxHz = 12000.0f;

    // On utilise une échelle logarithmique pour découper les fréquences
    // Cela permet d'avoir autant de barres pour les basses que pour les aigus
    float logMin = log10f(minHz);
    float logMax = log10f(maxHz);

    for (int i = 0; i < VISUAL_BARS_COUNT; i++)
    {
        // 1. Calculer quelle portion du tableau FFT correspond à cette barre visuelle
        // On découpe l'échelle log en tranches égales
        float tCurrent = (float)i / VISUAL_BARS_COUNT;
        float tNext = (float)(i + 1) / VISUAL_BARS_COUNT;

        // Conversion Log -> Fréquence (Hz)
        float freqStart = powf(10.0f, logMin + tCurrent * (logMax - logMin));
        float freqEnd = powf(10.0f, logMin + tNext * (logMax - logMin));

        // Conversion Fréquence -> Index tableau FFT
        int binStart = (int)(freqStart * FFT_SIZE / sampleRate);
        int binEnd = (int)(freqEnd * FFT_SIZE / sampleRate);
        if (binEnd <= binStart) binEnd = binStart + 1; // Sécurité
        if (binEnd > FFT_SIZE/2) binEnd = FFT_SIZE/2;

        // 2. Moyenne de l'énergie dans cette tranche
        float averageEnergy = 0.0f;
        for (int k = binStart; k < binEnd; k++) {
            averageEnergy += fftData[k];
        }
        averageEnergy /= (binEnd - binStart); // On normalise

        // Boost : On amplifie un peu les aigus qui sont souvent faibles
        float boost = 1.0f + ((float)i * 0.5f);
        float targetHeight = averageEnergy * 1000.0f * boost; // Ajuste le 1000.0f selon ton volume

        // 3. LISSAGE (Smoothing)
        // Si la nouvelle valeur est plus haute, on monte vite (attaque)
        // Si elle est plus basse, on descend doucement (decay) -> donne un effet "organique"
        if (targetHeight > smoothBuffer[i]) {
            smoothBuffer[i] = Lerp(smoothBuffer[i], targetHeight, 20.0f * dt); // Montée rapide
        } else {
            smoothBuffer[i] = Lerp(smoothBuffer[i], targetHeight, 5.0f * dt);  // Descente lente
        }
    }
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
    Music guitarTrack = LoadMusicStream("piano.mp3");
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

        // Calculer les visuels (on passe GetFrameTime() pour que la vitesse de lissage soit constante)
        AnalyzeAndSmooth(guitarFftData, smoothGuitar, GetFrameTime());
        AnalyzeAndSmooth(drumsFftData, smoothDrums, GetFrameTime());

        // Contrôles du joueur (Test pour voir si ça marche)
        if (IsKeyDown(KEY_UP)) volume += 0.01f;
        if (IsKeyDown(KEY_DOWN)) volume -= 0.01f;
        if (IsKeyDown(KEY_RIGHT)) pitch += 0.01f; // Accélère
        if (IsKeyDown(KEY_LEFT)) pitch -= 0.01f; // Ralentit

        SetMusicVolume(guitarTrack, volume);
        SetMusicVolume(drumsTrack, volume);

        SetMusicPitch(guitarTrack, pitch);
        SetMusicPitch(drumsTrack, pitch);

        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Utilise les FLECHES pour Pitch/Volume", 10, 10, 20, DARKGRAY);
            DrawText(TextFormat("Volume: %.2f", volume), 10, 40, 20, BLUE);
            DrawText(TextFormat("Pitch: %.2f", pitch), 10, 70, 20, RED);

            float barWidth = 800.0f / VISUAL_BARS_COUNT;

            for (int i = 0; i < VISUAL_BARS_COUNT; i++)
            {
                // --- GUITARE (Rouge, vers le haut) ---
                float hGuitar =(smoothGuitar[i] * volume ) / 100;

                // On dessine avec un petit espace entre les barres (-2)
                DrawRectangle(i * barWidth, 300 - hGuitar, barWidth - 2, hGuitar, MAROON);

                // Optionnel : Un petit contour pour le style
                DrawRectangleLines(i * barWidth, 300 - hGuitar, barWidth - 2, hGuitar, RED);


                // --- BATTERIE (Bleu, vers le bas) ---
                float hDrums = (smoothDrums[i] * volume) / 100;

                DrawRectangle(i * barWidth, 300, barWidth - 2, hDrums, Fade(BLUE, 0.8f));
                DrawRectangleLines(i * barWidth, 300, barWidth - 2, hDrums, DARKBLUE);
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
