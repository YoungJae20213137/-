/*========================== 컴파일 방법 =============================================================================================================================================================================

1) 링커 위치
ncurses: c/Program Files/gcctemp/project/etc/PDCurses-master/curses.h
pdcurses: c/Program Files/gcctemp/project/etc/PDCurses-master/wincon/pdcurses.a

2) 콘솔 모드로 컴파일
gcc -o my_program main.c -I/c/Program\ Files/gcctemp/project/etc/PDCurses-master -L/c/Program\ Files/gcctemp/project/etc/PDCurses-master/wincon -l:pdcurses.a

3) gui 모드로 컴파일
gcc -o my_program main.c -I/c/Program\ Files/gcctemp/project/etc/PDCursesMod-master -L/c/Program\ Files/gcctemp/project/etc/PDCursesMod-master/wingui -l:libpdcurses.a -lwinmm -lgdi32 -lcomdlg32

===================================================================================================================================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>

#define MAX_LINES 1024
#define MAX_LINE_LENGTH 256

// 자료구조 정의
typedef struct Node {
    char character;         // 저장된 문자
    struct Node *prev;      // 이전 노드에 대한 포인터
    struct Node *next;      // 다음 노드에 대한 포인터
    int row;                // 노드의 행 위치
    int col;                // 노드의 열 위치
} Node;

typedef struct TextBuffer {
    Node *head;             // 리스트의 첫 번째 노드
    Node *tail;             // 리스트의 마지막 노드
    Node *cursor;           // 현재 커서 위치
    int modified;           // 파일이 수정되었는지를 나타내는 플래그
    char filename[256];     // 파일 이름 저장
    int num_lines;          // 총 줄 수
    char *lines[MAX_LINES]; // 각 줄 관리 배열
    int cursor_row;         // 커서 위치 (행)
    int cursol_col;         // 커서 위치 (열)
} TextBuffer;

typedef struct SearchResult {
    Node *node;                 // 찾은 문자가 있는 노드
    struct SearchResult *next;  // 다음 결과 노드
} SearchResult;

typedef struct SearchState {
    SearchResult *head;         // 탐색 결과의 첫 번째 노드
    SearchResult *current;      // 현재 선택된 탐색 결과
} SearchState;

// 노드 생성 및 초기화 함수
Node* create_node(char character) {
    Node *new_node = (Node*)malloc(sizeof(Node));
    new_node->character = character;  // 노드에 문자 저장
    new_node->prev = NULL;            // 이전 노드 포인터 초기화
    new_node->next = NULL;            // 다음 노드 포인터 초기화
    return new_node;                  // 새 노드 반환
}

// 텍스트 버퍼 생성 및 초기화 함수
TextBuffer* create_text_buffer(const char *filename) {
    TextBuffer *buffer = (TextBuffer*)malloc(sizeof(TextBuffer));
    buffer->head = NULL;
    buffer->tail = NULL;
    buffer->cursor = NULL;
    buffer->modified = 0;  // 수정되지 않은 상태로 초기화

    // 파일 이름 초기화
    if (filename) {
        strncpy(buffer->filename, filename, sizeof(buffer->filename) - 1);
        buffer->filename[sizeof(buffer->filename) - 1] = '\0';  // 널 종료 문자 추가
    } else {
        strcpy(buffer->filename, "[No Name]");
    }

    buffer->num_lines = 1;  // 기본 한 줄로 초기화
    buffer->cursor_row = 0;
    buffer->cursol_col = 0;

    // 각 줄을 관리할 배열 초기화
    for (int i = 0; i < MAX_LINES; i++) {
        buffer->lines[i] = NULL;
    }

    // 첫 줄 메모리 할당 및 초기화
    buffer->lines[0] = malloc(MAX_LINE_LENGTH * sizeof(char));
    if (buffer->lines[0]) {
        buffer->lines[0][0] = '\0';  // 첫 줄을 빈 문자열로 초기화
    }

    return buffer;  // 초기화된 텍스트 버퍼 반환
}

// 상태 바 함수
void update_status_bar(TextBuffer *buffer) {
    // 상태 바 왼쪽에 표시할 파일 이름과 총 줄 수
    char status[256];
    snprintf(status, sizeof(status), "%s%s - %d lines",
             buffer->filename[0] ? buffer->filename : "[No Name]",
             buffer->modified ? " (modified)" : "", buffer->num_lines);

    // 상태 바 오른쪽 끝에 표시할 커서 위치
    char cursor_position[32];
    snprintf(cursor_position, sizeof(cursor_position), "Cursor: (%d, %d)", buffer->cursor_row + 1, buffer->cursol_col + 1);

    // 상태 바 표시
    move(LINES - 3, 0);  // 상태 바는 화면 하단에서 두 번째 줄에 표시
    attron(A_REVERSE);   // 반전 효과 적용

    // 왼쪽 상태 바 텍스트 출력
    printw("%s", status);

    // 오른쪽 끝에 커서 위치 출력
    int cursor_position_col = COLS - strlen(cursor_position);  // 오른쪽 끝 위치 계산
    mvprintw(LINES - 3, cursor_position_col, "%s", cursor_position);

    attroff(A_REVERSE);  // 반전 효과 해제
    refresh();
}

// 메시지 바 함수
void update_message_bar(const char *message) {
    move(LINES - 1, 0);
    clrtoeol();

    if (message) {
        printw("%s", message);
    } else {
        printw("Save: Ctrl+S | Search: Ctrl+F | Exit: Ctrl+Q");
    }

    refresh();
}

// main 함수 구현
int main(int argc, char *argv[]) {
    // ncurses 초기화
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(1);

    // TextBuffer 생성 및 초기화
    const char *filename = (argc > 1) ? argv[1] : "[No Name]";
    TextBuffer *buffer = create_text_buffer(filename);

    // 기본 상태 바 및 메시지 바 초기화
    update_status_bar(buffer);
    update_message_bar(NULL);

    int ch;
    while ((ch = getch()) != 17) {  // Ctrl+Q로 종료 (ASCII 17)
        // 커서 이동 및 텍스트 수정 예제
        if (ch == KEY_DOWN && buffer->cursor_row < buffer->num_lines - 1) {
            buffer->cursor_row++;
        } else if (ch == KEY_UP && buffer->cursor_row > 0) {
            buffer->cursor_row--;
        } else if (ch == KEY_LEFT && buffer->cursol_col > 0) {
            buffer->cursol_col--;
        } else if (ch == KEY_RIGHT && buffer->cursol_col < strlen(buffer->lines[buffer->cursor_row])) {
            buffer->cursol_col++;
        }

        // 상태 바 업데이트
        update_status_bar(buffer);
    }

    // 종료 전 메모리 해제
    for (int i = 0; i < buffer->num_lines; i++) {
        free(buffer->lines[i]);
    }
    free(buffer);

    endwin();
    return 0;
}
