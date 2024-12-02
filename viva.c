/* 단축키 정의 */
#ifdef _WIN32
#define SAVE_KEY    "Ctrl+S"
#define QUIT_KEY    "Ctrl+Q"
#define FIND_KEY    "Ctrl+F"
#elif defined(__APPLE__)
#define SAVE_KEY    "ESC+S"
#define QUIT_KEY    "ESC+Q"
#define FIND_KEY    "ESC+F"
#else
#define SAVE_KEY    "Ctrl+S"
#define QUIT_KEY    "Ctrl+Q"
#define FIND_KEY    "Ctrl+F"
#endif

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* 구조체 정의 */
typedef struct Node {       // 노드 구조체
    char character;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct TextBuffer { // 텍스트 버퍼 구조체
    Node *head;
    Node *tail;
    int modified;
    char *filename;
} TextBuffer;

typedef struct Cursor {     // 커서 구조체
    Node *current;
    int row;
    int col;
} Cursor;

typedef struct SearchContext {    // 탐색된 개체 구조체
    char query[256];
    Node **results;         // 찾은 노드들의 주소를 저장(포인터)
    int result_count;
    int current_index;
    Cursor original_cursor;     // 원래 커서를 복사
} SearchContext;

/* 노드 생성 함수 */
Node* createNode(char c) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->character = c;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

/* 노드 삽입 함수 */
void insertNode(TextBuffer *tb, Cursor *cursor, char c) {
    Node *newNode = createNode(c);
    tb->modified = 1; // 수정됨 표시

    if (tb->head == NULL) {
        // 빈 리스트일 경우
        tb->head = newNode;
        tb->tail = newNode;
        cursor->current = newNode;
    } else if (cursor->current == NULL) {
        // 텍스트 시작 부분에 삽입
        newNode->next = tb->head;
        tb->head->prev = newNode;
        tb->head = newNode;
        cursor->current = newNode;
    } else {
        // 현재 커서 위치 뒤에 삽입
        newNode->prev = cursor->current;
        newNode->next = cursor->current->next;
        if (cursor->current->next != NULL) {
            cursor->current->next->prev = newNode;
        } 
        else {  // 마지막 노드인 경우
            tb->tail = newNode;
        }
        cursor->current->next = newNode;
        cursor->current = newNode;
    }
}

/* 노드 삭제 함수 */
void deleteNode(TextBuffer *tb, Cursor *cursor) {
    if (cursor->current == NULL) {
        return;
    }
    tb->modified = 1; // 수정됨 표시
    Node *dNode = cursor->current;

    if (dNode->prev != NULL) {
        dNode->prev->next = dNode->next;
        cursor->current = dNode->prev;
    } else {
        // 헤드 노드 삭제
        tb->head = dNode->next;
        cursor->current = tb->head;
    }

    if (dNode->next != NULL) {
        dNode->next->prev = dNode->prev;
    } else {
        // 테일 노드 삭제
        tb->tail = dNode->prev;
    }

    free(dNode);
}

/* 라인 수 계산 함수 */
int countLines(TextBuffer *tb) {
    int lines = 1;
    Node *temp = tb->head;
    while (temp != NULL) {
        if (temp->character == '\n') {
            lines++;
        }
        temp = temp->next;
    }
    return lines;
}

/* 상태 바 표시 함수 */
void displayStatusBar(WINDOW *win, TextBuffer *tb, Cursor *cursor) {
    int status_bar = LINES - 2;
    char status[COLS];
    int total_lines = countLines(tb);
    snprintf(status, COLS, " [%s] - %d lines | Cursor: (%d:%d) ", 
        tb->filename ? tb->filename : "No Name", total_lines, cursor->row + 1, cursor->col + 1);
    // 반전 효과
    wattron(win, A_REVERSE);
    mvwprintw(win, status_bar, 0, "%-*s", COLS - 1, status);
    wattroff(win, A_REVERSE);
    wrefresh(win);
}

/* 메시지 바 표시 함수 */
void displayMessageBar(WINDOW *win) {
    int message_bar = LINES - 1;
    char message[COLS];
    snprintf(message, COLS, "HELP: %s = save | %s = quit | %s = find",
        SAVE_KEY, QUIT_KEY, FIND_KEY);
    mvwprintw(win, message_bar, 0, "%-*s", COLS - 1, message);
    wrefresh(win);
}

/* 텍스트 버퍼를 화면에 표시하는 함수 */
void displayList(WINDOW *win, TextBuffer *tb, Cursor *cursor) {
    wclear(win);
    Node *temp = tb->head;
    int x = 0, y = 0;

    while (temp != NULL) {
        if (temp->character == '\n') {
            x = 0;
            y++;
        } else {
            mvwaddch(win, y, x, temp->character);
            x++;
            if (x >= COLS) {
                x = 0;
                y++;
            }
        }
        if (temp == cursor->current) {
            wmove(win, y, x);
            cursor->row = y;
            cursor->col = x;
        }
        temp = temp->next;
        if (y >= LINES - 4) {
            // 화면의 표시 가능한 영역을 초과하면 더 이상 출력하지 않음
            break;
        }
    }
    wrefresh(win);
    displayStatusBar(win, tb, cursor);
    displayMessageBar(win);
}

/* 파일 로드 함수 */
void loadFile(TextBuffer *tb, Cursor *cursor, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        int ch;
        while ((ch = fgetc(file)) != EOF) {
            insertNode(tb, cursor, (char)ch);
        }
        fclose(file);
        tb->modified = 0;
        tb->filename = strdup(filename);
        cursor->current = tb->tail;
    } else {
        tb->filename = strdup(filename);
    }
}

/* 파일 저장 함수 */
void saveFile(TextBuffer *tb) {
    if (tb->filename) {
        FILE *file = fopen(tb->filename, "w");
        if (file) {
            Node *temp = tb->head;
            while (temp != NULL) {
                fputc(temp->character, file);
                temp = temp->next;
            }
            fclose(file);
            tb->modified = 0; // 저장 후 수정되지 않음으로 표시
        }
    }
}

/* 왼쪽 커서 이동 */
void moveCursorLeft(Cursor *cursor) {
    if (cursor->current != NULL && cursor->current->prev != NULL) {
        cursor->current = cursor->current->prev;
        if (cursor->current->character == '\n') {
            // 이전 줄의 끝으로 이동
            cursor->current = cursor->current->prev; // '\n' 이전 문자로 이동
            cursor->row--;
            cursor->col = 0;
            Node *temp = cursor->current;
            while (temp != NULL && temp->character != '\n') {
                temp = temp->prev;
                cursor->col++;
            }
        } else {
            cursor->col--;
        }
    }
}

/* 오른쪽 커서 이동 */
void moveCursorRight(Cursor *cursor) {
    if (cursor->current != NULL && cursor->current->next != NULL) {
        cursor->current = cursor->current->next;
        if (cursor->current->character == '\n') {
            cursor->row++;
            cursor->col = 0;
        } else {
            cursor->col++;
        }
    }
}

/* 위쪽 커서 이동 */
void moveCursorUp(Cursor *cursor) {
    if (cursor->row > 0) {
        int selected_col = cursor->col;
        // 현재 줄의 시작으로 이동
        while (cursor->current->prev != NULL && cursor->current->character != '\n') {
            cursor->current = cursor->current->prev;
        }
        if (cursor->current->prev != NULL) {
            cursor->current = cursor->current->prev; // 이전 줄의 끝의 '\n'로 이동
            cursor->row--;
            cursor->col = 0;
            // 이전 줄의 시작으로 이동
            while (cursor->current->prev != NULL && cursor->current->character != '\n') {
                cursor->current = cursor->current->prev;
            }
            if (cursor->current->character == '\n' && cursor->current->next != NULL) {
                cursor->current = cursor->current->next;
            }
            // 원하는 열로 이동
            while (cursor->col < selected_col && cursor->current->next != NULL && cursor->current->character != '\n') {
                cursor->current = cursor->current->next;
                cursor->col++;
            }
        }
    }
}

/* 아래쪽 커서 이동 */
void moveCursorDown(Cursor *cursor) {
    Node *temp = cursor->current;
    int selected_col = cursor->col;
    // 현재 줄의 끝으로 이동
    while (temp != NULL && temp->character != '\n') {
        temp = temp->next;
    }
    if (temp != NULL && temp->next != NULL) {
        cursor->current = temp->next; // 다음 줄의 시작으로 이동
        cursor->row++;
        cursor->col = 0;
        // 원하는 열로 이동
        while (cursor->col < selected_col && cursor->current->next != NULL && cursor->current->character != '\n') {
            cursor->current = cursor->current->next;
            cursor->col++;
        }
    }
}

/* 검색 기능 흐름 처리 */
void searchFunction(WINDOW *win, TextBuffer *tb, Cursor *cursor) {
    SearchContext sc;
    sc.result_count = 0;
    sc.current_index = 0;
    sc.results = NULL;
    sc.original_cursor = *cursor; // 검색 이전의 커서 위치 저장

    echo(); // 입력 에코 활성화
    curs_set(1); // 커서 표시

    // 사용자로부터 검색어 입력 받기
    displayPrompt(win, "Search: ", sc.query, sizeof(sc.query));

    noecho(); // 입력 에코 비활성화
    curs_set(0); // 커서 숨김

    if (strlen(sc.query) == 0) {
        // 검색어가 비어있으면 검색 취소
        return;
    }

    // 검색 결과 찾기
    findMatches(tb, &sc);

    if (sc.result_count == 0) {
        // 검색 결과가 없을 경우 메시지 표시
        mvwprintw(win, tb->lines + 1, 0, "No matches found.");
        wrefresh(win);
        getch(); // 사용자 입력 대기
        return;
    }

    // 첫 번째 검색 결과 하이라이트
    highlightMatch(win, tb, &sc);

    int ch;
    while (1) {
        ch = getch();
        if (ch == KEY_LEFT) {
            // 이전 검색 결과로 이동
            clearHighlight(win, tb, &sc);
            sc.current_index = (sc.current_index - 1 + sc.result_count) % sc.result_count;
            highlightMatch(win, tb, &sc);
        } else if (ch == KEY_RIGHT) {
            // 다음 검색 결과로 이동
            clearHighlight(win, tb, &sc);
            sc.current_index = (sc.current_index + 1) % sc.result_count;
            highlightMatch(win, tb, &sc);
        } else if (ch == '\n' || ch == '\r') {
            // Enter 키 눌렀을 때 검색 종료 및 편집 시작
            *cursor = getCursorFromNode(tb, sc.results[sc.current_index]);
            break;
        } else if (ch == 27) {
            // ESC 키 눌렀을 때 검색 취소 및 커서 복원
            clearHighlight(win, tb, &sc);
            *cursor = sc.original_cursor;
            break;
        }
    }

    // 메모리 해제
    free(sc.results);

    // 화면 갱신
    displayList(win, tb, cursor);
    move(cursor->row, cursor->col);
    refresh();
}

/* 검색 위치 저장 함수 */
void findMatches(TextBuffer *tb, SearchContext *sc) {
    Node *current = tb->head;
    int query_len = strlen(sc->query);
    sc->result_count = 0;

    // 첫 번째 패스: 검색 결과 개수 세기
    while (current != NULL) {
        if (current->character == sc->query[0]) {
            Node *temp = current;
            int i = 0;
            while (temp != NULL && sc->query[i] != '\0' && temp->character == sc->query[i]) {
                temp = temp->next;
                i++;
            }
            if (sc->query[i] == '\0') {
                sc->result_count++;
            }
        }
        current = current->next;
    }

    if (sc->result_count == 0) {
        sc->results = NULL;
        return;
    }

    // 메모리 할당
    sc->results = (Node **)malloc(sizeof(Node *) * sc->result_count);

    // 두 번째 패스: 검색 결과 저장
    current = tb->head;
    int index = 0;
    while (current != NULL) {
        if (current->character == sc->query[0]) {
            Node *temp = current;
            int i = 0;
            while (temp != NULL && sc->query[i] != '\0' && temp->character == sc->query[i]) {
                temp = temp->next;
                i++;
            }
            if (sc->query[i] == '\0') {
                sc->results[index++] = current;
            }
        }
        current = current->next;
    }
}

/* 검색 결과 하이라이트 함수 */
void highlightMatch(WINDOW *win, TextBuffer *tb, SearchContext *sc) {
    Node *current = sc->results[sc->current_index];
    int query_len = strlen(sc->query);

    // 현재 속성 저장
    attr_t attrs;
    short pair;
    wattr_get(win, &attrs, &pair, NULL);

    // 하이라이트 속성 적용
    wattron(win, A_REVERSE);

    int row = 0, col = 0;
    Node *temp = tb->head;
    while (temp != current && temp != NULL) {
        if (temp->character == '\n') {
            row++;
            col = 0;
        } else {
            col++;
        }
        temp = temp->next;
    }

    for (int i = 0; i < query_len && temp != NULL; i++) {
        mvwaddch(win, row, col, temp->character);
        if (temp->character == '\n') {
            row++;
            col = 0;
        } else {
            col++;
        }
        temp = temp->next;
    }

    // 이전 속성 복원
    wattroff(win, A_REVERSE);
    wattr_set(win, attrs, pair, NULL);

    wrefresh(win);
}

/* 언하이라이트 함수 */
void clearHighlight(WINDOW *win, TextBuffer *tb, SearchContext *sc) {
    // 전체 텍스트 버퍼를 다시 표시하여 하이라이트 제거
    displayList(win, tb, NULL);
}

/* 프롬프트 표시 함수 */
void displayPrompt(WINDOW *win, const char *prompt, char *buffer, int buffer_size) {
    int y, x;
    getmaxyx(win, y, x);
    mvwprintw(win, y - 1, 0, "%s", prompt);
    wclrtoeol(win); // 줄의 나머지 부분 지우기
    wrefresh(win);
    wgetnstr(win, buffer, buffer_size - 1);
}

/* 노드 위치 계산 함수 */ 
Cursor getCursorFromNode(TextBuffer *tb, Node *node) {
    Cursor cursor;
    cursor.current = node;
    cursor.row = 0;
    cursor.col = 0;

    Node *temp = tb->head;
    while (temp != node && temp != NULL) {
        if (temp->character == '\n') {
            cursor.row++;
            cursor.col = 0;
        } else {
            cursor.col++;
        }
        temp = temp->next;
    }
    return cursor;
}

/* 사용자 키 입력 처리 */
void processInput(WINDOW *win, TextBuffer *tb, Cursor *cursor) {
    int ch;
    while (1) {
        ch = getch();
    #ifdef _WIN32
        /* Windows에서 Ctrl 키 조합 처리 */
        if (ch == 19) { // Ctrl-S (저장)
            saveFile(tb);
        } else if (ch == 17) { // Ctrl-Q (종료)
            return;
        } else if (ch == 6) { // Ctrl-F (검색)
            searchFunction(win, tb, cursor);
        } else {
            /* 기존 입력 처리 */
            switch (ch) {
                case KEY_LEFT:
                    moveCursorLeft(cursor);
                    break;
                case KEY_RIGHT:
                    moveCursorRight(cursor);
                    break;
                case KEY_UP:
                    moveCursorUp(cursor);
                    break;
                case KEY_DOWN:
                    moveCursorDown(cursor);
                    break;
                case KEY_BACKSPACE:
                case 127:
                case 8:
                    /* 백스페이스 처리 */
                    deleteNode(tb, cursor);
                    if (cursor->col > 0)
                        cursor->col--;
                    else if (cursor->row > 0) {
                        cursor->row--;
                        // 이전 줄의 끝으로 이동
                        cursor->col = 0;
                        Node *temp = cursor->current;
                        while (temp != NULL && temp->character != '\n') {
                            temp = temp->prev;
                            cursor->col++;
                        }
                    }
                    break;
                default:
                    if (ch >= 32 && ch <= 126) { // 출력 가능한 문자
                        insertNode(tb, cursor, (char)ch);
                        cursor->col++;
                    } else if (ch == '\n' || ch == '\r') {
                        insertNode(tb, cursor, '\n');
                        cursor->col = 0;
                        cursor->row++;
                    }
                    break;
            }
        }
    #elif defined(__APPLE__)
        /* macOS에서 ESC 시퀀스 처리 */
        if (ch == 27) { // ESC 키를 눌렀을 때
            nodelay(win, TRUE); // 논블로킹 모드로 전환
            int next_ch = getch();
            nodelay(win, FALSE); // 블로킹 모드로 복원
            if (next_ch == ERR) {
                // ESC 키만 눌린 경우 계속 진행
                continue;
            } else {
                // ESC + 다른 키 조합 처리
                switch (next_ch) {
                    case 's':
                    case 'S':
                        // ESC + S 눌렀을 때 저장
                        saveFile(tb);
                        break;
                    case 'q':
                    case 'Q':
                        // ESC + Q 눌렀을 때 종료
                        return;
                    case 'f':
                    case 'F':
                        searchFunction(win, tb, cursor);
                        break;
                    default:
                        break;
                }
                continue;
            }
        } else {
            /* 기존 입력 처리 */
            switch (ch) {
                case KEY_LEFT:
                    moveCursorLeft(cursor);
                    break;
                case KEY_RIGHT:
                    moveCursorRight(cursor);
                    break;
                case KEY_UP:
                    moveCursorUp(cursor);
                    break;
                case KEY_DOWN:
                    moveCursorDown(cursor);
                    break;
                case KEY_BACKSPACE:
                case 127:
                    /* 백스페이스 처리 */
                    deleteNode(tb, cursor);
                    if (cursor->col > 0)
                        cursor->col--;
                    else if (cursor->row > 0) {
                        cursor->row--;
                        // 이전 줄의 끝으로 이동
                        cursor->col = 0;
                        Node *temp = cursor->current;
                        while (temp != NULL && temp->character != '\n') {
                            temp = temp->prev;
                            cursor->col++;
                        }
                    }
                    break;
                default:
                    if (ch >= 32 && ch <= 126) { // 출력 가능한 문자
                        insertNode(tb, cursor, (char)ch);
                        cursor->col++;
                    } else if (ch == '\n' || ch == '\r') {
                        insertNode(tb, cursor, '\n');
                        cursor->col = 0;
                        cursor->row++;
                    }
                    break;
            }
        }
    #else
        /* 기타 운영체제(Linux)에서 기본적으로 Ctrl 키 조합 사용 */
        if (ch == 19) { // Ctrl-S (저장)
            saveFile(tb);
        } else if (ch == 17) { // Ctrl-Q (종료)
            return;
        } else if (ch == 6) { // Ctrl-F (검색)
            searchFunction(win, tb, cursor);
        } else {
            /* 기존 입력 처리 */
            switch (ch) {
                case KEY_LEFT:
                    moveCursorLeft(cursor);
                    break;
                case KEY_RIGHT:
                    moveCursorRight(cursor);
                    break;
                case KEY_UP:
                    moveCursorUp(cursor);
                    break;
                case KEY_DOWN:
                    moveCursorDown(cursor);
                    break;
                case KEY_BACKSPACE:
                case 127:
                    /* 백스페이스 처리 */
                    deleteNode(tb, cursor);
                    if (cursor->col > 0)
                        cursor->col--;
                    else if (cursor->row > 0) {
                        cursor->row--;
                        // 이전 줄의 끝으로 이동
                        cursor->col = 0;
                        Node *temp = cursor->current;
                        while (temp != NULL && temp->character != '\n') {
                            temp = temp->prev;
                            cursor->col++;
                        }
                    }
                    break;
                default:
                    if (ch >= 32 && ch <= 126) { // 출력 가능한 문자
                        insertNode(tb, cursor, (char)ch);
                        cursor->col++;
                    } else if (ch == '\n' || ch == '\r') {
                        insertNode(tb, cursor, '\n');
                        cursor->col = 0;
                        cursor->row++;
                    }
                    break;
            }
        }
    #endif
        /* 화면 업데이트 */
        displayList(win, tb, cursor);
        move(cursor->row, cursor->col);
        refresh();
    }
}

/* 메모리 해제 함수 */
void freeResource(TextBuffer *tb) {
    Node *temp;
    while (tb->head != NULL) {
        temp = tb->head;
        tb->head = tb->head->next;
        free(temp);
    }

    if (tb->filename) {
        free(tb->filename);
    }
}

/* main */
int main(int argc, char *argv[]) {
    // ncurses 기본 세팅
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    TextBuffer tb = {NULL, NULL, 0, NULL};
    Cursor cursor = {NULL, 0, 0};

    if (argc > 1) {
        // 파일이 제공되었을 때
        loadFile(&tb, &cursor, argv[1]);
    }

    displayList(stdscr, &tb, &cursor);
    move(cursor.row, cursor.col);

    processInput(stdscr, &tb, &cursor);

    endwin();

    // 파일 저장
    if (tb.modified && tb.filename) {
        saveFile(&tb);
    }

    // 메모리 해제
    freeResource(&tb);

    return 0;
}