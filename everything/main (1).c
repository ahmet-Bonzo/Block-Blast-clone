#include "raylib.h"
#include <stdio.h>
//egedemo
#define MAX_COLORS 7
#define BLOCK_SIZE 60
#define GRID_WIDTH 8
#define GRID_HEIGHT 8
#define UI_HEIGHT 55
#define GRID_Y_OFFSET 150
#define MAGNETIC_RANGE 30
#define ANIMATION_DURATION 13

int score = 0;
int scoremult = 0;
int highscore = 0;


bool SaveFileData(const char *fileName,void *data, int dataSize);

unsigned char *LoadFileData(const char *fileName, int *dataSize);

void UnloadFileData(unsigned char *data);


typedef struct {
    int count;
    Vector2 offsets[9];  // max 9 hücreli blok
} BlockShape;


Sound clearSound;  // Global tanım — her yerden erişilebilir


typedef struct {
    bool occupied;
    Color color;
    bool clearing;
} GridCell;

typedef struct {
    Rectangle rects[9];
    int rectCount;
    Color color;
    bool dragging;
    bool active;
    Vector2 originalPos[6];
} Block;

typedef struct {
    bool active;
    int frame;
    bool rows[GRID_HEIGHT];
    bool cols[GRID_WIDTH];
} ClearAnimation;

// Yapılar
typedef struct {
    Rectangle rect;
    int hover;
} ColorRect;

typedef struct {
    Rectangle rectMinus;
    Rectangle rectPlus;
} VolumeControls;

// Ses seviyeleri
const int volumeLevels[3] = { 0,50 , 100 };

GridCell grid[GRID_HEIGHT][GRID_WIDTH] = { 0 };
ClearAnimation clearAnim = { 0 };
bool SaveFileData(const char* fileName, void* data, int dataSize);
unsigned char* LoadFileData(const char* fileName, int* dataSize);
void UnloadFileData(unsigned char* data);
float fabsfReplacement(float a);
Vector2 Vector2SubtractReplacement(Vector2 v1, Vector2 v2);
bool IsCellFilled(float x, float y);
void UpdateGrid(Block block);
bool IsRowFull(int row);
bool IsColumnFull(int col);
void StartClearAnimation();
void UpdateClearAnimation();
bool IsBlockPlaceable(Block b);
bool IsGameOver(Block blocks[3]);
Block GenerateRandomBlock(int screenWidth, int screenHeight, int index);
Rectangle retryButton = { 350, 450, 200, 60 };
bool gameOver = false;




int main(void)
{
    const int screenWidth = 900;
    const int screenHeight = 1000;

    const int volumeLevels[6] = { 0,2,25,50,75, 100 };
    int musicVolumeIndex = 5; // %100


    Music backgroundMusic;



    InitWindow(screenWidth, screenHeight, "Block Blast - Menu");
    InitAudioDevice();  // Ses sistemini başlat
    Sound placeSound = LoadSound("putsound.wav");
    clearSound = LoadSound("clearsound.wav");
    // Dosya adını seninkine göre değiştir
    backgroundMusic = LoadMusicStream("backmusic.wav");
    SetMusicVolume(backgroundMusic, volumeLevels[musicVolumeIndex] / 100.0f);
    PlayMusicStream(backgroundMusic);



    SetExitKey(0); // ESC pencereyi kapatmasın

    // Menüde gösterilecek açık renkler
    Color selectableColors[] = {
       GRAY, RED, BLUE, GREEN, BROWN, PURPLE, WHITE

    };

    bool showPauseMenu = false;
    // Sağ üstte ayar butonu
    Rectangle settingsButton = { GetScreenWidth() - 50, 10, 40, 40 };
    Rectangle retryButton = { GetScreenWidth() / 2 - 60,
        GetScreenHeight() / 2 - 20, 120, 40 };
    Rectangle menuButton = { GetScreenWidth() / 2 - 60,
        GetScreenHeight() / 2 + 30, 120, 40 };

    // Oyun içinde kullanılacak karşılık gelen dark renkler
    Color mappedDarkColors[] = {
        GRAY, RED, BLUE, GREEN, BROWN, PURPLE, WHITE
    };

    // Load textures
    Texture2D logo = LoadTexture("logo.png");
    Texture2D btnPlay = LoadTexture("play.png");
    Texture2D btnSettings = LoadTexture("settings.png");
    Texture2D btnExit = LoadTexture("exit.png");



    int centerX = screenWidth / 2;

    Vector2 logoPos = { centerX - logo.width / 2, 200 };

    int spacing = 40;

    Vector2 btnPlayPos = { centerX - btnPlay.width / 2,
        logoPos.y + logo.height + 300 };
    Vector2 btnSettingsPos = { centerX - btnSettings.width / 2,
        btnPlayPos.y + btnPlay.height + spacing };
    Vector2 btnExitPos = {
        centerX - btnExit.width / 2,
        btnSettingsPos.y + btnSettings.height + spacing };

    // Renk kutuları
    ColorRect colorRecs[MAX_COLORS];
    for (int i = 0; i < MAX_COLORS; i++) {
        colorRecs[i].rect = (Rectangle){
            50.0f + i * (100 + 10),
            400.0f,
            110.0f,
            110.0f };
        colorRecs[i].hover = 0;
    }

    // Ses ayar kutuları
    VolumeControls musicVolumeCtrl = {
        .rectMinus = { 150, 520, 40, 40 },
        .rectPlus = { 270, 520, 40, 40 }
    };
    VolumeControls sfxVolumeCtrl = {
        .rectMinus = { 150, 600, 40, 40 },
        .rectPlus = { 270, 600, 40, 40 }
    };

    int selectedColorIndex = 0;
    Color bgColor = mappedDarkColors[selectedColorIndex];


    bool showSettings = false;
    bool gamestart = false;

    SetTargetFPS(60);


oyun1:

    while (!WindowShouldClose())
    {

        UpdateMusicStream(backgroundMusic);

        Vector2 mousePos = GetMousePosition();

        bool btnPlayClicked = false;
        bool btnSettingsClicked = false;
        bool btnExitClicked = false;

        if (!showSettings)
        {
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Rectangle playRect = {
                    btnPlayPos.x,
                    btnPlayPos.y,
                    (float)btnPlay.width,
                    (float)btnPlay.height };
                Rectangle settingsRect = {
                    btnSettingsPos.x,
                    btnSettingsPos.y,
                    (float)btnSettings.width,
                    (float)btnSettings.height };
                Rectangle exitRect = {
                    btnExitPos.x,
                    btnExitPos.y,
                    (float)btnExit.width,
                    (float)btnExit.height };

                if (CheckCollisionPointRec
                (mousePos, playRect))
                    btnPlayClicked = true;
                else if (CheckCollisionPointRec
                (mousePos, settingsRect))
                    btnSettingsClicked = true;
                else if (CheckCollisionPointRec
                (mousePos, exitRect))
                    btnExitClicked = true;
            }
        }
        else
        {
            if (IsKeyPressed(KEY_ESCAPE))
            {
                showSettings = false;
            }

            for (int i = 0; i < MAX_COLORS; i++)
            {
                if (CheckCollisionPointRec(mousePos, colorRecs[i].rect))
                {
                    colorRecs[i].hover = 1;
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    {
                        selectedColorIndex = i;
                        bgColor = mappedDarkColors[i];
                        // Menüde açık renk gösteriliyor ama oyunda dark
                    }
                }
                else
                {
                    colorRecs[i].hover = 0;
                }
            }


        }

        // Butonlara basınca yapılacaklar
        if (btnPlayClicked) {
            printf("Oyun Başladı!\n");
            gamestart = true;
            PlayMusicStream(backgroundMusic);  // Müziği tekrar başlat
        }
        if (btnSettingsClicked) showSettings = true;
        if (btnExitClicked) break;

        // Çizim
        BeginDrawing();
        ClearBackground(bgColor);

        // Ana menü
        DrawTexture(logo, logoPos.x, logoPos.y, WHITE);
        DrawTexture(btnPlay, btnPlayPos.x, btnPlayPos.y, WHITE);
        DrawTexture(btnSettings, btnSettingsPos.x, btnSettingsPos.y, WHITE);
        DrawTexture(btnExit, btnExitPos.x, btnExitPos.y, WHITE);

        if (gamestart) {



            PlaySound(placeSound);

            unsigned char* data = LoadFileData("history.dat", &highscore);

            int highscore = *(int*)data;

            UnloadFileData(data);

            SetTargetFPS(60);
            int score = 0;
            Rectangle gridArea = {
                (screenWidth - GRID_WIDTH * BLOCK_SIZE) / 2.0f,
                GRID_Y_OFFSET,
                GRID_WIDTH * BLOCK_SIZE,
                GRID_HEIGHT * BLOCK_SIZE
            };

            Block blocks[3];
            for (int i = 0; i < 3; i++)
                blocks[i] = GenerateRandomBlock(screenWidth, screenHeight, i);

            Block* draggingBlock = 0;
            Vector2 mouseOffset = { 0 };

            while (!WindowShouldClose()) {
                UpdateMusicStream(backgroundMusic);

                BeginDrawing();
                ClearBackground(bgColor);
                DrawText(TextFormat("SCORE:%5d", score), 300, 50, 60, RED);
                DrawRectangle((screenWidth - GRID_WIDTH * BLOCK_SIZE) / 2,
                    GRID_Y_OFFSET,
                    GRID_WIDTH * BLOCK_SIZE,
                    GRID_HEIGHT * BLOCK_SIZE, WHITE);
                UpdateClearAnimation();

                if (!clearAnim.active && !gameOver) {
                    if (IsGameOver(blocks)) {
                        gameOver = true;



                        if (score > highscore) {
                            highscore = score;

                            SaveFileData("history.dat", &highscore, 4);
                        }

                        SaveFileData("history.dat", &highscore, 4);
                    }
                }
                Vector2 mouse = GetMousePosition();

                if (!gameOver) {

                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        for (int i = 0; i < 3; i++) {
                            if (!blocks[i].active) continue;
                            for (int j = 0; j < blocks[i].rectCount; j++) {
                                if (CheckCollisionPointRec(mouse,
                                    blocks[i].rects[j])) {
                                    draggingBlock = &blocks[i];
                                    draggingBlock->dragging = true;
                                    mouseOffset = Vector2SubtractReplacement
                                    (mouse,
                                        (Vector2) {
                                        blocks[i].rects[j].x,
                                            blocks[i].rects[j].y
                                    });
                                    for (int k = 0; k <
                                        draggingBlock->rectCount; k++)
                                        draggingBlock->originalPos[k] =
                                        (Vector2){
                                            draggingBlock->rects[k].x,
                                            draggingBlock->rects[k].y };
                                    break;
                                }
                            }
                        }
                    }

                    if (draggingBlock && draggingBlock->dragging) {
                        for (int j = 0; j < draggingBlock->rectCount; j++) {
                            Vector2 offset = Vector2SubtractReplacement
                            (draggingBlock->originalPos[j],
                                draggingBlock->originalPos[0]);
                            draggingBlock->rects[j].x = mouse.x -
                                mouseOffset.x + offset.x;
                            draggingBlock->rects[j].y = mouse.y -
                                mouseOffset.y + offset.y;
                        }
                    }

                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) &&
                        draggingBlock) {
                        draggingBlock->dragging = false;

                        Vector2 basePos = (Vector2){ draggingBlock->rects[0].x,
                            draggingBlock->rects[0].y };
                        int gridX = (int)((basePos.x - gridArea.x +
                            BLOCK_SIZE / 2) / BLOCK_SIZE) *
                            BLOCK_SIZE + gridArea.x;
                        int gridY = (int)((basePos.y - gridArea.y +
                            BLOCK_SIZE / 2) / BLOCK_SIZE) *
                            BLOCK_SIZE + gridArea.y;

                        bool valid = true;
                        for (int j = 0; j < draggingBlock->rectCount; j++) {
                            float x = gridX +
                                (draggingBlock->rects[j].x - basePos.x);
                            float y = gridY +
                                (draggingBlock->rects[j].y - basePos.y);
                            if (x < gridArea.x ||
                                x >= gridArea.x + gridArea.width ||
                                y < gridArea.y ||
                                y >= gridArea.y + gridArea.height ||
                                IsCellFilled(x, y)) {
                                valid = false;
                                break;
                            }
                        }

                        if (valid &&
                            fabsfReplacement(basePos.x - gridX) <
                            MAGNETIC_RANGE &&
                            fabsfReplacement(basePos.y - gridY) <
                            MAGNETIC_RANGE) {
                            Vector2 offset = { gridX - basePos.x,
                                gridY - basePos.y };
                            for (int j = 0; j < draggingBlock->rectCount; j++) {
                                draggingBlock->rects[j].x += offset.x;
                                draggingBlock->rects[j].y += offset.y;
                            }

                            UpdateGrid(*draggingBlock);
                            StartClearAnimation();



                            PlaySound(placeSound);

                            draggingBlock->active = false;

                            bool allInactive = true;
                            for (int i = 0; i < 3; i++)
                                if (blocks[i].active) allInactive = false;

                            if (allInactive)
                                for (int i = 0; i < 3; i++)
                                    blocks[i] =
                                    GenerateRandomBlock(screenWidth,
                                        screenHeight, i);
                        }
                        else {
                            for (int j = 0; j < draggingBlock->rectCount; j++)
                                draggingBlock->rects[j] =
                                (Rectangle){
                                    draggingBlock->originalPos[j].x,
                                    draggingBlock->originalPos[j].y,
                                    BLOCK_SIZE,
                                    BLOCK_SIZE };
                        }

                        draggingBlock = 0;


                    }
                }

                DrawRectangleLinesEx(gridArea, 2, WHITE);
                for (int i = 0; i <= GRID_WIDTH; i++)
                    DrawLineV((Vector2) {
                    gridArea.x + i * BLOCK_SIZE,
                        gridArea.y
                },
                        (Vector2) {
                    gridArea.x + i * BLOCK_SIZE,
                        gridArea.y + gridArea.height
                }, GRAY);
                for (int j = 0; j <= GRID_HEIGHT; j++)
                    DrawLineV((Vector2) {
                    gridArea.x,
                        gridArea.y + j * BLOCK_SIZE
                },
                        (Vector2) {
                    gridArea.x + gridArea.width,
                        gridArea.y + j * BLOCK_SIZE
                }, GRAY);

                for (int row = 0; row < GRID_HEIGHT; row++) {
                    for (int col = 0; col < GRID_WIDTH; col++) {
                        if (grid[row][col].occupied) {
                            Rectangle r = {
                                gridArea.x + col * BLOCK_SIZE,
                                gridArea.y + row * BLOCK_SIZE,
                                BLOCK_SIZE,
                                BLOCK_SIZE
                            };

                            if (scoremult < 2) {
                                score += scoremult * 10;
                            }
                            else {
                                score += scoremult * scoremult * 10;
                            }
                            scoremult = 0;




                            Color color = grid[row][col].color;
                            if (grid[row][col].clearing && clearAnim.active) {
                                float alpha = 1.0f -
                                    (float)clearAnim.frame /
                                    ANIMATION_DURATION;
                                color.a = (unsigned char)(255 * alpha);
                            }
                            DrawRectangleRounded(r,
                                0.2f,
                                10,
                                color);
                            DrawRectangleLinesEx(r,
                                2,
                                BLACK);
                        }
                    }
                }

                for (int i = 0; i < 3; i++) {
                    if (!blocks[i].active) continue;
                    for (int j = 0; j < blocks[i].rectCount; j++) {
                        DrawRectangleRounded(blocks[i].rects[j],
                            0.2f,
                            10,
                            blocks[i].color);
                        DrawRectangleLinesEx(blocks[i].rects[j],
                            2,
                            BLACK);
                    }
                }

                DrawRectangle(0,
                    screenHeight - UI_HEIGHT,
                    screenWidth,
                    UI_HEIGHT,
                    DARKGRAY);
                DrawText("Drag the blocks and blast them!",
                    20,
                    screenHeight - UI_HEIGHT + 20,
                    20,
                    WHITE);

                if (score > highscore) {
                    highscore = score;
                }
                int fontSize = 30;
                int x = 50;
                int y = 50;

                const char* label = "HIGH SCORE";
                int scoreTextWidth = MeasureText(TextFormat("%d", highscore),
                    fontSize);
                int labelTextWidth = MeasureText(label,
                    fontSize);

                // Genişliğe göre ortalamak için 
                // label'ın ortasına göre sayıyı hizalayalım
                int maxTextWidth = (labelTextWidth > scoreTextWidth) ?
                    labelTextWidth : scoreTextWidth;

                DrawText(label, x, y, fontSize, BLACK);
                DrawText(TextFormat("%d", highscore),
                    x + (labelTextWidth - scoreTextWidth) / 2,
                    y + fontSize + 10,
                    fontSize,
                    BLACK);
                if (gameOver) {
                    // Arka plan karartması
                    DrawRectangle(0,
                        0,
                        screenWidth,
                        screenHeight,
                        Fade(BLACK, 0.7f));

                    // GAME OVER yazısı
                    const char* overText = "GAME OVER!";
                    int fontSize = 60;
                    int textWidth = MeasureText(overText,
                        fontSize);
                    DrawText(overText,
                        screenWidth / 2 - textWidth / 2,
                        200,
                        fontSize,
                        RED);

                    // Skor bilgisi
                    DrawText(TextFormat("Score: %d", score),
                        screenWidth / 2 - 100,
                        280,
                        30,
                        WHITE);
                    DrawText(TextFormat("High Score: %d", highscore),
                        screenWidth / 2 - 100,
                        320,
                        30,
                        WHITE);

                    // Retry butonu
                    Rectangle retryButton = { screenWidth / 2 - 100,
                        400,
                        200,
                        50 };
                    DrawRectangleRec(retryButton,
                        LIGHTGRAY);
                    DrawText("Retry",
                        retryButton.x + 65,
                        retryButton.y + 10,
                        30,
                        BLACK);

                    // Menüye Dön butonu
                    Rectangle menuReturnButton = { screenWidth / 2 - 100,
                                                     470,
                                                     200,
                                                     50 };
                    DrawRectangleRec(menuReturnButton,
                        DARKGRAY);
                    DrawText("Menu",
                        menuReturnButton.x + 35,
                        menuReturnButton.y + 10,
                        30,
                        WHITE);

                    // Tıklama kontrolleri
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        Vector2 mouse = GetMousePosition();

                        if (CheckCollisionPointRec(mouse, retryButton)) {
                            score = 0;
                            unsigned char* data = LoadFileData("history.dat", &
                                highscore);
                            highscore = *(int*)data;
                            UnloadFileData(data);

                            for (int i = 0; i < GRID_HEIGHT; i++)
                                for (int j = 0; j < GRID_WIDTH; j++)
                                    grid[i][j] = (GridCell){
                                                     false,
                                                     GRAY,
                                                     false };

                            for (int i = 0; i < 3; i++)
                                blocks[i] = GenerateRandomBlock(screenWidth,
                                    screenHeight,
                                    i);

                            gameOver = false;
                            clearAnim.active = false;
                        }
                        else if (CheckCollisionPointRec(mouse, menuReturnButton)
                            && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            gamestart = false;
                            gameOver = false;

                            for (int i = 0; i < GRID_HEIGHT; i++)
                                for (int j = 0; j < GRID_WIDTH; j++)
                                    grid[i][j] = (GridCell){ false,
                                                     GRAY,
                                                     false };

                            goto oyun1;

                        }
                    }
                }
                EndDrawing();
            }
            return 0;
        }

        if (showSettings)
        {
            PlaySound(placeSound);

            int baseY = 400; // kutuların dikey başlangıç noktası
            int boxSize = 100;
            int spacing = 10;
            int boxX = 40;
            int boxY = 300;
            int boxW = screenWidth - 80;
            int boxH = 400;




            // Soft görünüm
            DrawRectangle(0,
                0,
                screenWidth,
                screenHeight,
                Fade(BLACK, 0.5f));
            DrawTexture(logo,
                logoPos.x,
                logoPos.y,
                Fade(WHITE, 0.2f));



            DrawRectangle(boxX,
                boxY,
                boxW,
                boxH,
                DARKGRAY);
            DrawRectangleLines(boxX,
                boxY,
                boxW,
                boxH,
                WHITE);

            DrawText("Settings",
                boxX + 20,
                boxY + 20,
                30,
                WHITE);
            DrawText("Collors Palette:",
                boxX + 20,
                boxY + 70,
                20,
                WHITE);


            for (int i = 0; i < MAX_COLORS; i++) {
                Rectangle colorBox = { 50 + i * (boxSize + spacing),
                    baseY,
                    boxSize,
                    boxSize - 10 };
                DrawRectangleRec(colorBox, selectableColors[i]);

                if (CheckCollisionPointRec(GetMousePosition(), colorBox)) {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        selectedColorIndex = i;
                    }
                }

                if (i == selectedColorIndex) {
                    DrawRectangleLinesEx(colorBox, 4, BLACK);
                }
            }

            // Başta (Update veya Draw'dan önce bir yerde tanımla):
            Rectangle resetScoreButtonRect = { boxX + 20, boxY + 300, 200, 40 }; // konum ve boyut

            // Draw kısmı:
            Color btnColor = CheckCollisionPointRec(GetMousePosition(), resetScoreButtonRect) ? LIGHTGRAY : GRAY;
            DrawRectangleRec(resetScoreButtonRect, btnColor);
            DrawText("Reset High Score", resetScoreButtonRect.x + 10, resetScoreButtonRect.y + 10, 20, BLACK);

            // Tıklama kontrolü:
            if (CheckCollisionPointRec(GetMousePosition(), resetScoreButtonRect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int score = 0;
                int* scorePtr = &score;

                SaveFileData("history.dat", &score, 4);

                unsigned char* data = LoadFileData("history.dat", &score);  

                int value = *(int*)data;

                UnloadFileData(data);

                TraceLog(LOG_INFO, "Loaded value: %d", value);
            
            }

          
            



            DrawText("Music Volume:", boxX + 20, boxY + 200, 20, WHITE);

            // Müzik sesi azalt
            if (CheckCollisionPointRec(mousePos, musicVolumeCtrl.rectMinus) &&
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (musicVolumeIndex > 0) {
                    musicVolumeIndex--;
                    SetMusicVolume(backgroundMusic,
                        volumeLevels[musicVolumeIndex] / 100.0f);
                }
            }

            // Müzik sesi artır
            if (CheckCollisionPointRec(mousePos,
                musicVolumeCtrl.rectPlus) &&
                IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (musicVolumeIndex < 5) {
                    musicVolumeIndex++;
                    SetMusicVolume(backgroundMusic,
                        volumeLevels[musicVolumeIndex] / 100.0f);
                }
            }


            char musicVolText[20];
            sprintf(musicVolText, "%d%%", volumeLevels[musicVolumeIndex]);
            DrawText(musicVolText, boxX + 260, boxY + 200, 20, WHITE);

            DrawRectangleRec(musicVolumeCtrl.rectMinus, GRAY);
            DrawText("-",
                musicVolumeCtrl.rectMinus.x + 12,
                musicVolumeCtrl.rectMinus.y + 4,
                30,
                BLACK);
            DrawRectangleRec(musicVolumeCtrl.rectPlus, GRAY);
            DrawText("+",
                musicVolumeCtrl.rectPlus.x + 12,
                musicVolumeCtrl.rectPlus.y + 4,
                30,
                BLACK);


            // BACK butonu (sağ üst köşe)
            float backBtnWidth = 90;
            float backBtnHeight = 40;
            Rectangle backBtn = { screenWidth - backBtnWidth - 20,
                20,
                backBtnWidth,
                backBtnHeight };

            bool hoveringBack = CheckCollisionPointRec(GetMousePosition(), backBtn);
            Color backColor = hoveringBack ? Fade(GRAY, 0.8f) : DARKGRAY;
            int borderThickness = hoveringBack ? 3 : 2;
            int fontSize = hoveringBack ? 22 : 20;

            if (hoveringBack && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                showSettings = false;
            }

            DrawRectangleRec(backBtn, backColor);
            DrawRectangleLinesEx(backBtn, (float)borderThickness, WHITE);

            // "Back" yazısını ortala
            const char* backText = "Back";
            int textWidth = MeasureText(backText, fontSize);
            DrawText(backText,
                backBtn.x + (backBtn.width - textWidth) / 2,
                backBtn.y + (backBtn.height - fontSize) / 2,
                fontSize,
                WHITE);
        }

        EndDrawing();
    }

    UnloadTexture(logo);
    UnloadTexture(btnPlay);
    UnloadTexture(btnSettings);
    UnloadTexture(btnExit);

    // Temizlik – Ses dosyalarını boşalt
    UnloadSound(placeSound);
    UnloadSound(clearSound);
    // Ses sistemini kapat
    UnloadMusicStream(backgroundMusic);




    CloseAudioDevice();

    // Pencereyi kapat
    CloseWindow();

}

float fabsfReplacement(float a) {
    return (a < 0) ? -a : a;
}
Vector2 Vector2SubtractReplacement(Vector2 v1, Vector2 v2) {
    return (Vector2) { v1.x - v2.x, v1.y - v2.y };
}

bool IsCellFilled(float x, float y) {
    int col = (int)((x - ((800 - GRID_WIDTH * BLOCK_SIZE) / 2)) / BLOCK_SIZE);
    int row = (int)((y - GRID_Y_OFFSET) / BLOCK_SIZE);

    return (row >= 0 && row < GRID_HEIGHT && col >= 0 && col < GRID_WIDTH) ?
        grid[row][col].occupied : true;
}
void UpdateGrid(Block block) {
    for (int i = 0; i < block.rectCount; i++) {
        int col = (int)((block.rects[i].x -
            ((800 - GRID_WIDTH * BLOCK_SIZE) / 2)) / BLOCK_SIZE);
        int row = (int)((block.rects[i].y - GRID_Y_OFFSET) / BLOCK_SIZE);
        if (row >= 0 && row < GRID_HEIGHT && col >= 0 && col < GRID_WIDTH) {
            grid[row][col].occupied = true;
            grid[row][col].color = block.color;
            grid[row][col].clearing = false;

        }
    }
}
bool IsRowFull(int row) {
    for (int col = 0; col < GRID_WIDTH; col++)
        if (!grid[row][col].occupied) return false;
    scoremult = scoremult + 1;

    return true;
}

bool IsColumnFull(int col) {
    for (int row = 0; row < GRID_HEIGHT; row++)
        if (!grid[row][col].occupied) return false;
    scoremult = scoremult + 1;
    return true;
}
void StartClearAnimation() {
    clearAnim.active = true;
    clearAnim.frame = 0;

    for (int i = 0; i < GRID_HEIGHT; i++)
        clearAnim.rows[i] = IsRowFull(i);

    for (int i = 0; i < GRID_WIDTH; i++)
        clearAnim.cols[i] = IsColumnFull(i);

    for (int row = 0; row < GRID_HEIGHT; row++) {
        for (int col = 0; col < GRID_WIDTH; col++) {
            if (clearAnim.rows[row] || clearAnim.cols[col])
                grid[row][col].clearing = true;
        }
    }
}

void UpdateClearAnimation() {
    if (clearAnim.active) {
        clearAnim.frame++;
        if (clearAnim.frame >= ANIMATION_DURATION) {

            for (int row = 0; row < GRID_HEIGHT; row++) {
                for (int col = 0; col < GRID_WIDTH; col++) {
                    if (grid[row][col].clearing) {
                        grid[row][col].occupied = false;
                        grid[row][col].clearing = false;
                        PlaySound(clearSound);
                    }
                }
            }
            clearAnim.active = false;
        }
    }
}
bool IsBlockPlaceable(Block b) {
    for (int row = 0; row <= GRID_HEIGHT; row++) {
        for (int col = 0; col <= GRID_WIDTH; col++) {
            bool valid = true;
            for (int i = 0; i < b.rectCount; i++) {
                float dx = b.rects[i].x - b.rects[0].x;
                float dy = b.rects[i].y - b.rects[0].y;
                float x = ((800 - GRID_WIDTH * BLOCK_SIZE) / 2) +
                    col * BLOCK_SIZE + dx;
                float y = GRID_Y_OFFSET + row * BLOCK_SIZE + dy;
                if (IsCellFilled(x, y)) {
                    valid = false;
                    break;
                }
            }
            if (valid) return true;
        }
    }
    return false;
}
bool IsGameOver(Block blocks[3]) {
    for (int i = 0; i < 3; i++)
        if (blocks[i].active && IsBlockPlaceable(blocks[i]))
            return false;
    return true;
}

BlockShape blockShapes[18] = {
    // 0: 2x2 kare
    {4, { {0,0}, {1,0}, {0,1}, {1,1} }},
    // 1: 3x3 orta 4 blok
    {4, { {0,0}, {1,0}, {0,1}, {1,1} }},
    // 2: L └
    {4, { {0,0}, {0,1}, {0,2}, {1,2} }},
    // 3: L ┌
    {4, { {0,0}, {0,1}, {0,2}, {-1,2} }},
    // 4: L ┐
    {4, { {0,0}, {1,0}, {2,0}, {2,1} }},
    // 5: L ┘
    {4, { {2,0}, {2,1}, {0,1}, {1,1} }},
    // 6: L + sağ orta
    {4, { {0,0}, {0,1}, {0,2}, {1,1} }},
    // 7: Ters L + sağ orta
    {4, { {1,0}, {1,1}, {1,2}, {0,1} }},
    // 8: Yatay L + alt orta
    {4, { {0,1}, {1,1}, {2,1}, {1,0} }},
    // 9: Ters yatay L + alt orta
    {4, { {2,1}, {1,1}, {0,1}, {1,0} }},
    // 10: Z
    {4, { {0,0}, {1,0}, {1,1}, {2,1} }},
    // 11: Ters Z
    {4, { {1,0}, {2,0}, {0,1}, {1,1} }},
    // 12: 2x1 yatay
    {2, { {0,0}, {1,0} }},
    // 13: 3x1 yatay
    {3, { {0,0}, {1,0}, {2,0} }},
    // 14: 4x1 yatay
    {4, { {0,0}, {1,0}, {2,0}, {3,0} }},
    // 15: 2x1 dikey
    {2, { {0,0}, {0,1} }},
    // 16: 3x1 dikey
    {3, { {0,0}, {0,1}, {0,2} }},
    // 17: 4x1 dikey
    {4, { {0,0}, {0,1}, {0,2}, {0,3} }},
    
};


Block GenerateRandomBlock(int screenWidth, int screenHeight, int index) {
    Block block = { 0 };
    block.active = true;

    // Renk ataması
    Color colors[] = { RED, GREEN, BLUE, ORANGE, PURPLE, YELLOW };
    int colorCount = sizeof(colors) / sizeof(colors[0]);
    block.color = colors[GetRandomValue(0, colorCount - 1)];

    // Başlangıç pozisyonu
    float startX = 180 + index * 260;
    float startY = 760;

    // Rastgele şekil seç
    int shapeType = GetRandomValue(0, 17);
    BlockShape shape = blockShapes[shapeType];

    // Şekli oluştur
    for (int i = 0; i < shape.count; i++) {
        block.rects[i] = (Rectangle){
            startX + shape.offsets[i].x * BLOCK_SIZE,
            startY + shape.offsets[i].y * BLOCK_SIZE,
            BLOCK_SIZE, BLOCK_SIZE
        };
        block.originalPos[i] = (Vector2){
            block.rects[i].x, block.rects[i].y
        };
    }

    block.rectCount = shape.count;
    return block;
}

