#include <curses.h>
#include <stdlib.h>
#include <string.h>

/* 단축키 정의 */
#ifdef _WIN32
#define SAVE_KEY    "Ctrl-S"
#define QUIT_KEY    "Ctrl-Q"
#define FIND_KEY    "Ctrl-F"
#elif defined(__APPLE__)
#define SAVE_KEY    "ESC+s"
#define QUIT_KEY    "ESC+q"
#define FIND_KEY    "ESC+f"
#else
#define SAVE_KEY    "Ctrl-S"
#define QUIT_KEY    "Ctrl-Q"
#define FIND_KEY    "Ctrl-F"
#endif

typedef struct Node {
    char character;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    int modified;
    char *filename;
} TextBuffer;

typedef struct {
    Node *current;
    int row;
    int col;
} Cursor;

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
        // 빈 리스트
        tb->head = newNode;
        tb->tail = newNode;
        cursor->current = newNode;
    } else if (cursor->current == NULL) {
        // 시작 부분에 삽입
        newNode->next = tb->head;
        tb->head->prev = newNode;
        tb->head = newNode;
        cursor->current = newNode;
    } else {
        // 현재 위치 뒤에 삽입
        newNode->prev = cursor->current;
        newNode->next = cursor->current->next;
        if (cursor->current->next != NULL) {
            cursor->current->next->prev = newNode;
        } else {
            // 마지막 노드인 경우
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
    Node *nodeToDelete = cursor->current;

    if (nodeToDelete->prev != NULL) {
        nodeToDelete->prev->next = nodeToDelete->next;
        cursor->current = nodeToDelete->prev;
    } else {
        // 헤드 노드 삭제
        tb->head = nodeToDelete->next;
        cursor->current = tb->head;
    }

    if (nodeToDelete->next != NULL) {
        nodeToDelete->next->prev = nodeToDelete->prev;
    } else {
        // 테일 노드 삭제
        tb->tail = nodeToDelete->prev;
    }

    free(nodeToDelete);
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
    int status_row = LINES - 2;
    char status[COLS];
    int total_lines = countLines(tb);
    snprintf(status, COLS, " [%s] - %d lines | Cursor: (%d:%d) ",
             tb->filename ? tb->filename : "No Name", total_lines, cursor->row + 1, cursor->col + 1);

    wattron(win, A_REVERSE);
    mvwprintw(win, status_row, 0, "%-*s", COLS - 1, status);
    wattroff(win, A_REVERSE);
    wrefresh(win);
}

/* 메시지 바 표시 함수 */
void displayMessageBar(WINDOW *win) {
    int msg_row = LINES - 1;
    char message[COLS];
    snprintf(message, COLS, "HELP: %s = save | %s = quit | %s = find",
             SAVE_KEY, QUIT_KEY, FIND_KEY);
    mvwprintw(win, msg_row, 0, "%-*s", COLS - 1, message);
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

/* 입력 처리 함수 */
void processInput(WINDOW *win, TextBuffer *tb, Cursor *cursor) {
    int ch;
    if (test_mode) {
        /* 테스트 모드에서 자동으로 ESC+q 입력 시뮬레이션 */
        napms(1000); // 1초 대기
        ch = 27; // ESC 키
        goto esc_sequence; // ESC 시퀀스 처리로 이동
    }

    while (1) {
        ch = getch();
        if (ch == 27) {
        esc_sequence:
            nodelay(win, TRUE); // 논블로킹 모드로 전환
            int next_ch;
            if (test_mode) {
                next_ch = 'q';
            } else {
                next_ch = getch();
            }
            nodelay(win, FALSE); // 블로킹 모드로 복원
            if (next_ch == ERR) {
                // ESC 키만 눌린 경우 계속 진행
                continue;
            } else {
                // ESC + 다른 키 조합 처리
                switch (next_ch) {
                    case 's':
                    case 'S':
                        // ESC + s 눌렀을 때 저장
                        saveFile(tb);
                        break;
                    case 'q':
                    case 'Q':
                        // ESC + q 눌렀을 때 종료
                        return;
                    case 'f':
                    case 'F':
                        // ESC + f 눌렀을 때 검색 기능 (구현 필요)
                        // searchFunction(tb, cursor);
                        break;
                    default:
                        break;
                }
                continue;
            }
        }
        switch (ch) {
            case KEY_LEFT:
                if (cursor->current != NULL && cursor->current->prev != NULL) {
                    cursor->current = cursor->current->prev;
                    if (cursor->current->character == '\n') {
                        // 이전 줄의 끝으로 이동
                        Node *temp = cursor->current;
                        int x = 0;
                        while (temp->prev != NULL && temp->prev->character != '\n') {
                            temp = temp->prev;
                            x++;
                        }
                        cursor->col = x;
                        cursor->row--;
                    } else {
                        cursor->col--;
                    }
                }
                break;
            case KEY_RIGHT:
                if (cursor->current != NULL && cursor->current->next != NULL) {
                    cursor->current = cursor->current->next;
                    if (cursor->current->character == '\n') {
                        cursor->col = 0;
                        cursor->row++;
                    } else {
                        cursor->col++;
                    }
                }
                break;
            case KEY_BACKSPACE:
            case 127:
            case 8: // Windows에서 Backspace 키 코드 추가
                deleteNode(tb, cursor);
                if (cursor->col > 0)
                    cursor->col--;
                else if (cursor->row > 0) {
                    cursor->row--;
                    // 이전 줄의 끝으로 이동
                    Node *temp = cursor->current;
                    int x = 0;
                    while (temp != NULL && temp->character != '\n') {
                        temp = temp->prev;
                        x++;
                    }
                    cursor->col = x;
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
        displayList(win, tb, cursor);
        move(cursor->row, cursor->col);
        refresh();
    }
}


/* 메모리 해제 함수 */
void freeResources(TextBuffer *tb) {
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

/* 테스트 모드 변수 추가 */
int test_mode = 0;

int main(int argc, char *argv[]) {
    // 테스트 모드 확인
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        test_mode = 1;
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    TextBuffer tb = {NULL, NULL, 0, NULL};
    Cursor cursor = {NULL, 0, 0};

    if (argc > 1 && strcmp(argv[1], "--test") != 0) {
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
    freeResources(&tb);

    return 0;
}