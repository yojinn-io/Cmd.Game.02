#include <stdio.h>
#include <string>
#include <time.h>
#include <math.h>
#include <vector>
#include <climits>
#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <thread>
#include <clocale>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <conio.h>
#include <windows.h>
#pragma comment(linker, "/STACK:8388608")
#pragma execution_character_set("utf-8")
#else
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#endif

// 文字列リテラルをUTF-8として埋め込むことを強制する（MSVCでは /utf-8 オプションでビルドすること）
static_assert(sizeof("あ") == 4, "Source must be compiled with a UTF-8 execution charset (MSVC: /utf-8)");

/*
* ANSI Escape Sequences 参考 =>  https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
* Hexadecimal を使う
* conio.h キーボード
* 
*/

using namespace std;

#pragma region "Platform"
// OS差分をここに集約する。Windows は Console API / conio、それ以外は POSIX termios を使う。
#ifndef _WIN32
static struct termios originalTermios;
static bool termiosSaved = false;

void PlatformRestoreTerminal() {
    if (termiosSaved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &originalTermios);
        termiosSaved = false;
    }
    // カーソル表示と文字色を元に戻す
    fputs("\x1b[?25h\x1b[0m", stdout);
    fflush(stdout);
}
#endif

// コンソールを UTF-8 + ANSIエスケープシーケンス対応にし、キー入力を1文字ずつ読めるようにする
void PlatformInit() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#else
    if (!setlocale(LC_ALL, "C.UTF-8") && !setlocale(LC_ALL, "en_US.UTF-8")) {
        setlocale(LC_ALL, "");
    }
    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &originalTermios) == 0) {
        termiosSaved = true;
        struct termios raw = originalTermios;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
    atexit(PlatformRestoreTerminal);
#endif
    // 1フレーム分の描画をまとめて出力する
    static char outputBuffer[1 << 16];
    setvbuf(stdout, outputBuffer, _IOFBF, sizeof(outputBuffer));
}

#ifndef _WIN32
static bool StdinReady(int timeoutMs) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv = { timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

static int ReadByte() {
    unsigned char c;
    return read(STDIN_FILENO, &c, 1) == 1 ? c : -1;
}
#endif

// キー入力があるかどうか（ブロックしない）
bool KeyHit() {
#ifdef _WIN32
    return _kbhit() != 0;
#else
    return StdinReady(0);
#endif
}

// 1キー読み取る。矢印キーはWindowsの _getch 2バイト目と同じコード（72/80/75/77）、Enterは13に揃える。
int GetKey() {
#ifdef _WIN32
    int key = _getch();
    if (key == 0 || key == 224) {
        key = _getch();
    }
    return key;
#else
    int key = ReadByte();
    if (key == 27) {
        // ESC単体か、矢印キーのエスケープシーケンス（ESC [ A など）かを判別する
        if (!StdinReady(20)) return 27;
        int next = ReadByte();
        if ((next == '[' || next == 'O') && StdinReady(20)) {
            switch (ReadByte()) {
                case 'A': return 72;
                case 'B': return 80;
                case 'C': return 77;
                case 'D': return 75;
                default: return -1;
            }
        }
        return -1;
    }
    if (key == '\n') return 13;
    return key;
#endif
}

void SleepMs(int ms) {
    this_thread::sleep_for(chrono::milliseconds(ms));
}

// 経過時間（ミリ秒、壁時計）
double NowMs() {
    return chrono::duration<double, milli>(chrono::steady_clock::now().time_since_epoch()).count();
}

// UTF-8文字列を1文字（コードポイント）ずつに分割する
vector<string> SplitUtf8(const string& str) {
    vector<string> chars;
    for (size_t i = 0; i < str.size();) {
        unsigned char c = str[i];
        size_t len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
        if (i + len > str.size()) len = str.size() - i;
        chars.push_back(str.substr(i, len));
        i += len;
    }
    return chars;
}
#pragma endregion

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
    string rStr = "";
    for (const string& c : SplitUtf8(str)) {
        rStr += c;
    }
    return rStr;
}

// 1文字（UTF-8）が端末上で2マス分の幅を取る全角文字かどうか（East Asian Wide / Fullwidth）
bool IsWideChar(const string& ch) {
    if (ch.size() < 3) return false;
    unsigned int cp = (unsigned char)ch[0] & (ch.size() == 3 ? 0x0F : 0x07);
    for (size_t i = 1; i < ch.size(); i++) {
        cp = (cp << 6) | ((unsigned char)ch[i] & 0x3F);
    }
    return (cp >= 0x1100 && cp <= 0x115F) || (cp >= 0x2E80 && cp <= 0x303E) || (cp >= 0x3041 && cp <= 0x33FF) ||
        (cp >= 0x3400 && cp <= 0x4DBF) || (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xA000 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) || (cp >= 0xF900 && cp <= 0xFAFF) || (cp >= 0xFE30 && cp <= 0xFE4F) ||
        (cp >= 0xFF00 && cp <= 0xFF60) || (cp >= 0xFFE0 && cp <= 0xFFE6) || (cp >= 0x1F300 && cp <= 0x1F64F) ||
        (cp >= 0x1F900 && cp <= 0x1F9FF) || (cp >= 0x20000 && cp <= 0x3FFFD);
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
    fputs(cmd.c_str(), stdout);
}

void OutPrint(string str) {
    printf("%s", str.c_str());
}

void OutPrintln(string str) {
    printf("%s\n", str.c_str());
}

void ConsoleCursorVisible(bool flag0) {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info = { 100, flag0 };// カーソルを非表示にする
    SetConsoleCursorInfo(hOut, &info);
#endif
    RunCmd(flag0 ? "\x1b[?25h" : "\x1b[?25l");
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
    // 画面全体を消去してカーソルを先頭に戻す。
    // 幅ぴったりの空白行を出力する方法だと、端末によっては自動改行が入って画面がスクロールする。
    RunCmd("\x1b[2J\x1b[H");
}

void ConsoleTitle(string title) {
#ifdef _WIN32
    // UTF-8 のタイトルを UTF-16 に変換して設定する（SetConsoleTitleA だとシステムのコードページで解釈されて文字化けする）
    int len = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, NULL, 0);
    wstring wide(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wide[0], len);
    SetConsoleTitleW(wide.c_str());
#else
    RunCmd("\x1b]0;" + title + "\x07");
#endif
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
    double startTime; // ミリ秒
    double endTime;   // ミリ秒
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
    int count; // 有効な候補点の数（AN以下）
};

struct Decision {
    Vector2 pos;
    int score;
};



typedef struct AIAnalysis {
    // 長さkの窓に含まれる石の数n（0〜k）ごとの重み
    vector<long long> lineWeight;

    // AI 計算したの結果
    Decision decision;


} AIAnalysis;

typedef struct AnalysisFuncInterface {
    void (*Init)(AnalysisFuncInterface*);
    int (*Evaluation)(AnalysisFuncInterface*, const vector<vector<Piece>>&);

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
    
    ConsoleTitle(title);
    return session;
}

bool CmdGameProcessMessage(GameSession* gs) {
    gs->sys.val.frame++;
    return gs->sys.running;
}

void CmdGameBeginFrame(GameSession* gs) {
    gs->sys.val.startTime = NowMs();
}

void CmdGameEndFrame(GameSession* gs) {
    fflush(stdout);
    static const double launchTime = NowMs();
    gs->sys.val.endTime = NowMs();
    double elapsedSec = (gs->sys.val.endTime - launchTime) / 1000.0;
    gs->sys.val.fps = elapsedSec > 0 ? (int)(gs->sys.val.frame / elapsedSec) : 0;
    double frameTime = gs->sys.val.endTime - gs->sys.val.startTime;
    if (frameTime < gs->sys.setting.frameDurationMS) {
        gs->sys.val.gameDelay = gs->sys.setting.frameDurationMS - frameTime;
        SleepMs((int)gs->sys.val.gameDelay);
    }
}

void UnregisterEventHandler(EventType type, EventHandler handler);
void HandleSceneChange(Event* e);

void CmdGameFinalize(GameSession* gs) {
    // 現在のシーンのローカルコンテキストは session を参照しているので、session より先に解放する
    if (getCurrentScene && getCurrentScene->Shutdown) {
        getCurrentScene->Shutdown(getCurrentScene);
    }
    UnregisterEventHandler(EventType::SYS_SCENE_CHANGE, HandleSceneChange);
    ConsoleCursorVisible(true);
    fflush(stdout);

    // func・シーン・イベントハンドラ表は静的に確保したグローバル変数なので delete しない
    delete gs;
    func = NULL;
    getCurrentScene = NULL;
    getNextScene = NULL;
}
#pragma endregion

#pragma region "描画"
// 画面バッファの全マスを空白にする。
// 空文字列のマスは「全角文字の右半分」を表し、描画時には何も出力しない。
void ClearScreenBuffer(GameLocalContext* ctx) {
    for (int x = 0; x < CONSOLE_WIDTH - 1; x++) {
        for (int y = 0; y < CONSOLE_HEIGHT; y++) {
            ctx->ui.screenBuffer[x][y] = string(1, SPACE);
        }
    }
}

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


    // 次のフレームに前の描画内容が残らないよう、バッファ全体をクリアする
    ClearScreenBuffer(ctx);

    ctx->ui.screenGraphic.clear(); // Clear the screen graphic buffer
}

void WriteToBuffer(int xPos, int yPos, string characters, GameLocalContext* ctx) {
    int x = xPos;
    int y = yPos;
    int screenWidth = ctx->session->sys.setting.screenWidth;
    int screenHeight = ctx->session->sys.setting.screenHeight;
    vector<string> chars = SplitUtf8(characters);
    for (int i = 0; i < chars.size(); i++) {
        const string& charStr = chars[i];
        if (charStr == string(1, NEWLINE)) {
            y++;
            x = xPos;
            continue;
        }
        // 全角文字は2マス使う（右半分のマスは空文字列にしておく）
        int width = IsWideChar(charStr) ? 2 : 1;
        if (x < 0 || y < 0 || x + width > screenWidth || y >= screenHeight) {
            x += width;
            continue;
        }
        // 既存の全角文字の片側だけを上書きする場合は、残った半分を空白に戻す
        if (ctx->ui.screenBuffer[x][y].empty() && x > 0) {
            ctx->ui.screenBuffer[x - 1][y] = string(1, SPACE);
        }
        if (x + width < screenWidth && ctx->ui.screenBuffer[x + width][y].empty()) {
            ctx->ui.screenBuffer[x + width][y] = string(1, SPACE);
        }
        ctx->ui.screenBuffer[x][y] = charStr;
        if (width == 2) {
            ctx->ui.screenBuffer[x + 1][y] = "";
        }
        x += width;
    }
}

void RenderToScreenWithColour(int xPos, int yPos, ConsoleColor colour, string characters, GameLocalContext* ctx) {
    int x = xPos;
    int y = yPos;
    ConsoleSetForegroundColor(colour);
    vector<string> chars = SplitUtf8(characters);

    for (int i = 0; i < chars.size(); i++) {
        const string& charStr = chars[i];

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

// 駒の色（BP = 先手、WP = 後手）
const ConsoleColor BP_COLOR = RED;
const ConsoleColor WP_COLOR = CYAN;

// 画面バッファの1マスに色付きの駒を書き込む。
// エスケープシーケンスは1マス分の文字列に含めるので、表示幅（レイアウト）は変わらない。
void WriteColoredPiece(int x, int y, Piece piece, GameLocalContext* ctx) {
    if (piece != BLACK_PIECE && piece != WHITE_PIECE) return;
    if (x < 0 || y < 0 || x >= ctx->session->sys.setting.screenWidth || y >= ctx->session->sys.setting.screenHeight) return;
    ConsoleColor color = piece == BLACK_PIECE ? BP_COLOR : WP_COLOR;
    string text = piece == BLACK_PIECE ? BP : WP;
    ctx->ui.screenBuffer[x][y] = "\x1b[" + to_string(BOLD) + ";" + to_string(color) + "m" + text + "\x1b[22;39m";
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

// イベントはハンドラが同期的に処理するので、スタック上に作れば十分（new すると毎回リークする）。
// ハンドラは Event や data のポインタを呼び出し後まで保持してはいけない。
void DispatchEvent(EventType type, void* data) {
    Event event = { type, data };
    DispatchEvent(&event);
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

// p->position の石を通る (dx, dy) 方向のラインで、同じ石が winningCount 個以上連続しているかを判定する
bool DetectBoardLine(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE], int dx, int dy) {
    int winCount = ctx->ui.mnk.winningCount;
    int row = ctx->ui.mnk.board.y;
    int col = ctx->ui.mnk.board.x;
    int px = p->position.x;
    int py = p->position.y;
    if (chessData[py][px] != p->marker) {
        return false;
    }

    int count = 1;
    // 正方向と逆方向に、盤面の端か別の石に当たるまで数える
    for (int s = -1; s <= 1; s += 2) {
        int x = px + dx * s;
        int y = py + dy * s;
        while (x >= 0 && x < col && y >= 0 && y < row && chessData[y][x] == p->marker) {
            count++;
            x += dx * s;
            y += dy * s;
        }
    }
    return count >= winCount;
}

bool DetectBoardHorizontal(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
    return DetectBoardLine(ctx, p, chessData, 1, 0);
}

bool DetectBoardVertical(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
    return DetectBoardLine(ctx, p, chessData, 0, 1);
}

// 左下から右上
bool DetectBoardDiagonalRight(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
    return DetectBoardLine(ctx, p, chessData, 1, -1);
}

// 左上から右下
bool DetectBoardDiagonalLeft(GameLocalContext* ctx, Player* p, Piece chessData[MAX_SIZE][MAX_SIZE]) {
    return DetectBoardLine(ctx, p, chessData, 1, 1);
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

// 引き分けかどうか：どちらのプレイヤーも、もう勝利条件を満たせない場合に true を返す。
// （盤面が埋まった場合も含む。長さ winningCount の窓に片方の石しかなければ、まだ勝てる可能性がある）
bool DetectDraw(GameLocalContext* ctx) {
    int winCount = ctx->ui.mnk.winningCount;
    int row = ctx->ui.mnk.board.y;
    int col = ctx->ui.mnk.board.x;
    const int dirs[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };

    for (int d = 0; d < 4; d++) {
        for (int y = 0; y < row; y++) {
            for (int x = 0; x < col; x++) {
                int ex = x + dirs[d][0] * (winCount - 1);
                int ey = y + dirs[d][1] * (winCount - 1);
                if (ex < 0 || ex >= col || ey < 0 || ey >= row) continue;

                bool hasBlack = false, hasWhite = false;
                for (int i = 0; i < winCount; i++) {
                    Piece piece = ctx->data.game.chessData[y + dirs[d][1] * i][x + dirs[d][0] * i];
                    hasBlack |= piece == Piece::BLACK_PIECE;
                    hasWhite |= piece == Piece::WHITE_PIECE;
                }
                if (!hasBlack || !hasWhite) {
                    return false;
                }
            }
        }
    }

    // 特別勝利（四隅）がまだ可能か
    if (ctx->session->config.specialVictory) {
        Piece corners[4] = {
            ctx->data.game.chessData[0][0], ctx->data.game.chessData[0][col - 1],
            ctx->data.game.chessData[row - 1][0], ctx->data.game.chessData[row - 1][col - 1],
        };
        bool hasBlack = false, hasWhite = false;
        for (Piece c : corners) {
            hasBlack |= c == Piece::BLACK_PIECE;
            hasWhite |= c == Piece::WHITE_PIECE;
        }
        if (!hasBlack || !hasWhite) {
            return false;
        }
    }
    return true;
}

#pragma endregion

#pragma region "AI.アルゴリズム.設定"

void MNK_Init(AnalysisFuncInterface* self);
int MNK_Evaluation(AnalysisFuncInterface* self, const vector<vector<Piece>>& board);

// 盤面サイズ・勝利数に依存しない汎用MNKゲームAI
AnalysisFuncInterface MNKShapeAI = {
    MNK_Init,
    MNK_Evaluation,
};

int Minimax(vector<vector<Piece>>& board, int depth, int alpha, int beta, bool isMax, GameLocalContext* ctx);

#pragma endregion

int main() {
    PlatformInit();
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
    ClearScreenBuffer(ctx);

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
    if (KeyHit()) {
        int key = GetKey();
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
                    // exit() だと現在のシーンの Shutdown と CmdGameFinalize が呼ばれないので、メインループを抜ける
                    ctx->session->sys.running = false;
                    return;
            }
            DispatchEvent(SYS_SCENE_CHANGE, &data);
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
    if (KeyHit()) {
        int key = GetKey();
        if (key == W_KEY || key == UP_ARROW) {
            ctx->data.config.selected = ((ctx->data.config.selected - 1 < 0) ? ctx->data.config.itemSize  : (ctx->data.config.selected )) - 1 % ctx->data.config.itemSize;
        } else if (key == S_KEY || key == DOWN_ARROW) {
            ctx->data.config.selected = (ctx->data.config.selected + 1) % ctx->data.config.itemSize;
        } else if (key == ESCAPE_KEY) {
            static SceneChangeData data;
            extern SceneInterface MenuScene;
            data.payload = &MenuScene;
            DispatchEvent(SYS_SCENE_CHANGE, &data);
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
                DispatchEvent(SYS_SCENE_CHANGE, &data);
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
    if (KeyHit()) {
        int key = GetKey();
        if (key == ENTER_KEY || key == ESCAPE_KEY) {
            static SceneChangeData data;
            extern SceneInterface MenuScene;
            data.payload = &MenuScene;
            DispatchEvent(SYS_SCENE_CHANGE, &data);
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

    // 駒だけ色を付けて上書きする（マス(x, y)の駒は罫線を含めて (4x + 2, 2y + 1) の位置にある）
    for (int y = 0; y < ctx->ui.mnk.board.y; y++) {
        for (int x = 0; x < ctx->ui.mnk.board.x; x++) {
            WriteColoredPiece(ctx->ui.offset.x + 4 * x + 2, ctx->ui.offset.y + 2 * y + 1, ctx->data.game.chessData[y][x], ctx);
        }
    }
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
    WriteColoredPiece(startX + titleKey.length(), startY, ctx->data.game.masterPlayer.marker, ctx);
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
    WriteColoredPiece(startX + titleKey.length(), startY, ctx->data.game.slavePlayer.marker, ctx);
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
    WriteColoredPiece(sx + 2, sy + 1, value, ctx);
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
            } else if (DetectDraw(ctx)) {
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
    Minimax(board, DEPTH, INT_MIN, INT_MAX, true, ctx);
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
            break;
        case MEDIUM:
            ctx->ui.mnk = { { 7, 7 }, 4 };
            break;
        case LARGE:
            ctx->ui.mnk = { { MAX_SIZE, MAX_SIZE }, 5 };
            break;
    }
    func = &MNKShapeAI;

    if (func) {
        func->localContext = ctx;
        func->Init(func);
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
    if (KeyHit()) {
        int key = GetKey();
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
            DispatchEvent(SYS_SCENE_CHANGE, &data);
        } else if (key == SPACE) {
            ctx->ui.selected = true;
        } else if (key == R_KEY) {
            static SceneChangeData data;
            extern SceneInterface GameScene;
            data.payload = &GameScene;
            DispatchEvent(SYS_SCENE_CHANGE, &data);
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
    // AIはゲームシーンのコンテキストを参照しているので、解放後に使われないよう切り離す
    if (func && func->localContext == self->localContext) {
        func->localContext = NULL;
    }
    DefaultShutdownImpl(self);
}
#pragma endregion

#pragma region "AI.アルゴリズム.実現"

Points CollectHotZones(vector<vector<Piece>>& board, GameLocalContext* ctx) {
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
    bool hasPiece = false;

    // 局所探索では、既に置かれた石の周囲8方向・3マス以内を対象に、一致する位置を探す。
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (board.at(y).at(x) != Piece::NONE_PIECE) {
                hasPiece = true;
                for (int k = -3; k <= 3; k++) {
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
    // 盤面に石が一つもない場合は中央を候補にする
    if (!hasPiece) {
        localBoard.at(rows / 2).at(cols / 2) = true;
    }

    // 局所的な探索のあとで手を打つシミュレーションを行い、その結果をヒューリスティックに評価する。
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (localBoard.at(y).at(x)) {
                board.at(y).at(x) = slave.marker;
                weight.at(y).at(x) = func->Evaluation(func, board);
                board.at(y).at(x) = Piece::NONE_PIECE;
            }
        }
    }

    // 評価値の高い順に最大AN個の候補点を取り出す。候補が足りなければそこで打ち切る。
    bestPoint.count = 0;
    for (int k = 0; k < AN; k++) {
        int w = INT_MIN;
        Vector2 p = { -1, -1 };
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                if (localBoard.at(y).at(x) && (p.x < 0 || weight.at(y).at(x) > w)) {
                    w = weight.at(y).at(x);
                    p.x = x, p.y = y;
                }
            }
        }
        if (p.x < 0) break; // 候補点がもう残っていない
        bestPoint.pos[k] = p, bestPoint.score[k] = w;
        bestPoint.count++;
        localBoard.at(p.y).at(p.x) = false; //最大値を除外して、次に大きいポイントを見つける。
    }

    return bestPoint;
}

void ReverseBoard(const vector<vector<Piece>>& board, vector<vector<Piece>>& rboard) {
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

// (x, y) に置いた石で勝利条件（k個並び、または特別勝利）を満たしたかどうか
bool IsWinningMove(const vector<vector<Piece>>& board, int x, int y, GameLocalContext* ctx) {
    int rows = board.size();
    int cols = board.at(0).size();
    int k = ctx->ui.mnk.winningCount;
    Piece marker = board.at(y).at(x);
    const int dirs[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };

    for (int d = 0; d < 4; d++) {
        int count = 1;
        for (int s = -1; s <= 1; s += 2) {
            int nx = x + dirs[d][0] * s;
            int ny = y + dirs[d][1] * s;
            while (nx >= 0 && nx < cols && ny >= 0 && ny < rows && board.at(ny).at(nx) == marker) {
                count++;
                nx += dirs[d][0] * s;
                ny += dirs[d][1] * s;
            }
        }
        if (count >= k) return true;
    }

    if (ctx->session->config.specialVictory) {
        if (board.at(0).at(0) == marker && board.at(0).at(cols - 1) == marker &&
            board.at(rows - 1).at(0) == marker && board.at(rows - 1).at(cols - 1) == marker) {
            return true;
        }
    }
    return false;
}

const int WIN_SCORE = INT_MAX / 2;

int Minimax(vector<vector<Piece>>& board, int depth, int alpha, int beta, bool isMax, GameLocalContext* ctx) {
    // 葉ノードでは現在の盤面をそのまま評価する
    if (depth == 0) {
        return func->Evaluation(func, board);
    }

    Points p;
    Piece marker;
    if (isMax) {
        p = CollectHotZones(board, ctx);
        marker = ctx->data.game.slavePlayer.marker;
    } else {
        vector<vector<Piece>> rBoard(board.size(), vector<Piece>(board.at(0).size()));
        // 転回する
        ReverseBoard(board, rBoard);
        // 敵の最優なポイント
        p = CollectHotZones(rBoard, ctx);
        marker = ctx->data.game.masterPlayer.marker;
    }

    // 置ける場所がない = 盤面が埋まって引き分け
    if (p.count == 0) {
        return 0;
    }

    // ルートでは、全候補が同点でも有効な手を返せるように最初の候補を入れておく
    if (depth == DEPTH) {
        func->analysis.decision.pos = p.pos[0];
        func->analysis.decision.score = p.score[0];
    }

    for (int i = 0; i < p.count; i++) {
        int x = p.pos[i].x;
        int y = p.pos[i].y;
        board.at(y).at(x) = marker;
        int a;
        if (IsWinningMove(board, x, y, ctx)) {
            // 早く勝つ手ほど高く、遅く負ける手ほどましに評価する
            a = isMax ? WIN_SCORE + depth : -WIN_SCORE - depth;
        } else {
            a = Minimax(board, depth - 1, alpha, beta, !isMax, ctx);
        }
        board.at(y).at(x) = Piece::NONE_PIECE;

        if (isMax) {
            if (a > alpha) {
                alpha = a;
                // もし、最大のalphaをさがしたら
                if (depth == DEPTH) {
                    func->analysis.decision.pos = p.pos[i];
                    func->analysis.decision.score = p.score[i];
                }
            }
        } else if (a < beta) {
            beta = a;
        }
        if (beta <= alpha) break; //枝刈りし
    }
    return isMax ? alpha : beta;
}



/////////////// MNK AI /////////////// 
// 隣の窓に石が1つ増えるごとに重みを何倍にするか
const long long LINE_WEIGHT_BASE = 10;
// 相手のラインを自分のラインの何倍危険とみなすか（守備寄りにする）
const long long DEFENSE_RATE = 2;

void MNK_Init(AnalysisFuncInterface* self) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    int k = ctx->ui.mnk.winningCount;
    self->analysis.lineWeight.assign(k + 1, 0);
    long long w = 1;
    for (int n = 1; n < k; n++) {
        self->analysis.lineWeight[n] = w;
        w *= LINE_WEIGHT_BASE;
    }
    // k個そろった窓は勝利なので、他のどの組み合わせよりも大きくする
    self->analysis.lineWeight[k] = WIN_SCORE / 4;
}

// MNKゲーム共通の評価関数（AI = slavePlayer 視点）。
// 盤面上の長さkの窓をすべて走査し、片方の石しか含まない窓だけを「まだ勝ちに使えるライン」として数える。
// 両端が空いたラインはより多くの窓に含まれるので、活・死の区別は窓の数として自然に反映される。
int MNK_Evaluation(AnalysisFuncInterface* self, const vector<vector<Piece>>& board) {
    GameLocalContext* ctx = (GameLocalContext*)(self->localContext);
    int rows = board.size();
    int cols = board.at(0).size();
    int k = ctx->ui.mnk.winningCount;
    Piece own = ctx->data.game.slavePlayer.marker;
    const vector<long long>& weight = self->analysis.lineWeight;
    const int dirs[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };

    long long score = 0;
    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0];
        int dy = dirs[d][1];
        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                int ex = x + dx * (k - 1);
                int ey = y + dy * (k - 1);
                if (ex < 0 || ex >= cols || ey < 0 || ey >= rows) continue;

                int mine = 0, theirs = 0;
                for (int i = 0; i < k; i++) {
                    Piece p = board.at(y + dy * i).at(x + dx * i);
                    if (p == own) {
                        mine++;
                    } else if (p != Piece::NONE_PIECE) {
                        theirs++;
                    }
                }
                if (mine > 0 && theirs == 0) {
                    score += weight[mine];
                } else if (theirs > 0 && mine == 0) {
                    score -= weight[theirs] * DEFENSE_RATE;
                }
            }
        }
    }

    // 勝敗が確定した局面（Minimax の WIN_SCORE）より大きくならないようにする
    long long limit = WIN_SCORE / 2;
    return (int)max(-limit, min(limit, score));
}

#pragma endregion


