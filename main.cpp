#include <stdio.h>
#include <string>
#include <time.h>
#include <conio.h>
#include <windows.h>
#include <math.h>
#include <vector>

#include <cwchar>                                       
#include <codecvt>
#include <wchar.h> 
#pragma comment(linker, "/STACK:8388608")

/*
* ANSI Escape Sequences 参考 =>  https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
* Hexadecimal を使う
* conio.h キーボード
* 
*/

using namespace std;

#pragma region "UI.Default"
const int CONSOLE_WIDTH = 120;
const int CONSOLE_HEIGHT = 30;
const bool EMPTY_CHAR_SW = false;
const int MAX_EVENT_HANDLERS = 15;
const string AUTHOR = "GC1B_10_ラ_ケツブン";
#pragma endregion
#pragma region "Game.Default"
const string BP = "X";  // ■ or X
const string WP = "O";  // □ or O
const int MAX_SIZE = 12; // 最大サイズ
#pragma endregion
#pragma region "AI.Default"
const int AN = 10;
const int DEPTH = 6;
#pragma endregion

struct Vector2 {
    int x;
    int y;
};


enum CharWord {
    SPACE = ' ',
    NEWLINE = '\n',
    NULL_CHAR = '\0',
    EMPTY_CHAR = '?',
};

enum Piece {
    NONE_PIECE, // 空白
    WHITE_PIECE, // 
    BLACK_PIECE, //  
    INVALID_PIECE, // 無効なピース
    PIECE_TYPE_COUNT,
};

enum PlayerType {
    AI,
    HUMAN,
};

enum PlayerRole {
    MASTER, // マスター プレイヤー
    SLAVE, // スレーブ プレイヤー
    NONE, // プレイヤーなし
};

enum AsciiCode {
    W_KEY = 119,      // wキー
    S_KEY = 115,      // sキー
    A_KEY = 97,      // aキー
    D_KEY = 100,      // dキー
    R_KEY = 114,       // rキー
    J_KEY = 106,      // jキー
    U_KEY = 117,      // uキー
    I_KEY = 105,      // iキー
    K_KEY = 107,      // kキー
    UP_ARROW = 72,      // 上矢印キー
    DOWN_ARROW = 80,    // 下矢印キー
    LEFT_ARROW = 75,    // 左矢印キー
    RIGHT_ARROW = 77,   // 右矢印キー
    SPACE_BAR = 32,     // スペースキー
    ESCAPE_KEY = 27,    // エスケープキー
    ENTER_KEY = 13,     // Enterキー
};

// = =||| 不気味な
enum Chessboard {
    SMALL, // 3x3
    MEDIUM, // 7x7
    LARGE, // 12x12
};

typedef enum EventType {
    // --- システム関連イベント ---
    SYS_SCENE_CHANGE,

    // --- ゲームロジック関連イベント ---
    GAME_START,
    GAME_OVER,
    AI_RUN,

    // --- UI・演出関連イベント ---
    UI_UPDATE,

    // --- その他のイベント ---
    // イベントの数
    EVENT_COUNT,
} EventType;


#pragma region "道具箱"
string JoinCopyStr(int count, string str) {
    string result;
    result.reserve(count * 4);
    for (int i = 0; i < count; i++) {
        result += str;
    }
    return result;
}

// 全角文字列の変換
string ConvertStr(string str) {
    wstring_convert<codecvt_utf8_utf16<wchar_t>> converterTo16;
    wstring_convert<codecvt_utf8<wchar_t> > converterTo8;
    wstring tmpStrTo16 = converterTo16.from_bytes(str);
    string rStr = "";
    for (int i = 0; i < tmpStrTo16.length(); i++) {
        rStr += converterTo8.to_bytes(tmpStrTo16[i]);
    }
    return rStr;
}


#pragma endregion

#pragma region "シーンデータ"


struct Player {
    
    Vector2 position;
    Piece marker; // プレイヤーマーカー
    PlayerType type;
    PlayerRole role; // プレイヤーの役割
    bool runing = false; // プレイヤーが動いているかどうか
    bool isWin = false;
    vector<Vector2> step; // プレイヤーのステップ


};

struct KeyValue {
    bool key; // キーコード
    string value; // キーに対応する値
};
struct MenuSceneData {
    string title; // メニューのタイトル
    vector<string> options; // メニューの選択肢
    int selected; // 選択されているオプションのインデックス
};

struct GameSceneData {
    string title; // ゲームのタイトル

    Player masterPlayer;
    Player slavePlayer; // AIプレイヤーのスレーブ

    Piece chessData[MAX_SIZE][MAX_SIZE];   // 12 x 12 Max
};

struct ConfigSceneData {
    string title; // メニューのタイトル
    vector<KeyValue> options; // メニューの選択肢
    bool flag0; // 特別勝利フラグ かどうか
    bool flag1; // AIモード かどうか
    int selected; // 選択されているオプションのインデックス
    int itemSize; // 選択肢の数
};

struct ReadmeSceneData {
    string title; // 説明のタイトル
    vector<string> content; // 説明の内容
};
#pragma endregion


/********* System , Config , Resource *************/
#pragma region "色"
// 明度
enum ConsoleColorLightness {
    BRIGHT = 0,
    BOLD = 1,
    DIMMED = 2,
};

// 色
enum ConsoleColor {
    BLACK = 30,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    DEFAULT = 39,

};
#pragma endregion

#pragma region "Console システム関数"
void RunCmd(string cmd) {
    printf(cmd.c_str());
}

void OutPrint(string str) {
    printf("%s", str.c_str());
}

void OutPrintln(string str) {
    printf("%s\n", str.c_str());
}

void ConsoleCursorVisible(bool flag0) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info = { 100, flag0 };// カーソルを非表示にする
    SetConsoleCursorInfo(hOut, &info);
}

void ConsoleSetCursorPosition(int x, int y) {
    string cmd = "\x1b[" + to_string(y) + ';' + to_string(x) + 'H';
    RunCmd(cmd);
}

void ConsoleSetForegroundColor(ConsoleColorLightness lightness, ConsoleColor color) {
    string cmd = "\x1b[" + to_string(lightness) + ';' + to_string(color) + 'm';
    RunCmd(cmd);
}

void ConsoleSetForegroundColor(ConsoleColor color) {
    ConsoleSetForegroundColor(BOLD, color);
}

void ConsoleResetColor() {
    string cmd = "\x1b[39m";
    RunCmd(cmd);
}

void ConsoleClear() {
    RunCmd("\x1b[H");
    for (int i = 0; i < CONSOLE_HEIGHT ; ++i) {
        string clear = string(CONSOLE_WIDTH, SPACE);
        if (i == CONSOLE_HEIGHT - 1) {
            OutPrint(clear);
        } else {
            OutPrintln(clear);
        }
    }
    // 最後にカーソルを先頭に戻す（ANSIが使える場合）
    RunCmd("\x1b[H");
}

void ConsoleTitle(string title) {
    SetConsoleTitleA(title.c_str());
}

#pragma endregion

#pragma region "システムとプレイヤー設定"

struct SystemSetting {
    int targetFPS;
    double frameDurationMS;
    int consoleWidth;
    int consoleHeight;
    int screenWidth;
    int screenHeight;
    int spriteMaxWidth;
    int spriteMaxHeight;

    SystemSetting():
        targetFPS(20),
        frameDurationMS(1000.0 / 20),
        consoleWidth(CONSOLE_WIDTH),
        consoleHeight(CONSOLE_HEIGHT),
        screenWidth(CONSOLE_WIDTH - 1),
        screenHeight(CONSOLE_HEIGHT),
        spriteMaxWidth(CONSOLE_WIDTH - 3),
        spriteMaxHeight(CONSOLE_HEIGHT - 2) {}
};

struct SystemVariable {
    int frame = 0;
    clock_t startTime;
    clock_t endTime;
    int fps;
    double gameDelay;
};

struct GameSystem {
    SystemSetting setting;
    SystemVariable val;
    bool running = true;
};
#pragma endregion

#pragma region "GameCoreData"
struct Points {
    Vector2 pos[AN];
    int score[AN];
};

struct Decision {
    Vector2 pos;
    int score;
};



enum L_ShapeCode {
    L_OTHER,           //0
    L_WIN,             //100000
    L_LOSE,            //-10000000
    L_FLEX4,           //50000
    L_flex4,           //-100000
    L_BLOCK4,          //400
    L_block4,          //-100000
    L_FLEX3,           //400
    L_flex3,           //-8000
    L_BLOCK3,          //20
    L_block3,         //-50
    L_FLEX2,          //20
    L_flex2,          //-50
    L_BLOCK2,         //1
    L_block2,         //-3
    L_FLEX1,          //1
    L_flex1,          //-3
    L_SHAPE_COUNT,
};

enum M_ShapeCode {
    M_OTHER,           //0
    M_WIN,             //100000
    M_LOSE,            //-10000000
    M_FLEX3,           //50000
    M_flex3,           //-100000
    M_BLOCK3,          //400
    M_block3,          //-100000
    M_FLEX2,           //400
    M_flex2,           //-8000
    M_BLOCK2,          //20
    M_block2,         //-50
    M_FLEX1,          //20
    M_flex1,          //-50
    M_BLOCK1,         //1
    M_block1,         //-3
    M_SHAPE_COUNT,
};

enum S_ShapeCode {
    S_OTHER,           //0
    S_WIN,             //100000
    S_LOSE,            //-10000000
    S_FLEX2,           //50000
    S_flex2,           //-100000
    S_BLOCK2,          //400
    S_block2,          //-100000
    S_FLEX1,           //400
    S_flex1,           //-8000
    S_BLOCK1,          //20
    S_block1,         //-50
    S_SHAPE_COUNT,
};

typedef struct AIAnalysis {
    // 3x3 k = 3
    S_ShapeCode smallPatternTable[Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT];
    // 7x7 k = 4
    M_ShapeCode mediumPatternTable[Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT];
    // 12x12 k = 5
    L_ShapeCode largePatternTable[Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT][Piece::PIECE_TYPE_COUNT];

    // AI 計算したの結果
    Decision decision;


} AIAnalysis;

typedef struct AnalysisFuncInterface {
    void (*Init)(AnalysisFuncInterface*);
    int (*Evaluation)(AnalysisFuncInterface*);

    void* localContext;

    AIAnalysis analysis;

} AnalysisFuncInterface;

AnalysisFuncInterface* func = NULL;






struct GameConfig {
    Chessboard size = Chessboard::SMALL;
    bool specialVictory = false; // 特別な勝利条件
    bool enableAI = false; // AIを有効にするかどうか
};

struct GameData {
    int masterPlayerTotalWin = 0; // プレイヤーの勝利回数
    int slavePlayerTotalWin = 0; // 
    PlayerRole preWin = PlayerRole::NONE; // プレイヤーの前回勝利
    int masterPlayerWinStreak = 0; // プレイヤーの連勝
    int slavePlayerWinStreak = 0; // プレイヤーの連勝
};


struct GameSession {
    GameSystem sys;
    GameConfig config;
    GameData data;
};

typedef struct SceneInterface {
    void (*Init)(SceneInterface*, GameSession*);
    void (*KeyInput)(SceneInterface*);
    void (*Update)(SceneInterface*);
    void (*Render)(SceneInterface*);
    void (*Shutdown)(SceneInterface*);

    void* localContext;
} SceneInterface;

SceneInterface* getCurrentScene = NULL;
SceneInterface* getNextScene = NULL;

typedef struct {
    EventType type;
    void* data;
} Event;

typedef void (*EventHandler)(Event*);

EventHandler getEventHandlers[EVENT_COUNT][MAX_EVENT_HANDLERS] = { 0 };

struct MNKBoard {
    Vector2 board; // ボードのサイズ
    int winningCount; // 勝利条件の数
};

struct GameUI {
    string screenBuffer[CONSOLE_WIDTH - 1][CONSOLE_HEIGHT];
    string screenGraphic;
    
    string border;
    MNKBoard mnk;

    Vector2 offset;

    Vector2 inputKey;
    bool selected;    
};   

struct SceneData {
    MenuSceneData menu; // メニューシーンのデータ
    GameSceneData game; // ゲームシーンのデータ
    ConfigSceneData config; // 設定シーンのデータ
    ReadmeSceneData readme; // 説明シーンのデータ
};



struct GameLocalContext {
    GameSession* session;  // ゲームセッションのインスタンス

    GameUI ui;
    
    SceneData data; // シーンデータ
};

GameSession* CmdGameInitialize(string title, int width, int height) {
    GameSession* session = new GameSession();
    
    SetConsoleTitleA(title.c_str());
    return session;
}

bool CmdGameProcessMessage(GameSession* gs) {
    gs->sys.val.frame++;
    return gs->sys.running;
}

void CmdGameBeginFrame(GameSession* gs) {
    gs->sys.val.startTime = clock();
}

void CmdGameEndFrame(GameSession* gs) {
    gs->sys.val.endTime = clock();
    gs->sys.val.fps = (int)(gs->sys.val.frame / ((double)clock() / CLOCKS_PER_SEC));
    double frameTime = (double)(gs->sys.val.endTime - gs->sys.val.startTime) * 1000 / CLOCKS_PER_SEC;
    if (frameTime < gs->sys.setting.frameDurationMS) {
        gs->sys.val.gameDelay = gs->sys.setting.frameDurationMS - frameTime;
        Sleep((int)gs->sys.val.gameDelay);
    }
}

void CmdGameFinalize(GameSession* gs) {
    delete gs;
    if (func) {
        delete func;
    }
    if (getCurrentScene) {
        delete getCurrentScene;
    }
    if (getNextScene) {
        delete getNextScene;
    }
    if (getEventHandlers) {
        delete getEventHandlers;
    }
}
#pragma endregion

#pragma region "描画"
void RenderToScreen(GameLocalContext* ctx) {
    for (int y = 0; y < ctx->session->sys.setting.screenHeight; y++) {
        for (int x = 0; x < ctx->session->sys.setting.screenWidth; x++) {
            if (ctx->ui.screenBuffer[x][y] == string(1, NULL_CHAR)) {
                ctx->ui.screenGraphic += (SPACE);
            } else {
                ctx->ui.screenGraphic += ctx->ui.screenBuffer[x][y];
            }
        }
        if (y < ctx->session->sys.setting.screenHeight - 1) {
            ctx->ui.screenGraphic += string(1, NEWLINE);
        }
    }
    ConsoleSetCursorPosition(0, 0);
    OutPrint(ctx->ui.screenGraphic);


    for (int i = 0; i < CONSOLE_WIDTH - 1; i++) {
        ctx->ui.screenBuffer[i]->clear();
    }

    ctx->ui.screenGraphic.clear(); // Clear the screen graphic buffer
}

void WriteToBuffer(int xPos, int yPos, string characters, GameLocalContext* ctx) {
    int x = xPos;
    int y = yPos;
    wstring_convert<codecvt_utf8_utf16<wchar_t>> converterTo16;
    wstring_convert<codecvt_utf8<wchar_t> > converterTo8;
    wstring wide = converterTo16.from_bytes(characters);
    for (int i = 0; i < wide.length(); i++) {
        string charStr = converterTo8.to_bytes(wide[i]);
        if (charStr == string(1, NEWLINE)) {
            y++;
            x = xPos;
        } else if (x < 0 || y < 0 || x >= ctx->session->sys.setting.screenWidth || y >= ctx->session->sys.setting.screenHeight) {
            x++;
        } else {
            ctx->ui.screenBuffer[x][y] = charStr;
            x++;
        }
    }
}

void RenderToScreenWithColour(int xPos, int yPos, ConsoleColor colour, string characters, GameLocalContext* ctx) {
    int x = xPos;
    int y = yPos;
    ConsoleSetForegroundColor(colour);
    wstring_convert<codecvt_utf8_utf16<wchar_t>> converterTo16;
    wstring_convert<codecvt_utf8<wchar_t> > converterTo8;
    wstring wide = converterTo16.from_bytes(characters);

    for (int i = 0; i < wide.length(); i++) {
        string charStr = converterTo8.to_bytes(wide[i]);

        if (charStr == string(1, NEWLINE)) {
            y++;
            x = xPos;
            continue;
        }

        if (charStr.find_first_not_of(SPACE) == string::npos) {
            x++;
            continue;
        }

        if (x >= 1 && x < ctx->session->sys.setting.screenWidth - 1 &&
            y >= 1 && y < ctx->session->sys.setting.screenHeight - 1) {
            if (EMPTY_CHAR_SW && charStr == string(1, EMPTY_CHAR)) {
                ConsoleSetCursorPosition(x, y);
                OutPrint(" ");
            } else {
                ConsoleSetCursorPosition(x, y);
                OutPrint(charStr);
            }
        }
        x++;
    }
    ConsoleSetForegroundColor(WHITE);
}

string SpritesEnvironmentBorder(GameSession& gs) {
    // Allocate strings on the heap instead of the stack  
    string* topBorder = new string("╔" + JoinCopyStr(gs.sys.setting.spriteMaxWidth, "═") + "╗" + string(1, NEWLINE));
    string* middleBorder = new string("║" + JoinCopyStr(gs.sys.setting.spriteMaxWidth, " ") + "║" + string(1, NEWLINE));
    string* bottomBorder = new string("╚" + JoinCopyStr(gs.sys.setting.spriteMaxWidth, "═") + "╝");

    // Combine the strings into a single result  
    string allBorder = *topBorder + JoinCopyStr(gs.sys.setting.spriteMaxHeight, *middleBorder) + *bottomBorder;

    // Free the allocated memory  
    delete topBorder;
    delete middleBorder;
    delete bottomBorder;

    return allBorder;
}
#pragma endregion

/**************************************************/

#pragma region "イベント駆動.シーン切り替え.イベント登録"



void RegisterEventHandler(EventType type, EventHandler handler) {
    for (int i = 0; i < MAX_EVENT_HANDLERS; ++i) {
        if (!getEventHandlers[type][i]) {
            getEventHandlers[type][i] = handler;
            return;
        }
    }
}

void UnregisterEventHandler(EventType type, EventHandler handler) {
    for (int i = 0; i < MAX_EVENT_HANDLERS; ++i) {
        if (getEventHandlers[type][i] == handler) {
            getEventHandlers[type][i] = NULL;
            return;
        }
    }
}

void DispatchEvent(Event* event) {
    for (int i = 0; i < MAX_EVENT_HANDLERS; ++i) {
        if (getEventHandlers[event->type][i]) {
            getEventHandlers[event->type][i](event);
        }
    }
}

#pragma endregion

#pragma region "イベント.シーンチェンジ"

typedef struct {
    SceneInterface* payload;
} SceneChangeData;

void HandleSceneChange(Event* e) {
    SceneChangeData* data = (SceneChangeData*)e->data;
    getNextScene = data->payload;
}
void DefaultInitImpl(SceneInterface* self, GameSession* session);

void DefaultUpdateImpl(SceneInterface* self);

void DefaultRenderImpl(SceneInterface* self);

void DefaultShutdownImpl(SceneInterface* self);

void DefaultKeyInputImpl(SceneInterface* self);
#pragma endregion

#pragma region "イベント.シーン.メニュー"

void Menu_Init(SceneInterface* self, GameSession* session);

void Menu_KeyInput(SceneInterface* self);

void Menu_Update(SceneInterface* self);

void Menu_Render(SceneInterface* self);

void Menu_Shutdown(SceneInterface* self);


SceneInterface MenuScene = {
    Menu_Init,
    Menu_KeyInput,
    Menu_Update,
    Menu_Render,
    Menu_Shutdown
};

#pragma endregion

#pragma region "イベント.コンフィグ"

void Config_Init(SceneInterface* self, GameSession* session);

void Config_KeyInput(SceneInterface* self);

void Config_Update(SceneInterface* self);

void Config_Render(SceneInterface* self);

void Config_Shutdown(SceneInterface* self);


SceneInterface ConfigScene = {
    Config_Init,
    Config_KeyInput,
    Config_Update,
    Config_Render,
    Config_Shutdown
};

#pragma endregion

#pragma region "イベント.シーン.説明"

void Readme_Init(SceneInterface* self, GameSession* session);

void Readme_KeyInput(SceneInterface* self);

void Readme_Update(SceneInterface* self);

void Readme_Render(SceneInterface* self);

void Readme_Shutdown(SceneInterface* self);


SceneInterface ReadmeScene = {
    Readme_Init,
    Readme_KeyInput,
    Readme_Update,
    Readme_Render,
    Readme_Shutdown
};

#pragma endregion

#pragma region "イベント.シーン.ゲーム"

void Game_Init(SceneInterface* self, GameSession* session);

void Game_KeyInput(SceneInterface* self);

void Game_Update(SceneInterface* self);

void Game_Render(SceneInterface* self);

void Game_Shutdown(SceneInterface* self);

SceneInterface GameScene = {
    Game_Init,
    Game_KeyInput,
    Game_Update,
    Game_Render,
    Game_Shutdown
};

#pragma endregion

#pragma region "判定.MNK類ゲームの通用アルゴリズム"
Player* GetCurrentPlayer(GameLocalContext* ctx) {
    if (ctx->data.game.masterPlayer.runing) {
        return &ctx->data.game.masterPlayer;
    } else if (ctx->data.game.slavePlayer.runing) {
        return &ctx->data.game.slavePlayer;
    }
    return nullptr;
}

void SetCurrentPlayerWinCount(GameLocalContext* ctx, Player* p) {
    p->isWin = true;
    if (p->role == PlayerRole::MASTER) {
        ctx->session->data.masterPlayerTotalWin++; // マスターの勝利回数を増やす
        if (ctx->session->data.preWin == p->role) {
            ctx->session->data.masterPlayerWinStreak++; // マスターの連勝回数を増やす
        } else {
            ctx->session->data.slavePlayerWinStreak = 0; // マスターの連勝回数をリセット
        }
    } else if (p->role == PlayerRole::SLAVE) {
        ctx->session->data.slavePlayerTotalWin++; // スレーブの勝利回数を増やす
        if (ctx->session->data.preWin == p->role) {
            ctx->session->data.slavePlayerWinStreak++; // スレーブの連勝回数を増やす
        } else {
            ctx->session->data.masterPlayerWinStreak = 0; // スレーブの連勝回数をリセット
        }
    }
    ctx->session->data.preWin = p->role; // 勝利したプレイヤーを記録
}

bool DetectBoardHorizontal(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
    int winCount = ctx->ui.mnk.winningCount;
    int col = ctx->ui.mnk.board.x;
    int initX = p->position.x - winCount + 1 < 0 ? 0 : p->position.x - winCount + 1;
    int finalX = p->position.x + winCount - 1 >= col ? col - 1 : p->position.x + winCount - 1;
    bool isWin = true;
    for (int x = initX; x <= finalX; x++) {
        if (x + winCount - 1 >= col) {
            break;
        }

        isWin = true;
        for (int k = x; k < (x + winCount - 1); k++) {
            if (chessData[p->position.y][k] == p->marker) {
                if (chessData[p->position.y][k] != chessData[p->position.y][k + 1]) {
                    isWin = false;
                    break;
                }
            } else {
                isWin = false;
            }
        }
        if (isWin) {
            break;
        }
    }

    return isWin;
}

bool DetectBoardVertical(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
   
    int winCount = ctx->ui.mnk.winningCount;
    int row = ctx->ui.mnk.board.y;
    int initY = p->position.y - winCount + 1 < 0 ? 0 : p->position.y - winCount + 1;
    int finalY = p->position.y + winCount - 1 >= row ? row - 1 : p->position.y + winCount - 1;
    bool isWin = true;

    for (int y = initY; y <= finalY; y++) {
        if (y + winCount - 1 >= row) {
            break;
        }
        isWin = true;
        for (int k = y; k < (y + winCount - 1); k++) {
            if (chessData[k][p->position.x] == p->marker) {
                if (chessData[k][p->position.x] != chessData[k + 1][p->position.x]) {
                    isWin = false;
                    break;
                }
            } else {
                isWin = false;
            }
        }
        if (isWin) {
            break;
        }
    }
    return isWin;
}
bool DetectBoardDiagonalRight(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
   
    int initY, initX, finalY, finalX;
    int winCount = ctx->ui.mnk.winningCount;
    int row = ctx->ui.mnk.board.y;
    int col = ctx->ui.mnk.board.x;
    if (p->position.y + p->position.x >= row) {
        initY = p->position.y + winCount - 1 >= row ? row - 1 : p->position.y + winCount - 1;
        initX = p->position.x - (initY - p->position.y);

        finalX = p->position.x + winCount - 1 >= col ? col - 1 : p->position.x + winCount - 1;
        finalY = initY - (finalX - p->position.x);
    } else {
        initX = p->position.x - winCount + 1 < 0 ? 0 : p->position.x - winCount + 1;
        initY = p->position.y + (p->position.x - initX);

        finalY = p->position.y - winCount + 1 < 0 ? 0 : p->position.y - winCount + 1;
        finalX = p->position.x + (p->position.y - finalY);
    }
    bool isWin = false;
    for (int x = initX, y = initY; y >= finalY; x++, y--) {
        if (y - winCount + 1 < 0) {
            break;
        }
        isWin = true;
        for (int l = x, k = y; k > y - winCount + 1; k--, l++) {
            if (chessData[k][l] == p->marker) {
                if (chessData[k][l] != chessData[k - 1][l + 1]) {
                    isWin = false;
                    break;
                }
            } else {
                isWin = false;
            }
        }
        if (isWin) {
            break;
        }
    }
    return isWin;
}
bool DetectBoardDiagonalLeft(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
    int initY, initX, finalY, finalX;
    int winCount = ctx->ui.mnk.winningCount;
    int row = ctx->ui.mnk.board.y;
    int col = ctx->ui.mnk.board.x;
    if (p->position.y >= p->position.x) {
        initX = p->position.x - winCount + 1 < 0 ? 0 : p->position.x - winCount + 1;
        initY = p->position.y - (p->position.x - initX);

        finalY = p->position.y + winCount - 1 >= row ? row - 1 : p->position.y + winCount - 1;
        finalX = p->position.x + (finalY - p->position.y);
    } else {
        initY = p->position.y - winCount + 1 < 0 ? 0 : p->position.y - winCount + 1;
        initX = p->position.x - (p->position.y - initY);

        finalX = p->position.x + winCount - 1 >= col ? col - 1 : p->position.x + winCount - 1;
        finalY = p->position.y + (finalX - p->position.x);
    }
    bool isWin = false;
    for (int y = initY, x = initX; y <= finalY; y++) {
        if ((y + winCount - 1 >= row) || (x + winCount - 1 >= col)) {
            break;
        }
        isWin = true;
        for (int k = y, l = x; k < y + winCount - 1; k++, l++) {
            if (chessData[k][l] == p->marker) {
                if (chessData[k][l] != chessData[k + 1][l + 1]) {
                    isWin = false;
                    break;
                }
            } else {
                isWin = false;
            }
        }
        if (isWin){
            //SetCurrentPlayerWinCount(ctx, p);
            break;
        }
    }
    return isWin;
}

bool DetectSpecialVictory(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
    if (ctx->session->config.specialVictory) {
        // 特別勝利条件の判定ロジックを実装
        if (chessData[0][0] == p->marker && 
            chessData[0][ctx->ui.mnk.board.x - 1] == p->marker && 
            chessData[ctx->ui.mnk.board.y - 1][0] == p->marker &&
            chessData[ctx->ui.mnk.board.y - 1][ctx->ui.mnk.board.x - 1] == p->marker) {
            return true;
        }
    }
    return false;
}

bool DetectWin(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
    if (DetectBoardHorizontal(ctx, p, chessData) || DetectBoardVertical(ctx, p, chessData) ||
        DetectBoardDiagonalRight(ctx, p, chessData) || DetectBoardDiagonalLeft(ctx, p, chessData) || DetectSpecialVictory(ctx, p, chessData)) {
        return true;
    }
    return false;
}

bool DetectDraw(GameLocalContext* ctx) {
    Piece cpB1[MAX_SIZE][MAX_SIZE];
    Piece cpB2[MAX_SIZE][MAX_SIZE];
    bool canWin = false;
    for (int y = 0; y < ctx->ui.mnk.board.y; y++) {
        for (int x = 0; x < ctx->ui.mnk.board.x; x++) {
            cpB1[y][x] = (ctx->data.game.chessData[y][x]);
            cpB2[y][x] = (ctx->data.game.chessData[y][x]);
        }
    }
    // 盤面が全て埋まっているかどうかをチェック
    for (int y = 0; y < ctx->ui.mnk.board.y; y++) {
        for (int x = 0; x < ctx->ui.mnk.board.x; x++) {
            if (ctx->data.game.chessData[y][x] == Piece::NONE_PIECE) {
                cpB1[y][x] = Piece::BLACK_PIECE;
                // 盤面に黒を置いて勝利判定
                
                canWin = DetectWin(ctx, &ctx->data.game.masterPlayer, cpB1);
                if (canWin) {
                    return true;
                }

                cpB2[y][x] = Piece::WHITE_PIECE;
                // 盤面に黒を置いて勝利判定
                canWin = DetectWin(ctx, &ctx->data.game.slavePlayer, cpB2);
                if (canWin) {
                    return true;
                }
            }
        }
    }
    return false;
}

#pragma endregion

#pragma region "AI.アルゴリズム.設定"





void Small_InitPatternTable(AnalysisFuncInterface* self);
int Small_Evaluation(AnalysisFuncInterface* self);

AnalysisFuncInterface SmallShapeAI = {
    Small_InitPatternTable,
    Small_Evaluation,
};

void Medium_InitPatternTable(AnalysisFuncInterface* self);
int Medium_Evaluation(AnalysisFuncInterface* self);

AnalysisFuncInterface MediumShapeAI = {
    Medium_InitPatternTable,
    Medium_Evaluation,
};

void Large_InitPatternTable(AnalysisFuncInterface* self);
int Large_Evaluation(AnalysisFuncInterface* self);

AnalysisFuncInterface LargeShapeAI = {
    Large_InitPatternTable,
    Large_Evaluation,
};

int Minimax(vector<vector<Piece>> board, int depth, int alpha, int beta, GameLocalContext* ctx);

#pragma endregion 

int main() {
    GameSession* session = CmdGameInitialize(AUTHOR, CONSOLE_WIDTH, CONSOLE_HEIGHT);
    ConsoleCursorVisible(false);

    RegisterEventHandler(EventType::SYS_SCENE_CHANGE, HandleSceneChange);
    getNextScene = &MenuScene;

    while (CmdGameProcessMessage(session)) {
        CmdGameBeginFrame(session);

        if (getNextScene) {
            if (getCurrentScene && getCurrentScene->Shutdown) {
                getCurrentScene->Shutdown(getCurrentScene); // 現在のシーンをシャットダウン
            }
            getCurrentScene = getNextScene; // 次のシーンを現在のシーンに設定
            getNextScene = NULL; // 次のシーンをリセット
            if (getCurrentScene->Init) {
                getCurrentScene->Init(getCurrentScene, session); // 現在のシーンを初期化
            }
        }
        if (getCurrentScene && getCurrentScene->KeyInput) {
            getCurrentScene->KeyInput(getCurrentScene); // 現在のシーンのキー入力を処理
        }

        if (getCurrentScene && getCurrentScene->Update) {
            getCurrentScene->Update(getCurrentScene); // 現在のシーンを更新
        }

        if (getCurrentScene && getCurrentScene->Render) {
            getCurrentScene->Render(getCurrentScene); // 現在のシーンを描画
        }


        string title = AUTHOR + " FPS:" + to_string(session->sys.val.fps);
        ConsoleTitle(title);

        CmdGameEndFrame(session);
    }
    CmdGameFinalize(session);
    return 0;
}

#pragma region "イベント.default.実現"
void DefaultInitImpl(SceneInterface* self, GameSession* session) {
    GameLocalContext* ctx = new GameLocalContext;
    ctx->session = session;
    self->localContext = ctx;

    ctx->ui.border = SpritesEnvironmentBorder(*session);
}

void DefaultUpdateImpl(SceneInterface* self) {
    self;
}

void DefaultRenderImpl(SceneInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    WriteToBuffer(0,0, ctx->ui.border, ctx);
}
void DefaultShutdownImpl(SceneInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    delete ctx;
    self->localContext = nullptr;
    ConsoleClear(); // コンソールをクリア
}

void DefaultKeyInputImpl(SceneInterface* self) {
    self;
}

#pragma endregion


#pragma region "イベント.メニュー.実現"
void Menu_Init(SceneInterface* self, GameSession* session) {
    DefaultInitImpl(self, session);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    
    ctx->data.menu.title = "Tic Tac Toe v0.1";
    ctx->data.menu.options = { "1. Game Start", "2. Config", "3. Readme", "4. Exit" };
    ctx->data.menu.selected = 0; // 最初のオプションを選択
    
}
void Menu_KeyInput(SceneInterface* self) {
    DefaultKeyInputImpl(self);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    if (_kbhit()) {
        int key = _getch();
        if (key == W_KEY || key == UP_ARROW) {
            ctx->data.menu.selected = (ctx->data.menu.selected - 1 + ctx->data.menu.options.size()) % ctx->data.menu.options.size();
        } else if (key == S_KEY || key == DOWN_ARROW) {
            ctx->data.menu.selected = (ctx->data.menu.selected + 1) % ctx->data.menu.options.size();
        } else if (key == ENTER_KEY) {
            static SceneChangeData data;
            // 選択されたオプションに応じてシーンを切り替える
            switch (ctx->data.menu.selected) {
                case 0: // ゲーム開始
                    extern SceneInterface GameScene;
                    data.payload = &GameScene;
                    break;
                case 1: // 設定
                    extern SceneInterface ConfigScene;
                    data.payload = &ConfigScene;
                    break;
                case 2: // 説明
                    extern SceneInterface ReadmeScene;
                    data.payload = &ReadmeScene;
                    break;
                case 3: // 終了
                    exit(0);
                    break;
            }
            DispatchEvent(new Event{ SYS_SCENE_CHANGE, &data });
        }
    }
}
void Menu_Update(SceneInterface* self) {
    DefaultUpdateImpl(self);
}
void Menu_Render(SceneInterface* self) {
    DefaultRenderImpl(self);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    int x = ctx->session->sys.setting.screenWidth / 2 - ctx->data.menu.title.length();
    int y = ctx->session->sys.setting.screenHeight / 4;
    // titleを描画
    WriteToBuffer(x, y, ctx->data.menu.title, ctx);
    y += 3; // タイトルの下にスペースを空ける
    // optionsを描画
    for (int i = 0; i < ctx->data.menu.options.size(); i++) {
        string text = ctx->data.menu.options[i];
        if (i == ctx->data.menu.selected) {
            // 選択されているオプションは強調表示
            text = "> " + text + " <"; // 矢印で囲む
            x--;
        } 
        WriteToBuffer(x, y, text, ctx);
        y += 2;
    }
    
    RenderToScreen(ctx);
}
void Menu_Shutdown(SceneInterface* self) {
    DefaultShutdownImpl(self);
}

#pragma endregion

#pragma region "イベント.コンフィグ.実現"
void Config_Init(SceneInterface* self, GameSession* session) {
    DefaultInitImpl(self, session);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);

    ctx->data.config.title = "Game Config";
    ctx->data.config.options = 
    { 
        { false, " 3 x 3 " }, 
        { false, " 7 x 7 " },
        { false, " 12 x 12 " },
    };
    ctx->data.config.options[(ctx->session->config.size)].key = true; // 選択されたオプションをtrueに設定
    ctx->data.config.flag0 = ctx->session->config.specialVictory; 
    ctx->data.config.flag1 = ctx->session->config.enableAI; 
    ctx->data.config.selected = 0;
    ctx->data.config.itemSize = ctx->data.config.options.size() + 3; // オプションの数 + 特別勝利フラグ + AIモード + 戻るボタン
}
void Config_KeyInput(SceneInterface* self) {
    DefaultKeyInputImpl(self);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    if (_kbhit()) {
        int key = _getch();
        if (key == W_KEY || key == UP_ARROW) {
            ctx->data.config.selected = ((ctx->data.config.selected - 1 < 0) ? ctx->data.config.itemSize  : (ctx->data.config.selected )) - 1 % ctx->data.config.itemSize;
        } else if (key == S_KEY || key == DOWN_ARROW) {
            ctx->data.config.selected = (ctx->data.config.selected + 1) % ctx->data.config.itemSize;
        } else if (key == ESCAPE_KEY) {
            static SceneChangeData data;
            extern SceneInterface MenuScene;
            data.payload = &MenuScene;
            DispatchEvent(new Event{ SYS_SCENE_CHANGE, &data });
        } else if (key == ENTER_KEY) {
            // 選択されたオプションに応じてシーンを切り替える
            if (ctx->data.config.selected == ctx->data.config.itemSize - 3) {
                ctx->data.config.flag0 = !ctx->data.config.flag0; // 特別勝利フラグを切り替える
                ctx->session->config.specialVictory = ctx->data.config.flag0; // セッションの設定も更新
            } else if (ctx->data.config.selected == ctx->data.config.itemSize - 2) {
                ctx->data.config.flag1 = !ctx->data.config.flag1; // 特別勝利フラグを切り替える
                ctx->session->config.enableAI = ctx->data.config.flag1; // セッションの設定も更新
            } else if (ctx->data.config.selected == ctx->data.config.itemSize - 1) {
                static SceneChangeData data;
                extern SceneInterface MenuScene;
                data.payload = &MenuScene;
                DispatchEvent(new Event{ SYS_SCENE_CHANGE, &data });
            } else {
                for (int i = 0; i < ctx->data.config.options.size(); i++) {
                    ctx->data.config.options[i].key = false;
                }
                ctx->data.config.options[ctx->data.config.selected].key = true;
                ctx->session->config.size = (Chessboard)ctx->data.config.selected; // セッションの設定を更新
            }
        }
    }
}
void Config_Update(SceneInterface* self) {
    DefaultUpdateImpl(self);
}
void Config_Render(SceneInterface* self) {
    DefaultRenderImpl(self);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    int x = ctx->session->sys.setting.screenWidth / 2 - ctx->data.config.title.length();
    int y = ctx->session->sys.setting.screenHeight / 4;

    int optionSize = ctx->data.config.itemSize;

    // titleを描画
    WriteToBuffer(x, y, ctx->data.config.title, ctx);
    y += 3; // タイトルの下にスペースを空ける
    
    string titleAText = "Chess Board Size: ";
    string titleBText = "Special Victory: ";
    string titleCText = "AI Mode: ";
    
    
    x -= titleAText.length();
    WriteToBuffer(x, y, titleAText, ctx);
    x += titleAText.length(); // xを戻す
    for (int i = 0; i < ctx->data.config.options.size(); i++) {
        KeyValue kv = ctx->data.config.options[i];
        string optionText = "[";
        optionText.append(kv.key ? "*" : " ");
        optionText.append("]");
        if (i <= (optionSize - 3)) {
            if ((i) == ctx->data.config.selected) {
                optionText.append("> " + kv.value + " <");
            } else {
                optionText.append(kv.value);
            }
        }
        WriteToBuffer(x, y, optionText , ctx);
        y++;
    }
    y++;
    
    string title = titleBText;
    string value = string(ctx->data.config.flag0 ? "Enabled" : "Disabled");
    if (ctx->data.config.selected == ctx->data.config.itemSize - 3) {
        title += ("> " + value + " <");
    } else {
        title += value;
    }
    x -= titleBText.length();
    WriteToBuffer(x, y, title, ctx);
    x += titleBText.length();
    y++;

    title = titleCText;
    value = string(ctx->data.config.flag1 ? "Enabled" : "Disabled");
    if (ctx->data.config.selected == ctx->data.config.itemSize - 2) {
        title += ("> " + value + " <");
    } else {
        title += value;
    }
    x -= titleCText.length();
    WriteToBuffer(x, y, title, ctx);
    x += titleCText.length();

    y += 2;
    string backText = "BACK";
    if (ctx->data.config.selected == ctx->data.config.itemSize - 1) {
        backText = ("> " + backText + " <");
    } 
    WriteToBuffer(x, y, backText, ctx);
    RenderToScreen(ctx);
}
void Config_Shutdown(SceneInterface* self) {
    DefaultShutdownImpl(self);
}
#pragma endregion

#pragma region "イベント.説明.実現"
void Readme_Init(SceneInterface* self, GameSession* session) {
    DefaultInitImpl(self, session);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
 
    ctx->data.readme.title = "Readme - Tic Tac Toe v0.1";
    ctx->data.readme.content.push_back("1. W/A/S/Dキーでカーソルを上下左右に移動できます。Rキーでゲームをリセットする、" );
    ctx->data.readme.content.push_back("   ENTERキーで選択を確定し、ESCキーで前の画面に戻り、SPACEキーで駒を置きます。");
    ctx->data.readme.content.push_back("2. 各プレイヤーのカーソル位置は、直前に駒を置いた場所が記録され、");
    ctx->data.readme.content.push_back("   次の操作はその位置から始まります。");
    ctx->data.readme.content.push_back("3. 「特殊勝利条件」を有効にすると、いずれかのプレイヤーが盤面の四隅すべて");
    ctx->data.readme.content.push_back("   に自分の駒を置いた場合、そのプレイヤーの勝利となります。");
    ctx->data.readme.content.push_back("4. AIと対戦する場合は、あらかじめゲーム設定でAI対戦オプションを");
    ctx->data.readme.content.push_back("   有効にしてください。");
}

void Readme_KeyInput(SceneInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    if (_kbhit()) {
        int key = _getch();
        if (key == ENTER_KEY || key == ESCAPE_KEY) {
            static SceneChangeData data;
            extern SceneInterface MenuScene;
            data.payload = &MenuScene;
            DispatchEvent(new Event{ SYS_SCENE_CHANGE, &data });
        }
    }
}
void Readme_Update(SceneInterface* self) {
}
void Readme_Render(SceneInterface* self) {
    //DefaultRenderImpl(self);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    int x = ctx->session->sys.setting.screenWidth / 2;
    int textX = ctx->session->sys.setting.screenWidth / 2 - 30;

    WriteToBuffer(x, 1, ctx->data.readme.title, ctx);
    
    for (int i = 0; i < ctx->data.readme.content.size(); i++) {
        WriteToBuffer(textX, 5 + i, ctx->data.readme.content[i], ctx);
    }

    WriteToBuffer(textX, 7 + ctx->data.readme.content.size() , ("> BACK <"), ctx);
    
    RenderToScreen(ctx);
}     
void Readme_Shutdown(SceneInterface* self) {
    DefaultShutdownImpl(self);
}
#pragma endregion

#pragma region "イベント.ゲーム.実現"
string GetRowStr(int x, int width, string x1, string x2, string x3, string x4) {
    string strBuffer;
    if (x == 0) {
        strBuffer += x1;
    } else if (x == width - 1) {
        strBuffer += x2;
    } else if (x % 2 == 0) {
        strBuffer += x3;
    } else {
        strBuffer += x4;
    }
    return strBuffer;
}

void WriteChessBoard(GameLocalContext* ctx) {
    /*
    game board
    ┌───┬───┐
    │ ■ │ □ │
    ├───┼───┤
    │ □ │ ■ │
    └───┴───┘
    */
    int width = ctx->ui.mnk.board.x * 2 + 1;  // 例: cols=3 → 幅=7
    int height = ctx->ui.mnk.board.y * 2 + 1; // 例: rows=2 → 高さ=5
    string strBuffer;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int isBorderRow = (y == 0);
            int isMiddleRow = (y % 2 == 0 && y != 0 && y != height - 1);
            int isBottomRow = (y == height - 1);
            int isContentRow = (y % 2 == 1);

            if (isBorderRow) {  // 最上段
                strBuffer += GetRowStr(x, width, "┌", "┐", "┬", "───");
            } else if (isMiddleRow) {  // 中間の横罫線
                strBuffer += GetRowStr(x, width, "├", "┤", "┼", "───");
            } else if (isBottomRow) {   // 最下段
                strBuffer += GetRowStr(x, width, "└", "┘", "┴", "───");
            } else if (isContentRow) {   // 内容のある行
                if (x % 2 == 0) {
                    strBuffer += ("│");
                } else {
                    int indexX = (x - 1) / 2;
                    int indexY = (y - 1) / 2;
                    //
                    if (ctx->data.game.chessData[indexY][indexX] == Piece::NONE_PIECE) {
                        strBuffer += ("   ");
                    } else {
                        string piece = (ctx->data.game.chessData[indexY][indexX] == BLACK_PIECE ? BP : WP);
                        strBuffer += (" " + piece + " ");
                    }
                }
            }
        }
        strBuffer += NEWLINE;
    }
    WriteToBuffer(ctx->ui.offset.x, ctx->ui.offset.y, strBuffer, ctx);
}

void WriteGameState(GameLocalContext* ctx) {
    // player 1 
    int startX = ctx->ui.offset.x - 10;
    int startY = ctx->ui.offset.y;
    string titleKey = "[Player 1 ]";
    startX -= titleKey.length();
    string playerState = ctx->data.game.masterPlayer.isWin ? " Win" : (ctx->data.game.masterPlayer.runing ? " Runing" : "");
    WriteToBuffer(startX + 3, startY, titleKey + playerState, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Piece: ";
    string value = ctx->data.game.masterPlayer.marker == BLACK_PIECE ? BP : WP;
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Type: ";
    value = ctx->data.game.masterPlayer.type == HUMAN ? "Human" : "AI";
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Step: ";
    value = to_string(ctx->data.game.masterPlayer.step.size());
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Score: ";
    value = to_string(ctx->session->data.masterPlayerTotalWin);
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Win Streak: ";
    value = to_string(ctx->session->data.masterPlayerWinStreak);
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY += 2;

    // player 2
    titleKey = "[Player 2]";
    startX -= titleKey.length();
    playerState = ctx->data.game.slavePlayer.isWin ? " Win" : (ctx->data.game.slavePlayer.runing ? " Runing" : "");
    WriteToBuffer(startX + 3, startY, titleKey + playerState, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Piece: ";
    value = ctx->data.game.slavePlayer.marker == BLACK_PIECE ? BP : WP;
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Type: ";
    value = ctx->data.game.slavePlayer.type == HUMAN ? "Human" : "AI";
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Step: ";
    value = to_string(ctx->data.game.slavePlayer.step.size());
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Score: ";
    value = to_string(ctx->session->data.slavePlayerTotalWin);
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Win Streak: ";
    value = to_string(ctx->session->data.slavePlayerWinStreak);
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + value, ctx);
    startX += titleKey.length();

    startY += 2;

    titleKey = "Chess Board Size : ";
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + to_string(ctx->ui.mnk.board.x) + " x " + to_string(ctx->ui.mnk.board.y), ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Win Condition : ";
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + to_string(ctx->ui.mnk.winningCount), ctx);
    startX += titleKey.length();
    startY++;

    titleKey = "Special Victory : ";
    startX -= titleKey.length();
    WriteToBuffer(startX, startY, titleKey + (ctx->session->config.specialVictory ? "Enabled" : "Disabled"), ctx);
    startX += titleKey.length();
    startY += 2;

    titleKey = "[ Game is Draw!! ]";
    startX -= (titleKey.length() / 2);
    if (!ctx->data.game.masterPlayer.isWin && !ctx->data.game.masterPlayer.runing && !ctx->data.game.slavePlayer.isWin && !ctx->data.game.slavePlayer.runing) {
        WriteToBuffer(startX, startY, titleKey, ctx);
    }

}

void WritePlayerCrosshair(GameLocalContext* ctx) {
    int officetX = ctx->ui.offset.x; 
    int officetY = ctx->ui.offset.y;

    int indexX = 0;
    int indexY = 0;
    if (ctx->data.game.masterPlayer.runing) {
        indexX = ctx->data.game.masterPlayer.position.x;
        indexY = ctx->data.game.masterPlayer.position.y;
    } else if (ctx->data.game.slavePlayer.runing) {
        indexX = ctx->data.game.slavePlayer.position.x;
        indexY = ctx->data.game.slavePlayer.position.y;
    }

    int sx = officetX + (4 * indexX);
    int sy = officetY + (2 * indexY);
    Piece value = ctx->data.game.chessData[indexY][indexX];
    string piece = (value == Piece::NONE_PIECE ? " " : (value == Piece::BLACK_PIECE ? BP : WP));

    // 上辺（╔───╗）
    WriteToBuffer(sx, sy, "╔───╗", ctx);
    // 中身（│   │）
    
    WriteToBuffer(sx, sy + 1, "│ " + piece + " │", ctx);
    // 下辺（╚───╝）
    WriteToBuffer(sx, sy + 2, "╚───╝", ctx);
    
}


void SelectedPointAction(GameLocalContext* ctx, Player* current, Player* next) {
    if (ctx->ui.selected) {
        // クリックされた位置にピースを置く
        if (ctx->data.game.chessData[current->position.y][current->position.x] == Piece::NONE_PIECE) {
            ctx->data.game.chessData[current->position.y][current->position.x] = current->marker;
            current->step.push_back({ current->position.x, current->position.y });
            ctx->ui.selected = false;

            // 
            if (DetectWin(ctx, current, ctx->data.game.chessData)) {
                current->isWin = true;
                current->runing = false;
                // 勝利処理
                SetCurrentPlayerWinCount(ctx, current);
            } else if (!DetectDraw(ctx)) {
                // 引き分け処理
                current->isWin = false;
                current->runing = false;
                next->isWin = false;
                next->runing = false;
                ctx->session->data.masterPlayerWinStreak = 0;
                ctx->session->data.slavePlayerWinStreak = 0;
            } else {
                current->runing = false;
                next->runing = true;
            }
        }
    }
}

void PlayerAction(GameLocalContext* ctx, Player* current, Player* next ) {
    current->position.x += ctx->ui.inputKey.x;
    current->position.y += ctx->ui.inputKey.y;
    // 範囲外チェック
    current->position.x = max(0, min(ctx->ui.mnk.board.x - 1, current->position.x));
    current->position.y = max(0, min(ctx->ui.mnk.board.y - 1, current->position.y));
    SelectedPointAction(ctx, current, next);

    // clear input key
    ctx->ui.inputKey = { 0, 0 };
    ctx->ui.selected = false;
}

void AIAction(GameLocalContext* ctx, Player* current, Player* next) {
    vector<vector<Piece>> board(ctx->ui.mnk.board.y, vector<Piece>(ctx->ui.mnk.board.x));
    for (int y = 0; y < ctx->ui.mnk.board.y; y++) {
        for (int x = 0; x < ctx->ui.mnk.board.x; x++) {
            board.at(y).at(x) = ctx->data.game.chessData[y][x];
        }
    }
    int a = Minimax(board, DEPTH, INT_MIN, INT_MAX, ctx);
    ctx->data.game.slavePlayer.position.y = func->analysis.decision.pos.y;
    ctx->data.game.slavePlayer.position.x = func->analysis.decision.pos.x;
    ctx->ui.selected = true;
    SelectedPointAction(ctx, current, next);

    // clear AI input
    func->analysis.decision.pos = { 0, 0 };
    func->analysis.decision.score = -1;

    ctx->ui.selected = false;
}



///////////////////////////////////////////////////////////////
void Game_Init(SceneInterface* self, GameSession* session) {
    DefaultInitImpl(self, session);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    switch (ctx->session->config.size) {
        case SMALL:
            ctx->ui.mnk = { { 3, 3 }, 3 };
            func = &SmallShapeAI;
            break;
        case MEDIUM:
            ctx->ui.mnk = { { 7, 7 }, 4 };
            func = &MediumShapeAI;
            break;
        case LARGE:
            ctx->ui.mnk = { { MAX_SIZE, MAX_SIZE }, 5 };
            func = &LargeShapeAI;
            break;
    }

    if (func) {
        func->Init(func);
        func->localContext = ctx;
    }

    // Initialize chess data
    for (int y = 0; y < MAX_SIZE; y++) {
        for (int x = 0; x < MAX_SIZE; x++) {
            if (y < ctx->ui.mnk.board.y && x < ctx->ui.mnk.board.x) {
                ctx->data.game.chessData[y][x] = Piece::NONE_PIECE; // 有効な位置は空のピース
            } else {
                ctx->data.game.chessData[y][x] = Piece::INVALID_PIECE;
            }
        }
    }

    int offisetX = ctx->session->sys.setting.screenWidth / 3 ;
    int offisetY = 1;

    ctx->ui.offset.x = offisetX;
    ctx->ui.offset.y = offisetY;

    ctx->data.game.masterPlayer = { { 0, 0 }, BLACK_PIECE, HUMAN, MASTER, false, false, {}};
    ctx->data.game.slavePlayer = { { 0, 0 }, WHITE_PIECE, ctx->session->config.enableAI ? PlayerType::AI : PlayerType::HUMAN, SLAVE,  false, false, {}};

    ctx->data.game.masterPlayer.runing = true; // プレイヤー1は最初に動ける状態
    ctx->ui.selected = false; // 選択状態を初期化
}

void Game_KeyInput(SceneInterface* self) {
    DefaultKeyInputImpl(self);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    if (_kbhit()) {
        int key = _getch();
        Vector2 inputPosi = { 0, 0 };
        if (key == W_KEY || key == UP_ARROW) {
            inputPosi.y = -1;
        } else if (key == S_KEY || key == DOWN_ARROW) {
            inputPosi.y = 1;
        } else if (key == A_KEY || key == LEFT_ARROW) {
            inputPosi.x = -1;
        } else if (key == D_KEY || key == RIGHT_ARROW) {
            inputPosi.x = 1;
        }
        ctx->ui.inputKey = inputPosi; // 入力キーを更新

        if (key == ESCAPE_KEY) {
            static SceneChangeData data;
            extern SceneInterface MenuScene;
            data.payload = &MenuScene;
            DispatchEvent(new Event{ SYS_SCENE_CHANGE, &data });
        } else if (key == SPACE) {
            ctx->ui.selected = true;
        } else if (key == R_KEY) {
            static SceneChangeData data;
            extern SceneInterface GameScene;
            data.payload = &GameScene;
            DispatchEvent(new Event{ SYS_SCENE_CHANGE, &data });
        }
    }
}

void Game_Update(SceneInterface* self) {
    DefaultUpdateImpl(self);
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    Player* player1 = &ctx->data.game.masterPlayer;
    Player* player2 = &ctx->data.game.slavePlayer;

    if (player1->runing) {
        PlayerAction(ctx, player1, player2);
    } else if (player2->runing) {
        if (!ctx->session->config.enableAI) {
            PlayerAction(ctx, player2, player1);
        } else {
            // AIのターン
            AIAction(ctx, player2, player1);
        }
    } 

    
}

void Game_Render(SceneInterface* self) {
    DefaultRenderImpl(self);   
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    
    // Render Game Board
    WriteGameState(ctx);
    WriteChessBoard(ctx);

    //RenderToScreenWithColour(ctx->data.game.masterPlayer.colorPosi.x, ctx->data.game.masterPlayer.colorPosi.y, (ConsoleColor)ctx->data.game.masterPlayer.color, "Player 1", ctx);
    //RenderToScreenWithColour(ctx->data.game.slavePlayer.colorPosi.x, ctx->data.game.slavePlayer.colorPosi.y, (ConsoleColor)ctx->data.game.slavePlayer.color, "Player 2", ctx);

    // Render Player Crosshair [Player 1 and Player 2]
    WritePlayerCrosshair(ctx);
    
    RenderToScreen(ctx);
}
void Game_Shutdown(SceneInterface* self) {
    DefaultShutdownImpl(self);
}
#pragma endregion

#pragma region "AI.アルゴリズム.実現"

Points CollectHotZones(vector<vector<Piece>> board, GameLocalContext* ctx) {
    Chessboard size = ctx->session->config.size;
    MNKBoard mnk = ctx->ui.mnk;
    Player slave = ctx->data.game.slavePlayer;

    if (ctx->session->config.enableAI && func == nullptr) {
        exit(0);
    } 
    
    Points bestPoint{};
    int rows = mnk.board.y;
    int cols = mnk.board.x;
    vector<vector<bool>> localBoard(rows, vector<bool>(cols));
    vector<vector<int>> weight(rows, vector<int>(cols));

    // 局所探索では、既に置かれた石の周囲8方向・3マス以内を対象に、一致する位置を探す。
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (board.at(y).at(x) != Piece::NONE_PIECE) {
                for (int k = -3; k < 3; k++) {
                    // 横向き
                    if ((x + k < cols) && (x + k >= 0) && board.at(y).at(x + k) == Piece::NONE_PIECE) { 
                        localBoard.at(y).at(x + k) = true; 
                    }
                    // 縦向き
                    if ((y + k < rows) && (y + k) >= 0 && board.at(y + k).at(x) == Piece::NONE_PIECE) { 
                        localBoard.at(y + k).at(x) = true; 
                    }
                    // 斜向き（左上から右下）
                    if ((y + k < rows) && (y + k) >= 0 && (x + k < cols) && (x + k >= 0) && board.at(y + k).at(x + k) == Piece::NONE_PIECE) {
                        localBoard.at(y + k).at(x + k) = true;
                    }
                    // 斜向き（左下から右上）    
                    if ((y - k < rows) && (y - k) >= 0 && (x + k < cols) && (x + k >= 0) && board.at(y - k).at(x + k) == Piece::NONE_PIECE) {
                        localBoard.at(y - k).at(x + k) = true;
                    }
                }
            }
        }
    }

    // 局所的な探索のあとで手を打つシミュレーションを行い、その結果をヒューリスティックに評価する。
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            weight.at(y).at(x) = INT_MIN;
            if (localBoard.at(y).at(x)) {
                board.at(y).at(x) = slave.marker;
                weight.at(y).at(x) = func->Evaluation(func);
                board.at(y).at(x) = Piece::NONE_PIECE;
            }
        }
    }

    int w;
    Vector2 p = { 0, 0 };
    for (int k = 0; k < AN; k++) {
        w = INT_MIN;
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (weight.at(y).at(x) > w) {
                    w = weight.at(y).at(x);
                    p.x = x, p.y = y;
                }
            }
        }
        bestPoint.pos[k] = p, bestPoint.score[k] = w;
        weight.at(p.y).at(p.x) = INT_MIN; //最大値を除外して、次に大きいポイントを見つける。
    }

    return bestPoint;
}

void ReverseBoard(vector<vector<Piece>> board, vector<vector<Piece>>& rboard) {
    for (int y = 0; y < board.size(); y++) {
        for (int x = 0; x < board.at(0).size(); x++) {
            if (board.at(y).at(x) == BLACK_PIECE) {
                rboard.at(y).at(x) = WHITE_PIECE; 
            }
            if (board.at(y).at(x) == WHITE_PIECE) { 
                rboard.at(y).at(x) = BLACK_PIECE; 
            }
        }
    }
}

int Minimax(vector<vector<Piece>> board, int depth, int alpha, int beta, GameLocalContext* ctx) {
    if (depth == 0) {
        Points p;
        p = CollectHotZones(board, ctx);
        return p.score[0];
    } else {
        vector<vector<Piece>> cpBoard = board;
        if (depth % 2 == 0) { //max
            Points p = CollectHotZones(board, ctx);
            for (int i = 0; i < AN; i++) {
                cpBoard.at(p.pos[i].x).at(p.pos[i].y) = ctx->data.game.slavePlayer.marker;
                int a = Minimax(cpBoard, depth - 1, alpha, beta, ctx);
                cpBoard.at(p.pos[i].x).at(p.pos[i].y) = Piece::NONE_PIECE;
                if (a > alpha) {
                    alpha = a;
                    // もし、最大のalphaをさがしたら
                    if (depth == DEPTH) {
                        func->analysis.decision.pos = { p.pos[i].x, p.pos[i].y };
                        func->analysis.decision.score = p.score[i];
                    }
                }
                if (beta <= alpha) break; //枝刈りし
            }
            return alpha;
        } else if (depth % 2 == 1) { //min
            vector<vector<Piece>> rBoard(board.size(), vector<Piece>(board.at(0).size()));
            // 転回する
            ReverseBoard(cpBoard, rBoard);

            // 敵の最優なポイント
            Points p = CollectHotZones(rBoard, ctx);
            for (int i = 0; i < AN; i++) {
                cpBoard.at(p.pos[i].x).at(p.pos[i].y) = ctx->data.game.masterPlayer.marker;
                int a = Minimax(cpBoard, depth - 1, alpha, beta, ctx);
                cpBoard.at(p.pos[i].x).at(p.pos[i].y) = Piece::NONE_PIECE;
                if (a < beta) {
                    beta = a;
                }
                if (beta <= alpha) break; //枝刈りし
            }
            return beta;
        }
    }
}



/////////////// small AI /////////////// 

void Small_InitPatternTable(AnalysisFuncInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    // black win   
    func->analysis.smallPatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = S_LOSE;
    func->analysis.smallPatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = S_LOSE;
    func->analysis.smallPatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = S_LOSE;
    func->analysis.smallPatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][WHITE_PIECE] = S_LOSE;
    func->analysis.smallPatternTable[WHITE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = S_LOSE;
    func->analysis.smallPatternTable[INVALID_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = S_LOSE;
    func->analysis.smallPatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][INVALID_PIECE] = S_LOSE;
    // white win
    func->analysis.smallPatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = S_WIN;
    func->analysis.smallPatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = S_WIN;
    func->analysis.smallPatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = S_WIN;
    func->analysis.smallPatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][BLACK_PIECE] = S_WIN;
    func->analysis.smallPatternTable[BLACK_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = S_WIN;
    func->analysis.smallPatternTable[INVALID_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = S_WIN;
    func->analysis.smallPatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][INVALID_PIECE] = S_WIN;
    //
    func->analysis.smallPatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][INVALID_PIECE] = S_flex2;
    func->analysis.smallPatternTable[INVALID_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = S_flex2;
    func->analysis.smallPatternTable[BLACK_PIECE][NONE_PIECE][BLACK_PIECE][INVALID_PIECE] = S_flex2;
    func->analysis.smallPatternTable[INVALID_PIECE][BLACK_PIECE][NONE_PIECE][BLACK_PIECE] = S_flex2;

    func->analysis.smallPatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][INVALID_PIECE] = S_FLEX2;
    func->analysis.smallPatternTable[INVALID_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = S_FLEX2;
    func->analysis.smallPatternTable[WHITE_PIECE][NONE_PIECE][WHITE_PIECE][INVALID_PIECE] = S_FLEX2;
    func->analysis.smallPatternTable[INVALID_PIECE][WHITE_PIECE][NONE_PIECE][WHITE_PIECE] = S_FLEX2;

    // black flex 1
    func->analysis.smallPatternTable[INVALID_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE] = S_block1;
    func->analysis.smallPatternTable[INVALID_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE] = S_flex1;
    func->analysis.smallPatternTable[INVALID_PIECE][NONE_PIECE][NONE_PIECE][BLACK_PIECE] = S_block1;
    func->analysis.smallPatternTable[NONE_PIECE][BLACK_PIECE][NONE_PIECE][INVALID_PIECE] = S_flex1;
    func->analysis.smallPatternTable[NONE_PIECE][NONE_PIECE][BLACK_PIECE][INVALID_PIECE] = S_block1;
    func->analysis.smallPatternTable[BLACK_PIECE][NONE_PIECE][NONE_PIECE][INVALID_PIECE] = S_block1;
    // white flex 1
    func->analysis.smallPatternTable[INVALID_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE] = S_BLOCK1;
    func->analysis.smallPatternTable[INVALID_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE] = S_FLEX1;
    func->analysis.smallPatternTable[INVALID_PIECE][NONE_PIECE][NONE_PIECE][WHITE_PIECE] = S_BLOCK1;
    func->analysis.smallPatternTable[NONE_PIECE][WHITE_PIECE][NONE_PIECE][INVALID_PIECE] = S_FLEX1;
    func->analysis.smallPatternTable[NONE_PIECE][NONE_PIECE][WHITE_PIECE][INVALID_PIECE] = S_BLOCK1;
    func->analysis.smallPatternTable[WHITE_PIECE][NONE_PIECE][NONE_PIECE][INVALID_PIECE] = S_BLOCK1;


    int p1, p2, p3, p4, x, y, ix, iy;
    for (p1 = Piece::NONE_PIECE; p1 < Piece::PIECE_TYPE_COUNT; ++p1) {
        for (p2 = NONE_PIECE; p2 < PIECE_TYPE_COUNT - 1; ++p2) {
            for (p3 = NONE_PIECE; p3 < PIECE_TYPE_COUNT - 1; ++p3) {
                for (p4 = NONE_PIECE; p4 < PIECE_TYPE_COUNT - 1; ++p4) {
                        x = y = ix = iy = 0;

                        if (p1 == WHITE_PIECE) { x++; } else if (p1 == BLACK_PIECE) { y++; }

                        if (p2 == WHITE_PIECE) { x++; ix++; } else if (p2 == BLACK_PIECE) { y++; iy++; }

                        if (p3 == WHITE_PIECE) { x++; ix++; } else if (p3 == BLACK_PIECE) { y++; iy++; }

                        if (p4 == WHITE_PIECE) { x++; ix++; } else if (p4 == BLACK_PIECE) { y++; iy++; }



                        if (p1 == INVALID_PIECE || p4 == INVALID_PIECE) {
                            if (p1 == INVALID_PIECE && p4!= INVALID_PIECE) {
                                
                                if (ix == 0 && iy == 2) {
                                    if (func->analysis.smallPatternTable[p1][p2][p3][p4] == S_OTHER)
                                        func->analysis.smallPatternTable[p1][p2][p3][p4] = S_block2;
                                }
                                if (ix == 2 && iy == 0) {
                                    if (func->analysis.smallPatternTable[p1][p2][p3][p4] == S_OTHER)
                                        func->analysis.smallPatternTable[p1][p2][p3][p4] = S_BLOCK2;
                                }
                            } else if (p4 == Piece::INVALID_PIECE && p1 != Piece::INVALID_PIECE) {
                               
                                if (x == 0 && y == 2) {
                                    if (func->analysis.smallPatternTable[p1][p2][p3][p4] == S_OTHER)
                                        func->analysis.smallPatternTable[p1][p2][p3][p4] = S_block2;
                                }
                                if (x == 2 && y == 0) {
                                    if (func->analysis.smallPatternTable[p1][p2][p3][p4] == S_OTHER)
                                        func->analysis.smallPatternTable[p1][p2][p3][p4] = S_BLOCK2;
                                }
                            }
                        } else {
                            
                            if ((x == 0 && y == 2) || (ix == 0 && iy == 2)) {
                                if (func->analysis.smallPatternTable[p1][p2][p3][p4] == S_OTHER)
                                    func->analysis.smallPatternTable[p1][p2][p3][p4] = S_block2;
                            }
                            if ((x == 2 && y == 0) || (ix == 2 && iy == 0)) {
                                if (func->analysis.smallPatternTable[p1][p2][p3][p4] == S_OTHER)
                                    func->analysis.smallPatternTable[p1][p2][p3][p4] = S_BLOCK2;
                            }
                        }
                }
            }
        }
    }


}
int Small_Evaluation(AnalysisFuncInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    int rows = ctx->ui.mnk.board.y;
    int cols = ctx->ui.mnk.board.x;
    int k = ctx->ui.mnk.winningCount;
    // 重みの定義
    int weight[S_ShapeCode::S_SHAPE_COUNT] = { 0, 1000000, -10000000, 50000, -1000000, 400, -1000000, 400, -8000, 20, -50 };
    // 4方向の棋形を記録、該当する場合はカウントを+1
    int shapeRecord[4][S_ShapeCode::S_SHAPE_COUNT] = { { 0 } };
    // 大盤の境界を初期化
    vector<vector<int>> bigBoard(rows + 2, vector<int>(cols + 2));
    for (int i = 0; i < rows + 2; i++) {
        bigBoard.at(0).at(i) = Piece::INVALID_PIECE;
        bigBoard.at(i).at(0) = Piece::INVALID_PIECE;
        bigBoard.at(rows + 1).at(i) = Piece::INVALID_PIECE;
        bigBoard.at(i).at(cols + 1) = Piece::INVALID_PIECE;
    }

    // 現在の盤面をコピー
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++)
            bigBoard.at(y + 1).at(x + 1) = ctx->data.game.chessData[y][x];
    }

    int limit = 3;
    int _limit = limit + 1;
    //    横向きを計算
    for (int y = 1; y <= rows; y++) {
        for (int x = 1; x < cols - limit; x++) {
            int type = func->analysis.mediumPatternTable[bigBoard.at(y).at(x)][bigBoard.at(y).at(x + 1)][bigBoard.at(y).at(x + 2)][bigBoard.at(y).at(x + 3)][bigBoard.at(y).at(x + 4)];
            shapeRecord[0][type]++;
        }
    }
    //    縦向きを計算
    for (int y = 0; y < rows - limit; y++) {
        for (int x = 1; x <= cols; x++) {
            int type = func->analysis.mediumPatternTable[bigBoard.at(y).at(x)][bigBoard.at(y + 1).at(x)][bigBoard.at(y + 2).at(x)][bigBoard.at(y + 3).at(x)][bigBoard.at(y + 4).at(x)];
            shapeRecord[1][type]++;
        }
    }
    //    斜向き（左下から右上）を計算
    for (int y = rows - 1; y > _limit; y--) {
        for (int x = 0; x < cols - limit; x++) {
            int type = func->analysis.mediumPatternTable[bigBoard.at(y).at(x)][bigBoard.at(y - 1).at(x + 1)][bigBoard.at(y - 2).at(x + 2)][bigBoard.at(y - 3).at(x + 3)][bigBoard.at(y - 4).at(x + 4)];
            shapeRecord[2][type]++;
        }
    }
    //    斜向き（左上から右下）を計算
    for (int y = 0; y < rows - limit; y++) {
        for (int x = 0; x < cols - limit; x++) {
            int type = func->analysis.mediumPatternTable[bigBoard.at(y).at(x)][bigBoard.at(y + 1).at(x + 1)][bigBoard.at(y + 2).at(x + 2)][bigBoard.at(y + 3).at(x + 3)][bigBoard.at(y + 4).at(x + 4)];
            shapeRecord[3][type]++;
        }
    }

    int score = 0;
    int count = 0;
    for (int i = 0; i < M_ShapeCode::M_SHAPE_COUNT; i++) {
        score += weight[i] * shapeRecord[0][i] + weight[i] * shapeRecord[1][i] + weight[i] * shapeRecord[2][i] +
            weight[i] * shapeRecord[3][i];
        count = shapeRecord[0][i] + shapeRecord[1][i] + shapeRecord[2][i] + shapeRecord[3][i];
        if (count > 0) {
            string t = "Shape " + to_string(i) + ": " + to_string(count) + " × weight = " + to_string(weight[i]);
            int i = 1;
        }

    }
    return score;
}

/////////////// medium AI /////////////// 
void Medium_InitPatternTable(AnalysisFuncInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    // black win   
    func->analysis.mediumPatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = M_LOSE;
    func->analysis.mediumPatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = M_LOSE;
    func->analysis.mediumPatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = M_LOSE;
    func->analysis.mediumPatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][WHITE_PIECE] = M_LOSE;
    func->analysis.mediumPatternTable[WHITE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = M_LOSE;
    func->analysis.mediumPatternTable[INVALID_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = M_LOSE;
    func->analysis.mediumPatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][INVALID_PIECE] = M_LOSE;
    // white win
    func->analysis.mediumPatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = M_WIN;
    func->analysis.mediumPatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = M_WIN;
    func->analysis.mediumPatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = M_WIN;
    func->analysis.mediumPatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][BLACK_PIECE] = M_WIN;
    func->analysis.mediumPatternTable[BLACK_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = M_WIN;
    func->analysis.mediumPatternTable[INVALID_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = M_WIN;
    func->analysis.mediumPatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][INVALID_PIECE] = M_WIN;
    //
    func->analysis.mediumPatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = M_flex3;
    //
    func->analysis.mediumPatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = M_FLEX3;
    //
    func->analysis.mediumPatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE] = M_flex2;
    func->analysis.mediumPatternTable[NONE_PIECE][BLACK_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE] = M_flex2;
    func->analysis.mediumPatternTable[NONE_PIECE][NONE_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = M_flex2;
    //
    func->analysis.mediumPatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE] = M_FLEX2;
    func->analysis.mediumPatternTable[NONE_PIECE][WHITE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE] = M_FLEX2;
    func->analysis.mediumPatternTable[NONE_PIECE][NONE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = M_FLEX2;
    // black flex 1
    func->analysis.mediumPatternTable[NONE_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE] = M_flex1;
    func->analysis.mediumPatternTable[NONE_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE] = M_flex1;
    func->analysis.mediumPatternTable[NONE_PIECE][NONE_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE] = M_flex1;
    // white flex 1
    func->analysis.mediumPatternTable[NONE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE] = M_FLEX1;
    func->analysis.mediumPatternTable[NONE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE] = M_FLEX1;
    func->analysis.mediumPatternTable[NONE_PIECE][NONE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE] = M_FLEX1;


    int p1, p2, p3, p4, p5, x, y, ix, iy;
    for (p1 = Piece::NONE_PIECE; p1 < Piece::PIECE_TYPE_COUNT; ++p1) {
        for (p2 = NONE_PIECE; p2 < PIECE_TYPE_COUNT - 1; ++p2) {
            for (p3 = NONE_PIECE; p3 < PIECE_TYPE_COUNT - 1; ++p3) {
                for (p4 = NONE_PIECE; p4 < PIECE_TYPE_COUNT - 1; ++p4) {
                    for (p5 = NONE_PIECE; p5 < PIECE_TYPE_COUNT - 1; ++p5) {
                        x = y = ix = iy = 0;

                        if (p1 == WHITE_PIECE) { x++; } else if (p1 == BLACK_PIECE) { y++; }

                        if (p2 == WHITE_PIECE) { x++; ix++; } else if (p2 == BLACK_PIECE) { y++; iy++; }

                        if (p3 == WHITE_PIECE) { x++; ix++; } else if (p3 == BLACK_PIECE) { y++; iy++; }

                        if (p4 == WHITE_PIECE) { x++; ix++; } else if (p4 == BLACK_PIECE) { y++; iy++; }

                        if (p5 == WHITE_PIECE) { x++; ix++; } else if (p5 == BLACK_PIECE) { y++; iy++; }


                        if (p1 == INVALID_PIECE || p5 == INVALID_PIECE) {
                            if (p1 == INVALID_PIECE && p5 != INVALID_PIECE) {
                                if (ix == 0 && iy == 3) {
                                    if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                        func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_block3;
                                }
                                if (ix == 3 && iy == 0) {
                                    if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                        func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_BLOCK3;
                                }
                                if (ix == 0 && iy == 2) {
                                    if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                        func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_block2;
                                }
                                if (ix == 2 && iy == 0) {
                                    if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                        func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_BLOCK2;
                                }
                            } else if (p5 == Piece::INVALID_PIECE && p1 != Piece::INVALID_PIECE) {
                                if (x == 0 && y == 3) {
                                    if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                        func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_block3;
                                }
                                if (x == 3 && y == 0) {
                                    if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                        func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_BLOCK3;
                                }
                                if (x == 0 && y == 2) {
                                    if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                        func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_block2;
                                }
                                if (x == 2 && y == 0) {
                                    if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                        func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_BLOCK2;
                                }
                            }
                        } else {
                            if ((x == 0 && y == 3) || (ix == 0 && iy == 3)) {
                                if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                    func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_block3;
                            }
                            if ((x == 3 && y == 0) || (ix == 3 && iy == 0)) {
                                if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                    func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_BLOCK3;
                            }
                            if ((x == 0 && y == 2) || (ix == 0 && iy == 2)) {
                                if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                    func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_block2;
                            }
                            if ((x == 2 && y == 0) || (ix == 2 && iy == 0)) {
                                if (func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] == M_OTHER)
                                    func->analysis.mediumPatternTable[p1][p2][p3][p4][p5] = M_BLOCK2;
                            }
                        }
                    }
                }
            }
        }
    }
}
int Medium_Evaluation(AnalysisFuncInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    int rows = ctx->ui.mnk.board.y;
    int cols = ctx->ui.mnk.board.x;
    int k = ctx->ui.mnk.winningCount;
    // 重みの定義
    int weight[M_ShapeCode::M_SHAPE_COUNT] = { 0, 1000000, -10000000, 50000, -1000000, 400, -1000000, 400, -8000, 20, -50, 20, -50, 1, -3 };
    // 4方向の棋形を記録、該当する場合はカウントを+1
    int shapeRecord[4][M_ShapeCode::M_SHAPE_COUNT] = { { 0 } };
    // 大盤の境界を初期化
    vector<vector<int>> bigBoard(rows + 2, vector<int>(cols + 2));
    for (int i = 0; i < rows + 2; i++) {
        bigBoard.at(0).at(i) = Piece::INVALID_PIECE;
        bigBoard.at(i).at(0) = Piece::INVALID_PIECE;
        bigBoard.at(rows + 1).at(i) = Piece::INVALID_PIECE;
        bigBoard.at(i).at(cols + 1) = Piece::INVALID_PIECE;
    }

    // 現在の盤面をコピー
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++)
            bigBoard.at(y + 1).at(x + 1) = ctx->data.game.chessData[y][x];
    }

    int limit = 3;
    int _limit = limit + 1;
    //    横向きを計算
    for (int y = 1; y <= rows; y++) {
        for (int x = 1; x < cols - limit; x++) {
            int type = func->analysis.mediumPatternTable[bigBoard.at(y).at(x)][bigBoard.at(y).at(x + 1)][bigBoard.at(y).at(x + 2)][bigBoard.at(y).at(x + 3)][bigBoard.at(y).at(x + 4)];
            shapeRecord[0][type]++;
        }
    }
    //    縦向きを計算
    for (int y = 0; y < rows - limit; y++) {
        for (int x = 1; x <= cols; x++) {
            int type = func->analysis.mediumPatternTable[bigBoard.at(y).at(x)][bigBoard.at(y + 1).at(x)][bigBoard.at(y + 2).at(x)][bigBoard.at(y + 3).at(x)][bigBoard.at(y + 4).at(x)];
            shapeRecord[1][type]++;
        }
    }
    //    斜向き（左下から右上）を計算
    for (int y = rows - 1; y > _limit; y--) {
        for (int x = 0; x < cols - limit; x++) {
            int type = func->analysis.mediumPatternTable[bigBoard.at(y).at(x)][bigBoard.at(y - 1).at(x + 1)][bigBoard.at(y - 2).at(x + 2)][bigBoard.at(y - 3).at(x + 3)][bigBoard.at(y - 4).at(x + 4)];
            shapeRecord[2][type]++;
        }
    }
    //    斜向き（左上から右下）を計算
    for (int y = 0; y < rows - limit; y++) {
        for (int x = 0; x < cols - limit; x++) {
            int type = func->analysis.mediumPatternTable[bigBoard.at(y).at(x)][bigBoard.at(y + 1).at(x + 1)][bigBoard.at(y + 2).at(x + 2)][bigBoard.at(y + 3).at(x + 3)][bigBoard.at(y + 4).at(x + 4)];
            shapeRecord[3][type]++;
        }
    }

    int score = 0;
    int count = 0;
    for (int i = 0; i < M_ShapeCode::M_SHAPE_COUNT; i++) {
        score += weight[i] * shapeRecord[0][i] + weight[i] * shapeRecord[1][i] + weight[i] * shapeRecord[2][i] +
            weight[i] * shapeRecord[3][i];
        count = shapeRecord[0][i] + shapeRecord[1][i] + shapeRecord[2][i] + shapeRecord[3][i];
        if (count > 0) {
            string t = "Shape " + to_string(i) + ": " + to_string(count) + " × weight = " + to_string(weight[i]);
            int i = 1;
        }

    }
    return score;
}

/////////////// large AI /////////////// 

void Large_InitPatternTable(AnalysisFuncInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    // black win   
    func->analysis.largePatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = L_LOSE;
    func->analysis.largePatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = L_LOSE;
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = L_LOSE;
    func->analysis.largePatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][WHITE_PIECE] = L_LOSE;
    func->analysis.largePatternTable[WHITE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = L_LOSE;
    func->analysis.largePatternTable[INVALID_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE] = L_LOSE;
    func->analysis.largePatternTable[BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][INVALID_PIECE] = L_LOSE;
    // white win
    func->analysis.largePatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = L_WIN;
    func->analysis.largePatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = L_WIN;
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = L_WIN;
    func->analysis.largePatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][BLACK_PIECE] = L_WIN;
    func->analysis.largePatternTable[BLACK_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = L_WIN;
    func->analysis.largePatternTable[INVALID_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE] = L_WIN;
    func->analysis.largePatternTable[WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][INVALID_PIECE] = L_WIN;
    //
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = L_flex4;
    //
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = L_FLEX4;
    //
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE] = L_flex3;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][BLACK_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = L_flex3;
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][NONE_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = L_flex3;
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE] = L_flex3;
    //
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE] = L_FLEX3;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][WHITE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = L_FLEX3;
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][NONE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = L_FLEX3;
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE] = L_FLEX3;
    //
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE] = L_flex2;
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE] = L_flex2;
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE] = L_flex2;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE] = L_flex2;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE] = L_flex2;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][NONE_PIECE][BLACK_PIECE][BLACK_PIECE][NONE_PIECE] = L_flex2;
    //
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE] = L_FLEX2;
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE] = L_FLEX2;
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE] = L_FLEX2;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE] = L_FLEX2;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE] = L_FLEX2;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][NONE_PIECE][WHITE_PIECE][WHITE_PIECE][NONE_PIECE] = L_FLEX2;
    // black flex 1
    func->analysis.largePatternTable[NONE_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE] = L_flex1;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE] = L_flex1;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE][NONE_PIECE] = L_flex1;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE][BLACK_PIECE][NONE_PIECE] = L_flex1;
    // white flex 1
    func->analysis.largePatternTable[NONE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE] = L_FLEX1;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE] = L_FLEX1;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE][NONE_PIECE] = L_FLEX1;
    func->analysis.largePatternTable[NONE_PIECE][NONE_PIECE][NONE_PIECE][NONE_PIECE][WHITE_PIECE][NONE_PIECE] = L_FLEX1;

    int p1, p2, p3, p4, p5, p6, x, y, ix, iy;
    for (p1 = Piece::NONE_PIECE; p1 < Piece::PIECE_TYPE_COUNT; ++p1) {
        for (p2 = NONE_PIECE; p2 < PIECE_TYPE_COUNT - 1; ++p2) {
            for (p3 = NONE_PIECE; p3 < PIECE_TYPE_COUNT - 1; ++p3) {
                for (p4 = NONE_PIECE; p4 < PIECE_TYPE_COUNT - 1; ++p4) {
                    for (p5 = NONE_PIECE; p5 < PIECE_TYPE_COUNT - 1; ++p5) {
                        for (p6 = NONE_PIECE; p6 < PIECE_TYPE_COUNT; ++p6) {
                            x = y = ix = iy = 0;

                            if (p1 == WHITE_PIECE) { x++; } else if (p1 == BLACK_PIECE) { y++; }

                            if (p2 == WHITE_PIECE) { x++; ix++; } else if (p2 == BLACK_PIECE) { y++; iy++; }

                            if (p3 == WHITE_PIECE) { x++; ix++; } else if (p3 == BLACK_PIECE) { y++; iy++; }

                            if (p4 == WHITE_PIECE) { x++; ix++; } else if (p4 == BLACK_PIECE) { y++; iy++; }

                            if (p5 == WHITE_PIECE) { x++; ix++; } else if (p5 == BLACK_PIECE) { y++; iy++; }

                            if (p6 == WHITE_PIECE) { ix++; } else if (p6 == BLACK_PIECE) { iy++; }

                            if (p1 == INVALID_PIECE || p6 == INVALID_PIECE) {
                                if (p1 == INVALID_PIECE && p6 != INVALID_PIECE) {
                                    if (ix == 0 && iy == 4) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block4;
                                    }
                                    if (ix == 4 && iy == 0) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK4;
                                    }
                                    if (ix == 0 && iy == 3) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block3;
                                    }
                                    if (ix == 3 && iy == 0) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK3;
                                    }
                                    if (ix == 0 && iy == 2) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block2;
                                    }
                                    if (ix == 2 && iy == 0) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK2;
                                    }
                                } else if (p6 == Piece::INVALID_PIECE && p1 != Piece::INVALID_PIECE) {
                                    if (x == 0 && y == 4) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block4;
                                    }
                                    if (x == 4 && y == 0) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK4;
                                    }
                                    if (x == 0 && y == 3) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block3;
                                    }
                                    if (x == 3 && y == 0) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK3;
                                    }
                                    if (x == 0 && y == 2) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block2;
                                    }
                                    if (x == 2 && y == 0) {
                                        if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                            func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK2;
                                    }
                                }
                            } else {
                                if ((x == 0 && y == 4) || (ix == 0 && iy == 4)) {
                                    if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                        func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block4;
                                }
                                if ((x == 4 && y == 0) || (ix == 4 && iy == 0)) {
                                    if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                        func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK4;
                                }
                                if ((x == 0 && y == 3) || (ix == 0 && iy == 3)) {
                                    if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                        func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block3;
                                }
                                if ((x == 3 && y == 0) || (ix == 3 && iy == 0)) {
                                    if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                        func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK3;
                                }
                                if ((x == 0 && y == 2) || (ix == 0 && iy == 2)) {
                                    if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                        func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_block2;
                                }
                                if ((x == 2 && y == 0) || (ix == 2 && iy == 0)) {
                                    if (func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] == L_OTHER)
                                        func->analysis.largePatternTable[p1][p2][p3][p4][p5][p6] = L_BLOCK2;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


int Large_Evaluation(AnalysisFuncInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    int rows = ctx->ui.mnk.board.y;
    int cols = ctx->ui.mnk.board.x;
    int k = ctx->ui.mnk.winningCount;
    // 重みの定義
    int weight[L_ShapeCode::L_SHAPE_COUNT] = { 0, 1000000, -10000000, 50000, -1000000, 400, -1000000, 400, -8000, 20, -50, 20, -50, 1, -3, 1, -3 };
    // 4方向の棋形を記録、該当する場合はカウントを+1
    int shapeRecord[4][L_ShapeCode::L_SHAPE_COUNT] = { { 0 } };
    // 大盤の境界を初期化
    vector<vector<int>> bigBoard(rows + 2, vector<int>(cols + 2));
    for (int i = 0; i < rows + 2; i++) {
        bigBoard.at(0).at(i) = Piece::INVALID_PIECE;
        bigBoard.at(i).at(0) = Piece::INVALID_PIECE;
        bigBoard.at(rows + 1).at(i) = Piece::INVALID_PIECE;
        bigBoard.at(i).at(cols + 1) = Piece::INVALID_PIECE;
    }

    // 現在の盤面をコピー
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++)
            bigBoard.at(y + 1).at(x + 1) = ctx->data.game.chessData[y][x];
    }

    int limit = 3;
    int _limit = limit + 1;
    //    横向きを計算
    for (int y = 1; y <= rows; y++) {
        for (int x = 1; x < cols - limit; x++) {
            int type = func->analysis.largePatternTable[bigBoard.at(y).at(x)][bigBoard.at(y).at(x + 1)][bigBoard.at(y).at(x + 2)][bigBoard.at(y).at(x + 3)][bigBoard.at(y).at(x + 4)][bigBoard.at(y).at(x + 5)];
            shapeRecord[0][type]++;
        }
    }
    //    縦向きを計算
    for (int y = 0; y < rows - limit; y++) {
        for (int x = 1; x <= cols; x++) {
            int type = func->analysis.largePatternTable[bigBoard.at(y).at(x)][bigBoard.at(y + 1).at(x)][bigBoard.at(y + 2).at(x)][bigBoard.at(y + 3).at(x)][bigBoard.at(y + 4).at(x)][bigBoard.at(y + 5).at(x)];
            shapeRecord[1][type]++;
        }
    }
    //    斜向き（左下から右上）を計算
    for (int y = rows - 1; y > _limit; y--) {
        for (int x = 0; x < cols - limit; x++) {
            int type = func->analysis.largePatternTable[bigBoard.at(y).at(x)][bigBoard.at(y - 1).at(x + 1)][bigBoard.at(y - 2).at(x + 2)][bigBoard.at(y - 3).at(x + 3)][bigBoard.at(y - 4).at(x + 4)][bigBoard.at(y - 5).at(x + 5)];
            shapeRecord[2][type]++;
        }
    }
    //    斜向き（左上から右下）を計算
    for (int y = 0; y < rows - limit; y++) {
        for (int x = 0; x < cols - limit; x++) {
            int type = func->analysis.largePatternTable[bigBoard.at(y).at(x)][bigBoard.at(y + 1).at(x + 1)][bigBoard.at(y + 2).at(x + 2)][bigBoard.at(y + 3).at(x + 3)][bigBoard.at(y + 4).at(x + 4)][bigBoard.at(y + 5).at(x + 5)];
            shapeRecord[3][type]++;
        }
    }

    int score = 0;
    int count = 0;
    for (int i = 0; i < L_ShapeCode::L_SHAPE_COUNT; i++) {
        score += weight[i] * shapeRecord[0][i] + weight[i] * shapeRecord[1][i] + weight[i] * shapeRecord[2][i] +
            weight[i] * shapeRecord[3][i];
        count = shapeRecord[0][i] + shapeRecord[1][i] + shapeRecord[2][i] + shapeRecord[3][i];
        if (count > 0) {
            string t = "Shape " + to_string(i) + ": " + to_string(count) + " × weight = " + to_string(weight[i]);
            int i = 1;
        }

    }
    return score;
}



#pragma endregion


